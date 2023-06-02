#ifndef _IQ_REG_H_
#define _IQ_REG_H_

#include "mach/rcw_macro.h"

// NOTE: SIE
//=============================================================================
// SIE Dgain
//=============================================================================
/*
    PATGEN_EN      :    [0x0, 0x1],            bits : 1
    DVI_EN         :    [0x0, 0x1],            bits : 2
    OB_AVG_EN      :    [0x0, 0x1],            bits : 3
    OB_SUB_SEL     :    [0x0, 0x1],            bits : 4
    OB_BYPASS_EN   :    [0x0, 0x1],            bits : 5
    OB_FRAME_AVG_EN:    [0x0, 0x1],            bits : 6
    OB_PLANE_SUB_EN:    [0x0, 0x1],            bits : 7
    SHDR_YOUT_EN   :    [0x0, 0x1],            bits : 8
    CGAIN_EN       :    [0x0, 0x1],            bits : 9
    GRID_LINE_EN   :    [0x0, 0x1],            bits : 10
    DPC_EN         :    [0x0, 0x1],            bits : 11
    PFPC_EN        :    [0x0, 0x1],            bits : 13
    ECS_EN         :    [0x0, 0x1],            bits : 15
    DGAIN_EN       :    [0x0, 0x1],            bits : 16
    BS_H_EN        :    [0x0, 0x1],            bits : 17
    BS_V_EN        :    [0x0, 0x1],            bits : 18
    RAWENC_EN      :    [0x0, 0x1],            bits : 19
    STCS_HISTO_Y_EN:    [0x0, 0x1],            bits : 21
    STCS_LA_EN     :    [0x0, 0x1],            bits : 23
    STCS_CA_EN     :    [0x0, 0x1],            bits : 24
    STCS_VA_EN     :    [0x0, 0x1],            bits : 25
    STCS_ETH_EN    :    [0x0, 0x1],            bits : 26
    DRAM_OUT0_EN   :    [0x0, 0x1],            bits : 27
    DRAM_OUT3_EN   :    [0x0, 0x1],            bits : 28
    BAYER_FORMAT   :    [0x0, 0x1],            bits : 30_29
*/
#define R4_ENGINE_FUNCTION_OFS 0x0004
REGDEF_BEGIN(R4_ENGINE_FUNCTION)
REGDEF_BIT(,        1)
REGDEF_BIT(PATGEN_EN,        1)
REGDEF_BIT(DVI_EN,        1)
REGDEF_BIT(OB_AVG_EN,        1)
REGDEF_BIT(OB_SUB_SEL,        1)
REGDEF_BIT(OB_BYPASS_EN,        1)
REGDEF_BIT(OB_FRAME_AVG_EN,        1)
REGDEF_BIT(OB_PLANE_SUB_EN,        1)
REGDEF_BIT(SHDR_YOUT_EN,        1)
REGDEF_BIT(CGAIN_EN,        1)
REGDEF_BIT(GRID_LINE_EN,        1)
REGDEF_BIT(DPC_EN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(PFPC_EN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ECS_EN,        1)
REGDEF_BIT(DGAIN_EN,        1)
REGDEF_BIT(BS_H_EN,        1)
REGDEF_BIT(BS_V_EN,        1)
REGDEF_BIT(RAWENC_EN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(STCS_HISTO_Y_EN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(STCS_LA_EN,        1)
REGDEF_BIT(STCS_CA_EN,        1)
REGDEF_BIT(STCS_VA_EN,        1)
REGDEF_BIT(STCS_ETH_EN,        1)
REGDEF_BIT(DRAM_OUT0_EN,        1)
REGDEF_BIT(COMPANDING_EN,        1)
REGDEF_BIT(BAYER_FORMAT,        2)
REGDEF_BIT(ECS_BAYER_MODE,        1)
REGDEF_END(R4_ENGINE_FUNCTION)

/*
    CRP_SZX:    [0x0, 0x3fff],            bits : 13_0
    CRP_SZY:    [0x0, 0x3fff],            bits : 29_16
*/
#define R38_ENGINE_TIMING_OFS 0x0038
REGDEF_BEGIN(R38_ENGINE_TIMING)
REGDEF_BIT(CRP_SZX,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(CRP_SZY,        14)
REGDEF_END(R38_ENGINE_TIMING)

/*
    DRAM_IN1_SAI:    [0x0, 0x7ffffff],            bits : 28_2
*/
#define R48_ENGINE_DRAM_OFS 0x0048
REGDEF_BEGIN(R48_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_IN1_SAI,        29)
REGDEF_END(R48_ENGINE_DRAM)

/*
    DRAM_IN2_SAI:    [0x0, 0x7ffffff],            bits : 28_2
*/
#define R50_ENGINE_DRAM_OFS 0x0050
REGDEF_BEGIN(R50_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_IN2_SAI,        29)
REGDEF_END(R50_ENGINE_DRAM)

/*
    OB_OFS   :    [0x0, 0x7ff],            bits : 10_0
    OBDT_GAIN:    [0x0, 0x3f] ,            bits : 21_16
*/
#define RBC_BASIC_OB_OFS 0x00bc
REGDEF_BEGIN(RBC_BASIC_OB)
REGDEF_BIT(OB_OFS,        11)
REGDEF_BIT(,         5)
REGDEF_BIT(OBDT_GAIN,         6)
REGDEF_END(RBC_BASIC_OB)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RD0_DECOMP_OFS 0x00d0
REGDEF_BEGIN(RD0_DECOMP)
REGDEF_BIT(DECOMP_KP0,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_KP1,        12)
REGDEF_END(RD0_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RD4_DECOMP_OFS 0x00d4
REGDEF_BEGIN(RD4_DECOMP)
REGDEF_BIT(DECOMP_KP2,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_KP3,        12)
REGDEF_END(RD4_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RD8_DECOMP_OFS 0x00d8
REGDEF_BEGIN(RD8_DECOMP)
REGDEF_BIT(DECOMP_KP4,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_KP5,        12)
REGDEF_END(RD8_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RDC_DECOMP_OFS 0x00dc
REGDEF_BEGIN(RDC_DECOMP)
REGDEF_BIT(DECOMP_KP6,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_KP7,        12)
REGDEF_END(RDC_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RE0_DECOMP_OFS 0x00e0
REGDEF_BEGIN(RE0_DECOMP)
REGDEF_BIT(DECOMP_KP8,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_KP9,        12)
REGDEF_END(RE0_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RE4_DECOMP_OFS 0x00e4
REGDEF_BEGIN(RE4_DECOMP)
REGDEF_BIT(DECOMP_KP10,        12)
REGDEF_END(RE4_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RE8_DECOMP_OFS 0x00e8
REGDEF_BEGIN(RE8_DECOMP)
REGDEF_BIT(DECOMP_SP0,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_SP1,        12)
REGDEF_END(RE8_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define REC_DECOMP_OFS 0x00ec
REGDEF_BEGIN(REC_DECOMP)
REGDEF_BIT(DECOMP_SP2,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_SP3,        12)
REGDEF_END(REC_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RF0_DECOMP_OFS 0x00f0
REGDEF_BEGIN(RF0_DECOMP)
REGDEF_BIT(DECOMP_SP4,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_SP5,        12)
REGDEF_END(RF0_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RF4_DECOMP_OFS 0x00f4
REGDEF_BEGIN(RF4_DECOMP)
REGDEF_BIT(DECOMP_SP6,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_SP7,        12)
REGDEF_END(RF4_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RF8_DECOMP_OFS 0x00f8
REGDEF_BEGIN(RF8_DECOMP)
REGDEF_BIT(DECOMP_SP8,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_SP9,        12)
REGDEF_END(RF8_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RFC_DECOMP_OFS 0x00fc
REGDEF_BEGIN(RFC_DECOMP)
REGDEF_BIT(DECOMP_SP10,        12)
REGDEF_BIT(          ,         4)
REGDEF_BIT(DECOMP_SP11,        12)
REGDEF_END(RFC_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R100_DECOMP_OFS 0x0100
REGDEF_BEGIN(R100_DECOMP)
REGDEF_BIT(DECOMP_SB0,         4)
REGDEF_BIT(DECOMP_SB1 ,        4)
REGDEF_BIT(DECOMP_SB2,         4)
REGDEF_BIT(DECOMP_SB3,         4)
REGDEF_BIT(DECOMP_SB4,         4)
REGDEF_BIT(DECOMP_SB5,         4)
REGDEF_BIT(DECOMP_SB6,         4)
REGDEF_BIT(DECOMP_SB7,         4)
REGDEF_END(R100_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R104_DECOMP_OFS 0x0104
REGDEF_BEGIN(R104_DECOMP)
REGDEF_BIT(DECOMP_SB8,         4)
REGDEF_BIT(DECOMP_SB9 ,        4)
REGDEF_BIT(DECOMP_SB10,        4)
REGDEF_BIT(DECOMP_SB11,        4)
REGDEF_END(R104_DECOMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R108_COMP_OFS 0x0108
REGDEF_BEGIN(R108_COMP)
REGDEF_BIT(COMP_KP0,        5)
REGDEF_BIT(        ,        3)
REGDEF_BIT(COMP_KP1,        5)
REGDEF_BIT(        ,        3)
REGDEF_BIT(COMP_KP2,        5)
REGDEF_BIT(        ,        3)
REGDEF_BIT(COMP_KP3,        5)
REGDEF_END(R108_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R10C_COMP_OFS 0x010c
REGDEF_BEGIN(R10C_COMP)
REGDEF_BIT(COMP_KP4,        5)
REGDEF_BIT(        ,        3)
REGDEF_BIT(COMP_KP5,        5)
REGDEF_BIT(        ,        3)
REGDEF_BIT(COMP_KP6,        5)
REGDEF_BIT(        ,        3)
REGDEF_BIT(COMP_KP7,        5)
REGDEF_END(R10C_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R110_COMP_OFS 0x0110
REGDEF_BEGIN(R110_COMP)
REGDEF_BIT(COMP_KP8,        5)
REGDEF_BIT(        ,        3)
REGDEF_BIT(COMP_KP9,        5)
REGDEF_BIT(        ,        3)
REGDEF_BIT(COMP_KP10,       5)
REGDEF_END(R110_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R114_COMP_OFS 0x0114
REGDEF_BEGIN(R114_COMP)
REGDEF_BIT(COMP_SP0,       12)
REGDEF_BIT(        ,        4)
REGDEF_BIT(COMP_SP1,       12)
REGDEF_END(R114_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R118_COMP_OFS 0x0118
REGDEF_BEGIN(R118_COMP)
REGDEF_BIT(COMP_SP2,       12)
REGDEF_BIT(        ,        4)
REGDEF_BIT(COMP_SP3,       12)
REGDEF_END(R118_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R11C_COMP_OFS 0x011c
REGDEF_BEGIN(R11C_COMP)
REGDEF_BIT(COMP_SP4,       12)
REGDEF_BIT(        ,        4)
REGDEF_BIT(COMP_SP5,       12)
REGDEF_END(R11C_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R120_COMP_OFS 0x0120
REGDEF_BEGIN(R120_COMP)
REGDEF_BIT(COMP_SP6,       12)
REGDEF_BIT(        ,        4)
REGDEF_BIT(COMP_SP7,       12)
REGDEF_END(R120_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R124_COMP_OFS 0x0124
REGDEF_BEGIN(R124_COMP)
REGDEF_BIT(COMP_SP8,       12)
REGDEF_BIT(        ,        4)
REGDEF_BIT(COMP_SP9,       12)
REGDEF_END(R124_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R128_COMP_OFS 0x0128
REGDEF_BEGIN(R128_COMP)
REGDEF_BIT(COMP_SP10,       12)
REGDEF_BIT(        ,        4)
REGDEF_BIT(COMP_SP11,       12)
REGDEF_END(R128_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R12C_COMP_OFS 0x012c
REGDEF_BEGIN(R12C_COMP)
REGDEF_BIT(COMP_SB0,        4)
REGDEF_BIT(COMP_SB1,        4)
REGDEF_BIT(COMP_SB2,        4)
REGDEF_BIT(COMP_SB3,        4)
REGDEF_BIT(COMP_SB4,        4)
REGDEF_BIT(COMP_SB5,        4)
REGDEF_BIT(COMP_SB6,        4)
REGDEF_BIT(COMP_SB7,        4)
REGDEF_END(R12C_COMP)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R130_COMP_OFS 0x0130
REGDEF_BEGIN(R130_COMP)
REGDEF_BIT(COMP_SB8,        4)
REGDEF_BIT(COMP_SB9,        4)
REGDEF_BIT(COMP_SB10,       4)
REGDEF_BIT(COMP_SB11,       4)
REGDEF_END(R130_COMP)

/*
    CGAIN_RGAIN:    [0x0, 0x3ff],            bits : 9_0
    CGAIN_GRGAIN:    [0x0, 0x3ff],            bits : 25_16
*/
#define BASIC_CG_0_OFS 0x01b4
REGDEF_BEGIN(BASIC_CG_0)
REGDEF_BIT(CGAIN_RGAIN,    10)
REGDEF_BIT(,        6)
REGDEF_BIT(CGAIN_GRGAIN,    10)
REGDEF_END(BASIC_CG_0)

/*
    CGAIN_GBGAIN    :    [0x0, 0x3ff],          bits : 9_0
    CGAIN_BGAIN    :    [0x0, 0x3ff],          bits : 25_16
*/
#define BASIC_CG_1_OFS 0x01b8
REGDEF_BEGIN(BASIC_CG_1)
REGDEF_BIT(CGAIN_GBGAIN,   10)
REGDEF_BIT(,        6)
REGDEF_BIT(CGAIN_BGAIN,    10)
REGDEF_END(BASIC_CG_1)

/*
    CGAIN_IRGAIN    :    [0x0, 0x3ff],          bits : 25_16
    CGAIN_LEVEL_SEL:    [0x0, 0x1],            bits : 28
*/
#define BASIC_CG_2_OFS 0x01bc
REGDEF_BEGIN(BASIC_CG_2)
REGDEF_BIT(CGAIN_IRGAIN, 10)
REGDEF_BIT(,         6)
REGDEF_BIT(CGAIN_LEVEL_SEL,  1)
REGDEF_END(BASIC_CG_2)

/*
    CRP_SZX:    [0x0, 0x3fff],            bits : 13_0
    CRP_SZY:    [0x0, 0x3fff],            bits : 29_16
*/
#define R38_ENGINE_TIMING_OFS 0x0038
REGDEF_BEGIN(ENGINE_TIMING)
REGDEF_BIT(CRP_SZX,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(CRP_SZY,        14)
REGDEF_END(ENGINE_TIMING)

/*
    DGAIN_GAIN:    [0x0, 0x3ff],            bits : 9_0
*/
#define R15C_BASIC_DGAIN_OFS 0x015c
REGDEF_BEGIN(R15C_BASIC_DGAIN)
REGDEF_BIT(DGAIN_GAIN,        16)
REGDEF_END(R15C_BASIC_DGAIN)


/*
    LA_CG_RGAIN    :    [0x0, 0x3ff],          bits : 9_0
    LA_CG_GGAIN    :    [0x0, 0x3ff],          bits : 25_16
*/
#define STCS_LA_CG_0_OFS 0x01ec
REGDEF_BEGIN(STCS_LA_CG_0)
REGDEF_BIT(LA_CG_RGAIN, 10)
REGDEF_BIT(,         6)
REGDEF_BIT(LA_CG_GGAIN,  10)
REGDEF_END(STCS_LA_CG_0)

/*
    STCS_VIG_EN               :    [0x0, 0x1],            bits : 1
    STCS_LA_CG_EN             :    [0x0, 0x1],            bits : 2
    STCS_GAMMA_1_EN           :    [0x0, 0x1],            bits : 3
    STCS_GAMMA_2_EN           :    [0x0, 0x1],            bits : 4
    STCS_HISTO_Y_SEL          :    [0x0, 0x1],            bits : 5
    STCS_CA_TH_EN             :    [0x0, 0x1],            bits : 6
    STCS_VA_CG_EN             :    [0x0, 0x1],            bits : 7
    STCS_LA_RGB2Y_SEL         :    [0x0, 0x1],            bits : 8
    STCS_VA_VUG_PAD_LINE_INTVL:    [0x0, 0x3],            bits : 31_30
*/
#define R1E8_STCS_OFS 0x01e8
REGDEF_BEGIN(R1E8_STCS)
REGDEF_BIT(,        1)
REGDEF_BIT(STCS_VIG_EN,        1)
REGDEF_BIT(STCS_LA_CG_EN,        1)
REGDEF_BIT(STCS_GAMMA_1_EN,        1)
REGDEF_BIT(STCS_GAMMA_2_EN,        1)
REGDEF_BIT(STCS_HISTO_Y_SEL,        1)
REGDEF_BIT(STCS_CA_TH_EN,        1)
REGDEF_BIT(STCS_VA_CG_EN,        1)
REGDEF_BIT(STCS_LA_RGB2Y_SEL,        1)
REGDEF_BIT(STCS_LA_RGB2Y1_MOD,        1)
REGDEF_BIT(STCS_LA_RGB2Y2_MOD,        1)
REGDEF_BIT(STCS_LA_TH_EN,        1)
REGDEF_BIT(STCS_CA_ACCM_SRC,		 1)
REGDEF_BIT(,		3)
REGDEF_BIT(STCS_COMPANDING_SHIFT,	 4)
REGDEF_BIT(,       12)
REGDEF_END(R1E8_STCS)


/*
    LA_CG_BGAIN    :    [0x0, 0x3ff],          bits : 9_0
*/
#define STCS_LA_CG_1_OFS 0x01f0
REGDEF_BEGIN(STCS_LA_CG_1)
REGDEF_BIT(LA_CG_BGAIN, 10)
REGDEF_END(STCS_LA_CG_1)

/*
    SMPL_X_FACT:    [0x0, 0x1fff],            bits : 12_0
    SMPL_Y_FACT:    [0x0, 0x1fff],            bits : 28_16
*/
#define R224_STCS_CA_OFS 0x0224
REGDEF_BEGIN(R224_STCS_CA)
REGDEF_BIT(SMPL_X_FACT,        13)
REGDEF_BIT(,         3)
REGDEF_BIT(SMPL_Y_FACT,        13)
REGDEF_END(R224_STCS_CA)

/*
    CA_CROP_SZX:    [0x0, 0x3fff],            bits : 13_0
    CA_CROP_SZY:    [0x0, 0x3fff],            bits : 29_16
*/
#define R228_STCS_CA_OFS 0x0228
REGDEF_BEGIN(R228_STCS_CA)
REGDEF_BIT(CA_CROP_SZX,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(CA_CROP_SZY,        14)
REGDEF_END(R228_STCS_CA)

/*
    CA_WIN_SZX:    [0x0, 0xff],            bits : 7_0
    CA_WIN_SZY:    [0x0, 0xff],            bits : 15_8
*/
#define STCS_CA_WIN_SZ_OFS 0x023c
REGDEF_BEGIN(STCS_CA_WIN_SZ)
REGDEF_BIT(CA_WIN_SZX,        8)
REGDEF_BIT(CA_WIN_SZY,        8)
REGDEF_END(STCS_CA_WIN_SZ)

/*
    CA_WIN_NUMX:    [0x0, 0x1f],            bits : 4_0
    CA_WIN_NUMY:    [0x0, 0x1f],            bits : 12_8
*/
#define STCS_CA_WIN_NUM_OFS 0x0240
REGDEF_BEGIN(STCS_CA_WIN_NUM)
REGDEF_BIT(CA_WIN_NUMX,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(CA_WIN_NUMY,        5)
REGDEF_END(STCS_CA_WIN_NUM)

/*
    STCS_CA_OB_OFS:    [0x0, 0xFff],            bits : 11_0
    STCS_LA_OB_OFS:    [0x0, 0xFff],            bits : 23_12
*/
#define R2A8_STCS_OB_OFS 0x02a8
REGDEF_BEGIN(R2A8_STCS_OB)
REGDEF_BIT(STCS_CA_OB_OFS,        12)
REGDEF_BIT(STCS_LA_OB_OFS,        12)
REGDEF_END(R2A8_STCS_OB)

/*
    LA_WIN_SZX:    [0x0, 0xff],            bits : 7_0
    LA_WIN_SZY:    [0x0, 0xff],            bits : 15_8
*/
#define STCS_LA_WIN_SZ_OFS 0x02ac
REGDEF_BEGIN(STCS_LA_WIN_SZ)
REGDEF_BIT(LA_WIN_SZX,        8)
REGDEF_BIT(LA_WIN_SZY,        8)
REGDEF_END(STCS_LA_WIN_SZ)

/*
    LA_WIN_NUMX:    [0x0, 0x1f],            bits : 4_0
    LA_WIN_NUMY:    [0x0, 0x1f],            bits : 12_8
    LA1_WIN_SUM:    [0x0, 0x3ffff],            bits : 31_14
*/
#define STCS_LA_WIN_NUM_OFS 0x02b0
REGDEF_BEGIN(STCS_LA_WIN_NUM)
REGDEF_BIT(LA_WIN_NUMX,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(LA_WIN_NUMY,        5)
REGDEF_BIT(,        1)
REGDEF_BIT(LA1_WIN_SUM,        18)
REGDEF_END(STCS_LA_WIN_NUM)

// NOTE: IFE
//=============================================================================
// IFE enable
//=============================================================================
/*
    IFE_MODE           :    [0x0, 0x3],         bits : 1_0
    INBIT_DEPTH        :    [0x0, 0x3],         bits : 6_5
    OUTBIT_DEPTH       :    [0x0, 0x3],         bits : 8_7
    CFAPAT             :    [0x0, 0x7],         bits : 11_9
    IFE_FILT_MODE      :    [0x0, 0x1],         bits : 12
    IFE_OUTL_EN        :    [0x0, 0x1],         bits : 13
    IFE_FILTER_EN      :    [0x0, 0x1],         bits : 14
    IFE_CGAIN_EN       :    [0x0, 0x1],         bits : 15
    IFE_VIG_EN         :    [0x0, 0x1],         bits : 16
    IFE_GBAL_EN        :    [0x0, 0x1],         bits : 17
    IFE_BINNING        :    [0x0, 0x7],         bits : 20_18
    BAYER_FORMAT       :    [0x0, 0x1],         bits : 21
    IFE_RGBIR_RB_NRFILL:    [0x0, 0x1],         bits : 22
    IFE_BILAT_TH_EN    :    [0x0, 0x1],         bits : 23
    IFE_F_NRS_EN       :    [0x0, 0x1],         bits : 24
    IFE_F_CG_EN        :    [0x0, 0x1],         bits : 25
    IFE_F_FUSION_EN    :    [0x0, 0x1],         bits : 26
    IFE_F_FUSION_FNUM  :    [0x0, 0x3],         bits : 28_27
    IFE_F_FC_EN        :    [0x0, 0x1],         bits : 29
    MIRROR_EN          :    [0x0, 0x1],         bits : 30
    IFE_R_DECODE_EN    :    [0x0, 0x1],         bits : 31
*/
#define IFE_EN_REGISTER_OFS 0x0004
REGDEF_BEGIN(IFE_EN_REGISTER)
REGDEF_BIT(ife_mode,        2)
REGDEF_BIT(,        3)
REGDEF_BIT(inbit_depth,        2)
REGDEF_BIT(outbit_depth,        2)
REGDEF_BIT(cfapat,        3)
REGDEF_BIT(filt_mode,        1)
REGDEF_BIT(outl_en,        1)
REGDEF_BIT(filter_en,        1)
REGDEF_BIT(cgain_en,        1)
REGDEF_BIT(vig_en,        1)
REGDEF_BIT(gbal_en,        1)
REGDEF_BIT(binning,        3)
REGDEF_BIT(bayer_fmt,        1)
REGDEF_BIT(rgbir_rb_nrfill,        1)
REGDEF_BIT(bilat_th_en,        1)
REGDEF_BIT(f_nrs_en,        1)
REGDEF_BIT(f_cg_en,        1)
REGDEF_BIT(f_fusion_en,        1)
REGDEF_BIT(f_fusion_fnum,        2)
REGDEF_BIT(f_fc_en,        1)
REGDEF_BIT(mirror_en,        1)
REGDEF_BIT(r_decode_en,        1)
REGDEF_END(IFE_EN_REGISTER)

//=============================================================================
// IFE OB offset / CGain
//=============================================================================
/*
    cgain_inv        :    [0x0, 0x1],           bits : 0
    cgain_hinv       :    [0x0, 0x1],           bits : 1
    cgain_range      :    [0x0, 0x1],           bits : 2
    ife_f_cgain_range:    [0x0, 0x1],           bits : 3
    cgain_mask       :    [0x0, 0xfff],         bits : 19_8
*/
#define IFE_CGAIN_CONTROL_OFS 0x0070
REGDEF_BEGIN(IFE_CGAIN_CONTROL)
REGDEF_BIT(cgain_inv,        1)
REGDEF_BIT(cgain_hinv,        1)
REGDEF_BIT(cgain_range,        1)
REGDEF_BIT(ife_f_cgain_range,        1)
REGDEF_BIT(,        4)
REGDEF_BIT(cgain_mask,        12)
REGDEF_END(IFE_CGAIN_CONTROL)

/*
    ife_cgain_r :    [0x0, 0x3ff],          bits : 9_0
    ife_cgain_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define IFE_CGAIN_0_OFS 0x0074
REGDEF_BEGIN(IFE_CGAIN_0)
REGDEF_BIT(ife_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cgain_gr,        10)
REGDEF_END(IFE_CGAIN_0)

/*
    ife_cgain_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_cgain_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define IFE_CGAIN_1_OFS 0x0078
REGDEF_BEGIN(IFE_CGAIN_1)
REGDEF_BIT(ife_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cgain_b,        10)
REGDEF_END(IFE_CGAIN_1)

/*
    ife_cgain_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define IFE_CGAIN_2_OFS 0x007c
REGDEF_BEGIN(IFE_CGAIN_2)
REGDEF_BIT(ife_cgain_ir,        10)
REGDEF_END(IFE_CGAIN_2)

/*
    ife_f_p0_cgain_r :    [0x0, 0x3ff],         bits : 9_0
    ife_f_p0_cgain_gr:    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_4_OFS 0x0080
REGDEF_BEGIN(COLOR_GAIN_REGISTER_4)
REGDEF_BIT(ife_f_p0_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cgain_gr,        10)
REGDEF_END(COLOR_GAIN_REGISTER_4)

/*
    ife_f_p0_cgain_gb:    [0x0, 0x3ff],         bits : 9_0
    ife_f_p0_cgain_b :    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_5_OFS 0x0084
REGDEF_BEGIN(COLOR_GAIN_REGISTER_5)
REGDEF_BIT(ife_f_p0_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cgain_b,        10)
REGDEF_END(COLOR_GAIN_REGISTER_5)

/*
    ife_f_p0_cgain_ir:    [0x0, 0x3ff],         bits : 9_0
*/
#define COLOR_GAIN_REGISTER_6_OFS 0x0088
REGDEF_BEGIN(COLOR_GAIN_REGISTER_6)
REGDEF_BIT(ife_f_p0_cgain_ir,        10)
REGDEF_END(COLOR_GAIN_REGISTER_6)

/*
    ife_f_p1_cgain_r :    [0x0, 0x3ff],         bits : 9_0
    ife_f_p1_cgain_gr:    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_7_OFS 0x008c
REGDEF_BEGIN(COLOR_GAIN_REGISTER_7)
REGDEF_BIT(ife_f_p1_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cgain_gr,        10)
REGDEF_END(COLOR_GAIN_REGISTER_7)

/*
    ife_f_p1_cgain_gb:    [0x0, 0x3ff],         bits : 9_0
    ife_f_p1_cgain_b :    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_8_OFS 0x0090
REGDEF_BEGIN(COLOR_GAIN_REGISTER_8)
REGDEF_BIT(ife_f_p1_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cgain_b,        10)
REGDEF_END(COLOR_GAIN_REGISTER_8)

/*
    ife_f_p1_cgain_ir:    [0x0, 0x3ff],         bits : 9_0
*/
#define COLOR_GAIN_REGISTER_9_OFS 0x0094
REGDEF_BEGIN(COLOR_GAIN_REGISTER_9)
REGDEF_BIT(ife_f_p1_cgain_ir,        10)
REGDEF_END(COLOR_GAIN_REGISTER_9)

/*
    ife_cofs_r :    [0x0, 0x3ff],           bits : 9_0
    ife_cofs_gr:    [0x0, 0x3ff],           bits : 25_16
*/
#define OB_OFS_0 0x00b0
REGDEF_BEGIN(OB_REGISTER_0)
REGDEF_BIT(ife_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cofs_gr,        10)
REGDEF_END(OB_REGISTER_0)

/*
    ife_cofs_gb:    [0x0, 0x3ff],           bits : 9_0
    ife_cofs_b :    [0x0, 0x3ff],           bits : 25_16
*/
#define OB_OFS_1 0x00b4
REGDEF_BEGIN(OB_REGISTER_1)
REGDEF_BIT(ife_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cofs_b,        10)
REGDEF_END(OB_REGISTER_1)

/*
    ife_cofs_ir:    [0x0, 0x3ff],           bits : 9_0
*/
#define OB_OFS_2 0x00b8
REGDEF_BEGIN(OB_REGISTER_2)
REGDEF_BIT(ife_cofs_ir,        10)
REGDEF_END(OB_REGISTER_2)

/*
    ife_f_p0_cofs_r :    [0x0, 0x3ff],          bits : 9_0
    ife_f_p0_cofs_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_3_OFS 0x00bc
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_3)
REGDEF_BIT(ife_f_p0_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cofs_gr,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_3)

/*
    ife_f_p0_cofs_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_f_p0_cofs_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_4_OFS 0x00c0
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_4)
REGDEF_BIT(ife_f_p0_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cofs_b,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_4)

/*
    ife_f_p0_cofs_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define COLOR_OFFSET_REGISTER_5_OFS 0x00c4
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_5)
REGDEF_BIT(ife_f_p0_cofs_ir,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_5)

/*
    ife_f_p1_cofs_r :    [0x0, 0x3ff],          bits : 9_0
    ife_f_p1_cofs_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_6_OFS 0x00c8
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_6)
REGDEF_BIT(ife_f_p1_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cofs_gr,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_6)

/*
    ife_f_p1_cofs_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_f_p1_cofs_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_7_OFS 0x00cc
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_7)
REGDEF_BIT(ife_f_p1_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cofs_b,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_7)

/*
    ife_f_p1_cofs_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define COLOR_OFFSET_REGISTER_8_OFS 0x00d0
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_8)
REGDEF_BIT(ife_f_p1_cofs_ir,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_8)

//=============================================================================
// IFE OUTLIER
//=============================================================================
/*
    ife_outlth_bri0 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark0:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_REGISTER_0_OFS 0x0190
REGDEF_BEGIN(OUTLIER_REGISTER_0)
REGDEF_BIT(ife_outlth_bri0,        12)
REGDEF_BIT(ife_outlth_dark0,        12)
REGDEF_END(OUTLIER_REGISTER_0)

/*
    ife_outlth_bri1 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark1:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_REGISTER_1_OFS 0x0194
REGDEF_BEGIN(OUTLIER_REGISTER_1)
REGDEF_BIT(ife_outlth_bri1,        12)
REGDEF_BIT(ife_outlth_dark1,        12)
REGDEF_END(OUTLIER_REGISTER_1)

/*
    ife_outlth_bri2 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark2:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_REGISTER_2_OFS 0x0198
REGDEF_BEGIN(OUTLIER_REGISTER_2)
REGDEF_BIT(ife_outlth_bri2,        12)
REGDEF_BIT(ife_outlth_dark2,        12)
REGDEF_END(OUTLIER_REGISTER_2)

/*
    ife_outlth_bri3 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark3:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_REGISTER_3_OFS 0x019c
REGDEF_BEGIN(OUTLIER_REGISTER_3)
REGDEF_BIT(ife_outlth_bri3,        12)
REGDEF_BIT(ife_outlth_dark3,        12)
REGDEF_END(OUTLIER_REGISTER_3)

/*
    ife_outlth_bri4 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark4:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_REGISTER_4_OFS 0x01a0
REGDEF_BEGIN(OUTLIER_REGISTER_4)
REGDEF_BIT(ife_outlth_bri4,        12)
REGDEF_BIT(ife_outlth_dark4,        12)
REGDEF_END(OUTLIER_REGISTER_4)

/*
    ife_outl_avg_mode:    [0x0, 0x1],           bits : 0
    ife_outl_weight  :    [0x0, 0xff],          bits : 15_8
    ife_outl_cnt1    :    [0x0, 0x1f],          bits : 20_16
    ife_outl_cnt2    :    [0x0, 0x1f],          bits : 28_24
*/
#define OUTLIER_REGISTER_5_OFS 0x01a4
REGDEF_BEGIN(OUTLIER_REGISTER_5)
REGDEF_BIT(ife_outl_avg_mode,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(ife_outl_weight,        8)
REGDEF_BIT(ife_outl_cnt1,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_outl_cnt2,        5)
REGDEF_END(OUTLIER_REGISTER_5)

/*
    IFE_ORD_RANGE_BRI :    [0x0, 0x7],          bits : 2_0
    ife_ord_range_dark:    [0x0, 0x7],          bits : 6_4
    ife_ord_protect_th:    [0x0, 0x3ff],            bits : 17_8
    IFE_ORD_BLEND_W   :    [0x0, 0xff],         bits : 27_20
*/
#define OUTLIER_REGISTER_6_OFS 0x01a8
REGDEF_BEGIN(OUTLIER_REGISTER_6)
REGDEF_BIT(ife_ord_range_bright,     3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_ord_range_dark,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_ord_protect_th,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_ord_blend_weight,      8)
REGDEF_END(OUTLIER_REGISTER_6)

//=============================================================================
// IFE NR_GBalance
//=============================================================================
/*
    ife_gbal_edge_protect_en:    [0x0, 0x1],            bits : 0
    ife_gbal_diff_thr_str   :    [0x0, 0x3ff],          bits : 17_8
    ife_gbal_diff_w_max     :    [0x0, 0xf],            bits : 23_20
*/
#define GBAL_REGISTER_0_OFS 0x01b4
REGDEF_BEGIN(GBAL_REGISTER_0)
REGDEF_BIT(ife_gbal_edge_protect_en,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(ife_gbal_diff_thr_str,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_diff_w_max,        4)
REGDEF_END(GBAL_REGISTER_0)

/*
    ife_gbal_edge_thr_1:    [0x0, 0x3ff],           bits : 9_0
    ife_gbal_edge_thr_0:    [0x0, 0x3ff],           bits : 25_16
*/
#define GBAL_REGISTER_1_OFS 0x01b8
REGDEF_BEGIN(GBAL_REGISTER_1)
REGDEF_BIT(ife_gbal_edge_thr_1,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_gbal_edge_thr_0,        10)
REGDEF_END(GBAL_REGISTER_1)

//=============================================================================
// IFE 2DNR Filter_TH
//=============================================================================
/*
    ife_rth_nlm_c0_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_R_TH_0_OFS 0x0110
REGDEF_BEGIN(RANGEA_R_TH_0)
REGDEF_BIT(ife_rth_nlm_c0_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_1,        10)
REGDEF_END(RANGEA_R_TH_0)

/*
    ife_rth_nlm_c0_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_R_TH_1_OFS 0x0114
REGDEF_BEGIN(RANGEA_R_TH_1)
REGDEF_BIT(ife_rth_nlm_c0_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_3,        10)
REGDEF_END(RANGEA_R_TH_1)

/*
    ife_rth_nlm_c0_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_R_TH_2_OFS 0x0118
REGDEF_BEGIN(RANGEA_R_TH_2)
REGDEF_BIT(ife_rth_nlm_c0_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_5,        10)
REGDEF_END(RANGEA_R_TH_2)

/*
    ife_rth_nlm_c1_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gr_TH_0_OFS 0x0120
REGDEF_BEGIN(RANGEA_Gr_TH_0)
REGDEF_BIT(ife_rth_nlm_c1_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_1,        10)
REGDEF_END(RANGEA_Gr_TH_0)

/*
    ife_rth_nlm_c1_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gr_TH_1_OFS 0x0124
REGDEF_BEGIN(RANGEA_Gr_TH_1)
REGDEF_BIT(ife_rth_nlm_c1_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_3,        10)
REGDEF_END(RANGEA_Gr_TH_1)

/*
    ife_rth_nlm_c1_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gr_TH_2_OFS 0x0128
REGDEF_BEGIN(RANGEA_Gr_TH_2)
REGDEF_BIT(ife_rth_nlm_c1_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_5,        10)
REGDEF_END(RANGEA_Gr_TH_2)

/*
    ife_rth_nlm_c2_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gb_TH_0_OFS 0x0130
REGDEF_BEGIN(RANGEA_Gb_TH_0)
REGDEF_BIT(ife_rth_nlm_c2_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_1,        10)
REGDEF_END(RANGEA_Gb_TH_0)

/*
    ife_rth_nlm_c2_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gb_TH_1_OFS 0x0134
REGDEF_BEGIN(RANGEA_Gb_TH_1)
REGDEF_BIT(ife_rth_nlm_c2_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_3,        10)
REGDEF_END(RANGEA_Gb_TH_1)

/*
    ife_rth_nlm_c2_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gb_TH_2_OFS 0x0138
REGDEF_BEGIN(RANGEA_Gb_TH_2)
REGDEF_BIT(ife_rth_nlm_c2_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_5,        10)
REGDEF_END(RANGEA_Gb_TH_2)

/*
    ife_rth_nlm_c3_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_B_TH_0_OFS 0x0140
REGDEF_BEGIN(RANGEA_B_TH_0)
REGDEF_BIT(ife_rth_nlm_c3_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_1,        10)
REGDEF_END(RANGEA_B_TH_0)

/*
    ife_rth_nlm_c3_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_B_TH_1_OFS 0x0144
REGDEF_BEGIN(RANGEA_B_TH_1)
REGDEF_BIT(ife_rth_nlm_c3_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_3,        10)
REGDEF_END(RANGEA_B_TH_1)

/*
    ife_rth_nlm_c3_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_B_TH_2_OFS 0x0148
REGDEF_BEGIN(RANGEA_B_TH_2)
REGDEF_BIT(ife_rth_nlm_c3_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_5,        10)
REGDEF_END(RANGEA_B_TH_2)

/*
    ife_rth_bilat_c0_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_R_TH_0_OFS 0x0150
REGDEF_BEGIN(RANGEB_R_TH_0)
REGDEF_BIT(ife_rth_bilat_c0_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_1,        10)
REGDEF_END(RANGEB_R_TH_0)

/*
    ife_rth_bilat_c0_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_R_TH_1_OFS 0x0154
REGDEF_BEGIN(RANGEB_R_TH_1)
REGDEF_BIT(ife_rth_bilat_c0_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_3,        10)
REGDEF_END(RANGEB_R_TH_1)

/*
    ife_rth_bilat_c0_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_R_TH_2_OFS 0x0158
REGDEF_BEGIN(RANGEB_R_TH_2)
REGDEF_BIT(ife_rth_bilat_c0_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_5,        10)
REGDEF_END(RANGEB_R_TH_2)

/*
    ife_rth_bilat_c1_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gr_TH_0_OFS 0x0160
REGDEF_BEGIN(RANGEB_Gr_TH_0)
REGDEF_BIT(ife_rth_bilat_c1_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_1,        10)
REGDEF_END(RANGEB_Gr_TH_0)

/*
    ife_rth_bilat_c1_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gr_TH_1_OFS 0x0164
REGDEF_BEGIN(RANGEB_Gr_TH_1)
REGDEF_BIT(ife_rth_bilat_c1_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_3,        10)
REGDEF_END(RANGEB_Gr_TH_1)

/*
    ife_rth_bilat_c1_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gr_TH_2_OFS 0x0168
REGDEF_BEGIN(RANGEB_Gr_TH_2)
REGDEF_BIT(ife_rth_bilat_c1_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_5,        10)
REGDEF_END(RANGEB_Gr_TH_2)

/*
    ife_rth_bilat_c2_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gb_TH_0_OFS 0x0170
REGDEF_BEGIN(RANGEB_Gb_TH_0)
REGDEF_BIT(ife_rth_bilat_c2_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_1,        10)
REGDEF_END(RANGEB_Gb_TH_0)

/*
    ife_rth_bilat_c2_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gb_TH_1_OFS 0x0174
REGDEF_BEGIN(RANGEB_Gb_TH_1)
REGDEF_BIT(ife_rth_bilat_c2_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_3,        10)
REGDEF_END(RANGEB_Gb_TH_1)

/*
    ife_rth_bilat_c2_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gb_TH_2_OFS 0x0178
REGDEF_BEGIN(RANGEB_Gb_TH_2)
REGDEF_BIT(ife_rth_bilat_c2_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_5,        10)
REGDEF_END(RANGEB_Gb_TH_2)

/*
    ife_rth_bilat_c3_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_B_TH_0_OFS 0x0180
REGDEF_BEGIN(RANGEB_B_TH_0)
REGDEF_BIT(ife_rth_bilat_c3_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_1,        10)
REGDEF_END(RANGEB_B_TH_0)

/*
    ife_rth_bilat_c3_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_B_TH_1_OFS 0x0184
REGDEF_BEGIN(RANGEB_B_TH_1)
REGDEF_BIT(ife_rth_bilat_c3_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_3,        10)
REGDEF_END(RANGEB_B_TH_1)

/*
    ife_rth_bilat_c3_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_B_TH_2_OFS 0x0188
REGDEF_BEGIN(RANGEB_B_TH_2)
REGDEF_BIT(ife_rth_bilat_c3_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_5,        10)
REGDEF_END(RANGEB_B_TH_2)

//=============================================================================
// IFE 2DNR Filter_TH
//=============================================================================
/*
    ife_rth_nlm_c0_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_R_LUT_0_OFS 0x01c0
REGDEF_BEGIN(RANGEA_R_LUT_0)
REGDEF_BIT(ife_rth_nlm_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_1,        10)
REGDEF_END(RANGEA_R_LUT_0)

/*
    ife_rth_nlm_c0_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_R_LUT_1_OFS 0x01c4
REGDEF_BEGIN(RANGEA_R_LUT_1)
REGDEF_BIT(ife_rth_nlm_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_3,        10)
REGDEF_END(RANGEA_R_LUT_1)

/*
    ife_rth_nlm_c0_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_R_LUT_2_OFS 0x01c8
REGDEF_BEGIN(RANGEA_R_LUT_2)
REGDEF_BIT(ife_rth_nlm_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_5,        10)
REGDEF_END(RANGEA_R_LUT_2)

/*
    ife_rth_nlm_c0_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_R_LUT_3_OFS 0x01cc
REGDEF_BEGIN(RANGEA_R_LUT_3)
REGDEF_BIT(ife_rth_nlm_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_7,        10)
REGDEF_END(RANGEA_R_LUT_3)

/*
    ife_rth_nlm_c0_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_R_LUT_4_OFS 0x01d0
REGDEF_BEGIN(RANGEA_R_LUT_4)
REGDEF_BIT(ife_rth_nlm_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_9,        10)
REGDEF_END(RANGEA_R_LUT_4)

/*
    ife_rth_nlm_c0_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_R_LUT_5_OFS 0x01d4
REGDEF_BEGIN(RANGEA_R_LUT_5)
REGDEF_BIT(ife_rth_nlm_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_11,        10)
REGDEF_END(RANGEA_R_LUT_5)

/*
    ife_rth_nlm_c0_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_R_LUT_6_OFS 0x01d8
REGDEF_BEGIN(RANGEA_R_LUT_6)
REGDEF_BIT(ife_rth_nlm_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_13,        10)
REGDEF_END(RANGEA_R_LUT_6)

/*
    ife_rth_nlm_c0_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_R_LUT_7_OFS 0x01dc
REGDEF_BEGIN(RANGEA_R_LUT_7)
REGDEF_BIT(ife_rth_nlm_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_15,        10)
REGDEF_END(RANGEA_R_LUT_7)

/*
    ife_rth_nlm_c0_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGEA_R_LUT_8_OFS 0x01e0
REGDEF_BEGIN(RANGEA_R_LUT_8)
REGDEF_BIT(ife_rth_nlm_c0_lut_16,        10)
REGDEF_END(RANGEA_R_LUT_8)

/*
    ife_rth_nlm_c1_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gr_LUT_0_OFS 0x01e4
REGDEF_BEGIN(RANGEA_Gr_LUT_0)
REGDEF_BIT(ife_rth_nlm_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_1,        10)
REGDEF_END(RANGEA_Gr_LUT_0)

/*
    ife_rth_nlm_c1_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gr_LUT_1_OFS 0x01e8
REGDEF_BEGIN(RANGEA_Gr_LUT_1)
REGDEF_BIT(ife_rth_nlm_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_3,        10)
REGDEF_END(RANGEA_Gr_LUT_1)

/*
    ife_rth_nlm_c1_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gr_LUT_2_OFS 0x01ec
REGDEF_BEGIN(RANGEA_Gr_LUT_2)
REGDEF_BIT(ife_rth_nlm_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_5,        10)
REGDEF_END(RANGEA_Gr_LUT_2)

/*
    ife_rth_nlm_c1_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gr_LUT_3_OFS 0x01f0
REGDEF_BEGIN(RANGEA_Gr_LUT_3)
REGDEF_BIT(ife_rth_nlm_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_7,        10)
REGDEF_END(RANGEA_Gr_LUT_3)

/*
    ife_rth_nlm_c1_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gr_LUT_4_OFS 0x01f4
REGDEF_BEGIN(RANGEA_Gr_LUT_4)
REGDEF_BIT(ife_rth_nlm_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_9,        10)
REGDEF_END(RANGEA_Gr_LUT_4)

/*
    ife_rth_nlm_c1_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_Gr_LUT_5_OFS 0x01f8
REGDEF_BEGIN(RANGEA_Gr_LUT_5)
REGDEF_BIT(ife_rth_nlm_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_11,        10)
REGDEF_END(RANGEA_Gr_LUT_5)

/*
    ife_rth_nlm_c1_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_Gr_LUT_6_OFS 0x01fc
REGDEF_BEGIN(RANGEA_Gr_LUT_6)
REGDEF_BIT(ife_rth_nlm_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_13,        10)
REGDEF_END(RANGEA_Gr_LUT_6)

/*
    ife_rth_nlm_c1_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_Gr_LUT_7_OFS 0x0200
REGDEF_BEGIN(RANGEA_Gr_LUT_7)
REGDEF_BIT(ife_rth_nlm_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_15,        10)
REGDEF_END(RANGEA_Gr_LUT_7)

/*
    ife_rth_nlm_c1_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGEA_Gr_LUT_8_OFS 0x0204
REGDEF_BEGIN(RANGEA_Gr_LUT_8)
REGDEF_BIT(ife_rth_nlm_c1_lut_16,        10)
REGDEF_END(RANGEA_Gr_LUT_8)

/*
    ife_rth_nlm_c2_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gb_LUT_0_OFS 0x0208
REGDEF_BEGIN(RANGEA_Gb_LUT_0)
REGDEF_BIT(ife_rth_nlm_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_1,        10)
REGDEF_END(RANGEA_Gb_LUT_0)

/*
    ife_rth_nlm_c2_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gb_LUT_1_OFS 0x020c
REGDEF_BEGIN(RANGEA_Gb_LUT_1)
REGDEF_BIT(ife_rth_nlm_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_3,        10)
REGDEF_END(RANGEA_Gb_LUT_1)

/*
    ife_rth_nlm_c2_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gb_LUT_2_OFS 0x0210
REGDEF_BEGIN(RANGEA_Gb_LUT_2)
REGDEF_BIT(ife_rth_nlm_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_5,        10)
REGDEF_END(RANGEA_Gb_LUT_2)

/*
    ife_rth_nlm_c2_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gb_LUT_3_OFS 0x0214
REGDEF_BEGIN(RANGEA_Gb_LUT_3)
REGDEF_BIT(ife_rth_nlm_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_7,        10)
REGDEF_END(RANGEA_Gb_LUT_3)

/*
    ife_rth_nlm_c2_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_Gb_LUT_4_OFS 0x0218
REGDEF_BEGIN(RANGEA_Gb_LUT_4)
REGDEF_BIT(ife_rth_nlm_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_9,        10)
REGDEF_END(RANGEA_Gb_LUT_4)

/*
    ife_rth_nlm_c2_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_Gb_LUT_5_OFS 0x021c
REGDEF_BEGIN(RANGEA_Gb_LUT_5)
REGDEF_BIT(ife_rth_nlm_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_11,        10)
REGDEF_END(RANGEA_Gb_LUT_5)

/*
    ife_rth_nlm_c2_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_Gb_LUT_6_OFS 0x0220
REGDEF_BEGIN(RANGEA_Gb_LUT_6)
REGDEF_BIT(ife_rth_nlm_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_13,        10)
REGDEF_END(RANGEA_Gb_LUT_6)

/*
    ife_rth_nlm_c2_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_Gb_LUT_7_OFS 0x0224
REGDEF_BEGIN(RANGEA_Gb_LUT_7)
REGDEF_BIT(ife_rth_nlm_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_15,        10)
REGDEF_END(RANGEA_Gb_LUT_7)

/*
    ife_rth_nlm_c2_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGEA_Gb_LUT_8_OFS 0x0228
REGDEF_BEGIN(RANGEA_Gb_LUT_8)
REGDEF_BIT(ife_rth_nlm_c2_lut_16,        10)
REGDEF_END(RANGEA_Gb_LUT_8)

/*
    ife_rth_nlm_c3_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_B_LUT_0_OFS 0x022c
REGDEF_BEGIN(RANGEA_B_LUT_0)
REGDEF_BIT(ife_rth_nlm_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_1,        10)
REGDEF_END(RANGEA_B_LUT_0)

/*
    ife_rth_nlm_c3_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_B_LUT_1_OFS 0x0230
REGDEF_BEGIN(RANGEA_B_LUT_1)
REGDEF_BIT(ife_rth_nlm_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_3,        10)
REGDEF_END(RANGEA_B_LUT_1)

/*
    ife_rth_nlm_c3_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_B_LUT_2_OFS 0x0234
REGDEF_BEGIN(RANGEA_B_LUT_2)
REGDEF_BIT(ife_rth_nlm_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_5,        10)
REGDEF_END(RANGEA_B_LUT_2)

/*
    ife_rth_nlm_c3_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_B_LUT_3_OFS 0x0238
REGDEF_BEGIN(RANGEA_B_LUT_3)
REGDEF_BIT(ife_rth_nlm_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_7,        10)
REGDEF_END(RANGEA_B_LUT_3)

/*
    ife_rth_nlm_c3_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGEA_B_LUT_4_OFS 0x023c
REGDEF_BEGIN(RANGEA_B_LUT_4)
REGDEF_BIT(ife_rth_nlm_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_9,        10)
REGDEF_END(RANGEA_B_LUT_4)

/*
    ife_rth_nlm_c3_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_B_LUT_5_OFS 0x0240
REGDEF_BEGIN(RANGEA_B_LUT_5)
REGDEF_BIT(ife_rth_nlm_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_11,        10)
REGDEF_END(RANGEA_B_LUT_5)

/*
    ife_rth_nlm_c3_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_B_LUT_6_OFS 0x0244
REGDEF_BEGIN(RANGEA_B_LUT_6)
REGDEF_BIT(ife_rth_nlm_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_13,        10)
REGDEF_END(RANGEA_B_LUT_6)

/*
    ife_rth_nlm_c3_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGEA_B_LUT_7_OFS 0x0248
REGDEF_BEGIN(RANGEA_B_LUT_7)
REGDEF_BIT(ife_rth_nlm_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_15,        10)
REGDEF_END(RANGEA_B_LUT_7)

/*
    ife_rth_nlm_c3_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGEA_B_LUT_8_OFS 0x024c
REGDEF_BEGIN(RANGEA_B_LUT_8)
REGDEF_BIT(ife_rth_nlm_c3_lut_16,        10)
REGDEF_END(RANGEA_B_LUT_8)

/*
    ife_rth_bilat_c0_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_R_LUT_0_OFS 0x0250
REGDEF_BEGIN(RANGEB_R_LUT_0)
REGDEF_BIT(ife_rth_bilat_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_1,        10)
REGDEF_END(RANGEB_R_LUT_0)

/*
    ife_rth_bilat_c0_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_R_LUT_1_OFS 0x0254
REGDEF_BEGIN(RANGEB_R_LUT_1)
REGDEF_BIT(ife_rth_bilat_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_3,        10)
REGDEF_END(RANGEB_R_LUT_1)

/*
    ife_rth_bilat_c0_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_R_LUT_2_OFS 0x0258
REGDEF_BEGIN(RANGEB_R_LUT_2)
REGDEF_BIT(ife_rth_bilat_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_5,        10)
REGDEF_END(RANGEB_R_LUT_2)

/*
    ife_rth_bilat_c0_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_R_LUT_3_OFS 0x025c
REGDEF_BEGIN(RANGEB_R_LUT_3)
REGDEF_BIT(ife_rth_bilat_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_7,        10)
REGDEF_END(RANGEB_R_LUT_3)

/*
    ife_rth_bilat_c0_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_R_LUT_4_OFS 0x0260
REGDEF_BEGIN(RANGEB_R_LUT_4)
REGDEF_BIT(ife_rth_bilat_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_9,        10)
REGDEF_END(RANGEB_R_LUT_4)

/*
    ife_rth_bilat_c0_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_R_LUT_5_OFS 0x0264
REGDEF_BEGIN(RANGEB_R_LUT_5)
REGDEF_BIT(ife_rth_bilat_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_11,        10)
REGDEF_END(RANGEB_R_LUT_5)

/*
    ife_rth_bilat_c0_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_R_LUT_6_OFS 0x0268
REGDEF_BEGIN(RANGEB_R_LUT_6)
REGDEF_BIT(ife_rth_bilat_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_13,        10)
REGDEF_END(RANGEB_R_LUT_6)

/*
    ife_rth_bilat_c0_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_R_LUT_7_OFS 0x026c
REGDEF_BEGIN(RANGEB_R_LUT_7)
REGDEF_BIT(ife_rth_bilat_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_15,        10)
REGDEF_END(RANGEB_R_LUT_7)

/*
    ife_rth_bilat_c0_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGEB_R_LUT_8_OFS 0x0270
REGDEF_BEGIN(RANGEB_R_LUT_8)
REGDEF_BIT(ife_rth_bilat_c0_lut_16,        10)
REGDEF_END(RANGEB_R_LUT_8)

/*
    ife_rth_bilat_c1_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gr_LUT_0_OFS 0x0274
REGDEF_BEGIN(RANGEB_Gr_LUT_0)
REGDEF_BIT(ife_rth_bilat_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_1,        10)
REGDEF_END(RANGEB_Gr_LUT_0)

/*
    ife_rth_bilat_c1_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gr_LUT_1_OFS 0x0278
REGDEF_BEGIN(RANGEB_Gr_LUT_1)
REGDEF_BIT(ife_rth_bilat_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_3,        10)
REGDEF_END(RANGEB_Gr_LUT_1)

/*
    ife_rth_bilat_c1_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gr_LUT_2_OFS 0x027c
REGDEF_BEGIN(RANGEB_Gr_LUT_2)
REGDEF_BIT(ife_rth_bilat_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_5,        10)
REGDEF_END(RANGEB_Gr_LUT_2)

/*
    ife_rth_bilat_c1_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gr_LUT_3_OFS 0x0280
REGDEF_BEGIN(RANGEB_Gr_LUT_3)
REGDEF_BIT(ife_rth_bilat_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_7,        10)
REGDEF_END(RANGEB_Gr_LUT_3)

/*
    ife_rth_bilat_c1_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gr_LUT_4_OFS 0x0284
REGDEF_BEGIN(RANGEB_Gr_LUT_4)
REGDEF_BIT(ife_rth_bilat_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_9,        10)
REGDEF_END(RANGEB_Gr_LUT_4)

/*
    ife_rth_bilat_c1_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_Gr_LUT_5_OFS 0x0288
REGDEF_BEGIN(RANGEB_Gr_LUT_5)
REGDEF_BIT(ife_rth_bilat_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_11,        10)
REGDEF_END(RANGEB_Gr_LUT_5)

/*
    ife_rth_bilat_c1_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_Gr_LUT_6_OFS 0x028c
REGDEF_BEGIN(RANGEB_Gr_LUT_6)
REGDEF_BIT(ife_rth_bilat_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_13,        10)
REGDEF_END(RANGEB_Gr_LUT_6)

/*
    ife_rth_bilat_c1_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_Gr_LUT_7_OFS 0x0290
REGDEF_BEGIN(RANGEB_Gr_LUT_7)
REGDEF_BIT(ife_rth_bilat_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_15,        10)
REGDEF_END(RANGEB_Gr_LUT_7)

/*
    ife_rth_bilat_c1_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGEB_Gr_LUT_8_OFS 0x0294
REGDEF_BEGIN(RANGEB_Gr_LUT_8)
REGDEF_BIT(ife_rth_bilat_c1_lut_16,        10)
REGDEF_END(RANGEB_Gr_LUT_8)

/*
    ife_rth_bilat_c2_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gb_LUT_0_OFS 0x0298
REGDEF_BEGIN(RANGEB_Gb_LUT_0)
REGDEF_BIT(ife_rth_bilat_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_1,        10)
REGDEF_END(RANGEB_Gb_LUT_0)

/*
    ife_rth_bilat_c2_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gb_LUT_1_OFS 0x029c
REGDEF_BEGIN(RANGEB_Gb_LUT_1)
REGDEF_BIT(ife_rth_bilat_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_3,        10)
REGDEF_END(RANGEB_Gb_LUT_1)

/*
    ife_rth_bilat_c2_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gb_LUT_2_OFS 0x02a0
REGDEF_BEGIN(RANGEB_Gb_LUT_2)
REGDEF_BIT(ife_rth_bilat_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_5,        10)
REGDEF_END(RANGEB_Gb_LUT_2)

/*
    ife_rth_bilat_c2_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gb_LUT_3_OFS 0x02a4
REGDEF_BEGIN(RANGEB_Gb_LUT_3)
REGDEF_BIT(ife_rth_bilat_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_7,        10)
REGDEF_END(RANGEB_Gb_LUT_3)

/*
    ife_rth_bilat_c2_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_Gb_LUT_4_OFS 0x02a8
REGDEF_BEGIN(RANGEB_Gb_LUT_4)
REGDEF_BIT(ife_rth_bilat_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_9,        10)
REGDEF_END(RANGEB_Gb_LUT_4)

/*
    ife_rth_bilat_c2_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_Gb_LUT_5_OFS 0x02ac
REGDEF_BEGIN(RANGEB_Gb_LUT_5)
REGDEF_BIT(ife_rth_bilat_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_11,        10)
REGDEF_END(RANGEB_Gb_LUT_5)

/*
    ife_rth_bilat_c2_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_Gb_LUT_6_OFS 0x02b0
REGDEF_BEGIN(RANGEB_Gb_LUT_6)
REGDEF_BIT(ife_rth_bilat_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_13,        10)
REGDEF_END(RANGEB_Gb_LUT_6)

/*
    ife_rth_bilat_c2_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_Gb_LUT_7_OFS 0x02b4
REGDEF_BEGIN(RANGEB_Gb_LUT_7)
REGDEF_BIT(ife_rth_bilat_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_15,        10)
REGDEF_END(RANGEB_Gb_LUT_7)

/*
    ife_rth_bilat_c2_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGEB_Gb_LUT_8_OFS 0x02b8
REGDEF_BEGIN(RANGEB_Gb_LUT_8)
REGDEF_BIT(ife_rth_bilat_c2_lut_16,        10)
REGDEF_END(RANGEB_Gb_LUT_8)

/*
    ife_rth_bilat_c3_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_B_LUT_0_OFS 0x02bc
REGDEF_BEGIN(RANGEB_B_LUT_0)
REGDEF_BIT(ife_rth_bilat_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_1,        10)
REGDEF_END(RANGEB_B_LUT_0)

/*
    ife_rth_bilat_c3_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_B_LUT_1_OFS 0x02c0
REGDEF_BEGIN(RANGEB_B_LUT_1)
REGDEF_BIT(ife_rth_bilat_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_3,        10)
REGDEF_END(RANGEB_B_LUT_1)

/*
    ife_rth_bilat_c3_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_B_LUT_2_OFS 0x02c4
REGDEF_BEGIN(RANGEB_B_LUT_2)
REGDEF_BIT(ife_rth_bilat_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_5,        10)
REGDEF_END(RANGEB_B_LUT_2)

/*
    ife_rth_bilat_c3_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_B_LUT_3_OFS 0x02c8
REGDEF_BEGIN(RANGEB_B_LUT_3)
REGDEF_BIT(ife_rth_bilat_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_7,        10)
REGDEF_END(RANGEB_B_LUT_3)

/*
    ife_rth_bilat_c3_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGEB_B_LUT_4_OFS 0x02cc
REGDEF_BEGIN(RANGEB_B_LUT_4)
REGDEF_BIT(ife_rth_bilat_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_9,        10)
REGDEF_END(RANGEB_B_LUT_4)

/*
    ife_rth_bilat_c3_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_B_LUT_5_OFS 0x02d0
REGDEF_BEGIN(RANGEB_B_LUT_5)
REGDEF_BIT(ife_rth_bilat_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_11,        10)
REGDEF_END(RANGEB_B_LUT_5)

/*
    ife_rth_bilat_c3_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_B_LUT_6_OFS 0x02d4
REGDEF_BEGIN(RANGEB_B_LUT_6)
REGDEF_BIT(ife_rth_bilat_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_13,        10)
REGDEF_END(RANGEB_B_LUT_6)

/*
    ife_rth_bilat_c3_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGEB_B_LUT_7_OFS 0x02d8
REGDEF_BEGIN(RANGEB_B_LUT_7)
REGDEF_BIT(ife_rth_bilat_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_15,        10)
REGDEF_END(RANGEB_B_LUT_7)

/*
    ife_rth_bilat_c3_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGEB_B_LUT_8_OFS 0x02dc
REGDEF_BEGIN(RANGEB_B_LUT_8)
REGDEF_BIT(ife_rth_bilat_c3_lut_16,        10)
REGDEF_END(RANGEB_B_LUT_8)

//=============================================================================
// IFE 2DNR Filter_setting
//=============================================================================
/*
    ife_clamp_th :    [0x0, 0xfff],         bits : 11_0
    ife_clamp_mul:    [0x0, 0xff],          bits : 19_12
    ife_clamp_dlt:    [0x0, 0xfff],         bits : 31_20
*/
#define FILTER_CLAMP_REGISTER_OFS 0x0100
REGDEF_BEGIN(FILTER_CLAMP_REGISTER)
REGDEF_BIT(ife_clamp_th,        12)
REGDEF_BIT(ife_clamp_mul,        8)
REGDEF_BIT(ife_clamp_dlt,        12)
REGDEF_END(FILTER_CLAMP_REGISTER)

/*
    ife_bilat_w  :    [0x0, 0xf],           bits : 3_0
    ife_rth_w    :    [0x0, 0xf],           bits : 7_4
    ife_bilat_th1:    [0x0, 0x3ff],         bits : 17_8
    ife_bilat_th2:    [0x0, 0x3ff],         bits : 27_18
*/
#define FILTER_BLEND_W_REGISTER_OFS 0x0104
REGDEF_BEGIN(FILTER_BLEND_W_REGISTER)
REGDEF_BIT(ife_bilat_w,        4)
REGDEF_BIT(ife_rth_w,        4)
REGDEF_BIT(ife_bilat_th1,        10)
REGDEF_BIT(ife_bilat_th2,        10)
REGDEF_END(FILTER_BLEND_W_REGISTER)

//=============================================================================
// IFE VIG
//=============================================================================
/*
    ife_distvgtx_c0:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c0:    [0x0, 0x3fff],          bits : 29_16
    ife_distgain   :    [0x0, 0x3],         bits : 31_30
*/
#define VIGNETTE_SETTING_REGISTER_0_OFS 0x02e0
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_0)
REGDEF_BIT(ife_distvgtx_c0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c0,        14)
REGDEF_BIT(ife_distgain,        2)
REGDEF_END(VIGNETTE_SETTING_REGISTER_0)

/*
    ife_distvgtx_c1:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c1:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_1_OFS 0x02e4
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_1)
REGDEF_BIT(ife_distvgtx_c1,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c1,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_1)

/*
    ife_distvgtx_c2:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c2:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_2_OFS 0x02e8
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_2)
REGDEF_BIT(ife_distvgtx_c2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c2,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_2)

/*
    ife_distvgtx_c3:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c3:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_3_OFS 0x02ec
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_3)
REGDEF_BIT(ife_distvgtx_c3,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c3,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_3)

/*
    ife_distvgxdiv:    [0x0, 0xfff],            bits : 11_0
    ife_distvgydiv:    [0x0, 0xfff],            bits : 23_12
*/
#define VIGNETTE_SETTING_REGISTER_4_OFS 0x02f0
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_4)
REGDEF_BIT(ife_distvgxdiv,        12)
REGDEF_BIT(ife_distvgydiv,        12)
REGDEF_END(VIGNETTE_SETTING_REGISTER_4) //?

/*
    ife_vig_c0_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_0_OFS 0x02f8
REGDEF_BEGIN(VIGNETTE_REGISTER_0)
REGDEF_BIT(ife_vig_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_0)

/*
    ife_vig_c0_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_1_OFS 0x02fc
REGDEF_BEGIN(VIGNETTE_REGISTER_1)
REGDEF_BIT(ife_vig_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_1)

/*
    ife_vig_c0_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_2_OFS 0x0300
REGDEF_BEGIN(VIGNETTE_REGISTER_2)
REGDEF_BIT(ife_vig_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_2)

/*
    ife_vig_c0_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_3_OFS 0x0304
REGDEF_BEGIN(VIGNETTE_REGISTER_3)
REGDEF_BIT(ife_vig_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_3)

/*
    ife_vig_c0_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_4_OFS 0x0308
REGDEF_BEGIN(VIGNETTE_REGISTER_4)
REGDEF_BIT(ife_vig_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_4)

/*
    ife_vig_c0_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_5_OFS 0x030c
REGDEF_BEGIN(VIGNETTE_REGISTER_5)
REGDEF_BIT(ife_vig_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_5)

/*
    ife_vig_c0_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_6_OFS 0x0310
REGDEF_BEGIN(VIGNETTE_REGISTER_6)
REGDEF_BIT(ife_vig_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_6)

/*
    ife_vig_c0_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_7_OFS 0x0314
REGDEF_BEGIN(VIGNETTE_REGISTER_7)
REGDEF_BIT(ife_vig_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_7)

/*
    ife_vig_c0_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_8_OFS 0x0318
REGDEF_BEGIN(VIGNETTE_REGISTER_8)
REGDEF_BIT(ife_vig_c0_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_8)

/*
    ife_vig_c1_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_9_OFS 0x031c
REGDEF_BEGIN(VIGNETTE_REGISTER_9)
REGDEF_BIT(ife_vig_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_9)

/*
    ife_vig_c1_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_10_OFS 0x0320
REGDEF_BEGIN(VIGNETTE_REGISTER_10)
REGDEF_BIT(ife_vig_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_10)

/*
    ife_vig_c1_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_11_OFS 0x0324
REGDEF_BEGIN(VIGNETTE_REGISTER_11)
REGDEF_BIT(ife_vig_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_11)

/*
    ife_vig_c1_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_12_OFS 0x0328
REGDEF_BEGIN(VIGNETTE_REGISTER_12)
REGDEF_BIT(ife_vig_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_12)

/*
    ife_vig_c1_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_13_OFS 0x032c
REGDEF_BEGIN(VIGNETTE_REGISTER_13)
REGDEF_BIT(ife_vig_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_13)

/*
    ife_vig_c1_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_14_OFS 0x0330
REGDEF_BEGIN(VIGNETTE_REGISTER_14)
REGDEF_BIT(ife_vig_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_14)

/*
    ife_vig_c1_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_15_OFS 0x0334
REGDEF_BEGIN(VIGNETTE_REGISTER_15)
REGDEF_BIT(ife_vig_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_15)

/*
    ife_vig_c1_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_16_OFS 0x0338
REGDEF_BEGIN(VIGNETTE_REGISTER_16)
REGDEF_BIT(ife_vig_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_16)

/*
    ife_vig_c1_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_17_OFS 0x033c
REGDEF_BEGIN(VIGNETTE_REGISTER_17)
REGDEF_BIT(ife_vig_c1_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_17)

/*
    ife_vig_c2_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_18_OFS 0x0340
REGDEF_BEGIN(VIGNETTE_REGISTER_18)
REGDEF_BIT(ife_vig_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_18)

/*
    ife_vig_c2_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_19_OFS 0x0344
REGDEF_BEGIN(VIGNETTE_REGISTER_19)
REGDEF_BIT(ife_vig_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_19)

/*
    ife_vig_c2_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_20_OFS 0x0348
REGDEF_BEGIN(VIGNETTE_REGISTER_20)
REGDEF_BIT(ife_vig_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_20)

/*
    ife_vig_c2_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_21_OFS 0x034c
REGDEF_BEGIN(VIGNETTE_REGISTER_21)
REGDEF_BIT(ife_vig_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_21)

/*
    ife_vig_c2_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_22_OFS 0x0350
REGDEF_BEGIN(VIGNETTE_REGISTER_22)
REGDEF_BIT(ife_vig_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_22)

/*
    ife_vig_c2_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_23_OFS 0x0354
REGDEF_BEGIN(VIGNETTE_REGISTER_23)
REGDEF_BIT(ife_vig_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_23)

/*
    ife_vig_c2_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_24_OFS 0x0358
REGDEF_BEGIN(VIGNETTE_REGISTER_24)
REGDEF_BIT(ife_vig_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_24)

/*
    ife_vig_c2_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_25_OFS 0x035c
REGDEF_BEGIN(VIGNETTE_REGISTER_25)
REGDEF_BIT(ife_vig_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_25)

/*
    ife_vig_c2_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_26_OFS 0x0360
REGDEF_BEGIN(VIGNETTE_REGISTER_26)
REGDEF_BIT(ife_vig_c2_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_26)

/*
    ife_vig_c3_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_27_OFS 0x0364
REGDEF_BEGIN(VIGNETTE_REGISTER_27)
REGDEF_BIT(ife_vig_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_27)

/*
    ife_vig_c3_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_28_OFS 0x0368
REGDEF_BEGIN(VIGNETTE_REGISTER_28)
REGDEF_BIT(ife_vig_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_28)

/*
    ife_vig_c3_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_29_OFS 0x036c
REGDEF_BEGIN(VIGNETTE_REGISTER_29)
REGDEF_BIT(ife_vig_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_29)

/*
    ife_vig_c3_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_30_OFS 0x0370
REGDEF_BEGIN(VIGNETTE_REGISTER_30)
REGDEF_BIT(ife_vig_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_30)

/*
    ife_vig_c3_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_31_OFS 0x0374
REGDEF_BEGIN(VIGNETTE_REGISTER_31)
REGDEF_BIT(ife_vig_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_31)

/*
    ife_vig_c3_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_32_OFS 0x0378
REGDEF_BEGIN(VIGNETTE_REGISTER_32)
REGDEF_BIT(ife_vig_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_32)

/*
    ife_vig_c3_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_33_OFS 0x037c
REGDEF_BEGIN(VIGNETTE_REGISTER_33)
REGDEF_BIT(ife_vig_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_33)

/*
    ife_vig_c3_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_34_OFS 0x0380
REGDEF_BEGIN(VIGNETTE_REGISTER_34)
REGDEF_BIT(ife_vig_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_34)

/*
    ife_vig_c3_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_35_OFS 0x0384
REGDEF_BEGIN(VIGNETTE_REGISTER_35)
REGDEF_BIT(ife_vig_c3_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_35)

//=============================================================================
// IFE HDR
//=============================================================================
/*
    IFE_F_NRS_O_EN            :    [0x0, 0x1],          bits : 0
    IFE_F_NRS_B_EN            :    [0x0, 0x1],          bits : 1
    IFE_F_NRS_G_EN            :    [0x0, 0x1],          bits : 2
    IFE_F_NRS_B_STRENGTH (0~8):    [0x0, 0xf],          bits : 19_16
    IFE_F_NRS_G_STRENGTH (0~8):    [0x0, 0xf],          bits : 23_20
    IFE_F_NRS_G_WEIGHT        :    [0x0, 0xf],          bits : 27_24
*/
#define NRS_REGISTER_OFS 0x0500
REGDEF_BEGIN(NRS_REGISTER)
REGDEF_BIT(ife_f_nrs_ord_en,        1)
REGDEF_BIT(ife_f_nrs_bilat_en,        1)
REGDEF_BIT(ife_f_nrs_gbilat_en,        1)
REGDEF_BIT(,       13)
REGDEF_BIT(ife_f_nrs_bilat_strength,        4)
REGDEF_BIT(ife_f_nrs_gbilat_strength,        4)
REGDEF_BIT(ife_f_nrs_gbilat_weight,        4)
REGDEF_END(NRS_REGISTER)



/*
    IFE_F_NRS_ORD_RANGE_BRI :    [0x0, 0x7],            bits : 2_0
    ife_f_nrs_ord_range_dark:    [0x0, 0x7],            bits : 6_4
    ife_f_nrs_ord_diff_thr  :    [0x0, 0x3ff],          bits : 17_8
*/
#define RHE_NRS_S_REGISTER_OFS 0x0504
REGDEF_BEGIN(RHE_NRS_S_REGISTER)
REGDEF_BIT(ife_f_nrs_ord_range_bright,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_nrs_ord_range_dark,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_nrs_ord_diff_thr,        10)
REGDEF_END(RHE_NRS_S_REGISTER)

/*
    IFE_F_NRS_B_LUT_OFS0:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_B_LUT_OFS1:    [0x0, 0xff],           bits : 15_8
    IFE_F_NRS_B_LUT_OFS2:    [0x0, 0xff],           bits : 23_16
    IFE_F_NRS_B_LUT_OFS3:    [0x0, 0xff],           bits : 31_24
*/
#define RHE_NRS_BILATERAL_LUT_OFFSET0_OFS 0x0510
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_OFFSET0)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset0,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset1,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset2,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset3,        8)
REGDEF_END(RHE_NRS_BILATERAL_LUT_OFFSET0)

/*
    IFE_F_NRS_B_LUT_OFS4:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_B_LUT_OFS5:    [0x0, 0xff],           bits : 15_8
*/
#define RHE_NRS_BILATERAL_LUT_OFFSET1_OFS 0x0514
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_OFFSET1)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset4,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset5,        8)
REGDEF_END(RHE_NRS_BILATERAL_LUT_OFFSET1)

/*
    IFE_F_NRS_B_LUT_RANGE5 :    [0x0, 0xfff],           bits : 11_0
    IFE_F_NRS_B_LUT_TH1:    [0x0, 0x3],         bits : 17_16
    IFE_F_NRS_B_LUT_TH2:    [0x0, 0x3],         bits : 19_18
    IFE_F_NRS_B_LUT_TH3:    [0x0, 0x3],         bits : 21_20
    IFE_F_NRS_B_LUT_TH4:    [0x0, 0x3],         bits : 23_22
    IFE_F_NRS_B_LUT_TH5:    [0x0, 0x3],         bits : 25_24
*/
#define RHE_NRS_BILATERAL_LUT_RANGE2_OFS 0x0524
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_RANGE2)
REGDEF_BIT(ife_f_nrs_bilat_lut_range5,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_bilat_lut_th1,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th2,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th3,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th4,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th5,        2)
REGDEF_END(RHE_NRS_BILATERAL_LUT_RANGE2)

/*
    IFE_F_NRS_G_LUT_OFS0:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_G_LUT_OFS1:    [0x0, 0xff],           bits : 15_8
    IFE_F_NRS_G_LUT_OFS2:    [0x0, 0xff],           bits : 23_16
    IFE_F_NRS_G_LUT_OFS3:    [0x0, 0xff],           bits : 31_24
*/
#define RHE_NRS_GBILATERAL_LUT_OFFSET0_OFS 0x0528
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_OFFSET0)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset0,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset1,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset2,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset3,        8)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_OFFSET0)

/*
    IFE_F_NRS_G_LUT_OFS4:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_G_LUT_OFS5:    [0x0, 0xff],           bits : 15_8
*/
#define RHE_NRS_GBILATERAL_LUT_OFFSET1_OFS 0x052c
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_OFFSET1)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset4,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset5,        8)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_OFFSET1)

/*
    IFE_F_NRS_G_LUT_RANGE5 :    [0x0, 0xfff],           bits : 11_0
    IFE_F_NRS_G_LUT_TH1:    [0x0, 0x3],         bits : 17_16
    IFE_F_NRS_G_LUT_TH2:    [0x0, 0x3],         bits : 19_18
    IFE_F_NRS_G_LUT_TH3:    [0x0, 0x3],         bits : 21_20
    IFE_F_NRS_G_LUT_TH4:    [0x0, 0x3],         bits : 23_22
    IFE_F_NRS_G_LUT_TH5:    [0x0, 0x3],         bits : 25_24
*/
#define RHE_NRS_GBILATERAL_LUT_RANGE2_OFS 0x053c
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_RANGE2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range5,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th1,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th2,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th3,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th4,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th5,        2)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_RANGE2)

/*
    IFE_F_FUSION_YM_SEL       :    [0x0, 0x3],          bits : 1_0
    IFE_F_FUSION_BCN_SEL (0~2):    [0x0, 0x3],          bits : 5_4
    IFE_F_FUSION_BCD_SEL (0~2):    [0x0, 0x3],          bits : 9_8
    ife_f_fusion_mode         :    [0x0, 0x3],          bits : 13_12
    ife_f_fusion_evratio      :    [0x0, 0xff],         bits : 23_16
*/
#define RHE_FUSION_REGISTER_OFS 0x0550
REGDEF_BEGIN(RHE_FUSION_REGISTER)
REGDEF_BIT(ife_f_fusion_ymean_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_normal_blend_curve_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_diff_blend_curve_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_mode,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_evratio,        8)
REGDEF_END(RHE_FUSION_REGISTER)


/*
    IFE_F_FUSION_BCNL_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCNL_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCNL_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCNL_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE_OFS 0x0554
REGDEF_BEGIN(RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE)


/*
    IFE_F_FUSION_BCNS_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCNS_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCNS_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCNS_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE_OFS 0x0558
REGDEF_BEGIN(RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE)


/*
    IFE_F_FUSION_BCDL_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCDL_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCDL_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCDL_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE_OFS 0x055c
REGDEF_BEGIN(RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE)


/*
    IFE_F_FUSION_BCDS_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCDS_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCDS_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCDS_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE_OFS 0x0560
REGDEF_BEGIN(RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE)

/*
    ife_f_fusion_mc_lumthr    :    [0x0, 0xfff],            bits : 11_0
    ife_f_fusion_mc_diff_ratio:    [0x0, 0x3],          bits : 13_12
*/
#define RHE_FUSION_MOTION_COMPENSATION_OFS 0x0570
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION)
REGDEF_BIT(ife_f_fusion_mc_lumthr,        12)
REGDEF_BIT(ife_f_fusion_mc_diff_ratio,        2)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION)


/*
    ife_f_fusion_mc_lut_positive_diff_weight0 (0~16):    [0x0, 0x1f],           bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight1 (0~16):    [0x0, 0x1f],           bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight2 (0~16):    [0x0, 0x1f],           bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight3 (0~16):    [0x0, 0x1f],           bits : 19_15
    ife_f_fusion_mc_lut_positive_diff_weight4 (0~16):    [0x0, 0x1f],           bits : 24_20
    ife_f_fusion_mc_lut_positive_diff_weight5 (0~16):    [0x0, 0x1f],           bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_0_OFS 0x0574
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_0)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight0,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight1,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight2,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight3,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight4,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight5,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_0)


/*
    ife_f_fusion_mc_lut_positive_diff_weight6 (0~16) :    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight7 (0~16) :    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight8 (0~16) :    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight9 (0~16) :    [0x0, 0x1f],          bits : 19_15
    ife_f_fusion_mc_lut_positive_diff_weight10 (0~16):    [0x0, 0x1f],          bits : 24_20
    ife_f_fusion_mc_lut_positive_diff_weight11 (0~16):    [0x0, 0x1f],          bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_1_OFS 0x0578
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_1)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight6,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight7,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight8,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight9,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight10,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight11,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_1)


/*
    ife_f_fusion_mc_lut_positive_diff_weight12 (0~16):    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight13 (0~16):    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight14 (0~16):    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight15 (0~16):    [0x0, 0x1f],          bits : 19_15
    IFE_F_FUSION_MC_LUT_DWD (0~16)  :    [0x0, 0x1f],           bits : 24_20
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_2_OFS 0x057c
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_2)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight12,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight13,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight14,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight15,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_difflumth_diff_weight,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_2)

/*
    ife_f_fusion_mc_lut_negative_diff_weight0 (0~16):    [0x0, 0x1f],           bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight1 (0~16):    [0x0, 0x1f],           bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight2 (0~16):    [0x0, 0x1f],           bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight3 (0~16):    [0x0, 0x1f],           bits : 19_15
    ife_f_fusion_mc_lut_negative_diff_weight4 (0~16):    [0x0, 0x1f],           bits : 24_20
    ife_f_fusion_mc_lut_negative_diff_weight5 (0~16):    [0x0, 0x1f],           bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_3_OFS 0x0580
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_3)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight0,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight1,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight2,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight3,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight4,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight5,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_3)


/*
    ife_f_fusion_mc_lut_negative_diff_weight6 (0~16) :    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight7 (0~16) :    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight8 (0~16) :    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight9 (0~16) :    [0x0, 0x1f],          bits : 19_15
    ife_f_fusion_mc_lut_negative_diff_weight10 (0~16):    [0x0, 0x1f],          bits : 24_20
    ife_f_fusion_mc_lut_negative_diff_weight11 (0~16):    [0x0, 0x1f],          bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_4_OFS 0x0584
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_4)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight6,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight7,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight8,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight9,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight10,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight11,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_4)


/*
    ife_f_fusion_mc_lut_negative_diff_weight12 (0~16):    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight13 (0~16):    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight14 (0~16):    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight15 (0~16):    [0x0, 0x1f],          bits : 19_15
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_5_OFS 0x0588
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight12,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight13,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight14,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight15,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_5)

/*
    IFE_F_FCURVE_YM_SEL (0~2)  :    [0x0, 0x3],         bits : 1_0
    ife_f_fcurve_yvweight (0~8):    [0x0, 0xf],         bits : 5_2
*/
#define RHE_FCURVE_CTRL_OFS 0x0600
REGDEF_BEGIN(RHE_FCURVE_CTRL)
REGDEF_BIT(ife_f_fcurve_ymean_select,        2)
REGDEF_BIT(ife_f_fcurve_yvweight,        4)
REGDEF_END(RHE_FCURVE_CTRL)

/*
    ife_f_fcurve_index_lut0:    [0x0, 0x3f],            bits : 5_0
    ife_f_fcurve_index_lut1:    [0x0, 0x3f],            bits : 13_8
    ife_f_fcurve_index_lut2:    [0x0, 0x3f],            bits : 21_16
    ife_f_fcurve_index_lut3:    [0x0, 0x3f],            bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER0_OFS 0x0618
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER0)
REGDEF_BIT(ife_f_fcurve_index_lut0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut3,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER0)


/*
    ife_f_fcurve_index_lut4:    [0x0, 0x3f],            bits : 5_0
    ife_f_fcurve_index_lut5:    [0x0, 0x3f],            bits : 13_8
    ife_f_fcurve_index_lut6:    [0x0, 0x3f],            bits : 21_16
    ife_f_fcurve_index_lut7:    [0x0, 0x3f],            bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER1_OFS 0x061c
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER1)
REGDEF_BIT(ife_f_fcurve_index_lut4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut7,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER1)


/*
    ife_f_fcurve_index_lut8 :    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut9 :    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut10:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut11:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER2_OFS 0x0620
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER2)
REGDEF_BIT(ife_f_fcurve_index_lut8,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut9,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut10,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut11,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER2)


/*
    ife_f_fcurve_index_lut12:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut13:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut14:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut15:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER3_OFS 0x0624
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER3)
REGDEF_BIT(ife_f_fcurve_index_lut12,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut13,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut14,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut15,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER3)


/*
    ife_f_fcurve_index_lut16:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut17:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut18:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut19:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER4_OFS 0x0628
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER4)
REGDEF_BIT(ife_f_fcurve_index_lut16,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut17,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut18,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut19,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER4)


/*
    ife_f_fcurve_index_lut20:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut21:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut22:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut23:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER5_OFS 0x062c
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER5)
REGDEF_BIT(ife_f_fcurve_index_lut20,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut21,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut22,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut23,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER5)


/*
    ife_f_fcurve_index_lut24:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut25:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut26:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut27:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER6_OFS 0x0630
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER6)
REGDEF_BIT(ife_f_fcurve_index_lut24,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut25,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut26,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut27,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER6)


/*
    ife_f_fcurve_index_lut28:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut29:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut30:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut31:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER7_OFS 0x0634
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER7)
REGDEF_BIT(ife_f_fcurve_index_lut28,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut29,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut30,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut31,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER7)

/*
    ife_f_fcurve_split_lut0 :    [0x0, 0x3],            bits : 1_0
    ife_f_fcurve_split_lut1 :    [0x0, 0x3],            bits : 3_2
    ife_f_fcurve_split_lut2 :    [0x0, 0x3],            bits : 5_4
    ife_f_fcurve_split_lut3 :    [0x0, 0x3],            bits : 7_6
    ife_f_fcurve_split_lut4 :    [0x0, 0x3],            bits : 9_8
    ife_f_fcurve_split_lut5 :    [0x0, 0x3],            bits : 11_10
    ife_f_fcurve_split_lut6 :    [0x0, 0x3],            bits : 13_12
    ife_f_fcurve_split_lut7 :    [0x0, 0x3],            bits : 15_14
    ife_f_fcurve_split_lut8 :    [0x0, 0x3],            bits : 17_16
    ife_f_fcurve_split_lut9 :    [0x0, 0x3],            bits : 19_18
    ife_f_fcurve_split_lut10:    [0x0, 0x3],            bits : 21_20
    ife_f_fcurve_split_lut11:    [0x0, 0x3],            bits : 23_22
    ife_f_fcurve_split_lut12:    [0x0, 0x3],            bits : 25_24
    ife_f_fcurve_split_lut13:    [0x0, 0x3],            bits : 27_26
    ife_f_fcurve_split_lut14:    [0x0, 0x3],            bits : 29_28
    ife_f_fcurve_split_lut15:    [0x0, 0x3],            bits : 31_30
*/
#define RHE_FCURVE_SPLIT_REGISTER0_OFS 0x0638
REGDEF_BEGIN(RHE_FCURVE_SPLIT_REGISTER0)
REGDEF_BIT(ife_f_fcurve_split_lut0,        2)
REGDEF_BIT(ife_f_fcurve_split_lut1,        2)
REGDEF_BIT(ife_f_fcurve_split_lut2,        2)
REGDEF_BIT(ife_f_fcurve_split_lut3,        2)
REGDEF_BIT(ife_f_fcurve_split_lut4,        2)
REGDEF_BIT(ife_f_fcurve_split_lut5,        2)
REGDEF_BIT(ife_f_fcurve_split_lut6,        2)
REGDEF_BIT(ife_f_fcurve_split_lut7,        2)
REGDEF_BIT(ife_f_fcurve_split_lut8,        2)
REGDEF_BIT(ife_f_fcurve_split_lut9,        2)
REGDEF_BIT(ife_f_fcurve_split_lut10,        2)
REGDEF_BIT(ife_f_fcurve_split_lut11,        2)
REGDEF_BIT(ife_f_fcurve_split_lut12,        2)
REGDEF_BIT(ife_f_fcurve_split_lut13,        2)
REGDEF_BIT(ife_f_fcurve_split_lut14,        2)
REGDEF_BIT(ife_f_fcurve_split_lut15,        2)
REGDEF_END(RHE_FCURVE_SPLIT_REGISTER0)


/*
    ife_f_fcurve_split_lut16:    [0x0, 0x3],            bits : 1_0
    ife_f_fcurve_split_lut17:    [0x0, 0x3],            bits : 3_2
    ife_f_fcurve_split_lut18:    [0x0, 0x3],            bits : 5_4
    ife_f_fcurve_split_lut19:    [0x0, 0x3],            bits : 7_6
    ife_f_fcurve_split_lut20:    [0x0, 0x3],            bits : 9_8
    ife_f_fcurve_split_lut21:    [0x0, 0x3],            bits : 11_10
    ife_f_fcurve_split_lut22:    [0x0, 0x3],            bits : 13_12
    ife_f_fcurve_split_lut23:    [0x0, 0x3],            bits : 15_14
    ife_f_fcurve_split_lut24:    [0x0, 0x3],            bits : 17_16
    ife_f_fcurve_split_lut25:    [0x0, 0x3],            bits : 19_18
    ife_f_fcurve_split_lut26:    [0x0, 0x3],            bits : 21_20
    ife_f_fcurve_split_lut27:    [0x0, 0x3],            bits : 23_22
    ife_f_fcurve_split_lut28:    [0x0, 0x3],            bits : 25_24
    ife_f_fcurve_split_lut29:    [0x0, 0x3],            bits : 27_26
    ife_f_fcurve_split_lut30:    [0x0, 0x3],            bits : 29_28
    ife_f_fcurve_split_lut31:    [0x0, 0x3],            bits : 31_30
*/
#define RHE_FCURVE_SPLIT_REGISTER1_OFS 0x063c
REGDEF_BEGIN(RHE_FCURVE_SPLIT_REGISTER1)
REGDEF_BIT(ife_f_fcurve_split_lut16,        2)
REGDEF_BIT(ife_f_fcurve_split_lut17,        2)
REGDEF_BIT(ife_f_fcurve_split_lut18,        2)
REGDEF_BIT(ife_f_fcurve_split_lut19,        2)
REGDEF_BIT(ife_f_fcurve_split_lut20,        2)
REGDEF_BIT(ife_f_fcurve_split_lut21,        2)
REGDEF_BIT(ife_f_fcurve_split_lut22,        2)
REGDEF_BIT(ife_f_fcurve_split_lut23,        2)
REGDEF_BIT(ife_f_fcurve_split_lut24,        2)
REGDEF_BIT(ife_f_fcurve_split_lut25,        2)
REGDEF_BIT(ife_f_fcurve_split_lut26,        2)
REGDEF_BIT(ife_f_fcurve_split_lut27,        2)
REGDEF_BIT(ife_f_fcurve_split_lut28,        2)
REGDEF_BIT(ife_f_fcurve_split_lut29,        2)
REGDEF_BIT(ife_f_fcurve_split_lut30,        2)
REGDEF_BIT(ife_f_fcurve_split_lut31,        2)
REGDEF_END(RHE_FCURVE_SPLIT_REGISTER1)

/*
    ife_f_fcurve_val_lut0:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut1:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER0_OFS 0x0640
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER0)
REGDEF_BIT(ife_f_fcurve_val_lut0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut1,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER0)


/*
    ife_f_fcurve_val_lut2:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut3:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER1_OFS 0x0644
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER1)
REGDEF_BIT(ife_f_fcurve_val_lut2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut3,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER1)


/*
    ife_f_fcurve_val_lut4:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut5:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER2_OFS 0x0648
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER2)
REGDEF_BIT(ife_f_fcurve_val_lut4,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut5,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER2)


/*
    ife_f_fcurve_val_lut6:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut7:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER3_OFS 0x064c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER3)
REGDEF_BIT(ife_f_fcurve_val_lut6,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut7,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER3)


/*
    ife_f_fcurve_val_lut8:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut9:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER4_OFS 0x0650
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER4)
REGDEF_BIT(ife_f_fcurve_val_lut8,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut9,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER4)


/*
    ife_f_fcurve_val_lut10:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut11:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER5_OFS 0x0654
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER5)
REGDEF_BIT(ife_f_fcurve_val_lut10,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut11,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER5)


/*
    ife_f_fcurve_val_lut12:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut13:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER6_OFS 0x0658
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER6)
REGDEF_BIT(ife_f_fcurve_val_lut12,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut13,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER6)


/*
    ife_f_fcurve_val_lut14:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut15:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER7_OFS 0x065c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER7)
REGDEF_BIT(ife_f_fcurve_val_lut14,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut15,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER7)


/*
    ife_f_fcurve_val_lut16:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut17:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER8_OFS 0x0660
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER8)
REGDEF_BIT(ife_f_fcurve_val_lut16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut17,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER8)


/*
    ife_f_fcurve_val_lut18:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut19:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER9_OFS 0x0664
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER9)
REGDEF_BIT(ife_f_fcurve_val_lut18,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut19,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER9)


/*
    ife_f_fcurve_val_lut20:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut21:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER10_OFS 0x0668
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER10)
REGDEF_BIT(ife_f_fcurve_val_lut20,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut21,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER10)


/*
    ife_f_fcurve_val_lut22:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut23:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER11_OFS 0x066c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER11)
REGDEF_BIT(ife_f_fcurve_val_lut22,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut23,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER11)


/*
    ife_f_fcurve_val_lut24:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut25:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER12_OFS 0x0670
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER12)
REGDEF_BIT(ife_f_fcurve_val_lut24,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut25,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER12)


/*
    ife_f_fcurve_val_lut26:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut27:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER13_OFS 0x0674
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER13)
REGDEF_BIT(ife_f_fcurve_val_lut26,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut27,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER13)


/*
    ife_f_fcurve_val_lut28:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut29:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER14_OFS 0x0678
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER14)
REGDEF_BIT(ife_f_fcurve_val_lut28,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut29,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER14)


/*
    ife_f_fcurve_val_lut30:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut31:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER15_OFS 0x067c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER15)
REGDEF_BIT(ife_f_fcurve_val_lut30,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut31,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER15)


/*
    ife_f_fcurve_val_lut32:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut33:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER16_OFS 0x0680
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER16)
REGDEF_BIT(ife_f_fcurve_val_lut32,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut33,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER16)


/*
    ife_f_fcurve_val_lut34:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut35:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER17_OFS 0x0684
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER17)
REGDEF_BIT(ife_f_fcurve_val_lut34,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut35,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER17)


/*
    ife_f_fcurve_val_lut36:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut37:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER18_OFS 0x0688
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER18)
REGDEF_BIT(ife_f_fcurve_val_lut36,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut37,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER18)


/*
    ife_f_fcurve_val_lut38:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut39:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER19_OFS 0x068c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER19)
REGDEF_BIT(ife_f_fcurve_val_lut38,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut39,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER19)


/*
    ife_f_fcurve_val_lut40:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut41:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER20_OFS 0x0690
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER20)
REGDEF_BIT(ife_f_fcurve_val_lut40,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut41,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER20)


/*
    ife_f_fcurve_val_lut42:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut43:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER21_OFS 0x0694
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER21)
REGDEF_BIT(ife_f_fcurve_val_lut42,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut43,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER21)


/*
    ife_f_fcurve_val_lut44:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut45:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER22_OFS 0x0698
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER22)
REGDEF_BIT(ife_f_fcurve_val_lut44,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut45,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER22)


/*
    ife_f_fcurve_val_lut46:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut47:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER23_OFS 0x069c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER23)
REGDEF_BIT(ife_f_fcurve_val_lut46,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut47,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER23)


/*
    ife_f_fcurve_val_lut48:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut49:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER24_OFS 0x06a0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER24)
REGDEF_BIT(ife_f_fcurve_val_lut48,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut49,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER24)


/*
    ife_f_fcurve_val_lut50:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut51:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER25_OFS 0x06a4
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER25)
REGDEF_BIT(ife_f_fcurve_val_lut50,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut51,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER25)


/*
    ife_f_fcurve_val_lut52:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut53:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER26_OFS 0x06a8
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER26)
REGDEF_BIT(ife_f_fcurve_val_lut52,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut53,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER26)


/*
    ife_f_fcurve_val_lut54:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut55:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER27_OFS 0x06ac
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER27)
REGDEF_BIT(ife_f_fcurve_val_lut54,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut55,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER27)


/*
    ife_f_fcurve_val_lut56:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut57:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER28_OFS 0x06b0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER28)
REGDEF_BIT(ife_f_fcurve_val_lut56,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut57,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER28)


/*
    ife_f_fcurve_val_lut58:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut59:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER29_OFS 0x06b4
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER29)
REGDEF_BIT(ife_f_fcurve_val_lut58,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut59,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER29)


/*
    ife_f_fcurve_val_lut60:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut61:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER30_OFS 0x06b8
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER30)
REGDEF_BIT(ife_f_fcurve_val_lut60,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut61,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER30)


/*
    ife_f_fcurve_val_lut62:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut63:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER31_OFS 0x06bc
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER31)
REGDEF_BIT(ife_f_fcurve_val_lut62,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut63,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER31)


/*
    ife_f_fcurve_val_lut64:    [0x0, 0xfff],            bits : 11_0
*/
#define RHE_FCURVE_VALUE_REGISTER32_OFS 0x06c0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER32)
REGDEF_BIT(ife_f_fcurve_val_lut64,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER32)

// NOTE: DCE
//=============================================================================
// DCE CONTROL
//=============================================================================
/*
    DCE_OP                 :    [0x0, 0x3],         bits : 1_0
    DCE_STP                :    [0x0, 0x1],         bits : 2
    SRAM_MODE              :    [0x0, 0x1],         bits : 3
    CFA_EN                 :    [0x0, 0x1],         bits : 4
    DC_EN                  :    [0x0, 0x1],         bits : 5
    CAC_EN                 :    [0x0, 0x1],         bits : 6
    YUV2RGB_EN             :    [0x0, 0x1],         bits : 7
    TCURVE_EN              :    [0x0, 0x1],         bits : 8
    WDR_EN                 :    [0x0, 0x1],         bits : 9
    WDR_SUBIMG_OUT_EN      :    [0x0, 0x1],         bits : 10
    CFA_SUBIMG_OUT_EN      :    [0x0, 0x1],         bits : 11
    CFA_SUBIMG_OUT_FLIP_EN :    [0x0, 0x1],         bits : 12
    CFA_PINKR_EN           :    [0x0, 0x1],         bits : 13
    HISTOGRAM_EN           :    [0x0, 0x1],         bits : 14
    HISTOGRAM_SEL          :    [0x0, 0x1],         bits : 15
    YUV2RGB_FMT            :    [0x0, 0x3],         bits : 17_16
    CROP_EN                :    [0x0, 0x1],         bits : 18
    DC_SEL                 :    [0x0, 0x1],         bits : 19
    CHKSUM_EN              :    [0x0, 0x1],         bits : 20
    D2DRAND                :    [0x0, 0x1],         bits : 21
    D2DRAND_RST            :    [0x0, 0x1],         bits : 22
    FSTR_RST_DISABLE       :    [0x0, 0x1],         bits : 23
    D2DFMT                 :    [0x0, 0x3],         bits : 25_24
    D2DUVFILT              :    [0x0, 0x1],         bits : 27
    D2DIOSTOP              :    [0x0, 0x1],         bits : 28
    DEBUG_SEL              :    [0x0, 0x7],         bits : 31_29
*/
#define DCE_FUNCTION_REGISTER_OFS 0x0004
REGDEF_BEGIN(DCE_FUNCTION_REGISTER)
REGDEF_BIT(DCE_OP,        2)
REGDEF_BIT(DCE_STP,        1)
REGDEF_BIT(SRAM_MODE,        1)
REGDEF_BIT(CFA_EN,        1)
REGDEF_BIT(DC_EN,        1)
REGDEF_BIT(CAC_EN,        1)
REGDEF_BIT(YUV2RGB_EN,        1)
REGDEF_BIT(TCURVE_EN,        1)
REGDEF_BIT(WDR_EN,        1)
REGDEF_BIT(WDR_SUBIMG_OUT_EN,        1)
REGDEF_BIT(CFA_SUBIMG_OUT_EN,        1)
REGDEF_BIT(CFA_SUBIMG_OUT_FLIP_EN,        1)
REGDEF_BIT(CFA_PINKR_EN,        1)
REGDEF_BIT(HISTOGRAM_EN,        1)
REGDEF_BIT(HISTOGRAM_SEL,        1)
REGDEF_BIT(YUV2RGB_FMT,        2)
REGDEF_BIT(CROP_EN,        1)
REGDEF_BIT(DC_SEL,        1)
REGDEF_BIT(CHKSUM_EN,        1)
REGDEF_BIT(D2DRAND,        1)
REGDEF_BIT(D2DRAND_RST,        1)
REGDEF_BIT(FSTR_RST_DISABLE,        1)
REGDEF_BIT(D2DFMT,        2)
REGDEF_BIT(,        1)
REGDEF_BIT(D2DUVFILT,        1)
REGDEF_BIT(D2DIOSTOP,        1)
REGDEF_BIT(DEBUG_SEL,        3)
REGDEF_END(DCE_FUNCTION_REGISTER)
//=============================================================================
// DCE CFA
//=============================================================================
/*
    CFA_EDGE_DTH :    [0x0, 0xfff],         bits : 11_0
    CFA_EDGE_DTH2:    [0x0, 0xfff],         bits : 23_12
*/
#define COLOR_INTERPOLATION_REGISTER1_OFS 0x00ac
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER1)
REGDEF_BIT(CFA_EDGE_DTH,        12)
REGDEF_BIT(CFA_EDGE_DTH2,        12)
REGDEF_END(COLOR_INTERPOLATION_REGISTER1)

/*
    CFA_LUMA_WEIGHT16:    [0x0, 0xff],          bits : 7_0
    CFA_FREQ_TH      :    [0x0, 0xfff],         bits : 27_16
*/
#define COLOR_INTERPOLATION_REGISTER9_OFS 0x00cc
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER9)
REGDEF_BIT(CFA_LUMA_WEIGHT16,        8)
REGDEF_BIT(,        8)
REGDEF_BIT(CFA_FREQ_TH,        12)
REGDEF_END(COLOR_INTERPOLATION_REGISTER9)

/*
    CFA_FCS_CORING:    [0x0, 0xff],         bits : 7_0
    CFA_FCS_WEIGHT:    [0x0, 0xff],         bits : 15_8
    CFA_FCS_DIRSEL:    [0x0, 0x1],          bits : 16
*/
#define COLOR_INTERPOLATION_REGISTER10_OFS 0x00d0
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER10)
REGDEF_BIT(CFA_FCS_CORING,        8)
REGDEF_BIT(CFA_FCS_WEIGHT,        8)
REGDEF_BIT(CFA_FCS_DIRSEL,        1)
REGDEF_END(COLOR_INTERPOLATION_REGISTER10)

/*
    CFA_FCS_STRENGTH0:    [0x0, 0xf],           bits : 3_0
    CFA_FCS_STRENGTH1:    [0x0, 0xf],           bits : 7_4
    CFA_FCS_STRENGTH2:    [0x0, 0xf],           bits : 11_8
    CFA_FCS_STRENGTH3:    [0x0, 0xf],           bits : 15_12
    CFA_FCS_STRENGTH4:    [0x0, 0xf],           bits : 19_16
    CFA_FCS_STRENGTH5:    [0x0, 0xf],           bits : 23_20
    CFA_FCS_STRENGTH6:    [0x0, 0xf],           bits : 27_24
    CFA_FCS_STRENGTH7:    [0x0, 0xf],           bits : 31_28
*/
#define COLOR_INTERPOLATION_REGISTER11_OFS 0x00d4
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER11)
REGDEF_BIT(CFA_FCS_STRENGTH0,        4)
REGDEF_BIT(CFA_FCS_STRENGTH1,        4)
REGDEF_BIT(CFA_FCS_STRENGTH2,        4)
REGDEF_BIT(CFA_FCS_STRENGTH3,        4)
REGDEF_BIT(CFA_FCS_STRENGTH4,        4)
REGDEF_BIT(CFA_FCS_STRENGTH5,        4)
REGDEF_BIT(CFA_FCS_STRENGTH6,        4)
REGDEF_BIT(CFA_FCS_STRENGTH7,        4)
REGDEF_END(COLOR_INTERPOLATION_REGISTER11)

/*
    CFA_FCS_STRENGTH8 :    [0x0, 0xf],          bits : 3_0
    CFA_FCS_STRENGTH9 :    [0x0, 0xf],          bits : 7_4
    CFA_FCS_STRENGTH10:    [0x0, 0xf],          bits : 11_8
    CFA_FCS_STRENGTH11:    [0x0, 0xf],          bits : 15_12
    CFA_FCS_STRENGTH12:    [0x0, 0xf],          bits : 19_16
    CFA_FCS_STRENGTH13:    [0x0, 0xf],          bits : 23_20
    CFA_FCS_STRENGTH14:    [0x0, 0xf],          bits : 27_24
    CFA_FCS_STRENGTH15:    [0x0, 0xf],          bits : 31_28
*/
#define COLOR_INTERPOLATION_REGISTER12_OFS 0x00d8
REGDEF_BEGIN(COLOR_INTERPOLATION_REGISTER12)
REGDEF_BIT(CFA_FCS_STRENGTH8,        4)
REGDEF_BIT(CFA_FCS_STRENGTH9,        4)
REGDEF_BIT(CFA_FCS_STRENGTH10,        4)
REGDEF_BIT(CFA_FCS_STRENGTH11,        4)
REGDEF_BIT(CFA_FCS_STRENGTH12,        4)
REGDEF_BIT(CFA_FCS_STRENGTH13,        4)
REGDEF_BIT(CFA_FCS_STRENGTH14,        4)
REGDEF_BIT(CFA_FCS_STRENGTH15,        4)
REGDEF_END(COLOR_INTERPOLATION_REGISTER12)

//=============================================================================
// DCE WDR
//=============================================================================
/*
    WDR_SUBIMG_WIDTH  :    [0x0, 0x1f],         bits : 4_0
    WDR_SUBIMG_HEIGHT :    [0x0, 0x1f],         bits : 9_5
    WDR_RAND_RESET    :    [0x0, 0x1],          bits : 10
    WDR_RAND_SEL      :    [0x0, 0x3],          bits : 12_11
*/
#define WDR_SUBIMAGE_REGISTER_0_OFS 0x0120
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_0)
REGDEF_BIT(WDR_SUBIMG_WIDTH,        5)
REGDEF_BIT(WDR_SUBIMG_HEIGHT,        5)
REGDEF_BIT(WDR_RAND_RESET,        1)
REGDEF_BIT(WDR_RAND_SEL,        2)
REGDEF_END(WDR_SUBIMAGE_REGISTER_0)

/*
    WDR_SUBIMG_LOFSI :    [0x0, 0x3fff],            bits : 15_2
*/
#define WDR_SUBIMAGE_REGISTER_2_OFS 0x0128
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_2)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_SUBIMG_LOFSI,        14)
REGDEF_END(WDR_SUBIMAGE_REGISTER_2)

/*
    WDR_SUBIMG_LOFSO :    [0x0, 0x3fff],            bits : 15_2
*/
#define WDR_SUBIMAGE_REGISTER_4_OFS 0x0130
REGDEF_BEGIN(WDR_SUBIMAGE_REGISTER_4)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_SUBIMG_LOFSO,        14)
REGDEF_END(WDR_SUBIMAGE_REGISTER_4)

/*
    WDR_OUTBLD_TABLE_EN :    [0x0, 0x1],            bits : 0
    WDR_GAINCTRL_EN     :    [0x0, 0x1],            bits : 1
    WDR_MAXGAIN         :    [0x0, 0xff],           bits : 23_16
    WDR_MINGAIN         :    [0x0, 0xff],           bits : 31_24
*/
#define WDR_CONTROL_REGISTER_OFS 0x0164
REGDEF_BEGIN(WDR_CONTROL_REGISTER)
REGDEF_BIT(WDR_OUTBLD_TABLE_EN,        1)
REGDEF_BIT(WDR_GAINCTRL_EN,        1)
REGDEF_BIT(,        14)
REGDEF_BIT(WDR_MAXGAIN,        8)
REGDEF_BIT(WDR_MINGAIN,        8)
REGDEF_END(WDR_CONTROL_REGISTER)

/*
    WDR_OUTBLD_INDEX_LUT0:    [0x0, 0x3f],          bits : 5_0
    WDR_OUTBLD_INDEX_LUT1:    [0x0, 0x3f],          bits : 13_8
    WDR_OUTBLD_INDEX_LUT2:    [0x0, 0x3f],          bits : 21_16
    WDR_OUTBLD_INDEX_LUT3:    [0x0, 0x3f],          bits : 29_24
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER0_OFS 0x0214
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER0)
REGDEF_BIT(WDR_OUTBLD_INDEX_LUT0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_OUTBLD_INDEX_LUT1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_OUTBLD_INDEX_LUT2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_OUTBLD_INDEX_LUT3,        6)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER0)

/*
    WDR_OUTBLD_SPLIT_LUT0 :    [0x0, 0x3],          bits : 1_0
    WDR_OUTBLD_SPLIT_LUT1 :    [0x0, 0x3],          bits : 3_2
    WDR_OUTBLD_SPLIT_LUT2 :    [0x0, 0x3],          bits : 5_4
    WDR_OUTBLD_SPLIT_LUT3 :    [0x0, 0x3],          bits : 7_6
    WDR_OUTBLD_SPLIT_LUT4 :    [0x0, 0x3],          bits : 9_8
    WDR_OUTBLD_SPLIT_LUT5 :    [0x0, 0x3],          bits : 11_10
    WDR_OUTBLD_SPLIT_LUT6 :    [0x0, 0x3],          bits : 13_12
    WDR_OUTBLD_SPLIT_LUT7 :    [0x0, 0x3],          bits : 15_14
    WDR_OUTBLD_SPLIT_LUT8 :    [0x0, 0x3],          bits : 17_16
    WDR_OUTBLD_SPLIT_LUT9 :    [0x0, 0x3],          bits : 19_18
    WDR_OUTBLD_SPLIT_LUT10:    [0x0, 0x3],          bits : 21_20
    WDR_OUTBLD_SPLIT_LUT11:    [0x0, 0x3],          bits : 23_22
    WDR_OUTBLD_SPLIT_LUT12:    [0x0, 0x3],          bits : 25_24
    WDR_OUTBLD_SPLIT_LUT13:    [0x0, 0x3],          bits : 27_26
    WDR_OUTBLD_SPLIT_LUT14:    [0x0, 0x3],          bits : 29_28
    WDR_OUTBLD_SPLIT_LUT15:    [0x0, 0x3],          bits : 31_30
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER8_OFS 0x0234
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER8)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT0,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT1,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT2,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT3,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT4,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT5,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT6,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT7,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT8,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT9,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT10,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT11,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT12,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT13,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT14,        2)
REGDEF_BIT(WDR_OUTBLD_SPLIT_LUT15,        2)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER8)

/*
    WDR_OUTBLD_VAL_LUT0:    [0x0, 0xfff],           bits : 11_0
    WDR_OUTBLD_VAL_LUT1:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER10_OFS 0x023c
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER10)
REGDEF_BIT(WDR_OUTBLD_VAL_LUT0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_OUTBLD_VAL_LUT1,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER10)


/*
    WDR_OUTBLD_VAL_LUT64:    [0x0, 0xfff],          bits : 11_0
*/
#define WDR_OUTPUT_BLENDING_CURVE_REGISTER42_OFS 0x02bc
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_CURVE_REGISTER42)
REGDEF_BIT(WDR_OUTBLD_VAL_LUT64,        12)
REGDEF_END(WDR_OUTPUT_BLENDING_CURVE_REGISTER42)


/*
    WDR_STRENGTH :    [0x0, 0xff],          bits : 7_0
    WDR_CONTRAST :    [0x0, 0xff],          bits : 15_8
*/
#define WDR_OUTPUT_BLENDING_REGISTER_OFS 0x02c0
REGDEF_BEGIN(WDR_OUTPUT_BLENDING_REGISTER)
REGDEF_BIT(WDR_STRENGTH,        8)
REGDEF_BIT(WDR_CONTRAST,        8)
REGDEF_END(WDR_OUTPUT_BLENDING_REGISTER)

//=============================================================================
// DCE TONE CURVE
//=============================================================================

/*
    WDR_INPUT_BLDRTO0 :    [0x0, 0xff],         bits : 7_0
    WDR_INPUT_BLDRTO1 :    [0x0, 0xff],         bits : 15_8
    WDR_INPUT_BLDRTO2 :    [0x0, 0xff],         bits : 23_16
    WDR_INPUT_BLDRTO3 :    [0x0, 0xff],         bits : 31_24
*/
#define WDR_INPUT_BLENDING_REGISTER0_OFS 0x0144
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER0)
REGDEF_BIT(WDR_INPUT_BLDRTO0,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO1,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO2,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO3,        8)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER0)


/*
    WDR_INPUT_BLDRTO4  :    [0x0, 0xff],            bits : 7_0
    WDR_INPUT_BLDRTO5  :    [0x0, 0xff],            bits : 15_8
    WDR_INPUT_BLDRTO6  :    [0x0, 0xff],            bits : 23_16
    WDR_INPUT_BLDRTO7  :    [0x0, 0xff],            bits : 31_24
*/
#define WDR_INPUT_BLENDING_REGISTER1_OFS 0x0148
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER1)
REGDEF_BIT(WDR_INPUT_BLDRTO4,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO5,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO6,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO7,        8)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER1)


/*
    WDR_INPUT_BLDRTO8  :    [0x0, 0xff],            bits : 7_0
    WDR_INPUT_BLDRTO9  :    [0x0, 0xff],            bits : 15_8
    WDR_INPUT_BLDRTO10 :    [0x0, 0xff],            bits : 23_16
    WDR_INPUT_BLDRTO11 :    [0x0, 0xff],            bits : 31_24
*/
#define WDR_INPUT_BLENDING_REGISTER2_OFS 0x014c
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER2)
REGDEF_BIT(WDR_INPUT_BLDRTO8,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO9,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO10,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO11,        8)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER2)


/*
    WDR_INPUT_BLDRTO12  :    [0x0, 0xff],           bits : 7_0
    WDR_INPUT_BLDRTO13  :    [0x0, 0xff],           bits : 15_8
    WDR_INPUT_BLDRTO14  :    [0x0, 0xff],           bits : 23_16
    WDR_INPUT_BLDRTO15  :    [0x0, 0xff],           bits : 31_24
*/
#define WDR_INPUT_BLENDING_REGISTER3_OFS 0x0150
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER3)
REGDEF_BIT(WDR_INPUT_BLDRTO12,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO13,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO14,        8)
REGDEF_BIT(WDR_INPUT_BLDRTO15,        8)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER3)


