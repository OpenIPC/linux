#ifndef _DCE_REGISTER_H_
#define _DCE_REGISTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__LINUX)
#include "mach/rcw_macro.h"
#include "plat/top.h"
#include "kwrap/type.h"
#elif defined (__FREERTOS)
#include "rcw_macro.h"
#include "plat/top.h"
#include "kwrap/type.h"
#else

#endif


//---------- for physical registers ------------
/*
    dce_swrst      :    [0x0, 0x1],         bits : 0
    dce_start      :    [0x0, 0x1],         bits : 1
    dce_load_start :    [0x0, 0x1],         bits : 2
    dce_load_fd    :    [0x0, 0x1],         bits : 3
    dce_load_fs    :    [0x0, 0x1],         bits : 4
    ll_fire        :    [0x0, 0x1],         bits : 28
*/
#define DCE_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(DCE_CONTROL_REGISTER)
REGDEF_BIT(dce_swrst,           1)
REGDEF_BIT(dce_start,           1)
REGDEF_BIT(dce_load_start,      1)
REGDEF_BIT(dce_load_fd,         1)
REGDEF_BIT(dce_load_fs,         1)
REGDEF_BIT(dce_load_global,     1)
REGDEF_BIT(,                       22)
REGDEF_BIT(ll_fire,             1)
REGDEF_END(DCE_CONTROL_REGISTER)


/*
    dce_op         :    [0x0, 0x3],         bits : 1_0
    dce_stp        :    [0x0, 0x1],         bits : 2
    sram_mode      :    [0x0, 0x1],         bits : 3
    cfa_en         :    [0x0, 0x1],         bits : 4
    dc_en          :    [0x0, 0x1],         bits : 5
    cac_en         :    [0x0, 0x1],         bits : 6
    yuv2rgb_en     :    [0x0, 0x1],         bits : 7
    tcurve_en      :    [0x0, 0x1],         bits : 8
    wdr_en         :    [0x0, 0x1],         bits : 9
    wdr_subimg_out_en:	[0x0, 0x1],         bits : 10
    cfa_subimg_out_en:	[0x0, 0x1],         bits : 11
    cfa_subimg_out_flip_en:	[0x0, 0x1],     bits : 12
    cfa_pinkr_en   :    [0x0, 0x1],         bits : 13
    histogram_en   :    [0x0, 0x1],         bits : 14
    histogram_sel  :    [0x0, 0x1],         bits : 15
    yuv2rgbfmt     :    [0x0, 0x3],         bits : 17_16
    crop_en        :    [0x0, 0x1],         bits : 18
    dc_sel         :    [0x0, 0x1],         bits : 19
    chksum_en      :    [0x0, 0x1],         bits : 20
    d2drand        :    [0x0, 0x1],         bits : 21
    d2drand_rst    :    [0x0, 0x1],         bits : 22
    fstr_rst_disable  :	[0x0, 0x1],         bits : 23
    d2dfmt         :    [0x0, 0x3],         bits : 25_24
    d2d_uv_out_disable: [0x0, 0x1],			bits : 26
    d2duvfilt      :    [0x0, 0x1],         bits : 27
    d2diostop      :    [0x0, 0x1],         bits : 28
    debug_sel      :    [0x0, 0x7],         bits : 31_29
*/
#define DCE_FUNCTION_REGISTER_OFS 0x0004
REGDEF_BEGIN(DCE_FUNCTION_REGISTER)
REGDEF_BIT(dce_op,              2)
REGDEF_BIT(dce_stp,             1)
REGDEF_BIT(sram_mode,           1)
REGDEF_BIT(cfa_en,              1)
REGDEF_BIT(dc_en,				1)
REGDEF_BIT(cac_en,				1)
REGDEF_BIT(yuv2rgb_en,			1)
REGDEF_BIT(tcurve_en,			1)
REGDEF_BIT(wdr_en,				1)
REGDEF_BIT(wdr_subimg_out_en,	1)
REGDEF_BIT(cfa_subimg_out_en,	1)
REGDEF_BIT(cfa_subimg_out_flip_en,1)
REGDEF_BIT(cfa_pinkr_en,		1)
REGDEF_BIT(histogram_en,		1)
REGDEF_BIT(histogram_sel,		1)
REGDEF_BIT(yuv2rgbfmt,			2)
REGDEF_BIT(crop_en,				1)
REGDEF_BIT(dc_sel,				1)
REGDEF_BIT(chksum_en,			1)
REGDEF_BIT(d2drand,				1)
REGDEF_BIT(d2drand_rst,			1)
REGDEF_BIT(fstr_rst_disable,	1)
REGDEF_BIT(d2dfmt,				2)
REGDEF_BIT(d2d_uv_out_disable,  1)
REGDEF_BIT(d2duvfilt,			1)
REGDEF_BIT(d2diostop,			1)
REGDEF_BIT(debug_sel,			3)
REGDEF_END(DCE_FUNCTION_REGISTER)


/*
    inte_frmst         :    [0x0, 0x1],         bits : 0
    inte_frmend        :    [0x0, 0x1],         bits : 1
    inte_stend         :    [0x0, 0x1],         bits : 2
    inte_stp_err       :    [0x0, 0x1],         bits : 3
    inte_lb_ovf		   :    [0x0, 0x1],         bits : 4
    inte_stp_ob        :    [0x0, 0x1],         bits : 6
    inte_y_back        :    [0x0, 0x1],         bits : 7
    inte_llend         :    [0x0, 0x1],         bits : 8
    inte_llerror       :    [0x0, 0x1],         bits : 9
    inte_llerror2      :    [0x0, 0x1],         bits : 10
    inte_lljobend      :    [0x0, 0x1],         bits : 11
    inte_frame_err     :    [0x0, 0x1],         bits : 12
*/
#define DCE_INTERRUPT_ENABLE_REGISTER_OFS 0x0008
REGDEF_BEGIN(DCE_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(inte_frmst,           1)
REGDEF_BIT(inte_frmend,          1)
REGDEF_BIT(inte_stend,           1)
REGDEF_BIT(inte_stp_err,         1)
REGDEF_BIT(inte_lb_ovf, 		 1)
REGDEF_BIT(,						 1)
REGDEF_BIT(inte_stp_ob,          1)
REGDEF_BIT(inte_y_back,          1)
REGDEF_BIT(inte_llend,           1)
REGDEF_BIT(inte_llerror,         1)
REGDEF_BIT(inte_llerror2,        1)
REGDEF_BIT(inte_lljobend,        1)
REGDEF_BIT(inte_frame_err,       1)
REGDEF_END(DCE_INTERRUPT_ENABLE_REGISTER)


/*
    int_frmst         :    [0x0, 0x1],         bits : 0
    int_frmend        :    [0x0, 0x1],         bits : 1
    int_stend         :    [0x0, 0x1],         bits : 2
    int_stp_err       :    [0x0, 0x1],         bits : 3
    int_lb_ovf        :    [0x0, 0x1],         bits : 4
    int_stp_ob        :    [0x0, 0x1],         bits : 6
    int_y_back        :    [0x0, 0x1],         bits : 7
    int_llend         :    [0x0, 0x1],         bits : 8
    int_llerror       :    [0x0, 0x1],         bits : 9
    int_llerror2      :    [0x0, 0x1],         bits : 10
    int_lljobend      :    [0x0, 0x1],         bits : 11
    int_frame_err     :    [0x0, 0x1],         bits : 12
*/
#define DCE_INTERRUPT_STATUS_REGISTER_OFS 0x000C
REGDEF_BEGIN(DCE_INTERRUPT_STATUS_REGISTER)
REGDEF_BIT(int_frmst,           1)
REGDEF_BIT(int_frmend,          1)
REGDEF_BIT(int_stend,           1)
REGDEF_BIT(int_stp_err,         1)
REGDEF_BIT(int_lb_ovf, 		    1)
REGDEF_BIT(,						1)
REGDEF_BIT(int_stp_ob,          1)
REGDEF_BIT(int_y_back,          1)
REGDEF_BIT(int_llend,           1)
REGDEF_BIT(int_llerror,         1)
REGDEF_BIT(int_llerror2,        1)
REGDEF_BIT(int_lljobend,        1)
REGDEF_BIT(int_frame_err,       1)
REGDEF_END(DCE_INTERRUPT_STATUS_REGISTER)



/*
    dce_hsizein		  :    [0x0, 0x7ff],       bits : 12_2
    dce_vsizein 	  :    [0x0, 0xfff],       bits : 28_17
*/
#define DCE_INPUT_SIZE_REGISTER_OFS 0x0010
REGDEF_BEGIN(DCE_INPUT_SIZE_REGISTER)
REGDEF_BIT(,        				2)
REGDEF_BIT(dce_hsizein,        11)
REGDEF_BIT(,        				4)
REGDEF_BIT(dce_vsizein,        12)
REGDEF_END(DCE_INPUT_SIZE_REGISTER)


/*
    ll_terminate:    [0x0, 0x1],            bits : 0
*/
#define LINKED_LIST_MODE_REGISTER_OFS 0x0014
REGDEF_BEGIN(LINKED_LIST_MODE_REGISTER)
REGDEF_BIT(ll_terminate,        1)
REGDEF_END(LINKED_LIST_MODE_REGISTER)


/*
    dce_dram_out0_single_en:    [0x0, 0x1],         bits : 0
    dce_dram_out1_single_en:    [0x0, 0x1],         bits : 1
    dce_dram_out_mode    :      [0x0, 0x1],         bits : 4
*/
#define DMA_OUTPUT_CHANNEL_ENABLE_REGISTER_OFS 0x0018
REGDEF_BEGIN(DMA_OUTPUT_CHANNEL_ENABLE_REGISTER)
REGDEF_BIT(dce_dram_out0_single_en, 	   1)
REGDEF_BIT(dce_dram_out1_single_en, 	   1)
REGDEF_BIT(,								   2)
REGDEF_BIT(dce_dram_out_mode,              1)
REGDEF_END(DMA_OUTPUT_CHANNEL_ENABLE_REGISTER)


/*
    dce_dram_sai_ll:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define DRAM_LINKED_LIST_REGISTER_OFS 0x001c
REGDEF_BEGIN(DRAM_LINKED_LIST_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dce_dram_sai_ll,        30)
REGDEF_END(DRAM_LINKED_LIST_REGISTER)


/*
    dram_saiy:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_INPUT_Y_CHANNEL_REGISTER_OFS 0x0020
REGDEF_BEGIN(DMA_INPUT_Y_CHANNEL_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_saiy,              30)
REGDEF_END(DMA_INPUT_Y_CHANNEL_REGISTER)


/*
    dram_ofsiy:    [0x0, 0x3fffffff],            bits : 19_2
*/
#define DMA_INPUT_Y_CHANNEL_LINE_OFFSET_REGISTER_OFS 0x0024
REGDEF_BEGIN(DMA_INPUT_Y_CHANNEL_LINE_OFFSET_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_ofsiy,             18)
REGDEF_END(DMA_INPUT_Y_CHANNEL_LINE_OFFSET_REGISTER)


/*
    dram_saiuv:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define DMA_INPUT_UV_CHANNEL_REGISTER_OFS 0x0028
REGDEF_BEGIN(DMA_INPUT_UV_CHANNEL_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_saiuv,		       30)
REGDEF_END(DMA_INPUT_UV_CHANNEL_REGISTER)


/*
    dram_ofsiuv:    [0x0, 0x3ffff],            bits : 19_2
*/
#define DMA_INPUT_UV_CHANNEL_LINE_OFFSET_REGISTER_OFS 0x002c
REGDEF_BEGIN(DMA_INPUT_UV_CHANNEL_LINE_OFFSET_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_ofsiuv,    	       18)
REGDEF_END(DMA_INPUT_UV_CHANNEL_LINE_OFFSET_REGISTER)


/*
    dram_saoy:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_OUTPUT_Y_CHANNEL_REGISTER_OFS 0x0030
REGDEF_BEGIN(DMA_OUTPUT_Y_CHANNEL_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_saoy,              30)
REGDEF_END(DMA_OUTPUT_Y_CHANNEL_REGISTER)


/*
    dram_ofsoy:    [0x0, 0x3ffff],            bits : 19_2
*/
#define DMA_OUTPUT_Y_CHANNEL_LINE_OFFSET_REGISTER_OFS 0x0034
REGDEF_BEGIN(DMA_OUTPUT_Y_CHANNEL_LINE_OFFSET_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_ofsoy,             18)
REGDEF_END(DMA_OUTPUT_Y_CHANNEL_LINE_OFFSET_REGISTER)


/*
    dram_saouv:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define DMA_OUTPUT_UV_CHANNEL_REGISTER_OFS 0x0038
REGDEF_BEGIN(DMA_OUTPUT_UV_CHANNEL_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_saouv,             30)
REGDEF_END(DMA_OUTPUT_UV_CHANNEL_REGISTER)


/*
    dram_ofsouv:    [0x0, 0x3ffff],           bits : 19_2
*/
#define DMA_OUTPUT_UV_CHANNEL_LINE_OFFSET_REGISTER_OFS 0x003c
REGDEF_BEGIN(DMA_OUTPUT_UV_CHANNEL_LINE_OFFSET_REGISTER)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_ofsouv,            30)
REGDEF_END(DMA_OUTPUT_UV_CHANNEL_LINE_OFFSET_REGISTER)


/*
    dce_status:    [0x0, 0x1],          bits : 0
    hcnt     :    [0x0, 0x3ff],         bits : 13_4
*/
#define DCE_STATUS_REGISTER_OFS 0x0040
REGDEF_BEGIN(DCE_STATUS_REGISTER)
REGDEF_BIT(dce_status, 	   		    1)
REGDEF_BIT(,                            3)
REGDEF_BIT(hcnt,		           10)
REGDEF_END(DCE_STATUS_REGISTER)


/*
    hstp0:    [0x0, 0x3ff],         bits : 9_0
    hstp1:    [0x0, 0x3ff],         bits : 19_10
    hstp2:    [0x0, 0x3ff],         bits : 29_20
*/
#define DCE_HORIZONTAL_STRIPE_REGISTER0_OFS 0x0044
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_REGISTER0)
REGDEF_BIT(hstp0,		 		   10)
REGDEF_BIT(hstp1,		 		   10)
REGDEF_BIT(hstp2,		 		   10)
REGDEF_END(DCE_HORIZONTAL_STRIPE_REGISTER0)


/*
	hstp3:    [0x0, 0x3ff],         bits : 9_0
	hstp4:    [0x0, 0x3ff],         bits : 19_10
	hstp5:    [0x0, 0x3ff],         bits : 29_20
*/
#define DCE_HORIZONTAL_STRIPE_REGISTER1_OFS 0x0048
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_REGISTER1)
REGDEF_BIT(hstp3,		 		   10)
REGDEF_BIT(hstp4,		 		   10)
REGDEF_BIT(hstp5,		 		   10)
REGDEF_END(DCE_HORIZONTAL_STRIPE_REGISTER1)


/*
	hstp6:    [0x0, 0x3ff],         bits : 9_0
	hstp7:    [0x0, 0x3ff],         bits : 19_10
	hstp8:    [0x0, 0x3ff],         bits : 29_20
*/
#define DCE_HORIZONTAL_STRIPE_REGISTER2_OFS 0x004c
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_REGISTER2)
REGDEF_BIT(hstp6,		 		   10)
REGDEF_BIT(hstp7,		 		   10)
REGDEF_BIT(hstp8,		 		   10)
REGDEF_END(DCE_HORIZONTAL_STRIPE_REGISTER2)


/*
	hstp9:    [0x0, 0x3ff],         bits : 9_0
	hstp10:    [0x0, 0x3ff],         bits : 19_10
	hstp11:    [0x0, 0x3ff],         bits : 29_20
*/
#define DCE_HORIZONTAL_STRIPE_REGISTER3_OFS 0x0050
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_REGISTER3)
REGDEF_BIT(hstp9,		 		   10)
REGDEF_BIT(hstp10,		 		   10)
REGDEF_BIT(hstp11,		 		   10)
REGDEF_END(DCE_HORIZONTAL_STRIPE_REGISTER3)


/*
    hstp12:    [0x0, 0x3ff],            bits : 9_0
    hstp13:    [0x0, 0x3ff],            bits : 19_10
    hstp14:    [0x0, 0x3ff],            bits : 29_20
*/
#define DCE_HORIZONTAL_STRIPE_REGISTER4_OFS 0x0054
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_REGISTER4)
REGDEF_BIT(hstp12,        		   10)
REGDEF_BIT(hstp13,        		   10)
REGDEF_BIT(hstp14,        		   10)
REGDEF_END(DCE_HORIZONTAL_STRIPE_REGISTER4)


/*
    hstp15            :    [0x0, 0x3ff],            bits : 9_0
    lbuf_back_rsv_line:    [0x0, 0xf],          bits : 15_12
    hstp_vmaxdce      :    [0x0, 0xf],          bits : 19_16
*/
#define DCE_HORIZONTAL_STRIPE_REGISTER5_OFS 0x0058
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_REGISTER5)
REGDEF_BIT(hstp15,        		   10)
REGDEF_BIT(,        		   		    2)
REGDEF_BIT(lbuf_back_rsv_line,      4)
REGDEF_BIT(hstp_vmaxdec,        	4)
REGDEF_END(DCE_HORIZONTAL_STRIPE_REGISTER5)


/*
    hstp_maxinc      	:    [0x0, 0xff],           bits : 7_0
    hstp_maxdec      	:    [0x0, 0xff],           bits : 15_8
    hstp_ipeolap_sel	:    [0x0, 0x1],            bits : 18
    hstp_imeolap_sel	:    [0x0, 0x3],            bits : 21_20
    hstp_imeolap	    :    [0x0, 0xff],           bits : 31_24
*/
#define DCE_HORIZONTAL_STRIPE_REGISTER6_OFS 0x005c
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_REGISTER6)
REGDEF_BIT(hstp_maxinc,        8)
REGDEF_BIT(hstp_maxdec,        8)
REGDEF_BIT(,        2)
REGDEF_BIT(hstp_ipeolap_sel,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(hstp_imeolap_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(hstp_imeolap,        8)
REGDEF_END(DCE_HORIZONTAL_STRIPE_REGISTER6)


/*
    bufheight_g			:    [0x0, 0x3ff],         bits : 9_0
    bufheight_rgb		:    [0x0, 0x3ff],         bits : 21_12
    bufheight_pix		:    [0x0, 0xff],          bits : 31_24
*/
#define DCE_BUFFER_HEIGHT_STATUS_REGISTER_OFS 0x0060
REGDEF_BEGIN(DCE_BUFFER_HEIGHT_STATUS_REGISTER)
REGDEF_BIT(buf_height_g,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(buf_height_rgb,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(buf_height_pix,        8)
REGDEF_END(DCE_BUFFER_HEIGHT_STATUS_REGISTER)


/*
    hstp_stx			:    [0x0, 0x1fff],         bits : 12_0
    hstp_edx			:    [0x0, 0x1fff],         bits : 28_16
*/
#define DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_1_OFS 0x0064
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_1)
REGDEF_BIT(hstp_stx,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(hstp_edx,        13)
REGDEF_END(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_1)


/*
    hstp_clm_stx		:    [0x0, 0x1fff],         bits : 12_0
    hstp_clm_edx		:    [0x0, 0x1fff],         bits : 28_16
*/
#define DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_2_OFS 0x0068
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_2)
REGDEF_BIT(hstp_clm_stx,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(hstp_clm_edx,        13)
REGDEF_END(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_2)


/*
    hstp_buf_mode		:    [0x0, 0x3f],          bits : 5_0
*/
#define DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_3_OFS 0x006c
REGDEF_BEGIN(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_3)
REGDEF_BIT(hstp_buf_mode,        6)
REGDEF_END(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_3)


/*
    dma_in_burst		:    [0x0, 0x1],           bits : 0
    dma_out_burst:    [0x0, 0x1],           bits : 1
*/
#define DMA_BURST_LENGTH_REGISTER_OFS 0x0070
REGDEF_BEGIN(DMA_BURST_LENGTH_REGISTER)
REGDEF_BIT(dma_in_burst,        1)
REGDEF_BIT(dma_out_burst,        1)
REGDEF_END(DMA_BURST_LENGTH_REGISTER)


/*
    dma_ch_disable:    [0x0, 0x1],			bits : 0
    dma_ch_idle   :    [0x0, 0x1],			bits : 4
*/
#define DMA_DISABLE_REGISTER_OFS 0x0074
REGDEF_BEGIN(DMA_DISABLE_REGISTER)
    REGDEF_BIT(dma_ch_disable,        1)
    REGDEF_BIT(              ,        3)
    REGDEF_BIT(dma_ch_idle   ,        1)
REGDEF_END(DMA_DISABLE_REGISTER)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED1_OFS 0x0078
REGDEF_BEGIN(DCE_RESERVED1)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED1)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED2_OFS 0x007c
REGDEF_BEGIN(DCE_RESERVED2)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED2)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED3_OFS 0x0080
REGDEF_BEGIN(DCE_RESERVED3)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED3)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED4_OFS 0x0084
REGDEF_BEGIN(DCE_RESERVED4)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED4)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED5_OFS 0x0088
REGDEF_BEGIN(DCE_RESERVED5)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED5)


/*
    crop_hsize		:    [0x0, 0x7ff],            bits : 12_2
*/
#define OUTPUT_CROP_REGISTER_0_OFS 0x008c
REGDEF_BEGIN(OUTPUT_CROP_REGISTER_0)
REGDEF_BIT(,        2)
REGDEF_BIT(crop_hsize,        11)
REGDEF_END(OUTPUT_CROP_REGISTER_0)


/*
    crop_hstart:    [0x0, 0x1fff],          bits : 12_0
*/
#define OUTPUT_CROP_REGISTER_1_OFS 0x0090
REGDEF_BEGIN(OUTPUT_CROP_REGISTER_1)
REGDEF_BIT(crop_hstart,        13)
REGDEF_END(OUTPUT_CROP_REGISTER_1)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED8_OFS 0x0094
REGDEF_BEGIN(DCE_RESERVED8)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED8)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED9_OFS 0x0098
REGDEF_BEGIN(DCE_RESERVED9)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED9)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED10_OFS 0x009c
REGDEF_BEGIN(DCE_RESERVED10)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED10)


/*
    cfapat			:    [0x0, 0x7],            bits : 2_0
    bayer_format	:    [0x0, 0x1],            bits : 3
*/
#define COLOR_INTERPOLATION_REGISTER_0_OFS 0x00a0
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER_0)
REGDEF_BIT(cfapat,        		   3)
REGDEF_BIT(bayer_format,           1)
REGDEF_END(COLOR_INTERPOLATION_REGISTER_0)


/*
    cfa_subimg_dramsao :    [0x0, 0x3fffffff],          bits : 31_2
*/
#define DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_REGISTER_OFS 0x00a4
REGDEF_BEGIN(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(cfa_subimg_dramsao,    30)
REGDEF_END(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_REGISTER)


/*
    cfa_subimg_lofso:    [0x0, 0x3ffff],           bits : 19_2
    cfa_subimg_chsel:    [0x0, 0x3],           bits : 27_26
    cfa_subimg_bit :    [0x0, 0x3],           bits : 29_28
    cfa_subimg_byte :    [0x0, 0x1],           bits : 30
*/
#define DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER_OFS 0x00a8
REGDEF_BEGIN(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(cfa_subimg_lofso,        18)
REGDEF_BIT(,        6)
REGDEF_BIT(cfa_subimg_chsel,        2)
REGDEF_BIT(cfa_subimg_bit,        2)
REGDEF_BIT(cfa_subimg_byte,        1)
REGDEF_END(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER)


/*
    cfa_edge_dth :    [0x0, 0xfff],         bits : 11_0
    cfa_edge_dth2:    [0x0, 0xfff],         bits : 23_12
*/
#define COLOR_INTERPOLATION_REGISTER1_OFS 0x00ac
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER1)
REGDEF_BIT(cfa_edge_dth,        12)
REGDEF_BIT(cfa_edge_dth2,        12)
REGDEF_END(COLOR_INTERPOLATION_REGISTER1)


/*
    cfa_rbcth1:    [0x0, 0x3ff],            bits : 9_0
    cfa_rbcth2:    [0x0, 0x3ff],            bits : 25_16
    cfa_rbcen :    [0x0, 0x1],          bits : 31
*/
#define COLOR_INTERPOLATION_REGISTER2_OFS 0x00b0
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER2)
REGDEF_BIT(cfa_rbcth1,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(cfa_rbcth2,        10)
REGDEF_BIT(,        5)
REGDEF_BIT(cfa_rbcen,        1)
REGDEF_END(COLOR_INTERPOLATION_REGISTER2)


/*
    cfa_freqblend_lut0:    [0x0, 0xf],          bits : 3_0
    cfa_freqblend_lut1:    [0x0, 0xf],          bits : 7_4
    cfa_freqblend_lut2:    [0x0, 0xf],          bits : 11_8
    cfa_freqblend_lut3:    [0x0, 0xf],          bits : 15_12
    cfa_freqblend_lut4:    [0x0, 0xf],          bits : 19_16
    cfa_freqblend_lut5:    [0x0, 0xf],          bits : 23_20
    cfa_freqblend_lut6:    [0x0, 0xf],          bits : 27_24
    cfa_freqblend_lut7:    [0x0, 0xf],          bits : 31_28
*/
#define COLOR_INTERPOLATION_REGISTER3_OFS 0x00b4
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER3)
REGDEF_BIT(cfa_freqblend_lut0,        4)
REGDEF_BIT(cfa_freqblend_lut1,        4)
REGDEF_BIT(cfa_freqblend_lut2,        4)
REGDEF_BIT(cfa_freqblend_lut3,        4)
REGDEF_BIT(cfa_freqblend_lut4,        4)
REGDEF_BIT(cfa_freqblend_lut5,        4)
REGDEF_BIT(cfa_freqblend_lut6,        4)
REGDEF_BIT(cfa_freqblend_lut7,        4)
REGDEF_END(COLOR_INTERPOLATION_REGISTER3)


