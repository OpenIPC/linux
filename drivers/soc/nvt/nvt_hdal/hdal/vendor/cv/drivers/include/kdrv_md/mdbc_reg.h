#ifndef _MDBC_NT98528_H_
#define _MDBC_NT98528_H_


#include "md_platform.h"

/*
    MDBC_SW_RST:    [0x0, 0x1],			bits : 0
    MDBC_START :    [0x0, 0x1],			bits : 1
    LL_FIRE    :    [0x0, 0x1],			bits : 28
*/
#define MDBC_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(MDBC_CONTROL_REGISTER)
    REGDEF_BIT(MDBC_SW_RST,        1)
    REGDEF_BIT(MDBC_START ,        1)
    REGDEF_BIT(           ,        26)
    REGDEF_BIT(LL_FIRE    ,        1)
REGDEF_END(MDBC_CONTROL_REGISTER)


/*
    MDBC_MODE       :    [0x0, 0x1],			bits : 0
    BC_UPDATE_NEI_EN:    [0x0, 0x1],			bits : 4
    BC_DEGHOST_EN   :    [0x0, 0x1],			bits : 8
    ROI_EN0         :    [0x0, 0x1],			bits : 12
    ROI_EN1         :    [0x0, 0x1],			bits : 13
    ROI_EN2         :    [0x0, 0x1],			bits : 14
    ROI_EN3         :    [0x0, 0x1],			bits : 15
    ROI_EN4         :    [0x0, 0x1],			bits : 16
    ROI_EN5         :    [0x0, 0x1],			bits : 17
    ROI_EN6         :    [0x0, 0x1],			bits : 18
    ROI_EN7         :    [0x0, 0x1],			bits : 19
    CHKSUM_EN       :    [0x0, 0x1],			bits : 20
    BGMW_SAVE_BW_EN :    [0x0, 0x1],			bits : 24
    BC_Y_ONLY_EN    :    [0x0, 0x1],			bits : 28
*/
#define MDBC_MODE_REGISTER0_OFS 0x0004
REGDEF_BEGIN(MDBC_MODE_REGISTER0)
    REGDEF_BIT(MDBC_MODE       ,        1)
    REGDEF_BIT(                ,        3)
    REGDEF_BIT(BC_UPDATE_NEI_EN,        1)
    REGDEF_BIT(                ,        3)
    REGDEF_BIT(BC_DEGHOST_EN   ,        1)
    REGDEF_BIT(                ,        3)
    REGDEF_BIT(ROI_EN0         ,        1)
    REGDEF_BIT(ROI_EN1         ,        1)
    REGDEF_BIT(ROI_EN2         ,        1)
    REGDEF_BIT(ROI_EN3         ,        1)
    REGDEF_BIT(ROI_EN4         ,        1)
    REGDEF_BIT(ROI_EN5         ,        1)
    REGDEF_BIT(ROI_EN6         ,        1)
    REGDEF_BIT(ROI_EN7         ,        1)
    REGDEF_BIT(CHKSUM_EN       ,        1)
    REGDEF_BIT(                ,        3)
    REGDEF_BIT(BGMW_SAVE_BW_EN ,        1)
    REGDEF_BIT(                ,        3)
    REGDEF_BIT(BC_Y_ONLY_EN    ,        1)
REGDEF_END(MDBC_MODE_REGISTER0)