/*
    WDR_INPUT_BLDRTO16   :    [0x0, 0xff],          bits : 7_0
    WDR_INPUT_BLDSRC_SEL :    [0x0, 0x3],           bits : 27_26
    WDR_INPUT_BLDWT      :    [0x0, 0xf],           bits : 31_28
*/
#define WDR_INPUT_BLENDING_REGISTER4_OFS 0x0154
REGDEF_BEGIN(WDR_INPUT_BLENDING_REGISTER4)
REGDEF_BIT(WDR_INPUT_BLDRTO16,        8)
REGDEF_BIT(,        18)
REGDEF_BIT(WDR_INPUT_BLDSRC_SEL,        2)
REGDEF_BIT(WDR_INPUT_BLDWT,        4)
REGDEF_END(WDR_INPUT_BLENDING_REGISTER4)

/*
    WDR_TCURVE_INDEX_LUT0:    [0x0, 0x3f],          bits : 5_0
    WDR_TCURVE_INDEX_LUT1:    [0x0, 0x3f],          bits : 13_8
    WDR_TCURVE_INDEX_LUT2:    [0x0, 0x3f],          bits : 21_16
    WDR_TCURVE_INDEX_LUT3:    [0x0, 0x3f],          bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER0_OFS 0x0168
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER0)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT3,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER0)


/*
    WDR_TCURVE_INDEX_LUT4:    [0x0, 0x3f],          bits : 5_0
    WDR_TCURVE_INDEX_LUT5:    [0x0, 0x3f],          bits : 13_8
    WDR_TCURVE_INDEX_LUT6:    [0x0, 0x3f],          bits : 21_16
    WDR_TCURVE_INDEX_LUT7:    [0x0, 0x3f],          bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER1_OFS 0x016c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER1)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT7,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER1)


/*
    WDR_TCURVE_INDEX_LUT8 :    [0x0, 0x3f],         bits : 5_0
    WDR_TCURVE_INDEX_LUT9 :    [0x0, 0x3f],         bits : 13_8
    WDR_TCURVE_INDEX_LUT10:    [0x0, 0x3f],         bits : 21_16
    WDR_TCURVE_INDEX_LUT11:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER2_OFS 0x0170
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT8,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT9,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT10,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT11,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER2)


/*
    WDR_TCURVE_INDEX_LUT12:    [0x0, 0x3f],         bits : 5_0
    WDR_TCURVE_INDEX_LUT13:    [0x0, 0x3f],         bits : 13_8
    WDR_TCURVE_INDEX_LUT14:    [0x0, 0x3f],         bits : 21_16
    WDR_TCURVE_INDEX_LUT15:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER3_OFS 0x0174
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER3)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT12,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT13,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT14,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT15,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER3)


/*
    WDR_TCURVE_INDEX_LUT16:    [0x0, 0x3f],         bits : 5_0
    WDR_TCURVE_INDEX_LUT17:    [0x0, 0x3f],         bits : 13_8
    WDR_TCURVE_INDEX_LUT18:    [0x0, 0x3f],         bits : 21_16
    WDR_TCURVE_INDEX_LUT19:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER4_OFS 0x0178
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER4)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT16,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT17,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT18,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT19,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER4)


/*
    WDR_TCURVE_INDEX_LUT20:    [0x0, 0x3f],         bits : 5_0
    WDR_TCURVE_INDEX_LUT21:    [0x0, 0x3f],         bits : 13_8
    WDR_TCURVE_INDEX_LUT22:    [0x0, 0x3f],         bits : 21_16
    WDR_TCURVE_INDEX_LUT23:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER5_OFS 0x017c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER5)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT20,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT21,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT22,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT23,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER5)


/*
    WDR_TCURVE_INDEX_LUT24:    [0x0, 0x3f],         bits : 5_0
    WDR_TCURVE_INDEX_LUT25:    [0x0, 0x3f],         bits : 13_8
    WDR_TCURVE_INDEX_LUT26:    [0x0, 0x3f],         bits : 21_16
    WDR_TCURVE_INDEX_LUT27:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER6_OFS 0x0180
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER6)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT24,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT25,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT26,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT27,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER6)


/*
    WDR_TCURVE_INDEX_LUT28:    [0x0, 0x3f],         bits : 5_0
    WDR_TCURVE_INDEX_LUT29:    [0x0, 0x3f],         bits : 13_8
    WDR_TCURVE_INDEX_LUT30:    [0x0, 0x3f],         bits : 21_16
    WDR_TCURVE_INDEX_LUT31:    [0x0, 0x3f],         bits : 29_24
*/
#define WDR_TONE_CURVE_REGISTER7_OFS 0x0184
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER7)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT28,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT29,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT30,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(WDR_TCURVE_INDEX_LUT31,        6)
REGDEF_END(WDR_TONE_CURVE_REGISTER7)