/*
    cfa_freqblend_lut8 :    [0x0, 0xf],         bits : 3_0
    cfa_freqblend_lut9 :    [0x0, 0xf],         bits : 7_4
    cfa_freqblend_lut10:    [0x0, 0xf],         bits : 11_8
    cfa_freqblend_lut11:    [0x0, 0xf],         bits : 15_12
    cfa_freqblend_lut12:    [0x0, 0xf],         bits : 19_16
    cfa_freqblend_lut13:    [0x0, 0xf],         bits : 23_20
    cfa_freqblend_lut14:    [0x0, 0xf],         bits : 27_24
    cfa_freqblend_lut15:    [0x0, 0xf],         bits : 31_28
*/
#define COLOR_INTERPOLATION_REGISTER4_OFS 0x00b8
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER4)
REGDEF_BIT(cfa_freqblend_lut8,        4)
REGDEF_BIT(cfa_freqblend_lut9,        4)
REGDEF_BIT(cfa_freqblend_lut10,        4)
REGDEF_BIT(cfa_freqblend_lut11,        4)
REGDEF_BIT(cfa_freqblend_lut12,        4)
REGDEF_BIT(cfa_freqblend_lut13,        4)
REGDEF_BIT(cfa_freqblend_lut14,        4)
REGDEF_BIT(cfa_freqblend_lut15,        4)
REGDEF_END(COLOR_INTERPOLATION_REGISTER4)


/*
    cfa_luma_weight0:    [0x0, 0xff],           bits : 7_0
    cfa_luma_weight1:    [0x0, 0xff],           bits : 15_8
    cfa_luma_weight2:    [0x0, 0xff],           bits : 23_16
    cfa_luma_weight3:    [0x0, 0xff],           bits : 31_24
*/
#define COLOR_INTERPOLATION_REGISTER5_OFS 0x00bc
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER5)
REGDEF_BIT(cfa_luma_weight0,        8)
REGDEF_BIT(cfa_luma_weight1,        8)
REGDEF_BIT(cfa_luma_weight2,        8)
REGDEF_BIT(cfa_luma_weight3,        8)
REGDEF_END(COLOR_INTERPOLATION_REGISTER5)


/*
    cfa_luma_weight4:    [0x0, 0xff],           bits : 7_0
    cfa_luma_weight5:    [0x0, 0xff],           bits : 15_8
    cfa_luma_weight6:    [0x0, 0xff],           bits : 23_16
    cfa_luma_weight7:    [0x0, 0xff],           bits : 31_24
*/
#define COLOR_INTERPOLATION_REGISTER6_OFS 0x00c0
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER6)
REGDEF_BIT(cfa_luma_weight4,        8)
REGDEF_BIT(cfa_luma_weight5,        8)
REGDEF_BIT(cfa_luma_weight6,        8)
REGDEF_BIT(cfa_luma_weight7,        8)
REGDEF_END(COLOR_INTERPOLATION_REGISTER6)


/*
    cfa_luma_weight8 :    [0x0, 0xff],          bits : 7_0
    cfa_luma_weight9 :    [0x0, 0xff],          bits : 15_8
    cfa_luma_weight10:    [0x0, 0xff],          bits : 23_16
    cfa_luma_weight11:    [0x0, 0xff],          bits : 31_24
*/
#define COLOR_INTERPOLATION_REGISTER7_OFS 0x00c4
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER7)
REGDEF_BIT(cfa_luma_weight8,        8)
REGDEF_BIT(cfa_luma_weight9,        8)
REGDEF_BIT(cfa_luma_weight10,        8)
REGDEF_BIT(cfa_luma_weight11,        8)
REGDEF_END(COLOR_INTERPOLATION_REGISTER7)


/*
    cfa_luma_weight12:    [0x0, 0xff],          bits : 7_0
    cfa_luma_weight13:    [0x0, 0xff],          bits : 15_8
    cfa_luma_weight14:    [0x0, 0xff],          bits : 23_16
    cfa_luma_weight15:    [0x0, 0xff],          bits : 31_24
*/
#define COLOR_INTERPOLATION_REGISTER8_OFS 0x00c8
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER8)
REGDEF_BIT(cfa_luma_weight12,        8)
REGDEF_BIT(cfa_luma_weight13,        8)
REGDEF_BIT(cfa_luma_weight14,        8)
REGDEF_BIT(cfa_luma_weight15,        8)
REGDEF_END(COLOR_INTERPOLATION_REGISTER8)


/*
    cfa_luma_weight16:    [0x0, 0xff],          bits : 7_0
    cfa_freq_th      :    [0x0, 0xfff],         bits : 27_16
*/
#define COLOR_INTERPOLATION_REGISTER9_OFS 0x00cc
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER9)
REGDEF_BIT(cfa_luma_weight16,        8)
REGDEF_BIT(,        8)
REGDEF_BIT(cfa_freq_th,        12)
REGDEF_END(COLOR_INTERPOLATION_REGISTER9)


/*
    cfa_fcs_coring:    [0x0, 0xff],         bits : 7_0
    cfa_fcs_weight:    [0x0, 0xff],         bits : 15_8
    cfa_fcs_dirsel:    [0x0, 0x1],          bits : 16
*/
#define COLOR_INTERPOLATION_REGISTER10_OFS 0x00d0
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER10)
REGDEF_BIT(cfa_fcs_coring,        8)
REGDEF_BIT(cfa_fcs_weight,        8)
REGDEF_BIT(cfa_fcs_dirsel,        1)
REGDEF_END(COLOR_INTERPOLATION_REGISTER10)


/*
    cfa_fcs_strength0:    [0x0, 0xf],           bits : 3_0
    cfa_fcs_strength1:    [0x0, 0xf],           bits : 7_4
    cfa_fcs_strength2:    [0x0, 0xf],           bits : 11_8
    cfa_fcs_strength3:    [0x0, 0xf],           bits : 15_12
    cfa_fcs_strength4:    [0x0, 0xf],           bits : 19_16
    cfa_fcs_strength5:    [0x0, 0xf],           bits : 23_20
    cfa_fcs_strength6:    [0x0, 0xf],           bits : 27_24
    cfa_fcs_strength7:    [0x0, 0xf],           bits : 31_28
*/
#define COLOR_INTERPOLATION_REGISTER11_OFS 0x00d4
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER11)
REGDEF_BIT(cfa_fcs_strength0,        4)
REGDEF_BIT(cfa_fcs_strength1,        4)
REGDEF_BIT(cfa_fcs_strength2,        4)
REGDEF_BIT(cfa_fcs_strength3,        4)
REGDEF_BIT(cfa_fcs_strength4,        4)
REGDEF_BIT(cfa_fcs_strength5,        4)
REGDEF_BIT(cfa_fcs_strength6,        4)
REGDEF_BIT(cfa_fcs_strength7,        4)
REGDEF_END(COLOR_INTERPOLATION_REGISTER11)


/*
    cfa_fcs_strength8 :    [0x0, 0xf],          bits : 3_0
    cfa_fcs_strength9 :    [0x0, 0xf],          bits : 7_4
    cfa_fcs_strength10:    [0x0, 0xf],          bits : 11_8
    cfa_fcs_strength11:    [0x0, 0xf],          bits : 15_12
    cfa_fcs_strength12:    [0x0, 0xf],          bits : 19_16
    cfa_fcs_strength13:    [0x0, 0xf],          bits : 23_20
    cfa_fcs_strength14:    [0x0, 0xf],          bits : 27_24
    cfa_fcs_strength15:    [0x0, 0xf],          bits : 31_28
*/
#define COLOR_INTERPOLATION_REGISTER12_OFS 0x00d8
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER12)
REGDEF_BIT(cfa_fcs_strength8,        4)
REGDEF_BIT(cfa_fcs_strength9,        4)
REGDEF_BIT(cfa_fcs_strength10,        4)
REGDEF_BIT(cfa_fcs_strength11,        4)
REGDEF_BIT(cfa_fcs_strength12,        4)
REGDEF_BIT(cfa_fcs_strength13,        4)
REGDEF_BIT(cfa_fcs_strength14,        4)
REGDEF_BIT(cfa_fcs_strength15,        4)
REGDEF_END(COLOR_INTERPOLATION_REGISTER12)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED11_OFS 0x00dc
REGDEF_BEGIN(DCE_RESERVED11)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED11)


/*
    cfa_rgbir_cl_en       :    [0x0, 0x1],          bits : 0
    cfa_rgbir_hf_en       :    [0x0, 0x1],          bits : 1
    cfa_rgbir_avg_mode    :    [0x0, 0x1],          bits : 2
    cfa_rgbir_cl_sel      :    [0x0, 0x1],          bits : 3
    cfa_rgbir_cl_thr      :    [0x0, 0xff],         bits : 15_8
    cfa_rgbir_gedge_th    :    [0x0, 0x3ff],            bits : 25_16
    cfa_rgbir_rb_cstrength:    [0x0, 0x7],          bits : 30_28
*/
#define RGBIR_COLOR_INTERPOLATION_REGISTER0_OFS 0x00e0
REGDEF_BEGIN(RGBIR_COLOR_INTERPOLATION_REGISTER0)
REGDEF_BIT(cfa_rgbir_cl_en,        1)
REGDEF_BIT(cfa_rgbir_hf_en,        1)
REGDEF_BIT(cfa_rgbir_avg_mode,        1)
REGDEF_BIT(cfa_rgbir_cl_sel,        1)
REGDEF_BIT(,        4)
REGDEF_BIT(cfa_rgbir_cl_thr,        8)
REGDEF_BIT(cfa_rgbir_gedge_th,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(cfa_rgbir_rb_cstrength,        3)
REGDEF_END(RGBIR_COLOR_INTERPOLATION_REGISTER0)


/*
    cfa_rgbir_hf_gthr:    [0x0, 0x7],           bits : 2_0
    cfa_rgbir_hf_diff:    [0x0, 0xff],          bits : 15_8
    cfa_rgbir_hf_ethr:    [0x0, 0xff],          bits : 23_16
*/
#define RGBIR_COLOR_INTERPOLATION_REGISTER1_OFS 0x00e4
REGDEF_BEGIN(RGBIR_COLOR_INTERPOLATION_REGISTER1)
REGDEF_BIT(cfa_rgbir_hf_gthr,        3)
REGDEF_BIT(,        5)
REGDEF_BIT(cfa_rgbir_hf_diff,        8)
REGDEF_BIT(cfa_rgbir_hf_ethr,        8)
REGDEF_END(RGBIR_COLOR_INTERPOLATION_REGISTER1)


/*
    irsub_r:    [0x0, 0xfff],           bits : 11_0
    irsub_g:    [0x0, 0xfff],           bits : 27_16
*/
#define RGBIR_COLOR_INTERPOLATION_REGISTER2_OFS 0x00e8
REGDEF_BEGIN(RGBIR_COLOR_INTERPOLATION_REGISTER2)
REGDEF_BIT(irsub_r,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(irsub_g,        12)
REGDEF_END(RGBIR_COLOR_INTERPOLATION_REGISTER2)


/*
    irsub_b           :    [0x0, 0xfff],            bits : 11_0
    irsub_weight_range:    [0x0, 0x3],          bits : 15_14
    irsub_weight_lb   :    [0x0, 0xff],         bits : 23_16
    irsub_th          :    [0x0, 0xff],         bits : 31_24
*/
#define RGBIR_COLOR_INTERPOLATION_REGISTER3_OFS 0x00ec
REGDEF_BEGIN(RGBIR_COLOR_INTERPOLATION_REGISTER3)
REGDEF_BIT(irsub_b,        12)
REGDEF_BIT(,        2)
REGDEF_BIT(irsub_weight_range,        2)
REGDEF_BIT(irsub_weight_lb,        8)
REGDEF_BIT(irsub_th,        8)
REGDEF_END(RGBIR_COLOR_INTERPOLATION_REGISTER3)


/*
    cgain_r:    [0x0, 0x3ff],           bits : 9_0
    cgain_g:    [0x0, 0x3ff],           bits : 25_16
*/
#define COLOR_GAIN_REGISTER_1_OFS 0x00f0
REGDEF_BEGIN(COLOR_GAIN_REGISTER_1)
REGDEF_BIT(cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(cgain_g,        10)
REGDEF_END(COLOR_GAIN_REGISTER_1)


/*
    cgain_b    :    [0x0, 0x3ff],           bits : 9_0
    cgain_range:    [0x0, 0x1],         bits : 16
*/
#define COLOR_GAIN_REGISTER_2_OFS 0x00f4
REGDEF_BEGIN(COLOR_GAIN_REGISTER_2)
REGDEF_BIT(cgain_b,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(cgain_range,        1)
REGDEF_END(COLOR_GAIN_REGISTER_2)


/*
    cfa_sta_gain:    [0x0, 0x3ff],          bits : 9_0
    pinkr_mode  :    [0x0, 0x1],            bits : 16
*/
#define CFA_SATURATION_GAIN_OFS 0x00f8
REGDEF_BEGIN(CFA_SATURATION_GAIN)
REGDEF_BIT(cfa_sta_gain,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(pinkr_mode,        1)
REGDEF_END(CFA_SATURATION_GAIN)


/*
    pinkr_thr1:    [0x0, 0xff],         bits : 7_0
    pinkr_thr2:    [0x0, 0xff],         bits : 15_8
    pinkr_thr3:    [0x0, 0xff],         bits : 23_16
    pinkr_thr4:    [0x0, 0xff],         bits : 31_24
*/
#define PINK_REDUCTION_REGISTER_OFS 0x00fc
REGDEF_BEGIN(PINK_REDUCTION_REGISTER)
REGDEF_BIT(pinkr_thr1,        8)
REGDEF_BIT(pinkr_thr2,        8)
REGDEF_BIT(pinkr_thr3,        8)
REGDEF_BIT(pinkr_thr4,        8)
REGDEF_END(PINK_REDUCTION_REGISTER)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED12_OFS 0x0100
REGDEF_BEGIN(DCE_RESERVED12)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED12)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED13_OFS 0x0104
REGDEF_BEGIN(DCE_RESERVED13)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED13)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED14_OFS 0x0108
REGDEF_BEGIN(DCE_RESERVED14)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED14)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED15_OFS 0x010c
REGDEF_BEGIN(DCE_RESERVED15)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED15)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED16_OFS 0x0110
REGDEF_BEGIN(DCE_RESERVED16)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED16)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED17_OFS 0x0114
REGDEF_BEGIN(DCE_RESERVED17)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED17)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED18_OFS 0x0118
REGDEF_BEGIN(DCE_RESERVED18)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED18)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED19_OFS 0x011c
REGDEF_BEGIN(DCE_RESERVED19)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED19)


/*
    wdr_subimg_width :    [0x0, 0x1f],         bits : 4_0
    wdr_subimg_height :    [0x0, 0x1f],         bits : 9_5
    wdr_rand_reset    :    [0x0, 0x1],          bits : 10
    wdr_rand_sel      :    [0x0, 0x3],          bits : 12_11
*/
#define WDR_SUBIMAGE_REGISTER_0_OFS 0x0120
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_0)
REGDEF_BIT(wdr_subimg_width,        5)
REGDEF_BIT(wdr_subimg_height,        5)
REGDEF_BIT(wdr_rand_reset,        1)
REGDEF_BIT(wdr_rand_sel,        2)
REGDEF_END(WDR_SUBIMAGE_REGISTER_0)


/*
    wdr_subimg_dramsai :    [0x0, 0x3fffffff],          bits : 31_2
*/
#define WDR_SUBIMAGE_REGISTER_1_OFS 0x0124
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_1)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_subimg_dramsai,        30)
REGDEF_END(WDR_SUBIMAGE_REGISTER_1)


/*
    wdr_subimg_lofsi :    [0x0, 0x3fff],            bits : 15_2
*/
#define WDR_SUBIMAGE_REGISTER_2_OFS 0x0128
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_2)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_subimg_lofsi,        14)
REGDEF_END(WDR_SUBIMAGE_REGISTER_2)


/*
    wdr_subimg_dramsao :    [0x0, 0x3fffffff],          bits : 31_2
*/
#define WDR_SUBIMAGE_REGISTER_3_OFS 0x012c
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_3)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_subimg_dramsao,        30)
REGDEF_END(WDR_SUBIMAGE_REGISTER_3)


/*
    wdr_subimg_lofso :    [0x0, 0x3fff],            bits : 15_2
*/
#define WDR_SUBIMAGE_REGISTER_4_OFS 0x0130
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_4)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_subimg_lofso,        14)
REGDEF_END(WDR_SUBIMAGE_REGISTER_4)


/*
    wdr_subimg_hfactor:    [0x0, 0xffff],           bits : 15_0
    wdr_subimg_vfactor:    [0x0, 0xffff],           bits : 31_16
*/
#define WDR_SUBIMAGE_REGISTER_5_OFS 0x0134
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_5)
REGDEF_BIT(wdr_subimg_hfactor,        16)
REGDEF_BIT(wdr_subimg_vfactor,        16)
REGDEF_END(WDR_SUBIMAGE_REGISTER_5)


/*
    wdr_sub_blk_sizeh    :    [0x0, 0x3ff],         bits : 9_0
    wdr_blk_cent_hfactor :    [0x0, 0x3fffff],          bits : 31_10
*/
#define WDR_SUBIMAGE_REGISTER_6_OFS 0x0138
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_6)
REGDEF_BIT(wdr_sub_blk_sizeh,        10)
REGDEF_BIT(wdr_blk_cent_hfactor,        22)
REGDEF_END(WDR_SUBIMAGE_REGISTER_6)


/*
    wdr_sub_blk_sizev    :    [0x0, 0x3ff],         bits : 9_0
    wdr_blk_cent_vfactor :    [0x0, 0x3fffff],          bits : 31_10
*/
#define WDR_SUBIMAGE_REGISTER_7_OFS 0x013c
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_7)
REGDEF_BIT(wdr_sub_blk_sizev,        10)
REGDEF_BIT(wdr_blk_cent_vfactor,        22)
REGDEF_END(WDR_SUBIMAGE_REGISTER_7)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED20_OFS 0x0140
REGDEF_BEGIN(DCE_RESERVED20)
REGDEF_BIT(reserved,        32)
REGDEF_END(DCE_RESERVED20)


/*
    wdr_input_bldrto0 :    [0x0, 0xff],         bits : 7_0
    wdr_input_bldrto1 :    [0x0, 0xff],         bits : 15_8
    wdr_input_bldrto2 :    [0x0, 0xff],         bits : 23_16
    wdr_input_bldrto3 :    [0x0, 0xff],         bits : 31_24
*/
#define WDR_INPUT_BLENDING_REGISTER0_OFS 0x0144
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER0)
REGDEF_BIT(wdr_input_bldrto0,        8)
REGDEF_BIT(wdr_input_bldrto1,        8)
REGDEF_BIT(wdr_input_bldrto2,        8)
REGDEF_BIT(wdr_input_bldrto3,        8)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER0)


/*
    wdr_input_bldrto4  :    [0x0, 0xff],            bits : 7_0
    wdr_input_bldrto5  :    [0x0, 0xff],            bits : 15_8
    wdr_input_bldrto6  :    [0x0, 0xff],            bits : 23_16
    wdr_input_bldrto7  :    [0x0, 0xff],            bits : 31_24
*/
#define WDR_INPUT_BLENDING_REGISTER1_OFS 0x0148
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER1)
REGDEF_BIT(wdr_input_bldrto4,        8)
REGDEF_BIT(wdr_input_bldrto5,        8)
REGDEF_BIT(wdr_input_bldrto6,        8)
REGDEF_BIT(wdr_input_bldrto7,        8)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER1)


/*
    wdr_input_bldrto8  :    [0x0, 0xff],            bits : 7_0
    wdr_input_bldrto9  :    [0x0, 0xff],            bits : 15_8
    wdr_input_bldrto10 :    [0x0, 0xff],            bits : 23_16
    wdr_input_bldrto11 :    [0x0, 0xff],            bits : 31_24
*/
#define WDR_INPUT_BLENDING_REGISTER2_OFS 0x014c
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER2)
REGDEF_BIT(wdr_input_bldrto8,        8)
REGDEF_BIT(wdr_input_bldrto9,        8)
REGDEF_BIT(wdr_input_bldrto10,        8)
REGDEF_BIT(wdr_input_bldrto11,        8)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER2)


/*
    wdr_input_bldrto12  :    [0x0, 0xff],           bits : 7_0
    wdr_input_bldrto13  :    [0x0, 0xff],           bits : 15_8
    wdr_input_bldrto14  :    [0x0, 0xff],           bits : 23_16
    wdr_input_bldrto15  :    [0x0, 0xff],           bits : 31_24
*/
#define WDR_INPUT_BLENDING_REGISTER3_OFS 0x0150
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER3)
REGDEF_BIT(wdr_input_bldrto12,        8)
REGDEF_BIT(wdr_input_bldrto13,        8)
REGDEF_BIT(wdr_input_bldrto14,        8)
REGDEF_BIT(wdr_input_bldrto15,        8)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER3)


/*
    wdr_input_bldrto16   :    [0x0, 0xff],          bits : 7_0
    wdr_input_bldsrc_sel :    [0x0, 0x3],           bits : 27_26
    wdr_input_bldwt      :    [0x0, 0xf],           bits : 31_28
*/
#define WDR_INPUT_BLENDING_REGISTER4_OFS 0x0154
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER4)
REGDEF_BIT(wdr_input_bldrto16,        8)
REGDEF_BIT(,        18)
REGDEF_BIT(wdr_input_bldsrc_sel,        2)
REGDEF_BIT(wdr_input_bldwt,        4)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER4)


/*
    wdr_lpf_c0:    [0x0, 0x7],         bits : 2_0
    wdr_lpf_c1 :    [0x0, 0x7],         bits : 5_3
    wdr_lpf_c2 :    [0x0, 0x7],         bits : 8_6
*/
#define WDR_SUBIMG_LPF_REGISTER_OFS 0x0158
REGDEF_BEGIN(WDR_SUBIMG_LPF_REGISTER)
REGDEF_BIT(wdr_lpf_c0,        3)
REGDEF_BIT(wdr_lpf_c1,        3)
REGDEF_BIT(wdr_lpf_c2,        3)
REGDEF_END(WDR_SUBIMG_LPF_REGISTER)