/*
    INTE_FRM_END :    [0x0, 0x1],			bits : 0
    INTE_LLEND   :    [0x0, 0x1],			bits : 4
    INTE_LLERROR :    [0x0, 0x1],			bits : 5
    INTE_LLERROR2:    [0x0, 0x1],			bits : 6
    INTE_LLJOBEND:    [0x0, 0x1],			bits : 7
*/
#define MDBC_INTERRUPT_ENABLE_REGISTER_OFS 0x0008
REGDEF_BEGIN(MDBC_INTERRUPT_ENABLE_REGISTER)
    REGDEF_BIT(INTE_FRM_END ,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(INTE_LLEND   ,        1)
    REGDEF_BIT(INTE_LLERROR ,        1)
    REGDEF_BIT(INTE_LLERROR2,        1)
    REGDEF_BIT(INTE_LLJOBEND,        1)
REGDEF_END(MDBC_INTERRUPT_ENABLE_REGISTER)


/*
    INTS_FRM_END :    [0x0, 0x1],			bits : 0
    INTS_LLEND   :    [0x0, 0x1],			bits : 4
    INTS_LLERROR :    [0x0, 0x1],			bits : 5
    INTS_LLERROR2:    [0x0, 0x1],			bits : 6
    INTS_LLJOBEND:    [0x0, 0x1],			bits : 7
*/
#define MDBC_INTERRUPT_STATUS_REGISTER_OFS 0x000c
REGDEF_BEGIN(MDBC_INTERRUPT_STATUS_REGISTER)
    REGDEF_BIT(INTS_FRM_END ,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(INTS_LLEND   ,        1)
    REGDEF_BIT(INTS_LLERROR ,        1)
    REGDEF_BIT(INTS_LLERROR2,        1)
    REGDEF_BIT(INTS_LLJOBEND,        1)
REGDEF_END(MDBC_INTERRUPT_STATUS_REGISTER)


/*
    DRAM_SAI0:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_MDBC_REGISTER0_OFS 0x0010
REGDEF_BEGIN(DMA_TO_MDBC_REGISTER0)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI0,        30)
REGDEF_END(DMA_TO_MDBC_REGISTER0)


/*
    DRAM_SAI1:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_MDBC_REGISTER1_OFS 0x0014
REGDEF_BEGIN(DMA_TO_MDBC_REGISTER1)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI1,        30)
REGDEF_END(DMA_TO_MDBC_REGISTER1)


/*
    DRAM_OFSI0:    [0x0, 0x3ffff],			bits : 19_2
*/
#define DMA_TO_MDBC_REGISTER2_OFS 0x0018
REGDEF_BEGIN(DMA_TO_MDBC_REGISTER2)
    REGDEF_BIT(          ,        2)
    REGDEF_BIT(DRAM_OFSI0,        18)
REGDEF_END(DMA_TO_MDBC_REGISTER2)


/*
    DRAM_OFSI1:    [0x0, 0x3ffff],			bits : 19_2
*/
#define DMA_TO_MDBC_REGISTER3_OFS 0x001c
REGDEF_BEGIN(DMA_TO_MDBC_REGISTER3)
    REGDEF_BIT(          ,        2)
    REGDEF_BIT(DRAM_OFSI1,        18)
REGDEF_END(DMA_TO_MDBC_REGISTER3)


/*
    DRAM_SAI2:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_MDBC_REGISTER4_OFS 0x0020
REGDEF_BEGIN(DMA_TO_MDBC_REGISTER4)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI2,        30)
REGDEF_END(DMA_TO_MDBC_REGISTER4)


/*
    DRAM_SAI3:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_MDBC_REGISTER5_OFS 0x0024
REGDEF_BEGIN(DMA_TO_MDBC_REGISTER5)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI3,        30)
REGDEF_END(DMA_TO_MDBC_REGISTER5)


/*
    DRAM_SAI4:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_MDBC_REGISTER6_OFS 0x0028
REGDEF_BEGIN(DMA_TO_MDBC_REGISTER6)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI4,        30)
REGDEF_END(DMA_TO_MDBC_REGISTER6)


/*
    DRAM_SAI5:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_MDBC_REGISTER7_OFS 0x002c
REGDEF_BEGIN(DMA_TO_MDBC_REGISTER7)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI5,        30)
REGDEF_END(DMA_TO_MDBC_REGISTER7)


/*
    DRAM_SAI_LL:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_MDBC_LINKED_LIST_REGISTER_OFS 0x0030
REGDEF_BEGIN(DMA_TO_MDBC_LINKED_LIST_REGISTER)
    REGDEF_BIT(           ,        2)
    REGDEF_BIT(DRAM_SAI_LL,        30)
REGDEF_END(DMA_TO_MDBC_LINKED_LIST_REGISTER)


/*
    DRAM_SAO0:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define MDBC_TO_DMA_REGISTER0_OFS 0x0034
REGDEF_BEGIN(MDBC_TO_DMA_REGISTER0)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAO0,        30)
REGDEF_END(MDBC_TO_DMA_REGISTER0)


/*
    DRAM_SAO1:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define MDBC_TO_DMA_REGISTER1_OFS 0x0038
REGDEF_BEGIN(MDBC_TO_DMA_REGISTER1)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAO1,        30)
REGDEF_END(MDBC_TO_DMA_REGISTER1)


/*
    DRAM_SAO2:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define MDBC_TO_DMA_REGISTER2_OFS 0x003c
REGDEF_BEGIN(MDBC_TO_DMA_REGISTER2)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAO2,        30)
REGDEF_END(MDBC_TO_DMA_REGISTER2)


/*
    DRAM_SAO3:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define MDBC_TO_DMA_REGISTER3_OFS 0x0040
REGDEF_BEGIN(MDBC_TO_DMA_REGISTER3)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAO3,        30)
REGDEF_END(MDBC_TO_DMA_REGISTER3)


/*
    IN_BURST_LENGTH :    [0x0, 0x1],			bits : 0
    OUT_BURST_LENGTH:    [0x0, 0x1],			bits : 1
    VERSION_CODE    :    [0x0, 0xffff],			bits : 31_16
*/
#define DESIGN_DEBUG_REGISTER0_OFS 0x0044
REGDEF_BEGIN(DESIGN_DEBUG_REGISTER0)
    REGDEF_BIT(IN_BURST_LENGTH ,        1)
    REGDEF_BIT(OUT_BURST_LENGTH,        1)
    REGDEF_BIT(                ,        14)
    REGDEF_BIT(VERSION_CODE    ,        16)
REGDEF_END(DESIGN_DEBUG_REGISTER0)


/*
    WIDTH :    [0x0, 0x1ff],			bits : 9_1
    HEIGHT:    [0x0, 0x1ff],			bits : 21_13
*/
#define INPUT_SIZE_REGISTER_OFS 0x0048
REGDEF_BEGIN(INPUT_SIZE_REGISTER)
    REGDEF_BIT(      ,        1)
    REGDEF_BIT(WIDTH ,        9)
    REGDEF_BIT(      ,        3)
    REGDEF_BIT(HEIGHT,        9)
REGDEF_END(INPUT_SIZE_REGISTER)


/*
    LBSP_TH :    [0x0, 0xff],			bits : 7_0
    D_COLOUR:    [0x0, 0xff],			bits : 15_8
    R_COLOUR:    [0x0, 0xff],			bits : 23_16
    D_LBSP  :    [0x0, 0xf],			bits : 27_24
    R_LBSP  :    [0x0, 0xf],			bits : 31_28
*/
#define MODEL_MATCH_REGISTER0_OFS 0x004c
REGDEF_BEGIN(MODEL_MATCH_REGISTER0)
    REGDEF_BIT(LBSP_TH ,        8)
    REGDEF_BIT(D_COLOUR,        8)
    REGDEF_BIT(R_COLOUR,        8)
    REGDEF_BIT(D_LBSP  ,        4)
    REGDEF_BIT(R_LBSP  ,        4)
REGDEF_END(MODEL_MATCH_REGISTER0)


/*
    BG_MODEL_NUM:    [0x0, 0xf],			bits : 3_0
    T_ALPHA     :    [0x0, 0xff],			bits : 11_4
    DW_SHIFT    :    [0x0, 0x7],			bits : 14_12
    D_LAST_ALPHA:    [0x0, 0x3ff],			bits : 25_16
*/
#define MODEL_MATCH_REGISTER1_OFS 0x0050
REGDEF_BEGIN(MODEL_MATCH_REGISTER1)
    REGDEF_BIT(BG_MODEL_NUM,        4)
    REGDEF_BIT(T_ALPHA     ,        8)
    REGDEF_BIT(DW_SHIFT    ,        3)
    REGDEF_BIT(            ,        1)
    REGDEF_BIT(D_LAST_ALPHA,        10)
REGDEF_END(MODEL_MATCH_REGISTER1)


/*
    BC_MIN_MATCH:    [0x0, 0xf],			bits : 3_0
    DLT_ALPHA   :    [0x0, 0x3ff],			bits : 13_4
    DST_ALPHA   :    [0x0, 0x3ff],			bits : 25_16
*/
#define MODEL_MATCH_REGISTER2_OFS 0x0054
REGDEF_BEGIN(MODEL_MATCH_REGISTER2)
    REGDEF_BIT(BC_MIN_MATCH,        4)
    REGDEF_BIT(DLT_ALPHA   ,        10)
    REGDEF_BIT(            ,        2)
    REGDEF_BIT(DST_ALPHA   ,        10)
REGDEF_END(MODEL_MATCH_REGISTER2)


/*
    BC_UV_THRES:    [0x0, 0xff],			bits : 7_0
    S_ALPHA    :    [0x0, 0x3ff],			bits : 21_12
*/
#define MODEL_MATCH_REGISTER3_OFS 0x0058
REGDEF_BEGIN(MODEL_MATCH_REGISTER3)
    REGDEF_BIT(BC_UV_THRES,        8)
    REGDEF_BIT(           ,        4)
    REGDEF_BIT(S_ALPHA    ,        10)
REGDEF_END(MODEL_MATCH_REGISTER3)


/*
    DBG_LUM_DIFF   :    [0x0, 0xfffffff],			bits : 27_0
    DBG_LUM_DIFF_EN:    [0x0, 0x1],			bits : 28
*/
#define MODEL_MATCH_REGISTER4_OFS 0x005c
REGDEF_BEGIN(MODEL_MATCH_REGISTER4)
    REGDEF_BIT(DBG_LUM_DIFF   ,        28)
    REGDEF_BIT(DBG_LUM_DIFF_EN,        1)
REGDEF_END(MODEL_MATCH_REGISTER4)


/*
    LUM_DIFF:    [0x0, 0xfffffff],			bits : 27_0
*/
#define MODEL_MATCH_REGISTER5_OFS 0x0060
REGDEF_BEGIN(MODEL_MATCH_REGISTER5)
    REGDEF_BIT(LUM_DIFF,        28)
REGDEF_END(MODEL_MATCH_REGISTER5)


/*
    MOR_TH0   :    [0x0, 0xf],			bits : 3_0
    MOR_TH1   :    [0x0, 0xf],			bits : 7_4
    MOR_TH2   :    [0x0, 0xf],			bits : 11_8
    MOR_TH3   :    [0x0, 0xf],			bits : 15_12
    MOR_TH_DIL:    [0x0, 0xf],			bits : 19_16
    MOR_SEL0  :    [0x0, 0x1],			bits : 20
    MOR_SEL1  :    [0x0, 0x1],			bits : 21
    MOR_SEL2  :    [0x0, 0x1],			bits : 22
    MOR_SEL3  :    [0x0, 0x1],			bits : 23
*/
#define MORPHOLOGICAL_PROCESS_REGISTER_OFS 0x0064
REGDEF_BEGIN(MORPHOLOGICAL_PROCESS_REGISTER)
    REGDEF_BIT(MOR_TH0   ,        4)
    REGDEF_BIT(MOR_TH1   ,        4)
    REGDEF_BIT(MOR_TH2   ,        4)
    REGDEF_BIT(MOR_TH3   ,        4)
    REGDEF_BIT(MOR_TH_DIL,        4)
    REGDEF_BIT(MOR_SEL0  ,        1)
    REGDEF_BIT(MOR_SEL1  ,        1)
    REGDEF_BIT(MOR_SEL2  ,        1)
    REGDEF_BIT(MOR_SEL3  ,        1)
REGDEF_END(MORPHOLOGICAL_PROCESS_REGISTER)


/*
    BC_MIN_T     :    [0x0, 0xff],			bits : 7_0
    BC_MAX_T     :    [0x0, 0xff],			bits : 15_8
    BC_MAX_FG_FRM:    [0x0, 0xff],			bits : 23_16
*/
#define UPDATE_REGISTER0_OFS 0x0068
REGDEF_BEGIN(UPDATE_REGISTER0)
    REGDEF_BIT(BC_MIN_T     ,        8)
    REGDEF_BIT(BC_MAX_T     ,        8)
    REGDEF_BIT(BC_MAX_FG_FRM,        8)
REGDEF_END(UPDATE_REGISTER0)


/*
    BC_DEGHOST_DTH:    [0x0, 0x1ff],			bits : 8_0
    BC_DEGHOST_STH:    [0x0, 0xff],			bits : 23_16
*/
#define UPDATE_REGISTER1_OFS 0x006c
REGDEF_BEGIN(UPDATE_REGISTER1)
    REGDEF_BIT(BC_DEGHOST_DTH,        9)
    REGDEF_BIT(              ,        7)
    REGDEF_BIT(BC_DEGHOST_STH,        8)
REGDEF_END(UPDATE_REGISTER1)


/*
    BC_STABLE_FRAME:    [0x0, 0xff],			bits : 7_0
    BC_UPDATE_DYN  :    [0x0, 0xff],			bits : 15_8
    BC_VA_DISTTH   :    [0x0, 0xff],			bits : 23_16
    BC_T_DISTTH    :    [0x0, 0xff],			bits : 31_24
*/
#define UPDATE_REGISTER2_OFS 0x0070
REGDEF_BEGIN(UPDATE_REGISTER2)
    REGDEF_BIT(BC_STABLE_FRAME,        8)
    REGDEF_BIT(BC_UPDATE_DYN  ,        8)
    REGDEF_BIT(BC_VA_DISTTH   ,        8)
    REGDEF_BIT(BC_T_DISTTH    ,        8)
REGDEF_END(UPDATE_REGISTER2)


/*
    DBG_FRM_ID   :    [0x0, 0xff],			bits : 7_0
    DBG_FRM_ID_EN:    [0x0, 0x1],			bits : 8
    FRM_ID       :    [0x0, 0xff],			bits : 19_12
*/
#define UPDATE_REGISTER3_OFS 0x0074
REGDEF_BEGIN(UPDATE_REGISTER3)
    REGDEF_BIT(DBG_FRM_ID   ,        8)
    REGDEF_BIT(DBG_FRM_ID_EN,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(FRM_ID       ,        8)
REGDEF_END(UPDATE_REGISTER3)


/*
    DBG_RND   :    [0x0, 0x7fff],			bits : 14_0
    DBG_RND_EN:    [0x0, 0x1],			bits : 15
    RND       :    [0x0, 0x7fff],			bits : 30_16
*/
#define UPDATE_REGISTER4_OFS 0x0078
REGDEF_BEGIN(UPDATE_REGISTER4)
    REGDEF_BIT(DBG_RND   ,        15)
    REGDEF_BIT(DBG_RND_EN,        1)
    REGDEF_BIT(RND       ,        15)
REGDEF_END(UPDATE_REGISTER4)


/*
    ROI_X0:    [0x0, 0x3ff],			bits : 9_0
    ROI_Y0:    [0x0, 0x3ff],			bits : 21_12
*/
#define ROI0_REGISTER0_OFS 0x007c
REGDEF_BEGIN(ROI0_REGISTER0)
    REGDEF_BIT(ROI_X0,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(ROI_Y0,        10)
REGDEF_END(ROI0_REGISTER0)


/*
    ROI_W0       :    [0x0, 0x3ff],			bits : 9_0
    ROI_H0       :    [0x0, 0x3ff],			bits : 21_12
    ROI_UV_THRES0:    [0x0, 0xff],			bits : 31_24
*/
#define ROI0_REGISTER1_OFS 0x0080
REGDEF_BEGIN(ROI0_REGISTER1)
    REGDEF_BIT(ROI_W0       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_H0       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_UV_THRES0,        8)
REGDEF_END(ROI0_REGISTER1)


/*
    ROI_LBSP_TH0 :    [0x0, 0xff],			bits : 7_0
    ROI_D_COLOUR0:    [0x0, 0xff],			bits : 15_8
    ROI_R_COLOUR0:    [0x0, 0xff],			bits : 23_16
    ROI_D_LBSP0  :    [0x0, 0xf],			bits : 27_24
    ROI_R_LBSP0  :    [0x0, 0xf],			bits : 31_28
*/
#define ROI0_REGISTER2_OFS 0x0084
REGDEF_BEGIN(ROI0_REGISTER2)
    REGDEF_BIT(ROI_LBSP_TH0 ,        8)
    REGDEF_BIT(ROI_D_COLOUR0,        8)
    REGDEF_BIT(ROI_R_COLOUR0,        8)
    REGDEF_BIT(ROI_D_LBSP0  ,        4)
    REGDEF_BIT(ROI_R_LBSP0  ,        4)
REGDEF_END(ROI0_REGISTER2)


/*
    ROI_MORPH_EN0:    [0x0, 0x1],			bits : 0
    ROI_MIN_T0   :    [0x0, 0xff],			bits : 11_4
    ROI_MAX_T0   :    [0x0, 0xff],			bits : 19_12
*/
#define ROI0_REGISTER3_OFS 0x0088
REGDEF_BEGIN(ROI0_REGISTER3)
    REGDEF_BIT(ROI_MORPH_EN0,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(ROI_MIN_T0   ,        8)
    REGDEF_BIT(ROI_MAX_T0   ,        8)
REGDEF_END(ROI0_REGISTER3)


/*
    ROI_X1:    [0x0, 0x3ff],			bits : 9_0
    ROI_Y1:    [0x0, 0x3ff],			bits : 21_12
*/
#define ROI1_REGISTER0_OFS 0x008c
REGDEF_BEGIN(ROI1_REGISTER0)
    REGDEF_BIT(ROI_X1,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(ROI_Y1,        10)
REGDEF_END(ROI1_REGISTER0)


/*
    ROI_W1       :    [0x0, 0x3ff],			bits : 9_0
    ROI_H1       :    [0x0, 0x3ff],			bits : 21_12
    ROI_UV_THRES1:    [0x0, 0xff],			bits : 31_24
*/
#define ROI1_REGISTER1_OFS 0x0090
REGDEF_BEGIN(ROI1_REGISTER1)
    REGDEF_BIT(ROI_W1       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_H1       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_UV_THRES1,        8)
REGDEF_END(ROI1_REGISTER1)


/*
    ROI_LBSP_TH1 :    [0x0, 0xff],			bits : 7_0
    ROI_D_COLOUR1:    [0x0, 0xff],			bits : 15_8
    ROI_R_COLOUR1:    [0x0, 0xff],			bits : 23_16
    ROI_D_LBSP1  :    [0x0, 0xf],			bits : 27_24
    ROI_R_LBSP1  :    [0x0, 0xf],			bits : 31_28
*/
#define ROI1_REGISTER2_OFS 0x0094
REGDEF_BEGIN(ROI1_REGISTER2)
    REGDEF_BIT(ROI_LBSP_TH1 ,        8)
    REGDEF_BIT(ROI_D_COLOUR1,        8)
    REGDEF_BIT(ROI_R_COLOUR1,        8)
    REGDEF_BIT(ROI_D_LBSP1  ,        4)
    REGDEF_BIT(ROI_R_LBSP1  ,        4)
REGDEF_END(ROI1_REGISTER2)


/*
    ROI_MORPH_EN1:    [0x0, 0x1],			bits : 0
    ROI_MIN_T1   :    [0x0, 0xff],			bits : 11_4
    ROI_MAX_T1   :    [0x0, 0xff],			bits : 19_12
*/
#define ROI1_REGISTER3_OFS 0x0098
REGDEF_BEGIN(ROI1_REGISTER3)
    REGDEF_BIT(ROI_MORPH_EN1,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(ROI_MIN_T1   ,        8)
    REGDEF_BIT(ROI_MAX_T1   ,        8)
REGDEF_END(ROI1_REGISTER3)


/*
    ROI_X2:    [0x0, 0x3ff],			bits : 9_0
    ROI_Y2:    [0x0, 0x3ff],			bits : 21_12
*/
#define ROI2_REGISTER0_OFS 0x009c
REGDEF_BEGIN(ROI2_REGISTER0)
    REGDEF_BIT(ROI_X2,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(ROI_Y2,        10)
REGDEF_END(ROI2_REGISTER0)


/*
    ROI_W2       :    [0x0, 0x3ff],			bits : 9_0
    ROI_H2       :    [0x0, 0x3ff],			bits : 21_12
    ROI_UV_THRES2:    [0x0, 0xff],			bits : 31_24
*/
#define ROI2_REGISTER1_OFS 0x00a0
REGDEF_BEGIN(ROI2_REGISTER1)
    REGDEF_BIT(ROI_W2       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_H2       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_UV_THRES2,        8)
REGDEF_END(ROI2_REGISTER1)


/*
    ROI_LBSP_TH2 :    [0x0, 0xff],			bits : 7_0
    ROI_D_COLOUR2:    [0x0, 0xff],			bits : 15_8
    ROI_R_COLOUR2:    [0x0, 0xff],			bits : 23_16
    ROI_D_LBSP2  :    [0x0, 0xf],			bits : 27_24
    ROI_R_LBSP2  :    [0x0, 0xf],			bits : 31_28
*/
#define ROI2_REGISTER2_OFS 0x00a4
REGDEF_BEGIN(ROI2_REGISTER2)
    REGDEF_BIT(ROI_LBSP_TH2 ,        8)
    REGDEF_BIT(ROI_D_COLOUR2,        8)
    REGDEF_BIT(ROI_R_COLOUR2,        8)
    REGDEF_BIT(ROI_D_LBSP2  ,        4)
    REGDEF_BIT(ROI_R_LBSP2  ,        4)
REGDEF_END(ROI2_REGISTER2)


/*
    ROI_MORPH_EN2:    [0x0, 0x1],			bits : 0
    ROI_MIN_T2   :    [0x0, 0xff],			bits : 11_4
    ROI_MAX_T2   :    [0x0, 0xff],			bits : 19_12
*/
#define ROI2_REGISTER3_OFS 0x00a8
REGDEF_BEGIN(ROI2_REGISTER3)
    REGDEF_BIT(ROI_MORPH_EN2,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(ROI_MIN_T2   ,        8)
    REGDEF_BIT(ROI_MAX_T2   ,        8)
REGDEF_END(ROI2_REGISTER3)


/*
    ROI_X3:    [0x0, 0x3ff],			bits : 9_0
    ROI_Y3:    [0x0, 0x3ff],			bits : 21_12
*/
#define ROI3_REGISTER0_OFS 0x00ac
REGDEF_BEGIN(ROI3_REGISTER0)
    REGDEF_BIT(ROI_X3,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(ROI_Y3,        10)
REGDEF_END(ROI3_REGISTER0)


/*
    ROI_W3       :    [0x0, 0x3ff],			bits : 9_0
    ROI_H3       :    [0x0, 0x3ff],			bits : 21_12
    ROI_UV_THRES3:    [0x0, 0xff],			bits : 31_24
*/
#define ROI3_REGISTER1_OFS 0x00b0
REGDEF_BEGIN(ROI3_REGISTER1)
    REGDEF_BIT(ROI_W3       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_H3       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_UV_THRES3,        8)
REGDEF_END(ROI3_REGISTER1)


/*
    ROI_LBSP_TH3 :    [0x0, 0xff],			bits : 7_0
    ROI_D_COLOUR3:    [0x0, 0xff],			bits : 15_8
    ROI_R_COLOUR3:    [0x0, 0xff],			bits : 23_16
    ROI_D_LBSP3  :    [0x0, 0xf],			bits : 27_24
    ROI_R_LBSP3  :    [0x0, 0xf],			bits : 31_28
*/
#define ROI3_REGISTER2_OFS 0x00b4
REGDEF_BEGIN(ROI3_REGISTER2)
    REGDEF_BIT(ROI_LBSP_TH3 ,        8)
    REGDEF_BIT(ROI_D_COLOUR3,        8)
    REGDEF_BIT(ROI_R_COLOUR3,        8)
    REGDEF_BIT(ROI_D_LBSP3  ,        4)
    REGDEF_BIT(ROI_R_LBSP3  ,        4)
REGDEF_END(ROI3_REGISTER2)


/*
    ROI_MORPH_EN3:    [0x0, 0x1],			bits : 0
    ROI_MIN_T3   :    [0x0, 0xff],			bits : 11_4
    ROI_MAX_T3   :    [0x0, 0xff],			bits : 19_12
*/
#define ROI3_REGISTER3_OFS 0x00b8
REGDEF_BEGIN(ROI3_REGISTER3)
    REGDEF_BIT(ROI_MORPH_EN3,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(ROI_MIN_T3   ,        8)
    REGDEF_BIT(ROI_MAX_T3   ,        8)
REGDEF_END(ROI3_REGISTER3)


/*
    ROI_X4:    [0x0, 0x3ff],			bits : 9_0
    ROI_Y4:    [0x0, 0x3ff],			bits : 21_12
*/
#define ROI4_REGISTER0_OFS 0x00bc
REGDEF_BEGIN(ROI4_REGISTER0)
    REGDEF_BIT(ROI_X4,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(ROI_Y4,        10)
REGDEF_END(ROI4_REGISTER0)


/*
    ROI_W4       :    [0x0, 0x3ff],			bits : 9_0
    ROI_H4       :    [0x0, 0x3ff],			bits : 21_12
    ROI_UV_THRES4:    [0x0, 0xff],			bits : 31_24
*/
#define ROI4_REGISTER1_OFS 0x00c0
REGDEF_BEGIN(ROI4_REGISTER1)
    REGDEF_BIT(ROI_W4       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_H4       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_UV_THRES4,        8)
REGDEF_END(ROI4_REGISTER1)


/*
    ROI_LBSP_TH4 :    [0x0, 0xff],			bits : 7_0
    ROI_D_COLOUR4:    [0x0, 0xff],			bits : 15_8
    ROI_R_COLOUR4:    [0x0, 0xff],			bits : 23_16
    ROI_D_LBSP4  :    [0x0, 0xf],			bits : 27_24
    ROI_R_LBSP4  :    [0x0, 0xf],			bits : 31_28
*/
#define ROI4_REGISTER2_OFS 0x00c4
REGDEF_BEGIN(ROI4_REGISTER2)
    REGDEF_BIT(ROI_LBSP_TH4 ,        8)
    REGDEF_BIT(ROI_D_COLOUR4,        8)
    REGDEF_BIT(ROI_R_COLOUR4,        8)
    REGDEF_BIT(ROI_D_LBSP4  ,        4)
    REGDEF_BIT(ROI_R_LBSP4  ,        4)
REGDEF_END(ROI4_REGISTER2)


/*
    ROI_MORPH_EN4:    [0x0, 0x1],			bits : 0
    ROI_MIN_T4   :    [0x0, 0xff],			bits : 11_4
    ROI_MAX_T4   :    [0x0, 0xff],			bits : 19_12
*/
#define ROI4_REGISTER3_OFS 0x00c8
REGDEF_BEGIN(ROI4_REGISTER3)
    REGDEF_BIT(ROI_MORPH_EN4,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(ROI_MIN_T4   ,        8)
    REGDEF_BIT(ROI_MAX_T4   ,        8)
REGDEF_END(ROI4_REGISTER3)


/*
    ROI_X5:    [0x0, 0x3ff],			bits : 9_0
    ROI_Y5:    [0x0, 0x3ff],			bits : 21_12
*/
#define ROI5_REGISTER0_OFS 0x00cc
REGDEF_BEGIN(ROI5_REGISTER0)
    REGDEF_BIT(ROI_X5,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(ROI_Y5,        10)
REGDEF_END(ROI5_REGISTER0)


/*
    ROI_W5       :    [0x0, 0x3ff],			bits : 9_0
    ROI_H5       :    [0x0, 0x3ff],			bits : 21_12
    ROI_UV_THRES5:    [0x0, 0xff],			bits : 31_24
*/
#define ROI5_REGISTER1_OFS 0x00d0
REGDEF_BEGIN(ROI5_REGISTER1)
    REGDEF_BIT(ROI_W5       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_H5       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_UV_THRES5,        8)
REGDEF_END(ROI5_REGISTER1)


/*
    ROI_LBSP_TH5 :    [0x0, 0xff],			bits : 7_0
    ROI_D_COLOUR5:    [0x0, 0xff],			bits : 15_8
    ROI_R_COLOUR5:    [0x0, 0xff],			bits : 23_16
    ROI_D_LBSP5  :    [0x0, 0xf],			bits : 27_24
    ROI_R_LBSP5  :    [0x0, 0xf],			bits : 31_28
*/
#define ROI5_REGISTER2_OFS 0x00d4
REGDEF_BEGIN(ROI5_REGISTER2)
    REGDEF_BIT(ROI_LBSP_TH5 ,        8)
    REGDEF_BIT(ROI_D_COLOUR5,        8)
    REGDEF_BIT(ROI_R_COLOUR5,        8)
    REGDEF_BIT(ROI_D_LBSP5  ,        4)
    REGDEF_BIT(ROI_R_LBSP5  ,        4)
REGDEF_END(ROI5_REGISTER2)


/*
    ROI_MORPH_EN5:    [0x0, 0x1],			bits : 0
    ROI_MIN_T5   :    [0x0, 0xff],			bits : 11_4
    ROI_MAX_T5   :    [0x0, 0xff],			bits : 19_12
*/
#define ROI5_REGISTER3_OFS 0x00d8
REGDEF_BEGIN(ROI5_REGISTER3)
    REGDEF_BIT(ROI_MORPH_EN5,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(ROI_MIN_T5   ,        8)
    REGDEF_BIT(ROI_MAX_T5   ,        8)
REGDEF_END(ROI5_REGISTER3)


/*
    ROI_X6:    [0x0, 0x3ff],			bits : 9_0
    ROI_Y6:    [0x0, 0x3ff],			bits : 21_12
*/
#define ROI6_REGISTER0_OFS 0x00dc
REGDEF_BEGIN(ROI6_REGISTER0)
    REGDEF_BIT(ROI_X6,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(ROI_Y6,        10)
REGDEF_END(ROI6_REGISTER0)


/*
    ROI_W6       :    [0x0, 0x3ff],			bits : 9_0
    ROI_H6       :    [0x0, 0x3ff],			bits : 21_12
    ROI_UV_THRES6:    [0x0, 0xff],			bits : 31_24
*/
#define ROI6_REGISTER1_OFS 0x00e0
REGDEF_BEGIN(ROI6_REGISTER1)
    REGDEF_BIT(ROI_W6       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_H6       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_UV_THRES6,        8)
REGDEF_END(ROI6_REGISTER1)


/*
    ROI_LBSP_TH6 :    [0x0, 0xff],			bits : 7_0
    ROI_D_COLOUR6:    [0x0, 0xff],			bits : 15_8
    ROI_R_COLOUR6:    [0x0, 0xff],			bits : 23_16
    ROI_D_LBSP6  :    [0x0, 0xf],			bits : 27_24
    ROI_R_LBSP6  :    [0x0, 0xf],			bits : 31_28
*/
#define ROI6_REGISTER2_OFS 0x00e4
REGDEF_BEGIN(ROI6_REGISTER2)
    REGDEF_BIT(ROI_LBSP_TH6 ,        8)
    REGDEF_BIT(ROI_D_COLOUR6,        8)
    REGDEF_BIT(ROI_R_COLOUR6,        8)
    REGDEF_BIT(ROI_D_LBSP6  ,        4)
    REGDEF_BIT(ROI_R_LBSP6  ,        4)
REGDEF_END(ROI6_REGISTER2)


/*
    ROI_MORPH_EN6:    [0x0, 0x1],			bits : 0
    ROI_MIN_T6   :    [0x0, 0xff],			bits : 11_4
    ROI_MAX_T6   :    [0x0, 0xff],			bits : 19_12
*/
#define ROI6_REGISTER3_OFS 0x00e8
REGDEF_BEGIN(ROI6_REGISTER3)
    REGDEF_BIT(ROI_MORPH_EN6,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(ROI_MIN_T6   ,        8)
    REGDEF_BIT(ROI_MAX_T6   ,        8)
REGDEF_END(ROI6_REGISTER3)


/*
    ROI_X7:    [0x0, 0x3ff],			bits : 9_0
    ROI_Y7:    [0x0, 0x3ff],			bits : 21_12
*/
#define ROI7_REGISTER0_OFS 0x00ec
REGDEF_BEGIN(ROI7_REGISTER0)
    REGDEF_BIT(ROI_X7,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(ROI_Y7,        10)
REGDEF_END(ROI7_REGISTER0)


/*
    ROI_W7       :    [0x0, 0x3ff],			bits : 9_0
    ROI_H7       :    [0x0, 0x3ff],			bits : 21_12
    ROI_UV_THRES7:    [0x0, 0xff],			bits : 31_24
*/
#define ROI7_REGISTER1_OFS 0x00f0
REGDEF_BEGIN(ROI7_REGISTER1)
    REGDEF_BIT(ROI_W7       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_H7       ,        10)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(ROI_UV_THRES7,        8)
REGDEF_END(ROI7_REGISTER1)


/*
    ROI_LBSP_TH7 :    [0x0, 0xff],			bits : 7_0
    ROI_D_COLOUR7:    [0x0, 0xff],			bits : 15_8
    ROI_R_COLOUR7:    [0x0, 0xff],			bits : 23_16
    ROI_D_LBSP7  :    [0x0, 0xf],			bits : 27_24
    ROI_R_LBSP7  :    [0x0, 0xf],			bits : 31_28
*/
#define ROI7_REGISTER2_OFS 0x00f4
REGDEF_BEGIN(ROI7_REGISTER2)
    REGDEF_BIT(ROI_LBSP_TH7 ,        8)
    REGDEF_BIT(ROI_D_COLOUR7,        8)
    REGDEF_BIT(ROI_R_COLOUR7,        8)
    REGDEF_BIT(ROI_D_LBSP7  ,        4)
    REGDEF_BIT(ROI_R_LBSP7  ,        4)
REGDEF_END(ROI7_REGISTER2)


/*
    ROI_MORPH_EN7:    [0x0, 0x1],			bits : 0
    ROI_MIN_T7   :    [0x0, 0xff],			bits : 11_4
    ROI_MAX_T7   :    [0x0, 0xff],			bits : 19_12
*/
#define ROI7_REGISTER3_OFS 0x00f8
REGDEF_BEGIN(ROI7_REGISTER3)
    REGDEF_BIT(ROI_MORPH_EN7,        1)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(ROI_MIN_T7   ,        8)
    REGDEF_BIT(ROI_MAX_T7   ,        8)
REGDEF_END(ROI7_REGISTER3)


/*
    LL_TABLE_IDX0:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX1:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX2:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX3:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER0_OFS 0x00fc
REGDEF_BEGIN(LL_FRAME_REGISTER0)
    REGDEF_BIT(LL_TABLE_IDX0,        8)
    REGDEF_BIT(LL_TABLE_IDX1,        8)
    REGDEF_BIT(LL_TABLE_IDX2,        8)
    REGDEF_BIT(LL_TABLE_IDX3,        8)
REGDEF_END(LL_FRAME_REGISTER0)


/*
    LL_TABLE_IDX4:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX5:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX6:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX7:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER1_OFS 0x0100
REGDEF_BEGIN(LL_FRAME_REGISTER1)
    REGDEF_BIT(LL_TABLE_IDX4,        8)
    REGDEF_BIT(LL_TABLE_IDX5,        8)
    REGDEF_BIT(LL_TABLE_IDX6,        8)
    REGDEF_BIT(LL_TABLE_IDX7,        8)
REGDEF_END(LL_FRAME_REGISTER1)


/*
    LL_TABLE_IDX8 :    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX9 :    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX10:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX11:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER2_OFS 0x0104
REGDEF_BEGIN(LL_FRAME_REGISTER2)
    REGDEF_BIT(LL_TABLE_IDX8 ,        8)
    REGDEF_BIT(LL_TABLE_IDX9 ,        8)
    REGDEF_BIT(LL_TABLE_IDX10,        8)
    REGDEF_BIT(LL_TABLE_IDX11,        8)
REGDEF_END(LL_FRAME_REGISTER2)


/*
    LL_TABLE_IDX12:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX13:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX14:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX15:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER3_OFS 0x0108
REGDEF_BEGIN(LL_FRAME_REGISTER3)
    REGDEF_BIT(LL_TABLE_IDX12,        8)
    REGDEF_BIT(LL_TABLE_IDX13,        8)
    REGDEF_BIT(LL_TABLE_IDX14,        8)
    REGDEF_BIT(LL_TABLE_IDX15,        8)
REGDEF_END(LL_FRAME_REGISTER3)


/*
    LL_TERMINATE:    [0x0, 0x1],			bits : 0
*/
#define LL_CONTROL_REGISTER_OFS 0x010c
REGDEF_BEGIN(LL_CONTROL_REGISTER)
    REGDEF_BIT(LL_TERMINATE,        1)
REGDEF_END(LL_CONTROL_REGISTER)


/*
    DMA_CH_DISABLE:    [0x0, 0x1],			bits : 0
    DMA_IDLE      :    [0x0, 0x1],			bits : 15
*/
#define DMA_DISABLE_REGISTER0_OFS 0x0110
REGDEF_BEGIN(DMA_DISABLE_REGISTER0)
    REGDEF_BIT(DMA_CH_DISABLE,        1)
    REGDEF_BIT(              ,        14)
    REGDEF_BIT(DMA_IDLE      ,        1)
REGDEF_END(DMA_DISABLE_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define RESERVED_REGISTER0_OFS 0x0114
REGDEF_BEGIN(RESERVED_REGISTER0)
    REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define RESERVED_REGISTER1_OFS 0x0118
REGDEF_BEGIN(RESERVED_REGISTER1)
    REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED_REGISTER1)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define RESERVED_REGISTER2_OFS 0x011c
REGDEF_BEGIN(RESERVED_REGISTER2)
    REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED_REGISTER2)


/*
    CHKSUM_SRCY:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER0_OFS 0x0120
REGDEF_BEGIN(CHECK_SUM_REGISTER0)
    REGDEF_BIT(CHKSUM_SRCY,        32)
REGDEF_END(CHECK_SUM_REGISTER0)


/*
    CHKSUM_SRCUV:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER1_OFS 0x0124
REGDEF_BEGIN(CHECK_SUM_REGISTER1)
    REGDEF_BIT(CHKSUM_SRCUV,        32)
REGDEF_END(CHECK_SUM_REGISTER1)


/*
    CHKSUM_PREUV:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER2_OFS 0x0128
REGDEF_BEGIN(CHECK_SUM_REGISTER2)
    REGDEF_BIT(CHKSUM_PREUV,        32)
REGDEF_END(CHECK_SUM_REGISTER2)


/*
    CHKSUM_BGMR:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER3_OFS 0x012c
REGDEF_BEGIN(CHECK_SUM_REGISTER3)
    REGDEF_BIT(CHKSUM_BGMR,        32)
REGDEF_END(CHECK_SUM_REGISTER3)


/*
    CHKSUM_VAR1:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER4_OFS 0x0130
REGDEF_BEGIN(CHECK_SUM_REGISTER4)
    REGDEF_BIT(CHKSUM_VAR1,        32)
REGDEF_END(CHECK_SUM_REGISTER4)


/*
    CHKSUM_FVAR2:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER5_OFS 0x0134
REGDEF_BEGIN(CHECK_SUM_REGISTER5)
    REGDEF_BIT(CHKSUM_FVAR2,        32)
REGDEF_END(CHECK_SUM_REGISTER5)


/*
    CHKSUM_BGMW:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER6_OFS 0x0138
REGDEF_BEGIN(CHECK_SUM_REGISTER6)
    REGDEF_BIT(CHKSUM_BGMW,        32)
REGDEF_END(CHECK_SUM_REGISTER6)


/*
    CHKSUM_UV1:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER7_OFS 0x013c
REGDEF_BEGIN(CHECK_SUM_REGISTER7)
    REGDEF_BIT(CHKSUM_UV1,        32)
REGDEF_END(CHECK_SUM_REGISTER7)


/*
    CHKSUM_BVAR2:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER8_OFS 0x0140
REGDEF_BEGIN(CHECK_SUM_REGISTER8)
    REGDEF_BIT(CHKSUM_BVAR2,        32)
REGDEF_END(CHECK_SUM_REGISTER8)


/*
    CHKSUM_UPD:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER9_OFS 0x0144
REGDEF_BEGIN(CHECK_SUM_REGISTER9)
    REGDEF_BIT(CHKSUM_UPD,        32)
REGDEF_END(CHECK_SUM_REGISTER9)


/*
    CHKSUM_PASS:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER10_OFS 0x0148
REGDEF_BEGIN(CHECK_SUM_REGISTER10)
    REGDEF_BIT(CHKSUM_PASS,        32)
REGDEF_END(CHECK_SUM_REGISTER10)


/*
    CHKSUM_MPH:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECK_SUM_REGISTER11_OFS 0x014c
REGDEF_BEGIN(CHECK_SUM_REGISTER11)
    REGDEF_BIT(CHKSUM_MPH,        32)
REGDEF_END(CHECK_SUM_REGISTER11)


typedef struct
{
    //0x0000
    T_MDBC_CONTROL_REGISTER 
    MDBC_Register_0000;

    //0x0004
    T_MDBC_MODE_REGISTER0 
    MDBC_Register_0004;

    //0x0008
    T_MDBC_INTERRUPT_ENABLE_REGISTER 
    MDBC_Register_0008;

    //0x000c
    T_MDBC_INTERRUPT_STATUS_REGISTER 
    MDBC_Register_000c;

    //0x0010
    T_DMA_TO_MDBC_REGISTER0 
    MDBC_Register_0010;

    //0x0014
    T_DMA_TO_MDBC_REGISTER1 
    MDBC_Register_0014;

    //0x0018
    T_DMA_TO_MDBC_REGISTER2 
    MDBC_Register_0018;

    //0x001c
    T_DMA_TO_MDBC_REGISTER3 
    MDBC_Register_001c;

    //0x0020
    T_DMA_TO_MDBC_REGISTER4 
    MDBC_Register_0020;

    //0x0024
    T_DMA_TO_MDBC_REGISTER5 
    MDBC_Register_0024;

    //0x0028
    T_DMA_TO_MDBC_REGISTER6 
    MDBC_Register_0028;

    //0x002c
    T_DMA_TO_MDBC_REGISTER7 
    MDBC_Register_002c;

    //0x0030
    T_DMA_TO_MDBC_LINKED_LIST_REGISTER 
    MDBC_Register_0030;

    //0x0034
    T_MDBC_TO_DMA_REGISTER0 
    MDBC_Register_0034;

    //0x0038
    T_MDBC_TO_DMA_REGISTER1 
    MDBC_Register_0038;

    //0x003c
    T_MDBC_TO_DMA_REGISTER2 
    MDBC_Register_003c;

    //0x0040
    T_MDBC_TO_DMA_REGISTER3 
    MDBC_Register_0040;

    //0x0044
    T_DESIGN_DEBUG_REGISTER0 
    MDBC_Register_0044;

    //0x0048
    T_INPUT_SIZE_REGISTER 
    MDBC_Register_0048;

    //0x004c
    T_MODEL_MATCH_REGISTER0 
    MDBC_Register_004c;

    //0x0050
    T_MODEL_MATCH_REGISTER1 
    MDBC_Register_0050;

    //0x0054
    T_MODEL_MATCH_REGISTER2 
    MDBC_Register_0054;

    //0x0058
    T_MODEL_MATCH_REGISTER3 
    MDBC_Register_0058;

    //0x005c
    T_MODEL_MATCH_REGISTER4 
    MDBC_Register_005c;

    //0x0060
    T_MODEL_MATCH_REGISTER5 
    MDBC_Register_0060;

    //0x0064
    T_MORPHOLOGICAL_PROCESS_REGISTER 
    MDBC_Register_0064;

    //0x0068
    T_UPDATE_REGISTER0 
    MDBC_Register_0068;

    //0x006c
    T_UPDATE_REGISTER1 
    MDBC_Register_006c;

    //0x0070
    T_UPDATE_REGISTER2 
    MDBC_Register_0070;

    //0x0074
    T_UPDATE_REGISTER3 
    MDBC_Register_0074;

    //0x0078
    T_UPDATE_REGISTER4 
    MDBC_Register_0078;

    //0x007c
    T_ROI0_REGISTER0 
    MDBC_Register_007c;

    //0x0080
    T_ROI0_REGISTER1 
    MDBC_Register_0080;

    //0x0084
    T_ROI0_REGISTER2 
    MDBC_Register_0084;

    //0x0088
    T_ROI0_REGISTER3 
    MDBC_Register_0088;

    //0x008c
    T_ROI1_REGISTER0 
    MDBC_Register_008c;

    //0x0090
    T_ROI1_REGISTER1 
    MDBC_Register_0090;

    //0x0094
    T_ROI1_REGISTER2 
    MDBC_Register_0094;

    //0x0098
    T_ROI1_REGISTER3 
    MDBC_Register_0098;

    //0x009c
    T_ROI2_REGISTER0 
    MDBC_Register_009c;

    //0x00a0
    T_ROI2_REGISTER1 
    MDBC_Register_00a0;

    //0x00a4
    T_ROI2_REGISTER2 
    MDBC_Register_00a4;

    //0x00a8
    T_ROI2_REGISTER3 
    MDBC_Register_00a8;

    //0x00ac
    T_ROI3_REGISTER0 
    MDBC_Register_00ac;

    //0x00b0
    T_ROI3_REGISTER1 
    MDBC_Register_00b0;

    //0x00b4
    T_ROI3_REGISTER2 
    MDBC_Register_00b4;

    //0x00b8
    T_ROI3_REGISTER3 
    MDBC_Register_00b8;

    //0x00bc
    T_ROI4_REGISTER0 
    MDBC_Register_00bc;

    //0x00c0
    T_ROI4_REGISTER1 
    MDBC_Register_00c0;

    //0x00c4
    T_ROI4_REGISTER2 
    MDBC_Register_00c4;

    //0x00c8
    T_ROI4_REGISTER3 
    MDBC_Register_00c8;

    //0x00cc
    T_ROI5_REGISTER0 
    MDBC_Register_00cc;

    //0x00d0
    T_ROI5_REGISTER1 
    MDBC_Register_00d0;

    //0x00d4
    T_ROI5_REGISTER2 
    MDBC_Register_00d4;

    //0x00d8
    T_ROI5_REGISTER3 
    MDBC_Register_00d8;

    //0x00dc
    T_ROI6_REGISTER0 
    MDBC_Register_00dc;

    //0x00e0
    T_ROI6_REGISTER1 
    MDBC_Register_00e0;

    //0x00e4
    T_ROI6_REGISTER2 
    MDBC_Register_00e4;

    //0x00e8
    T_ROI6_REGISTER3 
    MDBC_Register_00e8;

    //0x00ec
    T_ROI7_REGISTER0 
    MDBC_Register_00ec;

    //0x00f0
    T_ROI7_REGISTER1 
    MDBC_Register_00f0;

    //0x00f4
    T_ROI7_REGISTER2 
    MDBC_Register_00f4;

    //0x00f8
    T_ROI7_REGISTER3 
    MDBC_Register_00f8;

    //0x00fc
    T_LL_FRAME_REGISTER0 
    MDBC_Register_00fc;

    //0x0100
    T_LL_FRAME_REGISTER1 
    MDBC_Register_0100;

    //0x0104
    T_LL_FRAME_REGISTER2 
    MDBC_Register_0104;

    //0x0108
    T_LL_FRAME_REGISTER3 
    MDBC_Register_0108;

    //0x010c
    T_LL_CONTROL_REGISTER 
    MDBC_Register_010c;

    //0x0110
    T_DMA_DISABLE_REGISTER0 
    MDBC_Register_0110;

    //0x0114
    T_RESERVED_REGISTER0 
    MDBC_Register_0114;

    //0x0118
    T_RESERVED_REGISTER1 
    MDBC_Register_0118;

    //0x011c
    T_RESERVED_REGISTER2 
    MDBC_Register_011c;

    //0x0120
    T_CHECK_SUM_REGISTER0 
    MDBC_Register_0120;

    //0x0124
    T_CHECK_SUM_REGISTER1 
    MDBC_Register_0124;

    //0x0128
    T_CHECK_SUM_REGISTER2 
    MDBC_Register_0128;

    //0x012c
    T_CHECK_SUM_REGISTER3 
    MDBC_Register_012c;

    //0x0130
    T_CHECK_SUM_REGISTER4 
    MDBC_Register_0130;

    //0x0134
    T_CHECK_SUM_REGISTER5 
    MDBC_Register_0134;

    //0x0138
    T_CHECK_SUM_REGISTER6 
    MDBC_Register_0138;

    //0x013c
    T_CHECK_SUM_REGISTER7 
    MDBC_Register_013c;

    //0x0140
    T_CHECK_SUM_REGISTER8 
    MDBC_Register_0140;

    //0x0144
    T_CHECK_SUM_REGISTER9 
    MDBC_Register_0144;

    //0x0148
    T_CHECK_SUM_REGISTER10 
    MDBC_Register_0148;

    //0x014c
    T_CHECK_SUM_REGISTER11 
    MDBC_Register_014c;

} NT98528_MDBC_REG_STRUCT;

#endif