/*
    WDR_TCURVE_SPLIT_LUT0  :    [0x0, 0x3],         bits : 1_0
    WDR_TCURVE_SPLIT_LUT1  :    [0x0, 0x3],         bits : 3_2
    WDR_TCURVE_SPLIT_LUT2  :    [0x0, 0x3],         bits : 5_4
    WDR_TCURVE_SPLIT_LUT3  :    [0x0, 0x3],         bits : 7_6
    WDR_TCURVE_SPLIT_LUT4  :    [0x0, 0x3],         bits : 9_8
    WDR_TCURVE_SPLIT_LUT5  :    [0x0, 0x3],         bits : 11_10
    WDR_TCURVE_SPLIT_LUT6  :    [0x0, 0x3],         bits : 13_12
    WDR_TCURVE_SPLIT_LUT7  :    [0x0, 0x3],         bits : 15_14
    WDR_TCURVE_SPLIT_LUT8  :    [0x0, 0x3],         bits : 17_16
    WDR_TCURVE_SPLIT_LUT9  :    [0x0, 0x3],         bits : 19_18
    WDR_TCURVE_SPLIT_LUT10 :    [0x0, 0x3],         bits : 21_20
    WDR_TCURVE_SPLIT_LUT11 :    [0x0, 0x3],         bits : 23_22
    WDR_TCURVE_SPLIT_LUT12 :    [0x0, 0x3],         bits : 25_24
    WDR_TCURVE_SPLIT_LUT13 :    [0x0, 0x3],         bits : 27_26
    WDR_TCURVE_SPLIT_LUT14 :    [0x0, 0x3],         bits : 29_28
    WDR_TCURVE_SPLIT_LUT15 :    [0x0, 0x3],         bits : 31_30
*/
#define WDR_TONE_CURVE_REGISTER8_OFS 0x0188
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER8)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT0,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT1,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT2,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT3,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT4,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT5,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT6,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT7,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT8,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT9,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT10,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT11,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT12,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT13,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT14,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT15,        2)
REGDEF_END(WDR_TONE_CURVE_REGISTER8)