/*
    wdr_coeff1:    [0x0, 0x1fff],           bits : 12_0
    wdr_coeff2:    [0x0, 0x1fff],           bits : 28_16
*/
#define WDR_PARAMETER_REGISTER0_OFS 0x015c
REGDEF_BEGIN(WDR_PARAMETER_REGISTER0)
REGDEF_BIT(wdr_coeff1,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(wdr_coeff2,        13)
REGDEF_END(WDR_PARAMETER_REGISTER0)


/*
    wdr_coeff3:    [0x0, 0x1fff],           bits : 12_0
    wdr_coeff4:    [0x0, 0x1fff],           bits : 28_16
*/
#define WDR_PARAMETER_REGISTER1_OFS 0x0160
REGDEF_BEGIN(WDR_PARAMETER_REGISTER1)
REGDEF_BIT(wdr_coeff3,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(wdr_coeff4,        13)
REGDEF_END(WDR_PARAMETER_REGISTER1)


/*
    wdr_outbld_table_en:	[0x0, 0x1],            bits : 0
    wdr_gainctrl_en:		[0x0, 0x1],            bits : 1
    wdr_maxgain       :    	[0x0, 0xff],           bits : 23_16
    wdr_mingain       :    	[0x0, 0xff],           bits : 31_24
*/
#define WDR_CONTROL_REGISTER_OFS 0x0164
REGDEF_BEGIN(WDR_CONTROL_REGISTER)
REGDEF_BIT(wdr_outbld_table_en,        1)
REGDEF_BIT(wdr_gainctrl_en,        1)
REGDEF_BIT(,        14)
REGDEF_BIT(wdr_maxgain,        8)
REGDEF_BIT(wdr_mingain,        8)
REGDEF_END(WDR_CONTROL_REGISTER)


/*
    wdr_tcurve_index_lut0:    [0x0, 0x3f],          bits : 5_0
    wdr_tcurve_index_lut1:    [0x0, 0x3f],          bits : 13_8
    wdr_tcurve_index_lut2:    [0x0, 0x3f],          bits : 21_16
    wdr_tcurve_index_lut3:    [0x0, 0x3f],          bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER0_OFS 0x0168
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER0)
REGDEF_BIT(wdr_tcurve_index_lut0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut3,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER0)


/*
    wdr_tcurve_index_lut4:    [0x0, 0x3f],          bits : 5_0
    wdr_tcurve_index_lut5:    [0x0, 0x3f],          bits : 13_8
    wdr_tcurve_index_lut6:    [0x0, 0x3f],          bits : 21_16
    wdr_tcurve_index_lut7:    [0x0, 0x3f],          bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER1_OFS 0x016c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER1)
REGDEF_BIT(wdr_tcurve_index_lut4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut7,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER1)


/*
    wdr_tcurve_index_lut8 :    [0x0, 0x3f],         bits : 5_0
    wdr_tcurve_index_lut9 :    [0x0, 0x3f],         bits : 13_8
    wdr_tcurve_index_lut10:    [0x0, 0x3f],         bits : 21_16
    wdr_tcurve_index_lut11:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER2_OFS 0x0170
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER2)
REGDEF_BIT(wdr_tcurve_index_lut8,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut9,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut10,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut11,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER2)


/*
    wdr_tcurve_index_lut12:    [0x0, 0x3f],         bits : 5_0
    wdr_tcurve_index_lut13:    [0x0, 0x3f],         bits : 13_8
    wdr_tcurve_index_lut14:    [0x0, 0x3f],         bits : 21_16
    wdr_tcurve_index_lut15:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER3_OFS 0x0174
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER3)
REGDEF_BIT(wdr_tcurve_index_lut12,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut13,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut14,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut15,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER3)


/*
    wdr_tcurve_index_lut16:    [0x0, 0x3f],         bits : 5_0
    wdr_tcurve_index_lut17:    [0x0, 0x3f],         bits : 13_8
    wdr_tcurve_index_lut18:    [0x0, 0x3f],         bits : 21_16
    wdr_tcurve_index_lut19:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER4_OFS 0x0178
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER4)
REGDEF_BIT(wdr_tcurve_index_lut16,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut17,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut18,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut19,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER4)


/*
    wdr_tcurve_index_lut20:    [0x0, 0x3f],         bits : 5_0
    wdr_tcurve_index_lut21:    [0x0, 0x3f],         bits : 13_8
    wdr_tcurve_index_lut22:    [0x0, 0x3f],         bits : 21_16
    wdr_tcurve_index_lut23:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER5_OFS 0x017c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER5)
REGDEF_BIT(wdr_tcurve_index_lut20,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut21,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut22,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut23,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER5)


/*
    wdr_tcurve_index_lut24:    [0x0, 0x3f],         bits : 5_0
    wdr_tcurve_index_lut25:    [0x0, 0x3f],         bits : 13_8
    wdr_tcurve_index_lut26:    [0x0, 0x3f],         bits : 21_16
    wdr_tcurve_index_lut27:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER6_OFS 0x0180
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER6)
REGDEF_BIT(wdr_tcurve_index_lut24,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut25,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut26,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut27,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER6)


/*
    wdr_tcurve_index_lut28:    [0x0, 0x3f],         bits : 5_0
    wdr_tcurve_index_lut29:    [0x0, 0x3f],         bits : 13_8
    wdr_tcurve_index_lut30:    [0x0, 0x3f],         bits : 21_16
    wdr_tcurve_index_lut31:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER7_OFS 0x0184
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER7)
REGDEF_BIT(wdr_tcurve_index_lut28,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut29,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut30,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_tcurve_index_lut31,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER7)


/*
    wdr_tcurve_split_lut0  :    [0x0, 0x3],         bits : 1_0
    wdr_tcurve_split_lut1  :    [0x0, 0x3],         bits : 3_2
    wdr_tcurve_split_lut2  :    [0x0, 0x3],         bits : 5_4
    wdr_tcurve_split_lut3  :    [0x0, 0x3],         bits : 7_6
    wdr_tcurve_split_lut4  :    [0x0, 0x3],         bits : 9_8
    wdr_tcurve_split_lut5  :    [0x0, 0x3],         bits : 11_10
    wdr_tcurve_split_lut6  :    [0x0, 0x3],         bits : 13_12
    wdr_tcurve_split_lut7  :    [0x0, 0x3],         bits : 15_14
    wdr_tcurve_split_lut8  :    [0x0, 0x3],         bits : 17_16
    wdr_tcurve_split_lut9  :    [0x0, 0x3],         bits : 19_18
    wdr_tcurve_split_lut10 :    [0x0, 0x3],         bits : 21_20
    wdr_tcurve_split_lut11 :    [0x0, 0x3],         bits : 23_22
    wdr_tcurve_split_lut12 :    [0x0, 0x3],         bits : 25_24
    wdr_tcurve_split_lut13 :    [0x0, 0x3],         bits : 27_26
    wdr_tcurve_split_lut14 :    [0x0, 0x3],         bits : 29_28
    wdr_tcurve_split_lut15 :    [0x0, 0x3],         bits : 31_30
*/
#define WDR_TONE_CURVE_REGISTER8_OFS 0x0188
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER8)
REGDEF_BIT(wdr_tcurve_split_lut0,        2)
REGDEF_BIT(wdr_tcurve_split_lut1,        2)
REGDEF_BIT(wdr_tcurve_split_lut2,        2)
REGDEF_BIT(wdr_tcurve_split_lut3,        2)
REGDEF_BIT(wdr_tcurve_split_lut4,        2)
REGDEF_BIT(wdr_tcurve_split_lut5,        2)
REGDEF_BIT(wdr_tcurve_split_lut6,        2)
REGDEF_BIT(wdr_tcurve_split_lut7,        2)
REGDEF_BIT(wdr_tcurve_split_lut8,        2)
REGDEF_BIT(wdr_tcurve_split_lut9,        2)
REGDEF_BIT(wdr_tcurve_split_lut10,        2)
REGDEF_BIT(wdr_tcurve_split_lut11,        2)
REGDEF_BIT(wdr_tcurve_split_lut12,        2)
REGDEF_BIT(wdr_tcurve_split_lut13,        2)
REGDEF_BIT(wdr_tcurve_split_lut14,        2)
REGDEF_BIT(wdr_tcurve_split_lut15,        2)
REGDEF_END(WDR_TONE_CURVE_REGISTER8)


/*
    wdr_tcurve_split_lut16  :    [0x0, 0x3],            bits : 1_0
    wdr_tcurve_split_lut17  :    [0x0, 0x3],            bits : 3_2
    wdr_tcurve_split_lut18  :    [0x0, 0x3],            bits : 5_4
    wdr_tcurve_split_lut19  :    [0x0, 0x3],            bits : 7_6
    wdr_tcurve_split_lut20  :    [0x0, 0x3],            bits : 9_8
    wdr_tcurve_split_lut21  :    [0x0, 0x3],            bits : 11_10
    wdr_tcurve_split_lut22  :    [0x0, 0x3],            bits : 13_12
    wdr_tcurve_split_lut23  :    [0x0, 0x3],            bits : 15_14
    wdr_tcurve_split_lut24  :    [0x0, 0x3],            bits : 17_16
    wdr_tcurve_split_lut25  :    [0x0, 0x3],            bits : 19_18
    wdr_tcurve_split_lut26  :    [0x0, 0x3],            bits : 21_20
    wdr_tcurve_split_lut27  :    [0x0, 0x3],            bits : 23_22
    wdr_tcurve_split_lut28  :    [0x0, 0x3],            bits : 25_24
    wdr_tcurve_split_lut29  :    [0x0, 0x3],            bits : 27_26
    wdr_tcurve_split_lut30  :    [0x0, 0x3],            bits : 29_28
    wdr_tcurve_split_lut31  :    [0x0, 0x3],            bits : 31_30
*/
#define WDR_TONE_CURVE_REGISTER9_OFS 0x018c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER9)
REGDEF_BIT(wdr_tcurve_split_lut16,        2)
REGDEF_BIT(wdr_tcurve_split_lut17,        2)
REGDEF_BIT(wdr_tcurve_split_lut18,        2)
REGDEF_BIT(wdr_tcurve_split_lut19,        2)
REGDEF_BIT(wdr_tcurve_split_lut20,        2)
REGDEF_BIT(wdr_tcurve_split_lut21,        2)
REGDEF_BIT(wdr_tcurve_split_lut22,        2)
REGDEF_BIT(wdr_tcurve_split_lut23,        2)
REGDEF_BIT(wdr_tcurve_split_lut24,        2)
REGDEF_BIT(wdr_tcurve_split_lut25,        2)
REGDEF_BIT(wdr_tcurve_split_lut26,        2)
REGDEF_BIT(wdr_tcurve_split_lut27,        2)
REGDEF_BIT(wdr_tcurve_split_lut28,        2)
REGDEF_BIT(wdr_tcurve_split_lut29,        2)
REGDEF_BIT(wdr_tcurve_split_lut30,        2)
REGDEF_BIT(wdr_tcurve_split_lut31,        2)
REGDEF_END(WDR_TONE_CURVE_REGISTER9)


/*
    wdr_tcurve_val_lut0:    [0x0, 0xfff],           bits : 11_0
    wdr_tcurve_val_lut1:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER10_OFS 0x0190
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER10)
REGDEF_BIT(wdr_tcurve_val_lut0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut1,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER10)


/*
    wdr_tcurve_val_lut2:    [0x0, 0xfff],           bits : 11_0
    wdr_tcurve_val_lut3:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER11_OFS 0x0194
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER11)
REGDEF_BIT(wdr_tcurve_val_lut2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut3,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER11)


/*
    wdr_tcurve_val_lut4:    [0x0, 0xfff],           bits : 11_0
    wdr_tcurve_val_lut5:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER12_OFS 0x0198
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER12)
REGDEF_BIT(wdr_tcurve_val_lut4,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut5,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER12)


/*
    wdr_tcurve_val_lut6:    [0x0, 0xfff],           bits : 11_0
    wdr_tcurve_val_lut7:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER13_OFS 0x019c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER13)
REGDEF_BIT(wdr_tcurve_val_lut6,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut7,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER13)


/*
    wdr_tcurve_val_lut8:    [0x0, 0xfff],           bits : 11_0
    wdr_tcurve_val_lut9:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER14_OFS 0x01a0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER14)
REGDEF_BIT(wdr_tcurve_val_lut8,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut9,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER14)


/*
    wdr_tcurve_val_lut10:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut11:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER15_OFS 0x01a4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER15)
REGDEF_BIT(wdr_tcurve_val_lut10,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut11,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER15)


/*
    wdr_tcurve_val_lut12:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut13:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER16_OFS 0x01a8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER16)
REGDEF_BIT(wdr_tcurve_val_lut12,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut13,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER16)


/*
    wdr_tcurve_val_lut14:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut15:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER17_OFS 0x01ac
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER17)
REGDEF_BIT(wdr_tcurve_val_lut14,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut15,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER17)


/*
    wdr_tcurve_val_lut16:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut17:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER18_OFS 0x01b0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER18)
REGDEF_BIT(wdr_tcurve_val_lut16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut17,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER18)


/*
    wdr_tcurve_val_lut18:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut19:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER19_OFS 0x01b4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER19)
REGDEF_BIT(wdr_tcurve_val_lut18,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut19,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER19)


/*
    wdr_tcurve_val_lut20:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut21:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER20_OFS 0x01b8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER20)
REGDEF_BIT(wdr_tcurve_val_lut20,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut21,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER20)


/*
    wdr_tcurve_val_lut22:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut23:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER21_OFS 0x01bc
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER21)
REGDEF_BIT(wdr_tcurve_val_lut22,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut23,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER21)


/*
    wdr_tcurve_val_lut24:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut25:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER22_OFS 0x01c0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER22)
REGDEF_BIT(wdr_tcurve_val_lut24,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut25,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER22)


/*
    wdr_tcurve_val_lut26:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut27:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER23_OFS 0x01c4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER23)
REGDEF_BIT(wdr_tcurve_val_lut26,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut27,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER23)


/*
    wdr_tcurve_val_lut28:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut29:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER24_OFS 0x01c8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER24)
REGDEF_BIT(wdr_tcurve_val_lut28,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut29,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER24)


/*
    wdr_tcurve_val_lut30:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut31:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER25_OFS 0x01cc
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER25)
REGDEF_BIT(wdr_tcurve_val_lut30,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut31,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER25)


/*
    wdr_tcurve_val_lut32:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut33:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER26_OFS 0x01d0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER26)
REGDEF_BIT(wdr_tcurve_val_lut32,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut33,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER26)


/*
    wdr_tcurve_val_lut34:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut35:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER27_OFS 0x01d4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER27)
REGDEF_BIT(wdr_tcurve_val_lut34,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut35,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER27)


/*
    wdr_tcurve_val_lut36:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut37:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER28_OFS 0x01d8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER28)
REGDEF_BIT(wdr_tcurve_val_lut36,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut37,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER28)


/*
    wdr_tcurve_val_lut38:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut39:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER29_OFS 0x01dc
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER29)
REGDEF_BIT(wdr_tcurve_val_lut38,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut39,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER29)


/*
    wdr_tcurve_val_lut40:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut41:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER30_OFS 0x01e0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER30)
REGDEF_BIT(wdr_tcurve_val_lut40,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut41,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER30)


/*
    wdr_tcurve_val_lut42:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut43:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER31_OFS 0x01e4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER31)
REGDEF_BIT(wdr_tcurve_val_lut42,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut43,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER31)


/*
    wdr_tcurve_val_lut44:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut45:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER32_OFS 0x01e8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER32)
REGDEF_BIT(wdr_tcurve_val_lut44,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut45,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER32)


/*
    wdr_tcurve_val_lut46:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut47:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER33_OFS 0x01ec
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER33)
REGDEF_BIT(wdr_tcurve_val_lut46,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut47,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER33)


/*
    wdr_tcurve_val_lut48:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut49:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER34_OFS 0x01f0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER34)
REGDEF_BIT(wdr_tcurve_val_lut48,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut49,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER34)


/*
    wdr_tcurve_val_lut50:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut51:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER35_OFS 0x01f4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER35)
REGDEF_BIT(wdr_tcurve_val_lut50,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut51,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER35)


/*
    wdr_tcurve_val_lut52:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut53:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER36_OFS 0x01f8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER36)
REGDEF_BIT(wdr_tcurve_val_lut52,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut53,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER36)


/*
    wdr_tcurve_val_lut54:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut55:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER37_OFS 0x01fc
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER37)
REGDEF_BIT(wdr_tcurve_val_lut54,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut55,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER37)


/*
    wdr_tcurve_val_lut56:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut57:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER38_OFS 0x0200
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER38)
REGDEF_BIT(wdr_tcurve_val_lut56,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut57,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER38)


/*
    wdr_tcurve_val_lut58:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut59:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER39_OFS 0x0204
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER39)
REGDEF_BIT(wdr_tcurve_val_lut58,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut59,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER39)


/*
    wdr_tcurve_val_lut60:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut61:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER40_OFS 0x0208
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER40)
REGDEF_BIT(wdr_tcurve_val_lut60,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut61,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER40)


/*
    wdr_tcurve_val_lut62:    [0x0, 0xfff],          bits : 11_0
    wdr_tcurve_val_lut63:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER41_OFS 0x020c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER41)
REGDEF_BIT(wdr_tcurve_val_lut62,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_tcurve_val_lut63,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER41)


/*
    wdr_tcurve_val_lut64:    [0x0, 0xfff],          bits : 11_0
*/
#define WDR_TONE_CURVE_REGISTER42_OFS 0x0210
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER42)
REGDEF_BIT(wdr_tcurve_val_lut64,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER42)


/*
    wdr_outbld_index_lut0:    [0x0, 0x3f],          bits : 5_0
    wdr_outbld_index_lut1:    [0x0, 0x3f],          bits : 13_8
    wdr_outbld_index_lut2:    [0x0, 0x3f],          bits : 21_16
    wdr_outbld_index_lut3:    [0x0, 0x3f],          bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER0_OFS 0x0214
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER0)
REGDEF_BIT(wdr_outbld_index_lut0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut3,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER0)


/*
    wdr_outbld_index_lut4:    [0x0, 0x3f],          bits : 5_0
    wdr_outbld_index_lut5:    [0x0, 0x3f],          bits : 13_8
    wdr_outbld_index_lut6:    [0x0, 0x3f],          bits : 21_16
    wdr_outbld_index_lut7:    [0x0, 0x3f],          bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER1_OFS 0x0218
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER1)
REGDEF_BIT(wdr_outbld_index_lut4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut7,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER1)


/*
    wdr_outbld_index_lut8 :    [0x0, 0x3f],         bits : 5_0
    wdr_outbld_index_lut9 :    [0x0, 0x3f],         bits : 13_8
    wdr_outbld_index_lut10:    [0x0, 0x3f],         bits : 21_16
    wdr_outbld_index_lut11:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER2_OFS 0x021c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER2)
REGDEF_BIT(wdr_outbld_index_lut8,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut9,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut10,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut11,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER2)


/*
    wdr_outbld_index_lut12:    [0x0, 0x3f],         bits : 5_0
    wdr_outbld_index_lut13:    [0x0, 0x3f],         bits : 13_8
    wdr_outbld_index_lut14:    [0x0, 0x3f],         bits : 21_16
    wdr_outbld_index_lut15:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER3_OFS 0x0220
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER3)
REGDEF_BIT(wdr_outbld_index_lut12,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut13,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut14,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut15,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER3)


/*
    wdr_outbld_index_lut16:    [0x0, 0x3f],         bits : 5_0
    wdr_outbld_index_lut17:    [0x0, 0x3f],         bits : 13_8
    wdr_outbld_index_lut18:    [0x0, 0x3f],         bits : 21_16
    wdr_outbld_index_lut19:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER4_OFS 0x0224
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER4)
REGDEF_BIT(wdr_outbld_index_lut16,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut17,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut18,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut19,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER4)


/*
    wdr_outbld_index_lut20:    [0x0, 0x3f],         bits : 5_0
    wdr_outbld_index_lut21:    [0x0, 0x3f],         bits : 13_8
    wdr_outbld_index_lut22:    [0x0, 0x3f],         bits : 21_16
    wdr_outbld_index_lut23:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER5_OFS 0x0228
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER5)
REGDEF_BIT(wdr_outbld_index_lut20,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut21,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut22,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut23,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER5)


/*
    wdr_outbld_index_lut24:    [0x0, 0x3f],         bits : 5_0
    wdr_outbld_index_lut25:    [0x0, 0x3f],         bits : 13_8
    wdr_outbld_index_lut26:    [0x0, 0x3f],         bits : 21_16
    wdr_outbld_index_lut27:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER6_OFS 0x022c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER6)
REGDEF_BIT(wdr_outbld_index_lut24,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut25,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut26,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut27,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER6)


/*
    wdr_outbld_index_lut28:    [0x0, 0x3f],         bits : 5_0
    wdr_outbld_index_lut29:    [0x0, 0x3f],         bits : 13_8
    wdr_outbld_index_lut30:    [0x0, 0x3f],         bits : 21_16
    wdr_outbld_index_lut31:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER7_OFS 0x0230
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER7)
REGDEF_BIT(wdr_outbld_index_lut28,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut29,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut30,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(wdr_outbld_index_lut31,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER7)


/*
    wdr_outbld_split_lut0 :    [0x0, 0x3],          bits : 1_0
    wdr_outbld_split_lut1 :    [0x0, 0x3],          bits : 3_2
    wdr_outbld_split_lut2 :    [0x0, 0x3],          bits : 5_4
    wdr_outbld_split_lut3 :    [0x0, 0x3],          bits : 7_6
    wdr_outbld_split_lut4 :    [0x0, 0x3],          bits : 9_8
    wdr_outbld_split_lut5 :    [0x0, 0x3],          bits : 11_10
    wdr_outbld_split_lut6 :    [0x0, 0x3],          bits : 13_12
    wdr_outbld_split_lut7 :    [0x0, 0x3],          bits : 15_14
    wdr_outbld_split_lut8 :    [0x0, 0x3],          bits : 17_16
    wdr_outbld_split_lut9 :    [0x0, 0x3],          bits : 19_18
    wdr_outbld_split_lut10:    [0x0, 0x3],          bits : 21_20
    wdr_outbld_split_lut11:    [0x0, 0x3],          bits : 23_22
    wdr_outbld_split_lut12:    [0x0, 0x3],          bits : 25_24
    wdr_outbld_split_lut13:    [0x0, 0x3],          bits : 27_26
    wdr_outbld_split_lut14:    [0x0, 0x3],          bits : 29_28
    wdr_outbld_split_lut15:    [0x0, 0x3],          bits : 31_30
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER8_OFS 0x0234
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER8)
REGDEF_BIT(wdr_outbld_split_lut0,        2)
REGDEF_BIT(wdr_outbld_split_lut1,        2)
REGDEF_BIT(wdr_outbld_split_lut2,        2)
REGDEF_BIT(wdr_outbld_split_lut3,        2)
REGDEF_BIT(wdr_outbld_split_lut4,        2)
REGDEF_BIT(wdr_outbld_split_lut5,        2)
REGDEF_BIT(wdr_outbld_split_lut6,        2)
REGDEF_BIT(wdr_outbld_split_lut7,        2)
REGDEF_BIT(wdr_outbld_split_lut8,        2)
REGDEF_BIT(wdr_outbld_split_lut9,        2)
REGDEF_BIT(wdr_outbld_split_lut10,        2)
REGDEF_BIT(wdr_outbld_split_lut11,        2)
REGDEF_BIT(wdr_outbld_split_lut12,        2)
REGDEF_BIT(wdr_outbld_split_lut13,        2)
REGDEF_BIT(wdr_outbld_split_lut14,        2)
REGDEF_BIT(wdr_outbld_split_lut15,        2)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER8)


/*
    wdr_outbld_split_lut16:    [0x0, 0x3],          bits : 1_0
    wdr_outbld_split_lut17:    [0x0, 0x3],          bits : 3_2
    wdr_outbld_split_lut18:    [0x0, 0x3],          bits : 5_4
    wdr_outbld_split_lut19:    [0x0, 0x3],          bits : 7_6
    wdr_outbld_split_lut20:    [0x0, 0x3],          bits : 9_8
    wdr_outbld_split_lut21:    [0x0, 0x3],          bits : 11_10
    wdr_outbld_split_lut22:    [0x0, 0x3],          bits : 13_12
    wdr_outbld_split_lut23:    [0x0, 0x3],          bits : 15_14
    wdr_outbld_split_lut24:    [0x0, 0x3],          bits : 17_16
    wdr_outbld_split_lut25:    [0x0, 0x3],          bits : 19_18
    wdr_outbld_split_lut26:    [0x0, 0x3],          bits : 21_20
    wdr_outbld_split_lut27:    [0x0, 0x3],          bits : 23_22
    wdr_outbld_split_lut28:    [0x0, 0x3],          bits : 25_24
    wdr_outbld_split_lut29:    [0x0, 0x3],          bits : 27_26
    wdr_outbld_split_lut30:    [0x0, 0x3],          bits : 29_28
    wdr_outbld_split_lut31:    [0x0, 0x3],          bits : 31_30
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER9_OFS 0x0238
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER9)
REGDEF_BIT(wdr_outbld_split_lut16,        2)
REGDEF_BIT(wdr_outbld_split_lut17,        2)
REGDEF_BIT(wdr_outbld_split_lut18,        2)
REGDEF_BIT(wdr_outbld_split_lut19,        2)
REGDEF_BIT(wdr_outbld_split_lut20,        2)
REGDEF_BIT(wdr_outbld_split_lut21,        2)
REGDEF_BIT(wdr_outbld_split_lut22,        2)
REGDEF_BIT(wdr_outbld_split_lut23,        2)
REGDEF_BIT(wdr_outbld_split_lut24,        2)
REGDEF_BIT(wdr_outbld_split_lut25,        2)
REGDEF_BIT(wdr_outbld_split_lut26,        2)
REGDEF_BIT(wdr_outbld_split_lut27,        2)
REGDEF_BIT(wdr_outbld_split_lut28,        2)
REGDEF_BIT(wdr_outbld_split_lut29,        2)
REGDEF_BIT(wdr_outbld_split_lut30,        2)
REGDEF_BIT(wdr_outbld_split_lut31,        2)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER9)


/*
    wdr_outbld_val_lut0:    [0x0, 0xfff],           bits : 11_0
    wdr_outbld_val_lut1:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER10_OFS 0x023c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER10)
REGDEF_BIT(wdr_outbld_val_lut0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut1,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER10)


/*
    wdr_outbld_val_lut2:    [0x0, 0xfff],           bits : 11_0
    wdr_outbld_val_lut3:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER11_OFS 0x0240
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER11)
REGDEF_BIT(wdr_outbld_val_lut2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut3,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER11)


/*
    wdr_outbld_val_lut4:    [0x0, 0xfff],           bits : 11_0
    wdr_outbld_val_lut5:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER12_OFS 0x0244
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER12)
REGDEF_BIT(wdr_outbld_val_lut4,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut5,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER12)


/*
    wdr_outbld_val_lut6:    [0x0, 0xfff],           bits : 11_0
    wdr_outbld_val_lut7:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER13_OFS 0x0248
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER13)
REGDEF_BIT(wdr_outbld_val_lut6,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut7,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER13)


/*
    wdr_outbld_val_lut8:    [0x0, 0xfff],           bits : 11_0
    wdr_outbld_val_lut9:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER14_OFS 0x024c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER14)
REGDEF_BIT(wdr_outbld_val_lut8,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut9,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER14)


/*
    wdr_outbld_val_lut10:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut11:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER15_OFS 0x0250
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER15)
REGDEF_BIT(wdr_outbld_val_lut10,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut11,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER15)


/*
    wdr_outbld_val_lut12:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut13:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER16_OFS 0x0254
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER16)
REGDEF_BIT(wdr_outbld_val_lut12,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut13,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER16)


/*
    wdr_outbld_val_lut14:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut15:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER17_OFS 0x0258
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER17)
REGDEF_BIT(wdr_outbld_val_lut14,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut15,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER17)


/*
    wdr_outbld_val_lut16:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut17:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER18_OFS 0x025c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER18)
REGDEF_BIT(wdr_outbld_val_lut16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut17,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER18)


/*
    wdr_outbld_val_lut18:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut19:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER19_OFS 0x0260
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER19)
REGDEF_BIT(wdr_outbld_val_lut18,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut19,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER19)


/*
    wdr_outbld_val_lut20:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut21:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER20_OFS 0x0264
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER20)
REGDEF_BIT(wdr_outbld_val_lut20,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut21,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER20)


/*
    wdr_outbld_val_lut22:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut23:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER21_OFS 0x0268
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER21)
REGDEF_BIT(wdr_outbld_val_lut22,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut23,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER21)


/*
    wdr_outbld_val_lut24:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut25:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER22_OFS 0x026c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER22)
REGDEF_BIT(wdr_outbld_val_lut24,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut25,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER22)


/*
    wdr_outbld_val_lut26:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut27:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER23_OFS 0x0270
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER23)
REGDEF_BIT(wdr_outbld_val_lut26,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut27,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER23)


/*
    wdr_outbld_val_lut28:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut29:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER24_OFS 0x0274
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER24)
REGDEF_BIT(wdr_outbld_val_lut28,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut29,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER24)


/*
    wdr_outbld_val_lut30:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut31:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER25_OFS 0x0278
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER25)
REGDEF_BIT(wdr_outbld_val_lut30,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut31,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER25)


/*
    wdr_outbld_val_lut32:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut33:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER26_OFS 0x027c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER26)
REGDEF_BIT(wdr_outbld_val_lut32,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut33,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER26)


/*
    wdr_outbld_val_lut34:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut35:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER27_OFS 0x0280
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER27)
REGDEF_BIT(wdr_outbld_val_lut34,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut35,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER27)


/*
    wdr_outbld_val_lut36:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut37:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER28_OFS 0x0284
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER28)
REGDEF_BIT(wdr_outbld_val_lut36,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut37,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER28)


/*
    wdr_outbld_val_lut38:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut39:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER29_OFS 0x0288
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER29)
REGDEF_BIT(wdr_outbld_val_lut38,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut39,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER29)


/*
    wdr_outbld_val_lut40:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut41:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER30_OFS 0x028c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER30)
REGDEF_BIT(wdr_outbld_val_lut40,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut41,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER30)


/*
    wdr_outbld_val_lut42:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut43:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER31_OFS 0x0290
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER31)
REGDEF_BIT(wdr_outbld_val_lut42,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut43,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER31)


/*
    wdr_outbld_val_lut44:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut45:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER32_OFS 0x0294
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER32)
REGDEF_BIT(wdr_outbld_val_lut44,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut45,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER32)


/*
    wdr_outbld_val_lut46:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut47:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER33_OFS 0x0298
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER33)
REGDEF_BIT(wdr_outbld_val_lut46,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut47,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER33)


/*
    wdr_outbld_val_lut48:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut49:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER34_OFS 0x029c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER34)
REGDEF_BIT(wdr_outbld_val_lut48,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut49,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER34)


/*
    wdr_outbld_val_lut50:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut51:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER35_OFS 0x02a0
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER35)
REGDEF_BIT(wdr_outbld_val_lut50,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut51,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER35)


/*
    wdr_outbld_val_lut52:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut53:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER36_OFS 0x02a4
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER36)
REGDEF_BIT(wdr_outbld_val_lut52,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut53,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER36)


/*
    wdr_outbld_val_lut54:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut55:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER37_OFS 0x02a8
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER37)
REGDEF_BIT(wdr_outbld_val_lut54,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut55,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER37)


/*
    wdr_outbld_val_lut56:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut57:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER38_OFS 0x02ac
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER38)
REGDEF_BIT(wdr_outbld_val_lut56,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut57,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER38)


/*
    wdr_outbld_val_lut58:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut59:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER39_OFS 0x02b0
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER39)
REGDEF_BIT(wdr_outbld_val_lut58,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut59,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER39)


/*
    wdr_outbld_val_lut60:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut61:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER40_OFS 0x02b4
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER40)
REGDEF_BIT(wdr_outbld_val_lut60,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut61,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER40)


/*
    wdr_outbld_val_lut62:    [0x0, 0xfff],          bits : 11_0
    wdr_outbld_val_lut63:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER41_OFS 0x02b8
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER41)
REGDEF_BIT(wdr_outbld_val_lut62,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_outbld_val_lut63,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER41)


/*
    wdr_outbld_val_lut64:    [0x0, 0xfff],          bits : 11_0
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER42_OFS 0x02bc
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER42)
REGDEF_BIT(wdr_outbld_val_lut64,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER42)


/*
    wdr_strength :    [0x0, 0xff],          bits : 7_0
    wdr_contrast :    [0x0, 0xff],          bits : 15_8
*/
#define WDR_OUTPUT_BLENDING_REGISTER_OFS 0x02c0
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_REGISTER)
REGDEF_BIT(wdr_strength,        8)
REGDEF_BIT(wdr_contrast,        8)
REGDEF_END(WDR_OUTPUT_BLENDING_REGISTER)


