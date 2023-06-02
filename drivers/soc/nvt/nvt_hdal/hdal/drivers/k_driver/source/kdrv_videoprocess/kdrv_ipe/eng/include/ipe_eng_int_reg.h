#ifndef _IPE_INT_REGISTER_H_
#define _IPE_INT_REGISTER_H_

#ifdef __cplusplus
extern "C" {
#endif


#if defined (__LINUX)

#include "mach/rcw_macro.h"

#include "plat/top.h"
#include "kwrap/type.h"

//=========================================================================
#elif defined (__FREERTOS)


#include "rcw_macro.h"
#include "plat/top.h"
#include "kwrap/type.h"

#else


#endif




/*
    ipe_swrst          :    [0x0, 0x1],         bits : 0
    ipe_start          :    [0x0, 0x1],         bits : 1
    ipe_load_start     :    [0x0, 0x1],         bits : 2
    ipe_load_fd        :    [0x0, 0x1],         bits : 3
    ipe_load_fs        :    [0x0, 0x1],         bits : 4
    ipe_gbl_load_en    :    [0x0, 0x1],         bits : 5
    ipe_rwgamma        :    [0x0, 0x3],         bits : 9_8
    ipe_rwgamma_opt    :    [0x0, 0x3],         bits : 11_10
    ipe_dmach_dis      :    [0x0, 0x1],         bits : 27
    ll_fire            :    [0x0, 0x1],         bits : 28
*/
#define IPE_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(IPE_CONTROL_REGISTER)
REGDEF_BIT(ipe_swrst,           1)
REGDEF_BIT(ipe_start,           1)
REGDEF_BIT(ipe_load_start,      1)
REGDEF_BIT(ipe_load_fd,         1)
REGDEF_BIT(ipe_load_fs,         1)
REGDEF_BIT(ipe_gbl_load_en,     1)
REGDEF_BIT(,                        2)
REGDEF_BIT(ipe_rwgamma,         2)
REGDEF_BIT(ipe_rwgamma_opt,     2)
REGDEF_BIT(,                       15)
REGDEF_BIT(ipe_dmach_dis,       1)
REGDEF_BIT(ll_fire,             1)
REGDEF_END(IPE_CONTROL_REGISTER)


/*
    ipe_mode       :    [0x0, 0x3],         bits : 1_0
    ipe_imeo_en    :    [0x0, 0x1],         bits : 2
    ipe_dmao_en    :    [0x0, 0x1],         bits : 3
    dmao_sel       :    [0x0, 0x3],         bits : 5_4
    ipe_imat       :    [0x0, 0x3],         bits : 9_8
    ipe_omat       :    [0x0, 0x3],         bits : 11_10
    ipe_subhsel    :    [0x0, 0x1],         bits : 12
    ipe_mst_hovlp  :    [0x0, 0x1],         bits : 27
    iny_burst_mode :    [0x0, 0x1],         bits : 28
    inc_burst_mode :    [0x0, 0x1],         bits : 29
    outy_burst_mode:    [0x0, 0x1],         bits : 30
    outc_burst_mode:    [0x0, 0x1],         bits : 31
*/
#define IPE_MODE_REGISTER_0_OFS 0x0004
REGDEF_BEGIN(IPE_MODE_REGISTER_0)
REGDEF_BIT(ipe_mode,        2)
REGDEF_BIT(ipe_imeo_en,        1)
REGDEF_BIT(ipe_dmao_en,        1)
REGDEF_BIT(dmao_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ipe_imat,        2)
REGDEF_BIT(ipe_omat,        2)
REGDEF_BIT(ipe_subhsel,        1)
REGDEF_BIT(,        14)
REGDEF_BIT(ipe_mst_hovlp,        1)
REGDEF_BIT(iny_burst_mode,        1)
REGDEF_BIT(inc_burst_mode,        1)
REGDEF_BIT(outy_burst_mode,        1)
REGDEF_BIT(outc_burst_mode,        1)
REGDEF_END(IPE_MODE_REGISTER_0)


/*
    rgbgamma_en      :    [0x0, 0x1],           bits : 1
    ycurve_en        :    [0x0, 0x1],           bits : 2
    cr_en            :    [0x0, 0x1],           bits : 3
    lce_subimg_out_en:    [0x0, 0x1],           bits : 4
    lce_en           :    [0x0, 0x1],           bits : 6
    cst_en           :    [0x0, 0x1],           bits : 7
    ctrl_en          :    [0x0, 0x1],           bits : 9
    hadj_en          :    [0x0, 0x1],           bits : 10
    cadj_en          :    [0x0, 0x1],           bits : 11
    cadj_yenh_en     :    [0x0, 0x1],           bits : 12
    cadj_ycon_en     :    [0x0, 0x1],           bits : 13
    cadj_ccon_en     :    [0x0, 0x1],           bits : 14
    cadj_ycth_en     :    [0x0, 0x1],           bits : 15
    cstprotect_en    :    [0x0, 0x1],           bits : 16
    edge_dbg_en      :    [0x0, 0x1],           bits : 17
    vaccen           :    [0x0, 0x1],           bits : 19
    win0_vaen        :    [0x0, 0x1],           bits : 20
    win1_vaen        :    [0x0, 0x1],           bits : 21
    win2_vaen        :    [0x0, 0x1],           bits : 22
    win3_vaen        :    [0x0, 0x1],           bits : 23
    win4_vaen        :    [0x0, 0x1],           bits : 24
*/
#define IPE_MODE_REGISTER_1_OFS 0x0008
REGDEF_BEGIN(IPE_MODE_REGISTER_1)
REGDEF_BIT(,        1)
REGDEF_BIT(rgbgamma_en,        1)
REGDEF_BIT(ycurve_en,        1)
REGDEF_BIT(cr_en,        1)
REGDEF_BIT(lce_subimg_out_en,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(lce_en,        1)
REGDEF_BIT(cst_en,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ctrl_en,        1)
REGDEF_BIT(hadj_en,        1)
REGDEF_BIT(cadj_en,        1)
REGDEF_BIT(cadj_yenh_en,        1)
REGDEF_BIT(cadj_ycon_en,        1)
REGDEF_BIT(cadj_ccon_en,        1)
REGDEF_BIT(cadj_ycth_en,        1)
REGDEF_BIT(cstprotect_en,        1)
REGDEF_BIT(edge_dbg_en,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(vaccen,        1)
REGDEF_BIT(win0_vaen,        1)
REGDEF_BIT(win1_vaen,        1)
REGDEF_BIT(win2_vaen,        1)
REGDEF_BIT(win3_vaen,        1)
REGDEF_BIT(win4_vaen,        1)
REGDEF_END(IPE_MODE_REGISTER_1)


/*
    ipe_dram_out0_single_en:    [0x0, 0x1],         bits : 0
    ipe_dram_out1_single_en:    [0x0, 0x1],         bits : 1
    ipe_dram_out2_single_en:    [0x0, 0x1],         bits : 2
    ipe_dram_out3_single_en:    [0x0, 0x1],         bits : 3
    ipe_dram_out_mode      :    [0x0, 0x1],         bits : 4
*/
#define DMA_TO_IPE_REGISTER_0_OFS 0x000c
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_0)
REGDEF_BIT(ipe_dram_out0_single_en,        1)
REGDEF_BIT(ipe_dram_out1_single_en,        1)
REGDEF_BIT(ipe_dram_out2_single_en,        1)
REGDEF_BIT(ipe_dram_out3_single_en,        1)
REGDEF_BIT(ipe_dram_out_mode,        1)
REGDEF_END(DMA_TO_IPE_REGISTER_0)


/*
    ll_terminate:    [0x0, 0x1],            bits : 0
    ipe_dmach_idle:  [0x0, 0x1],            bits : 4
*/
#define LINKED_LIST_REGISTER_OFS 0x0010
REGDEF_BEGIN(LINKED_LIST_REGISTER)
REGDEF_BIT(ll_terminate,        1)
REGDEF_BIT(,                3)
REGDEF_BIT(ipe_dmach_idle,        1)
REGDEF_END(LINKED_LIST_REGISTER)


/*
    hn:    [0x0, 0x7ff],            bits : 10_0
    hl:    [0x0, 0x7ff],            bits : 22_12
    hm:    [0x0, 0xf],          bits : 27_24
*/
#define IPE_STRIPE_REGISTER_0_OFS 0x0014
REGDEF_BEGIN(IPE_STRIPE_REGISTER_0)
REGDEF_BIT(hn,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(hl,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(hm,        4)
REGDEF_END(IPE_STRIPE_REGISTER_0)


/*
    vl:    [0x0, 0x1fff],           bits : 12_0
*/
#define IPE_STRIPE_REGISTER_1_OFS 0x0018
REGDEF_BEGIN(IPE_STRIPE_REGISTER_1)
REGDEF_BIT(vl,        13)
REGDEF_END(IPE_STRIPE_REGISTER_1)


/*
    dram_sai_ll:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_1_OFS 0x001c
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_1)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_sai_ll,        30)
REGDEF_END(DMA_TO_IPE_REGISTER_1)


/*
    dram_sai_y:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_2_OFS 0x0020
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_2)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_sai_y,             30)
REGDEF_END(DMA_TO_IPE_REGISTER_2)


/*
    dram_sai_c:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_3_OFS 0x0024
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_3)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_sai_c,             30)
REGDEF_END(DMA_TO_IPE_REGISTER_3)


/*
    dram_sai_ycurve:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_4_OFS 0x0028
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_4)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_sai_ycurve,        30)
REGDEF_END(DMA_TO_IPE_REGISTER_4)


/*
    dram_sai_gamma:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_5_OFS 0x002c
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_5)
REGDEF_BIT(,                           2)
REGDEF_BIT(dram_sai_gamma,        30)
REGDEF_END(DMA_TO_IPE_REGISTER_5)


/*
    dram_ofsi_y:    [0x0, 0x3fff],            bits : 15_2
    inrand_en  :    [0x0, 0x1],           bits : 28
    inrand_rst :    [0x0, 0x1],           bits : 29
*/
#define DMA_TO_IPE_REGISTER_6_OFS 0x0030
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_6)
REGDEF_BIT(,                        2)
REGDEF_BIT(dram_ofsi_y,        14)
REGDEF_BIT(,                       12)
REGDEF_BIT(inrand_en,           1)
REGDEF_BIT(inrand_rst,          1)
REGDEF_END(DMA_TO_IPE_REGISTER_6)


/*
    dram_ofsi_c:    [0x0, 0x3fff],            bits : 15_2
*/
#define DMA_TO_IPE_REGISTER_7_OFS 0x0034
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_7)
REGDEF_BIT(,                        2)
REGDEF_BIT(dram_ofsi_c,        14)
REGDEF_END(DMA_TO_IPE_REGISTER_7)


/*
    dram_sao_y:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IPE_TO_DMA_YCC_CHANNEL_REGISTER_0_OFS 0x0038
REGDEF_BEGIN(IPE_TO_DMA_YCC_CHANNEL_REGISTER_0)
REGDEF_BIT(,                       2)
REGDEF_BIT(dram_sao_y,        30)
REGDEF_END(IPE_TO_DMA_YCC_CHANNEL_REGISTER_0)


/*
    dram_sao_c:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IPE_TO_DMA_YCC_CHANNEL_REGISTER_1_OFS 0x003c
REGDEF_BEGIN(IPE_TO_DMA_YCC_CHANNEL_REGISTER_1)
REGDEF_BIT(,                       2)
REGDEF_BIT(dram_sao_c,        30)
REGDEF_END(IPE_TO_DMA_YCC_CHANNEL_REGISTER_1)


/*
    dram_ofso_y:    [0x0, 0x3fff],           bits : 15_2
*/
#define IPE_TO_DMA_YCC_CHANNEL_REGISTER_2_OFS 0x0040
REGDEF_BEGIN(IPE_TO_DMA_YCC_CHANNEL_REGISTER_2)
REGDEF_BIT(,                        2)
REGDEF_BIT(dram_ofso_y,        14)
REGDEF_END(IPE_TO_DMA_YCC_CHANNEL_REGISTER_2)


/*
    dram_ofso_c:    [0x0, 0x3fff],           bits : 15_2
*/
#define IPE_TO_DMA_YCC_CHANNEL_REGISTER_3_OFS 0x0044
REGDEF_BEGIN(IPE_TO_DMA_YCC_CHANNEL_REGISTER_3)
REGDEF_BIT(,                        2)
REGDEF_BIT(dram_ofso_c,        14)
REGDEF_END(IPE_TO_DMA_YCC_CHANNEL_REGISTER_3)


/*
    dram_sao_va:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IPE_TO_DMA_VA_CHANNEL_REGISTER_0_OFS 0x0048
REGDEF_BEGIN(IPE_TO_DMA_VA_CHANNEL_REGISTER_0)
REGDEF_BIT(,                        2)
REGDEF_BIT(dram_sao_va,        30)
REGDEF_END(IPE_TO_DMA_VA_CHANNEL_REGISTER_0)


/*
    dram_ofso_va:    [0x0, 0x3fff],          bits : 15_2
*/
#define IPE_TO_DMA_VA_CHANNEL_REGISTER_1_OFS 0x004c
REGDEF_BEGIN(IPE_TO_DMA_VA_CHANNEL_REGISTER_1)
REGDEF_BIT(,                         2)
REGDEF_BIT(dram_ofso_va,        14)
REGDEF_END(IPE_TO_DMA_VA_CHANNEL_REGISTER_1)


/*
    ipe_inte_frm_end       :    [0x0, 0x1],         bits : 1
    ipe_inte_stp_end       :    [0x0, 0x1],         bits : 2
    ipe_inte_frm_start     :    [0x0, 0x1],         bits : 3
    ipe_inte_ycout_end     :    [0x0, 0x1],         bits : 4
    ipe_inte_dmain0_end    :    [0x0, 0x1],         bits : 5
    ipe_inte_lce_subout_end:    [0x0, 0x1],         bits : 6
    ipe_inte_vaout_end     :    [0x0, 0x1],         bits : 7
    ipe_inte_ll_done       :    [0x0, 0x1],         bits : 8
    ipe_inte_ll_job_end    :    [0x0, 0x1],         bits : 9
    ipe_inte_ll_error      :    [0x0, 0x1],         bits : 10
    ipe_inte_ll_error2     :    [0x0, 0x1],         bits : 11
*/
#define IPE_INTERRUPT_ENABLE_REGISTER_OFS 0x0050
REGDEF_BEGIN(IPE_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(,        1)
REGDEF_BIT(ipe_inte_frm_end,        1)
REGDEF_BIT(ipe_inte_stp_end,        1)
REGDEF_BIT(ipe_inte_frm_start,        1)
REGDEF_BIT(ipe_inte_ycout_end,        1)
REGDEF_BIT(ipe_inte_dmain0_end,        1)
REGDEF_BIT(ipe_inte_lce_subout_end,        1)
REGDEF_BIT(ipe_inte_vaout_end,        1)
REGDEF_BIT(ipe_inte_ll_done,        1)
REGDEF_BIT(ipe_inte_ll_job_end,        1)
REGDEF_BIT(ipe_inte_ll_error,        1)
REGDEF_BIT(ipe_inte_ll_error2,        1)
REGDEF_END(IPE_INTERRUPT_ENABLE_REGISTER)


/*
    ipe_ints_frm_end       :    [0x0, 0x1],         bits : 1
    ipe_ints_stp_end       :    [0x0, 0x1],         bits : 2
    ipe_ints_frm_start     :    [0x0, 0x1],         bits : 3
    ipe_ints_ycout_end     :    [0x0, 0x1],         bits : 4
    ipe_ints_dmain0_end    :    [0x0, 0x1],         bits : 5
    ipe_ints_lce_subout_end:    [0x0, 0x1],         bits : 6
    ipe_ints_vaout_end     :    [0x0, 0x1],         bits : 7
    ipe_ints_ll_done       :    [0x0, 0x1],         bits : 8
    ipe_ints_ll_job_end    :    [0x0, 0x1],         bits : 9
    ipe_ints_ll_error      :    [0x0, 0x1],         bits : 10
    ipe_ints_ll_error2     :    [0x0, 0x1],         bits : 11
*/
#define IPE_INTERRUPT_STATUS_REGISTER_OFS 0x0054
REGDEF_BEGIN(IPE_INTERRUPT_STATUS_REGISTER)
REGDEF_BIT(,        1)
REGDEF_BIT(ipe_ints_frm_end,        1)
REGDEF_BIT(ipe_ints_stp_end,        1)
REGDEF_BIT(ipe_ints_frm_start,        1)
REGDEF_BIT(ipe_ints_ycout_end,        1)
REGDEF_BIT(ipe_ints_dmain0_end,        1)
REGDEF_BIT(ipe_ints_lce_subout_end,        1)
REGDEF_BIT(ipe_ints_vaout_end,        1)
REGDEF_BIT(ipe_ints_ll_done,        1)
REGDEF_BIT(ipe_ints_ll_job_end,        1)
REGDEF_BIT(ipe_ints_ll_error,        1)
REGDEF_BIT(ipe_ints_ll_error2,        1)
REGDEF_END(IPE_INTERRUPT_STATUS_REGISTER)


/*
    ipestatus        :    [0x0, 0x1],           bits : 0
    hcnt             :    [0x0, 0xf],           bits : 7_4
    status_y         :    [0x0, 0x1],           bits : 12
    status_c         :    [0x0, 0x1],           bits : 13
    fstr_rst_disable :    [0x0, 0x1],           bits : 16
    sram0_light_sleep:    [0x0, 0x1],           bits : 24
    sram1_light_sleep:    [0x0, 0x1],           bits : 25
    sram2_light_sleep:    [0x0, 0x1],           bits : 26
    sram3_light_sleep:    [0x0, 0x1],           bits : 27
    sram4_light_sleep:    [0x0, 0x1],           bits : 28
    sram5_light_sleep:    [0x0, 0x1],           bits : 29
    sram6_light_sleep:    [0x0, 0x1],           bits : 30
    sram7_light_sleep:    [0x0, 0x1],           bits : 31
*/
#define DEBUG_STATUS_REGISTER_OFS 0x0058
REGDEF_BEGIN(DEBUG_STATUS_REGISTER)
REGDEF_BIT(ipestatus,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(hcnt,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(status_y,        1)
REGDEF_BIT(status_c,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(fstr_rst_disable,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(sram0_light_sleep,        1)
REGDEF_BIT(sram1_light_sleep,        1)
REGDEF_BIT(sram2_light_sleep,        1)
REGDEF_BIT(sram3_light_sleep,        1)
REGDEF_BIT(sram4_light_sleep,        1)
REGDEF_BIT(sram5_light_sleep,        1)
REGDEF_BIT(sram6_light_sleep,        1)
REGDEF_BIT(sram7_light_sleep,        1)
REGDEF_END(DEBUG_STATUS_REGISTER)


/*
    ll_table_idx_0:    [0x0, 0xff],         bits : 7_0
    ll_table_idx_1:    [0x0, 0xff],         bits : 15_8
    ll_table_idx_2:    [0x0, 0xff],         bits : 23_16
    ll_table_idx_3:    [0x0, 0xff],         bits : 31_24
*/
#define LL_FRAME_REGISTER_0_OFS 0x005c
REGDEF_BEGIN(LL_FRAME_REGISTER_0)
REGDEF_BIT(ll_table_idx_0,        8)
REGDEF_BIT(ll_table_idx_1,        8)
REGDEF_BIT(ll_table_idx_2,        8)
REGDEF_BIT(ll_table_idx_3,        8)
REGDEF_END(LL_FRAME_REGISTER_0)


/*
    ll_table_idx_4:    [0x0, 0xff],         bits : 7_0
    ll_table_idx_5:    [0x0, 0xff],         bits : 15_8
    ll_table_idx_6:    [0x0, 0xff],         bits : 23_16
    ll_table_idx_7:    [0x0, 0xff],         bits : 31_24
*/
#define LL_FRAME_REGISTER_1_OFS 0x0060
REGDEF_BEGIN(LL_FRAME_REGISTER_1)
REGDEF_BIT(ll_table_idx_4,        8)
REGDEF_BIT(ll_table_idx_5,        8)
REGDEF_BIT(ll_table_idx_6,        8)
REGDEF_BIT(ll_table_idx_7,        8)
REGDEF_END(LL_FRAME_REGISTER_1)


/*
    ll_table_idx_8 :    [0x0, 0xff],            bits : 7_0
    ll_table_idx_9 :    [0x0, 0xff],            bits : 15_8
    ll_table_idx_10:    [0x0, 0xff],            bits : 23_16
    ll_table_idx_11:    [0x0, 0xff],            bits : 31_24
*/
#define LL_FRAME_REGISTER_2_OFS 0x0064
REGDEF_BEGIN(LL_FRAME_REGISTER_2)
REGDEF_BIT(ll_table_idx_8,        8)
REGDEF_BIT(ll_table_idx_9,        8)
REGDEF_BIT(ll_table_idx_10,        8)
REGDEF_BIT(ll_table_idx_11,        8)
REGDEF_END(LL_FRAME_REGISTER_2)


/*
    ll_table_idx_12:    [0x0, 0xff],            bits : 7_0
    ll_table_idx_13:    [0x0, 0xff],            bits : 15_8
    ll_table_idx_14:    [0x0, 0xff],            bits : 23_16
    ll_table_idx_15:    [0x0, 0xff],            bits : 31_24
*/
#define LL_FRAME_REGISTER_3_OFS 0x0068
REGDEF_BEGIN(LL_FRAME_REGISTER_3)
REGDEF_BIT(ll_table_idx_12,        8)
REGDEF_BIT(ll_table_idx_13,        8)
REGDEF_BIT(ll_table_idx_14,        8)
REGDEF_BIT(ll_table_idx_15,        8)
REGDEF_END(LL_FRAME_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER1_OFS 0x006c
REGDEF_BEGIN(IPE_RESERVED_REGISTER1)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER1)


/*
    eext_chsel :    [0x0, 0x1],         bits : 0
    eext_gamsel:    [0x0, 0x1],         bits : 1
    eextdiv_e7 :    [0x0, 0xf],         bits : 7_4
    eext_enh_e7:    [0x0, 0x1f],            bits : 12_8
    eext0      :    [0x0, 0x3ff],           bits : 25_16
*/
#define EDGE_EXTRACTION_REGISTER_0_OFS 0x0070
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_0)
REGDEF_BIT(eext_chsel,        1)
REGDEF_BIT(eext_gamsel,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(eextdiv_e7,        4)
REGDEF_BIT(eext_enh_e7,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(eext0,        10)
REGDEF_END(EDGE_EXTRACTION_REGISTER_0)


/*
    eext1:    [0x0, 0x3ff],         bits : 9_0
    eext2:    [0x0, 0x3ff],         bits : 19_10
    eext3:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_EXTRACTION_REGISTER_1_OFS 0x0074
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_1)
REGDEF_BIT(eext1,        10)
REGDEF_BIT(eext2,        10)
REGDEF_BIT(eext3,        10)
REGDEF_END(EDGE_EXTRACTION_REGISTER_1)


/*
    eext4:    [0x0, 0x3ff],         bits : 9_0
    eext5:    [0x0, 0x3ff],         bits : 19_10
    eext6:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_EXTRACTION_REGISTER_2_OFS 0x0078
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_2)
REGDEF_BIT(eext4,        10)
REGDEF_BIT(eext5,        10)
REGDEF_BIT(eext6,        10)
REGDEF_END(EDGE_EXTRACTION_REGISTER_2)


/*
    eext7:    [0x0, 0x3ff],         bits : 9_0
    eext8:    [0x0, 0x3ff],         bits : 19_10
    eext9:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_EXTRACTION_REGISTER_3_OFS 0x007c
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_3)
REGDEF_BIT(eext7,        10)
REGDEF_BIT(eext8,        10)
REGDEF_BIT(eext9,        10)
REGDEF_END(EDGE_EXTRACTION_REGISTER_3)


/*
    eext_enh_e3 :    [0x0, 0x3f],           bits : 5_0
    eext_enh_e5a:    [0x0, 0x3f],           bits : 13_8
    eext_enh_e5b:    [0x0, 0x3f],           bits : 21_16
*/
#define EDGE_EXTRACTION_REGISTER_4_OFS 0x0080
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_4)
REGDEF_BIT(eext_enh_e3,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(eext_enh_e5a,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(eext_enh_e5b,        6)
REGDEF_END(EDGE_EXTRACTION_REGISTER_4)


/*
    eextdiv_e3 :    [0x0, 0x7],         bits : 2_0
    eextdiv_e5a:    [0x0, 0x7],         bits : 6_4
    eextdiv_e5b:    [0x0, 0x7],         bits : 10_8
    eextdiv_eng:    [0x0, 0xf],         bits : 15_12
    eextdiv_con:    [0x0, 0xf],         bits : 19_16
*/
#define EDGE_EXTRACTION_REGISTER_5_OFS 0x0084
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_5)
REGDEF_BIT(eextdiv_e3,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(eextdiv_e5a,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(eextdiv_e5b,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(eextdiv_eng,        4)
REGDEF_BIT(eextdiv_con,        4)
REGDEF_END(EDGE_EXTRACTION_REGISTER_5)


/*
    w_con_eng:    [0x0, 0xf],           bits : 3_0
    w_low    :    [0x0, 0x1f],          bits : 8_4
    w_high   :    [0x0, 0x1f],          bits : 16_12
*/
#define EDGE_REGION_EXTRACTION_REGISTER_0_OFS 0x0088
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_0)
REGDEF_BIT(w_con_eng,        4)
REGDEF_BIT(w_low,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(w_high,        5)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_0)


/*
    w_ker_thin    :    [0x0, 0xf],          bits : 3_0
    w_ker_robust  :    [0x0, 0xf],          bits : 7_4
    iso_ker_thin  :    [0x0, 0xf],          bits : 11_8
    iso_ker_robust:    [0x0, 0xf],          bits : 15_12
*/
#define EDGE_REGION_EXTRACTION_REGISTER_1_OFS 0x008c
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_1)
REGDEF_BIT(w_ker_thin,        4)
REGDEF_BIT(w_ker_robust,        4)
REGDEF_BIT(iso_ker_thin,        4)
REGDEF_BIT(iso_ker_robust,        4)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_1)


/*
    w_hld_low         :    [0x0, 0x1f],         bits : 4_0
    w_hld_high        :    [0x0, 0x1f],         bits : 12_8
    w_ker_thin_hld    :    [0x0, 0xf],          bits : 19_16
    w_ker_robust_hld  :    [0x0, 0xf],          bits : 23_20
    iso_ker_thin_hld  :    [0x0, 0xf],          bits : 27_24
    iso_ker_robust_hld:    [0x0, 0xf],          bits : 31_28
*/
#define EDGE_REGION_EXTRACTION_REGISTER_2_OFS 0x0090
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_2)
REGDEF_BIT(w_hld_low,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(w_hld_high,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(w_ker_thin_hld,        4)
REGDEF_BIT(w_ker_robust_hld,        4)
REGDEF_BIT(iso_ker_thin_hld,        4)
REGDEF_BIT(iso_ker_robust_hld,        4)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_2)


/*
    th_flat:    [0x0, 0x3ff],           bits : 9_0
    th_edge:    [0x0, 0x3ff],           bits : 21_12
*/
#define EDGE_REGION_EXTRACTION_REGISTER_3_OFS 0x0094
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_3)
REGDEF_BIT(th_flat,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(th_edge,        10)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_3)


/*
    th_flat_hld:    [0x0, 0x3ff],           bits : 9_0
    th_edge_hld:    [0x0, 0x3ff],           bits : 21_12
    th_lum_hld :    [0x0, 0x3ff],           bits : 31_22
*/
#define EDGE_REGION_EXTRACTION_REGISTER_4_OFS 0x0098
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_4)
REGDEF_BIT(th_flat_hld,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(th_edge_hld,        10)
REGDEF_BIT(th_lum_hld,        10)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_4)


/*
    slope_con_eng    :    [0x0, 0xffff],            bits : 15_0
    slope_hld_con_eng:    [0x0, 0xffff],            bits : 31_16
*/
#define EDGE_REGION_EXTRACTION_REGISTER_5_OFS 0x009c
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_5)
REGDEF_BIT(slope_con_eng,        16)
REGDEF_BIT(slope_hld_con_eng,        16)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_5)


/*
    overshoot_en :    [0x0, 0x1],           bits : 0
    wt_overshoot :    [0x0, 0xff],          bits : 11_4
    wt_undershoot:    [0x0, 0xff],          bits : 23_16
*/
#define OVERSHOOTING_CONTROL_REGISTER_0_OFS 0x00a0
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_0)
REGDEF_BIT(overshoot_en,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(wt_overshoot,        8)
REGDEF_BIT(,        4)
REGDEF_BIT(wt_undershoot,        8)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_0)


/*
    th_overshoot     :    [0x0, 0xff],          bits : 7_0
    th_undershoot    :    [0x0, 0xff],          bits : 15_8
    th_undershoot_lum:    [0x0, 0xff],          bits : 23_16
    th_undershoot_eng:    [0x0, 0xff],          bits : 31_24
*/
#define OVERSHOOTING_CONTROL_REGISTER_1_OFS 0x00a4
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_1)
REGDEF_BIT(th_overshoot,        8)
REGDEF_BIT(th_undershoot,        8)
REGDEF_BIT(th_undershoot_lum,        8)
REGDEF_BIT(th_undershoot_eng,        8)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_1)


/*
    clamp_wt_mod_lum:    [0x0, 0xff],           bits : 7_0
    clamp_wt_mod_eng:    [0x0, 0xff],           bits : 15_8
    strength_lum_eng:    [0x0, 0xff],           bits : 23_16
    norm_lum_eng    :    [0x0, 0xff],           bits : 31_24
*/
#define OVERSHOOTING_CONTROL_REGISTER_2_OFS 0x00a8
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_2)
REGDEF_BIT(clamp_wt_mod_lum,        8)
REGDEF_BIT(clamp_wt_mod_eng,        8)
REGDEF_BIT(strength_lum_eng,        8)
REGDEF_BIT(norm_lum_eng,        8)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_2)


/*
    slope_overshoot :    [0x0, 0x7fff],         bits : 14_0
    slope_undershoot:    [0x0, 0x7fff],         bits : 30_16
*/
#define OVERSHOOTING_CONTROL_REGISTER_3_OFS 0x00ac
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_3)
REGDEF_BIT(slope_overshoot,        15)
REGDEF_BIT(,        1)
REGDEF_BIT(slope_undershoot,        15)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_3)


/*
    slope_undershoot_lum:    [0x0, 0x7fff],         bits : 14_0
    slope_undershoot_eng:    [0x0, 0x7fff],         bits : 30_16
*/
#define OVERSHOOTING_CONTROL_REGISTER_4_OFS 0x00b0
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_4)
REGDEF_BIT(slope_undershoot_lum,        15)
REGDEF_BIT(,        1)
REGDEF_BIT(slope_undershoot_eng,        15)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_4)


/*
    esthrl:    [0x0, 0x3ff],            bits : 9_0
    esthrh:    [0x0, 0x3ff],            bits : 19_10
    establ:    [0x0, 0x7],          bits : 22_20
    estabh:    [0x0, 0x7],          bits : 26_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_0_OFS 0x00b4
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_0)
REGDEF_BIT(esthrl,        10)
REGDEF_BIT(esthrh,        10)
REGDEF_BIT(establ,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(estabh,        3)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_0)


/*
    eslutl0:    [0x0, 0xff],            bits : 7_0
    eslutl1:    [0x0, 0xff],            bits : 15_8
    eslutl2:    [0x0, 0xff],            bits : 23_16
    eslutl3:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_1_OFS 0x00b8
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_1)
REGDEF_BIT(eslutl0,        8)
REGDEF_BIT(eslutl1,        8)
REGDEF_BIT(eslutl2,        8)
REGDEF_BIT(eslutl3,        8)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_1)


/*
    eslutl4:    [0x0, 0xff],            bits : 7_0
    eslutl5:    [0x0, 0xff],            bits : 15_8
    eslutl6:    [0x0, 0xff],            bits : 23_16
    eslutl7:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_2_OFS 0x00bc
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_2)
REGDEF_BIT(eslutl4,        8)
REGDEF_BIT(eslutl5,        8)
REGDEF_BIT(eslutl6,        8)
REGDEF_BIT(eslutl7,        8)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_2)


/*
    esluth0:    [0x0, 0xff],            bits : 7_0
    esluth1:    [0x0, 0xff],            bits : 15_8
    esluth2:    [0x0, 0xff],            bits : 23_16
    esluth3:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_3_OFS 0x00c0
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_3)
REGDEF_BIT(esluth0,        8)
REGDEF_BIT(esluth1,        8)
REGDEF_BIT(esluth2,        8)
REGDEF_BIT(esluth3,        8)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_3)


/*
    esluth4:    [0x0, 0xff],            bits : 7_0
    esluth5:    [0x0, 0xff],            bits : 15_8
    esluth6:    [0x0, 0xff],            bits : 23_16
    esluth7:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_4_OFS 0x00c4
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_4)
REGDEF_BIT(esluth4,        8)
REGDEF_BIT(esluth5,        8)
REGDEF_BIT(esluth6,        8)
REGDEF_BIT(esluth7,        8)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_4)


/*
    edthrl :    [0x0, 0x3ff],           bits : 9_0
    edthrh :    [0x0, 0x3ff],           bits : 19_10
    edtabl :    [0x0, 0x7],         bits : 22_20
    edtabh :    [0x0, 0x7],         bits : 26_24
    edinsel:    [0x0, 0x3],         bits : 29_28
*/
#define EDGE_DMAP_PROCESS_REGISTER_0_OFS 0x00c8
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_0)
REGDEF_BIT(edthrl,        10)
REGDEF_BIT(edthrh,        10)
REGDEF_BIT(edtabl,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(edtabh,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(edinsel,        2)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_0)


/*
    edlutl0:    [0x0, 0xff],            bits : 7_0
    edlutl1:    [0x0, 0xff],            bits : 15_8
    edlutl2:    [0x0, 0xff],            bits : 23_16
    edlutl3:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_DMAP_PROCESS_REGISTER_1_OFS 0x00cc
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_1)
REGDEF_BIT(edlutl0,        8)
REGDEF_BIT(edlutl1,        8)
REGDEF_BIT(edlutl2,        8)
REGDEF_BIT(edlutl3,        8)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_1)


/*
    edlutl4:    [0x0, 0xff],            bits : 7_0
    edlutl5:    [0x0, 0xff],            bits : 15_8
    edlutl6:    [0x0, 0xff],            bits : 23_16
    edlutl7:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_DMAP_PROCESS_REGISTER_2_OFS 0x00d0
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_2)
REGDEF_BIT(edlutl4,        8)
REGDEF_BIT(edlutl5,        8)
REGDEF_BIT(edlutl6,        8)
REGDEF_BIT(edlutl7,        8)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_2)


/*
    edluth0:    [0x0, 0xff],            bits : 7_0
    edluth1:    [0x0, 0xff],            bits : 15_8
    edluth2:    [0x0, 0xff],            bits : 23_16
    edluth3:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_DMAP_PROCESS_REGISTER_3_OFS 0x00d4
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_3)
REGDEF_BIT(edluth0,        8)
REGDEF_BIT(edluth1,        8)
REGDEF_BIT(edluth2,        8)
REGDEF_BIT(edluth3,        8)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_3)


/*
    edluth4:    [0x0, 0xff],            bits : 7_0
    edluth5:    [0x0, 0xff],            bits : 15_8
    edluth6:    [0x0, 0xff],            bits : 23_16
    edluth7:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_DMAP_PROCESS_REGISTER_4_OFS 0x00d8
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_4)
REGDEF_BIT(edluth4,        8)
REGDEF_BIT(edluth5,        8)
REGDEF_BIT(edluth6,        8)
REGDEF_BIT(edluth7,        8)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_4)


/*
    localmax_statistics_max:    [0x0, 0x3ff],           bits : 9_0
    coneng_statistics_max  :    [0x0, 0x3ff],           bits : 19_10
    coneng_statistics_avg  :    [0x0, 0x3ff],           bits : 29_20
    reserved               :    [0x0, 0x3],         bits : 31_30
*/
#define EDGE_STATISTIC_REGISTER_OFS 0x00dc
REGDEF_BEGIN(EDGE_STATISTIC_REGISTER)
REGDEF_BIT(localmax_statistics_max,        10)
REGDEF_BIT(coneng_statistics_max,        10)
REGDEF_BIT(coneng_statistics_avg,        10)
REGDEF_BIT(reserved,        2)
REGDEF_END(EDGE_STATISTIC_REGISTER)


/*
    region_str_en:    [0x0, 0x1],           bits : 0
    enh_thin     :    [0x0, 0xff],          bits : 23_16
    enh_robust   :    [0x0, 0xff],          bits : 31_24
*/
#define EDGE_REGION_STRENGTH_REGISTER_1_OFS 0x00e0
REGDEF_BEGIN(EDGE_REGION_STRENGTH_REGISTER_1)
REGDEF_BIT(region_str_en,        1)
REGDEF_BIT(,        15)
REGDEF_BIT(enh_thin,        8)
REGDEF_BIT(enh_robust,        8)
REGDEF_END(EDGE_REGION_STRENGTH_REGISTER_1)


/*
    slope_flat:    [0x0, 0xffff],           bits : 15_0
    slope_edge:    [0x0, 0xffff],           bits : 31_16
*/
#define EDGE_REGION_STRENGTH_REGISTER_2_OFS 0x00e4
REGDEF_BEGIN(EDGE_REGION_STRENGTH_REGISTER_2)
REGDEF_BIT(slope_flat,        16)
REGDEF_BIT(slope_edge,        16)
REGDEF_END(EDGE_REGION_STRENGTH_REGISTER_2)


/*
    str_flat:    [0x0, 0xff],           bits : 7_0
    str_edge:    [0x0, 0xff],           bits : 15_8
*/
#define EDGE_REGION_STRENGTH_REGISTER_3_OFS 0x00e8
REGDEF_BEGIN(EDGE_REGION_STRENGTH_REGISTER_3)
REGDEF_BIT(str_flat,        8)
REGDEF_BIT(str_edge,        8)
REGDEF_END(EDGE_REGION_STRENGTH_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER2_OFS 0x00ec
REGDEF_BEGIN(IPE_RESERVED_REGISTER2)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RGB_LPF_REGISTER_0_OFS 0x00f0
REGDEF_BEGIN(RGB_LPF_REGISTER_0)
REGDEF_BIT(reserved,        32)
REGDEF_END(RGB_LPF_REGISTER_0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RGB_LPF_REGISTER_1_OFS 0x00f4
REGDEF_BEGIN(RGB_LPF_REGISTER_1)
REGDEF_BIT(reserved,        32)
REGDEF_END(RGB_LPF_REGISTER_1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RGB_LPF_REGISTER_2_OFS 0x00f8
REGDEF_BEGIN(RGB_LPF_REGISTER_2)
REGDEF_BIT(reserved,        32)
REGDEF_END(RGB_LPF_REGISTER_2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER3_OFS 0x00fc
REGDEF_BEGIN(IPE_RESERVED_REGISTER3)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER3)


/*
    ccrange   :    [0x0, 0x3],          bits : 1_0
    cc2_sel   :    [0x0, 0x1],          bits : 2
    ccr_gamsel:    [0x0, 0x1],          bits : 3
    ccstab_sel:    [0x0, 0x3],          bits : 5_4
    ccofs_sel :    [0x0, 0x3],          bits : 7_6
    coef_rr   :    [0x0, 0xfff],            bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_0_OFS 0x0100
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_0)
REGDEF_BIT(ccrange,        2)
REGDEF_BIT(cc2_sel,        1)
REGDEF_BIT(ccr_gamsel,        1)
REGDEF_BIT(ccstab_sel,        2)
REGDEF_BIT(ccofs_sel,        2)
REGDEF_BIT(,        8)
REGDEF_BIT(coef_rr,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_0)


/*
    coef_rg:    [0x0, 0xfff],           bits : 11_0
    coef_rb:    [0x0, 0xfff],           bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_1_OFS 0x0104
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_1)
REGDEF_BIT(coef_rg,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(coef_rb,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_1)


/*
    coef_gr:    [0x0, 0xfff],           bits : 11_0
    coef_gg:    [0x0, 0xfff],           bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_2_OFS 0x0108
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_2)
REGDEF_BIT(coef_gr,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(coef_gg,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_2)


/*
    coef_gb:    [0x0, 0xfff],           bits : 11_0
    coef_br:    [0x0, 0xfff],           bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_3_OFS 0x010c
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_3)
REGDEF_BIT(coef_gb,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(coef_br,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_3)


/*
    coef_bg:    [0x0, 0xfff],           bits : 11_0
    coef_bb:    [0x0, 0xfff],           bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_4_OFS 0x0110
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_4)
REGDEF_BIT(coef_bg,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(coef_bb,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_4)


/*
    fstab0:    [0x0, 0xff],         bits : 7_0
    fstab1:    [0x0, 0xff],         bits : 15_8
    fstab2:    [0x0, 0xff],         bits : 23_16
    fstab3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_0_OFS 0x0114
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_0)
REGDEF_BIT(fstab0,        8)
REGDEF_BIT(fstab1,        8)
REGDEF_BIT(fstab2,        8)
REGDEF_BIT(fstab3,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_0)


/*
    fstab4:    [0x0, 0xff],         bits : 7_0
    fstab5:    [0x0, 0xff],         bits : 15_8
    fstab6:    [0x0, 0xff],         bits : 23_16
    fstab7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_1_OFS 0x0118
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_1)
REGDEF_BIT(fstab4,        8)
REGDEF_BIT(fstab5,        8)
REGDEF_BIT(fstab6,        8)
REGDEF_BIT(fstab7,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_1)


/*
    fstab8 :    [0x0, 0xff],            bits : 7_0
    fstab9 :    [0x0, 0xff],            bits : 15_8
    fstab10:    [0x0, 0xff],            bits : 23_16
    fstab11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_2_OFS 0x011c
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_2)
REGDEF_BIT(fstab8,        8)
REGDEF_BIT(fstab9,        8)
REGDEF_BIT(fstab10,        8)
REGDEF_BIT(fstab11,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_2)


/*
    fstab12:    [0x0, 0xff],            bits : 7_0
    fstab13:    [0x0, 0xff],            bits : 15_8
    fstab14:    [0x0, 0xff],            bits : 23_16
    fstab15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_3_OFS 0x0120
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_3)
REGDEF_BIT(fstab12,        8)
REGDEF_BIT(fstab13,        8)
REGDEF_BIT(fstab14,        8)
REGDEF_BIT(fstab15,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_3)


/*
    fdtab0:    [0x0, 0xff],         bits : 7_0
    fdtab1:    [0x0, 0xff],         bits : 15_8
    fdtab2:    [0x0, 0xff],         bits : 23_16
    fdtab3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0_OFS 0x0124
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0)
REGDEF_BIT(fdtab0,        8)
REGDEF_BIT(fdtab1,        8)
REGDEF_BIT(fdtab2,        8)
REGDEF_BIT(fdtab3,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0)


/*
    fdtab4:    [0x0, 0xff],         bits : 7_0
    fdtab5:    [0x0, 0xff],         bits : 15_8
    fdtab6:    [0x0, 0xff],         bits : 23_16
    fdtab7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1_OFS 0x0128
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1)
REGDEF_BIT(fdtab4,        8)
REGDEF_BIT(fdtab5,        8)
REGDEF_BIT(fdtab6,        8)
REGDEF_BIT(fdtab7,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1)


/*
    fdtab8 :    [0x0, 0xff],            bits : 7_0
    fdtab9 :    [0x0, 0xff],            bits : 15_8
    fdtab10:    [0x0, 0xff],            bits : 23_16
    fdtab11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2_OFS 0x012c
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2)
REGDEF_BIT(fdtab8,        8)
REGDEF_BIT(fdtab9,        8)
REGDEF_BIT(fdtab10,        8)
REGDEF_BIT(fdtab11,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2)


/*
    fdtab12:    [0x0, 0xff],            bits : 7_0
    fdtab13:    [0x0, 0xff],            bits : 15_8
    fdtab14:    [0x0, 0xff],            bits : 23_16
    fdtab15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3_OFS 0x0130
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3)
REGDEF_BIT(fdtab12,        8)
REGDEF_BIT(fdtab13,        8)
REGDEF_BIT(fdtab14,        8)
REGDEF_BIT(fdtab15,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER4_OFS 0x0134
REGDEF_BEGIN(IPE_RESERVED_REGISTER4)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER4)


/*
    coef_yr :    [0x0, 0x1ff],          bits : 8_0
    coef_yg :    [0x0, 0x1ff],          bits : 17_9
    coef_yb :    [0x0, 0x1ff],          bits : 26_18
    cstp_rat:    [0x0, 0x1f],           bits : 31_27
*/
#define COLOR_SPACE_TRANSFORM_REGISTER0_OFS 0x0138
REGDEF_BEGIN(COLOR_SPACE_TRANSFORM_REGISTER0)
REGDEF_BIT(coef_yr,        9)
REGDEF_BIT(coef_yg,        9)
REGDEF_BIT(coef_yb,        9)
REGDEF_BIT(cstp_rat,        5)
REGDEF_END(COLOR_SPACE_TRANSFORM_REGISTER0)


/*
    coef_ur   :    [0x0, 0x1ff],            bits : 8_0
    coef_ug   :    [0x0, 0x1ff],            bits : 17_9
    coef_ub   :    [0x0, 0x1ff],            bits : 26_18
    cstoff_sel:    [0x0, 0x1],          bits : 28
*/
#define COLOR_SPACE_TRANSFORM_REGISTER1_OFS 0x013c
REGDEF_BEGIN(COLOR_SPACE_TRANSFORM_REGISTER1)
REGDEF_BIT(coef_ur,        9)
REGDEF_BIT(coef_ug,        9)
REGDEF_BIT(coef_ub,        9)
REGDEF_BIT(,        1)
REGDEF_BIT(cstoff_sel,        1)
REGDEF_END(COLOR_SPACE_TRANSFORM_REGISTER1)


/*
    coef_vr:    [0x0, 0x1ff],           bits : 8_0
    coef_vg:    [0x0, 0x1ff],           bits : 17_9
    coef_vb:    [0x0, 0x1ff],           bits : 26_18
*/
#define COLOR_SPACE_TRANSFORM_REGISTER2_OFS 0x0140
REGDEF_BEGIN(COLOR_SPACE_TRANSFORM_REGISTER2)
REGDEF_BIT(coef_vr,        9)
REGDEF_BIT(coef_vg,        9)
REGDEF_BIT(coef_vb,        9)
REGDEF_END(COLOR_SPACE_TRANSFORM_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER5_OFS 0x0144
REGDEF_BEGIN(IPE_RESERVED_REGISTER5)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER5)


/*
    int_ofs   :    [0x0, 0xff],         bits : 7_0
    sat_ofs   :    [0x0, 0xff],         bits : 15_8
    chue_roten:    [0x0, 0x1],          bits : 16
    chue_c2gen:    [0x0, 0x1],          bits : 17
    cctrl_sel :    [0x0, 0x3],          bits : 21_20
    vdet_div  :    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_REGISTER_OFS 0x0148
REGDEF_BEGIN(COLOR_CONTROL_REGISTER)
REGDEF_BIT(int_ofs,        8)
REGDEF_BIT(sat_ofs,        8)
REGDEF_BIT(chue_roten,        1)
REGDEF_BIT(chue_c2gen,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(cctrl_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(vdet_div,        8)
REGDEF_END(COLOR_CONTROL_REGISTER)


/*
    chuem0:    [0x0, 0xff],         bits : 7_0
    chuem1:    [0x0, 0xff],         bits : 15_8
    chuem2:    [0x0, 0xff],         bits : 23_16
    chuem3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_0_OFS 0x014c
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_0)
REGDEF_BIT(chuem0,        8)
REGDEF_BIT(chuem1,        8)
REGDEF_BIT(chuem2,        8)
REGDEF_BIT(chuem3,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_0)


/*
    chuem4:    [0x0, 0xff],         bits : 7_0
    chuem5:    [0x0, 0xff],         bits : 15_8
    chuem6:    [0x0, 0xff],         bits : 23_16
    chuem7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_1_OFS 0x0150
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_1)
REGDEF_BIT(chuem4,        8)
REGDEF_BIT(chuem5,        8)
REGDEF_BIT(chuem6,        8)
REGDEF_BIT(chuem7,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_1)


/*
    chuem8 :    [0x0, 0xff],            bits : 7_0
    chuem9 :    [0x0, 0xff],            bits : 15_8
    chuem10:    [0x0, 0xff],            bits : 23_16
    chuem11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_2_OFS 0x0154
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_2)
REGDEF_BIT(chuem8,        8)
REGDEF_BIT(chuem9,        8)
REGDEF_BIT(chuem10,        8)
REGDEF_BIT(chuem11,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_2)


/*
    chuem12:    [0x0, 0xff],            bits : 7_0
    chuem13:    [0x0, 0xff],            bits : 15_8
    chuem14:    [0x0, 0xff],            bits : 23_16
    chuem15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_3_OFS 0x0158
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_3)
REGDEF_BIT(chuem12,        8)
REGDEF_BIT(chuem13,        8)
REGDEF_BIT(chuem14,        8)
REGDEF_BIT(chuem15,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_3)


/*
    chuem16:    [0x0, 0xff],            bits : 7_0
    chuem17:    [0x0, 0xff],            bits : 15_8
    chuem18:    [0x0, 0xff],            bits : 23_16
    chuem19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_4_OFS 0x015c
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_4)
REGDEF_BIT(chuem16,        8)
REGDEF_BIT(chuem17,        8)
REGDEF_BIT(chuem18,        8)
REGDEF_BIT(chuem19,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_4)


/*
    chuem20:    [0x0, 0xff],            bits : 7_0
    chuem21:    [0x0, 0xff],            bits : 15_8
    chuem22:    [0x0, 0xff],            bits : 23_16
    chuem23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_5_OFS 0x0160
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_5)
REGDEF_BIT(chuem20,        8)
REGDEF_BIT(chuem21,        8)
REGDEF_BIT(chuem22,        8)
REGDEF_BIT(chuem23,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_5)


/*
    cintm0:    [0x0, 0xff],         bits : 7_0
    cintm1:    [0x0, 0xff],         bits : 15_8
    cintm2:    [0x0, 0xff],         bits : 23_16
    cintm3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0_OFS 0x0164
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0)
REGDEF_BIT(cintm0,        8)
REGDEF_BIT(cintm1,        8)
REGDEF_BIT(cintm2,        8)
REGDEF_BIT(cintm3,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0)


/*
    cintm4:    [0x0, 0xff],         bits : 7_0
    cintm5:    [0x0, 0xff],         bits : 15_8
    cintm6:    [0x0, 0xff],         bits : 23_16
    cintm7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1_OFS 0x0168
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1)
REGDEF_BIT(cintm4,        8)
REGDEF_BIT(cintm5,        8)
REGDEF_BIT(cintm6,        8)
REGDEF_BIT(cintm7,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1)


/*
    cintm8 :    [0x0, 0xff],            bits : 7_0
    cintm9 :    [0x0, 0xff],            bits : 15_8
    cintm10:    [0x0, 0xff],            bits : 23_16
    cintm11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2_OFS 0x016c
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2)
REGDEF_BIT(cintm8,        8)
REGDEF_BIT(cintm9,        8)
REGDEF_BIT(cintm10,        8)
REGDEF_BIT(cintm11,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2)


/*
    cintm12:    [0x0, 0xff],            bits : 7_0
    cintm13:    [0x0, 0xff],            bits : 15_8
    cintm14:    [0x0, 0xff],            bits : 23_16
    cintm15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3_OFS 0x0170
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3)
REGDEF_BIT(cintm12,        8)
REGDEF_BIT(cintm13,        8)
REGDEF_BIT(cintm14,        8)
REGDEF_BIT(cintm15,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3)


/*
    cintm16:    [0x0, 0xff],            bits : 7_0
    cintm17:    [0x0, 0xff],            bits : 15_8
    cintm18:    [0x0, 0xff],            bits : 23_16
    cintm19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4_OFS 0x0174
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4)
REGDEF_BIT(cintm16,        8)
REGDEF_BIT(cintm17,        8)
REGDEF_BIT(cintm18,        8)
REGDEF_BIT(cintm19,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4)


/*
    cintm20:    [0x0, 0xff],            bits : 7_0
    cintm21:    [0x0, 0xff],            bits : 15_8
    cintm22:    [0x0, 0xff],            bits : 23_16
    cintm23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5_OFS 0x0178
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5)
REGDEF_BIT(cintm20,        8)
REGDEF_BIT(cintm21,        8)
REGDEF_BIT(cintm22,        8)
REGDEF_BIT(cintm23,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5)


/*
    csatm0:    [0x0, 0xff],         bits : 7_0
    csatm1:    [0x0, 0xff],         bits : 15_8
    csatm2:    [0x0, 0xff],         bits : 23_16
    csatm3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0_OFS 0x017c
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0)
REGDEF_BIT(csatm0,        8)
REGDEF_BIT(csatm1,        8)
REGDEF_BIT(csatm2,        8)
REGDEF_BIT(csatm3,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0)


/*
    csatm4:    [0x0, 0xff],         bits : 7_0
    csatm5:    [0x0, 0xff],         bits : 15_8
    csatm6:    [0x0, 0xff],         bits : 23_16
    csatm7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1_OFS 0x0180
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1)
REGDEF_BIT(csatm4,        8)
REGDEF_BIT(csatm5,        8)
REGDEF_BIT(csatm6,        8)
REGDEF_BIT(csatm7,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1)


/*
    csatm8 :    [0x0, 0xff],            bits : 7_0
    csatm9 :    [0x0, 0xff],            bits : 15_8
    csatm10:    [0x0, 0xff],            bits : 23_16
    csatm11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2_OFS 0x0184
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2)
REGDEF_BIT(csatm8,        8)
REGDEF_BIT(csatm9,        8)
REGDEF_BIT(csatm10,        8)
REGDEF_BIT(csatm11,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2)


/*
    csatm12:    [0x0, 0xff],            bits : 7_0
    csatm13:    [0x0, 0xff],            bits : 15_8
    csatm14:    [0x0, 0xff],            bits : 23_16
    csatm15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3_OFS 0x0188
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3)
REGDEF_BIT(csatm12,        8)
REGDEF_BIT(csatm13,        8)
REGDEF_BIT(csatm14,        8)
REGDEF_BIT(csatm15,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3)


/*
    csatm16:    [0x0, 0xff],            bits : 7_0
    csatm17:    [0x0, 0xff],            bits : 15_8
    csatm18:    [0x0, 0xff],            bits : 23_16
    csatm19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4_OFS 0x018c
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4)
REGDEF_BIT(csatm16,        8)
REGDEF_BIT(csatm17,        8)
REGDEF_BIT(csatm18,        8)
REGDEF_BIT(csatm19,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4)


/*
    csatm20:    [0x0, 0xff],            bits : 7_0
    csatm21:    [0x0, 0xff],            bits : 15_8
    csatm22:    [0x0, 0xff],            bits : 23_16
    csatm23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5_OFS 0x0190
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5)
REGDEF_BIT(csatm20,        8)
REGDEF_BIT(csatm21,        8)
REGDEF_BIT(csatm22,        8)
REGDEF_BIT(csatm23,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5)


/*
    cedgem0:    [0x0, 0xff],         bits : 7_0
    cedgem1:    [0x0, 0xff],         bits : 15_8
    cedgem2:    [0x0, 0xff],         bits : 23_16
    cedgem3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_0_OFS 0x0194
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_0)
REGDEF_BIT(cedgem0,        8)
REGDEF_BIT(cedgem1,        8)
REGDEF_BIT(cedgem2,        8)
REGDEF_BIT(cedgem3,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_0)


/*
    cedgem4:    [0x0, 0xff],         bits : 7_0
    cedgem5:    [0x0, 0xff],         bits : 15_8
    cedgem6:    [0x0, 0xff],         bits : 23_16
    cedgem7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_1_OFS 0x0198
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_1)
REGDEF_BIT(cedgem4,        8)
REGDEF_BIT(cedgem5,        8)
REGDEF_BIT(cedgem6,        8)
REGDEF_BIT(cedgem7,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_1)


/*
    cedgem8 :    [0x0, 0xff],            bits : 7_0
    cedgem9 :    [0x0, 0xff],            bits : 15_8
    cedgem10:    [0x0, 0xff],            bits : 23_16
    cedgem11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_2_OFS 0x019c
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_2)
REGDEF_BIT(cedgem8,        8)
REGDEF_BIT(cedgem9,        8)
REGDEF_BIT(cedgem10,        8)
REGDEF_BIT(cedgem11,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_2)


/*
    cedgem12:    [0x0, 0xff],            bits : 7_0
    cedgem13:    [0x0, 0xff],            bits : 15_8
    cedgem14:    [0x0, 0xff],            bits : 23_16
    cedgem15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_3_OFS 0x01a0
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_3)
REGDEF_BIT(cedgem12,        8)
REGDEF_BIT(cedgem13,        8)
REGDEF_BIT(cedgem14,        8)
REGDEF_BIT(cedgem15,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_3)


/*
    cedgem16:    [0x0, 0xff],            bits : 7_0
    cedgem17:    [0x0, 0xff],            bits : 15_8
    cedgem18:    [0x0, 0xff],            bits : 23_16
    cedgem19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_4_OFS 0x01a4
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_4)
REGDEF_BIT(cedgem16,        8)
REGDEF_BIT(cedgem17,        8)
REGDEF_BIT(cedgem18,        8)
REGDEF_BIT(cedgem19,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_4)


/*
    cedgem20:    [0x0, 0xff],            bits : 7_0
    cedgem21:    [0x0, 0xff],            bits : 15_8
    cedgem22:    [0x0, 0xff],            bits : 23_16
    cedgem23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_5_OFS 0x01a8
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_5)
REGDEF_BIT(cedgem20,        8)
REGDEF_BIT(cedgem21,        8)
REGDEF_BIT(cedgem22,        8)
REGDEF_BIT(cedgem23,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_5)


/*
    dds0:    [0x0, 0x3f],           bits : 5_0
    dds1:    [0x0, 0x3f],           bits : 11_6
    dds2:    [0x0, 0x3f],           bits : 17_12
    dds3:    [0x0, 0x3f],           bits : 23_18
*/
#define COLOR_CONTROL_DDS_REGISTER_0_OFS 0x01ac
REGDEF_BEGIN(COLOR_CONTROL_DDS_REGISTER_0)
REGDEF_BIT(dds0,        6)
REGDEF_BIT(dds1,        6)
REGDEF_BIT(dds2,        6)
REGDEF_BIT(dds3,        6)
REGDEF_END(COLOR_CONTROL_DDS_REGISTER_0)


/*
    dds4:    [0x0, 0x3f],           bits : 5_0
    dds5:    [0x0, 0x3f],           bits : 11_6
    dds6:    [0x0, 0x3f],           bits : 17_12
    dds7:    [0x0, 0x3f],           bits : 23_18
*/
#define COLOR_CONTROL_DDS_REGISTER_1_OFS 0x01b0
REGDEF_BEGIN(COLOR_CONTROL_DDS_REGISTER_1)
REGDEF_BIT(dds4,        6)
REGDEF_BIT(dds5,        6)
REGDEF_BIT(dds6,        6)
REGDEF_BIT(dds7,        6)
REGDEF_END(COLOR_CONTROL_DDS_REGISTER_1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER6_OFS 0x01b4
REGDEF_BEGIN(IPE_RESERVED_REGISTER6)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER6)


/*
    y_enh_p :    [0x0, 0x3ff],          bits : 9_0
    y_enh_n :    [0x0, 0x3ff],          bits : 19_10
    y_einv_p:    [0x0, 0x1],            bits : 30
    y_einv_n:    [0x0, 0x1],            bits : 31
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS 0x01b8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0)
REGDEF_BIT(y_enh_p,        10)
REGDEF_BIT(y_enh_n,        10)
REGDEF_BIT(,        10)
REGDEF_BIT(y_einv_p,        1)
REGDEF_BIT(y_einv_n,        1)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0)


/*
    y_con       :    [0x0, 0xff],           bits : 7_0
    yc_randreset:    [0x0, 0x1],            bits : 8
    y_rand_en   :    [0x0, 0x1],            bits : 9
    y_rand      :    [0x0, 0x7],            bits : 14_12
    y_ethy      :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS 0x01bc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1)
REGDEF_BIT(y_con,        8)
REGDEF_BIT(yc_randreset,        1)
REGDEF_BIT(y_rand_en,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(y_rand,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(y_ethy,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1)


/*
    y_yth1     :    [0x0, 0xff],            bits : 7_0
    y_hit1sel  :    [0x0, 0x1],         bits : 12
    y_nhit1sel :    [0x0, 0x1],         bits : 13
    y_fix1_hit :    [0x0, 0xff],            bits : 23_16
    y_fix1_nhit:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_2_OFS 0x01c0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_2)
REGDEF_BIT(y_yth1,        8)
REGDEF_BIT(,        4)
REGDEF_BIT(y_hit1sel,        1)
REGDEF_BIT(y_nhit1sel,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(y_fix1_hit,        8)
REGDEF_BIT(y_fix1_nhit,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_2)


/*
    y_yth2     :    [0x0, 0xff],            bits : 7_0
    y_hit2sel  :    [0x0, 0x1],         bits : 12
    y_nhit2sel :    [0x0, 0x1],         bits : 13
    y_fix2_hit :    [0x0, 0xff],            bits : 23_16
    y_fix2_nhit:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_3_OFS 0x01c4
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_3)
REGDEF_BIT(y_yth2,        8)
REGDEF_BIT(,        4)
REGDEF_BIT(y_hit2sel,        1)
REGDEF_BIT(y_nhit2sel,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(y_fix2_hit,        8)
REGDEF_BIT(y_fix2_nhit,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_3)


/*
    ymask :    [0x0, 0xff],         bits : 7_0
    cbmask:    [0x0, 0xff],         bits : 15_8
    crmask:    [0x0, 0xff],         bits : 23_16
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_4_OFS 0x01c8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_4)
REGDEF_BIT(ymask,        8)
REGDEF_BIT(cbmask,        8)
REGDEF_BIT(crmask,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_4)


/*
    c_cbofs  :    [0x0, 0xff],          bits : 7_0
    c_crofs  :    [0x0, 0xff],          bits : 15_8
    c_con    :    [0x0, 0xff],          bits : 23_16
    c_rand_en:    [0x0, 0x1],           bits : 27
    c_rand   :    [0x0, 0x7],           bits : 30_28
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS 0x01cc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5)
REGDEF_BIT(c_cbofs,        8)
REGDEF_BIT(c_crofs,        8)
REGDEF_BIT(c_con,        8)
REGDEF_BIT(,        3)
REGDEF_BIT(c_rand_en,        1)
REGDEF_BIT(c_rand,        3)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5)


/*
    cb_fix_hit :    [0x0, 0xff],            bits : 7_0
    cr_fix_hit :    [0x0, 0xff],            bits : 15_8
    cb_fix_nhit:    [0x0, 0xff],            bits : 23_16
    cr_fix_nhit:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_6_OFS 0x01d0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_6)
REGDEF_BIT(cb_fix_hit,        8)
REGDEF_BIT(cr_fix_hit,        8)
REGDEF_BIT(cb_fix_nhit,        8)
REGDEF_BIT(cr_fix_nhit,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_6)


/*
    c_eth    :    [0x0, 0x3ff],         bits : 9_0
    c_hitsel :    [0x0, 0x1],           bits : 12
    c_nhitsel:    [0x0, 0x1],           bits : 13
    c_yth_h  :    [0x0, 0xff],          bits : 23_16
    c_yth_l  :    [0x0, 0xff],          bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_7_OFS 0x01d4
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_7)
REGDEF_BIT(c_eth,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(c_hitsel,        1)
REGDEF_BIT(c_nhitsel,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(c_yth_h,        8)
REGDEF_BIT(c_yth_l,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_7)


/*
    c_cbth_h:    [0x0, 0xff],           bits : 7_0
    c_cbth_l:    [0x0, 0xff],           bits : 15_8
    c_crth_h:    [0x0, 0xff],           bits : 23_16
    c_crth_l:    [0x0, 0xff],           bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_8_OFS 0x01d8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_8)
REGDEF_BIT(c_cbth_h,        8)
REGDEF_BIT(c_cbth_l,        8)
REGDEF_BIT(c_crth_h,        8)
REGDEF_BIT(c_crth_l,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_8)


/*
    ccontab0:    [0x0, 0x3ff],          bits : 9_0
    ccontab1:    [0x0, 0x3ff],          bits : 19_10
    ccontab2:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS 0x01dc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9)
REGDEF_BIT(ccontab0,        10)
REGDEF_BIT(ccontab1,        10)
REGDEF_BIT(ccontab2,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9)


/*
    ccontab3:    [0x0, 0x3ff],          bits : 9_0
    ccontab4:    [0x0, 0x3ff],          bits : 19_10
    ccontab5:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_10_OFS 0x01e0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10)
REGDEF_BIT(ccontab3,        10)
REGDEF_BIT(ccontab4,        10)
REGDEF_BIT(ccontab5,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10)


/*
    ccontab6:    [0x0, 0x3ff],          bits : 9_0
    ccontab7:    [0x0, 0x3ff],          bits : 19_10
    ccontab8:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_11_OFS 0x01e4
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11)
REGDEF_BIT(ccontab6,        10)
REGDEF_BIT(ccontab7,        10)
REGDEF_BIT(ccontab8,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11)


/*
    ccontab9 :    [0x0, 0x3ff],         bits : 9_0
    ccontab10:    [0x0, 0x3ff],         bits : 19_10
    ccontab11:    [0x0, 0x3ff],         bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_12_OFS 0x01e8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12)
REGDEF_BIT(ccontab9,        10)
REGDEF_BIT(ccontab10,        10)
REGDEF_BIT(ccontab11,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12)


/*
    ccontab12:    [0x0, 0x3ff],         bits : 9_0
    ccontab13:    [0x0, 0x3ff],         bits : 19_10
    ccontab14:    [0x0, 0x3ff],         bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_13_OFS 0x01ec
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13)
REGDEF_BIT(ccontab12,        10)
REGDEF_BIT(ccontab13,        10)
REGDEF_BIT(ccontab14,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13)


/*
    ccontab15  :    [0x0, 0x3ff],           bits : 9_0
    ccontab16  :    [0x0, 0x3ff],           bits : 19_10
    ccontab_sel:    [0x0, 0x1],         bits : 31
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS 0x01f0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14)
REGDEF_BIT(ccontab15,        10)
REGDEF_BIT(ccontab16,        10)
REGDEF_BIT(,        11)
REGDEF_BIT(ccontab_sel,        1)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER7_OFS 0x01f4
REGDEF_BEGIN(IPE_RESERVED_REGISTER7)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER7)


/*
    edge_dbg_modesel:    [0x0, 0x3],            bits : 1_0
*/
#define EDGE_DEBUG_REGISTER_OFS 0x01f8
REGDEF_BEGIN(EDGE_DEBUG_REGISTER)
REGDEF_BIT(edge_dbg_modesel,        2)
REGDEF_END(EDGE_DEBUG_REGISTER)


/*
    ycurve_sel:    [0x0, 0x3],          bits : 1_0
*/
#define YCURVE_REGISTER_OFS 0x01fc
REGDEF_BEGIN(YCURVE_REGISTER)
REGDEF_BIT(ycurve_sel,        2)
REGDEF_END(YCURVE_REGISTER)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER8_OFS 0x0200
REGDEF_BEGIN(IPE_RESERVED_REGISTER8)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER8)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER9_OFS 0x0204
REGDEF_BEGIN(IPE_RESERVED_REGISTER9)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER9)


/*
    gamyrand_en   :    [0x0, 0x1],          bits : 0
    gamyrand_reset:    [0x0, 0x1],          bits : 1
    gamyrand_shf  :    [0x0, 0x1],          bits : 4
*/
#define GAMMA_YCURVE_REGISTER_OFS 0x0208
REGDEF_BEGIN(GAMMA_YCURVE_REGISTER)
REGDEF_BIT(gamyrand_en,        1)
REGDEF_BIT(gamyrand_reset,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(gamyrand_shf,        1)
REGDEF_END(GAMMA_YCURVE_REGISTER)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER10_OFS 0x020c
REGDEF_BEGIN(IPE_RESERVED_REGISTER10)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER10)


/*
    eth_outfmt:    [0x0, 0x1],          bits : 1
    ethlow    :    [0x0, 0x3ff],            bits : 11_2
    ethmid    :    [0x0, 0x3ff],            bits : 21_12
    ethhigh   :    [0x0, 0x3ff],            bits : 31_22
*/
#define EDGE_THRESHOLD_REGISTER_0_OFS 0x0210
REGDEF_BEGIN(EDGE_THRESHOLD_REGISTER_0)
REGDEF_BIT(,        1)
REGDEF_BIT(eth_outfmt,        1)
REGDEF_BIT(ethlow,        10)
REGDEF_BIT(ethmid,        10)
REGDEF_BIT(ethhigh,        10)
REGDEF_END(EDGE_THRESHOLD_REGISTER_0)


/*
    hout_sel  :    [0x0, 0x1],          bits : 0
    vout_sel  :    [0x0, 0x1],          bits : 1
    eth_outsel:    [0x0, 0x3],          bits : 3_2
*/
#define EDGE_THRESHOLD_REGISTER_1_OFS 0x0214
REGDEF_BEGIN(EDGE_THRESHOLD_REGISTER_1)
REGDEF_BIT(hout_sel,        1)
REGDEF_BIT(vout_sel,        1)
REGDEF_BIT(eth_outsel,        2)
REGDEF_END(EDGE_THRESHOLD_REGISTER_1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER11_OFS 0x0218
REGDEF_BEGIN(IPE_RESERVED_REGISTER11)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER11)


/*
    vdet_gh1_a      :    [0x0, 0x1f],         bits : 4_0
    vdet_gh1_bcd_op :    [0x0, 0x1],          bits : 7
    vdet_gh1_b      :    [0x0, 0x1f],         bits : 12_8
    vdet_gh1_c      :    [0x0, 0xf],          bits : 19_16
    vdet_gh1_d      :    [0x0, 0xf],          bits : 23_20
    vdet_gh1_fsize  :    [0x0, 0x3],          bits : 25_24
    vdet_gh1_div    :    [0x0, 0xf],          bits : 31_28
*/
#define VARIATION_DETECTION_GROUP1_REGISTER_0_OFS 0x021c
REGDEF_BEGIN(VARIATION_DETECTION_GROUP1_REGISTER_0)
REGDEF_BIT(vdet_gh1_a,        5)
REGDEF_BIT(,                      2)
REGDEF_BIT(vdet_gh1_bcd_op,   1)
REGDEF_BIT(vdet_gh1_b,        5)
REGDEF_BIT(,                      3)
REGDEF_BIT(vdet_gh1_c,        4)
REGDEF_BIT(vdet_gh1_d,        4)
REGDEF_BIT(vdet_gh1_fsize,    2)
REGDEF_BIT(,                      2)
REGDEF_BIT(vdet_gh1_div,      4)
REGDEF_END(VARIATION_DETECTION_GROUP1_REGISTER_0)


/*
    vdet_gv1_a      :    [0x0, 0x1f],         bits : 4_0
    vdet_gv1_bcd_op :    [0x0, 0x1],          bits : 7
    vdet_gv1_b      :    [0x0, 0x1f],         bits : 12_8
    vdet_gv1_c      :    [0x0, 0xf],          bits : 19_16
    vdet_gv1_d      :    [0x0, 0xf],          bits : 23_20
    vdet_gv1_fsize  :    [0x0, 0x3],          bits : 25_24
    vdet_gv1_div    :    [0x0, 0xf],          bits : 31_28
*/
#define VARIATION_DETECTION_GROUP1_REGISTER_1_OFS 0x0220
REGDEF_BEGIN(VARIATION_DETECTION_GROUP1_REGISTER_1)
REGDEF_BIT(vdet_gv1_a,        5)
REGDEF_BIT(,                      2)
REGDEF_BIT(vdet_gv1_bcd_op,   1)
REGDEF_BIT(vdet_gv1_b,        5)
REGDEF_BIT(,                      3)
REGDEF_BIT(vdet_gv1_c,        4)
REGDEF_BIT(vdet_gv1_d,        4)
REGDEF_BIT(vdet_gv1_fsize,    2)
REGDEF_BIT(,                      2)
REGDEF_BIT(vdet_gv1_div,      4)
REGDEF_END(VARIATION_DETECTION_GROUP1_REGISTER_1)


/*
    vdet_gh2_a      :    [0x0, 0x1f],         bits : 4_0
    vdet_gh2_bcd_op :    [0x0, 0x1],          bits : 7
    vdet_gh2_b      :    [0x0, 0x1f],         bits : 12_8
    vdet_gh2_c      :    [0x0, 0xf],          bits : 19_16
    vdet_gh2_d      :    [0x0, 0xf],          bits : 23_20
    vdet_gh2_fsize  :    [0x0, 0x3],          bits : 25_24
    vdet_gh2_div    :    [0x0, 0xf],          bits : 31_28
*/
#define VARIATION_DETECTION_GROUP2_REGISTER_0_OFS 0x0224
REGDEF_BEGIN(VARIATION_DETECTION_GROUP2_REGISTER_0)
REGDEF_BIT(vdet_gh2_a,        5)
REGDEF_BIT(,                      2)
REGDEF_BIT(vdet_gh2_bcd_op,   1)
REGDEF_BIT(vdet_gh2_b,        5)
REGDEF_BIT(,                      3)
REGDEF_BIT(vdet_gh2_c,        4)
REGDEF_BIT(vdet_gh2_d,        4)
REGDEF_BIT(vdet_gh2_fsize,    2)
REGDEF_BIT(,                      2)
REGDEF_BIT(vdet_gh2_div,      4)
REGDEF_END(VARIATION_DETECTION_GROUP2_REGISTER_0)


/*
    vdet_gv2_a      :    [0x0, 0x1f],         bits : 4_0
    vdet_gv2_bcd_op :    [0x0, 0x1],          bits : 7
    vdet_gv2_b      :    [0x0, 0x1f],         bits : 12_8
    vdet_gv2_c      :    [0x0, 0xf],          bits : 19_16
    vdet_gv2_d      :    [0x0, 0xf],          bits : 23_20
    vdet_gv2_fsize  :    [0x0, 0x3],          bits : 25_24
    vdet_gv2_div    :    [0x0, 0xf],          bits : 31_28
*/
#define VARIATION_DETECTION_GROUP2_REGISTER_1_OFS 0x0228
REGDEF_BEGIN(VARIATION_DETECTION_GROUP2_REGISTER_1)
REGDEF_BIT(vdet_gv2_a,        5)
REGDEF_BIT(,                      2)
REGDEF_BIT(vdet_gv2_bcd_op,   1)
REGDEF_BIT(vdet_gv2_b,        5)
REGDEF_BIT(,                      3)
REGDEF_BIT(vdet_gv2_c,        4)
REGDEF_BIT(vdet_gv2_d,        4)
REGDEF_BIT(vdet_gv2_fsize,    2)
REGDEF_BIT(,                      2)
REGDEF_BIT(vdet_gv2_div,      4)
REGDEF_END(VARIATION_DETECTION_GROUP2_REGISTER_1)


/*
    vacc_outsel:    [0x0, 0x1],         bits : 0
    reserved   :    [0x0, 0x7],         bits : 3_1
    va_stx     :    [0x0, 0xfff],           bits : 15_4
    va_sty     :    [0x0, 0xfff],           bits : 27_16
*/
#define VARIATION_ACCUMULATION_REGISTER_0_OFS 0x022c
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_0)
REGDEF_BIT(vacc_outsel,        1)
REGDEF_BIT(reserved,        3)
REGDEF_BIT(va_stx,        12)
REGDEF_BIT(va_sty,        12)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_0)


/*
    va_g1h_thl:    [0x0, 0xff],          bits : 7_0
    va_g1h_thh:    [0x0, 0xff],          bits : 15_8
    va_g1v_thl:    [0x0, 0xff],          bits : 23_16
    va_g1v_thh:    [0x0, 0xff],          bits : 31_24
*/
#define VARIATION_ACCUMULATION_REGISTER_1_OFS 0x0230
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_1)
REGDEF_BIT(va_g1h_thl,        8)
REGDEF_BIT(va_g1h_thh,        8)
REGDEF_BIT(va_g1v_thl,        8)
REGDEF_BIT(va_g1v_thh,        8)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_1)


/*
    va_g2h_thl:    [0x0, 0xff],          bits : 7_0
    va_g2h_thh:    [0x0, 0xff],          bits : 15_8
    va_g2v_thl:    [0x0, 0xff],          bits : 23_16
    va_g2v_thh:    [0x0, 0xff],          bits : 31_24
*/
#define VARIATION_ACCUMULATION_REGISTER_2_OFS 0x0234
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_2)
REGDEF_BIT(va_g2h_thl,        8)
REGDEF_BIT(va_g2h_thh,        8)
REGDEF_BIT(va_g2v_thl,        8)
REGDEF_BIT(va_g2v_thh,        8)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_2)


/*
    va_win_szx         :    [0x0, 0x1ff],           bits : 8_0
    va_win_szy         :    [0x0, 0x1ff],           bits : 17_9
    va_g1_line_max_mode:    [0x0, 0x1],         bits : 18
    va_g2_line_max_mode:    [0x0, 0x1],         bits : 19
    va_g1_cnt_en       :    [0x0, 0x1],         bits : 20
    va_g2_cnt_en       :    [0x0, 0x1],         bits : 21
*/
#define VARIATION_ACCUMULATION_REGISTER_3_OFS 0x0238
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_3)
REGDEF_BIT(va_win_szx,        9)
REGDEF_BIT(va_win_szy,        9)
REGDEF_BIT(va_g1_line_max_mode,        1)
REGDEF_BIT(va_g2_line_max_mode,        1)
REGDEF_BIT(va_g1_cnt_en,        1)
REGDEF_BIT(va_g2_cnt_en,        1)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_3)


/*
    va_win_numx :    [0x0, 0x7],            bits : 2_0
    va_win_numy :    [0x0, 0x7],            bits : 6_4
    va_win_skipx:    [0x0, 0x3f],           bits : 21_16
    va_win_skipy:    [0x0, 0x1f],           bits : 28_24
*/
#define VARIATION_ACCUMULATION_REGISTER_4_OFS 0x023c
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_4)
REGDEF_BIT(va_win_numx,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(va_win_numy,        3)
REGDEF_BIT(,        9)
REGDEF_BIT(va_win_skipx,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(va_win_skipy,        5)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_4)


/*
    win0_stx:    [0x0, 0xfff],          bits : 11_0
    win0_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0_OFS 0x0240
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0)
REGDEF_BIT(win0_stx,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(win0_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0)


/*
    win0_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    win0_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    win0_hsz             :    [0x0, 0x1ff],         bits : 12_4
    win0_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1_OFS 0x0244
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1)
REGDEF_BIT(win0_g1_line_max_mode,        1)
REGDEF_BIT(win0_g2_line_max_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(win0_hsz,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(win0_vsz,        9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1)


/*
    win1_stx:    [0x0, 0xfff],          bits : 11_0
    win1_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_0_OFS 0x0248
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_0)
REGDEF_BIT(win1_stx,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(win1_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_0)


/*
    win1_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    win1_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    win1_hsz             :    [0x0, 0x1ff],         bits : 12_4
    win1_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_1_OFS 0x024c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_1)
REGDEF_BIT(win1_g1_line_max_mode,        1)
REGDEF_BIT(win1_g2_line_max_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(win1_hsz,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(win1_vsz,        9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_1)


/*
    win2_stx:    [0x0, 0xfff],          bits : 11_0
    win2_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_0_OFS 0x0250
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_0)
REGDEF_BIT(win2_stx,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(win2_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_0)


/*
    win2_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    win2_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    win2_hsz             :    [0x0, 0x1ff],         bits : 12_4
    win2_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_1_OFS 0x0254
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_1)
REGDEF_BIT(win2_g1_line_max_mode,        1)
REGDEF_BIT(win2_g2_line_max_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(win2_hsz,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(win2_vsz,        9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_1)


/*
    win3_stx:    [0x0, 0xfff],          bits : 11_0
    win3_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_0_OFS 0x0258
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_0)
REGDEF_BIT(win3_stx,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(win3_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_0)


/*
    win3_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    win3_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    win3_hsz             :    [0x0, 0x1ff],         bits : 12_4
    win3_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_1_OFS 0x025c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_1)
REGDEF_BIT(win3_g1_line_max_mode,        1)
REGDEF_BIT(win3_g2_line_max_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(win3_hsz,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(win3_vsz,        9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_1)


/*
    win4_stx:    [0x0, 0xfff],          bits : 11_0
    win4_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_0_OFS 0x0260
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_0)
REGDEF_BIT(win4_stx,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(win4_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_0)


/*
    win4_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    win4_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    win4_hsz             :    [0x0, 0x1ff],         bits : 12_4
    win4_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_1_OFS 0x0264
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_1)
REGDEF_BIT(win4_g1_line_max_mode,        1)
REGDEF_BIT(win4_g2_line_max_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(win4_hsz,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(win4_vsz,        9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_1)


/*
    va_win0g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win0g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_0_OFS 0x0268
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_0)
REGDEF_BIT(va_win0g1h_vacc,        16)
REGDEF_BIT(va_win0g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_0)


/*
    va_win0g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win0g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_1_OFS 0x026c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_1)
REGDEF_BIT(va_win0g1v_vacc,        16)
REGDEF_BIT(va_win0g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_1)


/*
    va_win0g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win0g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_2_OFS 0x0270
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_2)
REGDEF_BIT(va_win0g2h_vacc,        16)
REGDEF_BIT(va_win0g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_2)


/*
    va_win0g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win0g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_3_OFS 0x0274
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_3)
REGDEF_BIT(va_win0g2v_vacc,        16)
REGDEF_BIT(va_win0g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_3)


/*
    va_win1g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win1g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_4_OFS 0x0278
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_4)
REGDEF_BIT(va_win1g1h_vacc,        16)
REGDEF_BIT(va_win1g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_4)


/*
    va_win1g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win1g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_5_OFS 0x027c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_5)
REGDEF_BIT(va_win1g1v_vacc,        16)
REGDEF_BIT(va_win1g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_5)


/*
    va_win1g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win1g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_6_OFS 0x0280
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_6)
REGDEF_BIT(va_win1g2h_vacc,        16)
REGDEF_BIT(va_win1g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_6)


/*
    va_win1g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win1g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_7_OFS 0x0284
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_7)
REGDEF_BIT(va_win1g2v_vacc,        16)
REGDEF_BIT(va_win1g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_7)


/*
    va_win2g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win2g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_8_OFS 0x0288
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_8)
REGDEF_BIT(va_win2g1h_vacc,        16)
REGDEF_BIT(va_win2g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_8)


/*
    va_win2g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win2g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_9_OFS 0x028c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_9)
REGDEF_BIT(va_win2g1v_vacc,        16)
REGDEF_BIT(va_win2g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_9)


/*
    va_win2g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win2g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_10_OFS 0x0290
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_10)
REGDEF_BIT(va_win2g2h_vacc,        16)
REGDEF_BIT(va_win2g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_10)


/*
    va_win2g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win2g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_11_OFS 0x0294
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_11)
REGDEF_BIT(va_win2g2v_vacc,        16)
REGDEF_BIT(va_win2g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_11)


/*
    va_win3g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win3g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_12_OFS 0x0298
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_12)
REGDEF_BIT(va_win3g1h_vacc,        16)
REGDEF_BIT(va_win3g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_12)


/*
    va_win3g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win3g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_13_OFS 0x029c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_13)
REGDEF_BIT(va_win3g1v_vacc,        16)
REGDEF_BIT(va_win3g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_13)


/*
    va_win3g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win3g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_14_OFS 0x02a0
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_14)
REGDEF_BIT(va_win3g2h_vacc,        16)
REGDEF_BIT(va_win3g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_14)


/*
    va_win3g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win3g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_15_OFS 0x02a4
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_15)
REGDEF_BIT(va_win3g2v_vacc,        16)
REGDEF_BIT(va_win3g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_15)


/*
    va_win4g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win4g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_16_OFS 0x02a8
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_16)
REGDEF_BIT(va_win4g1h_vacc,        16)
REGDEF_BIT(va_win4g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_16)


/*
    va_win4g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win4g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_17_OFS 0x02ac
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_17)
REGDEF_BIT(va_win4g1v_vacc,        16)
REGDEF_BIT(va_win4g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_17)


/*
    va_win4g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win4g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_18_OFS 0x02b0
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_18)
REGDEF_BIT(va_win4g2h_vacc,        16)
REGDEF_BIT(va_win4g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_18)


/*
    va_win4g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    va_win4g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_19_OFS 0x02b4
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_19)
REGDEF_BIT(va_win4g2v_vacc,        16)
REGDEF_BIT(va_win4g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_19)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER12_OFS 0x02b8
REGDEF_BEGIN(IPE_RESERVED_REGISTER12)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER12)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER13_OFS 0x02bc
REGDEF_BEGIN(IPE_RESERVED_REGISTER13)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER13)


/*
    lce_subimg_width :    [0x0, 0x1f],          bits : 4_0
    lce_subimg_height:    [0x0, 0x1f],          bits : 9_5
*/
#define LCE_SUBIMG_REGISTER_OFS 0x02c0
REGDEF_BEGIN(LCE_SUBIMG_REGISTER)
REGDEF_BIT(lce_subimg_width,        5)
REGDEF_BIT(lce_subimg_height,        5)
REGDEF_END(LCE_SUBIMG_REGISTER)


/*
    lce_subimg_dramsai:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define DMA_LCE_SUBIMG_INPUT_CHANNEL_REGISTER_OFS 0x02c4
REGDEF_BEGIN(DMA_LCE_SUBIMG_INPUT_CHANNEL_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(lce_subimg_dramsai,        30)
REGDEF_END(DMA_LCE_SUBIMG_INPUT_CHANNEL_REGISTER)


/*
    lce_subimg_lofsi:    [0x0, 0x3fff],         bits : 15_2
*/
#define DMA_LCE_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS 0x02c8
REGDEF_BEGIN(DMA_LCE_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(lce_subimg_lofsi,        14)
REGDEF_END(DMA_LCE_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER)


/*
    lce_subimg_dramsao:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define DMA_LCE_SUBIMG_OUTPUT_CHANNEL_REGISTER_OFS 0x02cc
REGDEF_BEGIN(DMA_LCE_SUBIMG_OUTPUT_CHANNEL_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(lce_subimg_dramsao,        30)
REGDEF_END(DMA_LCE_SUBIMG_OUTPUT_CHANNEL_REGISTER)


/*
    lce_subimg_lofso:    [0x0, 0x3fff],         bits : 15_2
*/
#define DMA_LCE_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER_OFS 0x02d0
REGDEF_BEGIN(DMA_LCE_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(lce_subimg_lofso,        14)
REGDEF_END(DMA_LCE_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER)


/*
    lce_subimg_hfactor:    [0x0, 0xffff],           bits : 15_0
    lce_subimg_vfactor:    [0x0, 0xffff],           bits : 31_16
*/
#define LCE_SUBIMG_SCALING_REGISTER_0_OFS 0x02d4
REGDEF_BEGIN(LCE_SUBIMG_SCALING_REGISTER_0)
REGDEF_BIT(lce_subimg_hfactor,        16)
REGDEF_BIT(lce_subimg_vfactor,        16)
REGDEF_END(LCE_SUBIMG_SCALING_REGISTER_0)


/*
    lce_sub_blk_sizeh   :    [0x0, 0x3ff],          bits : 9_0
    lce_blk_cent_hfactor:    [0x0, 0x3fffff],           bits : 31_10
*/
#define LCE_SUBOUT_REGISTER_0_OFS 0x02d8
REGDEF_BEGIN(LCE_SUBOUT_REGISTER_0)
REGDEF_BIT(lce_sub_blk_sizeh,        10)
REGDEF_BIT(lce_blk_cent_hfactor,        22)
REGDEF_END(LCE_SUBOUT_REGISTER_0)


/*
    lce_sub_blk_sizev   :    [0x0, 0x3ff],          bits : 9_0
    lce_blk_cent_vfactor:    [0x0, 0x3fffff],           bits : 31_10
*/
#define LCE_SUBOUT_REGISTER_1_OFS 0x02dc
REGDEF_BEGIN(LCE_SUBOUT_REGISTER_1)
REGDEF_BIT(lce_sub_blk_sizev,        10)
REGDEF_BIT(lce_blk_cent_vfactor,        22)
REGDEF_END(LCE_SUBOUT_REGISTER_1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER14_OFS 0x02e0
REGDEF_BEGIN(IPE_RESERVED_REGISTER14)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER14)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER15_OFS 0x02e4
REGDEF_BEGIN(IPE_RESERVED_REGISTER15)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER15)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER16_OFS 0x02e8
REGDEF_BEGIN(IPE_RESERVED_REGISTER16)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER16)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER17_OFS 0x02ec
REGDEF_BEGIN(IPE_RESERVED_REGISTER17)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER17)


/*
    lce_lpf_c0:    [0x0, 0x7],          bits : 2_0
    lce_lpf_c1:    [0x0, 0x7],          bits : 5_3
    lce_lpf_c2:    [0x0, 0x7],          bits : 8_6
*/
#define LCE_SUBIMG_LPF_REGISTER_OFS 0x02f0
REGDEF_BEGIN(LCE_SUBIMG_LPF_REGISTER)
REGDEF_BIT(lce_lpf_c0,        3)
REGDEF_BIT(lce_lpf_c1,        3)
REGDEF_BIT(lce_lpf_c2,        3)
REGDEF_END(LCE_SUBIMG_LPF_REGISTER)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER18_OFS 0x02f4
REGDEF_BEGIN(IPE_RESERVED_REGISTER18)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER18)


/*
    reserved:    [0x0, 0x7fffffff],         bits : 31_1
*/
#define IPE_RESERVED_REGISTER19_OFS 0x02f8
REGDEF_BEGIN(IPE_RESERVED_REGISTER19)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER19)


/*
    reserved:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IPE_RESERVED_REGISTER20_OFS 0x02fc
REGDEF_BEGIN(IPE_RESERVED_REGISTER20)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER20)


/*
    reserved:    [0x0, 0x1fffffff],         bits : 31_3
*/
#define IPE_RESERVED_REGISTER21_OFS 0x0300
REGDEF_BEGIN(IPE_RESERVED_REGISTER21)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER21)


/*
    reserved:    [0x0, 0xfffffff],          bits : 31_4
*/
#define IPE_RESERVED_REGISTER22_OFS 0x0304
REGDEF_BEGIN(IPE_RESERVED_REGISTER22)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER22)


/*
    reserved:    [0x0, 0x7ffffff],          bits : 31_5
*/
#define IPE_RESERVED_REGISTER23_OFS 0x0308
REGDEF_BEGIN(IPE_RESERVED_REGISTER23)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER23)


/*
    reserved:    [0x0, 0x3ffffff],          bits : 31_6
*/
#define IPE_RESERVED_REGISTER24_OFS 0x030c
REGDEF_BEGIN(IPE_RESERVED_REGISTER24)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER24)


/*
    reserved:    [0x0, 0x1ffffff],          bits : 31_7
*/
#define IPE_RESERVED_REGISTER25_OFS 0x0310
REGDEF_BEGIN(IPE_RESERVED_REGISTER25)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER25)


/*
    reserved:    [0x0, 0xffffff],           bits : 31_8
*/
#define IPE_RESERVED_REGISTER26_OFS 0x0314
REGDEF_BEGIN(IPE_RESERVED_REGISTER26)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER26)


/*
    reserved:    [0x0, 0x7fffff],           bits : 31_9
*/
#define IPE_RESERVED_REGISTER27_OFS 0x0318
REGDEF_BEGIN(IPE_RESERVED_REGISTER27)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER27)


/*
    reserved:    [0x0, 0x3fffff],           bits : 31_10
*/
#define IPE_RESERVED_REGISTER28_OFS 0x031c
REGDEF_BEGIN(IPE_RESERVED_REGISTER28)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER28)


/*
    reserved:    [0x0, 0x1fffff],           bits : 31_11
*/
#define IPE_RESERVED_REGISTER29_OFS 0x0320
REGDEF_BEGIN(IPE_RESERVED_REGISTER29)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER29)


/*
    reserved:    [0x0, 0xfffff],            bits : 31_12
*/
#define IPE_RESERVED_REGISTER30_OFS 0x0324
REGDEF_BEGIN(IPE_RESERVED_REGISTER30)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER30)


/*
    reserved:    [0x0, 0x7ffff],            bits : 31_13
*/
#define IPE_RESERVED_REGISTER31_OFS 0x0328
REGDEF_BEGIN(IPE_RESERVED_REGISTER31)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER31)


/*
    reserved:    [0x0, 0x3ffff],            bits : 31_14
*/
#define IPE_RESERVED_REGISTER32_OFS 0x032c
REGDEF_BEGIN(IPE_RESERVED_REGISTER32)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER32)


/*
    reserved:    [0x0, 0x1ffff],            bits : 31_15
*/
#define IPE_RESERVED_REGISTER33_OFS 0x0330
REGDEF_BEGIN(IPE_RESERVED_REGISTER33)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER33)


/*
    reserved:    [0x0, 0xffff],         bits : 31_16
*/
#define IPE_RESERVED_REGISTER34_OFS 0x0334
REGDEF_BEGIN(IPE_RESERVED_REGISTER34)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER34)


/*
    reserved:    [0x0, 0x7fff],         bits : 31_17
*/
#define IPE_RESERVED_REGISTER35_OFS 0x0338
REGDEF_BEGIN(IPE_RESERVED_REGISTER35)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER35)


/*
    reserved:    [0x0, 0x3fff],         bits : 31_18
*/
#define IPE_RESERVED_REGISTER36_OFS 0x033c
REGDEF_BEGIN(IPE_RESERVED_REGISTER36)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER36)


/*
    reserved:    [0x0, 0x1fff],         bits : 31_19
*/
#define IPE_RESERVED_REGISTER37_OFS 0x0340
REGDEF_BEGIN(IPE_RESERVED_REGISTER37)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER37)


/*
    reserved:    [0x0, 0xfff],          bits : 31_20
*/
#define IPE_RESERVED_REGISTER38_OFS 0x0344
REGDEF_BEGIN(IPE_RESERVED_REGISTER38)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER38)


/*
    reserved:    [0x0, 0x7ff],          bits : 31_21
*/
#define IPE_RESERVED_REGISTER39_OFS 0x0348
REGDEF_BEGIN(IPE_RESERVED_REGISTER39)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER39)


/*
    reserved:    [0x0, 0x3ff],          bits : 31_22
*/
#define IPE_RESERVED_REGISTER40_OFS 0x034c
REGDEF_BEGIN(IPE_RESERVED_REGISTER40)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER40)


/*
    reserved:    [0x0, 0x1ff],          bits : 31_23
*/
#define IPE_RESERVED_REGISTER41_OFS 0x0350
REGDEF_BEGIN(IPE_RESERVED_REGISTER41)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER41)


/*
    reserved:    [0x0, 0xff],           bits : 31_24
*/
#define IPE_RESERVED_REGISTER42_OFS 0x0354
REGDEF_BEGIN(IPE_RESERVED_REGISTER42)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER42)


/*
    reserved:    [0x0, 0x7f],           bits : 31_25
*/
#define IPE_RESERVED_REGISTER43_OFS 0x0358
REGDEF_BEGIN(IPE_RESERVED_REGISTER43)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER43)


/*
    reserved:    [0x0, 0x3f],           bits : 31_26
*/
#define IPE_RESERVED_REGISTER44_OFS 0x035c
REGDEF_BEGIN(IPE_RESERVED_REGISTER44)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER44)


/*
    reserved:    [0x0, 0x1f],           bits : 31_27
*/
#define IPE_RESERVED_REGISTER45_OFS 0x0360
REGDEF_BEGIN(IPE_RESERVED_REGISTER45)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER45)


/*
    reserved:    [0x0, 0xf],            bits : 31_28
*/
#define IPE_RESERVED_REGISTER46_OFS 0x0364
REGDEF_BEGIN(IPE_RESERVED_REGISTER46)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER46)


/*
    reserved:    [0x0, 0x7],            bits : 31_29
*/
#define IPE_RESERVED_REGISTER47_OFS 0x0368
REGDEF_BEGIN(IPE_RESERVED_REGISTER47)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER47)


/*
    reserved:    [0x0, 0x3],            bits : 31_30
*/
#define IPE_RESERVED_REGISTER48_OFS 0x036c
REGDEF_BEGIN(IPE_RESERVED_REGISTER48)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER48)


/*
    reserved:    [0x0, 0x1],            bits : 31_31
*/
#define IPE_RESERVED_REGISTER49_OFS 0x0370
REGDEF_BEGIN(IPE_RESERVED_REGISTER49)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER49)


/*
    reserved:    [0x0, 0x3],            bits : 31_32
*/
#define IPE_RESERVED_REGISTER50_OFS 0x0374
REGDEF_BEGIN(IPE_RESERVED_REGISTER50)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER50)


/*
    reserved:    [0x0, 0x7],            bits : 31_33
*/
#define IPE_RESERVED_REGISTER51_OFS 0x0378
REGDEF_BEGIN(IPE_RESERVED_REGISTER51)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER51)


/*
    reserved:    [0x0, 0xf],            bits : 31_34
*/
#define IPE_RESERVED_REGISTER52_OFS 0x037c
REGDEF_BEGIN(IPE_RESERVED_REGISTER52)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER52)


/*
    reserved:    [0x0, 0x1f],           bits : 31_35
*/
#define IPE_RESERVED_REGISTER53_OFS 0x0380
REGDEF_BEGIN(IPE_RESERVED_REGISTER53)
REGDEF_BIT(reserved,        31)
REGDEF_END(IPE_RESERVED_REGISTER53)


/*
    lce_wt_diff_pos:    [0x0, 0xff],            bits : 7_0
    lce_wt_diff_neg:    [0x0, 0xff],            bits : 15_8
    lce_wt_diff_avg:    [0x0, 0xff],            bits : 23_16
*/
#define LCE_REGISTER_0_OFS 0x0384
REGDEF_BEGIN(LCE_REGISTER_0)
REGDEF_BIT(lce_wt_diff_pos,        8)
REGDEF_BIT(lce_wt_diff_neg,        8)
REGDEF_BIT(lce_wt_diff_avg,        8)
REGDEF_END(LCE_REGISTER_0)


/*
    lce_lum_adj_lut0:    [0x0, 0xff],           bits : 7_0
    lce_lum_adj_lut1:    [0x0, 0xff],           bits : 15_8
    lce_lum_adj_lut2:    [0x0, 0xff],           bits : 23_16
    lce_lum_adj_lut3:    [0x0, 0xff],           bits : 31_24
*/
#define LCE_REGISTER_1_OFS 0x0388
REGDEF_BEGIN(LCE_REGISTER_1)
REGDEF_BIT(lce_lum_adj_lut0,        8)
REGDEF_BIT(lce_lum_adj_lut1,        8)
REGDEF_BIT(lce_lum_adj_lut2,        8)
REGDEF_BIT(lce_lum_adj_lut3,        8)
REGDEF_END(LCE_REGISTER_1)


/*
    lce_lum_adj_lut4:    [0x0, 0xff],           bits : 7_0
    lce_lum_adj_lut5:    [0x0, 0xff],           bits : 15_8
    lce_lum_adj_lut6:    [0x0, 0xff],           bits : 23_16
    lce_lum_adj_lut7:    [0x0, 0xff],           bits : 31_24
*/
#define LCE_REGISTER_2_OFS 0x038c
REGDEF_BEGIN(LCE_REGISTER_2)
REGDEF_BIT(lce_lum_adj_lut4,        8)
REGDEF_BIT(lce_lum_adj_lut5,        8)
REGDEF_BIT(lce_lum_adj_lut6,        8)
REGDEF_BIT(lce_lum_adj_lut7,        8)
REGDEF_END(LCE_REGISTER_2)


/*
    lce_lum_adj_lut8:    [0x0, 0xff],           bits : 7_0
*/
#define LCE_REGISTER_3_OFS 0x0390
REGDEF_BEGIN(LCE_REGISTER_3)
REGDEF_BIT(lce_lum_adj_lut8,        8)
REGDEF_END(LCE_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER54_OFS 0x0394
REGDEF_BEGIN(IPE_RESERVED_REGISTER54)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER54)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER55_OFS 0x0398
REGDEF_BEGIN(IPE_RESERVED_REGISTER55)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER55)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER56_OFS 0x039c
REGDEF_BEGIN(IPE_RESERVED_REGISTER56)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER56)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER57_OFS 0x03a0
REGDEF_BEGIN(IPE_RESERVED_REGISTER57)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER57)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER58_OFS 0x03a4
REGDEF_BEGIN(IPE_RESERVED_REGISTER58)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER58)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER59_OFS 0x03a8
REGDEF_BEGIN(IPE_RESERVED_REGISTER59)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER59)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER60_OFS 0x03ac
REGDEF_BEGIN(IPE_RESERVED_REGISTER60)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER60)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER61_OFS 0x03b0
REGDEF_BEGIN(IPE_RESERVED_REGISTER61)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER61)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER62_OFS 0x03b4
REGDEF_BEGIN(IPE_RESERVED_REGISTER62)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER62)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER63_OFS 0x03b8
REGDEF_BEGIN(IPE_RESERVED_REGISTER63)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER63)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER64_OFS 0x03bc
REGDEF_BEGIN(IPE_RESERVED_REGISTER64)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER64)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER65_OFS 0x03c0
REGDEF_BEGIN(IPE_RESERVED_REGISTER65)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER65)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER66_OFS 0x03c4
REGDEF_BEGIN(IPE_RESERVED_REGISTER66)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER66)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER67_OFS 0x03c8
REGDEF_BEGIN(IPE_RESERVED_REGISTER67)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER67)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER68_OFS 0x03cc
REGDEF_BEGIN(IPE_RESERVED_REGISTER68)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER68)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER69_OFS 0x03d0
REGDEF_BEGIN(IPE_RESERVED_REGISTER69)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER69)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER70_OFS 0x03d4
REGDEF_BEGIN(IPE_RESERVED_REGISTER70)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER70)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER71_OFS 0x03d8
REGDEF_BEGIN(IPE_RESERVED_REGISTER71)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER71)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER72_OFS 0x03dc
REGDEF_BEGIN(IPE_RESERVED_REGISTER72)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER72)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER73_OFS 0x03e0
REGDEF_BEGIN(IPE_RESERVED_REGISTER73)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER73)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER74_OFS 0x03e4
REGDEF_BEGIN(IPE_RESERVED_REGISTER74)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER74)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER75_OFS 0x03e8
REGDEF_BEGIN(IPE_RESERVED_REGISTER75)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER75)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER76_OFS 0x03ec
REGDEF_BEGIN(IPE_RESERVED_REGISTER76)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER76)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER77_OFS 0x03f0
REGDEF_BEGIN(IPE_RESERVED_REGISTER77)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER77)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER78_OFS 0x03f4
REGDEF_BEGIN(IPE_RESERVED_REGISTER78)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER78)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER79_OFS 0x03f8
REGDEF_BEGIN(IPE_RESERVED_REGISTER79)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER79)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED_REGISTER80_OFS 0x03fc
REGDEF_BEGIN(IPE_RESERVED_REGISTER80)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED_REGISTER80)


/*
    r_lut0:    [0x0, 0x3ff],            bits : 9_0
    r_lut1:    [0x0, 0x3ff],            bits : 19_10
    r_lut2:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_R_GAMMA0_OFS 0x0400
REGDEF_BEGIN(SRAM_R_GAMMA0)
REGDEF_BIT(r_lut0,        10)
REGDEF_BIT(r_lut1,        10)
REGDEF_BIT(r_lut2,        10)
REGDEF_END(SRAM_R_GAMMA0)


/*
    r_lut2:    [0x0, 0x3ff],            bits : 9_0
    r_lut3:    [0x0, 0x3ff],            bits : 19_10
    r_lut4:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_R_GAMMA1_OFS 0x0404
REGDEF_BEGIN(SRAM_R_GAMMA1)
REGDEF_BIT(r_lut2,        10)
REGDEF_BIT(r_lut3,        10)
REGDEF_BIT(r_lut4,        10)
REGDEF_END(SRAM_R_GAMMA1)


/*
    r_lut4:    [0x0, 0x3ff],            bits : 9_0
    r_lut5:    [0x0, 0x3ff],            bits : 19_10
    r_lut6:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_R_GAMMA2_OFS 0x0408
REGDEF_BEGIN(SRAM_R_GAMMA2)
REGDEF_BIT(r_lut4,        10)
REGDEF_BIT(r_lut5,        10)
REGDEF_BIT(r_lut6,        10)
REGDEF_END(SRAM_R_GAMMA2)


/*
    r_lut6:    [0x0, 0x3ff],            bits : 9_0
    r_lut7:    [0x0, 0x3ff],            bits : 19_10
    r_lut8:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_R_GAMMA3_OFS 0x040c
REGDEF_BEGIN(SRAM_R_GAMMA3)
REGDEF_BIT(r_lut6,        10)
REGDEF_BIT(r_lut7,        10)
REGDEF_BIT(r_lut8,        10)
REGDEF_END(SRAM_R_GAMMA3)


/*
    r_lut8 :    [0x0, 0x3ff],           bits : 9_0
    r_lut9 :    [0x0, 0x3ff],           bits : 19_10
    r_lut10:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA4_OFS 0x0410
REGDEF_BEGIN(SRAM_R_GAMMA4)
REGDEF_BIT(r_lut8,        10)
REGDEF_BIT(r_lut9,        10)
REGDEF_BIT(r_lut10,        10)
REGDEF_END(SRAM_R_GAMMA4)


/*
    r_lut10:    [0x0, 0x3ff],           bits : 9_0
    r_lut11:    [0x0, 0x3ff],           bits : 19_10
    r_lut12:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA5_OFS 0x0414
REGDEF_BEGIN(SRAM_R_GAMMA5)
REGDEF_BIT(r_lut10,        10)
REGDEF_BIT(r_lut11,        10)
REGDEF_BIT(r_lut12,        10)
REGDEF_END(SRAM_R_GAMMA5)


/*
    r_lut12:    [0x0, 0x3ff],           bits : 9_0
    r_lut13:    [0x0, 0x3ff],           bits : 19_10
    r_lut14:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA6_OFS 0x0418
REGDEF_BEGIN(SRAM_R_GAMMA6)
REGDEF_BIT(r_lut12,        10)
REGDEF_BIT(r_lut13,        10)
REGDEF_BIT(r_lut14,        10)
REGDEF_END(SRAM_R_GAMMA6)


/*
    r_lut14:    [0x0, 0x3ff],           bits : 9_0
    r_lut15:    [0x0, 0x3ff],           bits : 19_10
    r_lut16:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA7_OFS 0x041c
REGDEF_BEGIN(SRAM_R_GAMMA7)
REGDEF_BIT(r_lut14,        10)
REGDEF_BIT(r_lut15,        10)
REGDEF_BIT(r_lut16,        10)
REGDEF_END(SRAM_R_GAMMA7)


/*
    r_lut16:    [0x0, 0x3ff],           bits : 9_0
    r_lut17:    [0x0, 0x3ff],           bits : 19_10
    r_lut18:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA8_OFS 0x0420
REGDEF_BEGIN(SRAM_R_GAMMA8)
REGDEF_BIT(r_lut16,        10)
REGDEF_BIT(r_lut17,        10)
REGDEF_BIT(r_lut18,        10)
REGDEF_END(SRAM_R_GAMMA8)


/*
    r_lut18:    [0x0, 0x3ff],           bits : 9_0
    r_lut19:    [0x0, 0x3ff],           bits : 19_10
    r_lut20:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA9_OFS 0x0424
REGDEF_BEGIN(SRAM_R_GAMMA9)
REGDEF_BIT(r_lut18,        10)
REGDEF_BIT(r_lut19,        10)
REGDEF_BIT(r_lut20,        10)
REGDEF_END(SRAM_R_GAMMA9)


/*
    r_lut20:    [0x0, 0x3ff],           bits : 9_0
    r_lut21:    [0x0, 0x3ff],           bits : 19_10
    r_lut22:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA10_OFS 0x0428
REGDEF_BEGIN(SRAM_R_GAMMA10)
REGDEF_BIT(r_lut20,        10)
REGDEF_BIT(r_lut21,        10)
REGDEF_BIT(r_lut22,        10)
REGDEF_END(SRAM_R_GAMMA10)


/*
    r_lut22:    [0x0, 0x3ff],           bits : 9_0
    r_lut23:    [0x0, 0x3ff],           bits : 19_10
    r_lut24:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA11_OFS 0x042c
REGDEF_BEGIN(SRAM_R_GAMMA11)
REGDEF_BIT(r_lut22,        10)
REGDEF_BIT(r_lut23,        10)
REGDEF_BIT(r_lut24,        10)
REGDEF_END(SRAM_R_GAMMA11)


/*
    r_lut24:    [0x0, 0x3ff],           bits : 9_0
    r_lut25:    [0x0, 0x3ff],           bits : 19_10
    r_lut26:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA12_OFS 0x0430
REGDEF_BEGIN(SRAM_R_GAMMA12)
REGDEF_BIT(r_lut24,        10)
REGDEF_BIT(r_lut25,        10)
REGDEF_BIT(r_lut26,        10)
REGDEF_END(SRAM_R_GAMMA12)


/*
    r_lut26:    [0x0, 0x3ff],           bits : 9_0
    r_lut27:    [0x0, 0x3ff],           bits : 19_10
    r_lut28:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA13_OFS 0x0434
REGDEF_BEGIN(SRAM_R_GAMMA13)
REGDEF_BIT(r_lut26,        10)
REGDEF_BIT(r_lut27,        10)
REGDEF_BIT(r_lut28,        10)
REGDEF_END(SRAM_R_GAMMA13)


/*
    r_lut28:    [0x0, 0x3ff],           bits : 9_0
    r_lut29:    [0x0, 0x3ff],           bits : 19_10
    r_lut30:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA14_OFS 0x0438
REGDEF_BEGIN(SRAM_R_GAMMA14)
REGDEF_BIT(r_lut28,        10)
REGDEF_BIT(r_lut29,        10)
REGDEF_BIT(r_lut30,        10)
REGDEF_END(SRAM_R_GAMMA14)


/*
    r_lut30:    [0x0, 0x3ff],           bits : 9_0
    r_lut31:    [0x0, 0x3ff],           bits : 19_10
    r_lut32:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA15_OFS 0x043c
REGDEF_BEGIN(SRAM_R_GAMMA15)
REGDEF_BIT(r_lut30,        10)
REGDEF_BIT(r_lut31,        10)
REGDEF_BIT(r_lut32,        10)
REGDEF_END(SRAM_R_GAMMA15)


/*
    r_lut32:    [0x0, 0x3ff],           bits : 9_0
    r_lut33:    [0x0, 0x3ff],           bits : 19_10
    r_lut34:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA16_OFS 0x0440
REGDEF_BEGIN(SRAM_R_GAMMA16)
REGDEF_BIT(r_lut32,        10)
REGDEF_BIT(r_lut33,        10)
REGDEF_BIT(r_lut34,        10)
REGDEF_END(SRAM_R_GAMMA16)


/*
    r_lut34:    [0x0, 0x3ff],           bits : 9_0
    r_lut35:    [0x0, 0x3ff],           bits : 19_10
    r_lut36:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA17_OFS 0x0444
REGDEF_BEGIN(SRAM_R_GAMMA17)
REGDEF_BIT(r_lut34,        10)
REGDEF_BIT(r_lut35,        10)
REGDEF_BIT(r_lut36,        10)
REGDEF_END(SRAM_R_GAMMA17)


/*
    r_lut36:    [0x0, 0x3ff],           bits : 9_0
    r_lut37:    [0x0, 0x3ff],           bits : 19_10
    r_lut38:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA18_OFS 0x0448
REGDEF_BEGIN(SRAM_R_GAMMA18)
REGDEF_BIT(r_lut36,        10)
REGDEF_BIT(r_lut37,        10)
REGDEF_BIT(r_lut38,        10)
REGDEF_END(SRAM_R_GAMMA18)


/*
    r_lut38:    [0x0, 0x3ff],           bits : 9_0
    r_lut39:    [0x0, 0x3ff],           bits : 19_10
    r_lut40:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA19_OFS 0x044c
REGDEF_BEGIN(SRAM_R_GAMMA19)
REGDEF_BIT(r_lut38,        10)
REGDEF_BIT(r_lut39,        10)
REGDEF_BIT(r_lut40,        10)
REGDEF_END(SRAM_R_GAMMA19)


/*
    r_lut40:    [0x0, 0x3ff],           bits : 9_0
    r_lut41:    [0x0, 0x3ff],           bits : 19_10
    r_lut42:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA20_OFS 0x0450
REGDEF_BEGIN(SRAM_R_GAMMA20)
REGDEF_BIT(r_lut40,        10)
REGDEF_BIT(r_lut41,        10)
REGDEF_BIT(r_lut42,        10)
REGDEF_END(SRAM_R_GAMMA20)


/*
    r_lut42:    [0x0, 0x3ff],           bits : 9_0
    r_lut43:    [0x0, 0x3ff],           bits : 19_10
    r_lut44:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA21_OFS 0x0454
REGDEF_BEGIN(SRAM_R_GAMMA21)
REGDEF_BIT(r_lut42,        10)
REGDEF_BIT(r_lut43,        10)
REGDEF_BIT(r_lut44,        10)
REGDEF_END(SRAM_R_GAMMA21)


/*
    r_lut44:    [0x0, 0x3ff],           bits : 9_0
    r_lut45:    [0x0, 0x3ff],           bits : 19_10
    r_lut46:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA22_OFS 0x0458
REGDEF_BEGIN(SRAM_R_GAMMA22)
REGDEF_BIT(r_lut44,        10)
REGDEF_BIT(r_lut45,        10)
REGDEF_BIT(r_lut46,        10)
REGDEF_END(SRAM_R_GAMMA22)


/*
    r_lut46:    [0x0, 0x3ff],           bits : 9_0
    r_lut47:    [0x0, 0x3ff],           bits : 19_10
    r_lut48:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA23_OFS 0x045c
REGDEF_BEGIN(SRAM_R_GAMMA23)
REGDEF_BIT(r_lut46,        10)
REGDEF_BIT(r_lut47,        10)
REGDEF_BIT(r_lut48,        10)
REGDEF_END(SRAM_R_GAMMA23)


/*
    r_lut48:    [0x0, 0x3ff],           bits : 9_0
    r_lut49:    [0x0, 0x3ff],           bits : 19_10
    r_lut50:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA24_OFS 0x0460
REGDEF_BEGIN(SRAM_R_GAMMA24)
REGDEF_BIT(r_lut48,        10)
REGDEF_BIT(r_lut49,        10)
REGDEF_BIT(r_lut50,        10)
REGDEF_END(SRAM_R_GAMMA24)


/*
    r_lut50:    [0x0, 0x3ff],           bits : 9_0
    r_lut51:    [0x0, 0x3ff],           bits : 19_10
    r_lut52:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA25_OFS 0x0464
REGDEF_BEGIN(SRAM_R_GAMMA25)
REGDEF_BIT(r_lut50,        10)
REGDEF_BIT(r_lut51,        10)
REGDEF_BIT(r_lut52,        10)
REGDEF_END(SRAM_R_GAMMA25)


/*
    r_lut52:    [0x0, 0x3ff],           bits : 9_0
    r_lut53:    [0x0, 0x3ff],           bits : 19_10
    r_lut54:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA26_OFS 0x0468
REGDEF_BEGIN(SRAM_R_GAMMA26)
REGDEF_BIT(r_lut52,        10)
REGDEF_BIT(r_lut53,        10)
REGDEF_BIT(r_lut54,        10)
REGDEF_END(SRAM_R_GAMMA26)


/*
    r_lut54:    [0x0, 0x3ff],           bits : 9_0
    r_lut55:    [0x0, 0x3ff],           bits : 19_10
    r_lut56:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA27_OFS 0x046c
REGDEF_BEGIN(SRAM_R_GAMMA27)
REGDEF_BIT(r_lut54,        10)
REGDEF_BIT(r_lut55,        10)
REGDEF_BIT(r_lut56,        10)
REGDEF_END(SRAM_R_GAMMA27)


/*
    r_lut56:    [0x0, 0x3ff],           bits : 9_0
    r_lut57:    [0x0, 0x3ff],           bits : 19_10
    r_lut58:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA28_OFS 0x0470
REGDEF_BEGIN(SRAM_R_GAMMA28)
REGDEF_BIT(r_lut56,        10)
REGDEF_BIT(r_lut57,        10)
REGDEF_BIT(r_lut58,        10)
REGDEF_END(SRAM_R_GAMMA28)


/*
    r_lut58:    [0x0, 0x3ff],           bits : 9_0
    r_lut59:    [0x0, 0x3ff],           bits : 19_10
    r_lut60:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA29_OFS 0x0474
REGDEF_BEGIN(SRAM_R_GAMMA29)
REGDEF_BIT(r_lut58,        10)
REGDEF_BIT(r_lut59,        10)
REGDEF_BIT(r_lut60,        10)
REGDEF_END(SRAM_R_GAMMA29)


/*
    r_lut60:    [0x0, 0x3ff],           bits : 9_0
    r_lut61:    [0x0, 0x3ff],           bits : 19_10
    r_lut62:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA30_OFS 0x0478
REGDEF_BEGIN(SRAM_R_GAMMA30)
REGDEF_BIT(r_lut60,        10)
REGDEF_BIT(r_lut61,        10)
REGDEF_BIT(r_lut62,        10)
REGDEF_END(SRAM_R_GAMMA30)


/*
    r_lut62:    [0x0, 0x3ff],           bits : 9_0
    r_lut63:    [0x0, 0x3ff],           bits : 19_10
    r_lut64:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA31_OFS 0x047c
REGDEF_BEGIN(SRAM_R_GAMMA31)
REGDEF_BIT(r_lut62,        10)
REGDEF_BIT(r_lut63,        10)
REGDEF_BIT(r_lut64,        10)
REGDEF_END(SRAM_R_GAMMA31)


/*
    r_lut64:    [0x0, 0x3ff],           bits : 9_0
    r_lut65:    [0x0, 0x3ff],           bits : 19_10
    r_lut66:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA32_OFS 0x0480
REGDEF_BEGIN(SRAM_R_GAMMA32)
REGDEF_BIT(r_lut64,        10)
REGDEF_BIT(r_lut65,        10)
REGDEF_BIT(r_lut66,        10)
REGDEF_END(SRAM_R_GAMMA32)


/*
    r_lut66:    [0x0, 0x3ff],           bits : 9_0
    r_lut67:    [0x0, 0x3ff],           bits : 19_10
    r_lut68:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA33_OFS 0x0484
REGDEF_BEGIN(SRAM_R_GAMMA33)
REGDEF_BIT(r_lut66,        10)
REGDEF_BIT(r_lut67,        10)
REGDEF_BIT(r_lut68,        10)
REGDEF_END(SRAM_R_GAMMA33)


/*
    r_lut68:    [0x0, 0x3ff],           bits : 9_0
    r_lut69:    [0x0, 0x3ff],           bits : 19_10
    r_lut70:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA34_OFS 0x0488
REGDEF_BEGIN(SRAM_R_GAMMA34)
REGDEF_BIT(r_lut68,        10)
REGDEF_BIT(r_lut69,        10)
REGDEF_BIT(r_lut70,        10)
REGDEF_END(SRAM_R_GAMMA34)


/*
    r_lut70:    [0x0, 0x3ff],           bits : 9_0
    r_lut71:    [0x0, 0x3ff],           bits : 19_10
    r_lut72:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA35_OFS 0x048c
REGDEF_BEGIN(SRAM_R_GAMMA35)
REGDEF_BIT(r_lut70,        10)
REGDEF_BIT(r_lut71,        10)
REGDEF_BIT(r_lut72,        10)
REGDEF_END(SRAM_R_GAMMA35)


/*
    r_lut72:    [0x0, 0x3ff],           bits : 9_0
    r_lut73:    [0x0, 0x3ff],           bits : 19_10
    r_lut74:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA36_OFS 0x0490
REGDEF_BEGIN(SRAM_R_GAMMA36)
REGDEF_BIT(r_lut72,        10)
REGDEF_BIT(r_lut73,        10)
REGDEF_BIT(r_lut74,        10)
REGDEF_END(SRAM_R_GAMMA36)


/*
    r_lut74:    [0x0, 0x3ff],           bits : 9_0
    r_lut75:    [0x0, 0x3ff],           bits : 19_10
    r_lut76:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA37_OFS 0x0494
REGDEF_BEGIN(SRAM_R_GAMMA37)
REGDEF_BIT(r_lut74,        10)
REGDEF_BIT(r_lut75,        10)
REGDEF_BIT(r_lut76,        10)
REGDEF_END(SRAM_R_GAMMA37)


/*
    r_lut76:    [0x0, 0x3ff],           bits : 9_0
    r_lut77:    [0x0, 0x3ff],           bits : 19_10
    r_lut78:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA38_OFS 0x0498
REGDEF_BEGIN(SRAM_R_GAMMA38)
REGDEF_BIT(r_lut76,        10)
REGDEF_BIT(r_lut77,        10)
REGDEF_BIT(r_lut78,        10)
REGDEF_END(SRAM_R_GAMMA38)


/*
    r_lut78:    [0x0, 0x3ff],           bits : 9_0
    r_lut79:    [0x0, 0x3ff],           bits : 19_10
    r_lut80:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA39_OFS 0x049c
REGDEF_BEGIN(SRAM_R_GAMMA39)
REGDEF_BIT(r_lut78,        10)
REGDEF_BIT(r_lut79,        10)
REGDEF_BIT(r_lut80,        10)
REGDEF_END(SRAM_R_GAMMA39)


/*
    r_lut80:    [0x0, 0x3ff],           bits : 9_0
    r_lut81:    [0x0, 0x3ff],           bits : 19_10
    r_lut82:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA40_OFS 0x04a0
REGDEF_BEGIN(SRAM_R_GAMMA40)
REGDEF_BIT(r_lut80,        10)
REGDEF_BIT(r_lut81,        10)
REGDEF_BIT(r_lut82,        10)
REGDEF_END(SRAM_R_GAMMA40)


/*
    r_lut82:    [0x0, 0x3ff],           bits : 9_0
    r_lut83:    [0x0, 0x3ff],           bits : 19_10
    r_lut84:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA41_OFS 0x04a4
REGDEF_BEGIN(SRAM_R_GAMMA41)
REGDEF_BIT(r_lut82,        10)
REGDEF_BIT(r_lut83,        10)
REGDEF_BIT(r_lut84,        10)
REGDEF_END(SRAM_R_GAMMA41)


/*
    r_lut84:    [0x0, 0x3ff],           bits : 9_0
    r_lut85:    [0x0, 0x3ff],           bits : 19_10
    r_lut86:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA42_OFS 0x04a8
REGDEF_BEGIN(SRAM_R_GAMMA42)
REGDEF_BIT(r_lut84,        10)
REGDEF_BIT(r_lut85,        10)
REGDEF_BIT(r_lut86,        10)
REGDEF_END(SRAM_R_GAMMA42)


/*
    r_lut86:    [0x0, 0x3ff],           bits : 9_0
    r_lut87:    [0x0, 0x3ff],           bits : 19_10
    r_lut88:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA43_OFS 0x04ac
REGDEF_BEGIN(SRAM_R_GAMMA43)
REGDEF_BIT(r_lut86,        10)
REGDEF_BIT(r_lut87,        10)
REGDEF_BIT(r_lut88,        10)
REGDEF_END(SRAM_R_GAMMA43)


/*
    r_lut88:    [0x0, 0x3ff],           bits : 9_0
    r_lut89:    [0x0, 0x3ff],           bits : 19_10
    r_lut90:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA44_OFS 0x04b0
REGDEF_BEGIN(SRAM_R_GAMMA44)
REGDEF_BIT(r_lut88,        10)
REGDEF_BIT(r_lut89,        10)
REGDEF_BIT(r_lut90,        10)
REGDEF_END(SRAM_R_GAMMA44)


/*
    r_lut90:    [0x0, 0x3ff],           bits : 9_0
    r_lut91:    [0x0, 0x3ff],           bits : 19_10
    r_lut92:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA45_OFS 0x04b4
REGDEF_BEGIN(SRAM_R_GAMMA45)
REGDEF_BIT(r_lut90,        10)
REGDEF_BIT(r_lut91,        10)
REGDEF_BIT(r_lut92,        10)
REGDEF_END(SRAM_R_GAMMA45)


/*
    r_lut92:    [0x0, 0x3ff],           bits : 9_0
    r_lut93:    [0x0, 0x3ff],           bits : 19_10
    r_lut94:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA46_OFS 0x04b8
REGDEF_BEGIN(SRAM_R_GAMMA46)
REGDEF_BIT(r_lut92,        10)
REGDEF_BIT(r_lut93,        10)
REGDEF_BIT(r_lut94,        10)
REGDEF_END(SRAM_R_GAMMA46)


/*
    r_lut94:    [0x0, 0x3ff],           bits : 9_0
    r_lut95:    [0x0, 0x3ff],           bits : 19_10
    r_lut96:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA47_OFS 0x04bc
REGDEF_BEGIN(SRAM_R_GAMMA47)
REGDEF_BIT(r_lut94,        10)
REGDEF_BIT(r_lut95,        10)
REGDEF_BIT(r_lut96,        10)
REGDEF_END(SRAM_R_GAMMA47)


/*
    r_lut96:    [0x0, 0x3ff],           bits : 9_0
    r_lut97:    [0x0, 0x3ff],           bits : 19_10
    r_lut98:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA48_OFS 0x04c0
REGDEF_BEGIN(SRAM_R_GAMMA48)
REGDEF_BIT(r_lut96,        10)
REGDEF_BIT(r_lut97,        10)
REGDEF_BIT(r_lut98,        10)
REGDEF_END(SRAM_R_GAMMA48)


/*
    r_lut98 :    [0x0, 0x3ff],          bits : 9_0
    r_lut99 :    [0x0, 0x3ff],          bits : 19_10
    r_lut100:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA49_OFS 0x04c4
REGDEF_BEGIN(SRAM_R_GAMMA49)
REGDEF_BIT(r_lut98,        10)
REGDEF_BIT(r_lut99,        10)
REGDEF_BIT(r_lut100,        10)
REGDEF_END(SRAM_R_GAMMA49)


/*
    r_lut100:    [0x0, 0x3ff],          bits : 9_0
    r_lut101:    [0x0, 0x3ff],          bits : 19_10
    r_lut102:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA50_OFS 0x04c8
REGDEF_BEGIN(SRAM_R_GAMMA50)
REGDEF_BIT(r_lut100,        10)
REGDEF_BIT(r_lut101,        10)
REGDEF_BIT(r_lut102,        10)
REGDEF_END(SRAM_R_GAMMA50)


/*
    r_lut102:    [0x0, 0x3ff],          bits : 9_0
    r_lut103:    [0x0, 0x3ff],          bits : 19_10
    r_lut104:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA51_OFS 0x04cc
REGDEF_BEGIN(SRAM_R_GAMMA51)
REGDEF_BIT(r_lut102,        10)
REGDEF_BIT(r_lut103,        10)
REGDEF_BIT(r_lut104,        10)
REGDEF_END(SRAM_R_GAMMA51)


/*
    r_lut104:    [0x0, 0x3ff],          bits : 9_0
    r_lut105:    [0x0, 0x3ff],          bits : 19_10
    r_lut106:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA52_OFS 0x04d0
REGDEF_BEGIN(SRAM_R_GAMMA52)
REGDEF_BIT(r_lut104,        10)
REGDEF_BIT(r_lut105,        10)
REGDEF_BIT(r_lut106,        10)
REGDEF_END(SRAM_R_GAMMA52)


/*
    r_lut106:    [0x0, 0x3ff],          bits : 9_0
    r_lut107:    [0x0, 0x3ff],          bits : 19_10
    r_lut108:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA53_OFS 0x04d4
REGDEF_BEGIN(SRAM_R_GAMMA53)
REGDEF_BIT(r_lut106,        10)
REGDEF_BIT(r_lut107,        10)
REGDEF_BIT(r_lut108,        10)
REGDEF_END(SRAM_R_GAMMA53)


/*
    r_lut108:    [0x0, 0x3ff],          bits : 9_0
    r_lut109:    [0x0, 0x3ff],          bits : 19_10
    r_lut110:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA54_OFS 0x04d8
REGDEF_BEGIN(SRAM_R_GAMMA54)
REGDEF_BIT(r_lut108,        10)
REGDEF_BIT(r_lut109,        10)
REGDEF_BIT(r_lut110,        10)
REGDEF_END(SRAM_R_GAMMA54)


/*
    r_lut110:    [0x0, 0x3ff],          bits : 9_0
    r_lut111:    [0x0, 0x3ff],          bits : 19_10
    r_lut112:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA55_OFS 0x04dc
REGDEF_BEGIN(SRAM_R_GAMMA55)
REGDEF_BIT(r_lut110,        10)
REGDEF_BIT(r_lut111,        10)
REGDEF_BIT(r_lut112,        10)
REGDEF_END(SRAM_R_GAMMA55)


/*
    r_lut112:    [0x0, 0x3ff],          bits : 9_0
    r_lut113:    [0x0, 0x3ff],          bits : 19_10
    r_lut114:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA56_OFS 0x04e0
REGDEF_BEGIN(SRAM_R_GAMMA56)
REGDEF_BIT(r_lut112,        10)
REGDEF_BIT(r_lut113,        10)
REGDEF_BIT(r_lut114,        10)
REGDEF_END(SRAM_R_GAMMA56)


/*
    r_lut114:    [0x0, 0x3ff],          bits : 9_0
    r_lut115:    [0x0, 0x3ff],          bits : 19_10
    r_lut116:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA57_OFS 0x04e4
REGDEF_BEGIN(SRAM_R_GAMMA57)
REGDEF_BIT(r_lut114,        10)
REGDEF_BIT(r_lut115,        10)
REGDEF_BIT(r_lut116,        10)
REGDEF_END(SRAM_R_GAMMA57)


/*
    r_lut116:    [0x0, 0x3ff],          bits : 9_0
    r_lut117:    [0x0, 0x3ff],          bits : 19_10
    r_lut118:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA58_OFS 0x04e8
REGDEF_BEGIN(SRAM_R_GAMMA58)
REGDEF_BIT(r_lut116,        10)
REGDEF_BIT(r_lut117,        10)
REGDEF_BIT(r_lut118,        10)
REGDEF_END(SRAM_R_GAMMA58)


/*
    r_lut118:    [0x0, 0x3ff],          bits : 9_0
    r_lut119:    [0x0, 0x3ff],          bits : 19_10
    r_lut120:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA59_OFS 0x04ec
REGDEF_BEGIN(SRAM_R_GAMMA59)
REGDEF_BIT(r_lut118,        10)
REGDEF_BIT(r_lut119,        10)
REGDEF_BIT(r_lut120,        10)
REGDEF_END(SRAM_R_GAMMA59)


/*
    r_lut120:    [0x0, 0x3ff],          bits : 9_0
    r_lut121:    [0x0, 0x3ff],          bits : 19_10
    r_lut122:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA60_OFS 0x04f0
REGDEF_BEGIN(SRAM_R_GAMMA60)
REGDEF_BIT(r_lut120,        10)
REGDEF_BIT(r_lut121,        10)
REGDEF_BIT(r_lut122,        10)
REGDEF_END(SRAM_R_GAMMA60)


/*
    r_lut122:    [0x0, 0x3ff],          bits : 9_0
    r_lut123:    [0x0, 0x3ff],          bits : 19_10
    r_lut124:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA61_OFS 0x04f4
REGDEF_BEGIN(SRAM_R_GAMMA61)
REGDEF_BIT(r_lut122,        10)
REGDEF_BIT(r_lut123,        10)
REGDEF_BIT(r_lut124,        10)
REGDEF_END(SRAM_R_GAMMA61)


/*
    r_lut124:    [0x0, 0x3ff],          bits : 9_0
    r_lut125:    [0x0, 0x3ff],          bits : 19_10
    r_lut126:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA62_OFS 0x04f8
REGDEF_BEGIN(SRAM_R_GAMMA62)
REGDEF_BIT(r_lut124,        10)
REGDEF_BIT(r_lut125,        10)
REGDEF_BIT(r_lut126,        10)
REGDEF_END(SRAM_R_GAMMA62)


/*
    r_lut126:    [0x0, 0x3ff],          bits : 9_0
    r_lut127:    [0x0, 0x3ff],          bits : 19_10
    r_lut128:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA63_OFS 0x04fc
REGDEF_BEGIN(SRAM_R_GAMMA63)
REGDEF_BIT(r_lut126,        10)
REGDEF_BIT(r_lut127,        10)
REGDEF_BIT(r_lut128,        10)
REGDEF_END(SRAM_R_GAMMA63)


/*
    g_lut0:    [0x0, 0x3ff],            bits : 9_0
    g_lut1:    [0x0, 0x3ff],            bits : 19_10
    g_lut2:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_G_GAMMA0_OFS 0x0500
REGDEF_BEGIN(SRAM_G_GAMMA0)
REGDEF_BIT(g_lut0,        10)
REGDEF_BIT(g_lut1,        10)
REGDEF_BIT(g_lut2,        10)
REGDEF_END(SRAM_G_GAMMA0)


/*
    g_lut2:    [0x0, 0x3ff],            bits : 9_0
    g_lut3:    [0x0, 0x3ff],            bits : 19_10
    g_lut4:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_G_GAMMA1_OFS 0x0504
REGDEF_BEGIN(SRAM_G_GAMMA1)
REGDEF_BIT(g_lut2,        10)
REGDEF_BIT(g_lut3,        10)
REGDEF_BIT(g_lut4,        10)
REGDEF_END(SRAM_G_GAMMA1)


/*
    g_lut4:    [0x0, 0x3ff],            bits : 9_0
    g_lut5:    [0x0, 0x3ff],            bits : 19_10
    g_lut6:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_G_GAMMA2_OFS 0x0508
REGDEF_BEGIN(SRAM_G_GAMMA2)
REGDEF_BIT(g_lut4,        10)
REGDEF_BIT(g_lut5,        10)
REGDEF_BIT(g_lut6,        10)
REGDEF_END(SRAM_G_GAMMA2)


/*
    g_lut6:    [0x0, 0x3ff],            bits : 9_0
    g_lut7:    [0x0, 0x3ff],            bits : 19_10
    g_lut8:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_G_GAMMA3_OFS 0x050c
REGDEF_BEGIN(SRAM_G_GAMMA3)
REGDEF_BIT(g_lut6,        10)
REGDEF_BIT(g_lut7,        10)
REGDEF_BIT(g_lut8,        10)
REGDEF_END(SRAM_G_GAMMA3)


/*
    g_lut8 :    [0x0, 0x3ff],           bits : 9_0
    g_lut9 :    [0x0, 0x3ff],           bits : 19_10
    g_lut10:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA4_OFS 0x0510
REGDEF_BEGIN(SRAM_G_GAMMA4)
REGDEF_BIT(g_lut8,        10)
REGDEF_BIT(g_lut9,        10)
REGDEF_BIT(g_lut10,        10)
REGDEF_END(SRAM_G_GAMMA4)


/*
    g_lut10:    [0x0, 0x3ff],           bits : 9_0
    g_lut11:    [0x0, 0x3ff],           bits : 19_10
    g_lut12:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA5_OFS 0x0514
REGDEF_BEGIN(SRAM_G_GAMMA5)
REGDEF_BIT(g_lut10,        10)
REGDEF_BIT(g_lut11,        10)
REGDEF_BIT(g_lut12,        10)
REGDEF_END(SRAM_G_GAMMA5)


/*
    g_lut12:    [0x0, 0x3ff],           bits : 9_0
    g_lut13:    [0x0, 0x3ff],           bits : 19_10
    g_lut14:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA6_OFS 0x0518
REGDEF_BEGIN(SRAM_G_GAMMA6)
REGDEF_BIT(g_lut12,        10)
REGDEF_BIT(g_lut13,        10)
REGDEF_BIT(g_lut14,        10)
REGDEF_END(SRAM_G_GAMMA6)


/*
    g_lut14:    [0x0, 0x3ff],           bits : 9_0
    g_lut15:    [0x0, 0x3ff],           bits : 19_10
    g_lut16:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA7_OFS 0x051c
REGDEF_BEGIN(SRAM_G_GAMMA7)
REGDEF_BIT(g_lut14,        10)
REGDEF_BIT(g_lut15,        10)
REGDEF_BIT(g_lut16,        10)
REGDEF_END(SRAM_G_GAMMA7)


/*
    g_lut16:    [0x0, 0x3ff],           bits : 9_0
    g_lut17:    [0x0, 0x3ff],           bits : 19_10
    g_lut18:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA8_OFS 0x0520
REGDEF_BEGIN(SRAM_G_GAMMA8)
REGDEF_BIT(g_lut16,        10)
REGDEF_BIT(g_lut17,        10)
REGDEF_BIT(g_lut18,        10)
REGDEF_END(SRAM_G_GAMMA8)


/*
    g_lut18:    [0x0, 0x3ff],           bits : 9_0
    g_lut19:    [0x0, 0x3ff],           bits : 19_10
    g_lut20:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA9_OFS 0x0524
REGDEF_BEGIN(SRAM_G_GAMMA9)
REGDEF_BIT(g_lut18,        10)
REGDEF_BIT(g_lut19,        10)
REGDEF_BIT(g_lut20,        10)
REGDEF_END(SRAM_G_GAMMA9)


/*
    g_lut20:    [0x0, 0x3ff],           bits : 9_0
    g_lut21:    [0x0, 0x3ff],           bits : 19_10
    g_lut22:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA10_OFS 0x0528
REGDEF_BEGIN(SRAM_G_GAMMA10)
REGDEF_BIT(g_lut20,        10)
REGDEF_BIT(g_lut21,        10)
REGDEF_BIT(g_lut22,        10)
REGDEF_END(SRAM_G_GAMMA10)


/*
    g_lut22:    [0x0, 0x3ff],           bits : 9_0
    g_lut23:    [0x0, 0x3ff],           bits : 19_10
    g_lut24:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA11_OFS 0x052c
REGDEF_BEGIN(SRAM_G_GAMMA11)
REGDEF_BIT(g_lut22,        10)
REGDEF_BIT(g_lut23,        10)
REGDEF_BIT(g_lut24,        10)
REGDEF_END(SRAM_G_GAMMA11)


/*
    g_lut24:    [0x0, 0x3ff],           bits : 9_0
    g_lut25:    [0x0, 0x3ff],           bits : 19_10
    g_lut26:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA12_OFS 0x0530
REGDEF_BEGIN(SRAM_G_GAMMA12)
REGDEF_BIT(g_lut24,        10)
REGDEF_BIT(g_lut25,        10)
REGDEF_BIT(g_lut26,        10)
REGDEF_END(SRAM_G_GAMMA12)


/*
    g_lut26:    [0x0, 0x3ff],           bits : 9_0
    g_lut27:    [0x0, 0x3ff],           bits : 19_10
    g_lut28:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA13_OFS 0x0534
REGDEF_BEGIN(SRAM_G_GAMMA13)
REGDEF_BIT(g_lut26,        10)
REGDEF_BIT(g_lut27,        10)
REGDEF_BIT(g_lut28,        10)
REGDEF_END(SRAM_G_GAMMA13)


/*
    g_lut28:    [0x0, 0x3ff],           bits : 9_0
    g_lut29:    [0x0, 0x3ff],           bits : 19_10
    g_lut30:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA14_OFS 0x0538
REGDEF_BEGIN(SRAM_G_GAMMA14)
REGDEF_BIT(g_lut28,        10)
REGDEF_BIT(g_lut29,        10)
REGDEF_BIT(g_lut30,        10)
REGDEF_END(SRAM_G_GAMMA14)


/*
    g_lut30:    [0x0, 0x3ff],           bits : 9_0
    g_lut31:    [0x0, 0x3ff],           bits : 19_10
    g_lut32:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA15_OFS 0x053c
REGDEF_BEGIN(SRAM_G_GAMMA15)
REGDEF_BIT(g_lut30,        10)
REGDEF_BIT(g_lut31,        10)
REGDEF_BIT(g_lut32,        10)
REGDEF_END(SRAM_G_GAMMA15)


/*
    g_lut32:    [0x0, 0x3ff],           bits : 9_0
    g_lut33:    [0x0, 0x3ff],           bits : 19_10
    g_lut34:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA16_OFS 0x0540
REGDEF_BEGIN(SRAM_G_GAMMA16)
REGDEF_BIT(g_lut32,        10)
REGDEF_BIT(g_lut33,        10)
REGDEF_BIT(g_lut34,        10)
REGDEF_END(SRAM_G_GAMMA16)


/*
    g_lut34:    [0x0, 0x3ff],           bits : 9_0
    g_lut35:    [0x0, 0x3ff],           bits : 19_10
    g_lut36:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA17_OFS 0x0544
REGDEF_BEGIN(SRAM_G_GAMMA17)
REGDEF_BIT(g_lut34,        10)
REGDEF_BIT(g_lut35,        10)
REGDEF_BIT(g_lut36,        10)
REGDEF_END(SRAM_G_GAMMA17)


/*
    g_lut36:    [0x0, 0x3ff],           bits : 9_0
    g_lut37:    [0x0, 0x3ff],           bits : 19_10
    g_lut38:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA18_OFS 0x0548
REGDEF_BEGIN(SRAM_G_GAMMA18)
REGDEF_BIT(g_lut36,        10)
REGDEF_BIT(g_lut37,        10)
REGDEF_BIT(g_lut38,        10)
REGDEF_END(SRAM_G_GAMMA18)


/*
    g_lut38:    [0x0, 0x3ff],           bits : 9_0
    g_lut39:    [0x0, 0x3ff],           bits : 19_10
    g_lut40:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA19_OFS 0x054c
REGDEF_BEGIN(SRAM_G_GAMMA19)
REGDEF_BIT(g_lut38,        10)
REGDEF_BIT(g_lut39,        10)
REGDEF_BIT(g_lut40,        10)
REGDEF_END(SRAM_G_GAMMA19)


/*
    g_lut40:    [0x0, 0x3ff],           bits : 9_0
    g_lut41:    [0x0, 0x3ff],           bits : 19_10
    g_lut42:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA20_OFS 0x0550
REGDEF_BEGIN(SRAM_G_GAMMA20)
REGDEF_BIT(g_lut40,        10)
REGDEF_BIT(g_lut41,        10)
REGDEF_BIT(g_lut42,        10)
REGDEF_END(SRAM_G_GAMMA20)


/*
    g_lut42:    [0x0, 0x3ff],           bits : 9_0
    g_lut43:    [0x0, 0x3ff],           bits : 19_10
    g_lut44:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA21_OFS 0x0554
REGDEF_BEGIN(SRAM_G_GAMMA21)
REGDEF_BIT(g_lut42,        10)
REGDEF_BIT(g_lut43,        10)
REGDEF_BIT(g_lut44,        10)
REGDEF_END(SRAM_G_GAMMA21)


/*
    g_lut44:    [0x0, 0x3ff],           bits : 9_0
    g_lut45:    [0x0, 0x3ff],           bits : 19_10
    g_lut46:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA22_OFS 0x0558
REGDEF_BEGIN(SRAM_G_GAMMA22)
REGDEF_BIT(g_lut44,        10)
REGDEF_BIT(g_lut45,        10)
REGDEF_BIT(g_lut46,        10)
REGDEF_END(SRAM_G_GAMMA22)


/*
    g_lut46:    [0x0, 0x3ff],           bits : 9_0
    g_lut47:    [0x0, 0x3ff],           bits : 19_10
    g_lut48:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA23_OFS 0x055c
REGDEF_BEGIN(SRAM_G_GAMMA23)
REGDEF_BIT(g_lut46,        10)
REGDEF_BIT(g_lut47,        10)
REGDEF_BIT(g_lut48,        10)
REGDEF_END(SRAM_G_GAMMA23)


/*
    g_lut48:    [0x0, 0x3ff],           bits : 9_0
    g_lut49:    [0x0, 0x3ff],           bits : 19_10
    g_lut50:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA24_OFS 0x0560
REGDEF_BEGIN(SRAM_G_GAMMA24)
REGDEF_BIT(g_lut48,        10)
REGDEF_BIT(g_lut49,        10)
REGDEF_BIT(g_lut50,        10)
REGDEF_END(SRAM_G_GAMMA24)


/*
    g_lut50:    [0x0, 0x3ff],           bits : 9_0
    g_lut51:    [0x0, 0x3ff],           bits : 19_10
    g_lut52:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA25_OFS 0x0564
REGDEF_BEGIN(SRAM_G_GAMMA25)
REGDEF_BIT(g_lut50,        10)
REGDEF_BIT(g_lut51,        10)
REGDEF_BIT(g_lut52,        10)
REGDEF_END(SRAM_G_GAMMA25)


/*
    g_lut52:    [0x0, 0x3ff],           bits : 9_0
    g_lut53:    [0x0, 0x3ff],           bits : 19_10
    g_lut54:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA26_OFS 0x0568
REGDEF_BEGIN(SRAM_G_GAMMA26)
REGDEF_BIT(g_lut52,        10)
REGDEF_BIT(g_lut53,        10)
REGDEF_BIT(g_lut54,        10)
REGDEF_END(SRAM_G_GAMMA26)


/*
    g_lut54:    [0x0, 0x3ff],           bits : 9_0
    g_lut55:    [0x0, 0x3ff],           bits : 19_10
    g_lut56:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA27_OFS 0x056c
REGDEF_BEGIN(SRAM_G_GAMMA27)
REGDEF_BIT(g_lut54,        10)
REGDEF_BIT(g_lut55,        10)
REGDEF_BIT(g_lut56,        10)
REGDEF_END(SRAM_G_GAMMA27)


/*
    g_lut56:    [0x0, 0x3ff],           bits : 9_0
    g_lut57:    [0x0, 0x3ff],           bits : 19_10
    g_lut58:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA28_OFS 0x0570
REGDEF_BEGIN(SRAM_G_GAMMA28)
REGDEF_BIT(g_lut56,        10)
REGDEF_BIT(g_lut57,        10)
REGDEF_BIT(g_lut58,        10)
REGDEF_END(SRAM_G_GAMMA28)


/*
    g_lut58:    [0x0, 0x3ff],           bits : 9_0
    g_lut59:    [0x0, 0x3ff],           bits : 19_10
    g_lut60:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA29_OFS 0x0574
REGDEF_BEGIN(SRAM_G_GAMMA29)
REGDEF_BIT(g_lut58,        10)
REGDEF_BIT(g_lut59,        10)
REGDEF_BIT(g_lut60,        10)
REGDEF_END(SRAM_G_GAMMA29)


/*
    g_lut60:    [0x0, 0x3ff],           bits : 9_0
    g_lut61:    [0x0, 0x3ff],           bits : 19_10
    g_lut62:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA30_OFS 0x0578
REGDEF_BEGIN(SRAM_G_GAMMA30)
REGDEF_BIT(g_lut60,        10)
REGDEF_BIT(g_lut61,        10)
REGDEF_BIT(g_lut62,        10)
REGDEF_END(SRAM_G_GAMMA30)


/*
    g_lut62:    [0x0, 0x3ff],           bits : 9_0
    g_lut63:    [0x0, 0x3ff],           bits : 19_10
    g_lut64:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA31_OFS 0x057c
REGDEF_BEGIN(SRAM_G_GAMMA31)
REGDEF_BIT(g_lut62,        10)
REGDEF_BIT(g_lut63,        10)
REGDEF_BIT(g_lut64,        10)
REGDEF_END(SRAM_G_GAMMA31)


/*
    g_lut64:    [0x0, 0x3ff],           bits : 9_0
    g_lut65:    [0x0, 0x3ff],           bits : 19_10
    g_lut66:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA32_OFS 0x0580
REGDEF_BEGIN(SRAM_G_GAMMA32)
REGDEF_BIT(g_lut64,        10)
REGDEF_BIT(g_lut65,        10)
REGDEF_BIT(g_lut66,        10)
REGDEF_END(SRAM_G_GAMMA32)


/*
    g_lut66:    [0x0, 0x3ff],           bits : 9_0
    g_lut67:    [0x0, 0x3ff],           bits : 19_10
    g_lut68:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA33_OFS 0x0584
REGDEF_BEGIN(SRAM_G_GAMMA33)
REGDEF_BIT(g_lut66,        10)
REGDEF_BIT(g_lut67,        10)
REGDEF_BIT(g_lut68,        10)
REGDEF_END(SRAM_G_GAMMA33)


/*
    g_lut68:    [0x0, 0x3ff],           bits : 9_0
    g_lut69:    [0x0, 0x3ff],           bits : 19_10
    g_lut70:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA34_OFS 0x0588
REGDEF_BEGIN(SRAM_G_GAMMA34)
REGDEF_BIT(g_lut68,        10)
REGDEF_BIT(g_lut69,        10)
REGDEF_BIT(g_lut70,        10)
REGDEF_END(SRAM_G_GAMMA34)


/*
    g_lut70:    [0x0, 0x3ff],           bits : 9_0
    g_lut71:    [0x0, 0x3ff],           bits : 19_10
    g_lut72:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA35_OFS 0x058c
REGDEF_BEGIN(SRAM_G_GAMMA35)
REGDEF_BIT(g_lut70,        10)
REGDEF_BIT(g_lut71,        10)
REGDEF_BIT(g_lut72,        10)
REGDEF_END(SRAM_G_GAMMA35)


/*
    g_lut72:    [0x0, 0x3ff],           bits : 9_0
    g_lut73:    [0x0, 0x3ff],           bits : 19_10
    g_lut74:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA36_OFS 0x0590
REGDEF_BEGIN(SRAM_G_GAMMA36)
REGDEF_BIT(g_lut72,        10)
REGDEF_BIT(g_lut73,        10)
REGDEF_BIT(g_lut74,        10)
REGDEF_END(SRAM_G_GAMMA36)


/*
    g_lut74:    [0x0, 0x3ff],           bits : 9_0
    g_lut75:    [0x0, 0x3ff],           bits : 19_10
    g_lut76:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA37_OFS 0x0594
REGDEF_BEGIN(SRAM_G_GAMMA37)
REGDEF_BIT(g_lut74,        10)
REGDEF_BIT(g_lut75,        10)
REGDEF_BIT(g_lut76,        10)
REGDEF_END(SRAM_G_GAMMA37)


/*
    g_lut76:    [0x0, 0x3ff],           bits : 9_0
    g_lut77:    [0x0, 0x3ff],           bits : 19_10
    g_lut78:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA38_OFS 0x0598
REGDEF_BEGIN(SRAM_G_GAMMA38)
REGDEF_BIT(g_lut76,        10)
REGDEF_BIT(g_lut77,        10)
REGDEF_BIT(g_lut78,        10)
REGDEF_END(SRAM_G_GAMMA38)


/*
    g_lut78:    [0x0, 0x3ff],           bits : 9_0
    g_lut79:    [0x0, 0x3ff],           bits : 19_10
    g_lut80:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA39_OFS 0x059c
REGDEF_BEGIN(SRAM_G_GAMMA39)
REGDEF_BIT(g_lut78,        10)
REGDEF_BIT(g_lut79,        10)
REGDEF_BIT(g_lut80,        10)
REGDEF_END(SRAM_G_GAMMA39)


/*
    g_lut80:    [0x0, 0x3ff],           bits : 9_0
    g_lut81:    [0x0, 0x3ff],           bits : 19_10
    g_lut82:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA40_OFS 0x05a0
REGDEF_BEGIN(SRAM_G_GAMMA40)
REGDEF_BIT(g_lut80,        10)
REGDEF_BIT(g_lut81,        10)
REGDEF_BIT(g_lut82,        10)
REGDEF_END(SRAM_G_GAMMA40)


/*
    g_lut82:    [0x0, 0x3ff],           bits : 9_0
    g_lut83:    [0x0, 0x3ff],           bits : 19_10
    g_lut84:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA41_OFS 0x05a4
REGDEF_BEGIN(SRAM_G_GAMMA41)
REGDEF_BIT(g_lut82,        10)
REGDEF_BIT(g_lut83,        10)
REGDEF_BIT(g_lut84,        10)
REGDEF_END(SRAM_G_GAMMA41)


/*
    g_lut84:    [0x0, 0x3ff],           bits : 9_0
    g_lut85:    [0x0, 0x3ff],           bits : 19_10
    g_lut86:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA42_OFS 0x05a8
REGDEF_BEGIN(SRAM_G_GAMMA42)
REGDEF_BIT(g_lut84,        10)
REGDEF_BIT(g_lut85,        10)
REGDEF_BIT(g_lut86,        10)
REGDEF_END(SRAM_G_GAMMA42)


/*
    g_lut86:    [0x0, 0x3ff],           bits : 9_0
    g_lut87:    [0x0, 0x3ff],           bits : 19_10
    g_lut88:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA43_OFS 0x05ac
REGDEF_BEGIN(SRAM_G_GAMMA43)
REGDEF_BIT(g_lut86,        10)
REGDEF_BIT(g_lut87,        10)
REGDEF_BIT(g_lut88,        10)
REGDEF_END(SRAM_G_GAMMA43)


/*
    g_lut88:    [0x0, 0x3ff],           bits : 9_0
    g_lut89:    [0x0, 0x3ff],           bits : 19_10
    g_lut90:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA44_OFS 0x05b0
REGDEF_BEGIN(SRAM_G_GAMMA44)
REGDEF_BIT(g_lut88,        10)
REGDEF_BIT(g_lut89,        10)
REGDEF_BIT(g_lut90,        10)
REGDEF_END(SRAM_G_GAMMA44)


/*
    g_lut90:    [0x0, 0x3ff],           bits : 9_0
    g_lut91:    [0x0, 0x3ff],           bits : 19_10
    g_lut92:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA45_OFS 0x05b4
REGDEF_BEGIN(SRAM_G_GAMMA45)
REGDEF_BIT(g_lut90,        10)
REGDEF_BIT(g_lut91,        10)
REGDEF_BIT(g_lut92,        10)
REGDEF_END(SRAM_G_GAMMA45)


/*
    g_lut92:    [0x0, 0x3ff],           bits : 9_0
    g_lut93:    [0x0, 0x3ff],           bits : 19_10
    g_lut94:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA46_OFS 0x05b8
REGDEF_BEGIN(SRAM_G_GAMMA46)
REGDEF_BIT(g_lut92,        10)
REGDEF_BIT(g_lut93,        10)
REGDEF_BIT(g_lut94,        10)
REGDEF_END(SRAM_G_GAMMA46)


/*
    g_lut94:    [0x0, 0x3ff],           bits : 9_0
    g_lut95:    [0x0, 0x3ff],           bits : 19_10
    g_lut96:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA47_OFS 0x05bc
REGDEF_BEGIN(SRAM_G_GAMMA47)
REGDEF_BIT(g_lut94,        10)
REGDEF_BIT(g_lut95,        10)
REGDEF_BIT(g_lut96,        10)
REGDEF_END(SRAM_G_GAMMA47)


/*
    g_lut96:    [0x0, 0x3ff],           bits : 9_0
    g_lut97:    [0x0, 0x3ff],           bits : 19_10
    g_lut98:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA48_OFS 0x05c0
REGDEF_BEGIN(SRAM_G_GAMMA48)
REGDEF_BIT(g_lut96,        10)
REGDEF_BIT(g_lut97,        10)
REGDEF_BIT(g_lut98,        10)
REGDEF_END(SRAM_G_GAMMA48)


/*
    g_lut98 :    [0x0, 0x3ff],          bits : 9_0
    g_lut99 :    [0x0, 0x3ff],          bits : 19_10
    g_lut100:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA49_OFS 0x05c4
REGDEF_BEGIN(SRAM_G_GAMMA49)
REGDEF_BIT(g_lut98,        10)
REGDEF_BIT(g_lut99,        10)
REGDEF_BIT(g_lut100,        10)
REGDEF_END(SRAM_G_GAMMA49)


/*
    g_lut100:    [0x0, 0x3ff],          bits : 9_0
    g_lut101:    [0x0, 0x3ff],          bits : 19_10
    g_lut102:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA50_OFS 0x05c8
REGDEF_BEGIN(SRAM_G_GAMMA50)
REGDEF_BIT(g_lut100,        10)
REGDEF_BIT(g_lut101,        10)
REGDEF_BIT(g_lut102,        10)
REGDEF_END(SRAM_G_GAMMA50)


/*
    g_lut102:    [0x0, 0x3ff],          bits : 9_0
    g_lut103:    [0x0, 0x3ff],          bits : 19_10
    g_lut104:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA51_OFS 0x05cc
REGDEF_BEGIN(SRAM_G_GAMMA51)
REGDEF_BIT(g_lut102,        10)
REGDEF_BIT(g_lut103,        10)
REGDEF_BIT(g_lut104,        10)
REGDEF_END(SRAM_G_GAMMA51)


/*
    g_lut104:    [0x0, 0x3ff],          bits : 9_0
    g_lut105:    [0x0, 0x3ff],          bits : 19_10
    g_lut106:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA52_OFS 0x05d0
REGDEF_BEGIN(SRAM_G_GAMMA52)
REGDEF_BIT(g_lut104,        10)
REGDEF_BIT(g_lut105,        10)
REGDEF_BIT(g_lut106,        10)
REGDEF_END(SRAM_G_GAMMA52)


/*
    g_lut106:    [0x0, 0x3ff],          bits : 9_0
    g_lut107:    [0x0, 0x3ff],          bits : 19_10
    g_lut108:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA53_OFS 0x05d4
REGDEF_BEGIN(SRAM_G_GAMMA53)
REGDEF_BIT(g_lut106,        10)
REGDEF_BIT(g_lut107,        10)
REGDEF_BIT(g_lut108,        10)
REGDEF_END(SRAM_G_GAMMA53)


/*
    g_lut108:    [0x0, 0x3ff],          bits : 9_0
    g_lut109:    [0x0, 0x3ff],          bits : 19_10
    g_lut110:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA54_OFS 0x05d8
REGDEF_BEGIN(SRAM_G_GAMMA54)
REGDEF_BIT(g_lut108,        10)
REGDEF_BIT(g_lut109,        10)
REGDEF_BIT(g_lut110,        10)
REGDEF_END(SRAM_G_GAMMA54)


/*
    g_lut110:    [0x0, 0x3ff],          bits : 9_0
    g_lut111:    [0x0, 0x3ff],          bits : 19_10
    g_lut112:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA55_OFS 0x05dc
REGDEF_BEGIN(SRAM_G_GAMMA55)
REGDEF_BIT(g_lut110,        10)
REGDEF_BIT(g_lut111,        10)
REGDEF_BIT(g_lut112,        10)
REGDEF_END(SRAM_G_GAMMA55)


/*
    g_lut112:    [0x0, 0x3ff],          bits : 9_0
    g_lut113:    [0x0, 0x3ff],          bits : 19_10
    g_lut114:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA56_OFS 0x05e0
REGDEF_BEGIN(SRAM_G_GAMMA56)
REGDEF_BIT(g_lut112,        10)
REGDEF_BIT(g_lut113,        10)
REGDEF_BIT(g_lut114,        10)
REGDEF_END(SRAM_G_GAMMA56)


/*
    g_lut114:    [0x0, 0x3ff],          bits : 9_0
    g_lut115:    [0x0, 0x3ff],          bits : 19_10
    g_lut116:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA57_OFS 0x05e4
REGDEF_BEGIN(SRAM_G_GAMMA57)
REGDEF_BIT(g_lut114,        10)
REGDEF_BIT(g_lut115,        10)
REGDEF_BIT(g_lut116,        10)
REGDEF_END(SRAM_G_GAMMA57)


/*
    g_lut116:    [0x0, 0x3ff],          bits : 9_0
    g_lut117:    [0x0, 0x3ff],          bits : 19_10
    g_lut118:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA58_OFS 0x05e8
REGDEF_BEGIN(SRAM_G_GAMMA58)
REGDEF_BIT(g_lut116,        10)
REGDEF_BIT(g_lut117,        10)
REGDEF_BIT(g_lut118,        10)
REGDEF_END(SRAM_G_GAMMA58)


/*
    g_lut118:    [0x0, 0x3ff],          bits : 9_0
    g_lut119:    [0x0, 0x3ff],          bits : 19_10
    g_lut120:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA59_OFS 0x05ec
REGDEF_BEGIN(SRAM_G_GAMMA59)
REGDEF_BIT(g_lut118,        10)
REGDEF_BIT(g_lut119,        10)
REGDEF_BIT(g_lut120,        10)
REGDEF_END(SRAM_G_GAMMA59)


/*
    g_lut120:    [0x0, 0x3ff],          bits : 9_0
    g_lut121:    [0x0, 0x3ff],          bits : 19_10
    g_lut122:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA60_OFS 0x05f0
REGDEF_BEGIN(SRAM_G_GAMMA60)
REGDEF_BIT(g_lut120,        10)
REGDEF_BIT(g_lut121,        10)
REGDEF_BIT(g_lut122,        10)
REGDEF_END(SRAM_G_GAMMA60)


/*
    g_lut122:    [0x0, 0x3ff],          bits : 9_0
    g_lut123:    [0x0, 0x3ff],          bits : 19_10
    g_lut124:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA61_OFS 0x05f4
REGDEF_BEGIN(SRAM_G_GAMMA61)
REGDEF_BIT(g_lut122,        10)
REGDEF_BIT(g_lut123,        10)
REGDEF_BIT(g_lut124,        10)
REGDEF_END(SRAM_G_GAMMA61)


/*
    g_lut124:    [0x0, 0x3ff],          bits : 9_0
    g_lut125:    [0x0, 0x3ff],          bits : 19_10
    g_lut126:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA62_OFS 0x05f8
REGDEF_BEGIN(SRAM_G_GAMMA62)
REGDEF_BIT(g_lut124,        10)
REGDEF_BIT(g_lut125,        10)
REGDEF_BIT(g_lut126,        10)
REGDEF_END(SRAM_G_GAMMA62)


/*
    g_lut126:    [0x0, 0x3ff],          bits : 9_0
    g_lut127:    [0x0, 0x3ff],          bits : 19_10
    g_lut128:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA63_OFS 0x05fc
REGDEF_BEGIN(SRAM_G_GAMMA63)
REGDEF_BIT(g_lut126,        10)
REGDEF_BIT(g_lut127,        10)
REGDEF_BIT(g_lut128,        10)
REGDEF_END(SRAM_G_GAMMA63)


/*
    b_lut0:    [0x0, 0x3ff],            bits : 9_0
    b_lut1:    [0x0, 0x3ff],            bits : 19_10
    b_lut2:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_B_GAMMA0_OFS 0x0600
REGDEF_BEGIN(SRAM_B_GAMMA0)
REGDEF_BIT(b_lut0,        10)
REGDEF_BIT(b_lut1,        10)
REGDEF_BIT(b_lut2,        10)
REGDEF_END(SRAM_B_GAMMA0)


/*
    b_lut2:    [0x0, 0x3ff],            bits : 9_0
    b_lut3:    [0x0, 0x3ff],            bits : 19_10
    b_lut4:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_B_GAMMA1_OFS 0x0604
REGDEF_BEGIN(SRAM_B_GAMMA1)
REGDEF_BIT(b_lut2,        10)
REGDEF_BIT(b_lut3,        10)
REGDEF_BIT(b_lut4,        10)
REGDEF_END(SRAM_B_GAMMA1)


/*
    b_lut4:    [0x0, 0x3ff],            bits : 9_0
    b_lut5:    [0x0, 0x3ff],            bits : 19_10
    b_lut6:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_B_GAMMA2_OFS 0x0608
REGDEF_BEGIN(SRAM_B_GAMMA2)
REGDEF_BIT(b_lut4,        10)
REGDEF_BIT(b_lut5,        10)
REGDEF_BIT(b_lut6,        10)
REGDEF_END(SRAM_B_GAMMA2)


/*
    b_lut6:    [0x0, 0x3ff],            bits : 9_0
    b_lut7:    [0x0, 0x3ff],            bits : 19_10
    b_lut8:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_B_GAMMA3_OFS 0x060c
REGDEF_BEGIN(SRAM_B_GAMMA3)
REGDEF_BIT(b_lut6,        10)
REGDEF_BIT(b_lut7,        10)
REGDEF_BIT(b_lut8,        10)
REGDEF_END(SRAM_B_GAMMA3)


/*
    b_lut8 :    [0x0, 0x3ff],           bits : 9_0
    b_lut9 :    [0x0, 0x3ff],           bits : 19_10
    b_lut10:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA4_OFS 0x0610
REGDEF_BEGIN(SRAM_B_GAMMA4)
REGDEF_BIT(b_lut8,        10)
REGDEF_BIT(b_lut9,        10)
REGDEF_BIT(b_lut10,        10)
REGDEF_END(SRAM_B_GAMMA4)


/*
    b_lut10:    [0x0, 0x3ff],           bits : 9_0
    b_lut11:    [0x0, 0x3ff],           bits : 19_10
    b_lut12:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA5_OFS 0x0614
REGDEF_BEGIN(SRAM_B_GAMMA5)
REGDEF_BIT(b_lut10,        10)
REGDEF_BIT(b_lut11,        10)
REGDEF_BIT(b_lut12,        10)
REGDEF_END(SRAM_B_GAMMA5)


/*
    b_lut12:    [0x0, 0x3ff],           bits : 9_0
    b_lut13:    [0x0, 0x3ff],           bits : 19_10
    b_lut14:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA6_OFS 0x0618
REGDEF_BEGIN(SRAM_B_GAMMA6)
REGDEF_BIT(b_lut12,        10)
REGDEF_BIT(b_lut13,        10)
REGDEF_BIT(b_lut14,        10)
REGDEF_END(SRAM_B_GAMMA6)


/*
    b_lut14:    [0x0, 0x3ff],           bits : 9_0
    b_lut15:    [0x0, 0x3ff],           bits : 19_10
    b_lut16:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA7_OFS 0x061c
REGDEF_BEGIN(SRAM_B_GAMMA7)
REGDEF_BIT(b_lut14,        10)
REGDEF_BIT(b_lut15,        10)
REGDEF_BIT(b_lut16,        10)
REGDEF_END(SRAM_B_GAMMA7)


/*
    b_lut16:    [0x0, 0x3ff],           bits : 9_0
    b_lut17:    [0x0, 0x3ff],           bits : 19_10
    b_lut18:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA8_OFS 0x0620
REGDEF_BEGIN(SRAM_B_GAMMA8)
REGDEF_BIT(b_lut16,        10)
REGDEF_BIT(b_lut17,        10)
REGDEF_BIT(b_lut18,        10)
REGDEF_END(SRAM_B_GAMMA8)


/*
    b_lut18:    [0x0, 0x3ff],           bits : 9_0
    b_lut19:    [0x0, 0x3ff],           bits : 19_10
    b_lut20:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA9_OFS 0x0624
REGDEF_BEGIN(SRAM_B_GAMMA9)
REGDEF_BIT(b_lut18,        10)
REGDEF_BIT(b_lut19,        10)
REGDEF_BIT(b_lut20,        10)
REGDEF_END(SRAM_B_GAMMA9)


/*
    b_lut20:    [0x0, 0x3ff],           bits : 9_0
    b_lut21:    [0x0, 0x3ff],           bits : 19_10
    b_lut22:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA10_OFS 0x0628
REGDEF_BEGIN(SRAM_B_GAMMA10)
REGDEF_BIT(b_lut20,        10)
REGDEF_BIT(b_lut21,        10)
REGDEF_BIT(b_lut22,        10)
REGDEF_END(SRAM_B_GAMMA10)


/*
    b_lut22:    [0x0, 0x3ff],           bits : 9_0
    b_lut23:    [0x0, 0x3ff],           bits : 19_10
    b_lut24:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA11_OFS 0x062c
REGDEF_BEGIN(SRAM_B_GAMMA11)
REGDEF_BIT(b_lut22,        10)
REGDEF_BIT(b_lut23,        10)
REGDEF_BIT(b_lut24,        10)
REGDEF_END(SRAM_B_GAMMA11)


/*
    b_lut24:    [0x0, 0x3ff],           bits : 9_0
    b_lut25:    [0x0, 0x3ff],           bits : 19_10
    b_lut26:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA12_OFS 0x0630
REGDEF_BEGIN(SRAM_B_GAMMA12)
REGDEF_BIT(b_lut24,        10)
REGDEF_BIT(b_lut25,        10)
REGDEF_BIT(b_lut26,        10)
REGDEF_END(SRAM_B_GAMMA12)


/*
    b_lut26:    [0x0, 0x3ff],           bits : 9_0
    b_lut27:    [0x0, 0x3ff],           bits : 19_10
    b_lut28:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA13_OFS 0x0634
REGDEF_BEGIN(SRAM_B_GAMMA13)
REGDEF_BIT(b_lut26,        10)
REGDEF_BIT(b_lut27,        10)
REGDEF_BIT(b_lut28,        10)
REGDEF_END(SRAM_B_GAMMA13)


/*
    b_lut28:    [0x0, 0x3ff],           bits : 9_0
    b_lut29:    [0x0, 0x3ff],           bits : 19_10
    b_lut30:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA14_OFS 0x0638
REGDEF_BEGIN(SRAM_B_GAMMA14)
REGDEF_BIT(b_lut28,        10)
REGDEF_BIT(b_lut29,        10)
REGDEF_BIT(b_lut30,        10)
REGDEF_END(SRAM_B_GAMMA14)


/*
    b_lut30:    [0x0, 0x3ff],           bits : 9_0
    b_lut31:    [0x0, 0x3ff],           bits : 19_10
    b_lut32:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA15_OFS 0x063c
REGDEF_BEGIN(SRAM_B_GAMMA15)
REGDEF_BIT(b_lut30,        10)
REGDEF_BIT(b_lut31,        10)
REGDEF_BIT(b_lut32,        10)
REGDEF_END(SRAM_B_GAMMA15)


/*
    b_lut32:    [0x0, 0x3ff],           bits : 9_0
    b_lut33:    [0x0, 0x3ff],           bits : 19_10
    b_lut34:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA16_OFS 0x0640
REGDEF_BEGIN(SRAM_B_GAMMA16)
REGDEF_BIT(b_lut32,        10)
REGDEF_BIT(b_lut33,        10)
REGDEF_BIT(b_lut34,        10)
REGDEF_END(SRAM_B_GAMMA16)


/*
    b_lut34:    [0x0, 0x3ff],           bits : 9_0
    b_lut35:    [0x0, 0x3ff],           bits : 19_10
    b_lut36:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA17_OFS 0x0644
REGDEF_BEGIN(SRAM_B_GAMMA17)
REGDEF_BIT(b_lut34,        10)
REGDEF_BIT(b_lut35,        10)
REGDEF_BIT(b_lut36,        10)
REGDEF_END(SRAM_B_GAMMA17)


/*
    b_lut36:    [0x0, 0x3ff],           bits : 9_0
    b_lut37:    [0x0, 0x3ff],           bits : 19_10
    b_lut38:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA18_OFS 0x0648
REGDEF_BEGIN(SRAM_B_GAMMA18)
REGDEF_BIT(b_lut36,        10)
REGDEF_BIT(b_lut37,        10)
REGDEF_BIT(b_lut38,        10)
REGDEF_END(SRAM_B_GAMMA18)


/*
    b_lut38:    [0x0, 0x3ff],           bits : 9_0
    b_lut39:    [0x0, 0x3ff],           bits : 19_10
    b_lut40:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA19_OFS 0x064c
REGDEF_BEGIN(SRAM_B_GAMMA19)
REGDEF_BIT(b_lut38,        10)
REGDEF_BIT(b_lut39,        10)
REGDEF_BIT(b_lut40,        10)
REGDEF_END(SRAM_B_GAMMA19)


/*
    b_lut40:    [0x0, 0x3ff],           bits : 9_0
    b_lut41:    [0x0, 0x3ff],           bits : 19_10
    b_lut42:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA20_OFS 0x0650
REGDEF_BEGIN(SRAM_B_GAMMA20)
REGDEF_BIT(b_lut40,        10)
REGDEF_BIT(b_lut41,        10)
REGDEF_BIT(b_lut42,        10)
REGDEF_END(SRAM_B_GAMMA20)


/*
    b_lut42:    [0x0, 0x3ff],           bits : 9_0
    b_lut43:    [0x0, 0x3ff],           bits : 19_10
    b_lut44:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA21_OFS 0x0654
REGDEF_BEGIN(SRAM_B_GAMMA21)
REGDEF_BIT(b_lut42,        10)
REGDEF_BIT(b_lut43,        10)
REGDEF_BIT(b_lut44,        10)
REGDEF_END(SRAM_B_GAMMA21)


/*
    b_lut44:    [0x0, 0x3ff],           bits : 9_0
    b_lut45:    [0x0, 0x3ff],           bits : 19_10
    b_lut46:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA22_OFS 0x0658
REGDEF_BEGIN(SRAM_B_GAMMA22)
REGDEF_BIT(b_lut44,        10)
REGDEF_BIT(b_lut45,        10)
REGDEF_BIT(b_lut46,        10)
REGDEF_END(SRAM_B_GAMMA22)


/*
    b_lut46:    [0x0, 0x3ff],           bits : 9_0
    b_lut47:    [0x0, 0x3ff],           bits : 19_10
    b_lut48:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA23_OFS 0x065c
REGDEF_BEGIN(SRAM_B_GAMMA23)
REGDEF_BIT(b_lut46,        10)
REGDEF_BIT(b_lut47,        10)
REGDEF_BIT(b_lut48,        10)
REGDEF_END(SRAM_B_GAMMA23)


/*
    b_lut48:    [0x0, 0x3ff],           bits : 9_0
    b_lut49:    [0x0, 0x3ff],           bits : 19_10
    b_lut50:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA24_OFS 0x0660
REGDEF_BEGIN(SRAM_B_GAMMA24)
REGDEF_BIT(b_lut48,        10)
REGDEF_BIT(b_lut49,        10)
REGDEF_BIT(b_lut50,        10)
REGDEF_END(SRAM_B_GAMMA24)


/*
    b_lut50:    [0x0, 0x3ff],           bits : 9_0
    b_lut51:    [0x0, 0x3ff],           bits : 19_10
    b_lut52:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA25_OFS 0x0664
REGDEF_BEGIN(SRAM_B_GAMMA25)
REGDEF_BIT(b_lut50,        10)
REGDEF_BIT(b_lut51,        10)
REGDEF_BIT(b_lut52,        10)
REGDEF_END(SRAM_B_GAMMA25)


/*
    b_lut52:    [0x0, 0x3ff],           bits : 9_0
    b_lut53:    [0x0, 0x3ff],           bits : 19_10
    b_lut54:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA26_OFS 0x0668
REGDEF_BEGIN(SRAM_B_GAMMA26)
REGDEF_BIT(b_lut52,        10)
REGDEF_BIT(b_lut53,        10)
REGDEF_BIT(b_lut54,        10)
REGDEF_END(SRAM_B_GAMMA26)


/*
    b_lut54:    [0x0, 0x3ff],           bits : 9_0
    b_lut55:    [0x0, 0x3ff],           bits : 19_10
    b_lut56:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA27_OFS 0x066c
REGDEF_BEGIN(SRAM_B_GAMMA27)
REGDEF_BIT(b_lut54,        10)
REGDEF_BIT(b_lut55,        10)
REGDEF_BIT(b_lut56,        10)
REGDEF_END(SRAM_B_GAMMA27)


/*
    b_lut56:    [0x0, 0x3ff],           bits : 9_0
    b_lut57:    [0x0, 0x3ff],           bits : 19_10
    b_lut58:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA28_OFS 0x0670
REGDEF_BEGIN(SRAM_B_GAMMA28)
REGDEF_BIT(b_lut56,        10)
REGDEF_BIT(b_lut57,        10)
REGDEF_BIT(b_lut58,        10)
REGDEF_END(SRAM_B_GAMMA28)


/*
    b_lut58:    [0x0, 0x3ff],           bits : 9_0
    b_lut59:    [0x0, 0x3ff],           bits : 19_10
    b_lut60:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA29_OFS 0x0674
REGDEF_BEGIN(SRAM_B_GAMMA29)
REGDEF_BIT(b_lut58,        10)
REGDEF_BIT(b_lut59,        10)
REGDEF_BIT(b_lut60,        10)
REGDEF_END(SRAM_B_GAMMA29)


/*
    b_lut60:    [0x0, 0x3ff],           bits : 9_0
    b_lut61:    [0x0, 0x3ff],           bits : 19_10
    b_lut62:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA30_OFS 0x0678
REGDEF_BEGIN(SRAM_B_GAMMA30)
REGDEF_BIT(b_lut60,        10)
REGDEF_BIT(b_lut61,        10)
REGDEF_BIT(b_lut62,        10)
REGDEF_END(SRAM_B_GAMMA30)


/*
    b_lut62:    [0x0, 0x3ff],           bits : 9_0
    b_lut63:    [0x0, 0x3ff],           bits : 19_10
    b_lut64:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA31_OFS 0x067c
REGDEF_BEGIN(SRAM_B_GAMMA31)
REGDEF_BIT(b_lut62,        10)
REGDEF_BIT(b_lut63,        10)
REGDEF_BIT(b_lut64,        10)
REGDEF_END(SRAM_B_GAMMA31)


/*
    b_lut64:    [0x0, 0x3ff],           bits : 9_0
    b_lut65:    [0x0, 0x3ff],           bits : 19_10
    b_lut66:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA32_OFS 0x0680
REGDEF_BEGIN(SRAM_B_GAMMA32)
REGDEF_BIT(b_lut64,        10)
REGDEF_BIT(b_lut65,        10)
REGDEF_BIT(b_lut66,        10)
REGDEF_END(SRAM_B_GAMMA32)


/*
    b_lut66:    [0x0, 0x3ff],           bits : 9_0
    b_lut67:    [0x0, 0x3ff],           bits : 19_10
    b_lut68:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA33_OFS 0x0684
REGDEF_BEGIN(SRAM_B_GAMMA33)
REGDEF_BIT(b_lut66,        10)
REGDEF_BIT(b_lut67,        10)
REGDEF_BIT(b_lut68,        10)
REGDEF_END(SRAM_B_GAMMA33)


/*
    b_lut68:    [0x0, 0x3ff],           bits : 9_0
    b_lut69:    [0x0, 0x3ff],           bits : 19_10
    b_lut70:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA34_OFS 0x0688
REGDEF_BEGIN(SRAM_B_GAMMA34)
REGDEF_BIT(b_lut68,        10)
REGDEF_BIT(b_lut69,        10)
REGDEF_BIT(b_lut70,        10)
REGDEF_END(SRAM_B_GAMMA34)


/*
    b_lut70:    [0x0, 0x3ff],           bits : 9_0
    b_lut71:    [0x0, 0x3ff],           bits : 19_10
    b_lut72:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA35_OFS 0x068c
REGDEF_BEGIN(SRAM_B_GAMMA35)
REGDEF_BIT(b_lut70,        10)
REGDEF_BIT(b_lut71,        10)
REGDEF_BIT(b_lut72,        10)
REGDEF_END(SRAM_B_GAMMA35)


/*
    b_lut72:    [0x0, 0x3ff],           bits : 9_0
    b_lut73:    [0x0, 0x3ff],           bits : 19_10
    b_lut74:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA36_OFS 0x0690
REGDEF_BEGIN(SRAM_B_GAMMA36)
REGDEF_BIT(b_lut72,        10)
REGDEF_BIT(b_lut73,        10)
REGDEF_BIT(b_lut74,        10)
REGDEF_END(SRAM_B_GAMMA36)


/*
    b_lut74:    [0x0, 0x3ff],           bits : 9_0
    b_lut75:    [0x0, 0x3ff],           bits : 19_10
    b_lut76:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA37_OFS 0x0694
REGDEF_BEGIN(SRAM_B_GAMMA37)
REGDEF_BIT(b_lut74,        10)
REGDEF_BIT(b_lut75,        10)
REGDEF_BIT(b_lut76,        10)
REGDEF_END(SRAM_B_GAMMA37)


/*
    b_lut76:    [0x0, 0x3ff],           bits : 9_0
    b_lut77:    [0x0, 0x3ff],           bits : 19_10
    b_lut78:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA38_OFS 0x0698
REGDEF_BEGIN(SRAM_B_GAMMA38)
REGDEF_BIT(b_lut76,        10)
REGDEF_BIT(b_lut77,        10)
REGDEF_BIT(b_lut78,        10)
REGDEF_END(SRAM_B_GAMMA38)


/*
    b_lut78:    [0x0, 0x3ff],           bits : 9_0
    b_lut79:    [0x0, 0x3ff],           bits : 19_10
    b_lut80:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA39_OFS 0x069c
REGDEF_BEGIN(SRAM_B_GAMMA39)
REGDEF_BIT(b_lut78,        10)
REGDEF_BIT(b_lut79,        10)
REGDEF_BIT(b_lut80,        10)
REGDEF_END(SRAM_B_GAMMA39)


/*
    b_lut80:    [0x0, 0x3ff],           bits : 9_0
    b_lut81:    [0x0, 0x3ff],           bits : 19_10
    b_lut82:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA40_OFS 0x06a0
REGDEF_BEGIN(SRAM_B_GAMMA40)
REGDEF_BIT(b_lut80,        10)
REGDEF_BIT(b_lut81,        10)
REGDEF_BIT(b_lut82,        10)
REGDEF_END(SRAM_B_GAMMA40)


/*
    b_lut82:    [0x0, 0x3ff],           bits : 9_0
    b_lut83:    [0x0, 0x3ff],           bits : 19_10
    b_lut84:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA41_OFS 0x06a4
REGDEF_BEGIN(SRAM_B_GAMMA41)
REGDEF_BIT(b_lut82,        10)
REGDEF_BIT(b_lut83,        10)
REGDEF_BIT(b_lut84,        10)
REGDEF_END(SRAM_B_GAMMA41)


/*
    b_lut84:    [0x0, 0x3ff],           bits : 9_0
    b_lut85:    [0x0, 0x3ff],           bits : 19_10
    b_lut86:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA42_OFS 0x06a8
REGDEF_BEGIN(SRAM_B_GAMMA42)
REGDEF_BIT(b_lut84,        10)
REGDEF_BIT(b_lut85,        10)
REGDEF_BIT(b_lut86,        10)
REGDEF_END(SRAM_B_GAMMA42)


/*
    b_lut86:    [0x0, 0x3ff],           bits : 9_0
    b_lut87:    [0x0, 0x3ff],           bits : 19_10
    b_lut88:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA43_OFS 0x06ac
REGDEF_BEGIN(SRAM_B_GAMMA43)
REGDEF_BIT(b_lut86,        10)
REGDEF_BIT(b_lut87,        10)
REGDEF_BIT(b_lut88,        10)
REGDEF_END(SRAM_B_GAMMA43)


/*
    b_lut88:    [0x0, 0x3ff],           bits : 9_0
    b_lut89:    [0x0, 0x3ff],           bits : 19_10
    b_lut90:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA44_OFS 0x06b0
REGDEF_BEGIN(SRAM_B_GAMMA44)
REGDEF_BIT(b_lut88,        10)
REGDEF_BIT(b_lut89,        10)
REGDEF_BIT(b_lut90,        10)
REGDEF_END(SRAM_B_GAMMA44)


/*
    b_lut90:    [0x0, 0x3ff],           bits : 9_0
    b_lut91:    [0x0, 0x3ff],           bits : 19_10
    b_lut92:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA45_OFS 0x06b4
REGDEF_BEGIN(SRAM_B_GAMMA45)
REGDEF_BIT(b_lut90,        10)
REGDEF_BIT(b_lut91,        10)
REGDEF_BIT(b_lut92,        10)
REGDEF_END(SRAM_B_GAMMA45)


/*
    b_lut92:    [0x0, 0x3ff],           bits : 9_0
    b_lut93:    [0x0, 0x3ff],           bits : 19_10
    b_lut94:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA46_OFS 0x06b8
REGDEF_BEGIN(SRAM_B_GAMMA46)
REGDEF_BIT(b_lut92,        10)
REGDEF_BIT(b_lut93,        10)
REGDEF_BIT(b_lut94,        10)
REGDEF_END(SRAM_B_GAMMA46)


/*
    b_lut94:    [0x0, 0x3ff],           bits : 9_0
    b_lut95:    [0x0, 0x3ff],           bits : 19_10
    b_lut96:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA47_OFS 0x06bc
REGDEF_BEGIN(SRAM_B_GAMMA47)
REGDEF_BIT(b_lut94,        10)
REGDEF_BIT(b_lut95,        10)
REGDEF_BIT(b_lut96,        10)
REGDEF_END(SRAM_B_GAMMA47)


/*
    b_lut96:    [0x0, 0x3ff],           bits : 9_0
    b_lut97:    [0x0, 0x3ff],           bits : 19_10
    b_lut98:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA48_OFS 0x06c0
REGDEF_BEGIN(SRAM_B_GAMMA48)
REGDEF_BIT(b_lut96,        10)
REGDEF_BIT(b_lut97,        10)
REGDEF_BIT(b_lut98,        10)
REGDEF_END(SRAM_B_GAMMA48)


/*
    b_lut98 :    [0x0, 0x3ff],          bits : 9_0
    b_lut99 :    [0x0, 0x3ff],          bits : 19_10
    b_lut100:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA49_OFS 0x06c4
REGDEF_BEGIN(SRAM_B_GAMMA49)
REGDEF_BIT(b_lut98,        10)
REGDEF_BIT(b_lut99,        10)
REGDEF_BIT(b_lut100,        10)
REGDEF_END(SRAM_B_GAMMA49)


/*
    b_lut100:    [0x0, 0x3ff],          bits : 9_0
    b_lut101:    [0x0, 0x3ff],          bits : 19_10
    b_lut102:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA50_OFS 0x06c8
REGDEF_BEGIN(SRAM_B_GAMMA50)
REGDEF_BIT(b_lut100,        10)
REGDEF_BIT(b_lut101,        10)
REGDEF_BIT(b_lut102,        10)
REGDEF_END(SRAM_B_GAMMA50)


/*
    b_lut102:    [0x0, 0x3ff],          bits : 9_0
    b_lut103:    [0x0, 0x3ff],          bits : 19_10
    b_lut104:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA51_OFS 0x06cc
REGDEF_BEGIN(SRAM_B_GAMMA51)
REGDEF_BIT(b_lut102,        10)
REGDEF_BIT(b_lut103,        10)
REGDEF_BIT(b_lut104,        10)
REGDEF_END(SRAM_B_GAMMA51)


/*
    b_lut104:    [0x0, 0x3ff],          bits : 9_0
    b_lut105:    [0x0, 0x3ff],          bits : 19_10
    b_lut106:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA52_OFS 0x06d0
REGDEF_BEGIN(SRAM_B_GAMMA52)
REGDEF_BIT(b_lut104,        10)
REGDEF_BIT(b_lut105,        10)
REGDEF_BIT(b_lut106,        10)
REGDEF_END(SRAM_B_GAMMA52)


/*
    b_lut106:    [0x0, 0x3ff],          bits : 9_0
    b_lut107:    [0x0, 0x3ff],          bits : 19_10
    b_lut108:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA53_OFS 0x06d4
REGDEF_BEGIN(SRAM_B_GAMMA53)
REGDEF_BIT(b_lut106,        10)
REGDEF_BIT(b_lut107,        10)
REGDEF_BIT(b_lut108,        10)
REGDEF_END(SRAM_B_GAMMA53)


/*
    b_lut108:    [0x0, 0x3ff],          bits : 9_0
    b_lut109:    [0x0, 0x3ff],          bits : 19_10
    b_lut110:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA54_OFS 0x06d8
REGDEF_BEGIN(SRAM_B_GAMMA54)
REGDEF_BIT(b_lut108,        10)
REGDEF_BIT(b_lut109,        10)
REGDEF_BIT(b_lut110,        10)
REGDEF_END(SRAM_B_GAMMA54)


/*
    b_lut110:    [0x0, 0x3ff],          bits : 9_0
    b_lut111:    [0x0, 0x3ff],          bits : 19_10
    b_lut112:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA55_OFS 0x06dc
REGDEF_BEGIN(SRAM_B_GAMMA55)
REGDEF_BIT(b_lut110,        10)
REGDEF_BIT(b_lut111,        10)
REGDEF_BIT(b_lut112,        10)
REGDEF_END(SRAM_B_GAMMA55)


/*
    b_lut112:    [0x0, 0x3ff],          bits : 9_0
    b_lut113:    [0x0, 0x3ff],          bits : 19_10
    b_lut114:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA56_OFS 0x06e0
REGDEF_BEGIN(SRAM_B_GAMMA56)
REGDEF_BIT(b_lut112,        10)
REGDEF_BIT(b_lut113,        10)
REGDEF_BIT(b_lut114,        10)
REGDEF_END(SRAM_B_GAMMA56)


/*
    b_lut114:    [0x0, 0x3ff],          bits : 9_0
    b_lut115:    [0x0, 0x3ff],          bits : 19_10
    b_lut116:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA57_OFS 0x06e4
REGDEF_BEGIN(SRAM_B_GAMMA57)
REGDEF_BIT(b_lut114,        10)
REGDEF_BIT(b_lut115,        10)
REGDEF_BIT(b_lut116,        10)
REGDEF_END(SRAM_B_GAMMA57)


/*
    b_lut116:    [0x0, 0x3ff],          bits : 9_0
    b_lut117:    [0x0, 0x3ff],          bits : 19_10
    b_lut118:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA58_OFS 0x06e8
REGDEF_BEGIN(SRAM_B_GAMMA58)
REGDEF_BIT(b_lut116,        10)
REGDEF_BIT(b_lut117,        10)
REGDEF_BIT(b_lut118,        10)
REGDEF_END(SRAM_B_GAMMA58)


/*
    b_lut118:    [0x0, 0x3ff],          bits : 9_0
    b_lut119:    [0x0, 0x3ff],          bits : 19_10
    b_lut120:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA59_OFS 0x06ec
REGDEF_BEGIN(SRAM_B_GAMMA59)
REGDEF_BIT(b_lut118,        10)
REGDEF_BIT(b_lut119,        10)
REGDEF_BIT(b_lut120,        10)
REGDEF_END(SRAM_B_GAMMA59)


/*
    b_lut120:    [0x0, 0x3ff],          bits : 9_0
    b_lut121:    [0x0, 0x3ff],          bits : 19_10
    b_lut122:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA60_OFS 0x06f0
REGDEF_BEGIN(SRAM_B_GAMMA60)
REGDEF_BIT(b_lut120,        10)
REGDEF_BIT(b_lut121,        10)
REGDEF_BIT(b_lut122,        10)
REGDEF_END(SRAM_B_GAMMA60)


/*
    b_lut122:    [0x0, 0x3ff],          bits : 9_0
    b_lut123:    [0x0, 0x3ff],          bits : 19_10
    b_lut124:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA61_OFS 0x06f4
REGDEF_BEGIN(SRAM_B_GAMMA61)
REGDEF_BIT(b_lut122,        10)
REGDEF_BIT(b_lut123,        10)
REGDEF_BIT(b_lut124,        10)
REGDEF_END(SRAM_B_GAMMA61)


/*
    b_lut124:    [0x0, 0x3ff],          bits : 9_0
    b_lut125:    [0x0, 0x3ff],          bits : 19_10
    b_lut126:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA62_OFS 0x06f8
REGDEF_BEGIN(SRAM_B_GAMMA62)
REGDEF_BIT(b_lut124,        10)
REGDEF_BIT(b_lut125,        10)
REGDEF_BIT(b_lut126,        10)
REGDEF_END(SRAM_B_GAMMA62)


/*
    b_lut126:    [0x0, 0x3ff],          bits : 9_0
    b_lut127:    [0x0, 0x3ff],          bits : 19_10
    b_lut128:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA63_OFS 0x06fc
REGDEF_BEGIN(SRAM_B_GAMMA63)
REGDEF_BIT(b_lut126,        10)
REGDEF_BIT(b_lut127,        10)
REGDEF_BIT(b_lut128,        10)
REGDEF_END(SRAM_B_GAMMA63)


/*
    y_lut0:    [0x0, 0x3ff],            bits : 9_0
    y_lut1:    [0x0, 0x3ff],            bits : 19_10
    y_lut2:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_Y_CURVE0_OFS 0x0700
REGDEF_BEGIN(SRAM_Y_CURVE0)
REGDEF_BIT(y_lut0,        10)
REGDEF_BIT(y_lut1,        10)
REGDEF_BIT(y_lut2,        10)
REGDEF_END(SRAM_Y_CURVE0)


/*
    y_lut2:    [0x0, 0x3ff],            bits : 9_0
    y_lut3:    [0x0, 0x3ff],            bits : 19_10
    y_lut4:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_Y_CURVE1_OFS 0x0704
REGDEF_BEGIN(SRAM_Y_CURVE1)
REGDEF_BIT(y_lut2,        10)
REGDEF_BIT(y_lut3,        10)
REGDEF_BIT(y_lut4,        10)
REGDEF_END(SRAM_Y_CURVE1)


/*
    y_lut4:    [0x0, 0x3ff],            bits : 9_0
    y_lut5:    [0x0, 0x3ff],            bits : 19_10
    y_lut6:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_Y_CURVE2_OFS 0x0708
REGDEF_BEGIN(SRAM_Y_CURVE2)
REGDEF_BIT(y_lut4,        10)
REGDEF_BIT(y_lut5,        10)
REGDEF_BIT(y_lut6,        10)
REGDEF_END(SRAM_Y_CURVE2)


/*
    y_lut6:    [0x0, 0x3ff],            bits : 9_0
    y_lut7:    [0x0, 0x3ff],            bits : 19_10
    y_lut8:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_Y_CURVE3_OFS 0x070c
REGDEF_BEGIN(SRAM_Y_CURVE3)
REGDEF_BIT(y_lut6,        10)
REGDEF_BIT(y_lut7,        10)
REGDEF_BIT(y_lut8,        10)
REGDEF_END(SRAM_Y_CURVE3)


/*
    y_lut8 :    [0x0, 0x3ff],           bits : 9_0
    y_lut9 :    [0x0, 0x3ff],           bits : 19_10
    y_lut10:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE4_OFS 0x0710
REGDEF_BEGIN(SRAM_Y_CURVE4)
REGDEF_BIT(y_lut8,        10)
REGDEF_BIT(y_lut9,        10)
REGDEF_BIT(y_lut10,        10)
REGDEF_END(SRAM_Y_CURVE4)


/*
    y_lut10:    [0x0, 0x3ff],           bits : 9_0
    y_lut11:    [0x0, 0x3ff],           bits : 19_10
    y_lut12:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE5_OFS 0x0714
REGDEF_BEGIN(SRAM_Y_CURVE5)
REGDEF_BIT(y_lut10,        10)
REGDEF_BIT(y_lut11,        10)
REGDEF_BIT(y_lut12,        10)
REGDEF_END(SRAM_Y_CURVE5)


/*
    y_lut12:    [0x0, 0x3ff],           bits : 9_0
    y_lut13:    [0x0, 0x3ff],           bits : 19_10
    y_lut14:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE6_OFS 0x0718
REGDEF_BEGIN(SRAM_Y_CURVE6)
REGDEF_BIT(y_lut12,        10)
REGDEF_BIT(y_lut13,        10)
REGDEF_BIT(y_lut14,        10)
REGDEF_END(SRAM_Y_CURVE6)


/*
    y_lut14:    [0x0, 0x3ff],           bits : 9_0
    y_lut15:    [0x0, 0x3ff],           bits : 19_10
    y_lut16:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE7_OFS 0x071c
REGDEF_BEGIN(SRAM_Y_CURVE7)
REGDEF_BIT(y_lut14,        10)
REGDEF_BIT(y_lut15,        10)
REGDEF_BIT(y_lut16,        10)
REGDEF_END(SRAM_Y_CURVE7)


/*
    y_lut16:    [0x0, 0x3ff],           bits : 9_0
    y_lut17:    [0x0, 0x3ff],           bits : 19_10
    y_lut18:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE8_OFS 0x0720
REGDEF_BEGIN(SRAM_Y_CURVE8)
REGDEF_BIT(y_lut16,        10)
REGDEF_BIT(y_lut17,        10)
REGDEF_BIT(y_lut18,        10)
REGDEF_END(SRAM_Y_CURVE8)


/*
    y_lut18:    [0x0, 0x3ff],           bits : 9_0
    y_lut19:    [0x0, 0x3ff],           bits : 19_10
    y_lut20:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE9_OFS 0x0724
REGDEF_BEGIN(SRAM_Y_CURVE9)
REGDEF_BIT(y_lut18,        10)
REGDEF_BIT(y_lut19,        10)
REGDEF_BIT(y_lut20,        10)
REGDEF_END(SRAM_Y_CURVE9)


/*
    y_lut20:    [0x0, 0x3ff],           bits : 9_0
    y_lut21:    [0x0, 0x3ff],           bits : 19_10
    y_lut22:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE10_OFS 0x0728
REGDEF_BEGIN(SRAM_Y_CURVE10)
REGDEF_BIT(y_lut20,        10)
REGDEF_BIT(y_lut21,        10)
REGDEF_BIT(y_lut22,        10)
REGDEF_END(SRAM_Y_CURVE10)


/*
    y_lut22:    [0x0, 0x3ff],           bits : 9_0
    y_lut23:    [0x0, 0x3ff],           bits : 19_10
    y_lut24:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE11_OFS 0x072c
REGDEF_BEGIN(SRAM_Y_CURVE11)
REGDEF_BIT(y_lut22,        10)
REGDEF_BIT(y_lut23,        10)
REGDEF_BIT(y_lut24,        10)
REGDEF_END(SRAM_Y_CURVE11)


/*
    y_lut24:    [0x0, 0x3ff],           bits : 9_0
    y_lut25:    [0x0, 0x3ff],           bits : 19_10
    y_lut26:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE12_OFS 0x0730
REGDEF_BEGIN(SRAM_Y_CURVE12)
REGDEF_BIT(y_lut24,        10)
REGDEF_BIT(y_lut25,        10)
REGDEF_BIT(y_lut26,        10)
REGDEF_END(SRAM_Y_CURVE12)


/*
    y_lut26:    [0x0, 0x3ff],           bits : 9_0
    y_lut27:    [0x0, 0x3ff],           bits : 19_10
    y_lut28:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE13_OFS 0x0734
REGDEF_BEGIN(SRAM_Y_CURVE13)
REGDEF_BIT(y_lut26,        10)
REGDEF_BIT(y_lut27,        10)
REGDEF_BIT(y_lut28,        10)
REGDEF_END(SRAM_Y_CURVE13)


/*
    y_lut28:    [0x0, 0x3ff],           bits : 9_0
    y_lut29:    [0x0, 0x3ff],           bits : 19_10
    y_lut30:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE14_OFS 0x0738
REGDEF_BEGIN(SRAM_Y_CURVE14)
REGDEF_BIT(y_lut28,        10)
REGDEF_BIT(y_lut29,        10)
REGDEF_BIT(y_lut30,        10)
REGDEF_END(SRAM_Y_CURVE14)


/*
    y_lut30:    [0x0, 0x3ff],           bits : 9_0
    y_lut31:    [0x0, 0x3ff],           bits : 19_10
    y_lut32:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE15_OFS 0x073c
REGDEF_BEGIN(SRAM_Y_CURVE15)
REGDEF_BIT(y_lut30,        10)
REGDEF_BIT(y_lut31,        10)
REGDEF_BIT(y_lut32,        10)
REGDEF_END(SRAM_Y_CURVE15)


/*
    y_lut32:    [0x0, 0x3ff],           bits : 9_0
    y_lut33:    [0x0, 0x3ff],           bits : 19_10
    y_lut34:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE16_OFS 0x0740
REGDEF_BEGIN(SRAM_Y_CURVE16)
REGDEF_BIT(y_lut32,        10)
REGDEF_BIT(y_lut33,        10)
REGDEF_BIT(y_lut34,        10)
REGDEF_END(SRAM_Y_CURVE16)


/*
    y_lut34:    [0x0, 0x3ff],           bits : 9_0
    y_lut35:    [0x0, 0x3ff],           bits : 19_10
    y_lut36:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE17_OFS 0x0744
REGDEF_BEGIN(SRAM_Y_CURVE17)
REGDEF_BIT(y_lut34,        10)
REGDEF_BIT(y_lut35,        10)
REGDEF_BIT(y_lut36,        10)
REGDEF_END(SRAM_Y_CURVE17)


/*
    y_lut36:    [0x0, 0x3ff],           bits : 9_0
    y_lut37:    [0x0, 0x3ff],           bits : 19_10
    y_lut38:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE18_OFS 0x0748
REGDEF_BEGIN(SRAM_Y_CURVE18)
REGDEF_BIT(y_lut36,        10)
REGDEF_BIT(y_lut37,        10)
REGDEF_BIT(y_lut38,        10)
REGDEF_END(SRAM_Y_CURVE18)


/*
    y_lut38:    [0x0, 0x3ff],           bits : 9_0
    y_lut39:    [0x0, 0x3ff],           bits : 19_10
    y_lut40:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE19_OFS 0x074c
REGDEF_BEGIN(SRAM_Y_CURVE19)
REGDEF_BIT(y_lut38,        10)
REGDEF_BIT(y_lut39,        10)
REGDEF_BIT(y_lut40,        10)
REGDEF_END(SRAM_Y_CURVE19)


/*
    y_lut40:    [0x0, 0x3ff],           bits : 9_0
    y_lut41:    [0x0, 0x3ff],           bits : 19_10
    y_lut42:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE20_OFS 0x0750
REGDEF_BEGIN(SRAM_Y_CURVE20)
REGDEF_BIT(y_lut40,        10)
REGDEF_BIT(y_lut41,        10)
REGDEF_BIT(y_lut42,        10)
REGDEF_END(SRAM_Y_CURVE20)


/*
    y_lut42:    [0x0, 0x3ff],           bits : 9_0
    y_lut43:    [0x0, 0x3ff],           bits : 19_10
    y_lut44:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE21_OFS 0x0754
REGDEF_BEGIN(SRAM_Y_CURVE21)
REGDEF_BIT(y_lut42,        10)
REGDEF_BIT(y_lut43,        10)
REGDEF_BIT(y_lut44,        10)
REGDEF_END(SRAM_Y_CURVE21)


/*
    y_lut44:    [0x0, 0x3ff],           bits : 9_0
    y_lut45:    [0x0, 0x3ff],           bits : 19_10
    y_lut46:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE22_OFS 0x0758
REGDEF_BEGIN(SRAM_Y_CURVE22)
REGDEF_BIT(y_lut44,        10)
REGDEF_BIT(y_lut45,        10)
REGDEF_BIT(y_lut46,        10)
REGDEF_END(SRAM_Y_CURVE22)


/*
    y_lut46:    [0x0, 0x3ff],           bits : 9_0
    y_lut47:    [0x0, 0x3ff],           bits : 19_10
    y_lut48:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE23_OFS 0x075c
REGDEF_BEGIN(SRAM_Y_CURVE23)
REGDEF_BIT(y_lut46,        10)
REGDEF_BIT(y_lut47,        10)
REGDEF_BIT(y_lut48,        10)
REGDEF_END(SRAM_Y_CURVE23)


/*
    y_lut48:    [0x0, 0x3ff],           bits : 9_0
    y_lut49:    [0x0, 0x3ff],           bits : 19_10
    y_lut50:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE24_OFS 0x0760
REGDEF_BEGIN(SRAM_Y_CURVE24)
REGDEF_BIT(y_lut48,        10)
REGDEF_BIT(y_lut49,        10)
REGDEF_BIT(y_lut50,        10)
REGDEF_END(SRAM_Y_CURVE24)


/*
    y_lut50:    [0x0, 0x3ff],           bits : 9_0
    y_lut51:    [0x0, 0x3ff],           bits : 19_10
    y_lut52:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE25_OFS 0x0764
REGDEF_BEGIN(SRAM_Y_CURVE25)
REGDEF_BIT(y_lut50,        10)
REGDEF_BIT(y_lut51,        10)
REGDEF_BIT(y_lut52,        10)
REGDEF_END(SRAM_Y_CURVE25)


/*
    y_lut52:    [0x0, 0x3ff],           bits : 9_0
    y_lut53:    [0x0, 0x3ff],           bits : 19_10
    y_lut54:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE26_OFS 0x0768
REGDEF_BEGIN(SRAM_Y_CURVE26)
REGDEF_BIT(y_lut52,        10)
REGDEF_BIT(y_lut53,        10)
REGDEF_BIT(y_lut54,        10)
REGDEF_END(SRAM_Y_CURVE26)


/*
    y_lut54:    [0x0, 0x3ff],           bits : 9_0
    y_lut55:    [0x0, 0x3ff],           bits : 19_10
    y_lut56:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE27_OFS 0x076c
REGDEF_BEGIN(SRAM_Y_CURVE27)
REGDEF_BIT(y_lut54,        10)
REGDEF_BIT(y_lut55,        10)
REGDEF_BIT(y_lut56,        10)
REGDEF_END(SRAM_Y_CURVE27)


/*
    y_lut56:    [0x0, 0x3ff],           bits : 9_0
    y_lut57:    [0x0, 0x3ff],           bits : 19_10
    y_lut58:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE28_OFS 0x0770
REGDEF_BEGIN(SRAM_Y_CURVE28)
REGDEF_BIT(y_lut56,        10)
REGDEF_BIT(y_lut57,        10)
REGDEF_BIT(y_lut58,        10)
REGDEF_END(SRAM_Y_CURVE28)


/*
    y_lut58:    [0x0, 0x3ff],           bits : 9_0
    y_lut59:    [0x0, 0x3ff],           bits : 19_10
    y_lut60:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE29_OFS 0x0774
REGDEF_BEGIN(SRAM_Y_CURVE29)
REGDEF_BIT(y_lut58,        10)
REGDEF_BIT(y_lut59,        10)
REGDEF_BIT(y_lut60,        10)
REGDEF_END(SRAM_Y_CURVE29)


/*
    y_lut60:    [0x0, 0x3ff],           bits : 9_0
    y_lut61:    [0x0, 0x3ff],           bits : 19_10
    y_lut62:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE30_OFS 0x0778
REGDEF_BEGIN(SRAM_Y_CURVE30)
REGDEF_BIT(y_lut60,        10)
REGDEF_BIT(y_lut61,        10)
REGDEF_BIT(y_lut62,        10)
REGDEF_END(SRAM_Y_CURVE30)


/*
    y_lut62:    [0x0, 0x3ff],           bits : 9_0
    y_lut63:    [0x0, 0x3ff],           bits : 19_10
    y_lut64:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE31_OFS 0x077c
REGDEF_BEGIN(SRAM_Y_CURVE31)
REGDEF_BIT(y_lut62,        10)
REGDEF_BIT(y_lut63,        10)
REGDEF_BIT(y_lut64,        10)
REGDEF_END(SRAM_Y_CURVE31)


/*
    y_lut64:    [0x0, 0x3ff],           bits : 9_0
    y_lut65:    [0x0, 0x3ff],           bits : 19_10
    y_lut66:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE32_OFS 0x0780
REGDEF_BEGIN(SRAM_Y_CURVE32)
REGDEF_BIT(y_lut64,        10)
REGDEF_BIT(y_lut65,        10)
REGDEF_BIT(y_lut66,        10)
REGDEF_END(SRAM_Y_CURVE32)


/*
    y_lut66:    [0x0, 0x3ff],           bits : 9_0
    y_lut67:    [0x0, 0x3ff],           bits : 19_10
    y_lut68:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE33_OFS 0x0784
REGDEF_BEGIN(SRAM_Y_CURVE33)
REGDEF_BIT(y_lut66,        10)
REGDEF_BIT(y_lut67,        10)
REGDEF_BIT(y_lut68,        10)
REGDEF_END(SRAM_Y_CURVE33)


/*
    y_lut68:    [0x0, 0x3ff],           bits : 9_0
    y_lut69:    [0x0, 0x3ff],           bits : 19_10
    y_lut70:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE34_OFS 0x0788
REGDEF_BEGIN(SRAM_Y_CURVE34)
REGDEF_BIT(y_lut68,        10)
REGDEF_BIT(y_lut69,        10)
REGDEF_BIT(y_lut70,        10)
REGDEF_END(SRAM_Y_CURVE34)


/*
    y_lut70:    [0x0, 0x3ff],           bits : 9_0
    y_lut71:    [0x0, 0x3ff],           bits : 19_10
    y_lut72:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE35_OFS 0x078c
REGDEF_BEGIN(SRAM_Y_CURVE35)
REGDEF_BIT(y_lut70,        10)
REGDEF_BIT(y_lut71,        10)
REGDEF_BIT(y_lut72,        10)
REGDEF_END(SRAM_Y_CURVE35)


/*
    y_lut72:    [0x0, 0x3ff],           bits : 9_0
    y_lut73:    [0x0, 0x3ff],           bits : 19_10
    y_lut74:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE36_OFS 0x0790
REGDEF_BEGIN(SRAM_Y_CURVE36)
REGDEF_BIT(y_lut72,        10)
REGDEF_BIT(y_lut73,        10)
REGDEF_BIT(y_lut74,        10)
REGDEF_END(SRAM_Y_CURVE36)


/*
    y_lut74:    [0x0, 0x3ff],           bits : 9_0
    y_lut75:    [0x0, 0x3ff],           bits : 19_10
    y_lut76:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE37_OFS 0x0794
REGDEF_BEGIN(SRAM_Y_CURVE37)
REGDEF_BIT(y_lut74,        10)
REGDEF_BIT(y_lut75,        10)
REGDEF_BIT(y_lut76,        10)
REGDEF_END(SRAM_Y_CURVE37)


/*
    y_lut76:    [0x0, 0x3ff],           bits : 9_0
    y_lut77:    [0x0, 0x3ff],           bits : 19_10
    y_lut78:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE38_OFS 0x0798
REGDEF_BEGIN(SRAM_Y_CURVE38)
REGDEF_BIT(y_lut76,        10)
REGDEF_BIT(y_lut77,        10)
REGDEF_BIT(y_lut78,        10)
REGDEF_END(SRAM_Y_CURVE38)


/*
    y_lut78:    [0x0, 0x3ff],           bits : 9_0
    y_lut79:    [0x0, 0x3ff],           bits : 19_10
    y_lut80:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE39_OFS 0x079c
REGDEF_BEGIN(SRAM_Y_CURVE39)
REGDEF_BIT(y_lut78,        10)
REGDEF_BIT(y_lut79,        10)
REGDEF_BIT(y_lut80,        10)
REGDEF_END(SRAM_Y_CURVE39)


/*
    y_lut80:    [0x0, 0x3ff],           bits : 9_0
    y_lut81:    [0x0, 0x3ff],           bits : 19_10
    y_lut82:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE40_OFS 0x07a0
REGDEF_BEGIN(SRAM_Y_CURVE40)
REGDEF_BIT(y_lut80,        10)
REGDEF_BIT(y_lut81,        10)
REGDEF_BIT(y_lut82,        10)
REGDEF_END(SRAM_Y_CURVE40)


/*
    y_lut82:    [0x0, 0x3ff],           bits : 9_0
    y_lut83:    [0x0, 0x3ff],           bits : 19_10
    y_lut84:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE41_OFS 0x07a4
REGDEF_BEGIN(SRAM_Y_CURVE41)
REGDEF_BIT(y_lut82,        10)
REGDEF_BIT(y_lut83,        10)
REGDEF_BIT(y_lut84,        10)
REGDEF_END(SRAM_Y_CURVE41)


/*
    y_lut84:    [0x0, 0x3ff],           bits : 9_0
    y_lut85:    [0x0, 0x3ff],           bits : 19_10
    y_lut86:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE42_OFS 0x07a8
REGDEF_BEGIN(SRAM_Y_CURVE42)
REGDEF_BIT(y_lut84,        10)
REGDEF_BIT(y_lut85,        10)
REGDEF_BIT(y_lut86,        10)
REGDEF_END(SRAM_Y_CURVE42)


/*
    y_lut86:    [0x0, 0x3ff],           bits : 9_0
    y_lut87:    [0x0, 0x3ff],           bits : 19_10
    y_lut88:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE43_OFS 0x07ac
REGDEF_BEGIN(SRAM_Y_CURVE43)
REGDEF_BIT(y_lut86,        10)
REGDEF_BIT(y_lut87,        10)
REGDEF_BIT(y_lut88,        10)
REGDEF_END(SRAM_Y_CURVE43)


/*
    y_lut88:    [0x0, 0x3ff],           bits : 9_0
    y_lut89:    [0x0, 0x3ff],           bits : 19_10
    y_lut90:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE44_OFS 0x07b0
REGDEF_BEGIN(SRAM_Y_CURVE44)
REGDEF_BIT(y_lut88,        10)
REGDEF_BIT(y_lut89,        10)
REGDEF_BIT(y_lut90,        10)
REGDEF_END(SRAM_Y_CURVE44)


/*
    y_lut90:    [0x0, 0x3ff],           bits : 9_0
    y_lut91:    [0x0, 0x3ff],           bits : 19_10
    y_lut92:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE45_OFS 0x07b4
REGDEF_BEGIN(SRAM_Y_CURVE45)
REGDEF_BIT(y_lut90,        10)
REGDEF_BIT(y_lut91,        10)
REGDEF_BIT(y_lut92,        10)
REGDEF_END(SRAM_Y_CURVE45)


/*
    y_lut92:    [0x0, 0x3ff],           bits : 9_0
    y_lut93:    [0x0, 0x3ff],           bits : 19_10
    y_lut94:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE46_OFS 0x07b8
REGDEF_BEGIN(SRAM_Y_CURVE46)
REGDEF_BIT(y_lut92,        10)
REGDEF_BIT(y_lut93,        10)
REGDEF_BIT(y_lut94,        10)
REGDEF_END(SRAM_Y_CURVE46)


/*
    y_lut94:    [0x0, 0x3ff],           bits : 9_0
    y_lut95:    [0x0, 0x3ff],           bits : 19_10
    y_lut96:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE47_OFS 0x07bc
REGDEF_BEGIN(SRAM_Y_CURVE47)
REGDEF_BIT(y_lut94,        10)
REGDEF_BIT(y_lut95,        10)
REGDEF_BIT(y_lut96,        10)
REGDEF_END(SRAM_Y_CURVE47)


/*
    y_lut96:    [0x0, 0x3ff],           bits : 9_0
    y_lut97:    [0x0, 0x3ff],           bits : 19_10
    y_lut98:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE48_OFS 0x07c0
REGDEF_BEGIN(SRAM_Y_CURVE48)
REGDEF_BIT(y_lut96,        10)
REGDEF_BIT(y_lut97,        10)
REGDEF_BIT(y_lut98,        10)
REGDEF_END(SRAM_Y_CURVE48)


/*
    y_lut98 :    [0x0, 0x3ff],          bits : 9_0
    y_lut99 :    [0x0, 0x3ff],          bits : 19_10
    y_lut100:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE49_OFS 0x07c4
REGDEF_BEGIN(SRAM_Y_CURVE49)
REGDEF_BIT(y_lut98,        10)
REGDEF_BIT(y_lut99,        10)
REGDEF_BIT(y_lut100,        10)
REGDEF_END(SRAM_Y_CURVE49)


/*
    y_lut100:    [0x0, 0x3ff],          bits : 9_0
    y_lut101:    [0x0, 0x3ff],          bits : 19_10
    y_lut102:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE50_OFS 0x07c8
REGDEF_BEGIN(SRAM_Y_CURVE50)
REGDEF_BIT(y_lut100,        10)
REGDEF_BIT(y_lut101,        10)
REGDEF_BIT(y_lut102,        10)
REGDEF_END(SRAM_Y_CURVE50)


/*
    y_lut102:    [0x0, 0x3ff],          bits : 9_0
    y_lut103:    [0x0, 0x3ff],          bits : 19_10
    y_lut104:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE51_OFS 0x07cc
REGDEF_BEGIN(SRAM_Y_CURVE51)
REGDEF_BIT(y_lut102,        10)
REGDEF_BIT(y_lut103,        10)
REGDEF_BIT(y_lut104,        10)
REGDEF_END(SRAM_Y_CURVE51)


/*
    y_lut104:    [0x0, 0x3ff],          bits : 9_0
    y_lut105:    [0x0, 0x3ff],          bits : 19_10
    y_lut106:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE52_OFS 0x07d0
REGDEF_BEGIN(SRAM_Y_CURVE52)
REGDEF_BIT(y_lut104,        10)
REGDEF_BIT(y_lut105,        10)
REGDEF_BIT(y_lut106,        10)
REGDEF_END(SRAM_Y_CURVE52)


/*
    y_lut106:    [0x0, 0x3ff],          bits : 9_0
    y_lut107:    [0x0, 0x3ff],          bits : 19_10
    y_lut108:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE53_OFS 0x07d4
REGDEF_BEGIN(SRAM_Y_CURVE53)
REGDEF_BIT(y_lut106,        10)
REGDEF_BIT(y_lut107,        10)
REGDEF_BIT(y_lut108,        10)
REGDEF_END(SRAM_Y_CURVE53)


/*
    y_lut108:    [0x0, 0x3ff],          bits : 9_0
    y_lut109:    [0x0, 0x3ff],          bits : 19_10
    y_lut110:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE54_OFS 0x07d8
REGDEF_BEGIN(SRAM_Y_CURVE54)
REGDEF_BIT(y_lut108,        10)
REGDEF_BIT(y_lut109,        10)
REGDEF_BIT(y_lut110,        10)
REGDEF_END(SRAM_Y_CURVE54)


/*
    y_lut110:    [0x0, 0x3ff],          bits : 9_0
    y_lut111:    [0x0, 0x3ff],          bits : 19_10
    y_lut112:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE55_OFS 0x07dc
REGDEF_BEGIN(SRAM_Y_CURVE55)
REGDEF_BIT(y_lut110,        10)
REGDEF_BIT(y_lut111,        10)
REGDEF_BIT(y_lut112,        10)
REGDEF_END(SRAM_Y_CURVE55)


/*
    y_lut112:    [0x0, 0x3ff],          bits : 9_0
    y_lut113:    [0x0, 0x3ff],          bits : 19_10
    y_lut114:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE56_OFS 0x07e0
REGDEF_BEGIN(SRAM_Y_CURVE56)
REGDEF_BIT(y_lut112,        10)
REGDEF_BIT(y_lut113,        10)
REGDEF_BIT(y_lut114,        10)
REGDEF_END(SRAM_Y_CURVE56)


/*
    y_lut114:    [0x0, 0x3ff],          bits : 9_0
    y_lut115:    [0x0, 0x3ff],          bits : 19_10
    y_lut116:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE57_OFS 0x07e4
REGDEF_BEGIN(SRAM_Y_CURVE57)
REGDEF_BIT(y_lut114,        10)
REGDEF_BIT(y_lut115,        10)
REGDEF_BIT(y_lut116,        10)
REGDEF_END(SRAM_Y_CURVE57)


/*
    y_lut116:    [0x0, 0x3ff],          bits : 9_0
    y_lut117:    [0x0, 0x3ff],          bits : 19_10
    y_lut118:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE58_OFS 0x07e8
REGDEF_BEGIN(SRAM_Y_CURVE58)
REGDEF_BIT(y_lut116,        10)
REGDEF_BIT(y_lut117,        10)
REGDEF_BIT(y_lut118,        10)
REGDEF_END(SRAM_Y_CURVE58)


/*
    y_lut118:    [0x0, 0x3ff],          bits : 9_0
    y_lut119:    [0x0, 0x3ff],          bits : 19_10
    y_lut120:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE59_OFS 0x07ec
REGDEF_BEGIN(SRAM_Y_CURVE59)
REGDEF_BIT(y_lut118,        10)
REGDEF_BIT(y_lut119,        10)
REGDEF_BIT(y_lut120,        10)
REGDEF_END(SRAM_Y_CURVE59)


/*
    y_lut120:    [0x0, 0x3ff],          bits : 9_0
    y_lut121:    [0x0, 0x3ff],          bits : 19_10
    y_lut122:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE60_OFS 0x07f0
REGDEF_BEGIN(SRAM_Y_CURVE60)
REGDEF_BIT(y_lut120,        10)
REGDEF_BIT(y_lut121,        10)
REGDEF_BIT(y_lut122,        10)
REGDEF_END(SRAM_Y_CURVE60)


/*
    y_lut122:    [0x0, 0x3ff],          bits : 9_0
    y_lut123:    [0x0, 0x3ff],          bits : 19_10
    y_lut124:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE61_OFS 0x07f4
REGDEF_BEGIN(SRAM_Y_CURVE61)
REGDEF_BIT(y_lut122,        10)
REGDEF_BIT(y_lut123,        10)
REGDEF_BIT(y_lut124,        10)
REGDEF_END(SRAM_Y_CURVE61)


/*
    y_lut124:    [0x0, 0x3ff],          bits : 9_0
    y_lut125:    [0x0, 0x3ff],          bits : 19_10
    y_lut126:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE62_OFS 0x07f8
REGDEF_BEGIN(SRAM_Y_CURVE62)
REGDEF_BIT(y_lut124,        10)
REGDEF_BIT(y_lut125,        10)
REGDEF_BIT(y_lut126,        10)
REGDEF_END(SRAM_Y_CURVE62)


/*
    y_lut126:    [0x0, 0x3ff],          bits : 9_0
    y_lut127:    [0x0, 0x3ff],          bits : 19_10
    y_lut128:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE63_OFS 0x07fc
REGDEF_BEGIN(SRAM_Y_CURVE63)
REGDEF_BIT(y_lut126,        10)
REGDEF_BIT(y_lut127,        10)
REGDEF_BIT(y_lut128,        10)
REGDEF_END(SRAM_Y_CURVE63)


/*
    edge_lut0:    [0x0, 0x3ff],         bits : 9_0
    edge_lut1:    [0x0, 0x3ff],         bits : 19_10
    edge_lut2:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA0_OFS 0x0800
REGDEF_BEGIN(EDGE_GAMMA0)
REGDEF_BIT(edge_lut0,        10)
REGDEF_BIT(edge_lut1,        10)
REGDEF_BIT(edge_lut2,        10)
REGDEF_END(EDGE_GAMMA0)


/*
    edge_lut3:    [0x0, 0x3ff],         bits : 9_0
    edge_lut4:    [0x0, 0x3ff],         bits : 19_10
    edge_lut5:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA1_OFS 0x0804
REGDEF_BEGIN(EDGE_GAMMA1)
REGDEF_BIT(edge_lut3,        10)
REGDEF_BIT(edge_lut4,        10)
REGDEF_BIT(edge_lut5,        10)
REGDEF_END(EDGE_GAMMA1)


/*
    edge_lut6:    [0x0, 0x3ff],         bits : 9_0
    edge_lut7:    [0x0, 0x3ff],         bits : 19_10
    edge_lut8:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA2_OFS 0x0808
REGDEF_BEGIN(EDGE_GAMMA2)
REGDEF_BIT(edge_lut6,        10)
REGDEF_BIT(edge_lut7,        10)
REGDEF_BIT(edge_lut8,        10)
REGDEF_END(EDGE_GAMMA2)


/*
    edge_lut9 :    [0x0, 0x3ff],            bits : 9_0
    edge_lut10:    [0x0, 0x3ff],            bits : 19_10
    edge_lut11:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA3_OFS 0x080c
REGDEF_BEGIN(EDGE_GAMMA3)
REGDEF_BIT(edge_lut9,        10)
REGDEF_BIT(edge_lut10,        10)
REGDEF_BIT(edge_lut11,        10)
REGDEF_END(EDGE_GAMMA3)


/*
    edge_lut12:    [0x0, 0x3ff],            bits : 9_0
    edge_lut13:    [0x0, 0x3ff],            bits : 19_10
    edge_lut14:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA4_OFS 0x0810
REGDEF_BEGIN(EDGE_GAMMA4)
REGDEF_BIT(edge_lut12,        10)
REGDEF_BIT(edge_lut13,        10)
REGDEF_BIT(edge_lut14,        10)
REGDEF_END(EDGE_GAMMA4)


/*
    edge_lut15:    [0x0, 0x3ff],            bits : 9_0
    edge_lut16:    [0x0, 0x3ff],            bits : 19_10
    edge_lut17:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA5_OFS 0x0814
REGDEF_BEGIN(EDGE_GAMMA5)
REGDEF_BIT(edge_lut15,        10)
REGDEF_BIT(edge_lut16,        10)
REGDEF_BIT(edge_lut17,        10)
REGDEF_END(EDGE_GAMMA5)


/*
    edge_lut18:    [0x0, 0x3ff],            bits : 9_0
    edge_lut19:    [0x0, 0x3ff],            bits : 19_10
    edge_lut20:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA6_OFS 0x0818
REGDEF_BEGIN(EDGE_GAMMA6)
REGDEF_BIT(edge_lut18,        10)
REGDEF_BIT(edge_lut19,        10)
REGDEF_BIT(edge_lut20,        10)
REGDEF_END(EDGE_GAMMA6)


/*
    edge_lut21:    [0x0, 0x3ff],            bits : 9_0
    edge_lut22:    [0x0, 0x3ff],            bits : 19_10
    edge_lut23:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA7_OFS 0x081c
REGDEF_BEGIN(EDGE_GAMMA7)
REGDEF_BIT(edge_lut21,        10)
REGDEF_BIT(edge_lut22,        10)
REGDEF_BIT(edge_lut23,        10)
REGDEF_END(EDGE_GAMMA7)


/*
    edge_lut24:    [0x0, 0x3ff],            bits : 9_0
    edge_lut25:    [0x0, 0x3ff],            bits : 19_10
    edge_lut26:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA8_OFS 0x0820
REGDEF_BEGIN(EDGE_GAMMA8)
REGDEF_BIT(edge_lut24,        10)
REGDEF_BIT(edge_lut25,        10)
REGDEF_BIT(edge_lut26,        10)
REGDEF_END(EDGE_GAMMA8)


/*
    edge_lut27:    [0x0, 0x3ff],            bits : 9_0
    edge_lut28:    [0x0, 0x3ff],            bits : 19_10
    edge_lut29:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA9_OFS 0x0824
REGDEF_BEGIN(EDGE_GAMMA9)
REGDEF_BIT(edge_lut27,        10)
REGDEF_BIT(edge_lut28,        10)
REGDEF_BIT(edge_lut29,        10)
REGDEF_END(EDGE_GAMMA9)


/*
    edge_lut30:    [0x0, 0x3ff],            bits : 9_0
    edge_lut31:    [0x0, 0x3ff],            bits : 19_10
    edge_lut32:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA10_OFS 0x0828
REGDEF_BEGIN(EDGE_GAMMA10)
REGDEF_BIT(edge_lut30,        10)
REGDEF_BIT(edge_lut31,        10)
REGDEF_BIT(edge_lut32,        10)
REGDEF_END(EDGE_GAMMA10)


/*
    edge_lut33:    [0x0, 0x3ff],            bits : 9_0
    edge_lut34:    [0x0, 0x3ff],            bits : 19_10
    edge_lut35:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA11_OFS 0x082c
REGDEF_BEGIN(EDGE_GAMMA11)
REGDEF_BIT(edge_lut33,        10)
REGDEF_BIT(edge_lut34,        10)
REGDEF_BIT(edge_lut35,        10)
REGDEF_END(EDGE_GAMMA11)


/*
    edge_lut36:    [0x0, 0x3ff],            bits : 9_0
    edge_lut37:    [0x0, 0x3ff],            bits : 19_10
    edge_lut38:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA12_OFS 0x0830
REGDEF_BEGIN(EDGE_GAMMA12)
REGDEF_BIT(edge_lut36,        10)
REGDEF_BIT(edge_lut37,        10)
REGDEF_BIT(edge_lut38,        10)
REGDEF_END(EDGE_GAMMA12)


/*
    edge_lut39:    [0x0, 0x3ff],            bits : 9_0
    edge_lut40:    [0x0, 0x3ff],            bits : 19_10
    edge_lut41:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA13_OFS 0x0834
REGDEF_BEGIN(EDGE_GAMMA13)
REGDEF_BIT(edge_lut39,        10)
REGDEF_BIT(edge_lut40,        10)
REGDEF_BIT(edge_lut41,        10)
REGDEF_END(EDGE_GAMMA13)


/*
    edge_lut42:    [0x0, 0x3ff],            bits : 9_0
    edge_lut43:    [0x0, 0x3ff],            bits : 19_10
    edge_lut44:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA14_OFS 0x0838
REGDEF_BEGIN(EDGE_GAMMA14)
REGDEF_BIT(edge_lut42,        10)
REGDEF_BIT(edge_lut43,        10)
REGDEF_BIT(edge_lut44,        10)
REGDEF_END(EDGE_GAMMA14)


/*
    edge_lut45:    [0x0, 0x3ff],            bits : 9_0
    edge_lut46:    [0x0, 0x3ff],            bits : 19_10
    edge_lut47:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA15_OFS 0x083c
REGDEF_BEGIN(EDGE_GAMMA15)
REGDEF_BIT(edge_lut45,        10)
REGDEF_BIT(edge_lut46,        10)
REGDEF_BIT(edge_lut47,        10)
REGDEF_END(EDGE_GAMMA15)


/*
    edge_lut48:    [0x0, 0x3ff],            bits : 9_0
    edge_lut49:    [0x0, 0x3ff],            bits : 19_10
    edge_lut50:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA16_OFS 0x0840
REGDEF_BEGIN(EDGE_GAMMA16)
REGDEF_BIT(edge_lut48,        10)
REGDEF_BIT(edge_lut49,        10)
REGDEF_BIT(edge_lut50,        10)
REGDEF_END(EDGE_GAMMA16)


/*
    edge_lut51:    [0x0, 0x3ff],            bits : 9_0
    edge_lut52:    [0x0, 0x3ff],            bits : 19_10
    edge_lut53:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA17_OFS 0x0844
REGDEF_BEGIN(EDGE_GAMMA17)
REGDEF_BIT(edge_lut51,        10)
REGDEF_BIT(edge_lut52,        10)
REGDEF_BIT(edge_lut53,        10)
REGDEF_END(EDGE_GAMMA17)


/*
    edge_lut54:    [0x0, 0x3ff],            bits : 9_0
    edge_lut55:    [0x0, 0x3ff],            bits : 19_10
    edge_lut56:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA18_OFS 0x0848
REGDEF_BEGIN(EDGE_GAMMA18)
REGDEF_BIT(edge_lut54,        10)
REGDEF_BIT(edge_lut55,        10)
REGDEF_BIT(edge_lut56,        10)
REGDEF_END(EDGE_GAMMA18)


/*
    edge_lut57:    [0x0, 0x3ff],            bits : 9_0
    edge_lut58:    [0x0, 0x3ff],            bits : 19_10
    edge_lut59:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA19_OFS 0x084c
REGDEF_BEGIN(EDGE_GAMMA19)
REGDEF_BIT(edge_lut57,        10)
REGDEF_BIT(edge_lut58,        10)
REGDEF_BIT(edge_lut59,        10)
REGDEF_END(EDGE_GAMMA19)


/*
    edge_lut60:    [0x0, 0x3ff],            bits : 9_0
    edge_lut61:    [0x0, 0x3ff],            bits : 19_10
    edge_lut62:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA20_OFS 0x0850
REGDEF_BEGIN(EDGE_GAMMA20)
REGDEF_BIT(edge_lut60,        10)
REGDEF_BIT(edge_lut61,        10)
REGDEF_BIT(edge_lut62,        10)
REGDEF_END(EDGE_GAMMA20)


/*
    edge_lut63:    [0x0, 0x3ff],            bits : 9_0
    edge_lut64:    [0x0, 0x3ff],            bits : 19_10
*/
#define EDGE_GAMMA21_OFS 0x0854
REGDEF_BEGIN(EDGE_GAMMA21)
REGDEF_BIT(edge_lut63,        10)
REGDEF_BIT(edge_lut64,        10)
REGDEF_END(EDGE_GAMMA21)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER0_OFS 0x0858
REGDEF_BEGIN(IP_CHECKSUM_REGISTER0)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER1_OFS 0x085c
REGDEF_BEGIN(IP_CHECKSUM_REGISTER1)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER2_OFS 0x0860
REGDEF_BEGIN(IP_CHECKSUM_REGISTER2)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER3_OFS 0x0864
REGDEF_BEGIN(IP_CHECKSUM_REGISTER3)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER4_OFS 0x0868
REGDEF_BEGIN(IP_CHECKSUM_REGISTER4)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER4)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER5_OFS 0x086c
REGDEF_BEGIN(IP_CHECKSUM_REGISTER5)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER5)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER6_OFS 0x0870
REGDEF_BEGIN(IP_CHECKSUM_REGISTER6)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER6)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER7_OFS 0x0874
REGDEF_BEGIN(IP_CHECKSUM_REGISTER7)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER7)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER8_OFS 0x0878
REGDEF_BEGIN(IP_CHECKSUM_REGISTER8)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER8)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER9_OFS 0x087c
REGDEF_BEGIN(IP_CHECKSUM_REGISTER9)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER9)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER10_OFS 0x0880
REGDEF_BEGIN(IP_CHECKSUM_REGISTER10)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER10)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER11_OFS 0x0884
REGDEF_BEGIN(IP_CHECKSUM_REGISTER11)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER11)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER12_OFS 0x0888
REGDEF_BEGIN(IP_CHECKSUM_REGISTER12)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER12)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER13_OFS 0x088c
REGDEF_BEGIN(IP_CHECKSUM_REGISTER13)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER13)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER14_OFS 0x0890
REGDEF_BEGIN(IP_CHECKSUM_REGISTER14)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER14)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER15_OFS 0x0894
REGDEF_BEGIN(IP_CHECKSUM_REGISTER15)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER15)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER16_OFS 0x0898
REGDEF_BEGIN(IP_CHECKSUM_REGISTER16)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER16)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER17_OFS 0x089c
REGDEF_BEGIN(IP_CHECKSUM_REGISTER17)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER17)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER18_OFS 0x08a0
REGDEF_BEGIN(IP_CHECKSUM_REGISTER18)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER18)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER19_OFS 0x08a4
REGDEF_BEGIN(IP_CHECKSUM_REGISTER19)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER19)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER20_OFS 0x08a8
REGDEF_BEGIN(IP_CHECKSUM_REGISTER20)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER20)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER21_OFS 0x08ac
REGDEF_BEGIN(IP_CHECKSUM_REGISTER21)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER21)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IP_CHECKSUM_REGISTER22_OFS 0x08b0
REGDEF_BEGIN(IP_CHECKSUM_REGISTER22)
REGDEF_BIT(reserved,        32)
REGDEF_END(IP_CHECKSUM_REGISTER22)

//----------------------------------------------------------

typedef struct {

	union {
		struct {
			unsigned ipe_swrst:                   1;
			unsigned ipe_start:                   1;
			unsigned ipe_load_start:              1;
			unsigned ipe_load_fd:                 1;
			unsigned ipe_load_fs:                 1;
			unsigned ipe_gbl_load_en:             1;
			unsigned :                            2;
			unsigned ipe_rwgamma:                 2;
			unsigned ipe_rwgamma_opt:             2;
			unsigned :                               16;
			unsigned ll_fire:                     1;
		} bit;
		uint32_t word;
	} reg_0; // 0x0000

	union {
		struct {
			unsigned ipe_mode               : 2;        // bits : 1_0
			unsigned ipe_imeo_en            : 1;        // bits : 2
			unsigned ipe_dmao_en            : 1;        // bits : 3
			unsigned dmao_sel               : 2;        // bits : 5_4
			unsigned                        : 2;
			unsigned ipe_imat               : 2;        // bits : 9_8
			unsigned ipe_omat               : 2;        // bits : 11_10
			unsigned ipe_subhsel            : 1;        // bits : 12
			unsigned                        : 14;
			unsigned ipe_mst_hovlp          : 1;        // bits : 27
			unsigned iny_burst_mode         : 1;        // bits : 28
			unsigned inc_burst_mode         : 1;        // bits : 29
			unsigned outy_burst_mode        : 1;        // bits : 30
			unsigned outc_burst_mode        : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_1; // 0x0004

	union {
		struct {
			unsigned                          : 1;
			unsigned rgbgamma_en              : 1;      // bits : 1
			unsigned ycurve_en                : 1;      // bits : 2
			unsigned cr_en                    : 1;      // bits : 3
			unsigned lce_subimg_out_en        : 1;      // bits : 4
			unsigned                          : 1;
			unsigned lce_en                   : 1;      // bits : 6
			unsigned cst_en                   : 1;      // bits : 7
			unsigned                          : 1;
			unsigned ctrl_en                  : 1;      // bits : 9
			unsigned hadj_en                  : 1;      // bits : 10
			unsigned cadj_en                  : 1;      // bits : 11
			unsigned cadj_yenh_en             : 1;      // bits : 12
			unsigned cadj_ycon_en             : 1;      // bits : 13
			unsigned cadj_ccon_en             : 1;      // bits : 14
			unsigned cadj_ycth_en             : 1;      // bits : 15
			unsigned cstprotect_en            : 1;      // bits : 16
			unsigned edge_dbg_en              : 1;      // bits : 17
			unsigned                          : 1;
			unsigned vacc_en                  : 1;      // bits : 19
			unsigned win0_va_en               : 1;      // bits : 20
			unsigned win1_va_en               : 1;      // bits : 21
			unsigned win2_va_en               : 1;      // bits : 22
			unsigned win3_va_en               : 1;      // bits : 23
			unsigned win4_va_en               : 1;      // bits : 24
		} bit;
		uint32_t word;
	} reg_2; // 0x0008

	union {
		struct {
			unsigned ipe_dram_out0_single_en        : 1;        // bits : 0
			unsigned ipe_dram_out1_single_en        : 1;        // bits : 1
			unsigned ipe_dram_out2_single_en        : 1;        // bits : 2
			unsigned ipe_dram_out3_single_en        : 1;        // bits : 3
			unsigned ipe_dram_out_mode              : 1;        // bits : 4
		} bit;
		uint32_t word;
	} reg_3; // 0x000c

	union {
		struct {
			unsigned ll_terminate        : 1;       // bits : 0
		} bit;
		uint32_t word;
	} reg_4; // 0x0010

	union {
		struct {
			unsigned hn        : 11;        // bits : 10_0
			unsigned           : 1;
			unsigned hl        : 11;        // bits : 22_12
			unsigned           : 1;
			unsigned hm        : 4;     // bits : 27_24
		} bit;
		uint32_t word;
	} reg_5; // 0x0014

	union {
		struct {
			unsigned vl        : 13;        // bits : 12_0
		} bit;
		uint32_t word;
	} reg_6; // 0x0018

	union {
		struct {
			unsigned                    : 2;
			unsigned dram_sai_ll        : 30;       // bits : 31_2
		} bit;
		uint32_t word;
	} reg_7; // 0x001c

	union {
		struct {
			unsigned                  : 2;
			unsigned dram_sai_y        : 30;     // bits : 31_2
		} bit;
		uint32_t word;
	} reg_8; // 0x0020

	union {
		struct {
			unsigned                  : 2;
			unsigned dram_sai_c        : 30;     // bits : 31_2
		} bit;
		uint32_t word;
	} reg_9; // 0x0024

	union {
		struct {
			unsigned                       : 2;
			unsigned dram_sai_ycurve        : 30;        // bits : 31_2
		} bit;
		uint32_t word;
	} reg_10; // 0x0028

	union {
		struct {
			unsigned                      : 2;
			unsigned dram_sai_gamma        : 30;     // bits : 31_2
		} bit;
		uint32_t word;
	} reg_11; // 0x002c

	union {
		struct {
			unsigned :                                2;
			unsigned dram_ofsi_y:                14;
			unsigned :                               12;
			unsigned inrand_en:                   1;
			unsigned inrand_rst:                  1;
		} bit;
		uint32_t word;
	} reg_12; // 0x0030

	union {
		struct {
			unsigned :                                2;
			unsigned dram_ofsi_c:                14;
		} bit;
		uint32_t word;
	} reg_13; // 0x0034

	union {
		struct {
			unsigned :                               2;
			unsigned dram_sao_y:                30;
		} bit;
		uint32_t word;
	} reg_14; // 0x0038

	union {
		struct {
			unsigned :                               2;
			unsigned dram_sao_c:                30;
		} bit;
		uint32_t word;
	} reg_15; // 0x003c

	union {
		struct {
			unsigned :                                2;
			unsigned dram_ofso_y:                14;
		} bit;
		uint32_t word;
	} reg_16; // 0x0040

	union {
		struct {
			unsigned :                                2;
			unsigned dram_ofso_c:                14;
		} bit;
		uint32_t word;
	} reg_17; // 0x0044

	union {
		struct {
			unsigned :                                2;
			unsigned dram_sao_va:                30;
		} bit;
		uint32_t word;
	} reg_18; // 0x0048

	union {
		struct {
			unsigned :                                 2;
			unsigned dram_ofso_va:                14;
		} bit;
		uint32_t word;
	} reg_19; // 0x004c

	union {
		struct {
			unsigned                                : 1;
			unsigned ipe_inte_frm_end               : 1;        // bits : 1
			unsigned ipe_inte_stp_end               : 1;        // bits : 2
			unsigned ipe_inte_frm_start             : 1;        // bits : 3
			unsigned ipe_inte_ycout_end             : 1;        // bits : 4
			unsigned ipe_inte_dmain0_end            : 1;        // bits : 5
			unsigned ipe_inte_lce_subout_end        : 1;        // bits : 6
			unsigned ipe_inte_vaout_end             : 1;        // bits : 7
			unsigned ipe_inte_ll_done               : 1;        // bits : 8
			unsigned ipe_inte_ll_job_end            : 1;        // bits : 9
			unsigned ipe_inte_ll_error              : 1;        // bits : 10
			unsigned ipe_inte_ll_error2             : 1;        // bits : 11
		} bit;
		uint32_t word;
	} reg_20; // 0x0050

	union {
		struct {
			unsigned                                : 1;
			unsigned ipe_ints_frm_end               : 1;        // bits : 1
			unsigned ipe_ints_stp_end               : 1;        // bits : 2
			unsigned ipe_ints_frm_start             : 1;        // bits : 3
			unsigned ipe_ints_ycout_end             : 1;        // bits : 4
			unsigned ipe_ints_dmain0_end            : 1;        // bits : 5
			unsigned ipe_ints_lce_subout_end        : 1;        // bits : 6
			unsigned ipe_ints_vaout_end             : 1;        // bits : 7
			unsigned ipe_ints_ll_done               : 1;        // bits : 8
			unsigned ipe_ints_ll_job_end            : 1;        // bits : 9
			unsigned ipe_ints_ll_error              : 1;        // bits : 10
			unsigned ipe_ints_ll_error2             : 1;        // bits : 11
		} bit;
		uint32_t word;
	} reg_21; // 0x0054

	union {
		struct {
			unsigned ipestatus                : 1;      // bits : 0
			unsigned                          : 3;
			unsigned hcnt                     : 4;      // bits : 7_4
			unsigned                          : 4;
			unsigned status_y                 : 1;      // bits : 12
			unsigned status_c                 : 1;      // bits : 13
			unsigned                          : 2;
			unsigned fstr_rst_disable         : 1;      // bits : 16
			unsigned                          : 7;
			unsigned sram0_light_sleep        : 1;      // bits : 24
			unsigned sram1_light_sleep        : 1;      // bits : 25
			unsigned sram2_light_sleep        : 1;      // bits : 26
			unsigned sram3_light_sleep        : 1;      // bits : 27
			unsigned sram4_light_sleep        : 1;      // bits : 28
			unsigned sram5_light_sleep        : 1;      // bits : 29
			unsigned sram6_light_sleep        : 1;      // bits : 30
			unsigned sram7_light_sleep        : 1;      // bits : 31
		} bit;
		uint32_t word;
	} reg_22; // 0x0058

	union {
		struct {
			unsigned ll_table_idx_0        : 8;     // bits : 7_0
			unsigned ll_table_idx_1        : 8;     // bits : 15_8
			unsigned ll_table_idx_2        : 8;     // bits : 23_16
			unsigned ll_table_idx_3        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_23; // 0x005c

	union {
		struct {
			unsigned ll_table_idx_4        : 8;     // bits : 7_0
			unsigned ll_table_idx_5        : 8;     // bits : 15_8
			unsigned ll_table_idx_6        : 8;     // bits : 23_16
			unsigned ll_table_idx_7        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_24; // 0x0060

	union {
		struct {
			unsigned ll_table_idx_8         : 8;        // bits : 7_0
			unsigned ll_table_idx_9         : 8;        // bits : 15_8
			unsigned ll_table_idx_10        : 8;        // bits : 23_16
			unsigned ll_table_idx_11        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_25; // 0x0064

	union {
		struct {
			unsigned ll_table_idx_12        : 8;        // bits : 7_0
			unsigned ll_table_idx_13        : 8;        // bits : 15_8
			unsigned ll_table_idx_14        : 8;        // bits : 23_16
			unsigned ll_table_idx_15        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_26; // 0x0068

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_27; // 0x006c

	union {
		struct {
			unsigned eext_chsel         : 1;        // bits : 0
			unsigned eext_gamsel        : 1;        // bits : 1
			unsigned                    : 2;
			unsigned eextdiv_e7         : 4;        // bits : 7_4
			unsigned eext_enh_e7        : 5;        // bits : 12_8
			unsigned                    : 3;
			unsigned eext0              : 10;       // bits : 25_16
		} bit;
		uint32_t word;
	} reg_28; // 0x0070

	union {
		struct {
			unsigned eext1        : 10;     // bits : 9_0
			unsigned eext2        : 10;     // bits : 19_10
			unsigned eext3        : 10;     // bits : 29_20
		} bit;
		uint32_t word;
	} reg_29; // 0x0074

	union {
		struct {
			unsigned eext4        : 10;     // bits : 9_0
			unsigned eext5        : 10;     // bits : 19_10
			unsigned eext6        : 10;     // bits : 29_20
		} bit;
		uint32_t word;
	} reg_30; // 0x0078

	union {
		struct {
			unsigned eext7        : 10;     // bits : 9_0
			unsigned eext8        : 10;     // bits : 19_10
			unsigned eext9        : 10;     // bits : 29_20
		} bit;
		uint32_t word;
	} reg_31; // 0x007c

	union {
		struct {
			unsigned eext_enh_e3         : 6;       // bits : 5_0
			unsigned                     : 2;
			unsigned eext_enh_e5a        : 6;       // bits : 13_8
			unsigned                     : 2;
			unsigned eext_enh_e5b        : 6;       // bits : 21_16
		} bit;
		uint32_t word;
	} reg_32; // 0x0080

	union {
		struct {
			unsigned eextdiv_e3         : 3;        // bits : 2_0
			unsigned                    : 1;
			unsigned eextdiv_e5a        : 3;        // bits : 6_4
			unsigned                    : 1;
			unsigned eextdiv_e5b        : 3;        // bits : 10_8
			unsigned                    : 1;
			unsigned eextdiv_eng        : 4;        // bits : 15_12
			unsigned eextdiv_con        : 4;        // bits : 19_16
		} bit;
		uint32_t word;
	} reg_33; // 0x0084

	union {
		struct {
			unsigned w_con_eng        : 4;      // bits : 3_0
			unsigned w_low            : 5;      // bits : 8_4
			unsigned                  : 3;
			unsigned w_high           : 5;      // bits : 16_12
		} bit;
		uint32_t word;
	} reg_34; // 0x0088

	union {
		struct {
			unsigned w_ker_thin            : 4;     // bits : 3_0
			unsigned w_ker_robust          : 4;     // bits : 7_4
			unsigned iso_ker_thin          : 4;     // bits : 11_8
			unsigned iso_ker_robust        : 4;     // bits : 15_12
		} bit;
		uint32_t word;
	} reg_35; // 0x008c

	union {
		struct {
			unsigned w_hld_low                 : 5;     // bits : 4_0
			unsigned                           : 3;
			unsigned w_hld_high                : 5;     // bits : 12_8
			unsigned                           : 3;
			unsigned w_ker_thin_hld            : 4;     // bits : 19_16
			unsigned w_ker_robust_hld          : 4;     // bits : 23_20
			unsigned iso_ker_thin_hld          : 4;     // bits : 27_24
			unsigned iso_ker_robust_hld        : 4;     // bits : 31_28
		} bit;
		uint32_t word;
	} reg_36; // 0x0090

	union {
		struct {
			unsigned th_flat        : 10;       // bits : 9_0
			unsigned                : 2;
			unsigned th_edge        : 10;       // bits : 21_12
		} bit;
		uint32_t word;
	} reg_37; // 0x0094

	union {
		struct {
			unsigned th_flat_hld        : 10;       // bits : 9_0
			unsigned                    : 2;
			unsigned th_edge_hld        : 10;       // bits : 21_12
			unsigned th_lum_hld         : 10;       // bits : 31_22
		} bit;
		uint32_t word;
	} reg_38; // 0x0098

	union {
		struct {
			unsigned slope_con_eng            : 16;     // bits : 15_0
			unsigned slope_hld_con_eng        : 16;     // bits : 31_16
		} bit;
		uint32_t word;
	} reg_39; // 0x009c

	union {
		struct {
			unsigned overshoot_en         : 1;      // bits : 0
			unsigned                      : 3;
			unsigned wt_overshoot         : 8;      // bits : 11_4
			unsigned                      : 4;
			unsigned wt_undershoot        : 8;      // bits : 23_16
		} bit;
		uint32_t word;
	} reg_40; // 0x00a0

	union {
		struct {
			unsigned th_overshoot             : 8;      // bits : 7_0
			unsigned th_undershoot            : 8;      // bits : 15_8
			unsigned th_undershoot_lum        : 8;      // bits : 23_16
			unsigned th_undershoot_eng        : 8;      // bits : 31_24
		} bit;
		uint32_t word;
	} reg_41; // 0x00a4

	union {
		struct {
			unsigned clamp_wt_mod_lum        : 8;       // bits : 7_0
			unsigned clamp_wt_mod_eng        : 8;       // bits : 15_8
			unsigned strength_lum_eng        : 8;       // bits : 23_16
			unsigned norm_lum_eng            : 8;       // bits : 31_24
		} bit;
		uint32_t word;
	} reg_42; // 0x00a8

	union {
		struct {
			unsigned slope_overshoot         : 15;      // bits : 14_0
			unsigned                         : 1;
			unsigned slope_undershoot        : 15;      // bits : 30_16
		} bit;
		uint32_t word;
	} reg_43; // 0x00ac

	union {
		struct {
			unsigned slope_undershoot_lum        : 15;      // bits : 14_0
			unsigned                             : 1;
			unsigned slope_undershoot_eng        : 15;      // bits : 30_16
		} bit;
		uint32_t word;
	} reg_44; // 0x00b0

	union {
		struct {
			unsigned esthrl        : 10;        // bits : 9_0
			unsigned esthrh        : 10;        // bits : 19_10
			unsigned establ        : 3;     // bits : 22_20
			unsigned               : 1;
			unsigned estabh        : 3;     // bits : 26_24
		} bit;
		uint32_t word;
	} reg_45; // 0x00b4

	union {
		struct {
			unsigned eslutl_0        : 8;        // bits : 7_0
			unsigned eslutl_1        : 8;        // bits : 15_8
			unsigned eslutl_2        : 8;        // bits : 23_16
			unsigned eslutl_3        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_46; // 0x00b8

	union {
		struct {
			unsigned eslutl_4        : 8;        // bits : 7_0
			unsigned eslutl_5        : 8;        // bits : 15_8
			unsigned eslutl_6        : 8;        // bits : 23_16
			unsigned eslutl_7        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_47; // 0x00bc

	union {
		struct {
			unsigned esluth_0        : 8;        // bits : 7_0
			unsigned esluth_1        : 8;        // bits : 15_8
			unsigned esluth_2        : 8;        // bits : 23_16
			unsigned esluth_3        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_48; // 0x00c0

	union {
		struct {
			unsigned esluth_4        : 8;        // bits : 7_0
			unsigned esluth_5        : 8;        // bits : 15_8
			unsigned esluth_6        : 8;        // bits : 23_16
			unsigned esluth_7        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_49; // 0x00c4

	union {
		struct {
			unsigned edthrl         : 10;       // bits : 9_0
			unsigned edthrh         : 10;       // bits : 19_10
			unsigned edtabl         : 3;        // bits : 22_20
			unsigned                : 1;
			unsigned edtabh         : 3;        // bits : 26_24
			unsigned                : 1;
			unsigned edin_sel        : 2;        // bits : 29_28
		} bit;
		uint32_t word;
	} reg_50; // 0x00c8

	union {
		struct {
			unsigned edlutl_0        : 8;        // bits : 7_0
			unsigned edlutl_1        : 8;        // bits : 15_8
			unsigned edlutl_2        : 8;        // bits : 23_16
			unsigned edlutl_3        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_51; // 0x00cc

	union {
		struct {
			unsigned edlutl_4        : 8;        // bits : 7_0
			unsigned edlutl_5        : 8;        // bits : 15_8
			unsigned edlutl_6        : 8;        // bits : 23_16
			unsigned edlutl_7        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_52; // 0x00d0

	union {
		struct {
			unsigned edluth_0        : 8;        // bits : 7_0
			unsigned edluth_1        : 8;        // bits : 15_8
			unsigned edluth_2        : 8;        // bits : 23_16
			unsigned edluth_3        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_53; // 0x00d4

	union {
		struct {
			unsigned edluth_4        : 8;        // bits : 7_0
			unsigned edluth_5        : 8;        // bits : 15_8
			unsigned edluth_6        : 8;        // bits : 23_16
			unsigned edluth_7        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_54; // 0x00d8

	union {
		struct {
			unsigned localmax_statistics_max        : 10;       // bits : 9_0
			unsigned coneng_statistics_max          : 10;       // bits : 19_10
			unsigned coneng_statistics_avg          : 10;       // bits : 29_20
			unsigned reserved                       : 2;        // bits : 31_30
		} bit;
		uint32_t word;
	} reg_55; // 0x00dc

	union {
		struct {
			unsigned region_str_en        : 1;      // bits : 0
			unsigned                      : 15;
			unsigned enh_thin             : 8;      // bits : 23_16
			unsigned enh_robust           : 8;      // bits : 31_24
		} bit;
		uint32_t word;
	} reg_56; // 0x00e0

	union {
		struct {
			unsigned slope_flat        : 16;        // bits : 15_0
			unsigned slope_edge        : 16;        // bits : 31_16
		} bit;
		uint32_t word;
	} reg_57; // 0x00e4

	union {
		struct {
			unsigned str_flat        : 8;       // bits : 7_0
			unsigned str_edge        : 8;       // bits : 15_8
		} bit;
		uint32_t word;
	} reg_58; // 0x00e8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_59; // 0x00ec

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_60; // 0x00f0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_61; // 0x00f4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_62; // 0x00f8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_63; // 0x00fc

	union {
		struct {
			unsigned ccrange           : 2;     // bits : 1_0
			unsigned cc2_sel           : 1;     // bits : 2
			unsigned cc_gamsel         : 1;     // bits : 3
			unsigned ccstab_sel        : 2;     // bits : 5_4
			unsigned ccofs_sel         : 2;     // bits : 7_6
			unsigned                   : 8;
			unsigned coef_rr           : 12;        // bits : 27_16
		} bit;
		uint32_t word;
	} reg_64; // 0x0100

	union {
		struct {
			unsigned coef_rg        : 12;       // bits : 11_0
			unsigned                : 4;
			unsigned coef_rb        : 12;       // bits : 27_16
		} bit;
		uint32_t word;
	} reg_65; // 0x0104

	union {
		struct {
			unsigned coef_gr        : 12;       // bits : 11_0
			unsigned                : 4;
			unsigned coef_gg        : 12;       // bits : 27_16
		} bit;
		uint32_t word;
	} reg_66; // 0x0108

	union {
		struct {
			unsigned coef_gb        : 12;       // bits : 11_0
			unsigned                : 4;
			unsigned coef_br        : 12;       // bits : 27_16
		} bit;
		uint32_t word;
	} reg_67; // 0x010c

	union {
		struct {
			unsigned coef_bg        : 12;       // bits : 11_0
			unsigned                : 4;
			unsigned coef_bb        : 12;       // bits : 27_16
		} bit;
		uint32_t word;
	} reg_68; // 0x0110

	union {
		struct {
			unsigned fstab0        : 8;     // bits : 7_0
			unsigned fstab1        : 8;     // bits : 15_8
			unsigned fstab2        : 8;     // bits : 23_16
			unsigned fstab3        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_69; // 0x0114

	union {
		struct {
			unsigned fstab4        : 8;     // bits : 7_0
			unsigned fstab5        : 8;     // bits : 15_8
			unsigned fstab6        : 8;     // bits : 23_16
			unsigned fstab7        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_70; // 0x0118

	union {
		struct {
			unsigned fstab8         : 8;        // bits : 7_0
			unsigned fstab9         : 8;        // bits : 15_8
			unsigned fstab10        : 8;        // bits : 23_16
			unsigned fstab11        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_71; // 0x011c

	union {
		struct {
			unsigned fstab12        : 8;        // bits : 7_0
			unsigned fstab13        : 8;        // bits : 15_8
			unsigned fstab14        : 8;        // bits : 23_16
			unsigned fstab15        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_72; // 0x0120

	union {
		struct {
			unsigned fdtab0        : 8;     // bits : 7_0
			unsigned fdtab1        : 8;     // bits : 15_8
			unsigned fdtab2        : 8;     // bits : 23_16
			unsigned fdtab3        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_73; // 0x0124

	union {
		struct {
			unsigned fdtab4        : 8;     // bits : 7_0
			unsigned fdtab5        : 8;     // bits : 15_8
			unsigned fdtab6        : 8;     // bits : 23_16
			unsigned fdtab7        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_74; // 0x0128

	union {
		struct {
			unsigned fdtab8         : 8;        // bits : 7_0
			unsigned fdtab9         : 8;        // bits : 15_8
			unsigned fdtab10        : 8;        // bits : 23_16
			unsigned fdtab11        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_75; // 0x012c

	union {
		struct {
			unsigned fdtab12        : 8;        // bits : 7_0
			unsigned fdtab13        : 8;        // bits : 15_8
			unsigned fdtab14        : 8;        // bits : 23_16
			unsigned fdtab15        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_76; // 0x0130

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_77; // 0x0134

	union {
		struct {
			unsigned coef_yr         : 9;       // bits : 8_0
			unsigned coef_yg         : 9;       // bits : 17_9
			unsigned coef_yb         : 9;       // bits : 26_18
			unsigned cstp_rat        : 5;       // bits : 31_27
		} bit;
		uint32_t word;
	} reg_78; // 0x0138

	union {
		struct {
			unsigned coef_ur           : 9;     // bits : 8_0
			unsigned coef_ug           : 9;     // bits : 17_9
			unsigned coef_ub           : 9;     // bits : 26_18
			unsigned                   : 1;
			unsigned cstoff_sel        : 1;     // bits : 28
		} bit;
		uint32_t word;
	} reg_79; // 0x013c

	union {
		struct {
			unsigned coef_vr        : 9;        // bits : 8_0
			unsigned coef_vg        : 9;        // bits : 17_9
			unsigned coef_vb        : 9;        // bits : 26_18
		} bit;
		uint32_t word;
	} reg_80; // 0x0140

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_81; // 0x0144

	union {
		struct {
			unsigned int_ofs           : 8;     // bits : 7_0
			unsigned sat_ofs           : 8;     // bits : 15_8
			unsigned chue_roten        : 1;     // bits : 16
			unsigned chue_c2gen        : 1;     // bits : 17
			unsigned                   : 2;
			unsigned cctrl_sel         : 2;     // bits : 21_20
			unsigned                   : 2;
			unsigned vdet_div          : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_82; // 0x0148

	union {
		struct {
			unsigned chuem0        : 8;     // bits : 7_0
			unsigned chuem1        : 8;     // bits : 15_8
			unsigned chuem2        : 8;     // bits : 23_16
			unsigned chuem3        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_83; // 0x014c

	union {
		struct {
			unsigned chuem4        : 8;     // bits : 7_0
			unsigned chuem5        : 8;     // bits : 15_8
			unsigned chuem6        : 8;     // bits : 23_16
			unsigned chuem7        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_84; // 0x0150

	union {
		struct {
			unsigned chuem8         : 8;        // bits : 7_0
			unsigned chuem9         : 8;        // bits : 15_8
			unsigned chuem10        : 8;        // bits : 23_16
			unsigned chuem11        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_85; // 0x0154

	union {
		struct {
			unsigned chuem12        : 8;        // bits : 7_0
			unsigned chuem13        : 8;        // bits : 15_8
			unsigned chuem14        : 8;        // bits : 23_16
			unsigned chuem15        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_86; // 0x0158

	union {
		struct {
			unsigned chuem16        : 8;        // bits : 7_0
			unsigned chuem17        : 8;        // bits : 15_8
			unsigned chuem18        : 8;        // bits : 23_16
			unsigned chuem19        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_87; // 0x015c

	union {
		struct {
			unsigned chuem20        : 8;        // bits : 7_0
			unsigned chuem21        : 8;        // bits : 15_8
			unsigned chuem22        : 8;        // bits : 23_16
			unsigned chuem23        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_88; // 0x0160

	union {
		struct {
			unsigned cintm0        : 8;     // bits : 7_0
			unsigned cintm1        : 8;     // bits : 15_8
			unsigned cintm2        : 8;     // bits : 23_16
			unsigned cintm3        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_89; // 0x0164

	union {
		struct {
			unsigned cintm4        : 8;     // bits : 7_0
			unsigned cintm5        : 8;     // bits : 15_8
			unsigned cintm6        : 8;     // bits : 23_16
			unsigned cintm7        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_90; // 0x0168

	union {
		struct {
			unsigned cintm8         : 8;        // bits : 7_0
			unsigned cintm9         : 8;        // bits : 15_8
			unsigned cintm10        : 8;        // bits : 23_16
			unsigned cintm11        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_91; // 0x016c

	union {
		struct {
			unsigned cintm12        : 8;        // bits : 7_0
			unsigned cintm13        : 8;        // bits : 15_8
			unsigned cintm14        : 8;        // bits : 23_16
			unsigned cintm15        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_92; // 0x0170

	union {
		struct {
			unsigned cintm16        : 8;        // bits : 7_0
			unsigned cintm17        : 8;        // bits : 15_8
			unsigned cintm18        : 8;        // bits : 23_16
			unsigned cintm19        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_93; // 0x0174

	union {
		struct {
			unsigned cintm20        : 8;        // bits : 7_0
			unsigned cintm21        : 8;        // bits : 15_8
			unsigned cintm22        : 8;        // bits : 23_16
			unsigned cintm23        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_94; // 0x0178

	union {
		struct {
			unsigned csatm0        : 8;     // bits : 7_0
			unsigned csatm1        : 8;     // bits : 15_8
			unsigned csatm2        : 8;     // bits : 23_16
			unsigned csatm3        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_95; // 0x017c

	union {
		struct {
			unsigned csatm4        : 8;     // bits : 7_0
			unsigned csatm5        : 8;     // bits : 15_8
			unsigned csatm6        : 8;     // bits : 23_16
			unsigned csatm7        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_96; // 0x0180

	union {
		struct {
			unsigned csatm8         : 8;        // bits : 7_0
			unsigned csatm9         : 8;        // bits : 15_8
			unsigned csatm10        : 8;        // bits : 23_16
			unsigned csatm11        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_97; // 0x0184

	union {
		struct {
			unsigned csatm12        : 8;        // bits : 7_0
			unsigned csatm13        : 8;        // bits : 15_8
			unsigned csatm14        : 8;        // bits : 23_16
			unsigned csatm15        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_98; // 0x0188

	union {
		struct {
			unsigned csatm16        : 8;        // bits : 7_0
			unsigned csatm17        : 8;        // bits : 15_8
			unsigned csatm18        : 8;        // bits : 23_16
			unsigned csatm19        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_99; // 0x018c

	union {
		struct {
			unsigned csatm20        : 8;        // bits : 7_0
			unsigned csatm21        : 8;        // bits : 15_8
			unsigned csatm22        : 8;        // bits : 23_16
			unsigned csatm23        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_100; // 0x0190

	union {
		struct {
			unsigned cedgem0        : 8;     // bits : 7_0
			unsigned cedgem1        : 8;     // bits : 15_8
			unsigned cedgem2        : 8;     // bits : 23_16
			unsigned cedgem3        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_101; // 0x0194

	union {
		struct {
			unsigned cedgem4        : 8;     // bits : 7_0
			unsigned cedgem5        : 8;     // bits : 15_8
			unsigned cedgem6        : 8;     // bits : 23_16
			unsigned cedgem7        : 8;     // bits : 31_24
		} bit;
		uint32_t word;
	} reg_102; // 0x0198

	union {
		struct {
			unsigned cedgem8         : 8;        // bits : 7_0
			unsigned cedgem9         : 8;        // bits : 15_8
			unsigned cedgem10        : 8;        // bits : 23_16
			unsigned cedgem11        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_103; // 0x019c

	union {
		struct {
			unsigned cedgem12        : 8;        // bits : 7_0
			unsigned cedgem13        : 8;        // bits : 15_8
			unsigned cedgem14        : 8;        // bits : 23_16
			unsigned cedgem15        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_104; // 0x01a0

	union {
		struct {
			unsigned cedgem16        : 8;        // bits : 7_0
			unsigned cedgem17        : 8;        // bits : 15_8
			unsigned cedgem18        : 8;        // bits : 23_16
			unsigned cedgem19        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_105; // 0x01a4

	union {
		struct {
			unsigned cedgem20        : 8;        // bits : 7_0
			unsigned cedgem21        : 8;        // bits : 15_8
			unsigned cedgem22        : 8;        // bits : 23_16
			unsigned cedgem23        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_106; // 0x01a8

	union {
		struct {
			unsigned dds0        : 6;       // bits : 5_0
			unsigned dds1        : 6;       // bits : 11_6
			unsigned dds2        : 6;       // bits : 17_12
			unsigned dds3        : 6;       // bits : 23_18
		} bit;
		uint32_t word;
	} reg_107; // 0x01ac

	union {
		struct {
			unsigned dds4        : 6;       // bits : 5_0
			unsigned dds5        : 6;       // bits : 11_6
			unsigned dds6        : 6;       // bits : 17_12
			unsigned dds7        : 6;       // bits : 23_18
		} bit;
		uint32_t word;
	} reg_108; // 0x01b0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_109; // 0x01b4

	union {
		struct {
			unsigned y_enh_p         : 10;      // bits : 9_0
			unsigned y_enh_n         : 10;      // bits : 19_10
			unsigned                 : 10;
			unsigned y_einv_p        : 1;       // bits : 30
			unsigned y_einv_n        : 1;       // bits : 31
		} bit;
		uint32_t word;
	} reg_110; // 0x01b8

	union {
		struct {
			unsigned y_con               : 8;       // bits : 7_0
			unsigned yc_randreset        : 1;       // bits : 8
			unsigned y_rand_en           : 1;       // bits : 9
			unsigned                     : 2;
			unsigned y_rand              : 3;       // bits : 14_12
			unsigned                     : 1;
			unsigned y_ethy              : 10;      // bits : 25_16
		} bit;
		uint32_t word;
	} reg_111; // 0x01bc

	union {
		struct {
			unsigned y_yth1             : 8;        // bits : 7_0
			unsigned                    : 4;
			unsigned y_hit1sel          : 1;        // bits : 12
			unsigned y_nhit1sel         : 1;        // bits : 13
			unsigned                    : 2;
			unsigned y_fix1_hit         : 8;        // bits : 23_16
			unsigned y_fix1_nhit        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_112; // 0x01c0

	union {
		struct {
			unsigned y_yth2             : 8;        // bits : 7_0
			unsigned                    : 4;
			unsigned y_hit2sel          : 1;        // bits : 12
			unsigned y_nhit2sel         : 1;        // bits : 13
			unsigned                    : 2;
			unsigned y_fix2_hit         : 8;        // bits : 23_16
			unsigned y_fix2_nhit        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_113; // 0x01c4

	union {
		struct {
			unsigned ymask         : 8;     // bits : 7_0
			unsigned cbmask        : 8;     // bits : 15_8
			unsigned crmask        : 8;     // bits : 23_16
		} bit;
		uint32_t word;
	} reg_114; // 0x01c8

	union {
		struct {
			unsigned c_cbofs          : 8;      // bits : 7_0
			unsigned c_crofs          : 8;      // bits : 15_8
			unsigned c_con            : 8;      // bits : 23_16
			unsigned                  : 3;
			unsigned c_rand_en        : 1;      // bits : 27
			unsigned c_rand           : 3;      // bits : 30_28
		} bit;
		uint32_t word;
	} reg_115; // 0x01cc

	union {
		struct {
			unsigned cb_fix_hit         : 8;        // bits : 7_0
			unsigned cr_fix_hit         : 8;        // bits : 15_8
			unsigned cb_fix_nhit        : 8;        // bits : 23_16
			unsigned cr_fix_nhit        : 8;        // bits : 31_24
		} bit;
		uint32_t word;
	} reg_116; // 0x01d0

	union {
		struct {
			unsigned c_eth            : 10;     // bits : 9_0
			unsigned                  : 2;
			unsigned c_hitsel         : 1;      // bits : 12
			unsigned c_nhitsel        : 1;      // bits : 13
			unsigned                  : 2;
			unsigned c_yth_h          : 8;      // bits : 23_16
			unsigned c_yth_l          : 8;      // bits : 31_24
		} bit;
		uint32_t word;
	} reg_117; // 0x01d4

	union {
		struct {
			unsigned c_cbth_h        : 8;       // bits : 7_0
			unsigned c_cbth_l        : 8;       // bits : 15_8
			unsigned c_crth_h        : 8;       // bits : 23_16
			unsigned c_crth_l        : 8;       // bits : 31_24
		} bit;
		uint32_t word;
	} reg_118; // 0x01d8

	union {
		struct {
			unsigned ccontab0        : 10;      // bits : 9_0
			unsigned ccontab1        : 10;      // bits : 19_10
			unsigned ccontab2        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_119; // 0x01dc

	union {
		struct {
			unsigned ccontab3        : 10;      // bits : 9_0
			unsigned ccontab4        : 10;      // bits : 19_10
			unsigned ccontab5        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_120; // 0x01e0

	union {
		struct {
			unsigned ccontab6        : 10;      // bits : 9_0
			unsigned ccontab7        : 10;      // bits : 19_10
			unsigned ccontab8        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_121; // 0x01e4

	union {
		struct {
			unsigned ccontab9         : 10;     // bits : 9_0
			unsigned ccontab10        : 10;     // bits : 19_10
			unsigned ccontab11        : 10;     // bits : 29_20
		} bit;
		uint32_t word;
	} reg_122; // 0x01e8

	union {
		struct {
			unsigned ccontab12        : 10;     // bits : 9_0
			unsigned ccontab13        : 10;     // bits : 19_10
			unsigned ccontab14        : 10;     // bits : 29_20
		} bit;
		uint32_t word;
	} reg_123; // 0x01ec

	union {
		struct {
			unsigned ccontab15          : 10;       // bits : 9_0
			unsigned ccontab16          : 10;       // bits : 19_10
			unsigned                    : 11;
			unsigned ccontab_sel        : 1;        // bits : 31
		} bit;
		uint32_t word;
	} reg_124; // 0x01f0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_125; // 0x01f4

	union {
		struct {
			unsigned edge_dbg_modesel        : 2;       // bits : 1_0
		} bit;
		uint32_t word;
	} reg_126; // 0x01f8

	union {
		struct {
			unsigned ycurve_sel        : 2;     // bits : 1_0
		} bit;
		uint32_t word;
	} reg_127; // 0x01fc

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_128; // 0x0200

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_129; // 0x0204

	union {
		struct {
			unsigned gamyrand_en           : 1;     // bits : 0
			unsigned gamyrand_reset        : 1;     // bits : 1
			unsigned                       : 2;
			unsigned gamyrand_shf          : 1;     // bits : 4
		} bit;
		uint32_t word;
	} reg_130; // 0x0208

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_131; // 0x020c

	union {
		struct {
			unsigned                   : 1;
			unsigned eth_outfmt        : 1;     // bits : 1
			unsigned ethlow            : 10;        // bits : 11_2
			unsigned ethmid            : 10;        // bits : 21_12
			unsigned ethhigh           : 10;        // bits : 31_22
		} bit;
		uint32_t word;
	} reg_132; // 0x0210

	union {
		struct {
			unsigned hout_sel          : 1;     // bits : 0
			unsigned vout_sel          : 1;     // bits : 1
			unsigned eth_outsel        : 2;     // bits : 3_2
		} bit;
		uint32_t word;
	} reg_133; // 0x0214

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_134; // 0x0218

	union {
		struct {
			unsigned vdet_gh1_a              : 5;     // bits : 4_0
			unsigned                       : 2;
			unsigned vdet_gh1_bcd_op        : 1;     // bits : 7
			unsigned vdet_gh1_b              : 5;     // bits : 12_8
			unsigned                       : 3;
			unsigned vdet_gh1_c              : 4;     // bits : 19_16
			unsigned vdet_gh1_d              : 4;     // bits : 23_20
			unsigned vdet_gh1_fsize         : 2;     // bits : 25_24
			unsigned                       : 2;
			unsigned vdet_gh1_div          : 4;     // bits : 31_28
		} bit;
		uint32_t word;
	} reg_135; // 0x021c

	union {
		struct {
			unsigned vdet_gv1_a              : 5;     // bits : 4_0
			unsigned                       : 2;
			unsigned vdet_gv1_bcd_op        : 1;     // bits : 7
			unsigned vdet_gv1_b              : 5;     // bits : 12_8
			unsigned                       : 3;
			unsigned vdet_gv1_c              : 4;     // bits : 19_16
			unsigned vdet_gv1_d              : 4;     // bits : 23_20
			unsigned vdet_gv1_fsize         : 2;     // bits : 25_24
			unsigned                       : 2;
			unsigned vdet_gv1_div          : 4;     // bits : 31_28
		} bit;
		uint32_t word;
	} reg_136; // 0x0220

	union {
		struct {
			unsigned vdet_gh2_a              : 5;     // bits : 4_0
			unsigned                       : 2;
			unsigned vdet_gh2_bcd_op        : 1;     // bits : 7
			unsigned vdet_gh2_b              : 5;     // bits : 12_8
			unsigned                       : 3;
			unsigned vdet_gh2_c              : 4;     // bits : 19_16
			unsigned vdet_gh2_d              : 4;     // bits : 23_20
			unsigned vdet_gh2_fsize         : 2;     // bits : 25_24
			unsigned                       : 2;
			unsigned vdet_gh2_div          : 4;     // bits : 31_28
		} bit;
		uint32_t word;
	} reg_137; // 0x0224

	union {
		struct {
			unsigned vdet_gv2_a              : 5;     // bits : 4_0
			unsigned                       : 2;
			unsigned vdet_gv2_bcd_op        : 1;     // bits : 7
			unsigned vdet_gv2_b              : 5;     // bits : 12_8
			unsigned                       : 3;
			unsigned vdet_gv2_c              : 4;     // bits : 19_16
			unsigned vdet_gv2_d              : 4;     // bits : 23_20
			unsigned vdet_gv2_fsize         : 2;     // bits : 25_24
			unsigned                       : 2;
			unsigned vdet_gv2_div          : 4;     // bits : 31_28
		} bit;
		uint32_t word;
	} reg_138; // 0x0228

	union {
		struct {
			unsigned vacc_outsel        : 1;        // bits : 0
			unsigned reserved           : 3;        // bits : 3_1
			unsigned va_stx             : 12;       // bits : 15_4
			unsigned va_sty             : 12;       // bits : 27_16
		} bit;
		uint32_t word;
	} reg_139; // 0x022c

	union {
		struct {
			unsigned va_g1h_thl        : 8;      // bits : 7_0
			unsigned va_g1h_thh        : 8;      // bits : 15_8
			unsigned va_g1v_thl        : 8;      // bits : 23_16
			unsigned va_g1v_thh        : 8;      // bits : 31_24
		} bit;
		uint32_t word;
	} reg_140; // 0x0230

	union {
		struct {
			unsigned va_g2h_thl        : 8;      // bits : 7_0
			unsigned va_g2h_thh        : 8;      // bits : 15_8
			unsigned va_g2v_thl        : 8;      // bits : 23_16
			unsigned va_g2v_thh        : 8;      // bits : 31_24
		} bit;
		uint32_t word;
	} reg_141; // 0x0234

	union {
		struct {
			unsigned va_win_szx                 : 9;        // bits : 8_0
			unsigned va_win_szy                 : 9;        // bits : 17_9
			unsigned va_g1_line_max_mode        : 1;        // bits : 18
			unsigned va_g2_line_max_mode        : 1;        // bits : 19
			unsigned va_g1_cnt_en               : 1;        // bits : 20
			unsigned va_g2_cnt_en               : 1;        // bits : 21
		} bit;
		uint32_t word;
	} reg_142; // 0x0238

	union {
		struct {
			unsigned va_win_numx         : 3;       // bits : 2_0
			unsigned                     : 1;
			unsigned va_win_numy         : 3;       // bits : 6_4
			unsigned                     : 9;
			unsigned va_win_skipx        : 6;       // bits : 21_16
			unsigned                     : 2;
			unsigned va_win_skipy        : 5;       // bits : 28_24
		} bit;
		uint32_t word;
	} reg_143; // 0x023c

	union {
		struct {
			unsigned win0_stx        : 12;      // bits : 11_0
			unsigned                 : 4;
			unsigned win0_sty        : 12;      // bits : 27_16
		} bit;
		uint32_t word;
	} reg_144; // 0x0240

	union {
		struct {
			unsigned win0_g1_line_max_mode        : 1;      // bits : 0
			unsigned win0_g2_line_max_mode        : 1;      // bits : 1
			unsigned                              : 2;
			unsigned win0_hsz                     : 9;      // bits : 12_4
			unsigned                              : 3;
			unsigned win0_vsz                     : 9;      // bits : 24_16
		} bit;
		uint32_t word;
	} reg_145; // 0x0244

	union {
		struct {
			unsigned win1_stx        : 12;      // bits : 11_0
			unsigned                 : 4;
			unsigned win1_sty        : 12;      // bits : 27_16
		} bit;
		uint32_t word;
	} reg_146; // 0x0248

	union {
		struct {
			unsigned win1_g1_line_max_mode        : 1;      // bits : 0
			unsigned win1_g2_line_max_mode        : 1;      // bits : 1
			unsigned                              : 2;
			unsigned win1_hsz                     : 9;      // bits : 12_4
			unsigned                              : 3;
			unsigned win1_vsz                     : 9;      // bits : 24_16
		} bit;
		uint32_t word;
	} reg_147; // 0x024c

	union {
		struct {
			unsigned win2_stx        : 12;      // bits : 11_0
			unsigned                 : 4;
			unsigned win2_sty        : 12;      // bits : 27_16
		} bit;
		uint32_t word;
	} reg_148; // 0x0250

	union {
		struct {
			unsigned win2_g1_line_max_mode        : 1;      // bits : 0
			unsigned win2_g2_line_max_mode        : 1;      // bits : 1
			unsigned                              : 2;
			unsigned win2_hsz                     : 9;      // bits : 12_4
			unsigned                              : 3;
			unsigned win2_vsz                     : 9;      // bits : 24_16
		} bit;
		uint32_t word;
	} reg_149; // 0x0254

	union {
		struct {
			unsigned win3_stx        : 12;      // bits : 11_0
			unsigned                 : 4;
			unsigned win3_sty        : 12;      // bits : 27_16
		} bit;
		uint32_t word;
	} reg_150; // 0x0258

	union {
		struct {
			unsigned win3_g1_line_max_mode        : 1;      // bits : 0
			unsigned win3_g2_line_max_mode        : 1;      // bits : 1
			unsigned                              : 2;
			unsigned win3_hsz                     : 9;      // bits : 12_4
			unsigned                              : 3;
			unsigned win3_vsz                     : 9;      // bits : 24_16
		} bit;
		uint32_t word;
	} reg_151; // 0x025c

	union {
		struct {
			unsigned win4_stx        : 12;      // bits : 11_0
			unsigned                 : 4;
			unsigned win4_sty        : 12;      // bits : 27_16
		} bit;
		uint32_t word;
	} reg_152; // 0x0260

	union {
		struct {
			unsigned win4_g1_line_max_mode        : 1;      // bits : 0
			unsigned win4_g2_line_max_mode        : 1;      // bits : 1
			unsigned                              : 2;
			unsigned win4_hsz                     : 9;      // bits : 12_4
			unsigned                              : 3;
			unsigned win4_vsz                     : 9;      // bits : 24_16
		} bit;
		uint32_t word;
	} reg_153; // 0x0264

	union {
		struct {
			unsigned va_win0g1h_vacc         : 16;      // bits : 15_0
			unsigned va_win0g1h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_154; // 0x0268

	union {
		struct {
			unsigned va_win0g1v_vacc         : 16;      // bits : 15_0
			unsigned va_win0g1v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_155; // 0x026c

	union {
		struct {
			unsigned va_win0g2h_vacc         : 16;      // bits : 15_0
			unsigned va_win0g2h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_156; // 0x0270

	union {
		struct {
			unsigned va_win0g2v_vacc         : 16;      // bits : 15_0
			unsigned va_win0g2v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_157; // 0x0274

	union {
		struct {
			unsigned va_win1g1h_vacc         : 16;      // bits : 15_0
			unsigned va_win1g1h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_158; // 0x0278

	union {
		struct {
			unsigned va_win1g1v_vacc         : 16;      // bits : 15_0
			unsigned va_win1g1v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_159; // 0x027c

	union {
		struct {
			unsigned va_win1g2h_vacc         : 16;      // bits : 15_0
			unsigned va_win1g2h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_160; // 0x0280

	union {
		struct {
			unsigned va_win1g2v_vacc         : 16;      // bits : 15_0
			unsigned va_win1g2v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_161; // 0x0284

	union {
		struct {
			unsigned va_win2g1h_vacc         : 16;      // bits : 15_0
			unsigned va_win2g1h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_162; // 0x0288

	union {
		struct {
			unsigned va_win2g1v_vacc         : 16;      // bits : 15_0
			unsigned va_win2g1v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_163; // 0x028c

	union {
		struct {
			unsigned va_win2g2h_vacc         : 16;      // bits : 15_0
			unsigned va_win2g2h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_164; // 0x0290

	union {
		struct {
			unsigned va_win2g2v_vacc         : 16;      // bits : 15_0
			unsigned va_win2g2v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_165; // 0x0294

	union {
		struct {
			unsigned va_win3g1h_vacc         : 16;      // bits : 15_0
			unsigned va_win3g1h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_166; // 0x0298

	union {
		struct {
			unsigned va_win3g1v_vacc         : 16;      // bits : 15_0
			unsigned va_win3g1v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_167; // 0x029c

	union {
		struct {
			unsigned va_win3g2h_vacc         : 16;      // bits : 15_0
			unsigned va_win3g2h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_168; // 0x02a0

	union {
		struct {
			unsigned va_win3g2v_vacc         : 16;      // bits : 15_0
			unsigned va_win3g2v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_169; // 0x02a4

	union {
		struct {
			unsigned va_win4g1h_vacc         : 16;      // bits : 15_0
			unsigned va_win4g1h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_170; // 0x02a8

	union {
		struct {
			unsigned va_win4g1v_vacc         : 16;      // bits : 15_0
			unsigned va_win4g1v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_171; // 0x02ac

	union {
		struct {
			unsigned va_win4g2h_vacc         : 16;      // bits : 15_0
			unsigned va_win4g2h_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_172; // 0x02b0

	union {
		struct {
			unsigned va_win4g2v_vacc         : 16;      // bits : 15_0
			unsigned va_win4g2v_vacnt        : 16;      // bits : 31_16
		} bit;
		uint32_t word;
	} reg_173; // 0x02b4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_174; // 0x02b8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_175; // 0x02bc

	union {
		struct {
			unsigned lce_subimg_width         : 5;      // bits : 4_0
			unsigned lce_subimg_height        : 5;      // bits : 9_5
		} bit;
		uint32_t word;
	} reg_176; // 0x02c0

	union {
		struct {
			unsigned                           : 2;
			unsigned lce_subimg_dramsai        : 30;        // bits : 31_2
		} bit;
		uint32_t word;
	} reg_177; // 0x02c4

	union {
		struct {
			unsigned                         : 2;
			unsigned lce_subimg_lofsi        : 14;      // bits : 15_2
		} bit;
		uint32_t word;
	} reg_178; // 0x02c8

	union {
		struct {
			unsigned                           : 2;
			unsigned lce_subimg_dramsao        : 30;        // bits : 31_2
		} bit;
		uint32_t word;
	} reg_179; // 0x02cc

	union {
		struct {
			unsigned                         : 2;
			unsigned lce_subimg_lofso        : 14;      // bits : 15_2
		} bit;
		uint32_t word;
	} reg_180; // 0x02d0

	union {
		struct {
			unsigned lce_subimg_hfactor        : 16;        // bits : 15_0
			unsigned lce_subimg_vfactor        : 16;        // bits : 31_16
		} bit;
		uint32_t word;
	} reg_181; // 0x02d4

	union {
		struct {
			unsigned lce_sub_blk_sizeh           : 10;      // bits : 9_0
			unsigned lce_blk_cent_hfactor        : 22;      // bits : 31_10
		} bit;
		uint32_t word;
	} reg_182; // 0x02d8

	union {
		struct {
			unsigned lce_sub_blk_sizev           : 10;      // bits : 9_0
			unsigned lce_blk_cent_vfactor        : 22;      // bits : 31_10
		} bit;
		uint32_t word;
	} reg_183; // 0x02dc

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_184; // 0x02e0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_185; // 0x02e4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_186; // 0x02e8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_187; // 0x02ec

	union {
		struct {
			unsigned lce_lpf_c0        : 3;     // bits : 2_0
			unsigned lce_lpf_c1        : 3;     // bits : 5_3
			unsigned lce_lpf_c2        : 3;     // bits : 8_6
		} bit;
		uint32_t word;
	} reg_188; // 0x02f0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_189; // 0x02f4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_190; // 0x02f8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_191; // 0x02fc

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_192; // 0x0300

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_193; // 0x0304

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_194; // 0x0308

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_195; // 0x030c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_196; // 0x0310

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_197; // 0x0314

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_198; // 0x0318

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_199; // 0x031c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_200; // 0x0320

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_201; // 0x0324

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_202; // 0x0328

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_203; // 0x032c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_204; // 0x0330

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_205; // 0x0334

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_206; // 0x0338

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_207; // 0x033c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_208; // 0x0340

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_209; // 0x0344

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_210; // 0x0348

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_211; // 0x034c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_212; // 0x0350

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_213; // 0x0354

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_214; // 0x0358

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_215; // 0x035c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_216; // 0x0360

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_217; // 0x0364

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_218; // 0x0368

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_219; // 0x036c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_220; // 0x0370

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_221; // 0x0374

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_222; // 0x0378

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_223; // 0x037c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_224; // 0x0380

	union {
		struct {
			unsigned lce_wt_diff_pos        : 8;        // bits : 7_0
			unsigned lce_wt_diff_neg        : 8;        // bits : 15_8
			unsigned lce_wt_diff_avg        : 8;        // bits : 23_16
		} bit;
		uint32_t word;
	} reg_225; // 0x0384

	union {
		struct {
			unsigned lce_lum_adj_lut0        : 8;       // bits : 7_0
			unsigned lce_lum_adj_lut1        : 8;       // bits : 15_8
			unsigned lce_lum_adj_lut2        : 8;       // bits : 23_16
			unsigned lce_lum_adj_lut3        : 8;       // bits : 31_24
		} bit;
		uint32_t word;
	} reg_226; // 0x0388

	union {
		struct {
			unsigned lce_lum_adj_lut4        : 8;       // bits : 7_0
			unsigned lce_lum_adj_lut5        : 8;       // bits : 15_8
			unsigned lce_lum_adj_lut6        : 8;       // bits : 23_16
			unsigned lce_lum_adj_lut7        : 8;       // bits : 31_24
		} bit;
		uint32_t word;
	} reg_227; // 0x038c

	union {
		struct {
			unsigned lce_lum_adj_lut8        : 8;       // bits : 7_0
		} bit;
		uint32_t word;
	} reg_228; // 0x0390

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_229; // 0x0394

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_230; // 0x0398

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_231; // 0x039c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_232; // 0x03a0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_233; // 0x03a4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_234; // 0x03a8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_235; // 0x03ac

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_236; // 0x03b0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_237; // 0x03b4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_238; // 0x03b8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_239; // 0x03bc

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_240; // 0x03c0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_241; // 0x03c4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_242; // 0x03c8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_243; // 0x03cc

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_244; // 0x03d0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_245; // 0x03d4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_246; // 0x03d8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_247; // 0x03dc

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_248; // 0x03e0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_249; // 0x03e4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_250; // 0x03e8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_251; // 0x03ec

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_252; // 0x03f0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_253; // 0x03f4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_254; // 0x03f8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_255; // 0x03fc

	union {
		struct {
			unsigned r_lut0        : 10;        // bits : 9_0
			unsigned r_lut1        : 10;        // bits : 19_10
			unsigned r_lut2        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_256; // 0x0400

	union {
		struct {
			unsigned r_lut2        : 10;        // bits : 9_0
			unsigned r_lut3        : 10;        // bits : 19_10
			unsigned r_lut4        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_257; // 0x0404

	union {
		struct {
			unsigned r_lut4        : 10;        // bits : 9_0
			unsigned r_lut5        : 10;        // bits : 19_10
			unsigned r_lut6        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_258; // 0x0408

	union {
		struct {
			unsigned r_lut6        : 10;        // bits : 9_0
			unsigned r_lut7        : 10;        // bits : 19_10
			unsigned r_lut8        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_259; // 0x040c

	union {
		struct {
			unsigned r_lut8         : 10;       // bits : 9_0
			unsigned r_lut9         : 10;       // bits : 19_10
			unsigned r_lut10        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_260; // 0x0410

	union {
		struct {
			unsigned r_lut10        : 10;       // bits : 9_0
			unsigned r_lut11        : 10;       // bits : 19_10
			unsigned r_lut12        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_261; // 0x0414

	union {
		struct {
			unsigned r_lut12        : 10;       // bits : 9_0
			unsigned r_lut13        : 10;       // bits : 19_10
			unsigned r_lut14        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_262; // 0x0418

	union {
		struct {
			unsigned r_lut14        : 10;       // bits : 9_0
			unsigned r_lut15        : 10;       // bits : 19_10
			unsigned r_lut16        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_263; // 0x041c

	union {
		struct {
			unsigned r_lut16        : 10;       // bits : 9_0
			unsigned r_lut17        : 10;       // bits : 19_10
			unsigned r_lut18        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_264; // 0x0420

	union {
		struct {
			unsigned r_lut18        : 10;       // bits : 9_0
			unsigned r_lut19        : 10;       // bits : 19_10
			unsigned r_lut20        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_265; // 0x0424

	union {
		struct {
			unsigned r_lut20        : 10;       // bits : 9_0
			unsigned r_lut21        : 10;       // bits : 19_10
			unsigned r_lut22        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_266; // 0x0428

	union {
		struct {
			unsigned r_lut22        : 10;       // bits : 9_0
			unsigned r_lut23        : 10;       // bits : 19_10
			unsigned r_lut24        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_267; // 0x042c

	union {
		struct {
			unsigned r_lut24        : 10;       // bits : 9_0
			unsigned r_lut25        : 10;       // bits : 19_10
			unsigned r_lut26        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_268; // 0x0430

	union {
		struct {
			unsigned r_lut26        : 10;       // bits : 9_0
			unsigned r_lut27        : 10;       // bits : 19_10
			unsigned r_lut28        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_269; // 0x0434

	union {
		struct {
			unsigned r_lut28        : 10;       // bits : 9_0
			unsigned r_lut29        : 10;       // bits : 19_10
			unsigned r_lut30        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_270; // 0x0438

	union {
		struct {
			unsigned r_lut30        : 10;       // bits : 9_0
			unsigned r_lut31        : 10;       // bits : 19_10
			unsigned r_lut32        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_271; // 0x043c

	union {
		struct {
			unsigned r_lut32        : 10;       // bits : 9_0
			unsigned r_lut33        : 10;       // bits : 19_10
			unsigned r_lut34        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_272; // 0x0440

	union {
		struct {
			unsigned r_lut34        : 10;       // bits : 9_0
			unsigned r_lut35        : 10;       // bits : 19_10
			unsigned r_lut36        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_273; // 0x0444

	union {
		struct {
			unsigned r_lut36        : 10;       // bits : 9_0
			unsigned r_lut37        : 10;       // bits : 19_10
			unsigned r_lut38        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_274; // 0x0448

	union {
		struct {
			unsigned r_lut38        : 10;       // bits : 9_0
			unsigned r_lut39        : 10;       // bits : 19_10
			unsigned r_lut40        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_275; // 0x044c

	union {
		struct {
			unsigned r_lut40        : 10;       // bits : 9_0
			unsigned r_lut41        : 10;       // bits : 19_10
			unsigned r_lut42        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_276; // 0x0450

	union {
		struct {
			unsigned r_lut42        : 10;       // bits : 9_0
			unsigned r_lut43        : 10;       // bits : 19_10
			unsigned r_lut44        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_277; // 0x0454

	union {
		struct {
			unsigned r_lut44        : 10;       // bits : 9_0
			unsigned r_lut45        : 10;       // bits : 19_10
			unsigned r_lut46        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_278; // 0x0458

	union {
		struct {
			unsigned r_lut46        : 10;       // bits : 9_0
			unsigned r_lut47        : 10;       // bits : 19_10
			unsigned r_lut48        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_279; // 0x045c

	union {
		struct {
			unsigned r_lut48        : 10;       // bits : 9_0
			unsigned r_lut49        : 10;       // bits : 19_10
			unsigned r_lut50        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_280; // 0x0460

	union {
		struct {
			unsigned r_lut50        : 10;       // bits : 9_0
			unsigned r_lut51        : 10;       // bits : 19_10
			unsigned r_lut52        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_281; // 0x0464

	union {
		struct {
			unsigned r_lut52        : 10;       // bits : 9_0
			unsigned r_lut53        : 10;       // bits : 19_10
			unsigned r_lut54        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_282; // 0x0468

	union {
		struct {
			unsigned r_lut54        : 10;       // bits : 9_0
			unsigned r_lut55        : 10;       // bits : 19_10
			unsigned r_lut56        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_283; // 0x046c

	union {
		struct {
			unsigned r_lut56        : 10;       // bits : 9_0
			unsigned r_lut57        : 10;       // bits : 19_10
			unsigned r_lut58        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_284; // 0x0470

	union {
		struct {
			unsigned r_lut58        : 10;       // bits : 9_0
			unsigned r_lut59        : 10;       // bits : 19_10
			unsigned r_lut60        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_285; // 0x0474

	union {
		struct {
			unsigned r_lut60        : 10;       // bits : 9_0
			unsigned r_lut61        : 10;       // bits : 19_10
			unsigned r_lut62        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_286; // 0x0478

	union {
		struct {
			unsigned r_lut62        : 10;       // bits : 9_0
			unsigned r_lut63        : 10;       // bits : 19_10
			unsigned r_lut64        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_287; // 0x047c

	union {
		struct {
			unsigned r_lut64        : 10;       // bits : 9_0
			unsigned r_lut65        : 10;       // bits : 19_10
			unsigned r_lut66        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_288; // 0x0480

	union {
		struct {
			unsigned r_lut66        : 10;       // bits : 9_0
			unsigned r_lut67        : 10;       // bits : 19_10
			unsigned r_lut68        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_289; // 0x0484

	union {
		struct {
			unsigned r_lut68        : 10;       // bits : 9_0
			unsigned r_lut69        : 10;       // bits : 19_10
			unsigned r_lut70        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_290; // 0x0488

	union {
		struct {
			unsigned r_lut70        : 10;       // bits : 9_0
			unsigned r_lut71        : 10;       // bits : 19_10
			unsigned r_lut72        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_291; // 0x048c

	union {
		struct {
			unsigned r_lut72        : 10;       // bits : 9_0
			unsigned r_lut73        : 10;       // bits : 19_10
			unsigned r_lut74        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_292; // 0x0490

	union {
		struct {
			unsigned r_lut74        : 10;       // bits : 9_0
			unsigned r_lut75        : 10;       // bits : 19_10
			unsigned r_lut76        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_293; // 0x0494

	union {
		struct {
			unsigned r_lut76        : 10;       // bits : 9_0
			unsigned r_lut77        : 10;       // bits : 19_10
			unsigned r_lut78        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_294; // 0x0498

	union {
		struct {
			unsigned r_lut78        : 10;       // bits : 9_0
			unsigned r_lut79        : 10;       // bits : 19_10
			unsigned r_lut80        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_295; // 0x049c

	union {
		struct {
			unsigned r_lut80        : 10;       // bits : 9_0
			unsigned r_lut81        : 10;       // bits : 19_10
			unsigned r_lut82        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_296; // 0x04a0

	union {
		struct {
			unsigned r_lut82        : 10;       // bits : 9_0
			unsigned r_lut83        : 10;       // bits : 19_10
			unsigned r_lut84        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_297; // 0x04a4

	union {
		struct {
			unsigned r_lut84        : 10;       // bits : 9_0
			unsigned r_lut85        : 10;       // bits : 19_10
			unsigned r_lut86        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_298; // 0x04a8

	union {
		struct {
			unsigned r_lut86        : 10;       // bits : 9_0
			unsigned r_lut87        : 10;       // bits : 19_10
			unsigned r_lut88        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_299; // 0x04ac

	union {
		struct {
			unsigned r_lut88        : 10;       // bits : 9_0
			unsigned r_lut89        : 10;       // bits : 19_10
			unsigned r_lut90        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_300; // 0x04b0

	union {
		struct {
			unsigned r_lut90        : 10;       // bits : 9_0
			unsigned r_lut91        : 10;       // bits : 19_10
			unsigned r_lut92        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_301; // 0x04b4

	union {
		struct {
			unsigned r_lut92        : 10;       // bits : 9_0
			unsigned r_lut93        : 10;       // bits : 19_10
			unsigned r_lut94        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_302; // 0x04b8

	union {
		struct {
			unsigned r_lut94        : 10;       // bits : 9_0
			unsigned r_lut95        : 10;       // bits : 19_10
			unsigned r_lut96        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_303; // 0x04bc

	union {
		struct {
			unsigned r_lut96        : 10;       // bits : 9_0
			unsigned r_lut97        : 10;       // bits : 19_10
			unsigned r_lut98        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_304; // 0x04c0

	union {
		struct {
			unsigned r_lut98         : 10;      // bits : 9_0
			unsigned r_lut99         : 10;      // bits : 19_10
			unsigned r_lut100        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_305; // 0x04c4

	union {
		struct {
			unsigned r_lut100        : 10;      // bits : 9_0
			unsigned r_lut101        : 10;      // bits : 19_10
			unsigned r_lut102        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_306; // 0x04c8

	union {
		struct {
			unsigned r_lut102        : 10;      // bits : 9_0
			unsigned r_lut103        : 10;      // bits : 19_10
			unsigned r_lut104        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_307; // 0x04cc

	union {
		struct {
			unsigned r_lut104        : 10;      // bits : 9_0
			unsigned r_lut105        : 10;      // bits : 19_10
			unsigned r_lut106        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_308; // 0x04d0

	union {
		struct {
			unsigned r_lut106        : 10;      // bits : 9_0
			unsigned r_lut107        : 10;      // bits : 19_10
			unsigned r_lut108        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_309; // 0x04d4

	union {
		struct {
			unsigned r_lut108        : 10;      // bits : 9_0
			unsigned r_lut109        : 10;      // bits : 19_10
			unsigned r_lut110        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_310; // 0x04d8

	union {
		struct {
			unsigned r_lut110        : 10;      // bits : 9_0
			unsigned r_lut111        : 10;      // bits : 19_10
			unsigned r_lut112        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_311; // 0x04dc

	union {
		struct {
			unsigned r_lut112        : 10;      // bits : 9_0
			unsigned r_lut113        : 10;      // bits : 19_10
			unsigned r_lut114        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_312; // 0x04e0

	union {
		struct {
			unsigned r_lut114        : 10;      // bits : 9_0
			unsigned r_lut115        : 10;      // bits : 19_10
			unsigned r_lut116        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_313; // 0x04e4

	union {
		struct {
			unsigned r_lut116        : 10;      // bits : 9_0
			unsigned r_lut117        : 10;      // bits : 19_10
			unsigned r_lut118        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_314; // 0x04e8

	union {
		struct {
			unsigned r_lut118        : 10;      // bits : 9_0
			unsigned r_lut119        : 10;      // bits : 19_10
			unsigned r_lut120        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_315; // 0x04ec

	union {
		struct {
			unsigned r_lut120        : 10;      // bits : 9_0
			unsigned r_lut121        : 10;      // bits : 19_10
			unsigned r_lut122        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_316; // 0x04f0

	union {
		struct {
			unsigned r_lut122        : 10;      // bits : 9_0
			unsigned r_lut123        : 10;      // bits : 19_10
			unsigned r_lut124        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_317; // 0x04f4

	union {
		struct {
			unsigned r_lut124        : 10;      // bits : 9_0
			unsigned r_lut125        : 10;      // bits : 19_10
			unsigned r_lut126        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_318; // 0x04f8

	union {
		struct {
			unsigned r_lut126        : 10;      // bits : 9_0
			unsigned r_lut127        : 10;      // bits : 19_10
			unsigned r_lut128        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_319; // 0x04fc

	union {
		struct {
			unsigned g_lut0        : 10;        // bits : 9_0
			unsigned g_lut1        : 10;        // bits : 19_10
			unsigned g_lut2        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_320; // 0x0500

	union {
		struct {
			unsigned g_lut2        : 10;        // bits : 9_0
			unsigned g_lut3        : 10;        // bits : 19_10
			unsigned g_lut4        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_321; // 0x0504

	union {
		struct {
			unsigned g_lut4        : 10;        // bits : 9_0
			unsigned g_lut5        : 10;        // bits : 19_10
			unsigned g_lut6        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_322; // 0x0508

	union {
		struct {
			unsigned g_lut6        : 10;        // bits : 9_0
			unsigned g_lut7        : 10;        // bits : 19_10
			unsigned g_lut8        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_323; // 0x050c

	union {
		struct {
			unsigned g_lut8         : 10;       // bits : 9_0
			unsigned g_lut9         : 10;       // bits : 19_10
			unsigned g_lut10        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_324; // 0x0510

	union {
		struct {
			unsigned g_lut10        : 10;       // bits : 9_0
			unsigned g_lut11        : 10;       // bits : 19_10
			unsigned g_lut12        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_325; // 0x0514

	union {
		struct {
			unsigned g_lut12        : 10;       // bits : 9_0
			unsigned g_lut13        : 10;       // bits : 19_10
			unsigned g_lut14        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_326; // 0x0518

	union {
		struct {
			unsigned g_lut14        : 10;       // bits : 9_0
			unsigned g_lut15        : 10;       // bits : 19_10
			unsigned g_lut16        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_327; // 0x051c

	union {
		struct {
			unsigned g_lut16        : 10;       // bits : 9_0
			unsigned g_lut17        : 10;       // bits : 19_10
			unsigned g_lut18        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_328; // 0x0520

	union {
		struct {
			unsigned g_lut18        : 10;       // bits : 9_0
			unsigned g_lut19        : 10;       // bits : 19_10
			unsigned g_lut20        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_329; // 0x0524

	union {
		struct {
			unsigned g_lut20        : 10;       // bits : 9_0
			unsigned g_lut21        : 10;       // bits : 19_10
			unsigned g_lut22        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_330; // 0x0528

	union {
		struct {
			unsigned g_lut22        : 10;       // bits : 9_0
			unsigned g_lut23        : 10;       // bits : 19_10
			unsigned g_lut24        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_331; // 0x052c

	union {
		struct {
			unsigned g_lut24        : 10;       // bits : 9_0
			unsigned g_lut25        : 10;       // bits : 19_10
			unsigned g_lut26        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_332; // 0x0530

	union {
		struct {
			unsigned g_lut26        : 10;       // bits : 9_0
			unsigned g_lut27        : 10;       // bits : 19_10
			unsigned g_lut28        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_333; // 0x0534

	union {
		struct {
			unsigned g_lut28        : 10;       // bits : 9_0
			unsigned g_lut29        : 10;       // bits : 19_10
			unsigned g_lut30        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_334; // 0x0538

	union {
		struct {
			unsigned g_lut30        : 10;       // bits : 9_0
			unsigned g_lut31        : 10;       // bits : 19_10
			unsigned g_lut32        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_335; // 0x053c

	union {
		struct {
			unsigned g_lut32        : 10;       // bits : 9_0
			unsigned g_lut33        : 10;       // bits : 19_10
			unsigned g_lut34        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_336; // 0x0540

	union {
		struct {
			unsigned g_lut34        : 10;       // bits : 9_0
			unsigned g_lut35        : 10;       // bits : 19_10
			unsigned g_lut36        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_337; // 0x0544

	union {
		struct {
			unsigned g_lut36        : 10;       // bits : 9_0
			unsigned g_lut37        : 10;       // bits : 19_10
			unsigned g_lut38        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_338; // 0x0548

	union {
		struct {
			unsigned g_lut38        : 10;       // bits : 9_0
			unsigned g_lut39        : 10;       // bits : 19_10
			unsigned g_lut40        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_339; // 0x054c

	union {
		struct {
			unsigned g_lut40        : 10;       // bits : 9_0
			unsigned g_lut41        : 10;       // bits : 19_10
			unsigned g_lut42        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_340; // 0x0550

	union {
		struct {
			unsigned g_lut42        : 10;       // bits : 9_0
			unsigned g_lut43        : 10;       // bits : 19_10
			unsigned g_lut44        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_341; // 0x0554

	union {
		struct {
			unsigned g_lut44        : 10;       // bits : 9_0
			unsigned g_lut45        : 10;       // bits : 19_10
			unsigned g_lut46        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_342; // 0x0558

	union {
		struct {
			unsigned g_lut46        : 10;       // bits : 9_0
			unsigned g_lut47        : 10;       // bits : 19_10
			unsigned g_lut48        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_343; // 0x055c

	union {
		struct {
			unsigned g_lut48        : 10;       // bits : 9_0
			unsigned g_lut49        : 10;       // bits : 19_10
			unsigned g_lut50        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_344; // 0x0560

	union {
		struct {
			unsigned g_lut50        : 10;       // bits : 9_0
			unsigned g_lut51        : 10;       // bits : 19_10
			unsigned g_lut52        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_345; // 0x0564

	union {
		struct {
			unsigned g_lut52        : 10;       // bits : 9_0
			unsigned g_lut53        : 10;       // bits : 19_10
			unsigned g_lut54        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_346; // 0x0568

	union {
		struct {
			unsigned g_lut54        : 10;       // bits : 9_0
			unsigned g_lut55        : 10;       // bits : 19_10
			unsigned g_lut56        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_347; // 0x056c

	union {
		struct {
			unsigned g_lut56        : 10;       // bits : 9_0
			unsigned g_lut57        : 10;       // bits : 19_10
			unsigned g_lut58        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_348; // 0x0570

	union {
		struct {
			unsigned g_lut58        : 10;       // bits : 9_0
			unsigned g_lut59        : 10;       // bits : 19_10
			unsigned g_lut60        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_349; // 0x0574

	union {
		struct {
			unsigned g_lut60        : 10;       // bits : 9_0
			unsigned g_lut61        : 10;       // bits : 19_10
			unsigned g_lut62        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_350; // 0x0578

	union {
		struct {
			unsigned g_lut62        : 10;       // bits : 9_0
			unsigned g_lut63        : 10;       // bits : 19_10
			unsigned g_lut64        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_351; // 0x057c

	union {
		struct {
			unsigned g_lut64        : 10;       // bits : 9_0
			unsigned g_lut65        : 10;       // bits : 19_10
			unsigned g_lut66        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_352; // 0x0580

	union {
		struct {
			unsigned g_lut66        : 10;       // bits : 9_0
			unsigned g_lut67        : 10;       // bits : 19_10
			unsigned g_lut68        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_353; // 0x0584

	union {
		struct {
			unsigned g_lut68        : 10;       // bits : 9_0
			unsigned g_lut69        : 10;       // bits : 19_10
			unsigned g_lut70        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_354; // 0x0588

	union {
		struct {
			unsigned g_lut70        : 10;       // bits : 9_0
			unsigned g_lut71        : 10;       // bits : 19_10
			unsigned g_lut72        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_355; // 0x058c

	union {
		struct {
			unsigned g_lut72        : 10;       // bits : 9_0
			unsigned g_lut73        : 10;       // bits : 19_10
			unsigned g_lut74        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_356; // 0x0590

	union {
		struct {
			unsigned g_lut74        : 10;       // bits : 9_0
			unsigned g_lut75        : 10;       // bits : 19_10
			unsigned g_lut76        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_357; // 0x0594

	union {
		struct {
			unsigned g_lut76        : 10;       // bits : 9_0
			unsigned g_lut77        : 10;       // bits : 19_10
			unsigned g_lut78        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_358; // 0x0598

	union {
		struct {
			unsigned g_lut78        : 10;       // bits : 9_0
			unsigned g_lut79        : 10;       // bits : 19_10
			unsigned g_lut80        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_359; // 0x059c

	union {
		struct {
			unsigned g_lut80        : 10;       // bits : 9_0
			unsigned g_lut81        : 10;       // bits : 19_10
			unsigned g_lut82        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_360; // 0x05a0

	union {
		struct {
			unsigned g_lut82        : 10;       // bits : 9_0
			unsigned g_lut83        : 10;       // bits : 19_10
			unsigned g_lut84        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_361; // 0x05a4

	union {
		struct {
			unsigned g_lut84        : 10;       // bits : 9_0
			unsigned g_lut85        : 10;       // bits : 19_10
			unsigned g_lut86        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_362; // 0x05a8

	union {
		struct {
			unsigned g_lut86        : 10;       // bits : 9_0
			unsigned g_lut87        : 10;       // bits : 19_10
			unsigned g_lut88        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_363; // 0x05ac

	union {
		struct {
			unsigned g_lut88        : 10;       // bits : 9_0
			unsigned g_lut89        : 10;       // bits : 19_10
			unsigned g_lut90        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_364; // 0x05b0

	union {
		struct {
			unsigned g_lut90        : 10;       // bits : 9_0
			unsigned g_lut91        : 10;       // bits : 19_10
			unsigned g_lut92        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_365; // 0x05b4

	union {
		struct {
			unsigned g_lut92        : 10;       // bits : 9_0
			unsigned g_lut93        : 10;       // bits : 19_10
			unsigned g_lut94        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_366; // 0x05b8

	union {
		struct {
			unsigned g_lut94        : 10;       // bits : 9_0
			unsigned g_lut95        : 10;       // bits : 19_10
			unsigned g_lut96        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_367; // 0x05bc

	union {
		struct {
			unsigned g_lut96        : 10;       // bits : 9_0
			unsigned g_lut97        : 10;       // bits : 19_10
			unsigned g_lut98        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_368; // 0x05c0

	union {
		struct {
			unsigned g_lut98         : 10;      // bits : 9_0
			unsigned g_lut99         : 10;      // bits : 19_10
			unsigned g_lut100        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_369; // 0x05c4

	union {
		struct {
			unsigned g_lut100        : 10;      // bits : 9_0
			unsigned g_lut101        : 10;      // bits : 19_10
			unsigned g_lut102        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_370; // 0x05c8

	union {
		struct {
			unsigned g_lut102        : 10;      // bits : 9_0
			unsigned g_lut103        : 10;      // bits : 19_10
			unsigned g_lut104        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_371; // 0x05cc

	union {
		struct {
			unsigned g_lut104        : 10;      // bits : 9_0
			unsigned g_lut105        : 10;      // bits : 19_10
			unsigned g_lut106        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_372; // 0x05d0

	union {
		struct {
			unsigned g_lut106        : 10;      // bits : 9_0
			unsigned g_lut107        : 10;      // bits : 19_10
			unsigned g_lut108        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_373; // 0x05d4

	union {
		struct {
			unsigned g_lut108        : 10;      // bits : 9_0
			unsigned g_lut109        : 10;      // bits : 19_10
			unsigned g_lut110        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_374; // 0x05d8

	union {
		struct {
			unsigned g_lut110        : 10;      // bits : 9_0
			unsigned g_lut111        : 10;      // bits : 19_10
			unsigned g_lut112        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_375; // 0x05dc

	union {
		struct {
			unsigned g_lut112        : 10;      // bits : 9_0
			unsigned g_lut113        : 10;      // bits : 19_10
			unsigned g_lut114        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_376; // 0x05e0

	union {
		struct {
			unsigned g_lut114        : 10;      // bits : 9_0
			unsigned g_lut115        : 10;      // bits : 19_10
			unsigned g_lut116        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_377; // 0x05e4

	union {
		struct {
			unsigned g_lut116        : 10;      // bits : 9_0
			unsigned g_lut117        : 10;      // bits : 19_10
			unsigned g_lut118        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_378; // 0x05e8

	union {
		struct {
			unsigned g_lut118        : 10;      // bits : 9_0
			unsigned g_lut119        : 10;      // bits : 19_10
			unsigned g_lut120        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_379; // 0x05ec

	union {
		struct {
			unsigned g_lut120        : 10;      // bits : 9_0
			unsigned g_lut121        : 10;      // bits : 19_10
			unsigned g_lut122        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_380; // 0x05f0

	union {
		struct {
			unsigned g_lut122        : 10;      // bits : 9_0
			unsigned g_lut123        : 10;      // bits : 19_10
			unsigned g_lut124        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_381; // 0x05f4

	union {
		struct {
			unsigned g_lut124        : 10;      // bits : 9_0
			unsigned g_lut125        : 10;      // bits : 19_10
			unsigned g_lut126        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_382; // 0x05f8

	union {
		struct {
			unsigned g_lut126        : 10;      // bits : 9_0
			unsigned g_lut127        : 10;      // bits : 19_10
			unsigned g_lut128        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_383; // 0x05fc

	union {
		struct {
			unsigned b_lut0        : 10;        // bits : 9_0
			unsigned b_lut1        : 10;        // bits : 19_10
			unsigned b_lut2        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_384; // 0x0600

	union {
		struct {
			unsigned b_lut2        : 10;        // bits : 9_0
			unsigned b_lut3        : 10;        // bits : 19_10
			unsigned b_lut4        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_385; // 0x0604

	union {
		struct {
			unsigned b_lut4        : 10;        // bits : 9_0
			unsigned b_lut5        : 10;        // bits : 19_10
			unsigned b_lut6        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_386; // 0x0608

	union {
		struct {
			unsigned b_lut6        : 10;        // bits : 9_0
			unsigned b_lut7        : 10;        // bits : 19_10
			unsigned b_lut8        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_387; // 0x060c

	union {
		struct {
			unsigned b_lut8         : 10;       // bits : 9_0
			unsigned b_lut9         : 10;       // bits : 19_10
			unsigned b_lut10        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_388; // 0x0610

	union {
		struct {
			unsigned b_lut10        : 10;       // bits : 9_0
			unsigned b_lut11        : 10;       // bits : 19_10
			unsigned b_lut12        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_389; // 0x0614

	union {
		struct {
			unsigned b_lut12        : 10;       // bits : 9_0
			unsigned b_lut13        : 10;       // bits : 19_10
			unsigned b_lut14        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_390; // 0x0618

	union {
		struct {
			unsigned b_lut14        : 10;       // bits : 9_0
			unsigned b_lut15        : 10;       // bits : 19_10
			unsigned b_lut16        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_391; // 0x061c

	union {
		struct {
			unsigned b_lut16        : 10;       // bits : 9_0
			unsigned b_lut17        : 10;       // bits : 19_10
			unsigned b_lut18        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_392; // 0x0620

	union {
		struct {
			unsigned b_lut18        : 10;       // bits : 9_0
			unsigned b_lut19        : 10;       // bits : 19_10
			unsigned b_lut20        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_393; // 0x0624

	union {
		struct {
			unsigned b_lut20        : 10;       // bits : 9_0
			unsigned b_lut21        : 10;       // bits : 19_10
			unsigned b_lut22        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_394; // 0x0628

	union {
		struct {
			unsigned b_lut22        : 10;       // bits : 9_0
			unsigned b_lut23        : 10;       // bits : 19_10
			unsigned b_lut24        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_395; // 0x062c

	union {
		struct {
			unsigned b_lut24        : 10;       // bits : 9_0
			unsigned b_lut25        : 10;       // bits : 19_10
			unsigned b_lut26        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_396; // 0x0630

	union {
		struct {
			unsigned b_lut26        : 10;       // bits : 9_0
			unsigned b_lut27        : 10;       // bits : 19_10
			unsigned b_lut28        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_397; // 0x0634

	union {
		struct {
			unsigned b_lut28        : 10;       // bits : 9_0
			unsigned b_lut29        : 10;       // bits : 19_10
			unsigned b_lut30        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_398; // 0x0638

	union {
		struct {
			unsigned b_lut30        : 10;       // bits : 9_0
			unsigned b_lut31        : 10;       // bits : 19_10
			unsigned b_lut32        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_399; // 0x063c

	union {
		struct {
			unsigned b_lut32        : 10;       // bits : 9_0
			unsigned b_lut33        : 10;       // bits : 19_10
			unsigned b_lut34        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_400; // 0x0640

	union {
		struct {
			unsigned b_lut34        : 10;       // bits : 9_0
			unsigned b_lut35        : 10;       // bits : 19_10
			unsigned b_lut36        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_401; // 0x0644

	union {
		struct {
			unsigned b_lut36        : 10;       // bits : 9_0
			unsigned b_lut37        : 10;       // bits : 19_10
			unsigned b_lut38        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_402; // 0x0648

	union {
		struct {
			unsigned b_lut38        : 10;       // bits : 9_0
			unsigned b_lut39        : 10;       // bits : 19_10
			unsigned b_lut40        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_403; // 0x064c

	union {
		struct {
			unsigned b_lut40        : 10;       // bits : 9_0
			unsigned b_lut41        : 10;       // bits : 19_10
			unsigned b_lut42        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_404; // 0x0650

	union {
		struct {
			unsigned b_lut42        : 10;       // bits : 9_0
			unsigned b_lut43        : 10;       // bits : 19_10
			unsigned b_lut44        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_405; // 0x0654

	union {
		struct {
			unsigned b_lut44        : 10;       // bits : 9_0
			unsigned b_lut45        : 10;       // bits : 19_10
			unsigned b_lut46        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_406; // 0x0658

	union {
		struct {
			unsigned b_lut46        : 10;       // bits : 9_0
			unsigned b_lut47        : 10;       // bits : 19_10
			unsigned b_lut48        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_407; // 0x065c

	union {
		struct {
			unsigned b_lut48        : 10;       // bits : 9_0
			unsigned b_lut49        : 10;       // bits : 19_10
			unsigned b_lut50        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_408; // 0x0660

	union {
		struct {
			unsigned b_lut50        : 10;       // bits : 9_0
			unsigned b_lut51        : 10;       // bits : 19_10
			unsigned b_lut52        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_409; // 0x0664

	union {
		struct {
			unsigned b_lut52        : 10;       // bits : 9_0
			unsigned b_lut53        : 10;       // bits : 19_10
			unsigned b_lut54        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_410; // 0x0668

	union {
		struct {
			unsigned b_lut54        : 10;       // bits : 9_0
			unsigned b_lut55        : 10;       // bits : 19_10
			unsigned b_lut56        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_411; // 0x066c

	union {
		struct {
			unsigned b_lut56        : 10;       // bits : 9_0
			unsigned b_lut57        : 10;       // bits : 19_10
			unsigned b_lut58        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_412; // 0x0670

	union {
		struct {
			unsigned b_lut58        : 10;       // bits : 9_0
			unsigned b_lut59        : 10;       // bits : 19_10
			unsigned b_lut60        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_413; // 0x0674

	union {
		struct {
			unsigned b_lut60        : 10;       // bits : 9_0
			unsigned b_lut61        : 10;       // bits : 19_10
			unsigned b_lut62        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_414; // 0x0678

	union {
		struct {
			unsigned b_lut62        : 10;       // bits : 9_0
			unsigned b_lut63        : 10;       // bits : 19_10
			unsigned b_lut64        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_415; // 0x067c

	union {
		struct {
			unsigned b_lut64        : 10;       // bits : 9_0
			unsigned b_lut65        : 10;       // bits : 19_10
			unsigned b_lut66        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_416; // 0x0680

	union {
		struct {
			unsigned b_lut66        : 10;       // bits : 9_0
			unsigned b_lut67        : 10;       // bits : 19_10
			unsigned b_lut68        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_417; // 0x0684

	union {
		struct {
			unsigned b_lut68        : 10;       // bits : 9_0
			unsigned b_lut69        : 10;       // bits : 19_10
			unsigned b_lut70        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_418; // 0x0688

	union {
		struct {
			unsigned b_lut70        : 10;       // bits : 9_0
			unsigned b_lut71        : 10;       // bits : 19_10
			unsigned b_lut72        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_419; // 0x068c

	union {
		struct {
			unsigned b_lut72        : 10;       // bits : 9_0
			unsigned b_lut73        : 10;       // bits : 19_10
			unsigned b_lut74        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_420; // 0x0690

	union {
		struct {
			unsigned b_lut74        : 10;       // bits : 9_0
			unsigned b_lut75        : 10;       // bits : 19_10
			unsigned b_lut76        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_421; // 0x0694

	union {
		struct {
			unsigned b_lut76        : 10;       // bits : 9_0
			unsigned b_lut77        : 10;       // bits : 19_10
			unsigned b_lut78        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_422; // 0x0698

	union {
		struct {
			unsigned b_lut78        : 10;       // bits : 9_0
			unsigned b_lut79        : 10;       // bits : 19_10
			unsigned b_lut80        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_423; // 0x069c

	union {
		struct {
			unsigned b_lut80        : 10;       // bits : 9_0
			unsigned b_lut81        : 10;       // bits : 19_10
			unsigned b_lut82        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_424; // 0x06a0

	union {
		struct {
			unsigned b_lut82        : 10;       // bits : 9_0
			unsigned b_lut83        : 10;       // bits : 19_10
			unsigned b_lut84        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_425; // 0x06a4

	union {
		struct {
			unsigned b_lut84        : 10;       // bits : 9_0
			unsigned b_lut85        : 10;       // bits : 19_10
			unsigned b_lut86        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_426; // 0x06a8

	union {
		struct {
			unsigned b_lut86        : 10;       // bits : 9_0
			unsigned b_lut87        : 10;       // bits : 19_10
			unsigned b_lut88        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_427; // 0x06ac

	union {
		struct {
			unsigned b_lut88        : 10;       // bits : 9_0
			unsigned b_lut89        : 10;       // bits : 19_10
			unsigned b_lut90        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_428; // 0x06b0

	union {
		struct {
			unsigned b_lut90        : 10;       // bits : 9_0
			unsigned b_lut91        : 10;       // bits : 19_10
			unsigned b_lut92        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_429; // 0x06b4

	union {
		struct {
			unsigned b_lut92        : 10;       // bits : 9_0
			unsigned b_lut93        : 10;       // bits : 19_10
			unsigned b_lut94        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_430; // 0x06b8

	union {
		struct {
			unsigned b_lut94        : 10;       // bits : 9_0
			unsigned b_lut95        : 10;       // bits : 19_10
			unsigned b_lut96        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_431; // 0x06bc

	union {
		struct {
			unsigned b_lut96        : 10;       // bits : 9_0
			unsigned b_lut97        : 10;       // bits : 19_10
			unsigned b_lut98        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_432; // 0x06c0

	union {
		struct {
			unsigned b_lut98         : 10;      // bits : 9_0
			unsigned b_lut99         : 10;      // bits : 19_10
			unsigned b_lut100        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_433; // 0x06c4

	union {
		struct {
			unsigned b_lut100        : 10;      // bits : 9_0
			unsigned b_lut101        : 10;      // bits : 19_10
			unsigned b_lut102        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_434; // 0x06c8

	union {
		struct {
			unsigned b_lut102        : 10;      // bits : 9_0
			unsigned b_lut103        : 10;      // bits : 19_10
			unsigned b_lut104        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_435; // 0x06cc

	union {
		struct {
			unsigned b_lut104        : 10;      // bits : 9_0
			unsigned b_lut105        : 10;      // bits : 19_10
			unsigned b_lut106        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_436; // 0x06d0

	union {
		struct {
			unsigned b_lut106        : 10;      // bits : 9_0
			unsigned b_lut107        : 10;      // bits : 19_10
			unsigned b_lut108        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_437; // 0x06d4

	union {
		struct {
			unsigned b_lut108        : 10;      // bits : 9_0
			unsigned b_lut109        : 10;      // bits : 19_10
			unsigned b_lut110        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_438; // 0x06d8

	union {
		struct {
			unsigned b_lut110        : 10;      // bits : 9_0
			unsigned b_lut111        : 10;      // bits : 19_10
			unsigned b_lut112        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_439; // 0x06dc

	union {
		struct {
			unsigned b_lut112        : 10;      // bits : 9_0
			unsigned b_lut113        : 10;      // bits : 19_10
			unsigned b_lut114        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_440; // 0x06e0

	union {
		struct {
			unsigned b_lut114        : 10;      // bits : 9_0
			unsigned b_lut115        : 10;      // bits : 19_10
			unsigned b_lut116        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_441; // 0x06e4

	union {
		struct {
			unsigned b_lut116        : 10;      // bits : 9_0
			unsigned b_lut117        : 10;      // bits : 19_10
			unsigned b_lut118        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_442; // 0x06e8

	union {
		struct {
			unsigned b_lut118        : 10;      // bits : 9_0
			unsigned b_lut119        : 10;      // bits : 19_10
			unsigned b_lut120        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_443; // 0x06ec

	union {
		struct {
			unsigned b_lut120        : 10;      // bits : 9_0
			unsigned b_lut121        : 10;      // bits : 19_10
			unsigned b_lut122        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_444; // 0x06f0

	union {
		struct {
			unsigned b_lut122        : 10;      // bits : 9_0
			unsigned b_lut123        : 10;      // bits : 19_10
			unsigned b_lut124        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_445; // 0x06f4

	union {
		struct {
			unsigned b_lut124        : 10;      // bits : 9_0
			unsigned b_lut125        : 10;      // bits : 19_10
			unsigned b_lut126        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_446; // 0x06f8

	union {
		struct {
			unsigned b_lut126        : 10;      // bits : 9_0
			unsigned b_lut127        : 10;      // bits : 19_10
			unsigned b_lut128        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_447; // 0x06fc

	union {
		struct {
			unsigned y_lut0        : 10;        // bits : 9_0
			unsigned y_lut1        : 10;        // bits : 19_10
			unsigned y_lut2        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_448; // 0x0700

	union {
		struct {
			unsigned y_lut2        : 10;        // bits : 9_0
			unsigned y_lut3        : 10;        // bits : 19_10
			unsigned y_lut4        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_449; // 0x0704

	union {
		struct {
			unsigned y_lut4        : 10;        // bits : 9_0
			unsigned y_lut5        : 10;        // bits : 19_10
			unsigned y_lut6        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_450; // 0x0708

	union {
		struct {
			unsigned y_lut6        : 10;        // bits : 9_0
			unsigned y_lut7        : 10;        // bits : 19_10
			unsigned y_lut8        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_451; // 0x070c

	union {
		struct {
			unsigned y_lut8         : 10;       // bits : 9_0
			unsigned y_lut9         : 10;       // bits : 19_10
			unsigned y_lut10        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_452; // 0x0710

	union {
		struct {
			unsigned y_lut10        : 10;       // bits : 9_0
			unsigned y_lut11        : 10;       // bits : 19_10
			unsigned y_lut12        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_453; // 0x0714

	union {
		struct {
			unsigned y_lut12        : 10;       // bits : 9_0
			unsigned y_lut13        : 10;       // bits : 19_10
			unsigned y_lut14        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_454; // 0x0718

	union {
		struct {
			unsigned y_lut14        : 10;       // bits : 9_0
			unsigned y_lut15        : 10;       // bits : 19_10
			unsigned y_lut16        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_455; // 0x071c

	union {
		struct {
			unsigned y_lut16        : 10;       // bits : 9_0
			unsigned y_lut17        : 10;       // bits : 19_10
			unsigned y_lut18        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_456; // 0x0720

	union {
		struct {
			unsigned y_lut18        : 10;       // bits : 9_0
			unsigned y_lut19        : 10;       // bits : 19_10
			unsigned y_lut20        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_457; // 0x0724

	union {
		struct {
			unsigned y_lut20        : 10;       // bits : 9_0
			unsigned y_lut21        : 10;       // bits : 19_10
			unsigned y_lut22        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_458; // 0x0728

	union {
		struct {
			unsigned y_lut22        : 10;       // bits : 9_0
			unsigned y_lut23        : 10;       // bits : 19_10
			unsigned y_lut24        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_459; // 0x072c

	union {
		struct {
			unsigned y_lut24        : 10;       // bits : 9_0
			unsigned y_lut25        : 10;       // bits : 19_10
			unsigned y_lut26        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_460; // 0x0730

	union {
		struct {
			unsigned y_lut26        : 10;       // bits : 9_0
			unsigned y_lut27        : 10;       // bits : 19_10
			unsigned y_lut28        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_461; // 0x0734

	union {
		struct {
			unsigned y_lut28        : 10;       // bits : 9_0
			unsigned y_lut29        : 10;       // bits : 19_10
			unsigned y_lut30        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_462; // 0x0738

	union {
		struct {
			unsigned y_lut30        : 10;       // bits : 9_0
			unsigned y_lut31        : 10;       // bits : 19_10
			unsigned y_lut32        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_463; // 0x073c

	union {
		struct {
			unsigned y_lut32        : 10;       // bits : 9_0
			unsigned y_lut33        : 10;       // bits : 19_10
			unsigned y_lut34        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_464; // 0x0740

	union {
		struct {
			unsigned y_lut34        : 10;       // bits : 9_0
			unsigned y_lut35        : 10;       // bits : 19_10
			unsigned y_lut36        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_465; // 0x0744

	union {
		struct {
			unsigned y_lut36        : 10;       // bits : 9_0
			unsigned y_lut37        : 10;       // bits : 19_10
			unsigned y_lut38        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_466; // 0x0748

	union {
		struct {
			unsigned y_lut38        : 10;       // bits : 9_0
			unsigned y_lut39        : 10;       // bits : 19_10
			unsigned y_lut40        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_467; // 0x074c

	union {
		struct {
			unsigned y_lut40        : 10;       // bits : 9_0
			unsigned y_lut41        : 10;       // bits : 19_10
			unsigned y_lut42        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_468; // 0x0750

	union {
		struct {
			unsigned y_lut42        : 10;       // bits : 9_0
			unsigned y_lut43        : 10;       // bits : 19_10
			unsigned y_lut44        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_469; // 0x0754

	union {
		struct {
			unsigned y_lut44        : 10;       // bits : 9_0
			unsigned y_lut45        : 10;       // bits : 19_10
			unsigned y_lut46        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_470; // 0x0758

	union {
		struct {
			unsigned y_lut46        : 10;       // bits : 9_0
			unsigned y_lut47        : 10;       // bits : 19_10
			unsigned y_lut48        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_471; // 0x075c

	union {
		struct {
			unsigned y_lut48        : 10;       // bits : 9_0
			unsigned y_lut49        : 10;       // bits : 19_10
			unsigned y_lut50        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_472; // 0x0760

	union {
		struct {
			unsigned y_lut50        : 10;       // bits : 9_0
			unsigned y_lut51        : 10;       // bits : 19_10
			unsigned y_lut52        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_473; // 0x0764

	union {
		struct {
			unsigned y_lut52        : 10;       // bits : 9_0
			unsigned y_lut53        : 10;       // bits : 19_10
			unsigned y_lut54        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_474; // 0x0768

	union {
		struct {
			unsigned y_lut54        : 10;       // bits : 9_0
			unsigned y_lut55        : 10;       // bits : 19_10
			unsigned y_lut56        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_475; // 0x076c

	union {
		struct {
			unsigned y_lut56        : 10;       // bits : 9_0
			unsigned y_lut57        : 10;       // bits : 19_10
			unsigned y_lut58        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_476; // 0x0770

	union {
		struct {
			unsigned y_lut58        : 10;       // bits : 9_0
			unsigned y_lut59        : 10;       // bits : 19_10
			unsigned y_lut60        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_477; // 0x0774

	union {
		struct {
			unsigned y_lut60        : 10;       // bits : 9_0
			unsigned y_lut61        : 10;       // bits : 19_10
			unsigned y_lut62        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_478; // 0x0778

	union {
		struct {
			unsigned y_lut62        : 10;       // bits : 9_0
			unsigned y_lut63        : 10;       // bits : 19_10
			unsigned y_lut64        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_479; // 0x077c

	union {
		struct {
			unsigned y_lut64        : 10;       // bits : 9_0
			unsigned y_lut65        : 10;       // bits : 19_10
			unsigned y_lut66        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_480; // 0x0780

	union {
		struct {
			unsigned y_lut66        : 10;       // bits : 9_0
			unsigned y_lut67        : 10;       // bits : 19_10
			unsigned y_lut68        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_481; // 0x0784

	union {
		struct {
			unsigned y_lut68        : 10;       // bits : 9_0
			unsigned y_lut69        : 10;       // bits : 19_10
			unsigned y_lut70        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_482; // 0x0788

	union {
		struct {
			unsigned y_lut70        : 10;       // bits : 9_0
			unsigned y_lut71        : 10;       // bits : 19_10
			unsigned y_lut72        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_483; // 0x078c

	union {
		struct {
			unsigned y_lut72        : 10;       // bits : 9_0
			unsigned y_lut73        : 10;       // bits : 19_10
			unsigned y_lut74        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_484; // 0x0790

	union {
		struct {
			unsigned y_lut74        : 10;       // bits : 9_0
			unsigned y_lut75        : 10;       // bits : 19_10
			unsigned y_lut76        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_485; // 0x0794

	union {
		struct {
			unsigned y_lut76        : 10;       // bits : 9_0
			unsigned y_lut77        : 10;       // bits : 19_10
			unsigned y_lut78        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_486; // 0x0798

	union {
		struct {
			unsigned y_lut78        : 10;       // bits : 9_0
			unsigned y_lut79        : 10;       // bits : 19_10
			unsigned y_lut80        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_487; // 0x079c

	union {
		struct {
			unsigned y_lut80        : 10;       // bits : 9_0
			unsigned y_lut81        : 10;       // bits : 19_10
			unsigned y_lut82        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_488; // 0x07a0

	union {
		struct {
			unsigned y_lut82        : 10;       // bits : 9_0
			unsigned y_lut83        : 10;       // bits : 19_10
			unsigned y_lut84        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_489; // 0x07a4

	union {
		struct {
			unsigned y_lut84        : 10;       // bits : 9_0
			unsigned y_lut85        : 10;       // bits : 19_10
			unsigned y_lut86        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_490; // 0x07a8

	union {
		struct {
			unsigned y_lut86        : 10;       // bits : 9_0
			unsigned y_lut87        : 10;       // bits : 19_10
			unsigned y_lut88        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_491; // 0x07ac

	union {
		struct {
			unsigned y_lut88        : 10;       // bits : 9_0
			unsigned y_lut89        : 10;       // bits : 19_10
			unsigned y_lut90        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_492; // 0x07b0

	union {
		struct {
			unsigned y_lut90        : 10;       // bits : 9_0
			unsigned y_lut91        : 10;       // bits : 19_10
			unsigned y_lut92        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_493; // 0x07b4

	union {
		struct {
			unsigned y_lut92        : 10;       // bits : 9_0
			unsigned y_lut93        : 10;       // bits : 19_10
			unsigned y_lut94        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_494; // 0x07b8

	union {
		struct {
			unsigned y_lut94        : 10;       // bits : 9_0
			unsigned y_lut95        : 10;       // bits : 19_10
			unsigned y_lut96        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_495; // 0x07bc

	union {
		struct {
			unsigned y_lut96        : 10;       // bits : 9_0
			unsigned y_lut97        : 10;       // bits : 19_10
			unsigned y_lut98        : 10;       // bits : 29_20
		} bit;
		uint32_t word;
	} reg_496; // 0x07c0

	union {
		struct {
			unsigned y_lut98         : 10;      // bits : 9_0
			unsigned y_lut99         : 10;      // bits : 19_10
			unsigned y_lut100        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_497; // 0x07c4

	union {
		struct {
			unsigned y_lut100        : 10;      // bits : 9_0
			unsigned y_lut101        : 10;      // bits : 19_10
			unsigned y_lut102        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_498; // 0x07c8

	union {
		struct {
			unsigned y_lut102        : 10;      // bits : 9_0
			unsigned y_lut103        : 10;      // bits : 19_10
			unsigned y_lut104        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_499; // 0x07cc

	union {
		struct {
			unsigned y_lut104        : 10;      // bits : 9_0
			unsigned y_lut105        : 10;      // bits : 19_10
			unsigned y_lut106        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_500; // 0x07d0

	union {
		struct {
			unsigned y_lut106        : 10;      // bits : 9_0
			unsigned y_lut107        : 10;      // bits : 19_10
			unsigned y_lut108        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_501; // 0x07d4

	union {
		struct {
			unsigned y_lut108        : 10;      // bits : 9_0
			unsigned y_lut109        : 10;      // bits : 19_10
			unsigned y_lut110        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_502; // 0x07d8

	union {
		struct {
			unsigned y_lut110        : 10;      // bits : 9_0
			unsigned y_lut111        : 10;      // bits : 19_10
			unsigned y_lut112        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_503; // 0x07dc

	union {
		struct {
			unsigned y_lut112        : 10;      // bits : 9_0
			unsigned y_lut113        : 10;      // bits : 19_10
			unsigned y_lut114        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_504; // 0x07e0

	union {
		struct {
			unsigned y_lut114        : 10;      // bits : 9_0
			unsigned y_lut115        : 10;      // bits : 19_10
			unsigned y_lut116        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_505; // 0x07e4

	union {
		struct {
			unsigned y_lut116        : 10;      // bits : 9_0
			unsigned y_lut117        : 10;      // bits : 19_10
			unsigned y_lut118        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_506; // 0x07e8

	union {
		struct {
			unsigned y_lut118        : 10;      // bits : 9_0
			unsigned y_lut119        : 10;      // bits : 19_10
			unsigned y_lut120        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_507; // 0x07ec

	union {
		struct {
			unsigned y_lut120        : 10;      // bits : 9_0
			unsigned y_lut121        : 10;      // bits : 19_10
			unsigned y_lut122        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_508; // 0x07f0

	union {
		struct {
			unsigned y_lut122        : 10;      // bits : 9_0
			unsigned y_lut123        : 10;      // bits : 19_10
			unsigned y_lut124        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_509; // 0x07f4

	union {
		struct {
			unsigned y_lut124        : 10;      // bits : 9_0
			unsigned y_lut125        : 10;      // bits : 19_10
			unsigned y_lut126        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_510; // 0x07f8

	union {
		struct {
			unsigned y_lut126        : 10;      // bits : 9_0
			unsigned y_lut127        : 10;      // bits : 19_10
			unsigned y_lut128        : 10;      // bits : 29_20
		} bit;
		uint32_t word;
	} reg_511; // 0x07fc

	union {
		struct {
			unsigned edge_lut0        : 10;     // bits : 9_0
			unsigned edge_lut1        : 10;     // bits : 19_10
			unsigned edge_lut2        : 10;     // bits : 29_20
		} bit;
		uint32_t word;
	} reg_512; // 0x0800

	union {
		struct {
			unsigned edge_lut3        : 10;     // bits : 9_0
			unsigned edge_lut4        : 10;     // bits : 19_10
			unsigned edge_lut5        : 10;     // bits : 29_20
		} bit;
		uint32_t word;
	} reg_513; // 0x0804

	union {
		struct {
			unsigned edge_lut6        : 10;     // bits : 9_0
			unsigned edge_lut7        : 10;     // bits : 19_10
			unsigned edge_lut8        : 10;     // bits : 29_20
		} bit;
		uint32_t word;
	} reg_514; // 0x0808

	union {
		struct {
			unsigned edge_lut9         : 10;        // bits : 9_0
			unsigned edge_lut10        : 10;        // bits : 19_10
			unsigned edge_lut11        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_515; // 0x080c

	union {
		struct {
			unsigned edge_lut12        : 10;        // bits : 9_0
			unsigned edge_lut13        : 10;        // bits : 19_10
			unsigned edge_lut14        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_516; // 0x0810

	union {
		struct {
			unsigned edge_lut15        : 10;        // bits : 9_0
			unsigned edge_lut16        : 10;        // bits : 19_10
			unsigned edge_lut17        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_517; // 0x0814

	union {
		struct {
			unsigned edge_lut18        : 10;        // bits : 9_0
			unsigned edge_lut19        : 10;        // bits : 19_10
			unsigned edge_lut20        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_518; // 0x0818

	union {
		struct {
			unsigned edge_lut21        : 10;        // bits : 9_0
			unsigned edge_lut22        : 10;        // bits : 19_10
			unsigned edge_lut23        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_519; // 0x081c

	union {
		struct {
			unsigned edge_lut24        : 10;        // bits : 9_0
			unsigned edge_lut25        : 10;        // bits : 19_10
			unsigned edge_lut26        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_520; // 0x0820

	union {
		struct {
			unsigned edge_lut27        : 10;        // bits : 9_0
			unsigned edge_lut28        : 10;        // bits : 19_10
			unsigned edge_lut29        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_521; // 0x0824

	union {
		struct {
			unsigned edge_lut30        : 10;        // bits : 9_0
			unsigned edge_lut31        : 10;        // bits : 19_10
			unsigned edge_lut32        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_522; // 0x0828

	union {
		struct {
			unsigned edge_lut33        : 10;        // bits : 9_0
			unsigned edge_lut34        : 10;        // bits : 19_10
			unsigned edge_lut35        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_523; // 0x082c

	union {
		struct {
			unsigned edge_lut36        : 10;        // bits : 9_0
			unsigned edge_lut37        : 10;        // bits : 19_10
			unsigned edge_lut38        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_524; // 0x0830

	union {
		struct {
			unsigned edge_lut39        : 10;        // bits : 9_0
			unsigned edge_lut40        : 10;        // bits : 19_10
			unsigned edge_lut41        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_525; // 0x0834

	union {
		struct {
			unsigned edge_lut42        : 10;        // bits : 9_0
			unsigned edge_lut43        : 10;        // bits : 19_10
			unsigned edge_lut44        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_526; // 0x0838

	union {
		struct {
			unsigned edge_lut45        : 10;        // bits : 9_0
			unsigned edge_lut46        : 10;        // bits : 19_10
			unsigned edge_lut47        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_527; // 0x083c

	union {
		struct {
			unsigned edge_lut48        : 10;        // bits : 9_0
			unsigned edge_lut49        : 10;        // bits : 19_10
			unsigned edge_lut50        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_528; // 0x0840

	union {
		struct {
			unsigned edge_lut51        : 10;        // bits : 9_0
			unsigned edge_lut52        : 10;        // bits : 19_10
			unsigned edge_lut53        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_529; // 0x0844

	union {
		struct {
			unsigned edge_lut54        : 10;        // bits : 9_0
			unsigned edge_lut55        : 10;        // bits : 19_10
			unsigned edge_lut56        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_530; // 0x0848

	union {
		struct {
			unsigned edge_lut57        : 10;        // bits : 9_0
			unsigned edge_lut58        : 10;        // bits : 19_10
			unsigned edge_lut59        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_531; // 0x084c

	union {
		struct {
			unsigned edge_lut60        : 10;        // bits : 9_0
			unsigned edge_lut61        : 10;        // bits : 19_10
			unsigned edge_lut62        : 10;        // bits : 29_20
		} bit;
		uint32_t word;
	} reg_532; // 0x0850

	union {
		struct {
			unsigned edge_lut63        : 10;        // bits : 9_0
			unsigned edge_lut64        : 10;        // bits : 19_10
		} bit;
		uint32_t word;
	} reg_533; // 0x0854

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_534; // 0x0858

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_535; // 0x085c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_536; // 0x0860

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_537; // 0x0864

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_538; // 0x0868

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_539; // 0x086c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_540; // 0x0870

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_541; // 0x0874

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_542; // 0x0878

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_543; // 0x087c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_544; // 0x0880

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_545; // 0x0884

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_546; // 0x0888

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_547; // 0x088c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_548; // 0x0890

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_549; // 0x0894

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_550; // 0x0898

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_551; // 0x089c

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_552; // 0x08a0

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_553; // 0x08a4

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_554; // 0x08a8

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_555; // 0x08ac

	union {
		struct {
			unsigned reserved        : 32;      // bits : 31_0
		} bit;
		uint32_t word;
	} reg_556; // 0x08b0

} NT98560_IPE_REGISTER_STRUCT;


#endif