/*
    WDR_TCURVE_SPLIT_LUT16  :    [0x0, 0x3],            bits : 1_0
    WDR_TCURVE_SPLIT_LUT17  :    [0x0, 0x3],            bits : 3_2
    WDR_TCURVE_SPLIT_LUT18  :    [0x0, 0x3],            bits : 5_4
    WDR_TCURVE_SPLIT_LUT19  :    [0x0, 0x3],            bits : 7_6
    WDR_TCURVE_SPLIT_LUT20  :    [0x0, 0x3],            bits : 9_8
    WDR_TCURVE_SPLIT_LUT21  :    [0x0, 0x3],            bits : 11_10
    WDR_TCURVE_SPLIT_LUT22  :    [0x0, 0x3],            bits : 13_12
    WDR_TCURVE_SPLIT_LUT23  :    [0x0, 0x3],            bits : 15_14
    WDR_TCURVE_SPLIT_LUT24  :    [0x0, 0x3],            bits : 17_16
    WDR_TCURVE_SPLIT_LUT25  :    [0x0, 0x3],            bits : 19_18
    WDR_TCURVE_SPLIT_LUT26  :    [0x0, 0x3],            bits : 21_20
    WDR_TCURVE_SPLIT_LUT27  :    [0x0, 0x3],            bits : 23_22
    WDR_TCURVE_SPLIT_LUT28  :    [0x0, 0x3],            bits : 25_24
    WDR_TCURVE_SPLIT_LUT29  :    [0x0, 0x3],            bits : 27_26
    WDR_TCURVE_SPLIT_LUT30  :    [0x0, 0x3],            bits : 29_28
    WDR_TCURVE_SPLIT_LUT31  :    [0x0, 0x3],            bits : 31_30
*/
#define WDR_TONE_CURVE_REGISTER9_OFS 0x018c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER9)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT16,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT17,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT18,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT19,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT20,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT21,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT22,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT23,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT24,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT25,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT26,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT27,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT28,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT29,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT30,        2)
REGDEF_BIT(WDR_TCURVE_SPLIT_LUT31,        2)
REGDEF_END(WDR_TONE_CURVE_REGISTER9)