/*
    wdr_sat_th    :    [0x0, 0xfff],            bits : 11_0
    wdr_sat_wt_low:    [0x0, 0xff],         bits : 23_16
    wdr_sat_delta :    [0x0, 0xff],         bits : 31_24
*/
#define WDR_SATURATION_REDUCTION_REGISTER_OFS 0x02c4
REGDEF_BEGIN(WDR_SATURATION_REDUCTION_REGISTER)
REGDEF_BIT(wdr_sat_th,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(wdr_sat_wt_low,        8)
REGDEF_BIT(wdr_sat_delta,        8)
REGDEF_END(WDR_SATURATION_REDUCTION_REGISTER)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED22_OFS 0x02c8
REGDEF_BEGIN(DCE_RESERVED22)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED22)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED23_OFS 0x02cc
REGDEF_BEGIN(DCE_RESERVED23)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED23)


/*
    histogram_h_step:    [0x0, 0x1f],           bits : 4_0
    histogram_v_step:    [0x0, 0x1f],           bits : 20_16
*/
#define HISTOGRAM_REGISTER0_OFS 0x02d0
REGDEF_BEGIN(HISTOGRAM_REGISTER0)
REGDEF_BIT(histogram_h_step,        5)
REGDEF_BIT(,        11)
REGDEF_BIT(histogram_v_step,        5)
REGDEF_END(HISTOGRAM_REGISTER0)


/*
    histogram_bin0:    [0x0, 0xffff],           bits : 15_0
    histogram_bin1:    [0x0, 0xffff],           bits : 31_16
*/
#define HISTOGRAM_REGISTER1_OFS 0x02d4
REGDEF_BEGIN(HISTOGRAM_REGISTER1)
REGDEF_BIT(histogram_bin0,        16)
REGDEF_BIT(histogram_bin1,        16)
REGDEF_END(HISTOGRAM_REGISTER1)


/*
    histogram_bin2:    [0x0, 0xffff],           bits : 15_0
    histogram_bin3:    [0x0, 0xffff],           bits : 31_16
*/
#define HISTOGRAM_REGISTER2_OFS 0x02d8
REGDEF_BEGIN(HISTOGRAM_REGISTER2)
REGDEF_BIT(histogram_bin2,        16)
REGDEF_BIT(histogram_bin3,        16)
REGDEF_END(HISTOGRAM_REGISTER2)


/*
    histogram_bin4:    [0x0, 0xffff],           bits : 15_0
    histogram_bin5:    [0x0, 0xffff],           bits : 31_16
*/
#define HISTOGRAM_REGISTER3_OFS 0x02dc
REGDEF_BEGIN(HISTOGRAM_REGISTER3)
REGDEF_BIT(histogram_bin4,        16)
REGDEF_BIT(histogram_bin5,        16)
REGDEF_END(HISTOGRAM_REGISTER3)


/*
    histogram_bin6:    [0x0, 0xffff],           bits : 15_0
    histogram_bin7:    [0x0, 0xffff],           bits : 31_16
*/
#define HISTOGRAM_REGISTER4_OFS 0x02e0
REGDEF_BEGIN(HISTOGRAM_REGISTER4)
REGDEF_BIT(histogram_bin6,        16)
REGDEF_BIT(histogram_bin7,        16)
REGDEF_END(HISTOGRAM_REGISTER4)


/*
    histogram_bin8:    [0x0, 0xffff],           bits : 15_0
    histogram_bin9:    [0x0, 0xffff],           bits : 31_16
*/
#define HISTOGRAM_REGISTER5_OFS 0x02e4
REGDEF_BEGIN(HISTOGRAM_REGISTER5)
REGDEF_BIT(histogram_bin8,        16)
REGDEF_BIT(histogram_bin9,        16)
REGDEF_END(HISTOGRAM_REGISTER5)


/*
    histogram_bin10:    [0x0, 0xffff],          bits : 15_0
    histogram_bin11:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER6_OFS 0x02e8
REGDEF_BEGIN(HISTOGRAM_REGISTER6)
REGDEF_BIT(histogram_bin10,        16)
REGDEF_BIT(histogram_bin11,        16)
REGDEF_END(HISTOGRAM_REGISTER6)


/*
    histogram_bin12:    [0x0, 0xffff],          bits : 15_0
    histogram_bin13:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER7_OFS 0x02ec
REGDEF_BEGIN(HISTOGRAM_REGISTER7)
REGDEF_BIT(histogram_bin12,        16)
REGDEF_BIT(histogram_bin13,        16)
REGDEF_END(HISTOGRAM_REGISTER7)


/*
    histogram_bin14:    [0x0, 0xffff],          bits : 15_0
    histogram_bin15:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER8_OFS 0x02f0
REGDEF_BEGIN(HISTOGRAM_REGISTER8)
REGDEF_BIT(histogram_bin14,        16)
REGDEF_BIT(histogram_bin15,        16)
REGDEF_END(HISTOGRAM_REGISTER8)


/*
    histogram_bin16:    [0x0, 0xffff],          bits : 15_0
    histogram_bin17:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER9_OFS 0x02f4
REGDEF_BEGIN(HISTOGRAM_REGISTER9)
REGDEF_BIT(histogram_bin16,        16)
REGDEF_BIT(histogram_bin17,        16)
REGDEF_END(HISTOGRAM_REGISTER9)


/*
    histogram_bin18:    [0x0, 0xffff],          bits : 15_0
    histogram_bin19:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER10_OFS 0x02f8
REGDEF_BEGIN(HISTOGRAM_REGISTER10)
REGDEF_BIT(histogram_bin18,        16)
REGDEF_BIT(histogram_bin19,        16)
REGDEF_END(HISTOGRAM_REGISTER10)


/*
    histogram_bin20:    [0x0, 0xffff],          bits : 15_0
    histogram_bin21:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER11_OFS 0x02fc
REGDEF_BEGIN(HISTOGRAM_REGISTER11)
REGDEF_BIT(histogram_bin20,        16)
REGDEF_BIT(histogram_bin21,        16)
REGDEF_END(HISTOGRAM_REGISTER11)


/*
    histogram_bin22:    [0x0, 0xffff],          bits : 15_0
    histogram_bin23:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER12_OFS 0x0300
REGDEF_BEGIN(HISTOGRAM_REGISTER12)
REGDEF_BIT(histogram_bin22,        16)
REGDEF_BIT(histogram_bin23,        16)
REGDEF_END(HISTOGRAM_REGISTER12)


/*
    histogram_bin24:    [0x0, 0xffff],          bits : 15_0
    histogram_bin25:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER13_OFS 0x0304
REGDEF_BEGIN(HISTOGRAM_REGISTER13)
REGDEF_BIT(histogram_bin24,        16)
REGDEF_BIT(histogram_bin25,        16)
REGDEF_END(HISTOGRAM_REGISTER13)


/*
    histogram_bin26:    [0x0, 0xffff],          bits : 15_0
    histogram_bin27:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER14_OFS 0x0308
REGDEF_BEGIN(HISTOGRAM_REGISTER14)
REGDEF_BIT(histogram_bin26,        16)
REGDEF_BIT(histogram_bin27,        16)
REGDEF_END(HISTOGRAM_REGISTER14)


/*
    histogram_bin28:    [0x0, 0xffff],          bits : 15_0
    histogram_bin29:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER15_OFS 0x030c
REGDEF_BEGIN(HISTOGRAM_REGISTER15)
REGDEF_BIT(histogram_bin28,        16)
REGDEF_BIT(histogram_bin29,        16)
REGDEF_END(HISTOGRAM_REGISTER15)


/*
    histogram_bin30:    [0x0, 0xffff],          bits : 15_0
    histogram_bin31:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER16_OFS 0x0310
REGDEF_BEGIN(HISTOGRAM_REGISTER16)
REGDEF_BIT(histogram_bin30,        16)
REGDEF_BIT(histogram_bin31,        16)
REGDEF_END(HISTOGRAM_REGISTER16)


/*
    histogram_bin32:    [0x0, 0xffff],          bits : 15_0
    histogram_bin33:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER17_OFS 0x0314
REGDEF_BEGIN(HISTOGRAM_REGISTER17)
REGDEF_BIT(histogram_bin32,        16)
REGDEF_BIT(histogram_bin33,        16)
REGDEF_END(HISTOGRAM_REGISTER17)


/*
    histogram_bin34:    [0x0, 0xffff],          bits : 15_0
    histogram_bin35:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER18_OFS 0x0318
REGDEF_BEGIN(HISTOGRAM_REGISTER18)
REGDEF_BIT(histogram_bin34,        16)
REGDEF_BIT(histogram_bin35,        16)
REGDEF_END(HISTOGRAM_REGISTER18)


/*
    histogram_bin36:    [0x0, 0xffff],          bits : 15_0
    histogram_bin37:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER19_OFS 0x031c
REGDEF_BEGIN(HISTOGRAM_REGISTER19)
REGDEF_BIT(histogram_bin36,        16)
REGDEF_BIT(histogram_bin37,        16)
REGDEF_END(HISTOGRAM_REGISTER19)


/*
    histogram_bin38:    [0x0, 0xffff],          bits : 15_0
    histogram_bin39:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER20_OFS 0x0320
REGDEF_BEGIN(HISTOGRAM_REGISTER20)
REGDEF_BIT(histogram_bin38,        16)
REGDEF_BIT(histogram_bin39,        16)
REGDEF_END(HISTOGRAM_REGISTER20)


/*
    histogram_bin40:    [0x0, 0xffff],          bits : 15_0
    histogram_bin41:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER21_OFS 0x0324
REGDEF_BEGIN(HISTOGRAM_REGISTER21)
REGDEF_BIT(histogram_bin40,        16)
REGDEF_BIT(histogram_bin41,        16)
REGDEF_END(HISTOGRAM_REGISTER21)


/*
    histogram_bin42:    [0x0, 0xffff],          bits : 15_0
    histogram_bin43:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER22_OFS 0x0328
REGDEF_BEGIN(HISTOGRAM_REGISTER22)
REGDEF_BIT(histogram_bin42,        16)
REGDEF_BIT(histogram_bin43,        16)
REGDEF_END(HISTOGRAM_REGISTER22)


/*
    histogram_bin44:    [0x0, 0xffff],          bits : 15_0
    histogram_bin45:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER23_OFS 0x032c
REGDEF_BEGIN(HISTOGRAM_REGISTER23)
REGDEF_BIT(histogram_bin44,        16)
REGDEF_BIT(histogram_bin45,        16)
REGDEF_END(HISTOGRAM_REGISTER23)


/*
    histogram_bin46:    [0x0, 0xffff],          bits : 15_0
    histogram_bin47:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER24_OFS 0x0330
REGDEF_BEGIN(HISTOGRAM_REGISTER24)
REGDEF_BIT(histogram_bin46,        16)
REGDEF_BIT(histogram_bin47,        16)
REGDEF_END(HISTOGRAM_REGISTER24)


/*
    histogram_bin48:    [0x0, 0xffff],          bits : 15_0
    histogram_bin49:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER25_OFS 0x0334
REGDEF_BEGIN(HISTOGRAM_REGISTER25)
REGDEF_BIT(histogram_bin48,        16)
REGDEF_BIT(histogram_bin49,        16)
REGDEF_END(HISTOGRAM_REGISTER25)


/*
    histogram_bin50:    [0x0, 0xffff],          bits : 15_0
    histogram_bin51:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER26_OFS 0x0338
REGDEF_BEGIN(HISTOGRAM_REGISTER26)
REGDEF_BIT(histogram_bin50,        16)
REGDEF_BIT(histogram_bin51,        16)
REGDEF_END(HISTOGRAM_REGISTER26)


/*
    histogram_bin52:    [0x0, 0xffff],          bits : 15_0
    histogram_bin53:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER27_OFS 0x033c
REGDEF_BEGIN(HISTOGRAM_REGISTER27)
REGDEF_BIT(histogram_bin52,        16)
REGDEF_BIT(histogram_bin53,        16)
REGDEF_END(HISTOGRAM_REGISTER27)


/*
    histogram_bin54:    [0x0, 0xffff],          bits : 15_0
    histogram_bin55:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER28_OFS 0x0340
REGDEF_BEGIN(HISTOGRAM_REGISTER28)
REGDEF_BIT(histogram_bin54,        16)
REGDEF_BIT(histogram_bin55,        16)
REGDEF_END(HISTOGRAM_REGISTER28)


/*
    histogram_bin56:    [0x0, 0xffff],          bits : 15_0
    histogram_bin57:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER29_OFS 0x0344
REGDEF_BEGIN(HISTOGRAM_REGISTER29)
REGDEF_BIT(histogram_bin56,        16)
REGDEF_BIT(histogram_bin57,        16)
REGDEF_END(HISTOGRAM_REGISTER29)


/*
    histogram_bin58:    [0x0, 0xffff],          bits : 15_0
    histogram_bin59:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER30_OFS 0x0348
REGDEF_BEGIN(HISTOGRAM_REGISTER30)
REGDEF_BIT(histogram_bin58,        16)
REGDEF_BIT(histogram_bin59,        16)
REGDEF_END(HISTOGRAM_REGISTER30)


/*
    histogram_bin60:    [0x0, 0xffff],          bits : 15_0
    histogram_bin61:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER31_OFS 0x034c
REGDEF_BEGIN(HISTOGRAM_REGISTER31)
REGDEF_BIT(histogram_bin60,        16)
REGDEF_BIT(histogram_bin61,        16)
REGDEF_END(HISTOGRAM_REGISTER31)


/*
    histogram_bin62:    [0x0, 0xffff],          bits : 15_0
    histogram_bin63:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER32_OFS 0x0350
REGDEF_BEGIN(HISTOGRAM_REGISTER32)
REGDEF_BIT(histogram_bin62,        16)
REGDEF_BIT(histogram_bin63,        16)
REGDEF_END(HISTOGRAM_REGISTER32)


/*
    histogram_bin64:    [0x0, 0xffff],          bits : 15_0
    histogram_bin65:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER33_OFS 0x0354
REGDEF_BEGIN(HISTOGRAM_REGISTER33)
REGDEF_BIT(histogram_bin64,        16)
REGDEF_BIT(histogram_bin65,        16)
REGDEF_END(HISTOGRAM_REGISTER33)


/*
    histogram_bin66:    [0x0, 0xffff],          bits : 15_0
    histogram_bin67:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER34_OFS 0x0358
REGDEF_BEGIN(HISTOGRAM_REGISTER34)
REGDEF_BIT(histogram_bin66,        16)
REGDEF_BIT(histogram_bin67,        16)
REGDEF_END(HISTOGRAM_REGISTER34)


/*
    histogram_bin68:    [0x0, 0xffff],          bits : 15_0
    histogram_bin69:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER35_OFS 0x035c
REGDEF_BEGIN(HISTOGRAM_REGISTER35)
REGDEF_BIT(histogram_bin68,        16)
REGDEF_BIT(histogram_bin69,        16)
REGDEF_END(HISTOGRAM_REGISTER35)


/*
    histogram_bin70:    [0x0, 0xffff],          bits : 15_0
    histogram_bin71:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER36_OFS 0x0360
REGDEF_BEGIN(HISTOGRAM_REGISTER36)
REGDEF_BIT(histogram_bin70,        16)
REGDEF_BIT(histogram_bin71,        16)
REGDEF_END(HISTOGRAM_REGISTER36)


/*
    histogram_bin72:    [0x0, 0xffff],          bits : 15_0
    histogram_bin73:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER37_OFS 0x0364
REGDEF_BEGIN(HISTOGRAM_REGISTER37)
REGDEF_BIT(histogram_bin72,        16)
REGDEF_BIT(histogram_bin73,        16)
REGDEF_END(HISTOGRAM_REGISTER37)


/*
    histogram_bin74:    [0x0, 0xffff],          bits : 15_0
    histogram_bin75:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER38_OFS 0x0368
REGDEF_BEGIN(HISTOGRAM_REGISTER38)
REGDEF_BIT(histogram_bin74,        16)
REGDEF_BIT(histogram_bin75,        16)
REGDEF_END(HISTOGRAM_REGISTER38)


/*
    histogram_bin76:    [0x0, 0xffff],          bits : 15_0
    histogram_bin77:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER39_OFS 0x036c
REGDEF_BEGIN(HISTOGRAM_REGISTER39)
REGDEF_BIT(histogram_bin76,        16)
REGDEF_BIT(histogram_bin77,        16)
REGDEF_END(HISTOGRAM_REGISTER39)


/*
    histogram_bin78:    [0x0, 0xffff],          bits : 15_0
    histogram_bin79:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER40_OFS 0x0370
REGDEF_BEGIN(HISTOGRAM_REGISTER40)
REGDEF_BIT(histogram_bin78,        16)
REGDEF_BIT(histogram_bin79,        16)
REGDEF_END(HISTOGRAM_REGISTER40)


/*
    histogram_bin80:    [0x0, 0xffff],          bits : 15_0
    histogram_bin81:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER41_OFS 0x0374
REGDEF_BEGIN(HISTOGRAM_REGISTER41)
REGDEF_BIT(histogram_bin80,        16)
REGDEF_BIT(histogram_bin81,        16)
REGDEF_END(HISTOGRAM_REGISTER41)


/*
    histogram_bin82:    [0x0, 0xffff],          bits : 15_0
    histogram_bin83:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER42_OFS 0x0378
REGDEF_BEGIN(HISTOGRAM_REGISTER42)
REGDEF_BIT(histogram_bin82,        16)
REGDEF_BIT(histogram_bin83,        16)
REGDEF_END(HISTOGRAM_REGISTER42)


/*
    histogram_bin84:    [0x0, 0xffff],          bits : 15_0
    histogram_bin85:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER43_OFS 0x037c
REGDEF_BEGIN(HISTOGRAM_REGISTER43)
REGDEF_BIT(histogram_bin84,        16)
REGDEF_BIT(histogram_bin85,        16)
REGDEF_END(HISTOGRAM_REGISTER43)


/*
    histogram_bin86:    [0x0, 0xffff],          bits : 15_0
    histogram_bin87:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER44_OFS 0x0380
REGDEF_BEGIN(HISTOGRAM_REGISTER44)
REGDEF_BIT(histogram_bin86,        16)
REGDEF_BIT(histogram_bin87,        16)
REGDEF_END(HISTOGRAM_REGISTER44)


/*
    histogram_bin88:    [0x0, 0xffff],          bits : 15_0
    histogram_bin89:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER45_OFS 0x0384
REGDEF_BEGIN(HISTOGRAM_REGISTER45)
REGDEF_BIT(histogram_bin88,        16)
REGDEF_BIT(histogram_bin89,        16)
REGDEF_END(HISTOGRAM_REGISTER45)


/*
    histogram_bin90:    [0x0, 0xffff],          bits : 15_0
    histogram_bin91:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER46_OFS 0x0388
REGDEF_BEGIN(HISTOGRAM_REGISTER46)
REGDEF_BIT(histogram_bin90,        16)
REGDEF_BIT(histogram_bin91,        16)
REGDEF_END(HISTOGRAM_REGISTER46)


/*
    histogram_bin92:    [0x0, 0xffff],          bits : 15_0
    histogram_bin93:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER47_OFS 0x038c
REGDEF_BEGIN(HISTOGRAM_REGISTER47)
REGDEF_BIT(histogram_bin92,        16)
REGDEF_BIT(histogram_bin93,        16)
REGDEF_END(HISTOGRAM_REGISTER47)


/*
    histogram_bin94:    [0x0, 0xffff],          bits : 15_0
    histogram_bin95:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER48_OFS 0x0390
REGDEF_BEGIN(HISTOGRAM_REGISTER48)
REGDEF_BIT(histogram_bin94,        16)
REGDEF_BIT(histogram_bin95,        16)
REGDEF_END(HISTOGRAM_REGISTER48)


/*
    histogram_bin96:    [0x0, 0xffff],          bits : 15_0
    histogram_bin97:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER49_OFS 0x0394
REGDEF_BEGIN(HISTOGRAM_REGISTER49)
REGDEF_BIT(histogram_bin96,        16)
REGDEF_BIT(histogram_bin97,        16)
REGDEF_END(HISTOGRAM_REGISTER49)


/*
    histogram_bin98:    [0x0, 0xffff],          bits : 15_0
    histogram_bin99:    [0x0, 0xffff],          bits : 31_16
*/
#define HISTOGRAM_REGISTER50_OFS 0x0398
REGDEF_BEGIN(HISTOGRAM_REGISTER50)
REGDEF_BIT(histogram_bin98,        16)
REGDEF_BIT(histogram_bin99,        16)
REGDEF_END(HISTOGRAM_REGISTER50)