/*
    WDR_TCURVE_VAL_LUT0:    [0x0, 0xfff],           bits : 11_0
    WDR_TCURVE_VAL_LUT1:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER10_OFS 0x0190
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER10)
REGDEF_BIT(WDR_TCURVE_VAL_LUT0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT1,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER10)


/*
    WDR_TCURVE_VAL_LUT2:    [0x0, 0xfff],           bits : 11_0
    WDR_TCURVE_VAL_LUT3:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER11_OFS 0x0194
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER11)
REGDEF_BIT(WDR_TCURVE_VAL_LUT2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT3,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER11)


/*
    WDR_TCURVE_VAL_LUT4:    [0x0, 0xfff],           bits : 11_0
    WDR_TCURVE_VAL_LUT5:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER12_OFS 0x0198
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER12)
REGDEF_BIT(WDR_TCURVE_VAL_LUT4,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT5,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER12)


/*
    WDR_TCURVE_VAL_LUT6:    [0x0, 0xfff],           bits : 11_0
    WDR_TCURVE_VAL_LUT7:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER13_OFS 0x019c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER13)
REGDEF_BIT(WDR_TCURVE_VAL_LUT6,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT7,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER13)


/*
    WDR_TCURVE_VAL_LUT8:    [0x0, 0xfff],           bits : 11_0
    WDR_TCURVE_VAL_LUT9:    [0x0, 0xfff],           bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER14_OFS 0x01a0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER14)
REGDEF_BIT(WDR_TCURVE_VAL_LUT8,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT9,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER14)


/*
    WDR_TCURVE_VAL_LUT10:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT11:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER15_OFS 0x01a4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER15)
REGDEF_BIT(WDR_TCURVE_VAL_LUT10,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT11,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER15)


/*
    WDR_TCURVE_VAL_LUT12:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT13:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER16_OFS 0x01a8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER16)
REGDEF_BIT(WDR_TCURVE_VAL_LUT12,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT13,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER16)


/*
    WDR_TCURVE_VAL_LUT14:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT15:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER17_OFS 0x01ac
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER17)
REGDEF_BIT(WDR_TCURVE_VAL_LUT14,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT15,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER17)


/*
    WDR_TCURVE_VAL_LUT16:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT17:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER18_OFS 0x01b0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER18)
REGDEF_BIT(WDR_TCURVE_VAL_LUT16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT17,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER18)


/*
    WDR_TCURVE_VAL_LUT18:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT19:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER19_OFS 0x01b4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER19)
REGDEF_BIT(WDR_TCURVE_VAL_LUT18,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT19,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER19)


/*
    WDR_TCURVE_VAL_LUT20:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT21:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER20_OFS 0x01b8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER20)
REGDEF_BIT(WDR_TCURVE_VAL_LUT20,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT21,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER20)


/*
    WDR_TCURVE_VAL_LUT22:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT23:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER21_OFS 0x01bc
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER21)
REGDEF_BIT(WDR_TCURVE_VAL_LUT22,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT23,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER21)


/*
    WDR_TCURVE_VAL_LUT24:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT25:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER22_OFS 0x01c0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER22)
REGDEF_BIT(WDR_TCURVE_VAL_LUT24,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT25,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER22)


/*
    WDR_TCURVE_VAL_LUT26:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT27:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER23_OFS 0x01c4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER23)
REGDEF_BIT(WDR_TCURVE_VAL_LUT26,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT27,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER23)


/*
    WDR_TCURVE_VAL_LUT28:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT29:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER24_OFS 0x01c8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER24)
REGDEF_BIT(WDR_TCURVE_VAL_LUT28,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT29,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER24)


/*
    WDR_TCURVE_VAL_LUT30:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT31:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER25_OFS 0x01cc
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER25)
REGDEF_BIT(WDR_TCURVE_VAL_LUT30,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT31,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER25)


/*
    WDR_TCURVE_VAL_LUT32:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT33:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER26_OFS 0x01d0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER26)
REGDEF_BIT(WDR_TCURVE_VAL_LUT32,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT33,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER26)


/*
    WDR_TCURVE_VAL_LUT34:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT35:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER27_OFS 0x01d4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER27)
REGDEF_BIT(WDR_TCURVE_VAL_LUT34,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT35,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER27)


/*
    WDR_TCURVE_VAL_LUT36:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT37:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER28_OFS 0x01d8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER28)
REGDEF_BIT(WDR_TCURVE_VAL_LUT36,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT37,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER28)


/*
    WDR_TCURVE_VAL_LUT38:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT39:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER29_OFS 0x01dc
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER29)
REGDEF_BIT(WDR_TCURVE_VAL_LUT38,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT39,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER29)


/*
    WDR_TCURVE_VAL_LUT40:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT41:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER30_OFS 0x01e0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER30)
REGDEF_BIT(WDR_TCURVE_VAL_LUT40,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT41,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER30)


/*
    WDR_TCURVE_VAL_LUT42:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT43:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER31_OFS 0x01e4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER31)
REGDEF_BIT(WDR_TCURVE_VAL_LUT42,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT43,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER31)


/*
    WDR_TCURVE_VAL_LUT44:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT45:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER32_OFS 0x01e8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER32)
REGDEF_BIT(WDR_TCURVE_VAL_LUT44,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT45,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER32)


/*
    WDR_TCURVE_VAL_LUT46:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT47:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER33_OFS 0x01ec
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER33)
REGDEF_BIT(WDR_TCURVE_VAL_LUT46,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT47,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER33)


/*
    WDR_TCURVE_VAL_LUT48:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT49:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER34_OFS 0x01f0
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER34)
REGDEF_BIT(WDR_TCURVE_VAL_LUT48,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT49,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER34)


/*
    WDR_TCURVE_VAL_LUT50:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT51:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER35_OFS 0x01f4
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER35)
REGDEF_BIT(WDR_TCURVE_VAL_LUT50,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT51,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER35)


/*
    WDR_TCURVE_VAL_LUT52:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT53:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER36_OFS 0x01f8
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER36)
REGDEF_BIT(WDR_TCURVE_VAL_LUT52,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT53,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER36)


/*
    WDR_TCURVE_VAL_LUT54:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT55:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER37_OFS 0x01fc
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER37)
REGDEF_BIT(WDR_TCURVE_VAL_LUT54,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT55,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER37)


/*
    WDR_TCURVE_VAL_LUT56:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT57:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER38_OFS 0x0200
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER38)
REGDEF_BIT(WDR_TCURVE_VAL_LUT56,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT57,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER38)


/*
    WDR_TCURVE_VAL_LUT58:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT59:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER39_OFS 0x0204
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER39)
REGDEF_BIT(WDR_TCURVE_VAL_LUT58,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT59,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER39)


/*
    WDR_TCURVE_VAL_LUT60:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT61:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER40_OFS 0x0208
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER40)
REGDEF_BIT(WDR_TCURVE_VAL_LUT60,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT61,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER40)


/*
    WDR_TCURVE_VAL_LUT62:    [0x0, 0xfff],          bits : 11_0
    WDR_TCURVE_VAL_LUT63:    [0x0, 0xfff],          bits : 27_16
*/
#define WDR_TONE_CURVE_REGISTER41_OFS 0x020c
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER41)
REGDEF_BIT(WDR_TCURVE_VAL_LUT62,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_TCURVE_VAL_LUT63,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER41)


/*
    WDR_TCURVE_VAL_LUT64:    [0x0, 0xfff],          bits : 11_0
*/
#define WDR_TONE_CURVE_REGISTER42_OFS 0x0210
REGDEF_BEGIN(WDR_TONE_CURVE_REGISTER42)
REGDEF_BIT(WDR_TCURVE_VAL_LUT64,        12)
REGDEF_END(WDR_TONE_CURVE_REGISTER42)