/*
    histogram_bin100:    [0x0, 0xffff],         bits : 15_0
    histogram_bin101:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER51_OFS 0x039c
REGDEF_BEGIN(HISTOGRAM_REGISTER51)
REGDEF_BIT(histogram_bin100,        16)
REGDEF_BIT(histogram_bin101,        16)
REGDEF_END(HISTOGRAM_REGISTER51)


/*
    histogram_bin102:    [0x0, 0xffff],         bits : 15_0
    histogram_bin103:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER52_OFS 0x03a0
REGDEF_BEGIN(HISTOGRAM_REGISTER52)
REGDEF_BIT(histogram_bin102,        16)
REGDEF_BIT(histogram_bin103,        16)
REGDEF_END(HISTOGRAM_REGISTER52)


/*
    histogram_bin104:    [0x0, 0xffff],         bits : 15_0
    histogram_bin105:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER53_OFS 0x03a4
REGDEF_BEGIN(HISTOGRAM_REGISTER53)
REGDEF_BIT(histogram_bin104,        16)
REGDEF_BIT(histogram_bin105,        16)
REGDEF_END(HISTOGRAM_REGISTER53)


/*
    histogram_bin106:    [0x0, 0xffff],         bits : 15_0
    histogram_bin107:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER54_OFS 0x03a8
REGDEF_BEGIN(HISTOGRAM_REGISTER54)
REGDEF_BIT(histogram_bin106,        16)
REGDEF_BIT(histogram_bin107,        16)
REGDEF_END(HISTOGRAM_REGISTER54)


/*
    histogram_bin108:    [0x0, 0xffff],         bits : 15_0
    histogram_bin109:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER55_OFS 0x03ac
REGDEF_BEGIN(HISTOGRAM_REGISTER55)
REGDEF_BIT(histogram_bin108,        16)
REGDEF_BIT(histogram_bin109,        16)
REGDEF_END(HISTOGRAM_REGISTER55)


/*
    histogram_bin110:    [0x0, 0xffff],         bits : 15_0
    histogram_bin111:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER56_OFS 0x03b0
REGDEF_BEGIN(HISTOGRAM_REGISTER56)
REGDEF_BIT(histogram_bin110,        16)
REGDEF_BIT(histogram_bin111,        16)
REGDEF_END(HISTOGRAM_REGISTER56)


/*
    histogram_bin112:    [0x0, 0xffff],         bits : 15_0
    histogram_bin113:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER57_OFS 0x03b4
REGDEF_BEGIN(HISTOGRAM_REGISTER57)
REGDEF_BIT(histogram_bin112,        16)
REGDEF_BIT(histogram_bin113,        16)
REGDEF_END(HISTOGRAM_REGISTER57)


/*
    histogram_bin114:    [0x0, 0xffff],         bits : 15_0
    histogram_bin115:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER58_OFS 0x03b8
REGDEF_BEGIN(HISTOGRAM_REGISTER58)
REGDEF_BIT(histogram_bin114,        16)
REGDEF_BIT(histogram_bin115,        16)
REGDEF_END(HISTOGRAM_REGISTER58)


/*
    histogram_bin116:    [0x0, 0xffff],         bits : 15_0
    histogram_bin117:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER59_OFS 0x03bc
REGDEF_BEGIN(HISTOGRAM_REGISTER59)
REGDEF_BIT(histogram_bin116,        16)
REGDEF_BIT(histogram_bin117,        16)
REGDEF_END(HISTOGRAM_REGISTER59)


/*
    histogram_bin118:    [0x0, 0xffff],         bits : 15_0
    histogram_bin119:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER60_OFS 0x03c0
REGDEF_BEGIN(HISTOGRAM_REGISTER60)
REGDEF_BIT(histogram_bin118,        16)
REGDEF_BIT(histogram_bin119,        16)
REGDEF_END(HISTOGRAM_REGISTER60)


/*
    histogram_bin120:    [0x0, 0xffff],         bits : 15_0
    histogram_bin121:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER61_OFS 0x03c4
REGDEF_BEGIN(HISTOGRAM_REGISTER61)
REGDEF_BIT(histogram_bin120,        16)
REGDEF_BIT(histogram_bin121,        16)
REGDEF_END(HISTOGRAM_REGISTER61)


/*
    histogram_bin122:    [0x0, 0xffff],         bits : 15_0
    histogram_bin123:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER62_OFS 0x03c8
REGDEF_BEGIN(HISTOGRAM_REGISTER62)
REGDEF_BIT(histogram_bin122,        16)
REGDEF_BIT(histogram_bin123,        16)
REGDEF_END(HISTOGRAM_REGISTER62)


/*
    histogram_bin124:    [0x0, 0xffff],         bits : 15_0
    histogram_bin125:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER63_OFS 0x03cc
REGDEF_BEGIN(HISTOGRAM_REGISTER63)
REGDEF_BIT(histogram_bin124,        16)
REGDEF_BIT(histogram_bin125,        16)
REGDEF_END(HISTOGRAM_REGISTER63)


/*
    histogram_bin126:    [0x0, 0xffff],         bits : 15_0
    histogram_bin127:    [0x0, 0xffff],         bits : 31_16
*/
#define HISTOGRAM_REGISTER64_OFS 0x03d0
REGDEF_BEGIN(HISTOGRAM_REGISTER64)
REGDEF_BIT(histogram_bin126,        16)
REGDEF_BIT(histogram_bin127,        16)
REGDEF_END(HISTOGRAM_REGISTER64)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED24_OFS 0x03d4
REGDEF_BEGIN(DCE_RESERVED24)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED24)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED25_OFS 0x03d8
REGDEF_BEGIN(DCE_RESERVED25)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED25)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED26_OFS 0x03dc
REGDEF_BEGIN(DCE_RESERVED26)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED26)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED27_OFS 0x03e0
REGDEF_BEGIN(DCE_RESERVED27)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED27)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED28_OFS 0x03e4
REGDEF_BEGIN(DCE_RESERVED28)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED28)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED29_OFS 0x03e8
REGDEF_BEGIN(DCE_RESERVED29)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED29)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED30_OFS 0x03ec
REGDEF_BEGIN(DCE_RESERVED30)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED30)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED31_OFS 0x03f0
REGDEF_BEGIN(DCE_RESERVED31)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED31)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED32_OFS 0x03f4
REGDEF_BEGIN(DCE_RESERVED32)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED32)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED33_OFS 0x03f8
REGDEF_BEGIN(DCE_RESERVED33)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED33)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED34_OFS 0x03fc
REGDEF_BEGIN(DCE_RESERVED34)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED34)


/*
    gdc_mode:    [0x0, 0x1],            bits : 1
*/
#define GDC_REGISTER_OFS 0x0400
REGDEF_BEGIN(GDC_REGISTER)
REGDEF_BIT(,        1)
REGDEF_BIT(gdc_mode,        1)
REGDEF_END(GDC_REGISTER)


/*
    gdc_centx:    [0x0, 0x3fff],            bits : 13_0
    gdc_centy:    [0x0, 0x3fff],            bits : 29_16
*/
#define GEO_CENTER_REGISTER_OFS 0x0404
REGDEF_BEGIN(GEO_CENTER_REGISTER)
REGDEF_BIT(gdc_centx,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(gdc_centy,        14)
REGDEF_END(GEO_CENTER_REGISTER)


/*
    gdc_xdist:    [0x0, 0xfff],         bits : 11_0
    gdc_ydist:    [0x0, 0xfff],         bits : 23_12
*/
#define GEO_AXIS_DISTANCE_REGISTER_OFS 0x0408
REGDEF_BEGIN(GEO_AXIS_DISTANCE_REGISTER)
REGDEF_BIT(gdc_xdist,        12)
REGDEF_BIT(gdc_ydist,        12)
REGDEF_END(GEO_AXIS_DISTANCE_REGISTER)


/*
    gdc_normfact:    [0x0, 0xff],           bits : 7_0
    gdc_normbit :    [0x0, 0x1f],           bits : 12_8
*/
#define GEO_DISTANCE_NORMALIZATION_REGISTER_OFS 0x040c
REGDEF_BEGIN(GEO_DISTANCE_NORMALIZATION_REGISTER)
REGDEF_BIT(gdc_normfact,        8)
REGDEF_BIT(gdc_normbit,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(gdc_normfact_10b,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(gdc_normfact_sel,        1)
REGDEF_END(GEO_DISTANCE_NORMALIZATION_REGISTER)


/*
    gdc_fovgain :    [0x0, 0xfff],          bits : 11_0
    gdc_fovbound:    [0x0, 0x1],            bits : 12
*/
#define GDC_FOV_REGISTER_OFS 0x0410
REGDEF_BEGIN(GDC_FOV_REGISTER)
REGDEF_BIT(gdc_fovgain,        12)
REGDEF_BIT(gdc_fovbound,        1)
REGDEF_END(GDC_FOV_REGISTER)


/*
    gdc_boundr:    [0x0, 0x3ff],            bits : 9_0
    gdc_boundg:    [0x0, 0x3ff],            bits : 19_10
    gdc_boundb:    [0x0, 0x3ff],            bits : 29_20
*/
#define GDC_FOV_BOUNDARY_REGISTER_OFS 0x0414
REGDEF_BEGIN(GDC_FOV_BOUNDARY_REGISTER)
REGDEF_BIT(gdc_boundr,        10)
REGDEF_BIT(gdc_boundg,        10)
REGDEF_BIT(gdc_boundb,        10)
REGDEF_END(GDC_FOV_BOUNDARY_REGISTER)


/*
    cac_rlutgain:    [0x0, 0x1fff],         bits : 12_0
    cac_glutgain:    [0x0, 0x1fff],         bits : 28_16
*/
#define GEO_ABERRATION_REGISTER0_OFS 0x0418
REGDEF_BEGIN(GEO_ABERRATION_REGISTER0)
REGDEF_BIT(cac_rlutgain,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(cac_glutgain,        13)
REGDEF_END(GEO_ABERRATION_REGISTER0)


/*
    cac_blutgain:    [0x0, 0x1fff],         bits : 12_0
    cac_sel     :    [0x0, 0x1],            bits : 16
*/
#define GEO_ABERRATION_REGISTER1_OFS 0x041c
REGDEF_BEGIN(GEO_ABERRATION_REGISTER1)
REGDEF_BIT(cac_blutgain,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(cac_sel,        1)
REGDEF_END(GEO_ABERRATION_REGISTER1)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED37_OFS 0x0420
REGDEF_BEGIN(DCE_RESERVED37)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED37)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED38_OFS 0x0424
REGDEF_BEGIN(DCE_RESERVED38)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED38)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED39_OFS 0x0428
REGDEF_BEGIN(DCE_RESERVED39)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED39)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED40_OFS 0x042c
REGDEF_BEGIN(DCE_RESERVED40)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED40)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED41_OFS 0x0430
REGDEF_BEGIN(DCE_RESERVED41)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED41)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED42_OFS 0x0434
REGDEF_BEGIN(DCE_RESERVED42)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED42)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED43_OFS 0x0438
REGDEF_BEGIN(DCE_RESERVED43)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED43)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED44_OFS 0x043c
REGDEF_BEGIN(DCE_RESERVED44)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED44)


/*
    gdc_lutg0:    [0x0, 0xffff],            bits : 15_0
    gdc_lutg1:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG1_OFS 0x0440
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG1)
REGDEF_BIT(gdc_lutg0,        16)
REGDEF_BIT(gdc_lutg1,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG1)


/*
    gdc_lutg2:    [0x0, 0xffff],            bits : 15_0
    gdc_lutg3:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG2_OFS 0x0444
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG2)
REGDEF_BIT(gdc_lutg2,        16)
REGDEF_BIT(gdc_lutg3,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG2)


/*
    gdc_lutg4:    [0x0, 0xffff],            bits : 15_0
    gdc_lutg5:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG3_OFS 0x0448
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG3)
REGDEF_BIT(gdc_lutg4,        16)
REGDEF_BIT(gdc_lutg5,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG3)


/*
    gdc_lutg6:    [0x0, 0xffff],            bits : 15_0
    gdc_lutg7:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG4_OFS 0x044c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG4)
REGDEF_BIT(gdc_lutg6,        16)
REGDEF_BIT(gdc_lutg7,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG4)


/*
    gdc_lutg8:    [0x0, 0xffff],            bits : 15_0
    gdc_lutg9:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG5_OFS 0x0450
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG5)
REGDEF_BIT(gdc_lutg8,        16)
REGDEF_BIT(gdc_lutg9,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG5)


/*
    gdc_lutg10:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg11:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG6_OFS 0x0454
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG6)
REGDEF_BIT(gdc_lutg10,        16)
REGDEF_BIT(gdc_lutg11,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG6)


/*
    gdc_lutg12:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg13:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG7_OFS 0x0458
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG7)
REGDEF_BIT(gdc_lutg12,        16)
REGDEF_BIT(gdc_lutg13,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG7)


/*
    gdc_lutg14:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg15:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG8_OFS 0x045c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG8)
REGDEF_BIT(gdc_lutg14,        16)
REGDEF_BIT(gdc_lutg15,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG8)


/*
    gdc_lutg16:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg17:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG9_OFS 0x0460
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG9)
REGDEF_BIT(gdc_lutg16,        16)
REGDEF_BIT(gdc_lutg17,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG9)


/*
    gdc_lutg18:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg19:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG10_OFS 0x0464
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG10)
REGDEF_BIT(gdc_lutg18,        16)
REGDEF_BIT(gdc_lutg19,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG10)


/*
    gdc_lutg20:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg21:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG11_OFS 0x0468
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG11)
REGDEF_BIT(gdc_lutg20,        16)
REGDEF_BIT(gdc_lutg21,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG11)


/*
    gdc_lutg22:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg23:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG12_OFS 0x046c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG12)
REGDEF_BIT(gdc_lutg22,        16)
REGDEF_BIT(gdc_lutg23,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG12)


/*
    gdc_lutg24:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg25:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG13_OFS 0x0470
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG13)
REGDEF_BIT(gdc_lutg24,        16)
REGDEF_BIT(gdc_lutg25,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG13)


/*
    gdc_lutg26:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg27:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG14_OFS 0x0474
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG14)
REGDEF_BIT(gdc_lutg26,        16)
REGDEF_BIT(gdc_lutg27,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG14)


/*
    gdc_lutg28:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg29:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG15_OFS 0x0478
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG15)
REGDEF_BIT(gdc_lutg28,        16)
REGDEF_BIT(gdc_lutg29,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG15)


/*
    gdc_lutg30:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg31:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG16_OFS 0x047c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG16)
REGDEF_BIT(gdc_lutg30,        16)
REGDEF_BIT(gdc_lutg31,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG16)


/*
    gdc_lutg32:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg33:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG17_OFS 0x0480
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG17)
REGDEF_BIT(gdc_lutg32,        16)
REGDEF_BIT(gdc_lutg33,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG17)


/*
    gdc_lutg34:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg35:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG18_OFS 0x0484
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG18)
REGDEF_BIT(gdc_lutg34,        16)
REGDEF_BIT(gdc_lutg35,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG18)


/*
    gdc_lutg36:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg37:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG19_OFS 0x0488
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG19)
REGDEF_BIT(gdc_lutg36,        16)
REGDEF_BIT(gdc_lutg37,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG19)


/*
    gdc_lutg38:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg39:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG20_OFS 0x048c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG20)
REGDEF_BIT(gdc_lutg38,        16)
REGDEF_BIT(gdc_lutg39,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG20)


/*
    gdc_lutg40:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg41:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG21_OFS 0x0490
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG21)
REGDEF_BIT(gdc_lutg40,        16)
REGDEF_BIT(gdc_lutg41,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG21)


/*
    gdc_lutg42:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg43:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG22_OFS 0x0494
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG22)
REGDEF_BIT(gdc_lutg42,        16)
REGDEF_BIT(gdc_lutg43,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG22)


/*
    gdc_lutg44:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg45:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG23_OFS 0x0498
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG23)
REGDEF_BIT(gdc_lutg44,        16)
REGDEF_BIT(gdc_lutg45,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG23)


/*
    gdc_lutg46:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg47:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG24_OFS 0x049c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG24)
REGDEF_BIT(gdc_lutg46,        16)
REGDEF_BIT(gdc_lutg47,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG24)


/*
    gdc_lutg48:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg49:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG25_OFS 0x04a0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG25)
REGDEF_BIT(gdc_lutg48,        16)
REGDEF_BIT(gdc_lutg49,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG25)


/*
    gdc_lutg50:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg51:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG26_OFS 0x04a4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG26)
REGDEF_BIT(gdc_lutg50,        16)
REGDEF_BIT(gdc_lutg51,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG26)


/*
    gdc_lutg52:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg53:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG27_OFS 0x04a8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG27)
REGDEF_BIT(gdc_lutg52,        16)
REGDEF_BIT(gdc_lutg53,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG27)


/*
    gdc_lutg54:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg55:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG28_OFS 0x04ac
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG28)
REGDEF_BIT(gdc_lutg54,        16)
REGDEF_BIT(gdc_lutg55,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG28)


/*
    gdc_lutg56:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg57:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG29_OFS 0x04b0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG29)
REGDEF_BIT(gdc_lutg56,        16)
REGDEF_BIT(gdc_lutg57,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG29)


/*
    gdc_lutg58:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg59:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG30_OFS 0x04b4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG30)
REGDEF_BIT(gdc_lutg58,        16)
REGDEF_BIT(gdc_lutg59,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG30)


/*
    gdc_lutg60:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg61:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG31_OFS 0x04b8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG31)
REGDEF_BIT(gdc_lutg60,        16)
REGDEF_BIT(gdc_lutg61,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG31)


/*
    gdc_lutg62:    [0x0, 0xffff],           bits : 15_0
    gdc_lutg63:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG32_OFS 0x04bc
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG32)
REGDEF_BIT(gdc_lutg62,        16)
REGDEF_BIT(gdc_lutg63,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG32)


/*
    gdc_lutg64:    [0x0, 0xffff],           bits : 15_0
*/
#define GEOMETRIC_LOOK_UP_TABLEG33_OFS 0x04c0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG33)
REGDEF_BIT(gdc_lutg64,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG33)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED45_OFS 0x04c4
REGDEF_BEGIN(DCE_RESERVED45)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED45)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED46_OFS 0x04c8
REGDEF_BEGIN(DCE_RESERVED46)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED46)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED47_OFS 0x04cc
REGDEF_BEGIN(DCE_RESERVED47)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED47)


/*
    cac_lutr0:    [0x0, 0x7ff],         bits : 10_0
    cac_lutr1:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER1_OFS 0x04d0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER1)
REGDEF_BIT(cac_lutr0,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr1,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER1)


/*
    cac_lutr2:    [0x0, 0x7ff],         bits : 10_0
    cac_lutr3:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER2_OFS 0x04d4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER2)
REGDEF_BIT(cac_lutr2,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr3,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER2)


/*
    cac_lutr4:    [0x0, 0x7ff],         bits : 10_0
    cac_lutr5:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER3_OFS 0x04d8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER3)
REGDEF_BIT(cac_lutr4,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr5,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER3)


/*
    cac_lutr6:    [0x0, 0x7ff],         bits : 10_0
    cac_lutr7:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER4_OFS 0x04dc
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER4)
REGDEF_BIT(cac_lutr6,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr7,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER4)


/*
    cac_lutr8:    [0x0, 0x7ff],         bits : 10_0
    cac_lutr9:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER5_OFS 0x04e0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER5)
REGDEF_BIT(cac_lutr8,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr9,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER5)


/*
    cac_lutr10:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr11:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER6_OFS 0x04e4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER6)
REGDEF_BIT(cac_lutr10,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr11,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER6)


/*
    cac_lutr12:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr13:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER7_OFS 0x04e8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER7)
REGDEF_BIT(cac_lutr12,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr13,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER7)


/*
    cac_lutr14:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr15:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER8_OFS 0x04ec
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER8)
REGDEF_BIT(cac_lutr14,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr15,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER8)


/*
    cac_lutr16:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr17:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER9_OFS 0x04f0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER9)
REGDEF_BIT(cac_lutr16,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr17,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER9)


/*
    cac_lutr18:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr19:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER10_OFS 0x04f4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER10)
REGDEF_BIT(cac_lutr18,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr19,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER10)


/*
    cac_lutr20:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr21:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER11_OFS 0x04f8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER11)
REGDEF_BIT(cac_lutr20,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr21,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER11)


/*
    cac_lutr22:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr23:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER12_OFS 0x04fc
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER12)
REGDEF_BIT(cac_lutr22,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr23,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER12)


/*
    cac_lutr24:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr25:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER13_OFS 0x0500
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER13)
REGDEF_BIT(cac_lutr24,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr25,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER13)


/*
    cac_lutr26:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr27:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER14_OFS 0x0504
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER14)
REGDEF_BIT(cac_lutr26,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr27,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER14)


/*
    cac_lutr28:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr29:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER15_OFS 0x0508
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER15)
REGDEF_BIT(cac_lutr28,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr29,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER15)


/*
    cac_lutr30:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr31:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER16_OFS 0x050c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER16)
REGDEF_BIT(cac_lutr30,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr31,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER16)


/*
    cac_lutr32:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr33:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER17_OFS 0x0510
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER17)
REGDEF_BIT(cac_lutr32,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr33,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER17)


/*
    cac_lutr34:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr35:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER18_OFS 0x0514
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER18)
REGDEF_BIT(cac_lutr34,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr35,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER18)


/*
    cac_lutr36:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr37:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER19_OFS 0x0518
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER19)
REGDEF_BIT(cac_lutr36,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr37,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER19)


/*
    cac_lutr38:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr39:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER20_OFS 0x051c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER20)
REGDEF_BIT(cac_lutr38,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr39,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER20)


/*
    cac_lutr40:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr41:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER21_OFS 0x0520
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER21)
REGDEF_BIT(cac_lutr40,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr41,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER21)


/*
    cac_lutr42:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr43:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER22_OFS 0x0524
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER22)
REGDEF_BIT(cac_lutr42,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr43,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER22)


/*
    cac_lutr44:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr45:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER23_OFS 0x0528
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER23)
REGDEF_BIT(cac_lutr44,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr45,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER23)


/*
    cac_lutr46:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr47:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER24_OFS 0x052c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER24)
REGDEF_BIT(cac_lutr46,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr47,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER24)


/*
    cac_lutr48:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr49:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER25_OFS 0x0530
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER25)
REGDEF_BIT(cac_lutr48,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr49,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER25)


/*
    cac_lutr50:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr51:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER26_OFS 0x0534
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER26)
REGDEF_BIT(cac_lutr50,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr51,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER26)


/*
    cac_lutr52:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr53:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER27_OFS 0x0538
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER27)
REGDEF_BIT(cac_lutr52,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr53,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER27)


/*
    cac_lutr54:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr55:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER28_OFS 0x053c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER28)
REGDEF_BIT(cac_lutr54,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr55,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER28)


/*
    cac_lutr56:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr57:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER29_OFS 0x0540
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER29)
REGDEF_BIT(cac_lutr56,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr57,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER29)


/*
    cac_lutr58:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr59:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER30_OFS 0x0544
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER30)
REGDEF_BIT(cac_lutr58,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr59,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER30)


/*
    cac_lutr60:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr61:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER31_OFS 0x0548
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER31)
REGDEF_BIT(cac_lutr60,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr61,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER31)


/*
    cac_lutr62:    [0x0, 0x7ff],            bits : 10_0
    cac_lutr63:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLER32_OFS 0x054c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER32)
REGDEF_BIT(cac_lutr62,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutr63,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER32)


/*
    cac_lutr64:    [0x0, 0x7ff],            bits : 10_0
*/
#define GEOMETRIC_LOOK_UP_TABLER33_OFS 0x0550
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLER33)
REGDEF_BIT(cac_lutr64,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLER33)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED48_OFS 0x0554
REGDEF_BEGIN(DCE_RESERVED48)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED48)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED49_OFS 0x0558
REGDEF_BEGIN(DCE_RESERVED49)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED49)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED50_OFS 0x055c
REGDEF_BEGIN(DCE_RESERVED50)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED50)


/*
    cac_lutb0:    [0x0, 0x7ff],         bits : 10_0
    cac_lutb1:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB1_OFS 0x0560
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB1)
REGDEF_BIT(cac_lutb0,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb1,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB1)


/*
    cac_lutb2:    [0x0, 0x7ff],         bits : 10_0
    cac_lutb3:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB2_OFS 0x0564
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB2)
REGDEF_BIT(cac_lutb2,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb3,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB2)


/*
    cac_lutb4:    [0x0, 0x7ff],         bits : 10_0
    cac_lutb5:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB3_OFS 0x0568
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB3)
REGDEF_BIT(cac_lutb4,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb5,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB3)


/*
    cac_lutb6:    [0x0, 0x7ff],         bits : 10_0
    cac_lutb7:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB4_OFS 0x056c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB4)
REGDEF_BIT(cac_lutb6,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb7,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB4)


/*
    cac_lutb8:    [0x0, 0x7ff],         bits : 10_0
    cac_lutb9:    [0x0, 0x7ff],         bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB5_OFS 0x0570
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB5)
REGDEF_BIT(cac_lutb8,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb9,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB5)


/*
    cac_lutb10:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb11:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB6_OFS 0x0574
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB6)
REGDEF_BIT(cac_lutb10,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb11,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB6)


/*
    cac_lutb12:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb13:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB7_OFS 0x0578
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB7)
REGDEF_BIT(cac_lutb12,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb13,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB7)


/*
    cac_lutb14:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb15:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB8_OFS 0x057c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB8)
REGDEF_BIT(cac_lutb14,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb15,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB8)


/*
    cac_lutb16:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb17:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB9_OFS 0x0580
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB9)
REGDEF_BIT(cac_lutb16,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb17,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB9)


/*
    cac_lutb18:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb19:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB10_OFS 0x0584
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB10)
REGDEF_BIT(cac_lutb18,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb19,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB10)


/*
    cac_lutb20:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb21:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB11_OFS 0x0588
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB11)
REGDEF_BIT(cac_lutb20,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb21,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB11)


/*
    cac_lutb22:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb23:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB12_OFS 0x058c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB12)
REGDEF_BIT(cac_lutb22,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb23,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB12)


/*
    cac_lutb24:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb25:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB13_OFS 0x0590
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB13)
REGDEF_BIT(cac_lutb24,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb25,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB13)


/*
    cac_lutb26:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb27:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB14_OFS 0x0594
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB14)
REGDEF_BIT(cac_lutb26,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb27,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB14)


/*
    cac_lutb28:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb29:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB15_OFS 0x0598
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB15)
REGDEF_BIT(cac_lutb28,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb29,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB15)


/*
    cac_lutb30:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb31:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB16_OFS 0x059c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB16)
REGDEF_BIT(cac_lutb30,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb31,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB16)


/*
    cac_lutb32:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb33:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB17_OFS 0x05a0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB17)
REGDEF_BIT(cac_lutb32,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb33,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB17)


/*
    cac_lutb34:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb35:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB18_OFS 0x05a4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB18)
REGDEF_BIT(cac_lutb34,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb35,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB18)


/*
    cac_lutb36:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb37:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB19_OFS 0x05a8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB19)
REGDEF_BIT(cac_lutb36,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb37,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB19)


/*
    cac_lutb38:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb39:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB20_OFS 0x05ac
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB20)
REGDEF_BIT(cac_lutb38,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb39,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB20)


/*
    cac_lutb40:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb41:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB21_OFS 0x05b0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB21)
REGDEF_BIT(cac_lutb40,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb41,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB21)


/*
    cac_lutb42:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb43:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB22_OFS 0x05b4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB22)
REGDEF_BIT(cac_lutb42,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb43,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB22)


/*
    cac_lutb44:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb45:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB23_OFS 0x05b8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB23)
REGDEF_BIT(cac_lutb44,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb45,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB23)


/*
    cac_lutb46:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb47:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB24_OFS 0x05bc
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB24)
REGDEF_BIT(cac_lutb46,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb47,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB24)


/*
    cac_lutb48:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb49:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB25_OFS 0x05c0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB25)
REGDEF_BIT(cac_lutb48,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb49,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB25)


/*
    cac_lutb50:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb51:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB26_OFS 0x05c4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB26)
REGDEF_BIT(cac_lutb50,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb51,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB26)


/*
    cac_lutb52:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb53:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB27_OFS 0x05c8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB27)
REGDEF_BIT(cac_lutb52,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb53,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB27)


/*
    cac_lutb54:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb55:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB28_OFS 0x05cc
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB28)
REGDEF_BIT(cac_lutb54,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb55,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB28)


/*
    cac_lutb56:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb57:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB29_OFS 0x05d0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB29)
REGDEF_BIT(cac_lutb56,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb57,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB29)


/*
    cac_lutb58:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb59:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB30_OFS 0x05d4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB30)
REGDEF_BIT(cac_lutb58,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb59,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB30)


/*
    cac_lutb60:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb61:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB31_OFS 0x05d8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB31)
REGDEF_BIT(cac_lutb60,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb61,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB31)


/*
    cac_lutb62:    [0x0, 0x7ff],            bits : 10_0
    cac_lutb63:    [0x0, 0x7ff],            bits : 26_16
*/
#define GEOMETRIC_LOOK_UP_TABLEB32_OFS 0x05dc
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB32)
REGDEF_BIT(cac_lutb62,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(cac_lutb63,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB32)


/*
    cac_lutb64:    [0x0, 0x7ff],            bits : 10_0
*/
#define GEOMETRIC_LOOK_UP_TABLEB33_OFS 0x05e0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEB33)
REGDEF_BIT(cac_lutb64,        11)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEB33)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED51_OFS 0x05e4
REGDEF_BEGIN(DCE_RESERVED51)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED51)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED52_OFS 0x05e8
REGDEF_BEGIN(DCE_RESERVED52)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED52)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED53_OFS 0x05ec
REGDEF_BEGIN(DCE_RESERVED53)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED53)