/*
    WDR_SAT_TH    :    [0x0, 0xfff],            bits : 11_0
    WDR_SAT_WT_LOW:    [0x0, 0xff],         bits : 23_16
    WDR_SAT_DELTA :    [0x0, 0xff],         bits : 31_24
*/
#define WDR_SATURATION_REDUCTION_REGISTER_OFS 0x02c4
REGDEF_BEGIN(WDR_SATURATION_REDUCTION_REGISTER)
REGDEF_BIT(WDR_SAT_TH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(WDR_SAT_WT_LOW,        8)
REGDEF_BIT(WDR_SAT_DELTA,        8)
REGDEF_END(WDR_SATURATION_REDUCTION_REGISTER)

//=============================================================================
// DCE HIST
//=============================================================================
/*
    HISTOGRAM_BIN0:    [0x0, 0xffff],           bits : 15_0
    HISTOGRAM_BIN1:    [0x0, 0xffff],           bits : 31_16
*/
#define HISTOGRAM_REGISTER1_OFS 0x02d4
REGDEF_BEGIN(HISTOGRAM_REGISTER1)
REGDEF_BIT(HISTOGRAM_BIN0,        16)
REGDEF_BIT(HISTOGRAM_BIN1,        16)
REGDEF_END(HISTOGRAM_REGISTER1)

//=============================================================================
// DCE LDC
//=============================================================================
/*
    GDC_CENTX:    [0x0, 0x3fff],            bits : 13_0
    GDC_CENTY:    [0x0, 0x3fff],            bits : 29_16
*/
#define GEO_CENTER_REGISTER_OFS 0x0404
REGDEF_BEGIN(GEO_CENTER_REGISTER)
REGDEF_BIT(GDC_CENTX,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(GDC_CENTY,        14)
REGDEF_END(GEO_CENTER_REGISTER)

/*
    GDC_FOVGAIN :    [0x0, 0xfff],          bits : 11_0
    GDC_FOVBOUND:    [0x0, 0x1],            bits : 12
*/
#define GDC_FOV_REGISTER_OFS 0x0410
REGDEF_BEGIN(GDC_FOV_REGISTER)
REGDEF_BIT(GDC_FOVGAIN,        12)
REGDEF_BIT(GDC_FOVBOUND,        1)
REGDEF_END(GDC_FOV_REGISTER)

/*
    CAC_RLUTGAIN:    [0x0, 0x1fff],         bits : 12_0
    CAC_GLUTGAIN:    [0x0, 0x1fff],         bits : 28_16
*/
#define GEO_ABERRATION_REGISTER0_OFS 0x0418
REGDEF_BEGIN(GEO_ABERRATION_REGISTER0)
REGDEF_BIT(CAC_RLUTGAIN,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(CAC_GLUTGAIN,        13)
REGDEF_END(GEO_ABERRATION_REGISTER0)


/*
    CAC_BLUTGAIN:    [0x0, 0x1fff],         bits : 12_0
    CAC_SEL     :    [0x0, 0x1],            bits : 16
*/
#define GEO_ABERRATION_REGISTER1_OFS 0x041c
REGDEF_BEGIN(GEO_ABERRATION_REGISTER1)
REGDEF_BIT(CAC_BLUTGAIN,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(CAC_SEL,        1)
REGDEF_END(GEO_ABERRATION_REGISTER1)

/*
    GDC_LUTG0:    [0x0, 0xffff],            bits : 15_0
    GDC_LUTG1:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG1_OFS 0x0440
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG1)
REGDEF_BIT(GDC_LUTG0,        16)
REGDEF_BIT(GDC_LUTG1,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG1)


/*
    GDC_LUTG2:    [0x0, 0xffff],            bits : 15_0
    GDC_LUTG3:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG2_OFS 0x0444
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG2)
REGDEF_BIT(GDC_LUTG2,        16)
REGDEF_BIT(GDC_LUTG3,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG2)


/*
    GDC_LUTG4:    [0x0, 0xffff],            bits : 15_0
    GDC_LUTG5:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG3_OFS 0x0448
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG3)
REGDEF_BIT(GDC_LUTG4,        16)
REGDEF_BIT(GDC_LUTG5,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG3)


/*
    GDC_LUTG6:    [0x0, 0xffff],            bits : 15_0
    GDC_LUTG7:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG4_OFS 0x044c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG4)
REGDEF_BIT(GDC_LUTG6,        16)
REGDEF_BIT(GDC_LUTG7,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG4)


/*
    GDC_LUTG8:    [0x0, 0xffff],            bits : 15_0
    GDC_LUTG9:    [0x0, 0xffff],            bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG5_OFS 0x0450
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG5)
REGDEF_BIT(GDC_LUTG8,        16)
REGDEF_BIT(GDC_LUTG9,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG5)


/*
    GDC_LUTG10:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG11:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG6_OFS 0x0454
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG6)
REGDEF_BIT(GDC_LUTG10,        16)
REGDEF_BIT(GDC_LUTG11,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG6)


/*
    GDC_LUTG12:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG13:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG7_OFS 0x0458
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG7)
REGDEF_BIT(GDC_LUTG12,        16)
REGDEF_BIT(GDC_LUTG13,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG7)


/*
    GDC_LUTG14:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG15:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG8_OFS 0x045c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG8)
REGDEF_BIT(GDC_LUTG14,        16)
REGDEF_BIT(GDC_LUTG15,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG8)


/*
    GDC_LUTG16:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG17:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG9_OFS 0x0460
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG9)
REGDEF_BIT(GDC_LUTG16,        16)
REGDEF_BIT(GDC_LUTG17,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG9)


/*
    GDC_LUTG18:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG19:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG10_OFS 0x0464
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG10)
REGDEF_BIT(GDC_LUTG18,        16)
REGDEF_BIT(GDC_LUTG19,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG10)


/*
    GDC_LUTG20:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG21:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG11_OFS 0x0468
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG11)
REGDEF_BIT(GDC_LUTG20,        16)
REGDEF_BIT(GDC_LUTG21,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG11)


/*
    GDC_LUTG22:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG23:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG12_OFS 0x046c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG12)
REGDEF_BIT(GDC_LUTG22,        16)
REGDEF_BIT(GDC_LUTG23,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG12)


/*
    GDC_LUTG24:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG25:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG13_OFS 0x0470
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG13)
REGDEF_BIT(GDC_LUTG24,        16)
REGDEF_BIT(GDC_LUTG25,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG13)


/*
    GDC_LUTG26:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG27:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG14_OFS 0x0474
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG14)
REGDEF_BIT(GDC_LUTG26,        16)
REGDEF_BIT(GDC_LUTG27,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG14)


/*
    GDC_LUTG28:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG29:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG15_OFS 0x0478
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG15)
REGDEF_BIT(GDC_LUTG28,        16)
REGDEF_BIT(GDC_LUTG29,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG15)


/*
    GDC_LUTG30:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG31:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG16_OFS 0x047c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG16)
REGDEF_BIT(GDC_LUTG30,        16)
REGDEF_BIT(GDC_LUTG31,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG16)


/*
    GDC_LUTG32:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG33:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG17_OFS 0x0480
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG17)
REGDEF_BIT(GDC_LUTG32,        16)
REGDEF_BIT(GDC_LUTG33,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG17)


/*
    GDC_LUTG34:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG35:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG18_OFS 0x0484
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG18)
REGDEF_BIT(GDC_LUTG34,        16)
REGDEF_BIT(GDC_LUTG35,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG18)


/*
    GDC_LUTG36:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG37:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG19_OFS 0x0488
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG19)
REGDEF_BIT(GDC_LUTG36,        16)
REGDEF_BIT(GDC_LUTG37,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG19)


/*
    GDC_LUTG38:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG39:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG20_OFS 0x048c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG20)
REGDEF_BIT(GDC_LUTG38,        16)
REGDEF_BIT(GDC_LUTG39,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG20)


/*
    GDC_LUTG40:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG41:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG21_OFS 0x0490
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG21)
REGDEF_BIT(GDC_LUTG40,        16)
REGDEF_BIT(GDC_LUTG41,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG21)


/*
    GDC_LUTG42:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG43:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG22_OFS 0x0494
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG22)
REGDEF_BIT(GDC_LUTG42,        16)
REGDEF_BIT(GDC_LUTG43,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG22)


/*
    GDC_LUTG44:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG45:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG23_OFS 0x0498
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG23)
REGDEF_BIT(GDC_LUTG44,        16)
REGDEF_BIT(GDC_LUTG45,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG23)


/*
    GDC_LUTG46:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG47:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG24_OFS 0x049c
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG24)
REGDEF_BIT(GDC_LUTG46,        16)
REGDEF_BIT(GDC_LUTG47,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG24)


/*
    GDC_LUTG48:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG49:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG25_OFS 0x04a0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG25)
REGDEF_BIT(GDC_LUTG48,        16)
REGDEF_BIT(GDC_LUTG49,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG25)


/*
    GDC_LUTG50:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG51:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG26_OFS 0x04a4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG26)
REGDEF_BIT(GDC_LUTG50,        16)
REGDEF_BIT(GDC_LUTG51,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG26)


/*
    GDC_LUTG52:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG53:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG27_OFS 0x04a8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG27)
REGDEF_BIT(GDC_LUTG52,        16)
REGDEF_BIT(GDC_LUTG53,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG27)


/*
    GDC_LUTG54:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG55:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG28_OFS 0x04ac
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG28)
REGDEF_BIT(GDC_LUTG54,        16)
REGDEF_BIT(GDC_LUTG55,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG28)


/*
    GDC_LUTG56:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG57:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG29_OFS 0x04b0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG29)
REGDEF_BIT(GDC_LUTG56,        16)
REGDEF_BIT(GDC_LUTG57,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG29)


/*
    GDC_LUTG58:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG59:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG30_OFS 0x04b4
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG30)
REGDEF_BIT(GDC_LUTG58,        16)
REGDEF_BIT(GDC_LUTG59,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG30)


/*
    GDC_LUTG60:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG61:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG31_OFS 0x04b8
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG31)
REGDEF_BIT(GDC_LUTG60,        16)
REGDEF_BIT(GDC_LUTG61,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG31)


/*
    GDC_LUTG62:    [0x0, 0xffff],           bits : 15_0
    GDC_LUTG63:    [0x0, 0xffff],           bits : 31_16
*/
#define GEOMETRIC_LOOK_UP_TABLEG32_OFS 0x04bc
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG32)
REGDEF_BIT(GDC_LUTG62,        16)
REGDEF_BIT(GDC_LUTG63,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG32)

/*
    GDC_LUTG64:    [0x0, 0xffff],           bits : 15_0
*/
#define GEOMETRIC_LOOK_UP_TABLEG33_OFS 0x04c0
REGDEF_BEGIN(GEOMETRIC_LOOK_UP_TABLEG33)
REGDEF_BIT(GDC_LUTG64,        16)
REGDEF_END(GEOMETRIC_LOOK_UP_TABLEG33)

/*
    DRAM_SAI2DLUT:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_LUT2D_IN_ADDRESS_OFS 0x05f0
REGDEF_BEGIN(DMA_LUT2D_IN_ADDRESS)
REGDEF_BIT(,        2)
REGDEF_BIT(DRAM_SAI2DLUT,        30)
REGDEF_END(DMA_LUT2D_IN_ADDRESS)

/*
    LUT2D_HFACT:    [0x0, 0xffffff],            bits : 23_0
*/
#define LUT2D_REGISTER3_OFS 0x05fc
REGDEF_BEGIN(LUT2D_REGISTER3)
REGDEF_BIT(LUT2D_HFACT,        24)
REGDEF_END(LUT2D_REGISTER3)


/*
    LUT2D_VFACT           :    [0x0, 0xffffff],         bits : 23_0
    LUT2D_NUMSEL          :    [0x0, 0x3],          bits : 29_28
    LUT2D_TOP_YMIN_AUTO_EN:    [0x0, 0x1],          bits : 31
*/
#define LUT2D_REGISTER4_OFS 0x0600
REGDEF_BEGIN(LUT2D_REGISTER4)
REGDEF_BIT(LUT2D_VFACT,        24)
REGDEF_BIT(,        4)
REGDEF_BIT(LUT2D_NUMSEL,        2)
REGDEF_BIT(,        1)
REGDEF_BIT(LUT2D_TOP_YMIN_AUTO_EN,        1)
REGDEF_END(LUT2D_REGISTER4)

// NOTE: IPE
//=============================================================================
// IPE CONTROL
//=============================================================================
/*
    reg_ipe_swrst      :    [0x0, 0x1],         bits : 0
    reg_ipe_start      :    [0x0, 0x1],         bits : 1
    reg_ipe_load_start :    [0x0, 0x1],         bits : 2
    reg_ipe_load_fd    :    [0x0, 0x1],         bits : 3
    reg_ipe_load_fs    :    [0x0, 0x1],         bits : 4
    reg_ipe_rwgamma    :    [0x0, 0x3],         bits : 9_8
    reg_ipe_rwgamma_opt:    [0x0, 0x3],         bits : 11_10
    reg_ll_fire        :    [0x0, 0x1],         bits : 28
*/
#define IPE_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(IPE_CONTROL_REGISTER)
REGDEF_BIT(reg_ipe_swrst,           1)
REGDEF_BIT(reg_ipe_start,           1)
REGDEF_BIT(reg_ipe_load_start,      1)
REGDEF_BIT(reg_ipe_load_fd,         1)
REGDEF_BIT(reg_ipe_load_fs,         1)
REGDEF_BIT(,                        3)
REGDEF_BIT(reg_ipe_rwgamma,         2)
REGDEF_BIT(reg_ipe_rwgamma_opt,     2)
REGDEF_BIT(,                       16)
REGDEF_BIT(reg_ll_fire,             1)
REGDEF_END(IPE_CONTROL_REGISTER)

/*
    reg_rgblpf_en          :    [0x0, 0x1],         bits : 0
    reg_rgbgamma_en        :    [0x0, 0x1],         bits : 1
    reg_ycurve_en          :    [0x0, 0x1],         bits : 2
    reg_cr_en              :    [0x0, 0x1],         bits : 3
    reg_defog_subimg_out_en:    [0x0, 0x1],         bits : 4
    reg_defog_en           :    [0x0, 0x1],         bits : 5
    reg_lce_en             :    [0x0, 0x1],         bits : 6
    reg_cst_en             :    [0x0, 0x1],         bits : 7
    reg_ctrl_en            :    [0x0, 0x1],         bits : 9
    reg_hadj_en            :    [0x0, 0x1],         bits : 10
    reg_cadj_en            :    [0x0, 0x1],         bits : 11
    reg_cadj_yenh_en       :    [0x0, 0x1],         bits : 12
    reg_cadj_ycon_en       :    [0x0, 0x1],         bits : 13
    reg_cadj_ccon_en       :    [0x0, 0x1],         bits : 14
    reg_cadj_ycth_en       :    [0x0, 0x1],         bits : 15
    reg_cstprotect_en      :    [0x0, 0x1],         bits : 16
    reg_edge_dbg_en        :    [0x0, 0x1],         bits : 17
    reg_vacc_en             :    [0x0, 0x1],         bits : 19
    reg_win0_va_en          :    [0x0, 0x1],         bits : 20
    reg_win1_va_en          :    [0x0, 0x1],         bits : 21
    reg_win2_va_en          :    [0x0, 0x1],         bits : 22
    reg_win3_va_en          :    [0x0, 0x1],         bits : 23
    reg_win4_va_en          :    [0x0, 0x1],         bits : 24
    reg_pfr_en             :    [0x0, 0x1],         bits : 25
*/
#define IPE_MODE_REGISTER_1_OFS 0x0008
REGDEF_BEGIN(IPE_MODE_REGISTER_1)
REGDEF_BIT(reg_rgblpf_en,           1)
REGDEF_BIT(reg_rgbgamma_en,         1)
REGDEF_BIT(reg_ycurve_en,           1)
REGDEF_BIT(reg_cr_en,               1)
REGDEF_BIT(reg_defog_subimg_out_en, 1)
REGDEF_BIT(reg_defog_en,            1)
REGDEF_BIT(reg_lce_en,              1)
REGDEF_BIT(reg_cst_en,              1)
REGDEF_BIT(,                        1)
REGDEF_BIT(reg_ctrl_en,             1)
REGDEF_BIT(reg_hadj_en,             1)
REGDEF_BIT(reg_cadj_en,             1)
REGDEF_BIT(reg_cadj_yenh_en,        1)
REGDEF_BIT(reg_cadj_ycon_en,        1)
REGDEF_BIT(reg_cadj_ccon_en,        1)
REGDEF_BIT(reg_cadj_ycth_en,        1)
REGDEF_BIT(reg_cstprotect_en,       1)
REGDEF_BIT(reg_edge_dbg_en,         1)
REGDEF_BIT(,                        1)
REGDEF_BIT(reg_vacc_en,             1)
REGDEF_BIT(reg_win0_va_en,          1)
REGDEF_BIT(reg_win1_va_en,          1)
REGDEF_BIT(reg_win2_va_en,          1)
REGDEF_BIT(reg_win3_va_en,          1)
REGDEF_BIT(reg_win4_va_en,          1)
REGDEF_BIT(reg_pfr_en,              1)
REGDEF_END(IPE_MODE_REGISTER_1)

//=============================================================================
// IPE YCURVE
//=============================================================================
/*
    dram_sai_ycurve:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_4_OFS 0x0028
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_4)
REGDEF_BIT(,                            2)
REGDEF_BIT(dram_sai_ycurve,        30)
REGDEF_END(DMA_TO_IPE_REGISTER_4)

//=============================================================================
// IPE GAMMA
//=============================================================================
/*
    reg_dram_sai_gamma:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_5_OFS 0x002c
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_5)
REGDEF_BIT(,                           2)
REGDEF_BIT(reg_dram_sai_gamma,        30)
REGDEF_END(DMA_TO_IPE_REGISTER_5)

//=============================================================================
// IPE CCM
//=============================================================================
/*
    reg_ccrange   :    [0x0, 0x3],          bits : 1_0
    reg_cc2_sel   :    [0x0, 0x1],          bits : 2
    reg_cc_gamsel :    [0x0, 0x1],          bits : 3
    reg_ccstab_sel:    [0x0, 0x3],          bits : 5_4
    reg_ccofs_sel :    [0x0, 0x3],          bits : 7_6
    reg_coef_rr   :    [0x0, 0xfff],            bits : 27_16
*/
#define CCM_0_OFS 0x0100
REGDEF_BEGIN(CCM_0)
REGDEF_BIT(reg_ccrange,          2)
REGDEF_BIT(reg_cc2_sel,          1)
REGDEF_BIT(reg_cc_gamsel,        1)
REGDEF_BIT(reg_ccstab_sel,       2)
REGDEF_BIT(reg_ccofs_sel,        2)
REGDEF_BIT(,                     8)
REGDEF_BIT(reg_coef_rr,          12)
REGDEF_END(CCM_0)

/*
    reg_coef_rg:    [0x0, 0xfff],           bits : 11_0
    reg_coef_rb:    [0x0, 0xfff],           bits : 27_16
*/
#define CCM_1_OFS 0x0104
REGDEF_BEGIN(CCM_1)
REGDEF_BIT(reg_coef_rg,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_coef_rb,        12)
REGDEF_END(CCM_1)

/*
    reg_coef_gr:    [0x0, 0xfff],           bits : 11_0
    reg_coef_gg:    [0x0, 0xfff],           bits : 27_16
*/
#define CCM_2_OFS 0x0108
REGDEF_BEGIN(CCM_2)
REGDEF_BIT(reg_coef_gr,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_coef_gg,        12)
REGDEF_END(CCM_2)

/*
    reg_coef_gb:    [0x0, 0xfff],           bits : 11_0
    reg_coef_br:    [0x0, 0xfff],           bits : 27_16
*/
#define CCM_3_OFS 0x010c
REGDEF_BEGIN(CCM_3)
REGDEF_BIT(reg_coef_gb,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_coef_br,        12)
REGDEF_END(CCM_3)

/*
    reg_coef_bg:    [0x0, 0xfff],           bits : 11_0
    reg_coef_bb:    [0x0, 0xfff],           bits : 27_16
*/
#define CCM_4_OFS 0x0110
REGDEF_BEGIN(CCM_4)
REGDEF_BIT(reg_coef_bg,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_coef_bb,        12)
REGDEF_END(CCM_4)

//=============================================================================
// IPE COLOR
//=============================================================================
/*
    reg_c_cbofs  :    [0x0, 0xff],          bits : 7_0
    reg_c_crofs  :    [0x0, 0xff],          bits : 15_8
    reg_c_con    :    [0x0, 0xff],          bits : 23_16
    reg_c_rand_en:    [0x0, 0x1],           bits : 27
    reg_c_rand   :    [0x0, 0x7],           bits : 30_28
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS 0x01cc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5)
REGDEF_BIT(reg_c_cbofs,        8)
REGDEF_BIT(reg_c_crofs,        8)
REGDEF_BIT(reg_c_con,          8)
REGDEF_BIT(,                   3)
REGDEF_BIT(reg_c_rand_en,      1)
REGDEF_BIT(reg_c_rand,         3)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5)

/*
    reg_fstab0:    [0x0, 0xff],         bits : 7_0
    reg_fstab1:    [0x0, 0xff],         bits : 15_8
    reg_fstab2:    [0x0, 0xff],         bits : 23_16
    reg_fstab3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_0_OFS 0x0114
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_0)
REGDEF_BIT(reg_fstab0,        8)
REGDEF_BIT(reg_fstab1,        8)
REGDEF_BIT(reg_fstab2,        8)
REGDEF_BIT(reg_fstab3,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_0)

/*
    reg_fstab4:    [0x0, 0xff],         bits : 7_0
    reg_fstab5:    [0x0, 0xff],         bits : 15_8
    reg_fstab6:    [0x0, 0xff],         bits : 23_16
    reg_fstab7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_1_OFS 0x0118
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_1)
REGDEF_BIT(reg_fstab4,        8)
REGDEF_BIT(reg_fstab5,        8)
REGDEF_BIT(reg_fstab6,        8)
REGDEF_BIT(reg_fstab7,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_1)

/*
    reg_fstab8 :    [0x0, 0xff],            bits : 7_0
    reg_fstab9 :    [0x0, 0xff],            bits : 15_8
    reg_fstab10:    [0x0, 0xff],            bits : 23_16
    reg_fstab11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_2_OFS 0x011c
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_2)
REGDEF_BIT(reg_fstab8,        8)
REGDEF_BIT(reg_fstab9,        8)
REGDEF_BIT(reg_fstab10,        8)
REGDEF_BIT(reg_fstab11,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_2)

/*
    reg_fstab12:    [0x0, 0xff],            bits : 7_0
    reg_fstab13:    [0x0, 0xff],            bits : 15_8
    reg_fstab14:    [0x0, 0xff],            bits : 23_16
    reg_fstab15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_3_OFS 0x0120
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_3)
REGDEF_BIT(reg_fstab12,        8)
REGDEF_BIT(reg_fstab13,        8)
REGDEF_BIT(reg_fstab14,        8)
REGDEF_BIT(reg_fstab15,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_3)

/*
    reg_fdtab0:    [0x0, 0xff],         bits : 7_0
    reg_fdtab1:    [0x0, 0xff],         bits : 15_8
    reg_fdtab2:    [0x0, 0xff],         bits : 23_16
    reg_fdtab3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0_OFS 0x0124
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0)
REGDEF_BIT(reg_fdtab0,        8)
REGDEF_BIT(reg_fdtab1,        8)
REGDEF_BIT(reg_fdtab2,        8)
REGDEF_BIT(reg_fdtab3,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0)

/*
    reg_fdtab4:    [0x0, 0xff],         bits : 7_0
    reg_fdtab5:    [0x0, 0xff],         bits : 15_8
    reg_fdtab6:    [0x0, 0xff],         bits : 23_16
    reg_fdtab7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1_OFS 0x0128
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1)
REGDEF_BIT(reg_fdtab4,        8)
REGDEF_BIT(reg_fdtab5,        8)
REGDEF_BIT(reg_fdtab6,        8)
REGDEF_BIT(reg_fdtab7,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1)

/*
    reg_fdtab8 :    [0x0, 0xff],            bits : 7_0
    reg_fdtab9 :    [0x0, 0xff],            bits : 15_8
    reg_fdtab10:    [0x0, 0xff],            bits : 23_16
    reg_fdtab11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2_OFS 0x012c
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2)
REGDEF_BIT(reg_fdtab8,        8)
REGDEF_BIT(reg_fdtab9,        8)
REGDEF_BIT(reg_fdtab10,        8)
REGDEF_BIT(reg_fdtab11,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2)

/*
    reg_fdtab12:    [0x0, 0xff],            bits : 7_0
    reg_fdtab13:    [0x0, 0xff],            bits : 15_8
    reg_fdtab14:    [0x0, 0xff],            bits : 23_16
    reg_fdtab15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3_OFS 0x0130
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3)
REGDEF_BIT(reg_fdtab12,        8)
REGDEF_BIT(reg_fdtab13,        8)
REGDEF_BIT(reg_fdtab14,        8)
REGDEF_BIT(reg_fdtab15,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3)

/*
    reg_int_ofs   :    [0x0, 0xff],         bits : 7_0
    reg_sat_ofs   :    [0x0, 0xff],         bits : 15_8
    reg_chue_roten:    [0x0, 0x1],          bits : 16
    reg_chue_c2gen:    [0x0, 0x1],          bits : 17
    reg_cctrl_sel :    [0x0, 0x3],          bits : 21_20
    reg_vdet_div  :    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_REGISTER_OFS 0x0148
REGDEF_BEGIN(SAT_CONTROL_REGISTER)
REGDEF_BIT(reg_int_ofs,        8)
REGDEF_BIT(reg_sat_ofs,        8)
REGDEF_BIT(reg_chue_roten,     1)
REGDEF_BIT(reg_chue_c2gen,     1)
REGDEF_BIT(,                   2)
REGDEF_BIT(reg_cctrl_sel,      2)
REGDEF_BIT(,                   2)
REGDEF_BIT(reg_vdet_div,       8)
REGDEF_END(SAT_CONTROL_REGISTER)

/*
    reg_chuem0:    [0x0, 0xff],         bits : 7_0
    reg_chuem1:    [0x0, 0xff],         bits : 15_8
    reg_chuem2:    [0x0, 0xff],         bits : 23_16
    reg_chuem3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_0_OFS 0x014c
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_0)
REGDEF_BIT(reg_chuem0,        8)
REGDEF_BIT(reg_chuem1,        8)
REGDEF_BIT(reg_chuem2,        8)
REGDEF_BIT(reg_chuem3,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_0)


/*
    reg_chuem4:    [0x0, 0xff],         bits : 7_0
    reg_chuem5:    [0x0, 0xff],         bits : 15_8
    reg_chuem6:    [0x0, 0xff],         bits : 23_16
    reg_chuem7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_1_OFS 0x0150
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_1)
REGDEF_BIT(reg_chuem4,        8)
REGDEF_BIT(reg_chuem5,        8)
REGDEF_BIT(reg_chuem6,        8)
REGDEF_BIT(reg_chuem7,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_1)


/*
    reg_chuem8 :    [0x0, 0xff],            bits : 7_0
    reg_chuem9 :    [0x0, 0xff],            bits : 15_8
    reg_chuem10:    [0x0, 0xff],            bits : 23_16
    reg_chuem11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_2_OFS 0x0154
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_2)
REGDEF_BIT(reg_chuem8,        8)
REGDEF_BIT(reg_chuem9,        8)
REGDEF_BIT(reg_chuem10,        8)
REGDEF_BIT(reg_chuem11,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_2)


/*
    reg_chuem12:    [0x0, 0xff],            bits : 7_0
    reg_chuem13:    [0x0, 0xff],            bits : 15_8
    reg_chuem14:    [0x0, 0xff],            bits : 23_16
    reg_chuem15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_3_OFS 0x0158
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_3)
REGDEF_BIT(reg_chuem12,        8)
REGDEF_BIT(reg_chuem13,        8)
REGDEF_BIT(reg_chuem14,        8)
REGDEF_BIT(reg_chuem15,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_3)


/*
    reg_chuem16:    [0x0, 0xff],            bits : 7_0
    reg_chuem17:    [0x0, 0xff],            bits : 15_8
    reg_chuem18:    [0x0, 0xff],            bits : 23_16
    reg_chuem19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_4_OFS 0x015c
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_4)
REGDEF_BIT(reg_chuem16,        8)
REGDEF_BIT(reg_chuem17,        8)
REGDEF_BIT(reg_chuem18,        8)
REGDEF_BIT(reg_chuem19,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_4)


/*
    reg_chuem20:    [0x0, 0xff],            bits : 7_0
    reg_chuem21:    [0x0, 0xff],            bits : 15_8
    reg_chuem22:    [0x0, 0xff],            bits : 23_16
    reg_chuem23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_5_OFS 0x0160
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_5)
REGDEF_BIT(reg_chuem20,        8)
REGDEF_BIT(reg_chuem21,        8)
REGDEF_BIT(reg_chuem22,        8)
REGDEF_BIT(reg_chuem23,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_5)

/*
    reg_csatm0:    [0x0, 0xff],         bits : 7_0
    reg_csatm1:    [0x0, 0xff],         bits : 15_8
    reg_csatm2:    [0x0, 0xff],         bits : 23_16
    reg_csatm3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0_OFS 0x017c
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0)
REGDEF_BIT(reg_csatm0,        8)
REGDEF_BIT(reg_csatm1,        8)
REGDEF_BIT(reg_csatm2,        8)
REGDEF_BIT(reg_csatm3,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0)


/*
    reg_csatm4:    [0x0, 0xff],         bits : 7_0
    reg_csatm5:    [0x0, 0xff],         bits : 15_8
    reg_csatm6:    [0x0, 0xff],         bits : 23_16
    reg_csatm7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1_OFS 0x0180
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1)
REGDEF_BIT(reg_csatm4,        8)
REGDEF_BIT(reg_csatm5,        8)
REGDEF_BIT(reg_csatm6,        8)
REGDEF_BIT(reg_csatm7,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1)


/*
    reg_csatm8 :    [0x0, 0xff],            bits : 7_0
    reg_csatm9 :    [0x0, 0xff],            bits : 15_8
    reg_csatm10:    [0x0, 0xff],            bits : 23_16
    reg_csatm11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2_OFS 0x0184
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2)
REGDEF_BIT(reg_csatm8,        8)
REGDEF_BIT(reg_csatm9,        8)
REGDEF_BIT(reg_csatm10,        8)
REGDEF_BIT(reg_csatm11,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2)


/*
    reg_csatm12:    [0x0, 0xff],            bits : 7_0
    reg_csatm13:    [0x0, 0xff],            bits : 15_8
    reg_csatm14:    [0x0, 0xff],            bits : 23_16
    reg_csatm15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3_OFS 0x0188
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3)
REGDEF_BIT(reg_csatm12,        8)
REGDEF_BIT(reg_csatm13,        8)
REGDEF_BIT(reg_csatm14,        8)
REGDEF_BIT(reg_csatm15,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3)


/*
    reg_csatm16:    [0x0, 0xff],            bits : 7_0
    reg_csatm17:    [0x0, 0xff],            bits : 15_8
    reg_csatm18:    [0x0, 0xff],            bits : 23_16
    reg_csatm19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4_OFS 0x018c
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4)
REGDEF_BIT(reg_csatm16,        8)
REGDEF_BIT(reg_csatm17,        8)
REGDEF_BIT(reg_csatm18,        8)
REGDEF_BIT(reg_csatm19,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4)


/*
    reg_csatm20:    [0x0, 0xff],            bits : 7_0
    reg_csatm21:    [0x0, 0xff],            bits : 15_8
    reg_csatm22:    [0x0, 0xff],            bits : 23_16
    reg_csatm23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5_OFS 0x0190
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5)
REGDEF_BIT(reg_csatm20,        8)
REGDEF_BIT(reg_csatm21,        8)
REGDEF_BIT(reg_csatm22,        8)
REGDEF_BIT(reg_csatm23,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5)

/*
    reg_ccontab0:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab1:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab2:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_CCONTAB_REGISTER_0_OFS 0x01dc
REGDEF_BEGIN(COLOR_CCONTAB_REGISTER_0)
REGDEF_BIT(reg_ccontab0,        10)
REGDEF_BIT(reg_ccontab1,        10)
REGDEF_BIT(reg_ccontab2,        10)
REGDEF_END(COLOR_CCONTAB_REGISTER_0)

/*
    reg_ccontab3:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab4:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab5:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_CCONTAB_REGISTER_1_OFS 0x01e0
REGDEF_BEGIN(COLOR_CCONTAB_REGISTER_1)
REGDEF_BIT(reg_ccontab3,        10)
REGDEF_BIT(reg_ccontab4,        10)
REGDEF_BIT(reg_ccontab5,        10)
REGDEF_END(COLOR_CCONTAB_REGISTER_1)

/*
    reg_ccontab6:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab7:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab8:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_CCONTAB_REGISTER_2_OFS 0x01e4
REGDEF_BEGIN(COLOR_CCONTAB_REGISTER_2)
REGDEF_BIT(reg_ccontab6,        10)
REGDEF_BIT(reg_ccontab7,        10)
REGDEF_BIT(reg_ccontab8,        10)
REGDEF_END(COLOR_CCONTAB_REGISTER_2)

/*
    reg_ccontab9 :    [0x0, 0x3ff],         bits : 9_0
    reg_ccontab10:    [0x0, 0x3ff],         bits : 19_10
    reg_ccontab11:    [0x0, 0x3ff],         bits : 29_20
*/
#define COLOR_CCONTAB_REGISTER_3_OFS 0x01e8
REGDEF_BEGIN(COLOR_CCONTAB_REGISTER_3)
REGDEF_BIT(reg_ccontab9,        10)
REGDEF_BIT(reg_ccontab10,        10)
REGDEF_BIT(reg_ccontab11,        10)
REGDEF_END(COLOR_CCONTAB_REGISTER_3)

/*
    reg_ccontab12:    [0x0, 0x3ff],         bits : 9_0
    reg_ccontab13:    [0x0, 0x3ff],         bits : 19_10
    reg_ccontab14:    [0x0, 0x3ff],         bits : 29_20
*/
#define COLOR_CCONTAB_REGISTER_4_OFS 0x01ec
REGDEF_BEGIN(COLOR_CCONTAB_REGISTER_4)
REGDEF_BIT(reg_ccontab12,        10)
REGDEF_BIT(reg_ccontab13,        10)
REGDEF_BIT(reg_ccontab14,        10)
REGDEF_END(COLOR_CCONTAB_REGISTER_4)

/*
    reg_ccontab15  :    [0x0, 0x3ff],           bits : 9_0
    reg_ccontab16  :    [0x0, 0x3ff],           bits : 19_10
    reg_ccontab_sel:    [0x0, 0x1],         bits : 31
*/
#define COLOR_CCONTAB_REGISTER_5_OFS 0x01f0
REGDEF_BEGIN(COLOR_CCONTAB_REGISTER_5)
REGDEF_BIT(reg_ccontab15,        10)
REGDEF_BIT(reg_ccontab16,        10)
REGDEF_BIT(,        11)
REGDEF_BIT(reg_ccontab_sel,        1)
REGDEF_END(COLOR_CCONTAB_REGISTER_5)

/*
    reg_ccontab0:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab1:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab2:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS 0x01dc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9)
REGDEF_BIT(reg_ccontab0,        10)
REGDEF_BIT(reg_ccontab1,        10)
REGDEF_BIT(reg_ccontab2,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9)

/*
    reg_ccontab3:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab4:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab5:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_10_OFS 0x01e0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10)
REGDEF_BIT(reg_ccontab3,        10)
REGDEF_BIT(reg_ccontab4,        10)
REGDEF_BIT(reg_ccontab5,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10)

/*
    reg_ccontab6:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab7:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab8:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_11_OFS 0x01e4
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11)
REGDEF_BIT(reg_ccontab6,        10)
REGDEF_BIT(reg_ccontab7,        10)
REGDEF_BIT(reg_ccontab8,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11)

/*
    reg_ccontab9 :    [0x0, 0x3ff],         bits : 9_0
    reg_ccontab10:    [0x0, 0x3ff],         bits : 19_10
    reg_ccontab11:    [0x0, 0x3ff],         bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_12_OFS 0x01e8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12)
REGDEF_BIT(reg_ccontab9,        10)
REGDEF_BIT(reg_ccontab10,        10)
REGDEF_BIT(reg_ccontab11,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12)

/*
    reg_ccontab12:    [0x0, 0x3ff],         bits : 9_0
    reg_ccontab13:    [0x0, 0x3ff],         bits : 19_10
    reg_ccontab14:    [0x0, 0x3ff],         bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_13_OFS 0x01ec
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13)
REGDEF_BIT(reg_ccontab12,        10)
REGDEF_BIT(reg_ccontab13,        10)
REGDEF_BIT(reg_ccontab14,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13)

/*
    reg_ccontab15  :    [0x0, 0x3ff],           bits : 9_0
    reg_ccontab16  :    [0x0, 0x3ff],           bits : 19_10
    reg_ccontab_sel:    [0x0, 0x1],         bits : 31
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS 0x01f0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14)
REGDEF_BIT(reg_ccontab15,        10)
REGDEF_BIT(reg_ccontab16,        10)
REGDEF_BIT(,        11)
REGDEF_BIT(reg_ccontab_sel,        1)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14)

//=============================================================================
// IPE CONTRAST
//=============================================================================
/*
    reg_y_con       :    [0x0, 0xff],           bits : 7_0
    reg_yc_randreset:    [0x0, 0x1],            bits : 8
    reg_y_rand_en   :    [0x0, 0x1],            bits : 9
    reg_y_rand      :    [0x0, 0x7],            bits : 14_12
    reg_y_ethy      :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS 0x01bc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1)
REGDEF_BIT(reg_y_con,            8)
REGDEF_BIT(reg_yc_randreset,     1)
REGDEF_BIT(reg_y_rand_en,        1)
REGDEF_BIT(,                     2)
REGDEF_BIT(reg_y_rand,           3)
REGDEF_BIT(,                     1)
REGDEF_BIT(reg_y_ethy,          10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1)

/*
    reg_lce_wt_diff_pos:    [0x0, 0xff],            bits : 7_0
    reg_lce_wt_diff_neg:    [0x0, 0xff],            bits : 15_8
    reg_lce_wt_diff_avg:    [0x0, 0xff],            bits : 23_16
*/
#define LCE_REGISTER_0_OFS 0x0384
REGDEF_BEGIN(LCE_REGISTER_0)
REGDEF_BIT(reg_lce_wt_diff_pos,        8)
REGDEF_BIT(reg_lce_wt_diff_neg,        8)
REGDEF_BIT(reg_lce_wt_diff_avg,        8)
REGDEF_END(LCE_REGISTER_0)

/*
    reg_lce_lum_adj_lut0:    [0x0, 0xff],           bits : 7_0
    reg_lce_lum_adj_lut1:    [0x0, 0xff],           bits : 15_8
    reg_lce_lum_adj_lut2:    [0x0, 0xff],           bits : 23_16
    reg_lce_lum_adj_lut3:    [0x0, 0xff],           bits : 31_24
*/
#define LCE_REGISTER_1_OFS 0x0388
REGDEF_BEGIN(LCE_REGISTER_1)
REGDEF_BIT(reg_lce_lum_adj_lut0,        8)
REGDEF_BIT(reg_lce_lum_adj_lut1,        8)
REGDEF_BIT(reg_lce_lum_adj_lut2,        8)
REGDEF_BIT(reg_lce_lum_adj_lut3,        8)
REGDEF_END(LCE_REGISTER_1)

/*
    reg_lce_lum_adj_lut4:    [0x0, 0xff],           bits : 7_0
    reg_lce_lum_adj_lut5:    [0x0, 0xff],           bits : 15_8
    reg_lce_lum_adj_lut6:    [0x0, 0xff],           bits : 23_16
    reg_lce_lum_adj_lut7:    [0x0, 0xff],           bits : 31_24
*/
#define LCE_REGISTER_2_OFS 0x038c
REGDEF_BEGIN(LCE_REGISTER_2)
REGDEF_BIT(reg_lce_lum_adj_lut4,        8)
REGDEF_BIT(reg_lce_lum_adj_lut5,        8)
REGDEF_BIT(reg_lce_lum_adj_lut6,        8)
REGDEF_BIT(reg_lce_lum_adj_lut7,        8)
REGDEF_END(LCE_REGISTER_2)

/*
    reg_lce_lum_adj_lut8:    [0x0, 0xff],           bits : 7_0
*/
#define LCE_REGISTER_3_OFS 0x0390
REGDEF_BEGIN(LCE_REGISTER_3)
REGDEF_BIT(reg_lce_lum_adj_lut8,        8)
REGDEF_END(LCE_REGISTER_3)

/*
    reg_int_ofs   :    [0x0, 0xff],         bits : 7_0
    reg_sat_ofs   :    [0x0, 0xff],         bits : 15_8
    reg_chue_roten:    [0x0, 0x1],          bits : 16
    reg_chue_c2gen:    [0x0, 0x1],          bits : 17
    reg_cctrl_sel :    [0x0, 0x3],          bits : 21_20
    reg_vdet_div  :    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_REGISTER_OFS 0x0148
REGDEF_BEGIN(COLOR_CONTROL_REGISTER)
REGDEF_BIT(reg_int_ofs,        8)
REGDEF_BIT(reg_sat_ofs,        8)
REGDEF_BIT(reg_chue_roten,     1)
REGDEF_BIT(reg_chue_c2gen,     1)
REGDEF_BIT(,                   2)
REGDEF_BIT(reg_cctrl_sel,      2)
REGDEF_BIT(,                   2)
REGDEF_BIT(reg_vdet_div,       8)
REGDEF_END(COLOR_CONTROL_REGISTER)

/*
    reg_defog_subimg_lofsi:    [0x0, 0x3fff],           bits : 15_2
*/
#define DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS 0x02c8
REGDEF_BEGIN(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER)
REGDEF_BIT(,                               2)
REGDEF_BIT(reg_defog_subimg_lofsi,        14)
REGDEF_END(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER)

//=============================================================================
// IPE EDGE
//=============================================================================
#define IPE_EN_REGISTER_OFS 0x0008
REGDEF_BEGIN(IPE_EN_REGISTER)
REGDEF_BIT(reg_rgblpf_en,           1)
REGDEF_BIT(reg_rgbgamma_en,         1)
REGDEF_BIT(reg_ycurve_en,           1)
REGDEF_BIT(reg_cr_en,               1)
REGDEF_BIT(reg_defog_subimg_out_en, 1)
REGDEF_BIT(reg_defog_en,            1)
REGDEF_BIT(reg_lce_en,              1)
REGDEF_BIT(reg_cst_en,              1)
REGDEF_BIT(,                        1)
REGDEF_BIT(reg_ctrl_en,             1)
REGDEF_BIT(reg_hadj_en,             1)
REGDEF_BIT(reg_cadj_en,             1)
REGDEF_BIT(reg_cadj_yenh_en,        1)
REGDEF_BIT(reg_cadj_ycon_en,        1)
REGDEF_BIT(reg_cadj_ccon_en,        1)
REGDEF_BIT(reg_cadj_ycth_en,        1)
REGDEF_BIT(reg_cstprotect_en,       1)
REGDEF_BIT(reg_edge_dbg_en,         1)
REGDEF_BIT(,                        1)
REGDEF_BIT(reg_vacc_en,             1)
REGDEF_BIT(reg_win0_va_en,          1)
REGDEF_BIT(reg_win1_va_en,          1)
REGDEF_BIT(reg_win2_va_en,          1)
REGDEF_BIT(reg_win3_va_en,          1)
REGDEF_BIT(reg_win4_va_en,          1)
REGDEF_BIT(reg_pfr_en,              1)
REGDEF_END(IPE_EN_REGISTER)

/*
    reg_th_overshoot     :    [0x0, 0xff],          bits : 7_0
    reg_th_undershoot    :    [0x0, 0xff],          bits : 15_8
    reg_th_undershoot_lum:    [0x0, 0xff],          bits : 23_16
    reg_th_undershoot_eng:    [0x0, 0xff],          bits : 31_24
*/
#define OVERSHOOTING_REGISTER_OFS 0x00a4
REGDEF_BEGIN(OVERSHOOTING_REGISTER)
REGDEF_BIT(reg_th_overshoot,             8)
REGDEF_BIT(reg_th_undershoot,            8)
REGDEF_BIT(reg_th_undershoot_lum,        8)
REGDEF_BIT(reg_th_undershoot_eng,        8)
REGDEF_END(OVERSHOOTING_REGISTER)

/*
    reg_eslutl_0:    [0x0, 0xff],            bits : 7_0
    reg_eslutl_1:    [0x0, 0xff],            bits : 15_8
    reg_eslutl_2:    [0x0, 0xff],            bits : 23_16
    reg_eslutl_3:    [0x0, 0xff],            bits : 31_24
*/
#define ES_MAP_LUT_0_OFS 0x00b8
REGDEF_BEGIN(ES_MAP_LUT_0)
REGDEF_BIT(reg_eslutl_0,        8)
REGDEF_BIT(reg_eslutl_1,        8)
REGDEF_BIT(reg_eslutl_2,        8)
REGDEF_BIT(reg_eslutl_3,        8)
REGDEF_END(ES_MAP_LUT_0)

/*
    reg_eslutl_4:    [0x0, 0xff],            bits : 7_0
    reg_eslutl_5:    [0x0, 0xff],            bits : 15_8
    reg_eslutl_6:    [0x0, 0xff],            bits : 23_16
    reg_eslutl_7:    [0x0, 0xff],            bits : 31_24
*/
#define ES_MAP_LUT_1_OFS 0x00bc
REGDEF_BEGIN(ES_MAP_LUT_1)
REGDEF_BIT(reg_eslutl_4,        8)
REGDEF_BIT(reg_eslutl_5,        8)
REGDEF_BIT(reg_eslutl_6,        8)
REGDEF_BIT(reg_eslutl_7,        8)
REGDEF_END(ES_MAP_LUT_1)

/*
    reg_esluth_0:    [0x0, 0xff],            bits : 7_0
    reg_esluth_1:    [0x0, 0xff],            bits : 15_8
    reg_esluth_2:    [0x0, 0xff],            bits : 23_16
    reg_esluth_3:    [0x0, 0xff],            bits : 31_24
*/
#define ES_MAP_LUT_2_OFS 0x00c0
REGDEF_BEGIN(ES_MAP_LUT_2)
REGDEF_BIT(reg_esluth_0,        8)
REGDEF_BIT(reg_esluth_1,        8)
REGDEF_BIT(reg_esluth_2,        8)
REGDEF_BIT(reg_esluth_3,        8)
REGDEF_END(ES_MAP_LUT_2)

/*
    reg_esluth_4:    [0x0, 0xff],            bits : 7_0
    reg_esluth_5:    [0x0, 0xff],            bits : 15_8
    reg_esluth_6:    [0x0, 0xff],            bits : 23_16
    reg_esluth_7:    [0x0, 0xff],            bits : 31_24
*/
#define ES_MAP_LUT_3_OFS 0x00c4
REGDEF_BEGIN(ES_MAP_LUT_3)
REGDEF_BIT(reg_esluth_4,        8)
REGDEF_BIT(reg_esluth_5,        8)
REGDEF_BIT(reg_esluth_6,        8)
REGDEF_BIT(reg_esluth_7,        8)
REGDEF_END(ES_MAP_LUT_3)

/*
    reg_edlutl_0:    [0x0, 0xff],            bits : 7_0
    reg_edlutl_1:    [0x0, 0xff],            bits : 15_8
    reg_edlutl_2:    [0x0, 0xff],            bits : 23_16
    reg_edlutl_3:    [0x0, 0xff],            bits : 31_24
*/
#define ED_MAP_LUT_0_OFS 0x00cc
REGDEF_BEGIN(ED_MAP_LUT_0)
REGDEF_BIT(reg_edlutl_0,        8)
REGDEF_BIT(reg_edlutl_1,        8)
REGDEF_BIT(reg_edlutl_2,        8)
REGDEF_BIT(reg_edlutl_3,        8)
REGDEF_END(ED_MAP_LUT_0)

/*
    reg_edlutl_4:    [0x0, 0xff],            bits : 7_0
    reg_edlutl_5:    [0x0, 0xff],            bits : 15_8
    reg_edlutl_6:    [0x0, 0xff],            bits : 23_16
    reg_edlutl_7:    [0x0, 0xff],            bits : 31_24
*/
#define ED_MAP_LUT_1_OFS 0x00d0
REGDEF_BEGIN(ED_MAP_LUT_1)
REGDEF_BIT(reg_edlutl_4,        8)
REGDEF_BIT(reg_edlutl_5,        8)
REGDEF_BIT(reg_edlutl_6,        8)
REGDEF_BIT(reg_edlutl_7,        8)
REGDEF_END(ED_MAP_LUT_1)

/*
    reg_edluth_0:    [0x0, 0xff],            bits : 7_0
    reg_edluth_1:    [0x0, 0xff],            bits : 15_8
    reg_edluth_2:    [0x0, 0xff],            bits : 23_16
    reg_edluth_3:    [0x0, 0xff],            bits : 31_24
*/
#define ED_MAP_LUT_2_OFS 0x00d4
REGDEF_BEGIN(ED_MAP_LUT_2)
REGDEF_BIT(reg_edluth_0,        8)
REGDEF_BIT(reg_edluth_1,        8)
REGDEF_BIT(reg_edluth_2,        8)
REGDEF_BIT(reg_edluth_3,        8)
REGDEF_END(ED_MAP_LUT_2)

/*
    reg_edluth_4:    [0x0, 0xff],            bits : 7_0
    reg_edluth_5:    [0x0, 0xff],            bits : 15_8
    reg_edluth_6:    [0x0, 0xff],            bits : 23_16
    reg_edluth_7:    [0x0, 0xff],            bits : 31_24
*/
#define ED_MAP_LUT_3_OFS 0x00d8
REGDEF_BEGIN(ED_MAP_LUT_3)
REGDEF_BIT(reg_edluth_4,        8)
REGDEF_BIT(reg_edluth_5,        8)
REGDEF_BIT(reg_edluth_6,        8)
REGDEF_BIT(reg_edluth_7,        8)
REGDEF_END(ED_MAP_LUT_3)

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
    reg_cedgem0:    [0x0, 0xff],         bits : 7_0
    reg_cedgem1:    [0x0, 0xff],         bits : 15_8
    reg_cedgem2:    [0x0, 0xff],         bits : 23_16
    reg_cedgem3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_EDGE_LUT_0_OFS 0x0194
REGDEF_BEGIN(COLOR_EDGE_LUT_0)
REGDEF_BIT(reg_cedgem0,        8)
REGDEF_BIT(reg_cedgem1,        8)
REGDEF_BIT(reg_cedgem2,        8)
REGDEF_BIT(reg_cedgem3,        8)
REGDEF_END(COLOR_EDGE_LUT_0)

/*
    reg_cedgem4:    [0x0, 0xff],         bits : 7_0
    reg_cedgem5:    [0x0, 0xff],         bits : 15_8
    reg_cedgem6:    [0x0, 0xff],         bits : 23_16
    reg_cedgem7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_EDGE_LUT_1_OFS 0x0198
REGDEF_BEGIN(COLOR_EDGE_LUT_1)
REGDEF_BIT(reg_cedgem4,        8)
REGDEF_BIT(reg_cedgem5,        8)
REGDEF_BIT(reg_cedgem6,        8)
REGDEF_BIT(reg_cedgem7,        8)
REGDEF_END(COLOR_EDGE_LUT_1)

/*
    reg_cedgem8 :    [0x0, 0xff],            bits : 7_0
    reg_cedgem9 :    [0x0, 0xff],            bits : 15_8
    reg_cedgem10:    [0x0, 0xff],            bits : 23_16
    reg_cedgem11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_EDGE_LUT_2_OFS 0x019c
REGDEF_BEGIN(COLOR_EDGE_LUT_2)
REGDEF_BIT(reg_cedgem8,        8)
REGDEF_BIT(reg_cedgem9,        8)
REGDEF_BIT(reg_cedgem10,        8)
REGDEF_BIT(reg_cedgem11,        8)
REGDEF_END(COLOR_EDGE_LUT_2)

/*
    reg_cedgem12:    [0x0, 0xff],            bits : 7_0
    reg_cedgem13:    [0x0, 0xff],            bits : 15_8
    reg_cedgem14:    [0x0, 0xff],            bits : 23_16
    reg_cedgem15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_EDGE_LUT_3_OFS 0x01a0
REGDEF_BEGIN(COLOR_EDGE_LUT_3)
REGDEF_BIT(reg_cedgem12,        8)
REGDEF_BIT(reg_cedgem13,        8)
REGDEF_BIT(reg_cedgem14,        8)
REGDEF_BIT(reg_cedgem15,        8)
REGDEF_END(COLOR_EDGE_LUT_3)

/*
    reg_cedgem16:    [0x0, 0xff],            bits : 7_0
    reg_cedgem17:    [0x0, 0xff],            bits : 15_8
    reg_cedgem18:    [0x0, 0xff],            bits : 23_16
    reg_cedgem19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_EDGE_LUT_4_OFS 0x01a4
REGDEF_BEGIN(COLOR_EDGE_LUT_4)
REGDEF_BIT(reg_cedgem16,        8)
REGDEF_BIT(reg_cedgem17,        8)
REGDEF_BIT(reg_cedgem18,        8)
REGDEF_BIT(reg_cedgem19,        8)
REGDEF_END(COLOR_EDGE_LUT_4)

/*
    reg_cedgem20:    [0x0, 0xff],            bits : 7_0
    reg_cedgem21:    [0x0, 0xff],            bits : 15_8
    reg_cedgem22:    [0x0, 0xff],            bits : 23_16
    reg_cedgem23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_EDGE_LUT_5_OFS 0x01a8
REGDEF_BEGIN(COLOR_EDGE_LUT_5)
REGDEF_BIT(reg_cedgem20,        8)
REGDEF_BIT(reg_cedgem21,        8)
REGDEF_BIT(reg_cedgem22,        8)
REGDEF_BIT(reg_cedgem23,        8)
REGDEF_END(COLOR_EDGE_LUT_5)

/*
    reg_y_enh_p :    [0x0, 0x3ff],          bits : 9_0
    reg_y_enh_n :    [0x0, 0x3ff],          bits : 19_10
    reg_y_einv_p:    [0x0, 0x1],            bits : 30
    reg_y_einv_n:    [0x0, 0x1],            bits : 31
*/
#define EDGE_ENH_ADJUSTMENT_0_OFS 0x01b8
REGDEF_BEGIN(EDGE_ENH_ADJUSTMENT_0)
REGDEF_BIT(reg_y_enh_p,        10)
REGDEF_BIT(reg_y_enh_n,        10)
REGDEF_BIT(,                   10)
REGDEF_BIT(reg_y_einv_p,        1)
REGDEF_BIT(reg_y_einv_n,        1)
REGDEF_END(EDGE_ENH_ADJUSTMENT_0)

/*
    reg_w_con_eng:    [0x0, 0xf],           bits : 3_0
    reg_w_low    :    [0x0, 0x1f],          bits : 8_4
    reg_w_high   :    [0x0, 0x1f],          bits : 16_12
*/
#define EDGE_REGION_EXTRACTION_0_OFS 0x0088
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_0)
REGDEF_BIT(reg_w_con_eng,        4)
REGDEF_BIT(reg_w_low,            5)
REGDEF_BIT(,                     3)
REGDEF_BIT(reg_w_high,           5)
REGDEF_END(EDGE_REGION_EXTRACTION_0)

/*
    reg_w_ker_thin    :    [0x0, 0xf],          bits : 3_0
    reg_w_ker_robust  :    [0x0, 0xf],          bits : 7_4
    reg_iso_ker_thin  :    [0x0, 0xf],          bits : 11_8
    reg_iso_ker_robust:    [0x0, 0xf],          bits : 15_12
*/
#define EDGE_REGION_EXTRACTION_REGISTER_1_OFS 0x008c
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_1)
REGDEF_BIT(reg_w_ker_thin,          4)
REGDEF_BIT(reg_w_ker_robust,        4)
REGDEF_BIT(reg_iso_ker_thin,        4)
REGDEF_BIT(reg_iso_ker_robust,      4)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_1)

/*
    reg_w_hld_low         :    [0x0, 0x1f],         bits : 4_0
    reg_w_hld_high        :    [0x0, 0x1f],         bits : 12_8
    reg_w_ker_thin_hld    :    [0x0, 0xf],          bits : 19_16
    reg_w_ker_robust_hld  :    [0x0, 0xf],          bits : 23_20
    reg_iso_ker_thin_hld  :    [0x0, 0xf],          bits : 27_24
    reg_iso_ker_robust_hld:    [0x0, 0xf],          bits : 31_28
*/
#define EDGE_REGION_EXTRACTION_1_OFS 0x0090
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_1)
REGDEF_BIT(reg_w_hld_low,            5)
REGDEF_BIT(,                         3)
REGDEF_BIT(reg_w_hld_high,           5)
REGDEF_BIT(,                         3)
REGDEF_BIT(reg_w_ker_thin_hld,       4)
REGDEF_BIT(reg_w_ker_robust_hld,     4)
REGDEF_BIT(reg_iso_ker_thin_hld,     4)
REGDEF_BIT(reg_iso_ker_robust_hld,   4)
REGDEF_END(EDGE_REGION_EXTRACTION_1)

/*
    reg_th_flat:    [0x0, 0x3ff],           bits : 9_0
    reg_th_edge:    [0x0, 0x3ff],           bits : 21_12
*/
#define EDGE_REGION_EXTRACTION_2_OFS 0x0094
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_2)
REGDEF_BIT(reg_th_flat,        10)
REGDEF_BIT(,                    2)
REGDEF_BIT(reg_th_edge,        10)
REGDEF_END(EDGE_REGION_EXTRACTION_2)

/*
    reg_th_flat_hld:    [0x0, 0x3ff],           bits : 9_0
    reg_th_edge_hld:    [0x0, 0x3ff],           bits : 21_12
    reg_th_lum_hld :    [0x0, 0x3ff],           bits : 31_22
*/
#define EDGE_REGION_EXTRACTION_3_OFS 0x0098
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_3)
REGDEF_BIT(reg_th_flat_hld,        10)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_th_edge_hld,        10)
REGDEF_BIT(reg_th_lum_hld,         10)
REGDEF_END(EDGE_REGION_EXTRACTION_3)

/*
    reg_slope_con_eng    :    [0x0, 0xffff],            bits : 15_0
    reg_slope_hld_con_eng:    [0x0, 0xffff],            bits : 31_16
*/
#define EDGE_REGION_EXTRACTION_REGISTER_5_OFS 0x009c
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_5)
REGDEF_BIT(reg_slope_con_eng,        16)
REGDEF_BIT(reg_slope_hld_con_eng,        16)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_5)


/*
    reg_slope_overshoot :    [0x0, 0x7fff],         bits : 14_0
    reg_slope_undershoot:    [0x0, 0x7fff],         bits : 30_16
*/
#define OVERSHOOTING_SLOPE_OFS 0x00ac
REGDEF_BEGIN(OVERSHOOTING_SLOPE_0)
REGDEF_BIT(reg_slope_overshoot,         15)
REGDEF_BIT(,                             1)
REGDEF_BIT(reg_slope_undershoot,        15)
REGDEF_END(OVERSHOOTING_SLOPE_0)

/*
    reg_esthrl:    [0x0, 0x3ff],            bits : 9_0
    reg_esthrh:    [0x0, 0x3ff],            bits : 19_10
    reg_establ:    [0x0, 0x7],          bits : 22_20
    reg_estabh:    [0x0, 0x7],          bits : 26_24
*/
#define ES_TABLE_0_OFS 0x00b4
REGDEF_BEGIN(ES_TABLE_0)
REGDEF_BIT(reg_esthrl,        10)
REGDEF_BIT(reg_esthrh,        10)
REGDEF_BIT(reg_establ,         3)
REGDEF_BIT(,                   1)
REGDEF_BIT(reg_estabh,         3)
REGDEF_END(ES_TABLE_0)

/*
    reg_edthrl :    [0x0, 0x3ff],           bits : 9_0
    reg_edthrh :    [0x0, 0x3ff],           bits : 19_10
    reg_edtabl :    [0x0, 0x7],         bits : 22_20
    reg_edtabh :    [0x0, 0x7],         bits : 26_24
    reg_edin_sel:    [0x0, 0x3],         bits : 29_28
*/
#define ED_TABLE_0_OFS 0x00c8
REGDEF_BEGIN(ED_TABLE_0)
REGDEF_BIT(reg_edthrl,        10)
REGDEF_BIT(reg_edthrh,        10)
REGDEF_BIT(reg_edtabl,        3)
REGDEF_BIT(,                  1)
REGDEF_BIT(reg_edtabh,        3)
REGDEF_BIT(,                  1)
REGDEF_BIT(reg_edin_sel,      2)
REGDEF_END(ED_TABLE_0)

/*
    reg_edge_lut_0:    [0x0, 0x3ff],         bits : 9_0
    reg_edge_lut_1:    [0x0, 0x3ff],         bits : 19_10
    reg_edge_lut_2:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA0_OFS 0x0800
REGDEF_BEGIN(EDGE_GAMMA0)
REGDEF_BIT(reg_edge_lut_0,        10)
REGDEF_BIT(reg_edge_lut_1,        10)
REGDEF_BIT(reg_edge_lut_2,        10)
REGDEF_END(EDGE_GAMMA0)


/*
    reg_edge_lut_3:    [0x0, 0x3ff],         bits : 9_0
    reg_edge_lut_4:    [0x0, 0x3ff],         bits : 19_10
    reg_edge_lut_5:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA1_OFS 0x0804
REGDEF_BEGIN(EDGE_GAMMA1)
REGDEF_BIT(reg_edge_lut_3,        10)
REGDEF_BIT(reg_edge_lut_4,        10)
REGDEF_BIT(reg_edge_lut_5,        10)
REGDEF_END(EDGE_GAMMA1)


/*
    reg_edge_lut_6:    [0x0, 0x3ff],         bits : 9_0
    reg_edge_lut_7:    [0x0, 0x3ff],         bits : 19_10
    reg_edge_lut_8:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA2_OFS 0x0808
REGDEF_BEGIN(EDGE_GAMMA2)
REGDEF_BIT(reg_edge_lut_6,        10)
REGDEF_BIT(reg_edge_lut_7,        10)
REGDEF_BIT(reg_edge_lut_8,        10)
REGDEF_END(EDGE_GAMMA2)


/*
    reg_edge_lut_9 :    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_10:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_11:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA3_OFS 0x080c
REGDEF_BEGIN(EDGE_GAMMA3)
REGDEF_BIT(reg_edge_lut_9,        10)
REGDEF_BIT(reg_edge_lut_10,        10)
REGDEF_BIT(reg_edge_lut_11,        10)
REGDEF_END(EDGE_GAMMA3)


/*
    reg_edge_lut_12:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_13:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_14:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA4_OFS 0x0810
REGDEF_BEGIN(EDGE_GAMMA4)
REGDEF_BIT(reg_edge_lut_12,        10)
REGDEF_BIT(reg_edge_lut_13,        10)
REGDEF_BIT(reg_edge_lut_14,        10)
REGDEF_END(EDGE_GAMMA4)


/*
    reg_edge_lut_15:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_16:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_17:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA5_OFS 0x0814
REGDEF_BEGIN(EDGE_GAMMA5)
REGDEF_BIT(reg_edge_lut_15,        10)
REGDEF_BIT(reg_edge_lut_16,        10)
REGDEF_BIT(reg_edge_lut_17,        10)
REGDEF_END(EDGE_GAMMA5)


/*
    reg_edge_lut_18:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_19:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_20:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA6_OFS 0x0818
REGDEF_BEGIN(EDGE_GAMMA6)
REGDEF_BIT(reg_edge_lut_18,        10)
REGDEF_BIT(reg_edge_lut_19,        10)
REGDEF_BIT(reg_edge_lut_20,        10)
REGDEF_END(EDGE_GAMMA6)


/*
    reg_edge_lut_21:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_22:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_23:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA7_OFS 0x081c
REGDEF_BEGIN(EDGE_GAMMA7)
REGDEF_BIT(reg_edge_lut_21,        10)
REGDEF_BIT(reg_edge_lut_22,        10)
REGDEF_BIT(reg_edge_lut_23,        10)
REGDEF_END(EDGE_GAMMA7)


/*
    reg_edge_lut_24:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_25:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_26:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA8_OFS 0x0820
REGDEF_BEGIN(EDGE_GAMMA8)
REGDEF_BIT(reg_edge_lut_24,        10)
REGDEF_BIT(reg_edge_lut_25,        10)
REGDEF_BIT(reg_edge_lut_26,        10)
REGDEF_END(EDGE_GAMMA8)


/*
    reg_edge_lut_27:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_28:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_29:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA9_OFS 0x0824
REGDEF_BEGIN(EDGE_GAMMA9)
REGDEF_BIT(reg_edge_lut_27,        10)
REGDEF_BIT(reg_edge_lut_28,        10)
REGDEF_BIT(reg_edge_lut_29,        10)
REGDEF_END(EDGE_GAMMA9)


/*
    reg_edge_lut_30:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_31:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_32:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA10_OFS 0x0828
REGDEF_BEGIN(EDGE_GAMMA10)
REGDEF_BIT(reg_edge_lut_30,        10)
REGDEF_BIT(reg_edge_lut_31,        10)
REGDEF_BIT(reg_edge_lut_32,        10)
REGDEF_END(EDGE_GAMMA10)


/*
    reg_edge_lut_33:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_34:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_35:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA11_OFS 0x082c
REGDEF_BEGIN(EDGE_GAMMA11)
REGDEF_BIT(reg_edge_lut_33,        10)
REGDEF_BIT(reg_edge_lut_34,        10)
REGDEF_BIT(reg_edge_lut_35,        10)
REGDEF_END(EDGE_GAMMA11)


/*
    reg_edge_lut_36:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_37:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_38:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA12_OFS 0x0830
REGDEF_BEGIN(EDGE_GAMMA12)
REGDEF_BIT(reg_edge_lut_36,        10)
REGDEF_BIT(reg_edge_lut_37,        10)
REGDEF_BIT(reg_edge_lut_38,        10)
REGDEF_END(EDGE_GAMMA12)


/*
    reg_edge_lut_39:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_40:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_41:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA13_OFS 0x0834
REGDEF_BEGIN(EDGE_GAMMA13)
REGDEF_BIT(reg_edge_lut_39,        10)
REGDEF_BIT(reg_edge_lut_40,        10)
REGDEF_BIT(reg_edge_lut_41,        10)
REGDEF_END(EDGE_GAMMA13)


/*
    reg_edge_lut_42:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_43:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_44:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA14_OFS 0x0838
REGDEF_BEGIN(EDGE_GAMMA14)
REGDEF_BIT(reg_edge_lut_42,        10)
REGDEF_BIT(reg_edge_lut_43,        10)
REGDEF_BIT(reg_edge_lut_44,        10)
REGDEF_END(EDGE_GAMMA14)


/*
    reg_edge_lut_45:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_46:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_47:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA15_OFS 0x083c
REGDEF_BEGIN(EDGE_GAMMA15)
REGDEF_BIT(reg_edge_lut_45,        10)
REGDEF_BIT(reg_edge_lut_46,        10)
REGDEF_BIT(reg_edge_lut_47,        10)
REGDEF_END(EDGE_GAMMA15)


/*
    reg_edge_lut_48:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_49:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_50:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA16_OFS 0x0840
REGDEF_BEGIN(EDGE_GAMMA16)
REGDEF_BIT(reg_edge_lut_48,        10)
REGDEF_BIT(reg_edge_lut_49,        10)
REGDEF_BIT(reg_edge_lut_50,        10)
REGDEF_END(EDGE_GAMMA16)


/*
    reg_edge_lut_51:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_52:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_53:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA17_OFS 0x0844
REGDEF_BEGIN(EDGE_GAMMA17)
REGDEF_BIT(reg_edge_lut_51,        10)
REGDEF_BIT(reg_edge_lut_52,        10)
REGDEF_BIT(reg_edge_lut_53,        10)
REGDEF_END(EDGE_GAMMA17)


/*
    reg_edge_lut_54:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_55:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_56:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA18_OFS 0x0848
REGDEF_BEGIN(EDGE_GAMMA18)
REGDEF_BIT(reg_edge_lut_54,        10)
REGDEF_BIT(reg_edge_lut_55,        10)
REGDEF_BIT(reg_edge_lut_56,        10)
REGDEF_END(EDGE_GAMMA18)


/*
    reg_edge_lut_57:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_58:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_59:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA19_OFS 0x084c
REGDEF_BEGIN(EDGE_GAMMA19)
REGDEF_BIT(reg_edge_lut_57,        10)
REGDEF_BIT(reg_edge_lut_58,        10)
REGDEF_BIT(reg_edge_lut_59,        10)
REGDEF_END(EDGE_GAMMA19)


/*
    reg_edge_lut_60:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_61:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_62:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA20_OFS 0x0850
REGDEF_BEGIN(EDGE_GAMMA20)
REGDEF_BIT(reg_edge_lut_60,        10)
REGDEF_BIT(reg_edge_lut_61,        10)
REGDEF_BIT(reg_edge_lut_62,        10)
REGDEF_END(EDGE_GAMMA20)


/*
    reg_edge_lut_63:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_64:    [0x0, 0x3ff],            bits : 19_10
*/
#define EDGE_GAMMA21_OFS 0x0854
REGDEF_BEGIN(EDGE_GAMMA21)
REGDEF_BIT(reg_edge_lut_63,        10)
REGDEF_BIT(reg_edge_lut_64,        10)
REGDEF_END(EDGE_GAMMA21)

// NOTE: IME
//=============================================================================
// IME TMNR2
//=============================================================================
/*
    ime_src                         :    [0x0, 0x1],            bits : 0
    ime_dir_ctrl                       :    [0x0, 0x1],         bits : 1
    ime_p1_en                         :    [0x0, 0x1],          bits : 2
    ime_p2_en                         :    [0x0, 0x1],          bits : 3
    ime_p3_en                         :    [0x0, 0x1],          bits : 4
    ime_chra_en                     :    [0x0, 0x1],            bits : 10
    ime_chra_ca_en                   :    [0x0, 0x1],           bits : 11
    ime_chra_la_en                   :    [0x0, 0x1],           bits : 12
    ime_dbcs_en                     :    [0x0, 0x1],            bits : 13
    ime_ds_cst_en                     :    [0x0, 0x1],          bits : 16
    ime_ds_en0                       :    [0x0, 0x1],           bits : 17
    ime_ds_en1                       :    [0x0, 0x1],           bits : 18
    ime_ds_en2                       :    [0x0, 0x1],           bits : 19
    ime_ds_en3                       :    [0x0, 0x1],           bits : 20
    ime_ds_plt_sel         :    [0x0, 0x3],         bits : 22_21
    ime_chra_subout_en     :    [0x0, 0x1],         bits : 25
    ime_p1_enc_en          :    [0x0, 0x1],         bits : 26
    ime_3dnr_en            :    [0x0, 0x1],         bits : 27
    ime_tmnr_ref_in_dec_en :    [0x0, 0x1],         bits : 29
    ime_tmnr_ref_out_en    :    [0x0, 0x1],         bits : 30
    ime_tmnr_ref_out_enc_en:    [0x0, 0x1],         bits : 31
*/
#define IME_FUNCTION_CONTROL_REGISTER0_OFS 0x0004
REGDEF_BEGIN(IME_FUNCTION_CONTROL_REGISTER0)
REGDEF_BIT(ime_src,        1)
REGDEF_BIT(ime_dir_ctrl,        1)
REGDEF_BIT(ime_p1_en,        1)
REGDEF_BIT(ime_p2_en,        1)
REGDEF_BIT(ime_p3_en,        1)
//REGDEF_BIT(ime_p4_en,        1)
REGDEF_BIT(,        5)
REGDEF_BIT(ime_chra_en,        1)
REGDEF_BIT(ime_chra_ca_en,        1)
REGDEF_BIT(ime_chra_la_en,        1)
REGDEF_BIT(ime_dbcs_en,        1)
//REGDEF_BIT(ime_stl_en,        1)
//REGDEF_BIT(ime_stl_ftr_en,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds_cst_en,        1)
REGDEF_BIT(ime_ds_en0,        1)
REGDEF_BIT(ime_ds_en1,        1)
REGDEF_BIT(ime_ds_en2,        1)
REGDEF_BIT(ime_ds_en3,        1)
REGDEF_BIT(ime_ds_plt_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_chra_subout_en,        1)
REGDEF_BIT(ime_p1_enc_en,        1)
REGDEF_BIT(ime_3dnr_en,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_tmnr_ref_in_dec_en,        1)
REGDEF_BIT(ime_tmnr_ref_out_en,        1)
REGDEF_BIT(ime_tmnr_ref_out_enc_en,        1)
REGDEF_END(IME_FUNCTION_CONTROL_REGISTER0)

/*
    chra_fmt                      :    [0x0, 0x1],          bits : 0
    chra_in_bypass              :    [0x0, 0x1],            bits : 1
    chra_src                      :    [0x0, 0x1],          bits : 2
    chra_bypass                :    [0x0, 0x1],         bits : 3
*/
#define IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS 0x0230
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0)
REGDEF_BIT(chra_fmt,        1)
REGDEF_BIT(chra_in_bypass,        1)
REGDEF_BIT(chra_src,        1)
REGDEF_BIT(chra_bypass,        1)
REGDEF_END(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0)

/*
    chra_refy_wt                :    [0x0, 0x1f],           bits : 4_0
    chra_refc_wt                :    [0x0, 0x1f],           bits : 9_5
    chra_out_wt              :    [0x0, 0x1f],          bits : 14_10
    luma_refy_wt                :    [0x0, 0x1f],           bits : 19_15
    luma_out_wt              :    [0x0, 0x1f],          bits : 24_20
*/
#define IME_CHROMA_ADAPTATION_REGISTER0_OFS 0x023c
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_REGISTER0)
REGDEF_BIT(chra_refy_wt,        5)
REGDEF_BIT(chra_refc_wt,        5)
REGDEF_BIT(chra_out_wt,        5)
REGDEF_BIT(luma_refy_wt,        5)
REGDEF_BIT(luma_out_wt,        5)
REGDEF_END(IME_CHROMA_ADAPTATION_REGISTER0)