/*
    dram_sai2dlut:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_LUT2D_IN_ADDRESS_OFS 0x05f0
REGDEF_BEGIN(DMA_LUT2D_IN_ADDRESS)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_sai2dlut,        30)
REGDEF_END(DMA_LUT2D_IN_ADDRESS)


/*
    lut2d_xofs_int :    [0x0, 0x3f],            bits : 5_0
    lut2d_xofs_frac:    [0x0, 0xffffff],            bits : 31_8
*/
#define LUT2D_REGISTER1_OFS 0x05f4
REGDEF_BEGIN(LUT2D_REGISTER1)
REGDEF_BIT(lut2d_xofs_int,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(lut2d_xofs_frac,        24)
REGDEF_END(LUT2D_REGISTER1)


/*
    lut2d_yofs_int :    [0x0, 0x3f],            bits : 5_0
    lut2d_yofs_frac:    [0x0, 0xffffff],            bits : 31_8
*/
#define LUT2D_REGISTER2_OFS 0x05f8
REGDEF_BEGIN(LUT2D_REGISTER2)
REGDEF_BIT(lut2d_yofs_int,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(lut2d_yofs_frac,        24)
REGDEF_END(LUT2D_REGISTER2)


/*
    lut2d_hfact:    [0x0, 0xffffff],            bits : 23_0
*/
#define LUT2D_REGISTER3_OFS 0x05fc
REGDEF_BEGIN(LUT2D_REGISTER3)
REGDEF_BIT(lut2d_hfact,        24)
REGDEF_END(LUT2D_REGISTER3)


/*
    lut2d_vfact           :    [0x0, 0xffffff],         bits : 23_0
    lut2d_numsel          :    [0x0, 0x3],          bits : 29_28
    lut2d_top_ymin_auto_en:    [0x0, 0x1],          bits : 31
*/
#define LUT2D_REGISTER4_OFS 0x0600
REGDEF_BEGIN(LUT2D_REGISTER4)
REGDEF_BIT(lut2d_vfact,        24)
REGDEF_BIT(,        4)
REGDEF_BIT(lut2d_numsel,        2)
REGDEF_BIT(,        1)
REGDEF_BIT(lut2d_top_ymin_auto_en,        1)
REGDEF_END(LUT2D_REGISTER4)


/*
    Reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_RESERVED54_OFS 0x0604
REGDEF_BEGIN(DCE_RESERVED54)
REGDEF_BIT(Reserved,        32)
REGDEF_END(DCE_RESERVED54)


/*
    ll_cmd_start_addr_info:    [0x0, 0xffffffff],           bits : 31_0
*/
#define LINKED_LIST_REGISTER_1_OFS 0x0608
REGDEF_BEGIN(LINKED_LIST_REGISTER_1)
REGDEF_BIT(ll_cmd_start_addr_info,        32)
REGDEF_END(LINKED_LIST_REGISTER_1)


/*
    ll_cmdprs_cnt:    [0x0, 0xfffff],           bits : 19_0
*/
#define LINKED_LIST_REGISTER_2_OFS 0x060c
REGDEF_BEGIN(LINKED_LIST_REGISTER_2)
REGDEF_BIT(ll_cmdprs_cnt,        20)
REGDEF_END(LINKED_LIST_REGISTER_2)


/*
    dce_version:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VERSION_REGISTER_OFS 0x0610
REGDEF_BEGIN(VERSION_REGISTER)
REGDEF_BIT(dce_version,        32)
REGDEF_END(VERSION_REGISTER)


/*
    dce_checksum_ich:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_CHECKSUM_REGISTER_0_OFS 0x0614
REGDEF_BEGIN(DCE_CHECKSUM_REGISTER_0)
REGDEF_BIT(dce_checksum_ich,        32)
REGDEF_END(DCE_CHECKSUM_REGISTER_0)


/*
    dce_checksum_wdr:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_CHECKSUM_REGISTER_1_OFS 0x0618
REGDEF_BEGIN(DCE_CHECKSUM_REGISTER_1)
REGDEF_BIT(dce_checksum_wdr,        32)
REGDEF_END(DCE_CHECKSUM_REGISTER_1)


/*
    dce_checksum_cfa:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_CHECKSUM_REGISTER_2_OFS 0x061c
REGDEF_BEGIN(DCE_CHECKSUM_REGISTER_2)
REGDEF_BIT(dce_checksum_cfa,        32)
REGDEF_END(DCE_CHECKSUM_REGISTER_2)


/*
    dce_checksum_och:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_CHECKSUM_REGISTER_3_OFS 0x0620
REGDEF_BEGIN(DCE_CHECKSUM_REGISTER_3)
REGDEF_BIT(dce_checksum_och,        32)
REGDEF_END(DCE_CHECKSUM_REGISTER_3)


/*
    dce_checksum_ich2:    [0x0, 0xffffffff],            bits : 31_0
*/
#define DCE_CHECKSUM_REGISTER_4_OFS 0x0624
REGDEF_BEGIN(DCE_CHECKSUM_REGISTER_4)
REGDEF_BIT(dce_checksum_ich2,        32)
REGDEF_END(DCE_CHECKSUM_REGISTER_4)


/*
    dce_checksum_llc:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DCE_CHECKSUM_REGISTER_5_OFS 0x0628
REGDEF_BEGIN(DCE_CHECKSUM_REGISTER_5)
REGDEF_BIT(dce_checksum_llc,        32)
REGDEF_END(DCE_CHECKSUM_REGISTER_5)


//---------- for register buffers ------------
typedef struct {

	union {
		struct {
			unsigned dce_rst                  : 1;        // bits : 0
			unsigned dce_start                : 1;        // bits : 1
			unsigned dce_start_load           : 1;        // bits : 2
			unsigned dce_frmend_load          : 1;        // bits : 3
			unsigned dce_frmstart_load        : 1;        // bits : 4
			unsigned dce_load_global		  : 1;        // bits : 5
			unsigned                          : 22;
			unsigned ll_fire                  : 1;        // bits : 28
		} bit;
		UINT32 word;
	} reg_0; // 0x0000

	union {
		struct {
			unsigned dce_op                         : 2;      // bits : 1_0
			unsigned dce_stp                        : 1;      // bits : 2
			unsigned sram_mode                      : 1;      // bits : 3
			unsigned cfa_en                         : 1;      // bits : 4
			unsigned dc_en                          : 1;      // bits : 5
			unsigned cac_en                         : 1;      // bits : 6
			unsigned yuv2rgb_en                     : 1;      // bits : 7
			unsigned tcurve_en                      : 1;      // bits : 8
			unsigned wdr_en                         : 1;      // bits : 9
			unsigned wdr_subimg_out_en              : 1;      // bits : 10
			unsigned cfa_subimg_out_en              : 1;      // bits : 11
			unsigned cfa_subimg_out_flip_en         : 1;      // bits : 12
			unsigned cfa_pinkr_en                   : 1;      // bits : 13
			unsigned histogram_en                   : 1;      // bits : 14
			unsigned histogram_sel                  : 1;      // bits : 15
			unsigned yuv2rgb_fmt                    : 2;      // bits : 17_16
			unsigned crop_en                        : 1;      // bits : 18
			unsigned dc_sel                         : 1;      // bits : 19
			unsigned chksum_en                      : 1;      // bits : 20
			unsigned d2drand                        : 1;      // bits : 21
			unsigned d2drand_rst                    : 1;      // bits : 22
			unsigned fstr_rst_disable               : 1;      // bits : 23
			unsigned d2dfmt                         : 2;      // bits : 25_24
			unsigned d2d_uv_out_disable             : 1;      // bits : 26
			unsigned d2duvfilt                      : 1;      // bits : 27
			unsigned d2diostop                      : 1;      // bits : 28
			unsigned debug_sel                      : 3;      // bits : 31_29
		} bit;
		UINT32 word;
	} reg_1; // 0x0004

	union {
		struct {
			unsigned inte_frmst            : 1;       // bits : 0
			unsigned inte_frmend           : 1;       // bits : 1
			unsigned inte_stend            : 1;       // bits : 2
			unsigned inte_stp_err          : 1;       // bits : 3
			unsigned inte_lb_ovf           : 1;       // bits : 4
			unsigned                       : 1;
			unsigned inte_stp_ob           : 1;       // bits : 6
			unsigned inte_y_back           : 1;       // bits : 7
			unsigned inte_llend            : 1;       // bits : 8
			unsigned inte_llerror          : 1;       // bits : 9
			unsigned inte_llerror2         : 1;       // bits : 10
			unsigned inte_lljobend         : 1;       // bits : 11
			unsigned inte_frame_err        : 1;       // bits : 12
		} bit;
		UINT32 word;
	} reg_2; // 0x0008

	union {
		struct {
			unsigned int_frmst            : 1;        // bits : 0
			unsigned int_frmend           : 1;        // bits : 1
			unsigned int_stend            : 1;        // bits : 2
			unsigned int_stp_err          : 1;        // bits : 3
			unsigned int_lb_ovf           : 1;        // bits : 4
			unsigned                      : 1;
			unsigned int_stp_ob           : 1;        // bits : 6
			unsigned int_y_back           : 1;        // bits : 7
			unsigned ints_llend           : 1;        // bits : 8
			unsigned ints_llerror         : 1;        // bits : 9
			unsigned int_llerror2         : 1;        // bits : 10
			unsigned int_lljobend         : 1;        // bits : 11
			unsigned int_frame_err        : 1;        // bits : 12
		} bit;
		UINT32 word;
	} reg_3; // 0x000c

	union {
		struct {
			unsigned                     : 2;
			unsigned dce_hsizein         : 11;        // bits : 12_2
			unsigned                     : 4;
			unsigned dce_vsizein         : 12;        // bits : 28_17
		} bit;
		UINT32 word;
	} reg_4; // 0x0010

	union {
		struct {
			unsigned ll_terminate        : 1;     // bits : 0
		} bit;
		UINT32 word;
	} reg_5; // 0x0014

	union {
		struct {
			unsigned dce_dram_out0_single_en        : 1;      // bits : 0
			unsigned dce_dram_out1_single_en        : 1;      // bits : 1
			unsigned                                : 2;
			unsigned dce_dram_out_mode              : 1;      // bits : 4
		} bit;
		UINT32 word;
	} reg_6; // 0x0018

	union {
		struct {
			unsigned                        : 2;
			unsigned dce_dram_sai_ll        : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_7; // 0x001c

	union {
		struct {
			unsigned                  : 2;
			unsigned dram_saiy        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_8; // 0x0020

	union {
		struct {
			unsigned                   : 2;
			unsigned dram_ofsiy        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_9; // 0x0024

	union {
		struct {
			unsigned                   : 2;
			unsigned dram_saiuv        : 30;      // bits : 31_2
		} bit;
		UINT32 word;
	} reg_10; // 0x0028

	union {
		struct {
			unsigned                    : 2;
			unsigned dram_ofsiuv        : 18;     // bits : 19_2
		} bit;
		UINT32 word;
	} reg_11; // 0x002c

	union {
		struct {
			unsigned                  : 2;
			unsigned dram_saoy        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_12; // 0x0030

	union {
		struct {
			unsigned                   : 2;
			unsigned dram_ofsoy        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_13; // 0x0034

	union {
		struct {
			unsigned                   : 2;
			unsigned dram_saouv        : 30;      // bits : 31_2
		} bit;
		UINT32 word;
	} reg_14; // 0x0038

	union {
		struct {
			unsigned                    : 2;
			unsigned dram_ofsouv        : 18;     // bits : 19_2
		} bit;
		UINT32 word;
	} reg_15; // 0x003c

	union {
		struct {
			unsigned dce_status        : 1;       // bits : 0
			unsigned                   : 3;
			unsigned hcnt              : 10;      // bits : 13_4
		} bit;
		UINT32 word;
	} reg_16; // 0x0040

	union {
		struct {
			unsigned hstp0        : 10;       // bits : 9_0
			unsigned hstp1        : 10;       // bits : 19_10
			unsigned hstp2        : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_17; // 0x0044

	union {
		struct {
			unsigned hstp3        : 10;       // bits : 9_0
			unsigned hstp4        : 10;       // bits : 19_10
			unsigned hstp5        : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_18; // 0x0048

	union {
		struct {
			unsigned hstp6        : 10;       // bits : 9_0
			unsigned hstp7        : 10;       // bits : 19_10
			unsigned hstp8        : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_19; // 0x004c

	union {
		struct {
			unsigned hstp9         : 10;      // bits : 9_0
			unsigned hstp10        : 10;      // bits : 19_10
			unsigned hstp11        : 10;      // bits : 29_20
		} bit;
		UINT32 word;
	} reg_20; // 0x0050

	union {
		struct {
			unsigned hstp12        : 10;      // bits : 9_0
			unsigned hstp13        : 10;      // bits : 19_10
			unsigned hstp14        : 10;      // bits : 29_20
		} bit;
		UINT32 word;
	} reg_21; // 0x0054

	union {
		struct {
			unsigned hstp15                    : 10;      // bits : 9_0
			unsigned                           : 2;
			unsigned lbuf_back_rsv_line        : 4;       // bits : 15_12
			unsigned hstp_vmaxdec              : 4;       // bits : 19_16
		} bit;
		UINT32 word;
	} reg_22; // 0x0058

	union {
		struct {
			unsigned hstp_maxinc             : 8;     // bits : 7_0
			unsigned hstp_maxdec             : 8;     // bits : 15_8
			unsigned                         : 2;
			unsigned hstp_ipeolap_sel        : 1;     // bits : 18
			unsigned                         : 1;
			unsigned hstp_imeolap_sel        : 2;     // bits : 21_20
			unsigned                         : 2;
			unsigned hstp_imeolap            : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_23; // 0x005c

	union {
		struct {
			unsigned bufheight_g          : 10;       // bits : 9_0
			unsigned                      : 2;
			unsigned bufheight_rgb        : 10;       // bits : 21_12
			unsigned                      : 2;
			unsigned bufheight_pix        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_24; // 0x0060

	union {
		struct {
			unsigned hstp_stx        : 13;        // bits : 12_0
			unsigned                 : 3;
			unsigned hstp_edx        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_25; // 0x0064

	union {
		struct {
			unsigned hstp_clm_stx        : 13;        // bits : 12_0
			unsigned                     : 3;
			unsigned hstp_clm_edx        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_26; // 0x0068

	union {
		struct {
			unsigned hstp_buf_mode        : 6;        // bits : 5_0
		} bit;
		UINT32 word;
	} reg_27; // 0x006c

	union {
		struct {
			unsigned dma_in_burst         : 1;        // bits : 0
			unsigned dma_out_burst        : 1;        // bits : 1
		} bit;
		UINT32 word;
	} reg_28; // 0x0070

	union {
		struct {
			unsigned dma_ch_disable        : 1;		// bits : 0
			unsigned                       : 3;
			unsigned dma_ch_idle           : 1;		// bits : 4
		} bit;
		UINT32 word;
	} reg_29; // 0x0074

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_30; // 0x0078

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_31; // 0x007c

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_32; // 0x0080

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_33; // 0x0084

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_34; // 0x0088

	union {
		struct {
			unsigned                   : 2;
			unsigned crop_hsize        : 11;      // bits : 12_2
		} bit;
		UINT32 word;
	} reg_35; // 0x008c

	union {
		struct {
			unsigned crop_hstart        : 13;     // bits : 12_0
		} bit;
		UINT32 word;
	} reg_36; // 0x0090

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_37; // 0x0094

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_38; // 0x0098

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_39; // 0x009c

	union {
		struct {
			unsigned cfapat              : 3;     // bits : 2_0
			unsigned bayer_format        : 1;     // bits : 3
		} bit;
		UINT32 word;
	} reg_40; // 0x00a0

	union {
		struct {
			unsigned                            : 2;
			unsigned cfa_subimg_dramsao         : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_41; // 0x00a4

	union {
		struct {
			unsigned                          : 2;
			unsigned cfa_subimg_lofso         : 18;       // bits : 19_2
			unsigned                          : 6;
			unsigned cfa_subimg_chsel         : 2;        // bits : 27_26
			unsigned cfa_subimg_bit           : 2;        // bits : 29_28
			unsigned cfa_subimg_byte          : 1;        // bits : 30
		} bit;
		UINT32 word;
	} reg_42; // 0x00a8

	union {
		struct {
			unsigned cfa_edge_dth         : 12;       // bits : 11_0
			unsigned cfa_edge_dth2        : 12;       // bits : 23_12
		} bit;
		UINT32 word;
	} reg_43; // 0x00ac

	union {
		struct {
			unsigned cfa_rbcth1        : 10;      // bits : 9_0
			unsigned                   : 6;
			unsigned cfa_rbcth2        : 10;      // bits : 25_16
			unsigned                   : 5;
			unsigned cfa_rbcen         : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_44; // 0x00b0

	union {
		struct {
			unsigned cfa_freqblend_lut0        : 4;       // bits : 3_0
			unsigned cfa_freqblend_lut1        : 4;       // bits : 7_4
			unsigned cfa_freqblend_lut2        : 4;       // bits : 11_8
			unsigned cfa_freqblend_lut3        : 4;       // bits : 15_12
			unsigned cfa_freqblend_lut4        : 4;       // bits : 19_16
			unsigned cfa_freqblend_lut5        : 4;       // bits : 23_20
			unsigned cfa_freqblend_lut6        : 4;       // bits : 27_24
			unsigned cfa_freqblend_lut7        : 4;       // bits : 31_28
		} bit;
		UINT32 word;
	} reg_45; // 0x00b4

	union {
		struct {
			unsigned cfa_freqblend_lut8         : 4;      // bits : 3_0
			unsigned cfa_freqblend_lut9         : 4;      // bits : 7_4
			unsigned cfa_freqblend_lut10        : 4;      // bits : 11_8
			unsigned cfa_freqblend_lut11        : 4;      // bits : 15_12
			unsigned cfa_freqblend_lut12        : 4;      // bits : 19_16
			unsigned cfa_freqblend_lut13        : 4;      // bits : 23_20
			unsigned cfa_freqblend_lut14        : 4;      // bits : 27_24
			unsigned cfa_freqblend_lut15        : 4;      // bits : 31_28
		} bit;
		UINT32 word;
	} reg_46; // 0x00b8

	union {
		struct {
			unsigned cfa_luma_weight0        : 8;     // bits : 7_0
			unsigned cfa_luma_weight1        : 8;     // bits : 15_8
			unsigned cfa_luma_weight2        : 8;     // bits : 23_16
			unsigned cfa_luma_weight3        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_47; // 0x00bc

	union {
		struct {
			unsigned cfa_luma_weight4        : 8;     // bits : 7_0
			unsigned cfa_luma_weight5        : 8;     // bits : 15_8
			unsigned cfa_luma_weight6        : 8;     // bits : 23_16
			unsigned cfa_luma_weight7        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_48; // 0x00c0

	union {
		struct {
			unsigned cfa_luma_weight8         : 8;        // bits : 7_0
			unsigned cfa_luma_weight9         : 8;        // bits : 15_8
			unsigned cfa_luma_weight10        : 8;        // bits : 23_16
			unsigned cfa_luma_weight11        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_49; // 0x00c4

	union {
		struct {
			unsigned cfa_luma_weight12        : 8;        // bits : 7_0
			unsigned cfa_luma_weight13        : 8;        // bits : 15_8
			unsigned cfa_luma_weight14        : 8;        // bits : 23_16
			unsigned cfa_luma_weight15        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_50; // 0x00c8

	union {
		struct {
			unsigned cfa_luma_weight16        : 8;        // bits : 7_0
			unsigned                          : 8;
			unsigned cfa_freq_th              : 12;       // bits : 27_16
		} bit;
		UINT32 word;
	} reg_51; // 0x00cc

	union {
		struct {
			unsigned cfa_fcs_coring        : 8;       // bits : 7_0
			unsigned cfa_fcs_weight        : 8;       // bits : 15_8
			unsigned cfa_fcs_dirsel        : 1;       // bits : 16
		} bit;
		UINT32 word;
	} reg_52; // 0x00d0

	union {
		struct {
			unsigned cfa_fcs_strength0        : 4;        // bits : 3_0
			unsigned cfa_fcs_strength1        : 4;        // bits : 7_4
			unsigned cfa_fcs_strength2        : 4;        // bits : 11_8
			unsigned cfa_fcs_strength3        : 4;        // bits : 15_12
			unsigned cfa_fcs_strength4        : 4;        // bits : 19_16
			unsigned cfa_fcs_strength5        : 4;        // bits : 23_20
			unsigned cfa_fcs_strength6        : 4;        // bits : 27_24
			unsigned cfa_fcs_strength7        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_53; // 0x00d4

	union {
		struct {
			unsigned cfa_fcs_strength8         : 4;       // bits : 3_0
			unsigned cfa_fcs_strength9         : 4;       // bits : 7_4
			unsigned cfa_fcs_strength10        : 4;       // bits : 11_8
			unsigned cfa_fcs_strength11        : 4;       // bits : 15_12
			unsigned cfa_fcs_strength12        : 4;       // bits : 19_16
			unsigned cfa_fcs_strength13        : 4;       // bits : 23_20
			unsigned cfa_fcs_strength14        : 4;       // bits : 27_24
			unsigned cfa_fcs_strength15        : 4;       // bits : 31_28
		} bit;
		UINT32 word;
	} reg_54; // 0x00d8

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_55; // 0x00dc

	union {
		struct {
			unsigned cfa_rgbir_cl_en               : 1;       // bits : 0
			unsigned cfa_rgbir_hf_en               : 1;       // bits : 1
			unsigned cfa_rgbir_avg_mode            : 1;       // bits : 2
			unsigned cfa_rgbir_cl_sel              : 1;       // bits : 3
			unsigned                               : 4;
			unsigned cfa_rgbir_cl_thr              : 8;       // bits : 15_8
			unsigned cfa_rgbir_gedge_th            : 10;      // bits : 25_16
			unsigned                               : 2;
			unsigned cfa_rgbir_rb_cstrength        : 3;       // bits : 30_28
		} bit;
		UINT32 word;
	} reg_56; // 0x00e0

	union {
		struct {
			unsigned cfa_rgbir_hf_gthr        : 3;        // bits : 2_0
			unsigned                          : 5;
			unsigned cfa_rgbir_hf_diff        : 8;        // bits : 15_8
			unsigned cfa_rgbir_hf_ethr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_57; // 0x00e4

	union {
		struct {
			unsigned irsub_r        : 12;     // bits : 11_0
			unsigned                : 4;
			unsigned irsub_g        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_58; // 0x00e8

	union {
		struct {
			unsigned irsub_b                   : 12;      // bits : 11_0
			unsigned                           : 2;
			unsigned irsub_weight_range        : 2;       // bits : 15_14
			unsigned irsub_weight_lb           : 8;       // bits : 23_16
			unsigned irsub_th                  : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_59; // 0x00ec

	union {
		struct {
			unsigned cgain_r        : 10;     // bits : 9_0
			unsigned                : 6;
			unsigned cgain_g        : 10;     // bits : 25_16
		} bit;
		UINT32 word;
	} reg_60; // 0x00f0

	union {
		struct {
			unsigned cgain_b            : 10;     // bits : 9_0
			unsigned                    : 6;
			unsigned cgain_range        : 1;      // bits : 16
		} bit;
		UINT32 word;
	} reg_61; // 0x00f4

	union {
		struct {
			unsigned cfa_sat_gain        : 10;        // bits : 9_0
			unsigned                     : 6;
			unsigned pinkr_mode          : 1;     // bits : 16
		} bit;
		UINT32 word;
	} reg_62; // 0x00f8

	union {
		struct {
			unsigned pinkr_thr1        : 8;       // bits : 7_0
			unsigned pinkr_thr2        : 8;       // bits : 15_8
			unsigned pinkr_thr3        : 8;       // bits : 23_16
			unsigned pinkr_thr4        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_63; // 0x00fc

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_64; // 0x0100

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_65; // 0x0104

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_66; // 0x0108

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_67; // 0x010c

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_68; // 0x0110

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_69; // 0x0114

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_70; // 0x0118

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_71; // 0x011c

	union {
		struct {
			unsigned wdr_subimg_width          : 5;       // bits : 4_0
			unsigned wdr_subimg_height         : 5;       // bits : 9_5
			unsigned wdr_rand_reset            : 1;       // bits : 10
			unsigned wdr_rand_sel              : 2;       // bits : 12_11
		} bit;
		UINT32 word;
	} reg_72; // 0x0120

	union {
		struct {
			unsigned                            : 2;
			unsigned wdr_subimg_dramsai         : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_73; // 0x0124

	union {
		struct {
			unsigned                          : 2;
			unsigned wdr_subimg_lofsi         : 14;       // bits : 15_2
		} bit;
		UINT32 word;
	} reg_74; // 0x0128

	union {
		struct {
			unsigned                            : 2;
			unsigned wdr_subimg_dramsao         : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_75; // 0x012c

	union {
		struct {
			unsigned                          : 2;
			unsigned wdr_subimg_lofso         : 14;       // bits : 15_2
		} bit;
		UINT32 word;
	} reg_76; // 0x0130

	union {
		struct {
			unsigned wdr_subimg_hfactor        : 16;      // bits : 15_0
			unsigned wdr_subimg_vfactor        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_77; // 0x0134

	union {
		struct {
			unsigned wdr_sub_blk_sizeh            : 10;       // bits : 9_0
			unsigned wdr_blk_cent_hfactor         : 22;       // bits : 31_10
		} bit;
		UINT32 word;
	} reg_78; // 0x0138

	union {
		struct {
			unsigned wdr_sub_blk_sizev            : 10;       // bits : 9_0
			unsigned wdr_blk_cent_vfactor         : 22;       // bits : 31_10
		} bit;
		UINT32 word;
	} reg_79; // 0x013c

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_80; // 0x0140

	union {
		struct {
			unsigned wdr_input_bldrto0         : 8;       // bits : 7_0
			unsigned wdr_input_bldrto1         : 8;       // bits : 15_8
			unsigned wdr_input_bldrto2         : 8;       // bits : 23_16
			unsigned wdr_input_bldrto3         : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_81; // 0x0144

	union {
		struct {
			unsigned wdr_input_bldrto4          : 8;      // bits : 7_0
			unsigned wdr_input_bldrto5          : 8;      // bits : 15_8
			unsigned wdr_input_bldrto6          : 8;      // bits : 23_16
			unsigned wdr_input_bldrto7          : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_82; // 0x0148

	union {
		struct {
			unsigned wdr_input_bldrto8          : 8;      // bits : 7_0
			unsigned wdr_input_bldrto9          : 8;      // bits : 15_8
			unsigned wdr_input_bldrto10         : 8;      // bits : 23_16
			unsigned wdr_input_bldrto11         : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_83; // 0x014c

	union {
		struct {
			unsigned wdr_input_bldrto12          : 8;     // bits : 7_0
			unsigned wdr_input_bldrto13          : 8;     // bits : 15_8
			unsigned wdr_input_bldrto14          : 8;     // bits : 23_16
			unsigned wdr_input_bldrto15          : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_84; // 0x0150

	union {
		struct {
			unsigned wdr_input_bldrto16           : 8;        // bits : 7_0
			unsigned                              : 18;
			unsigned wdr_input_bldsrc_sel         : 2;        // bits : 27_26
			unsigned wdr_input_bldwt              : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_85; // 0x0154

	union {
		struct {
			unsigned wdr_lpf_c0         : 3;      // bits : 2_0
			unsigned wdr_lpf_c1         : 3;      // bits : 5_3
			unsigned wdr_lpf_c2         : 3;      // bits : 8_6
		} bit;
		UINT32 word;
	} reg_86; // 0x0158

	union {
		struct {
			unsigned wdr_coeff1        : 13;      // bits : 12_0
			unsigned                   : 3;
			unsigned wdr_coeff2        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_87; // 0x015c

	union {
		struct {
			unsigned wdr_coeff3        : 13;      // bits : 12_0
			unsigned                   : 3;
			unsigned wdr_coeff4        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_88; // 0x0160

	union {
		struct {
			unsigned wdr_outbld_table_en         : 1;     // bits : 0
			unsigned wdr_gainctrl_en             : 1;     // bits : 1
			unsigned                             : 14;
			unsigned wdr_maxgain                 : 8;     // bits : 23_16
			unsigned wdr_mingain                 : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_89; // 0x0164

	union {
		struct {
			unsigned wdr_tcurve_index_lut0        : 6;        // bits : 5_0
			unsigned                              : 2;
			unsigned wdr_tcurve_index_lut1        : 6;        // bits : 13_8
			unsigned                              : 2;
			unsigned wdr_tcurve_index_lut2        : 6;        // bits : 21_16
			unsigned                              : 2;
			unsigned wdr_tcurve_index_lut3        : 6;        // bits : 29_24
		} bit;
		UINT32 word;
	} reg_90; // 0x0168

	union {
		struct {
			unsigned wdr_tcurve_index_lut4        : 6;        // bits : 5_0
			unsigned                              : 2;
			unsigned wdr_tcurve_index_lut5        : 6;        // bits : 13_8
			unsigned                              : 2;
			unsigned wdr_tcurve_index_lut6        : 6;        // bits : 21_16
			unsigned                              : 2;
			unsigned wdr_tcurve_index_lut7        : 6;        // bits : 29_24
		} bit;
		UINT32 word;
	} reg_91; // 0x016c

	union {
		struct {
			unsigned wdr_tcurve_index_lut8         : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut9         : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut10        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut11        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_92; // 0x0170

	union {
		struct {
			unsigned wdr_tcurve_index_lut12        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut13        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut14        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut15        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_93; // 0x0174

	union {
		struct {
			unsigned wdr_tcurve_index_lut16        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut17        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut18        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut19        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_94; // 0x0178

	union {
		struct {
			unsigned wdr_tcurve_index_lut20        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut21        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut22        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut23        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_95; // 0x017c

	union {
		struct {
			unsigned wdr_tcurve_index_lut24        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut25        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut26        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut27        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_96; // 0x0180

	union {
		struct {
			unsigned wdr_tcurve_index_lut28        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut29        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut30        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_tcurve_index_lut31        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_97; // 0x0184

	union {
		struct {
			unsigned wdr_tcurve_split_lut0          : 2;      // bits : 1_0
			unsigned wdr_tcurve_split_lut1          : 2;      // bits : 3_2
			unsigned wdr_tcurve_split_lut2          : 2;      // bits : 5_4
			unsigned wdr_tcurve_split_lut3          : 2;      // bits : 7_6
			unsigned wdr_tcurve_split_lut4          : 2;      // bits : 9_8
			unsigned wdr_tcurve_split_lut5          : 2;      // bits : 11_10
			unsigned wdr_tcurve_split_lut6          : 2;      // bits : 13_12
			unsigned wdr_tcurve_split_lut7          : 2;      // bits : 15_14
			unsigned wdr_tcurve_split_lut8          : 2;      // bits : 17_16
			unsigned wdr_tcurve_split_lut9          : 2;      // bits : 19_18
			unsigned wdr_tcurve_split_lut10         : 2;      // bits : 21_20
			unsigned wdr_tcurve_split_lut11         : 2;      // bits : 23_22
			unsigned wdr_tcurve_split_lut12         : 2;      // bits : 25_24
			unsigned wdr_tcurve_split_lut13         : 2;      // bits : 27_26
			unsigned wdr_tcurve_split_lut14         : 2;      // bits : 29_28
			unsigned wdr_tcurve_split_lut15         : 2;      // bits : 31_30
		} bit;
		UINT32 word;
	} reg_98; // 0x0188

	union {
		struct {
			unsigned wdr_tcurve_split_lut16          : 2;     // bits : 1_0
			unsigned wdr_tcurve_split_lut17          : 2;     // bits : 3_2
			unsigned wdr_tcurve_split_lut18          : 2;     // bits : 5_4
			unsigned wdr_tcurve_split_lut19          : 2;     // bits : 7_6
			unsigned wdr_tcurve_split_lut20          : 2;     // bits : 9_8
			unsigned wdr_tcurve_split_lut21          : 2;     // bits : 11_10
			unsigned wdr_tcurve_split_lut22          : 2;     // bits : 13_12
			unsigned wdr_tcurve_split_lut23          : 2;     // bits : 15_14
			unsigned wdr_tcurve_split_lut24          : 2;     // bits : 17_16
			unsigned wdr_tcurve_split_lut25          : 2;     // bits : 19_18
			unsigned wdr_tcurve_split_lut26          : 2;     // bits : 21_20
			unsigned wdr_tcurve_split_lut27          : 2;     // bits : 23_22
			unsigned wdr_tcurve_split_lut28          : 2;     // bits : 25_24
			unsigned wdr_tcurve_split_lut29          : 2;     // bits : 27_26
			unsigned wdr_tcurve_split_lut30          : 2;     // bits : 29_28
			unsigned wdr_tcurve_split_lut31          : 2;     // bits : 31_30
		} bit;
		UINT32 word;
	} reg_99; // 0x018c

	union {
		struct {
			unsigned wdr_tcurve_val_lut0        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_tcurve_val_lut1        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_100; // 0x0190

	union {
		struct {
			unsigned wdr_tcurve_val_lut2        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_tcurve_val_lut3        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_101; // 0x0194

	union {
		struct {
			unsigned wdr_tcurve_val_lut4        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_tcurve_val_lut5        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_102; // 0x0198

	union {
		struct {
			unsigned wdr_tcurve_val_lut6        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_tcurve_val_lut7        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_103; // 0x019c

	union {
		struct {
			unsigned wdr_tcurve_val_lut8        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_tcurve_val_lut9        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_104; // 0x01a0

	union {
		struct {
			unsigned wdr_tcurve_val_lut10        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut11        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_105; // 0x01a4

	union {
		struct {
			unsigned wdr_tcurve_val_lut12        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut13        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_106; // 0x01a8

	union {
		struct {
			unsigned wdr_tcurve_val_lut14        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut15        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_107; // 0x01ac

	union {
		struct {
			unsigned wdr_tcurve_val_lut16        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut17        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_108; // 0x01b0

	union {
		struct {
			unsigned wdr_tcurve_val_lut18        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut19        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_109; // 0x01b4

	union {
		struct {
			unsigned wdr_tcurve_val_lut20        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut21        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_110; // 0x01b8

	union {
		struct {
			unsigned wdr_tcurve_val_lut22        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut23        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_111; // 0x01bc

	union {
		struct {
			unsigned wdr_tcurve_val_lut24        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut25        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_112; // 0x01c0

	union {
		struct {
			unsigned wdr_tcurve_val_lut26        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut27        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_113; // 0x01c4

	union {
		struct {
			unsigned wdr_tcurve_val_lut28        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut29        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_114; // 0x01c8

	union {
		struct {
			unsigned wdr_tcurve_val_lut30        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut31        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_115; // 0x01cc

	union {
		struct {
			unsigned wdr_tcurve_val_lut32        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut33        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_116; // 0x01d0

	union {
		struct {
			unsigned wdr_tcurve_val_lut34        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut35        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_117; // 0x01d4

	union {
		struct {
			unsigned wdr_tcurve_val_lut36        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut37        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_118; // 0x01d8

	union {
		struct {
			unsigned wdr_tcurve_val_lut38        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut39        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_119; // 0x01dc

	union {
		struct {
			unsigned wdr_tcurve_val_lut40        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut41        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_120; // 0x01e0

	union {
		struct {
			unsigned wdr_tcurve_val_lut42        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut43        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_121; // 0x01e4

	union {
		struct {
			unsigned wdr_tcurve_val_lut44        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut45        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_122; // 0x01e8

	union {
		struct {
			unsigned wdr_tcurve_val_lut46        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut47        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_123; // 0x01ec

	union {
		struct {
			unsigned wdr_tcurve_val_lut48        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut49        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_124; // 0x01f0

	union {
		struct {
			unsigned wdr_tcurve_val_lut50        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut51        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_125; // 0x01f4

	union {
		struct {
			unsigned wdr_tcurve_val_lut52        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut53        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_126; // 0x01f8

	union {
		struct {
			unsigned wdr_tcurve_val_lut54        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut55        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_127; // 0x01fc

	union {
		struct {
			unsigned wdr_tcurve_val_lut56        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut57        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_128; // 0x0200

	union {
		struct {
			unsigned wdr_tcurve_val_lut58        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut59        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_129; // 0x0204

	union {
		struct {
			unsigned wdr_tcurve_val_lut60        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut61        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_130; // 0x0208

	union {
		struct {
			unsigned wdr_tcurve_val_lut62        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_tcurve_val_lut63        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_131; // 0x020c

	union {
		struct {
			unsigned wdr_tcurve_val_lut64        : 12;        // bits : 11_0
		} bit;
		UINT32 word;
	} reg_132; // 0x0210

	union {
		struct {
			unsigned wdr_outbld_index_lut0        : 6;        // bits : 5_0
			unsigned                              : 2;
			unsigned wdr_outbld_index_lut1        : 6;        // bits : 13_8
			unsigned                              : 2;
			unsigned wdr_outbld_index_lut2        : 6;        // bits : 21_16
			unsigned                              : 2;
			unsigned wdr_outbld_index_lut3        : 6;        // bits : 29_24
		} bit;
		UINT32 word;
	} reg_133; // 0x0214

	union {
		struct {
			unsigned wdr_outbld_index_lut4        : 6;        // bits : 5_0
			unsigned                              : 2;
			unsigned wdr_outbld_index_lut5        : 6;        // bits : 13_8
			unsigned                              : 2;
			unsigned wdr_outbld_index_lut6        : 6;        // bits : 21_16
			unsigned                              : 2;
			unsigned wdr_outbld_index_lut7        : 6;        // bits : 29_24
		} bit;
		UINT32 word;
	} reg_134; // 0x0218

	union {
		struct {
			unsigned wdr_outbld_index_lut8         : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut9         : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut10        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut11        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_135; // 0x021c

	union {
		struct {
			unsigned wdr_outbld_index_lut12        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut13        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut14        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut15        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_136; // 0x0220

	union {
		struct {
			unsigned wdr_outbld_index_lut16        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut17        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut18        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut19        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_137; // 0x0224

	union {
		struct {
			unsigned wdr_outbld_index_lut20        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut21        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut22        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut23        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_138; // 0x0228

	union {
		struct {
			unsigned wdr_outbld_index_lut24        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut25        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut26        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut27        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_139; // 0x022c

	union {
		struct {
			unsigned wdr_outbld_index_lut28        : 6;       // bits : 5_0
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut29        : 6;       // bits : 13_8
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut30        : 6;       // bits : 21_16
			unsigned                               : 2;
			unsigned wdr_outbld_index_lut31        : 6;       // bits : 29_24
		} bit;
		UINT32 word;
	} reg_140; // 0x0230

	union {
		struct {
			unsigned wdr_outbld_split_lut0         : 2;       // bits : 1_0
			unsigned wdr_outbld_split_lut1         : 2;       // bits : 3_2
			unsigned wdr_outbld_split_lut2         : 2;       // bits : 5_4
			unsigned wdr_outbld_split_lut3         : 2;       // bits : 7_6
			unsigned wdr_outbld_split_lut4         : 2;       // bits : 9_8
			unsigned wdr_outbld_split_lut5         : 2;       // bits : 11_10
			unsigned wdr_outbld_split_lut6         : 2;       // bits : 13_12
			unsigned wdr_outbld_split_lut7         : 2;       // bits : 15_14
			unsigned wdr_outbld_split_lut8         : 2;       // bits : 17_16
			unsigned wdr_outbld_split_lut9         : 2;       // bits : 19_18
			unsigned wdr_outbld_split_lut10        : 2;       // bits : 21_20
			unsigned wdr_outbld_split_lut11        : 2;       // bits : 23_22
			unsigned wdr_outbld_split_lut12        : 2;       // bits : 25_24
			unsigned wdr_outbld_split_lut13        : 2;       // bits : 27_26
			unsigned wdr_outbld_split_lut14        : 2;       // bits : 29_28
			unsigned wdr_outbld_split_lut15        : 2;       // bits : 31_30
		} bit;
		UINT32 word;
	} reg_141; // 0x0234

	union {
		struct {
			unsigned wdr_outbld_split_lut16        : 2;       // bits : 1_0
			unsigned wdr_outbld_split_lut17        : 2;       // bits : 3_2
			unsigned wdr_outbld_split_lut18        : 2;       // bits : 5_4
			unsigned wdr_outbld_split_lut19        : 2;       // bits : 7_6
			unsigned wdr_outbld_split_lut20        : 2;       // bits : 9_8
			unsigned wdr_outbld_split_lut21        : 2;       // bits : 11_10
			unsigned wdr_outbld_split_lut22        : 2;       // bits : 13_12
			unsigned wdr_outbld_split_lut23        : 2;       // bits : 15_14
			unsigned wdr_outbld_split_lut24        : 2;       // bits : 17_16
			unsigned wdr_outbld_split_lut25        : 2;       // bits : 19_18
			unsigned wdr_outbld_split_lut26        : 2;       // bits : 21_20
			unsigned wdr_outbld_split_lut27        : 2;       // bits : 23_22
			unsigned wdr_outbld_split_lut28        : 2;       // bits : 25_24
			unsigned wdr_outbld_split_lut29        : 2;       // bits : 27_26
			unsigned wdr_outbld_split_lut30        : 2;       // bits : 29_28
			unsigned wdr_outbld_split_lut31        : 2;       // bits : 31_30
		} bit;
		UINT32 word;
	} reg_142; // 0x0238

	union {
		struct {
			unsigned wdr_outbld_val_lut0        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_outbld_val_lut1        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_143; // 0x023c

	union {
		struct {
			unsigned wdr_outbld_val_lut2        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_outbld_val_lut3        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_144; // 0x0240

	union {
		struct {
			unsigned wdr_outbld_val_lut4        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_outbld_val_lut5        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_145; // 0x0244

	union {
		struct {
			unsigned wdr_outbld_val_lut6        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_outbld_val_lut7        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_146; // 0x0248

	union {
		struct {
			unsigned wdr_outbld_val_lut8        : 12;     // bits : 11_0
			unsigned                            : 4;
			unsigned wdr_outbld_val_lut9        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_147; // 0x024c

	union {
		struct {
			unsigned wdr_outbld_val_lut10        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut11        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_148; // 0x0250

	union {
		struct {
			unsigned wdr_outbld_val_lut12        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut13        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_149; // 0x0254

	union {
		struct {
			unsigned wdr_outbld_val_lut14        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut15        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_150; // 0x0258

	union {
		struct {
			unsigned wdr_outbld_val_lut16        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut17        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_151; // 0x025c

	union {
		struct {
			unsigned wdr_outbld_val_lut18        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut19        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_152; // 0x0260

	union {
		struct {
			unsigned wdr_outbld_val_lut20        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut21        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_153; // 0x0264

	union {
		struct {
			unsigned wdr_outbld_val_lut22        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut23        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_154; // 0x0268

	union {
		struct {
			unsigned wdr_outbld_val_lut24        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut25        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_155; // 0x026c

	union {
		struct {
			unsigned wdr_outbld_val_lut26        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut27        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_156; // 0x0270

	union {
		struct {
			unsigned wdr_outbld_val_lut28        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut29        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_157; // 0x0274

	union {
		struct {
			unsigned wdr_outbld_val_lut30        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut31        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_158; // 0x0278

	union {
		struct {
			unsigned wdr_outbld_val_lut32        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut33        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_159; // 0x027c

	union {
		struct {
			unsigned wdr_outbld_val_lut34        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut35        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_160; // 0x0280

	union {
		struct {
			unsigned wdr_outbld_val_lut36        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut37        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_161; // 0x0284

	union {
		struct {
			unsigned wdr_outbld_val_lut38        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut39        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_162; // 0x0288

	union {
		struct {
			unsigned wdr_outbld_val_lut40        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut41        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_163; // 0x028c

	union {
		struct {
			unsigned wdr_outbld_val_lut42        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut43        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_164; // 0x0290

	union {
		struct {
			unsigned wdr_outbld_val_lut44        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut45        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_165; // 0x0294

	union {
		struct {
			unsigned wdr_outbld_val_lut46        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut47        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_166; // 0x0298

	union {
		struct {
			unsigned wdr_outbld_val_lut48        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut49        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_167; // 0x029c

	union {
		struct {
			unsigned wdr_outbld_val_lut50        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut51        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_168; // 0x02a0

	union {
		struct {
			unsigned wdr_outbld_val_lut52        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut53        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_169; // 0x02a4

	union {
		struct {
			unsigned wdr_outbld_val_lut54        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut55        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_170; // 0x02a8

	union {
		struct {
			unsigned wdr_outbld_val_lut56        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut57        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_171; // 0x02ac

	union {
		struct {
			unsigned wdr_outbld_val_lut58        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut59        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_172; // 0x02b0

	union {
		struct {
			unsigned wdr_outbld_val_lut60        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut61        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_173; // 0x02b4

	union {
		struct {
			unsigned wdr_outbld_val_lut62        : 12;        // bits : 11_0
			unsigned                             : 4;
			unsigned wdr_outbld_val_lut63        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_174; // 0x02b8

	union {
		struct {
			unsigned wdr_outbld_val_lut64        : 12;        // bits : 11_0
		} bit;
		UINT32 word;
	} reg_175; // 0x02bc

	union {
		struct {
			unsigned wdr_strength         : 8;        // bits : 7_0
			unsigned wdr_contrast         : 8;        // bits : 15_8
		} bit;
		UINT32 word;
	} reg_176; // 0x02c0

	union {
		struct {
			unsigned wdr_sat_th            : 12;      // bits : 11_0
			unsigned                       : 4;
			unsigned wdr_sat_wt_low        : 8;       // bits : 23_16
			unsigned wdr_sat_delta         : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_177; // 0x02c4

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_178; // 0x02c8

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_179; // 0x02cc

	union {
		struct {
			unsigned histogram_h_step        : 5;     // bits : 4_0
			unsigned                         : 11;
			unsigned histogram_v_step        : 5;     // bits : 20_16
		} bit;
		UINT32 word;
	} reg_180; // 0x02d0

	union {
		struct {
			unsigned histogram_bin0        : 16;      // bits : 15_0
			unsigned histogram_bin1        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_181; // 0x02d4

	union {
		struct {
			unsigned histogram_bin2        : 16;      // bits : 15_0
			unsigned histogram_bin3        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_182; // 0x02d8

	union {
		struct {
			unsigned histogram_bin4        : 16;      // bits : 15_0
			unsigned histogram_bin5        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_183; // 0x02dc

	union {
		struct {
			unsigned histogram_bin6        : 16;      // bits : 15_0
			unsigned histogram_bin7        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_184; // 0x02e0

	union {
		struct {
			unsigned histogram_bin8        : 16;      // bits : 15_0
			unsigned histogram_bin9        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_185; // 0x02e4

	union {
		struct {
			unsigned histogram_bin10        : 16;     // bits : 15_0
			unsigned histogram_bin11        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_186; // 0x02e8

	union {
		struct {
			unsigned histogram_bin12        : 16;     // bits : 15_0
			unsigned histogram_bin13        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_187; // 0x02ec

	union {
		struct {
			unsigned histogram_bin14        : 16;     // bits : 15_0
			unsigned histogram_bin15        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_188; // 0x02f0

	union {
		struct {
			unsigned histogram_bin16        : 16;     // bits : 15_0
			unsigned histogram_bin17        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_189; // 0x02f4

	union {
		struct {
			unsigned histogram_bin18        : 16;     // bits : 15_0
			unsigned histogram_bin19        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_190; // 0x02f8

	union {
		struct {
			unsigned histogram_bin20        : 16;     // bits : 15_0
			unsigned histogram_bin21        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_191; // 0x02fc

	union {
		struct {
			unsigned histogram_bin22        : 16;     // bits : 15_0
			unsigned histogram_bin23        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_192; // 0x0300

	union {
		struct {
			unsigned histogram_bin24        : 16;     // bits : 15_0
			unsigned histogram_bin25        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_193; // 0x0304

	union {
		struct {
			unsigned histogram_bin26        : 16;     // bits : 15_0
			unsigned histogram_bin27        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_194; // 0x0308

	union {
		struct {
			unsigned histogram_bin28        : 16;     // bits : 15_0
			unsigned histogram_bin29        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_195; // 0x030c

	union {
		struct {
			unsigned histogram_bin30        : 16;     // bits : 15_0
			unsigned histogram_bin31        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_196; // 0x0310

	union {
		struct {
			unsigned histogram_bin32        : 16;     // bits : 15_0
			unsigned histogram_bin33        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_197; // 0x0314

	union {
		struct {
			unsigned histogram_bin34        : 16;     // bits : 15_0
			unsigned histogram_bin35        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_198; // 0x0318

	union {
		struct {
			unsigned histogram_bin36        : 16;     // bits : 15_0
			unsigned histogram_bin37        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_199; // 0x031c

	union {
		struct {
			unsigned histogram_bin38        : 16;     // bits : 15_0
			unsigned histogram_bin39        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_200; // 0x0320

	union {
		struct {
			unsigned histogram_bin40        : 16;     // bits : 15_0
			unsigned histogram_bin41        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_201; // 0x0324

	union {
		struct {
			unsigned histogram_bin42        : 16;     // bits : 15_0
			unsigned histogram_bin43        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_202; // 0x0328

	union {
		struct {
			unsigned histogram_bin44        : 16;     // bits : 15_0
			unsigned histogram_bin45        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_203; // 0x032c

	union {
		struct {
			unsigned histogram_bin46        : 16;     // bits : 15_0
			unsigned histogram_bin47        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_204; // 0x0330

	union {
		struct {
			unsigned histogram_bin48        : 16;     // bits : 15_0
			unsigned histogram_bin49        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_205; // 0x0334

	union {
		struct {
			unsigned histogram_bin50        : 16;     // bits : 15_0
			unsigned histogram_bin51        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_206; // 0x0338

	union {
		struct {
			unsigned histogram_bin52        : 16;     // bits : 15_0
			unsigned histogram_bin53        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_207; // 0x033c

	union {
		struct {
			unsigned histogram_bin54        : 16;     // bits : 15_0
			unsigned histogram_bin55        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_208; // 0x0340

	union {
		struct {
			unsigned histogram_bin56        : 16;     // bits : 15_0
			unsigned histogram_bin57        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_209; // 0x0344

	union {
		struct {
			unsigned histogram_bin58        : 16;     // bits : 15_0
			unsigned histogram_bin59        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_210; // 0x0348

	union {
		struct {
			unsigned histogram_bin60        : 16;     // bits : 15_0
			unsigned histogram_bin61        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_211; // 0x034c

	union {
		struct {
			unsigned histogram_bin62        : 16;     // bits : 15_0
			unsigned histogram_bin63        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_212; // 0x0350

	union {
		struct {
			unsigned histogram_bin64        : 16;     // bits : 15_0
			unsigned histogram_bin65        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_213; // 0x0354

	union {
		struct {
			unsigned histogram_bin66        : 16;     // bits : 15_0
			unsigned histogram_bin67        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_214; // 0x0358

	union {
		struct {
			unsigned histogram_bin68        : 16;     // bits : 15_0
			unsigned histogram_bin69        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_215; // 0x035c

	union {
		struct {
			unsigned histogram_bin70        : 16;     // bits : 15_0
			unsigned histogram_bin71        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_216; // 0x0360

	union {
		struct {
			unsigned histogram_bin72        : 16;     // bits : 15_0
			unsigned histogram_bin73        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_217; // 0x0364

	union {
		struct {
			unsigned histogram_bin74        : 16;     // bits : 15_0
			unsigned histogram_bin75        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_218; // 0x0368

	union {
		struct {
			unsigned histogram_bin76        : 16;     // bits : 15_0
			unsigned histogram_bin77        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_219; // 0x036c

	union {
		struct {
			unsigned histogram_bin78        : 16;     // bits : 15_0
			unsigned histogram_bin79        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_220; // 0x0370

	union {
		struct {
			unsigned histogram_bin80        : 16;     // bits : 15_0
			unsigned histogram_bin81        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_221; // 0x0374

	union {
		struct {
			unsigned histogram_bin82        : 16;     // bits : 15_0
			unsigned histogram_bin83        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_222; // 0x0378

	union {
		struct {
			unsigned histogram_bin84        : 16;     // bits : 15_0
			unsigned histogram_bin85        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_223; // 0x037c

	union {
		struct {
			unsigned histogram_bin86        : 16;     // bits : 15_0
			unsigned histogram_bin87        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_224; // 0x0380

	union {
		struct {
			unsigned histogram_bin88        : 16;     // bits : 15_0
			unsigned histogram_bin89        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_225; // 0x0384

	union {
		struct {
			unsigned histogram_bin90        : 16;     // bits : 15_0
			unsigned histogram_bin91        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_226; // 0x0388

	union {
		struct {
			unsigned histogram_bin92        : 16;     // bits : 15_0
			unsigned histogram_bin93        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_227; // 0x038c

	union {
		struct {
			unsigned histogram_bin94        : 16;     // bits : 15_0
			unsigned histogram_bin95        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_228; // 0x0390

	union {
		struct {
			unsigned histogram_bin96        : 16;     // bits : 15_0
			unsigned histogram_bin97        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_229; // 0x0394

	union {
		struct {
			unsigned histogram_bin98        : 16;     // bits : 15_0
			unsigned histogram_bin99        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_230; // 0x0398

	union {
		struct {
			unsigned histogram_bin100        : 16;        // bits : 15_0
			unsigned histogram_bin101        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_231; // 0x039c

	union {
		struct {
			unsigned histogram_bin102        : 16;        // bits : 15_0
			unsigned histogram_bin103        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_232; // 0x03a0

	union {
		struct {
			unsigned histogram_bin104        : 16;        // bits : 15_0
			unsigned histogram_bin105        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_233; // 0x03a4

	union {
		struct {
			unsigned histogram_bin106        : 16;        // bits : 15_0
			unsigned histogram_bin107        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_234; // 0x03a8

	union {
		struct {
			unsigned histogram_bin108        : 16;        // bits : 15_0
			unsigned histogram_bin109        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_235; // 0x03ac

	union {
		struct {
			unsigned histogram_bin110        : 16;        // bits : 15_0
			unsigned histogram_bin111        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_236; // 0x03b0

	union {
		struct {
			unsigned histogram_bin112        : 16;        // bits : 15_0
			unsigned histogram_bin113        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_237; // 0x03b4

	union {
		struct {
			unsigned histogram_bin114        : 16;        // bits : 15_0
			unsigned histogram_bin115        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_238; // 0x03b8

	union {
		struct {
			unsigned histogram_bin116        : 16;        // bits : 15_0
			unsigned histogram_bin117        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_239; // 0x03bc

	union {
		struct {
			unsigned histogram_bin118        : 16;        // bits : 15_0
			unsigned histogram_bin119        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_240; // 0x03c0

	union {
		struct {
			unsigned histogram_bin120        : 16;        // bits : 15_0
			unsigned histogram_bin121        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_241; // 0x03c4

	union {
		struct {
			unsigned histogram_bin122        : 16;        // bits : 15_0
			unsigned histogram_bin123        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_242; // 0x03c8

	union {
		struct {
			unsigned histogram_bin124        : 16;        // bits : 15_0
			unsigned histogram_bin125        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_243; // 0x03cc

	union {
		struct {
			unsigned histogram_bin126        : 16;        // bits : 15_0
			unsigned histogram_bin127        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_244; // 0x03d0

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_245; // 0x03d4

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_246; // 0x03d8

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_247; // 0x03dc

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_248; // 0x03e0

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_249; // 0x03e4

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_250; // 0x03e8

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_251; // 0x03ec

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_252; // 0x03f0

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_253; // 0x03f4

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_254; // 0x03f8

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_255; // 0x03fc

	union {
		struct {
			unsigned                 : 1;
			unsigned gdc_mode        : 1;     // bits : 1
		} bit;
		UINT32 word;
	} reg_256; // 0x0400

	union {
		struct {
			unsigned gdc_centx        : 14;       // bits : 13_0
			unsigned                  : 2;
			unsigned gdc_centy        : 14;       // bits : 29_16
		} bit;
		UINT32 word;
	} reg_257; // 0x0404

	union {
		struct {
			unsigned gdc_xdist        : 12;       // bits : 11_0
			unsigned gdc_ydist        : 12;       // bits : 23_12
		} bit;
		UINT32 word;
	} reg_258; // 0x0408

	union {
		struct {
			unsigned gdc_normfact            : 8;     // bits : 7_0
			unsigned gdc_normbit             : 5;     // bits : 12_8
			unsigned                         : 3;
			unsigned gdc_normfact_10b        : 10;        // bits : 25_16
			unsigned                         : 2;
			unsigned gdc_normfact_sel        : 1;     // bits : 28
		} bit;
		UINT32 word;
	} reg_259; // 0x040c

	union {
		struct {
			unsigned gdc_fovgain         : 12;        // bits : 11_0
			unsigned gdc_fovbound        : 1;     // bits : 12
		} bit;
		UINT32 word;
	} reg_260; // 0x0410

	union {
		struct {
			unsigned gdc_boundr        : 10;      // bits : 9_0
			unsigned gdc_boundg        : 10;      // bits : 19_10
			unsigned gdc_boundb        : 10;      // bits : 29_20
		} bit;
		UINT32 word;
	} reg_261; // 0x0414

	union {
		struct {
			unsigned cac_rlutgain        : 13;        // bits : 12_0
			unsigned                     : 3;
			unsigned cac_glutgain        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_262; // 0x0418

	union {
		struct {
			unsigned cac_blutgain        : 13;        // bits : 12_0
			unsigned                     : 3;
			unsigned cac_sel             : 1;     // bits : 16
		} bit;
		UINT32 word;
	} reg_263; // 0x041c

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_264; // 0x0420

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_265; // 0x0424

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_266; // 0x0428

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_267; // 0x042c

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_268; // 0x0430

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_269; // 0x0434

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_270; // 0x0438

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_271; // 0x043c

	union {
		struct {
			unsigned gdc_lutg0        : 16;       // bits : 15_0
			unsigned gdc_lutg1        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_272; // 0x0440

	union {
		struct {
			unsigned gdc_lutg2        : 16;       // bits : 15_0
			unsigned gdc_lutg3        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_273; // 0x0444

	union {
		struct {
			unsigned gdc_lutg4        : 16;       // bits : 15_0
			unsigned gdc_lutg5        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_274; // 0x0448

	union {
		struct {
			unsigned gdc_lutg6        : 16;       // bits : 15_0
			unsigned gdc_lutg7        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_275; // 0x044c

	union {
		struct {
			unsigned gdc_lutg8        : 16;       // bits : 15_0
			unsigned gdc_lutg9        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_276; // 0x0450

	union {
		struct {
			unsigned gdc_lutg10        : 16;      // bits : 15_0
			unsigned gdc_lutg11        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_277; // 0x0454

	union {
		struct {
			unsigned gdc_lutg12        : 16;      // bits : 15_0
			unsigned gdc_lutg13        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_278; // 0x0458

	union {
		struct {
			unsigned gdc_lutg14        : 16;      // bits : 15_0
			unsigned gdc_lutg15        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_279; // 0x045c

	union {
		struct {
			unsigned gdc_lutg16        : 16;      // bits : 15_0
			unsigned gdc_lutg17        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_280; // 0x0460

	union {
		struct {
			unsigned gdc_lutg18        : 16;      // bits : 15_0
			unsigned gdc_lutg19        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_281; // 0x0464

	union {
		struct {
			unsigned gdc_lutg20        : 16;      // bits : 15_0
			unsigned gdc_lutg21        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_282; // 0x0468

	union {
		struct {
			unsigned gdc_lutg22        : 16;      // bits : 15_0
			unsigned gdc_lutg23        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_283; // 0x046c

	union {
		struct {
			unsigned gdc_lutg24        : 16;      // bits : 15_0
			unsigned gdc_lutg25        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_284; // 0x0470

	union {
		struct {
			unsigned gdc_lutg26        : 16;      // bits : 15_0
			unsigned gdc_lutg27        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_285; // 0x0474

	union {
		struct {
			unsigned gdc_lutg28        : 16;      // bits : 15_0
			unsigned gdc_lutg29        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_286; // 0x0478

	union {
		struct {
			unsigned gdc_lutg30        : 16;      // bits : 15_0
			unsigned gdc_lutg31        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_287; // 0x047c

	union {
		struct {
			unsigned gdc_lutg32        : 16;      // bits : 15_0
			unsigned gdc_lutg33        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_288; // 0x0480

	union {
		struct {
			unsigned gdc_lutg34        : 16;      // bits : 15_0
			unsigned gdc_lutg35        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_289; // 0x0484

	union {
		struct {
			unsigned gdc_lutg36        : 16;      // bits : 15_0
			unsigned gdc_lutg37        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_290; // 0x0488

	union {
		struct {
			unsigned gdc_lutg38        : 16;      // bits : 15_0
			unsigned gdc_lutg39        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_291; // 0x048c

	union {
		struct {
			unsigned gdc_lutg40        : 16;      // bits : 15_0
			unsigned gdc_lutg41        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_292; // 0x0490

	union {
		struct {
			unsigned gdc_lutg42        : 16;      // bits : 15_0
			unsigned gdc_lutg43        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_293; // 0x0494

	union {
		struct {
			unsigned gdc_lutg44        : 16;      // bits : 15_0
			unsigned gdc_lutg45        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_294; // 0x0498

	union {
		struct {
			unsigned gdc_lutg46        : 16;      // bits : 15_0
			unsigned gdc_lutg47        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_295; // 0x049c

	union {
		struct {
			unsigned gdc_lutg48        : 16;      // bits : 15_0
			unsigned gdc_lutg49        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_296; // 0x04a0

	union {
		struct {
			unsigned gdc_lutg50        : 16;      // bits : 15_0
			unsigned gdc_lutg51        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_297; // 0x04a4

	union {
		struct {
			unsigned gdc_lutg52        : 16;      // bits : 15_0
			unsigned gdc_lutg53        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_298; // 0x04a8

	union {
		struct {
			unsigned gdc_lutg54        : 16;      // bits : 15_0
			unsigned gdc_lutg55        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_299; // 0x04ac

	union {
		struct {
			unsigned gdc_lutg56        : 16;      // bits : 15_0
			unsigned gdc_lutg57        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_300; // 0x04b0

	union {
		struct {
			unsigned gdc_lutg58        : 16;      // bits : 15_0
			unsigned gdc_lutg59        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_301; // 0x04b4

	union {
		struct {
			unsigned gdc_lutg60        : 16;      // bits : 15_0
			unsigned gdc_lutg61        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_302; // 0x04b8

	union {
		struct {
			unsigned gdc_lutg62        : 16;      // bits : 15_0
			unsigned gdc_lutg63        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_303; // 0x04bc

	union {
		struct {
			unsigned gdc_lutg64        : 16;      // bits : 15_0
		} bit;
		UINT32 word;
	} reg_304; // 0x04c0

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_305; // 0x04c4

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_306; // 0x04c8

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_307; // 0x04cc

	union {
		struct {
			unsigned cac_lutr0        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutr1        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_308; // 0x04d0

	union {
		struct {
			unsigned cac_lutr2        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutr3        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_309; // 0x04d4

	union {
		struct {
			unsigned cac_lutr4        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutr5        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_310; // 0x04d8

	union {
		struct {
			unsigned cac_lutr6        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutr7        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_311; // 0x04dc

	union {
		struct {
			unsigned cac_lutr8        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutr9        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_312; // 0x04e0

	union {
		struct {
			unsigned cac_lutr10        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr11        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_313; // 0x04e4

	union {
		struct {
			unsigned cac_lutr12        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr13        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_314; // 0x04e8

	union {
		struct {
			unsigned cac_lutr14        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr15        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_315; // 0x04ec

	union {
		struct {
			unsigned cac_lutr16        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr17        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_316; // 0x04f0

	union {
		struct {
			unsigned cac_lutr18        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr19        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_317; // 0x04f4

	union {
		struct {
			unsigned cac_lutr20        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr21        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_318; // 0x04f8

	union {
		struct {
			unsigned cac_lutr22        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr23        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_319; // 0x04fc

	union {
		struct {
			unsigned cac_lutr24        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr25        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_320; // 0x0500

	union {
		struct {
			unsigned cac_lutr26        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr27        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_321; // 0x0504

	union {
		struct {
			unsigned cac_lutr28        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr29        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_322; // 0x0508

	union {
		struct {
			unsigned cac_lutr30        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr31        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_323; // 0x050c

	union {
		struct {
			unsigned cac_lutr32        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr33        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_324; // 0x0510

	union {
		struct {
			unsigned cac_lutr34        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr35        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_325; // 0x0514

	union {
		struct {
			unsigned cac_lutr36        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr37        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_326; // 0x0518

	union {
		struct {
			unsigned cac_lutr38        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr39        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_327; // 0x051c

	union {
		struct {
			unsigned cac_lutr40        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr41        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_328; // 0x0520

	union {
		struct {
			unsigned cac_lutr42        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr43        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_329; // 0x0524

	union {
		struct {
			unsigned cac_lutr44        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr45        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_330; // 0x0528

	union {
		struct {
			unsigned cac_lutr46        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr47        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_331; // 0x052c

	union {
		struct {
			unsigned cac_lutr48        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr49        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_332; // 0x0530

	union {
		struct {
			unsigned cac_lutr50        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr51        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_333; // 0x0534

	union {
		struct {
			unsigned cac_lutr52        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr53        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_334; // 0x0538

	union {
		struct {
			unsigned cac_lutr54        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr55        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_335; // 0x053c

	union {
		struct {
			unsigned cac_lutr56        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr57        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_336; // 0x0540

	union {
		struct {
			unsigned cac_lutr58        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr59        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_337; // 0x0544

	union {
		struct {
			unsigned cac_lutr60        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr61        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_338; // 0x0548

	union {
		struct {
			unsigned cac_lutr62        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutr63        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_339; // 0x054c

	union {
		struct {
			unsigned cac_lutr64        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_340; // 0x0550

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_341; // 0x0554

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_342; // 0x0558

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_343; // 0x055c

	union {
		struct {
			unsigned cac_lutb0        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutb1        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_344; // 0x0560

	union {
		struct {
			unsigned cac_lutb2        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutb3        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_345; // 0x0564

	union {
		struct {
			unsigned cac_lutb4        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutb5        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_346; // 0x0568

	union {
		struct {
			unsigned cac_lutb6        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutb7        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_347; // 0x056c

	union {
		struct {
			unsigned cac_lutb8        : 11;       // bits : 10_0
			unsigned                  : 5;
			unsigned cac_lutb9        : 11;       // bits : 26_16
		} bit;
		UINT32 word;
	} reg_348; // 0x0570

	union {
		struct {
			unsigned cac_lutb10        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb11        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_349; // 0x0574

	union {
		struct {
			unsigned cac_lutb12        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb13        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_350; // 0x0578

	union {
		struct {
			unsigned cac_lutb14        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb15        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_351; // 0x057c

	union {
		struct {
			unsigned cac_lutb16        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb17        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_352; // 0x0580

	union {
		struct {
			unsigned cac_lutb18        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb19        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_353; // 0x0584

	union {
		struct {
			unsigned cac_lutb20        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb21        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_354; // 0x0588

	union {
		struct {
			unsigned cac_lutb22        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb23        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_355; // 0x058c

	union {
		struct {
			unsigned cac_lutb24        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb25        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_356; // 0x0590

	union {
		struct {
			unsigned cac_lutb26        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb27        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_357; // 0x0594

	union {
		struct {
			unsigned cac_lutb28        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb29        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_358; // 0x0598

	union {
		struct {
			unsigned cac_lutb30        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb31        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_359; // 0x059c

	union {
		struct {
			unsigned cac_lutb32        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb33        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_360; // 0x05a0

	union {
		struct {
			unsigned cac_lutb34        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb35        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_361; // 0x05a4

	union {
		struct {
			unsigned cac_lutb36        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb37        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_362; // 0x05a8

	union {
		struct {
			unsigned cac_lutb38        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb39        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_363; // 0x05ac

	union {
		struct {
			unsigned cac_lutb40        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb41        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_364; // 0x05b0

	union {
		struct {
			unsigned cac_lutb42        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb43        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_365; // 0x05b4

	union {
		struct {
			unsigned cac_lutb44        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb45        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_366; // 0x05b8

	union {
		struct {
			unsigned cac_lutb46        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb47        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_367; // 0x05bc

	union {
		struct {
			unsigned cac_lutb48        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb49        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_368; // 0x05c0

	union {
		struct {
			unsigned cac_lutb50        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb51        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_369; // 0x05c4

	union {
		struct {
			unsigned cac_lutb52        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb53        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_370; // 0x05c8

	union {
		struct {
			unsigned cac_lutb54        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb55        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_371; // 0x05cc

	union {
		struct {
			unsigned cac_lutb56        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb57        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_372; // 0x05d0

	union {
		struct {
			unsigned cac_lutb58        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb59        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_373; // 0x05d4

	union {
		struct {
			unsigned cac_lutb60        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb61        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_374; // 0x05d8

	union {
		struct {
			unsigned cac_lutb62        : 11;      // bits : 10_0
			unsigned                   : 5;
			unsigned cac_lutb63        : 11;      // bits : 26_16
		} bit;
		UINT32 word;
	} reg_375; // 0x05dc

	union {
		struct {
			unsigned cac_lutb64        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_376; // 0x05e0

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_377; // 0x05e4

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_378; // 0x05e8

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_379; // 0x05ec

	union {
		struct {
			unsigned                      : 2;
			unsigned dram_sai2dlut        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_380; // 0x05f0

	union {
		struct {
			unsigned lut2d_xofs_int         : 6;      // bits : 5_0
			unsigned                        : 2;
			unsigned lut2d_xofs_frac        : 24;     // bits : 31_8
		} bit;
		UINT32 word;
	} reg_381; // 0x05f4

	union {
		struct {
			unsigned lut2d_yofs_int         : 6;      // bits : 5_0
			unsigned                        : 2;
			unsigned lut2d_yofs_frac        : 24;     // bits : 31_8
		} bit;
		UINT32 word;
	} reg_382; // 0x05f8

	union {
		struct {
			unsigned lut2d_hfact        : 24;     // bits : 23_0
		} bit;
		UINT32 word;
	} reg_383; // 0x05fc

	union {
		struct {
			unsigned lut2d_vfact                   : 24;      // bits : 23_0
			unsigned                               : 4;
			unsigned lut2d_numsel                  : 2;       // bits : 29_28
			unsigned                               : 1;
			unsigned lut2d_top_ymin_auto_en        : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_384; // 0x0600

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_385; // 0x0604

	union {
		struct {
			unsigned ll_cmd_start_addr_info        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_386; // 0x0608

	union {
		struct {
			unsigned ll_cmdprs_cnt        : 20;       // bits : 19_0
		} bit;
		UINT32 word;
	} reg_387; // 0x060c

	union {
		struct {
			unsigned dce_version        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_388; // 0x0610

	union {
		struct {
			unsigned dce_checksum_ich        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_389; // 0x0614

	union {
		struct {
			unsigned dce_checksum_wdr        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_390; // 0x0618

	union {
		struct {
			unsigned dce_checksum_cfa        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_391; // 0x061c

	union {
		struct {
			unsigned dce_checksum_och        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_392; // 0x0620

	union {
		struct {
			unsigned dce_checksum_ich2        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_393; // 0x0624

} NT98560_DCE_REGISTER_STRUCT;

#ifdef __cplusplus
}
#endif

#endif