/*
    dbcs_ctr_u      :    [0x0, 0xff],           bits : 7_0
    dbcs_ctr_v                :    [0x0, 0xff],         bits : 15_8
    dbcs_mode                  :    [0x0, 0x3],         bits : 17_16
    dbcs_step_y              :    [0x0, 0x3],           bits : 21_20
    dbcs_step_uv                :    [0x0, 0x3],            bits : 23_22
*/
#define IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS 0x0280
REGDEF_BEGIN(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0)
REGDEF_BIT(dbcs_ctr_u,        8)
REGDEF_BIT(dbcs_ctr_v,        8)
REGDEF_BIT(dbcs_mode,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(dbcs_step_y,        2)
REGDEF_BIT(dbcs_step_uv,        2)
REGDEF_END(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0)

/*
    ime_3dnr_pre_y_blur_str      :    [0x0, 0x3],           bits : 1_0
    ime_3dnr_pf_type             :    [0x0, 0x3],           bits : 3_2
    ime_3dnr_me_update_mode      :    [0x0, 0x1],           bits : 4
    ime_3dnr_me_boundary_set     :    [0x0, 0x1],           bits : 5
    ime_3dnr_me_mv_ds_mode       :    [0x0, 0x3],           bits : 7_6
    ime_3dnr_ps_smart_roi_ctrl   :    [0x0, 0x1],           bits : 8
    ime_3dnr_nr_center_wzero_y_3d:    [0x0, 0x1],           bits : 9
    ime_3dnr_ps_mv_check_en      :    [0x0, 0x1],           bits : 10
    ime_3dnr_ps_mv_check_roi_en  :    [0x0, 0x1],           bits : 11
    ime_3dnr_ps_mv_info_mode     :    [0x0, 0x3],           bits : 13_12
    ime_3dnr_ps_mode             :    [0x0, 0x1],           bits : 14
    ime_3dnr_me_sad_type         :    [0x0, 0x1],           bits : 15
    ime_3dnr_me_sad_shift        :    [0x0, 0xf],           bits : 19_16
    ime_3dnr_nr_y_ch_en          :    [0x0, 0x1],           bits : 20
    ime_3dnr_nr_c_ch_en          :    [0x0, 0x1],           bits : 21
    ime_3dnr_nr_c_fsv_en         :    [0x0, 0x1],           bits : 22
    ime_3dnr_seed_reset_en       :    [0x0, 0x1],           bits : 23
    ime_3dnr_nr_c_fsv            :    [0x0, 0xff],          bits : 31_24
*/
#define IME_TMNR_CONTROL_REGISTER0_OFS 0x0600
REGDEF_BEGIN(IME_TMNR_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_pre_y_blur_str,        2)
REGDEF_BIT(ime_3dnr_pf_type,        2)
REGDEF_BIT(ime_3dnr_me_update_mode,        1)
REGDEF_BIT(ime_3dnr_me_boundary_set,        1)
REGDEF_BIT(ime_3dnr_me_mv_ds_mode,        2)
REGDEF_BIT(ime_3dnr_ps_smart_roi_ctrl,        1)
REGDEF_BIT(ime_3dnr_nr_center_wzero_y_3d,        1)
REGDEF_BIT(ime_3dnr_ps_mv_check_en,        1)
REGDEF_BIT(ime_3dnr_ps_mv_check_roi_en,        1)
REGDEF_BIT(ime_3dnr_ps_mv_info_mode,        2)
REGDEF_BIT(ime_3dnr_ps_mode,        1)
REGDEF_BIT(ime_3dnr_me_sad_type,        1)
REGDEF_BIT(ime_3dnr_me_sad_shift,        4)
REGDEF_BIT(ime_3dnr_nr_y_ch_en,        1)
REGDEF_BIT(ime_3dnr_nr_c_ch_en,        1)
REGDEF_BIT(ime_3dnr_nr_c_fsv_en,        1)
REGDEF_BIT(ime_3dnr_seed_reset_en,        1)
REGDEF_BIT(ime_3dnr_nr_c_fsv,        8)
REGDEF_END(IME_TMNR_CONTROL_REGISTER0)

/*
    ime_3dnr_ne_sample_step_x   :    [0x0, 0xff],           bits : 7_0
    ime_3dnr_ne_sample_step_y   :    [0x0, 0xff],           bits : 15_8
    ime_3dnr_statistic_output_en:    [0x0, 0x1],            bits : 16
    ime_3dnr_ps_fastc_en        :    [0x0, 0x1],            bits : 17
    ime_3dnr_dbg_mv0            :    [0x0, 0x1],            bits : 18
    ime_3dnr_dbg_mode           :    [0x0, 0xf],            bits : 31_28
*/
#define IME_TMNR_CONTROL_REGISTER1_OFS 0x0604
REGDEF_BEGIN(IME_TMNR_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_ne_sample_step_x,        8)
REGDEF_BIT(ime_3dnr_ne_sample_step_y,        8)
REGDEF_BIT(ime_3dnr_statistic_output_en,        1)
REGDEF_BIT(ime_3dnr_ps_fastc_en,        1)
REGDEF_BIT(ime_3dnr_dbg_mv0,        1)
REGDEF_BIT(,        9)
REGDEF_BIT(ime_3dnr_dbg_mode,        4)
REGDEF_END(IME_TMNR_CONTROL_REGISTER1)


/*
    ime_3dnr_me_sad_penalty_0:    [0x0, 0x3ff],         bits : 9_0
    ime_3dnr_me_sad_penalty_1:    [0x0, 0x3ff],         bits : 19_10
    ime_3dnr_me_sad_penalty_2:    [0x0, 0x3ff],         bits : 29_20
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0_OFS 0x0628
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_me_sad_penalty_0,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_1,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_2,        10)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0)

/*
    ime_3dnr_me_sad_penalty_3:    [0x0, 0x3ff],         bits : 9_0
    ime_3dnr_me_sad_penalty_4:    [0x0, 0x3ff],         bits : 19_10
    ime_3dnr_me_sad_penalty_5:    [0x0, 0x3ff],         bits : 29_20
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1_OFS 0x062c
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_me_sad_penalty_3,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_4,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_5,        10)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1)

/*
    ime_3dnr_me_sad_penalty_6:    [0x0, 0x3ff],         bits : 9_0
    ime_3dnr_me_sad_penalty_7:    [0x0, 0x3ff],         bits : 19_10
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2_OFS 0x0630
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2)
REGDEF_BIT(ime_3dnr_me_sad_penalty_6,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_7,        10)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2)

/*
    ime_3dnr_me_switch_th0:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_me_switch_th1:    [0x0, 0xff],         bits : 15_8
    ime_3dnr_me_switch_th2:    [0x0, 0xff],         bits : 23_16
    ime_3dnr_me_switch_th3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3_OFS 0x0634
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_me_switch_th0,        8)
REGDEF_BIT(ime_3dnr_me_switch_th1,        8)
REGDEF_BIT(ime_3dnr_me_switch_th2,        8)
REGDEF_BIT(ime_3dnr_me_switch_th3,        8)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3)

/*
    ime_3dnr_me_switch_th4:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_me_switch_th5:    [0x0, 0xff],         bits : 15_8
    ime_3dnr_me_switch_th6:    [0x0, 0xff],         bits : 23_16
    ime_3dnr_me_switch_th7:    [0x0, 0xff],         bits : 31_24
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER4_OFS 0x0638
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_me_switch_th4,        8)
REGDEF_BIT(ime_3dnr_me_switch_th5,        8)
REGDEF_BIT(ime_3dnr_me_switch_th6,        8)
REGDEF_BIT(ime_3dnr_me_switch_th7,        8)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER4)

/*
    ime_3dnr_me_switch_ratio:    [0x0, 0xff],           bits : 7_0
    ime_3dnr_me_cost_blend  :    [0x0, 0xf],            bits : 11_8
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5_OFS 0x063c
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5)
REGDEF_BIT(ime_3dnr_me_switch_ratio,        8)
REGDEF_BIT(ime_3dnr_me_cost_blend,        4)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5)

/*
    ime_3dnr_me_detail_penalty0:    [0x0, 0xf],         bits : 3_0
    ime_3dnr_me_detail_penalty1:    [0x0, 0xf],         bits : 7_4
    ime_3dnr_me_detail_penalty2:    [0x0, 0xf],         bits : 11_8
    ime_3dnr_me_detail_penalty3:    [0x0, 0xf],         bits : 15_12
    ime_3dnr_me_detail_penalty4:    [0x0, 0xf],         bits : 19_16
    ime_3dnr_me_detail_penalty5:    [0x0, 0xf],         bits : 23_20
    ime_3dnr_me_detail_penalty6:    [0x0, 0xf],         bits : 27_24
    ime_3dnr_me_detail_penalty7:    [0x0, 0xf],         bits : 31_28
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6_OFS 0x0640
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_me_detail_penalty0,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty1,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty2,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty3,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty4,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty5,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty6,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty7,        4)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6)

/*
    ime_3dnr_me_probability0:    [0x0, 0x1],            bits : 0
    ime_3dnr_me_probability1:    [0x0, 0x1],            bits : 1
    ime_3dnr_me_probability2:    [0x0, 0x1],            bits : 2
    ime_3dnr_me_probability3:    [0x0, 0x1],            bits : 3
    ime_3dnr_me_probability4:    [0x0, 0x1],            bits : 4
    ime_3dnr_me_probability5:    [0x0, 0x1],            bits : 5
    ime_3dnr_me_probability6:    [0x0, 0x1],            bits : 6
    ime_3dnr_me_probability7:    [0x0, 0x1],            bits : 7
    ime_3dnr_me_rand_bit_x  :    [0x0, 0x7],            bits : 10_8
    ime_3dnr_me_rand_bit_y  :    [0x0, 0x7],            bits : 14_12
    ime_3dnr_me_min_detail  :    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS 0x0644
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7)
REGDEF_BIT(ime_3dnr_me_probability0,        1)
REGDEF_BIT(ime_3dnr_me_probability1,        1)
REGDEF_BIT(ime_3dnr_me_probability2,        1)
REGDEF_BIT(ime_3dnr_me_probability3,        1)
REGDEF_BIT(ime_3dnr_me_probability4,        1)
REGDEF_BIT(ime_3dnr_me_probability5,        1)
REGDEF_BIT(ime_3dnr_me_probability6,        1)
REGDEF_BIT(ime_3dnr_me_probability7,        1)
REGDEF_BIT(ime_3dnr_me_rand_bit_x,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_me_rand_bit_y,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_me_min_detail,        14)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7)

/*
    ime_3dnr_fast_converge_sp  :    [0x0, 0xf],         bits : 3_0
    ime_3dnr_fast_converge_step:    [0x0, 0xf],         bits : 7_4
*/
#define IME_3DNR_FAST_CONVERGE_CONTROL_REGISTER0_OFS 0x0654
REGDEF_BEGIN(IME_3DNR_FAST_CONVERGE_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_fast_converge_sp,        4)
REGDEF_BIT(ime_3dnr_fast_converge_step,        4)
REGDEF_END(IME_3DNR_FAST_CONVERGE_CONTROL_REGISTER0)

/*
    ime_3dnr_md_sad_coef_a0:    [0x0, 0x3f],            bits : 5_0
    ime_3dnr_md_sad_coef_a1:    [0x0, 0x3f],            bits : 13_8
    ime_3dnr_md_sad_coef_a2:    [0x0, 0x3f],            bits : 21_16
    ime_3dnr_md_sad_coef_a3:    [0x0, 0x3f],            bits : 29_24
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0_OFS 0x0660
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_md_sad_coef_a0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a3,        6)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0)

/*
    ime_3dnr_md_sad_coef_a4:    [0x0, 0x3f],            bits : 5_0
    ime_3dnr_md_sad_coef_a5:    [0x0, 0x3f],            bits : 13_8
    ime_3dnr_md_sad_coef_a6:    [0x0, 0x3f],            bits : 21_16
    ime_3dnr_md_sad_coef_a7:    [0x0, 0x3f],            bits : 29_24
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER1_OFS 0x0664
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_md_sad_coef_a4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a7,        6)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER1)

/*
    ime_3dnr_md_sad_coef_b0:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_md_sad_coef_b1:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2_OFS 0x0668
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b1,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2)

/*
    ime_3dnr_md_sad_coef_b2:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_md_sad_coef_b3:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER3_OFS 0x066c
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_md_sad_coef_b2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b3,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER3)

/*
    ime_3dnr_md_sad_coef_b4:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_md_sad_coef_b5:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER4_OFS 0x0670
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_md_sad_coef_b4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b5,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER4)

/*
    ime_3dnr_md_sad_coef_b6:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_md_sad_coef_b7:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER5_OFS 0x0674
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER5)
REGDEF_BIT(ime_3dnr_md_sad_coef_b6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b7,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER5)

/*
    ime_3dnr_md_sad_std0:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_md_sad_std1:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6_OFS 0x0678
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_md_sad_std0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_std1,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6)

/*
    ime_3dnr_md_sad_std2:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_md_sad_std3:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER7_OFS 0x067c
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER7)
REGDEF_BIT(ime_3dnr_md_sad_std2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_std3,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER7)

/*
    ime_3dnr_md_sad_std4:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_md_sad_std5:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER8_OFS 0x0680
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER8)
REGDEF_BIT(ime_3dnr_md_sad_std4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_std5,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER8)

/*
    ime_3dnr_md_sad_std6:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_md_sad_std7:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER9_OFS 0x0684
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER9)
REGDEF_BIT(ime_3dnr_md_sad_std6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_std7,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER9)

/*
    ime_3dnr_md_k1:    [0x0, 0x3f],         bits : 5_0
    ime_3dnr_md_k2:    [0x0, 0x3f],         bits : 13_8
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10_OFS 0x0688
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10)
REGDEF_BIT(ime_3dnr_md_k1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_k2,        6)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10)

/*
    ime_3dnr_mc_sad_base0:    [0x0, 0x3fff],            bits : 13_0
    ime_3dnr_mc_sad_base1:    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS 0x06a0
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_mc_sad_base0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_base1,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0)

/*
    ime_3dnr_mc_sad_base2:    [0x0, 0x3fff],            bits : 13_0
    ime_3dnr_mc_sad_base3:    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER1_OFS 0x06a4
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_mc_sad_base2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_base3,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER1)

/*
    ime_3dnr_mc_sad_base4:    [0x0, 0x3fff],            bits : 13_0
    ime_3dnr_mc_sad_base5:    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER2_OFS 0x06a8
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER2)
REGDEF_BIT(ime_3dnr_mc_sad_base4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_base5,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER2)

/*
    ime_3dnr_mc_sad_base6:    [0x0, 0x3fff],            bits : 13_0
    ime_3dnr_mc_sad_base7:    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER3_OFS 0x06ac
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_mc_sad_base6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_base7,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER3)

/*
    ime_3dnr_mc_sad_coef_a0:    [0x0, 0x3f],            bits : 5_0
    ime_3dnr_mc_sad_coef_a1:    [0x0, 0x3f],            bits : 13_8
    ime_3dnr_mc_sad_coef_a2:    [0x0, 0x3f],            bits : 21_16
    ime_3dnr_mc_sad_coef_a3:    [0x0, 0x3f],            bits : 29_24
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4_OFS 0x06b0
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a3,        6)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4)

/*
    ime_3dnr_mc_sad_coef_a4:    [0x0, 0x3f],            bits : 5_0
    ime_3dnr_mc_sad_coef_a5:    [0x0, 0x3f],            bits : 13_8
    ime_3dnr_mc_sad_coef_a6:    [0x0, 0x3f],            bits : 21_16
    ime_3dnr_mc_sad_coef_a7:    [0x0, 0x3f],            bits : 29_24
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER5_OFS 0x06b4
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER5)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a7,        6)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER5)

/*
    ime_3dnr_mc_sad_coef_b0:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_mc_sad_coef_b1:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6_OFS 0x06b8
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b1,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6)

/*
    ime_3dnr_mc_sad_coef_b2:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_mc_sad_coef_b3:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER7_OFS 0x06bc
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER7)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b3,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER7)

/*
    ime_3dnr_mc_sad_coef_b4:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_mc_sad_coef_b5:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER8_OFS 0x06c0
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER8)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b5,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER8)

/*
    ime_3dnr_mc_sad_coef_b6:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_mc_sad_coef_b7:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER9_OFS 0x06c4
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER9)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b7,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER9)

/*
    ime_3dnr_mc_sad_std0:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_mc_sad_std1:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10_OFS 0x06c8
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10)
REGDEF_BIT(ime_3dnr_mc_sad_std0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_std1,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10)

/*
    ime_3dnr_mc_sad_std2:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_mc_sad_std3:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER11_OFS 0x06cc
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER11)
REGDEF_BIT(ime_3dnr_mc_sad_std2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_std3,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER11)

/*
    ime_3dnr_mc_sad_std4:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_mc_sad_std5:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER12_OFS 0x06d0
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER12)
REGDEF_BIT(ime_3dnr_mc_sad_std4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_std5,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER12)

/*
    ime_3dnr_mc_sad_std6:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_mc_sad_std7:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER13_OFS 0x06d4
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER13)
REGDEF_BIT(ime_3dnr_mc_sad_std6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_std7,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER13)

/*
    ime_3dnr_mc_k1:    [0x0, 0x3f],         bits : 5_0
    ime_3dnr_mc_k2:    [0x0, 0x3f],         bits : 13_8
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14_OFS 0x06d8
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14)
REGDEF_BIT(ime_3dnr_mc_k1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_k2,        6)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14)

/*
    ime_3dnr_nr_y_tf0_blur_str0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_y_tf0_blur_str1:    [0x0, 0xff],            bits : 15_8
    ime_3dnr_nr_y_tf0_blur_str2:    [0x0, 0xff],            bits : 23_16
    ime_3dnr_nr_y_tf0_str0     :    [0x0, 0xff],            bits : 31_24
*/
#define IME_3DNR_TF0_REGISTER0_OFS 0x0714
REGDEF_BEGIN(IME_3DNR_TF0_REGISTER0)
REGDEF_BIT(ime_3dnr_nr_y_tf0_blur_str0,        8)
REGDEF_BIT(ime_3dnr_nr_y_tf0_blur_str1,        8)
REGDEF_BIT(ime_3dnr_nr_y_tf0_blur_str2,        8)
REGDEF_BIT(ime_3dnr_nr_y_tf0_str0,        8)
REGDEF_END(IME_3DNR_TF0_REGISTER0)

/*
    ime_3dnr_nr_y_tf0_str1:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_nr_y_tf0_str2:    [0x0, 0xff],         bits : 15_8
    ime_3dnr_nr_c_tf0_str0:    [0x0, 0xff],         bits : 23_16
    ime_3dnr_nr_c_tf0_str1:    [0x0, 0xff],         bits : 31_24
*/
#define IME_3DNR_TF0_REGISTER1_OFS 0x0718
REGDEF_BEGIN(IME_3DNR_TF0_REGISTER1)
REGDEF_BIT(ime_3dnr_nr_y_tf0_str1,        8)
REGDEF_BIT(ime_3dnr_nr_y_tf0_str2,        8)
REGDEF_BIT(ime_3dnr_nr_c_tf0_str0,        8)
REGDEF_BIT(ime_3dnr_nr_c_tf0_str1,        8)
REGDEF_END(IME_3DNR_TF0_REGISTER1)

/*
    ime_3dnr_nr_c_tf0_str2:    [0x0, 0xff],         bits : 7_0
*/
#define IME_3DNR_TF0_REGISTER2_OFS 0x071c
REGDEF_BEGIN(IME_3DNR_TF0_REGISTER2)
REGDEF_BIT(ime_3dnr_nr_c_tf0_str2,        8)
REGDEF_END(IME_3DNR_TF0_REGISTER2)

/*
    ime_3dnr_ps_mv_th    :    [0x0, 0x3f],          bits : 5_0
    ime_3dnr_ps_roi_mv_th:    [0x0, 0x3f],          bits : 13_8
    ime_3dnr_ps_fs_th    :    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_PS_CONTROL_REGISTER0_OFS 0x0730
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_ps_mv_th,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ps_roi_mv_th,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ps_fs_th,        14)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER0)

/*
    ime_3dnr_ps_mix_ratio0:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_ps_mix_ratio1:    [0x0, 0xff],         bits : 15_8
*/
#define IME_3DNR_PS_CONTROL_REGISTER1_OFS 0x0734
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_ps_mix_ratio0,        8)
REGDEF_BIT(ime_3dnr_ps_mix_ratio1,        8)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER1)

/*
    ime_3dnr_ps_mix_slope0:    [0x0, 0xffff],           bits : 15_0
    ime_3dnr_ps_mix_slope1:    [0x0, 0xffff],           bits : 31_16
*/
#define IME_3DNR_PS_CONTROL_REGISTER3_OFS 0x073c
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_ps_mix_slope0,        16)
REGDEF_BIT(ime_3dnr_ps_mix_slope1,        16)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER3)

/*
    ime_3dnr_ps_ds_th    :    [0x0, 0x1f],          bits : 4_0
    ime_3dnr_ps_ds_th_roi:    [0x0, 0x1f],          bits : 12_8
    ime_3dnr_ps_edge_w   :    [0x0, 0xff],          bits : 23_16
*/
#define IME_3DNR_PS_CONTROL_REGISTER4_OFS 0x0760
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_ps_ds_th,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_3dnr_ps_ds_th_roi,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_3dnr_ps_edge_w,        8)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER4)

/*
    ime_3dnr_nr_residue_th_y0:    [0x0, 0xf],           bits : 3_0
    ime_3dnr_nr_residue_th_y1:    [0x0, 0xf],           bits : 11_8
    ime_3dnr_nr_residue_th_y2:    [0x0, 0xf],           bits : 19_16
    ime_3dnr_nr_residue_th_c :    [0x0, 0xf],           bits : 27_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER0_OFS 0x076c
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_nr_residue_th_y0,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_3dnr_nr_residue_th_y1,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_3dnr_nr_residue_th_y2,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_3dnr_nr_residue_th_c,        4)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER0)

/*
    ime_3dnr_nr_freq_w0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_freq_w1:    [0x0, 0xff],            bits : 15_8
    ime_3dnr_nr_freq_w2:    [0x0, 0xff],            bits : 23_16
    ime_3dnr_nr_freq_w3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER1_OFS 0x0770
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_nr_freq_w0,        8)
REGDEF_BIT(ime_3dnr_nr_freq_w1,        8)
REGDEF_BIT(ime_3dnr_nr_freq_w2,        8)
REGDEF_BIT(ime_3dnr_nr_freq_w3,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER1)

/*
    ime_3dnr_nr_luma_w0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_luma_w1:    [0x0, 0xff],            bits : 15_8
    ime_3dnr_nr_luma_w2:    [0x0, 0xff],            bits : 23_16
    ime_3dnr_nr_luma_w3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER3_OFS 0x0790
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_nr_luma_w0,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w1,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w2,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w3,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER3)

/*
    ime_3dnr_nr_luma_w4:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_luma_w5:    [0x0, 0xff],            bits : 15_8
    ime_3dnr_nr_luma_w6:    [0x0, 0xff],            bits : 23_16
    ime_3dnr_nr_luma_w7:    [0x0, 0xff],            bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER4_OFS 0x0794
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_nr_luma_w4,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w5,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w6,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w7,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER4)

/*
    ime_3dnr_nr_pre_filtering_str0:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_nr_pre_filtering_str1:    [0x0, 0xff],         bits : 15_8
    ime_3dnr_nr_pre_filtering_str2:    [0x0, 0xff],         bits : 23_16
    ime_3dnr_nr_pre_filtering_str3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER6_OFS 0x07b0
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_str0,        8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_str1,        8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_str2,        8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_str3,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER6)

/*
    ime_3dnr_nr_pre_filtering_ratio0:    [0x0, 0xff],           bits : 7_0
    ime_3dnr_nr_pre_filtering_ratio1:    [0x0, 0xff],           bits : 15_8
    ime_3dnr_nr_snr_str0            :    [0x0, 0xff],           bits : 23_16
    ime_3dnr_nr_snr_str1            :    [0x0, 0xff],           bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER8_OFS 0x07c0
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_ratio0,        8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_ratio1,        8)
REGDEF_BIT(ime_3dnr_nr_snr_str0,        8)
REGDEF_BIT(ime_3dnr_nr_snr_str1,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER8)

/*
    ime_3dnr_nr_snr_str2:    [0x0, 0xff],           bits : 7_0
    ime_3dnr_nr_tnr_str0:    [0x0, 0xff],           bits : 15_8
    ime_3dnr_nr_tnr_str1:    [0x0, 0xff],           bits : 23_16
    ime_3dnr_nr_tnr_str2:    [0x0, 0xff],           bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER9_OFS 0x07c4
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER9)
REGDEF_BIT(ime_3dnr_nr_snr_str2,        8)
REGDEF_BIT(ime_3dnr_nr_tnr_str0,        8)
REGDEF_BIT(ime_3dnr_nr_tnr_str1,        8)
REGDEF_BIT(ime_3dnr_nr_tnr_str2,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER9)

/*
    ime_3dnr_nr_base_th_snr:    [0x0, 0xffff],          bits : 15_0
    ime_3dnr_nr_base_th_tnr:    [0x0, 0xffff],          bits : 31_16
*/
#define IME_3DNR_NR_CONTROL_REGISTER10_OFS 0x07c8
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER10)
REGDEF_BIT(ime_3dnr_nr_base_th_snr,        16)
REGDEF_BIT(ime_3dnr_nr_base_th_tnr,        16)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER10)

/*
    ime_3dnr_nr_y_3d_ratio0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_y_3d_ratio1:    [0x0, 0xff],            bits : 15_8
*/
#define IME_3DNR_NR_CONTROL_REGISTER11_OFS 0x07cc
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER11)
REGDEF_BIT(ime_3dnr_nr_y_3d_ratio0,        8)
REGDEF_BIT(ime_3dnr_nr_y_3d_ratio1,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER11)

/*
    ime_3dnr_nr_y_3d_lut0:    [0x0, 0x7f],          bits : 6_0
    ime_3dnr_nr_y_3d_lut1:    [0x0, 0x7f],          bits : 14_8
    ime_3dnr_nr_y_3d_lut2:    [0x0, 0x7f],          bits : 22_16
    ime_3dnr_nr_y_3d_lut3:    [0x0, 0x7f],          bits : 30_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER12_OFS 0x07d0
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER12)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut0,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut1,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut2,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut3,        7)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER12)

/*
    ime_3dnr_nr_y_3d_lut4:    [0x0, 0x7f],          bits : 6_0
    ime_3dnr_nr_y_3d_lut5:    [0x0, 0x7f],          bits : 14_8
    ime_3dnr_nr_y_3d_lut6:    [0x0, 0x7f],          bits : 22_16
    ime_3dnr_nr_y_3d_lut7:    [0x0, 0x7f],          bits : 30_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER13_OFS 0x07d4
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER13)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut4,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut5,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut6,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut7,        7)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER13)

/*
    ime_3dnr_nr_c_3d_lut0:    [0x0, 0x7f],          bits : 6_0
    ime_3dnr_nr_c_3d_lut1:    [0x0, 0x7f],          bits : 14_8
    ime_3dnr_nr_c_3d_lut2:    [0x0, 0x7f],          bits : 22_16
    ime_3dnr_nr_c_3d_lut3:    [0x0, 0x7f],          bits : 30_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER14_OFS 0x07d8
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER14)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut0,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut1,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut2,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut3,        7)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER14)

/*
    ime_3dnr_nr_c_3d_lut4:    [0x0, 0x7f],          bits : 6_0
    ime_3dnr_nr_c_3d_lut5:    [0x0, 0x7f],          bits : 14_8
    ime_3dnr_nr_c_3d_lut6:    [0x0, 0x7f],          bits : 22_16
    ime_3dnr_nr_c_3d_lut7:    [0x0, 0x7f],          bits : 30_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER15_OFS 0x07dc
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER15)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut4,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut5,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut6,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut7,        7)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER15)

/*
    ime_3dnr_nr_c_3d_ratio0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_c_3d_ratio1:    [0x0, 0xff],            bits : 15_8
*/
#define IME_3DNR_NR_CONTROL_REGISTER16_OFS 0x07e0
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER16)
REGDEF_BIT(ime_3dnr_nr_c_3d_ratio0,        8)
REGDEF_BIT(ime_3dnr_nr_c_3d_ratio1,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER16)

/*
    ime_lcaf_y_rcth0:    [0x0, 0xff],           bits : 7_0
    ime_lcaf_y_rcth1:    [0x0, 0xff],           bits : 15_8
    ime_lcaf_y_rcth2:    [0x0, 0xff],           bits : 23_16
    ime_lcaf_y_cwt  :    [0x0, 0x1f],           bits : 28_24
*/
#define IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS 0x0a68
REGDEF_BEGIN(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0)
REGDEF_BIT(ime_lcaf_y_rcth0,        8)
REGDEF_BIT(ime_lcaf_y_rcth1,        8)
REGDEF_BIT(ime_lcaf_y_rcth2,        8)
REGDEF_BIT(ime_lcaf_y_cwt,        5)
REGDEF_END(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0)

/*
    ime_lcaf_uv_rcth0:    [0x0, 0xff],          bits : 7_0
    ime_lcaf_uv_rcth1:    [0x0, 0xff],          bits : 15_8
    ime_lcaf_uv_rcth2:    [0x0, 0xff],          bits : 23_16
    ime_lcaf_uv_cwt  :    [0x0, 0x1f],          bits : 28_24
*/
#define IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS 0x0a70
REGDEF_BEGIN(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2)
REGDEF_BIT(ime_lcaf_uv_rcth0,        8)
REGDEF_BIT(ime_lcaf_uv_rcth1,        8)
REGDEF_BIT(ime_lcaf_uv_rcth2,        8)
REGDEF_BIT(ime_lcaf_uv_cwt,        5)
REGDEF_END(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2)

/*
    ime_lcaf_y_fth0:    [0x0, 0xff],            bits : 7_0
    ime_lcaf_y_fth1:    [0x0, 0xff],            bits : 15_8
    ime_lcaf_y_fth2:    [0x0, 0xff],            bits : 23_16
    ime_lcaf_y_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER0_OFS 0x0a80
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER0)
REGDEF_BIT(ime_lcaf_y_fth0,        8)
REGDEF_BIT(ime_lcaf_y_fth1,        8)
REGDEF_BIT(ime_lcaf_y_fth2,        8)
REGDEF_BIT(ime_lcaf_y_fth3,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER0)

/*
    ime_lcaf_y_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER1_OFS 0x0a84
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER1)
REGDEF_BIT(ime_lcaf_y_fth4,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER1)

/*
    ime_lcaf_u_fth0:    [0x0, 0xff],            bits : 7_0
    ime_lcaf_u_fth1:    [0x0, 0xff],            bits : 15_8
    ime_lcaf_u_fth2:    [0x0, 0xff],            bits : 23_16
    ime_lcaf_u_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER0_OFS 0x0a8c
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER0)
REGDEF_BIT(ime_lcaf_u_fth0,        8)
REGDEF_BIT(ime_lcaf_u_fth1,        8)
REGDEF_BIT(ime_lcaf_u_fth2,        8)
REGDEF_BIT(ime_lcaf_u_fth3,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER0)

/*
    ime_lcaf_u_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER1_OFS 0x0a90
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER1)
REGDEF_BIT(ime_lcaf_u_fth4,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER1)

/*
    ime_lcaf_v_fth0:    [0x0, 0xff],            bits : 7_0
    ime_lcaf_v_fth1:    [0x0, 0xff],            bits : 15_8
    ime_lcaf_v_fth2:    [0x0, 0xff],            bits : 23_16
    ime_lcaf_v_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER0_OFS 0x0a98
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER0)
REGDEF_BIT(ime_lcaf_v_fth0,        8)
REGDEF_BIT(ime_lcaf_v_fth1,        8)
REGDEF_BIT(ime_lcaf_v_fth2,        8)
REGDEF_BIT(ime_lcaf_v_fth3,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER0)

/*
    ime_lcaf_v_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER1_OFS 0x0a9c
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER1)
REGDEF_BIT(ime_lcaf_v_fth4,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER1)

//post sharpen
typedef struct {
	BOOL  bEnable;          ///< [r/w] Enable sharpen
	UINT8 ucConEng;         ///< [r/w] The weight of the local contrast  0~8
	UINT8 ucSlopConEng;     ///< [r/w] Transition region slop 0~255
	UINT8 ucBHC;            ///< [r/w] Bright halo clip ratio 0~128
	UINT8 ucDHC;            ///< [r/w] Dark halo clip ratio 0~128
	UINT8 ucEWT;            ///< [r/w] Edge weight coring threshold 0~255
	UINT8 ucEWG;            ///< [r/w] Edge weight gain 0~255
	UINT8 ucEdgeSharpStr1;  ///< [r/w] Sharpen strength 1 of edge region 0~255
	UINT8 ucCT;             ///< [r/w] Coring threshold 0~255
	UINT8 ucNL;             ///< [r/w] Noise level 0~255
	UINT8 ucBIG;            ///< [r/w] Blending ratio of HPF results 0~128
	UINT16 usFlatTh;        ///< [r/w] flat region threshold 0~2047
	UINT16 usEdgeTh;        ///< [r/w] Edge region threshold 0~2047
	UINT8 ucLowWt;          ///< [r/w] Region classification weight threshold 1 0~16
	UINT8 ucHightWt;        ///< [r/w] Region classification weight threshold 2 0~16
	UINT16 usHLDFlatTh;     ///< [r/w] High light flat region threshold 0~2047
	UINT16 usHLDEdgeTh;     ///< [r/w] High light edge threshold 0~2047
	UINT8 ucHLDLowWt;       ///< [r/w] High light reigon classification weight threshold 1 0~16
	UINT8 ucHLDHightWt;     ///< [r/w] High light region classification weight threshold 2 0~16
	UINT8 ucEdgeStr;        ///< [r/w] Edge weight strength 0~255
	UINT8 ucTransitionStr;  ///< [r/w] Transition region weight strength 0~255
	UINT8 ucMotionStr;      ///< [r/w] Motion region edge weight 0~255
	UINT8 ucStaticStr;      ///< [r/w] Static region edge weight strength 0~255
	UINT8 ucHLDSlopConEng;  ///< [r/w] High light transition region slop 0~255
	UINT8 ucFlatStr;        ///< [r/w] Flag region weight strength 0~16
	UINT16 usHLDLumTh;      ///< [r/w] High light luminance  threshold 0~255
	UINT8 ucNC[17];         ///< [r/w] 17 control points of noise modulation curve
} KDRV_H26XENC_SPN;


#endif

