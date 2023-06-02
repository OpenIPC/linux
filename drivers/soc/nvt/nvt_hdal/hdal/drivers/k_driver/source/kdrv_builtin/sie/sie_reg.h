#ifndef __SIE_REG_H__
#define __SIE_REG_H__

//#define REGISTER_ADDR_START 0xf0c00000
//#define REGISTER_ADDR_END   0xf0c00900
//#define IRQ_ID              1


/*
    SIE register related header file.

    @file       sie1_reg.h
    @ingroup    mIIPPSIE

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
//This file should be included only once//#ifndef _SIE_NT96680_H_
//This file should be included only once//#define _SIE_NT96680_H_

//#include "mach/rcw_macro.h"
//#include "kwrap/type.h"
#include "sie_platform.h"

#if 0
#ifdef __KERNEL__
#define SIE_REG_ADDR(addr,ofs)        (addr+(ofs))
#define SIE_SETREG(addr,ofs, value)   iowrite32(value, (void*)(addr + ofs))
#define SIE_GETREG(addr,ofs)          ioread32((void*)(addr + ofs))
#else
#define SIE_REG_ADDR(addr,ofs)        (addr+(ofs))
#define SIE_SETREG(addr,ofs, value)   OUTW((addr + ofs), (value))
#define SIE_GETREG(addr,ofs)          INW(addr + ofs)
#endif
#endif
/*
    SIE_RST         :    [0x0, 0x1],            bits : 0
    SIE_LOAD        :    [0x0, 0x1],            bits : 1
    SIE_ACT_EN      :    [0x0, 0x1],            bits : 2
    MAIN_IN_SEL     :    [0x0, 0x3],            bits : 4_3
    PARAPL_VD_SEL   :    [0x0, 0x3],            bits : 6_5
    DRAM_IN_START   :    [0x0, 0x1],            bits : 7
    VD_PHASE        :    [0x0, 0x1],            bits : 8
    HD_PHASE        :    [0x0, 0x1],            bits : 9
    DATA_PHASE      :    [0x0, 0x1],            bits : 10
    VD_INV          :    [0x0, 0x1],            bits : 11
    HD_INV          :    [0x0, 0x1],            bits : 12
    RW_OBP          :    [0x0, 0x1],            bits : 13
    DIRECT_TO_RHE   :    [0x0, 0x1],            bits : 14
    SRAM_PWR_SAVE_BS:    [0x0, 0x1],            bits : 15
    SRAM_PWR_SAVE_VA:    [0x0, 0x1],            bits : 16
    MAIN_IN_SEL     :    [0x0, 0x3],            bits : 19_17
    PARAPL_VD_SEL   :    [0x0, 0x2],            bits : 21_20
*/
#define R0_ENGINE_CONTROL_OFS 0x0000
REGDEF_BEGIN(R0_ENGINE_CONTROL)
REGDEF_BIT(SIE_RST,        1)
REGDEF_BIT(SIE_LOAD,        1)
REGDEF_BIT(SIE_ACT_EN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(SIE_RAW_SEL, 	  1)
REGDEF_BIT(DRAM_OUT0_MODE,    1)
REGDEF_BIT(SIE_LOAD_FROM_SIE2_EN, 1)
REGDEF_BIT(DRAM_IN_START,        1)
REGDEF_BIT(VD_PHASE,        1)
REGDEF_BIT(HD_PHASE,        1)
REGDEF_BIT(DATA_PHASE,        1)
REGDEF_BIT(VD_INV,        1)
REGDEF_BIT(HD_INV,        1)
REGDEF_BIT(RW_OBP,        1)
REGDEF_BIT(DIRECT_TO_RHE,        1)
REGDEF_BIT(SRAM_PWR_SAVE_BS,        1)
REGDEF_BIT(SRAM_PWR_SAVE_VA,        1)
REGDEF_BIT(MAIN_IN_SEL,        3)
REGDEF_BIT(PARAPL_VD_SEL,        3)
REGDEF_BIT(,		1)
REGDEF_BIT(SIE_DMACH_DIS,		1)
REGDEF_BIT(SIE_DMACH_IDLE,		1)
REGDEF_BIT(,		4)
REGDEF_BIT(DRAM_OUT1_MODE,		1)
REGDEF_BIT(DRAM_OUT2_MODE,		1)
REGDEF_END(R0_ENGINE_CONTROL)


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
    INTE_VD             :    [0x0, 0x1],            bits : 0
    INTE_BP1            :    [0x0, 0x1],            bits : 1
    INTE_BP2            :    [0x0, 0x1],            bits : 2
    INTE_BP3            :    [0x0, 0x1],            bits : 3
    INTE_ACTST          :    [0x0, 0x1],            bits : 4
    INTE_CRPST          :    [0x0, 0x1],            bits : 5
    INTE_DRAM_IN_OUT_ERR:    [0x0, 0x1],            bits : 6
    INTE_DRAM_OUT0_END  :    [0x0, 0x1],            bits : 7
    INTE_DRAM_OUT1_END  :    [0x0, 0x1],            bits : 8
    INTE_DRAM_OUT2_END  :    [0x0, 0x1],            bits : 9
    INTE_DRAM_OUT3_END  :    [0x0, 0x1],            bits : 10
    INTE_DRAM_OUT4_END  :    [0x0, 0x1],            bits : 11
    INTE_DRAM_OUT5_END  :    [0x0, 0x1],            bits : 12
    INTE_DPCF           :    [0x0, 0x1],            bits : 13
    INTE_RAWENC_END     :    [0x0, 0x1],            bits : 14
    INTE_RAWENC_OUTOVFL :    [0x0, 0x1],            bits : 15
    INTE_ACTEND         :    [0x0, 0x1],            bits : 16
    INTE_CROPEND        :    [0x0, 0x1],            bits : 17
*/
#define R8_ENGINE_INTERRUPT_OFS 0x0008
REGDEF_BEGIN(R8_ENGINE_INTERRUPT)
REGDEF_BIT(INTE_VD,        1)
REGDEF_BIT(INTE_BP1,        1)
REGDEF_BIT(INTE_BP2,        1)
REGDEF_BIT(INTE_BP3,        1)
REGDEF_BIT(INTE_ACTST,        1)
REGDEF_BIT(INTE_CRPST,        1)
REGDEF_BIT(INTE_DRAM_IN_OUT_ERR,        1)
REGDEF_BIT(INTE_DRAM_OUT0_END,        1)
REGDEF_BIT(INTE_DRAM_OUT1_END,        1)
REGDEF_BIT(INTE_DRAM_OUT2_END,        1)
REGDEF_BIT(INTE_DRAM_OUT3_END,        1)
REGDEF_BIT(INTE_DRAM_OUT4_END,        1)
REGDEF_BIT(INTE_DRAM_OUT5_END,        1)
REGDEF_BIT(INTE_DPCF,        1)
REGDEF_BIT(INTE_RAWENC_END,        1)
REGDEF_BIT(INTE_RAWENC_OUTOVFL,        1)
REGDEF_BIT(INTE_ACTEND,        1)
REGDEF_BIT(INTE_CRPEND,        1)
REGDEF_BIT(, 	   1)
REGDEF_BIT(INTE_MD_HIT,        1)
REGDEF_END(R8_ENGINE_INTERRUPT)


/*
    INT_VD             :    [0x0, 0x1],            bits : 0
    INT_BP1            :    [0x0, 0x1],            bits : 1
    INT_BP2            :    [0x0, 0x1],            bits : 2
    INT_BP3            :    [0x0, 0x1],            bits : 3
    INT_ACTST          :    [0x0, 0x1],            bits : 4
    INT_CRPST          :    [0x0, 0x1],            bits : 5
    INT_DRAM_IN_OUT_ERR:    [0x0, 0x1],            bits : 6
    INT_DRAM_OUT0_END  :    [0x0, 0x1],            bits : 7
    INT_DRAM_OUT1_END  :    [0x0, 0x1],            bits : 8
    INT_DRAM_OUT2_END  :    [0x0, 0x1],            bits : 9
    INT_DRAM_OUT3_END  :    [0x0, 0x1],            bits : 10
    INT_DRAM_OUT4_END  :    [0x0, 0x1],            bits : 11
    INT_DRAM_OUT5_END  :    [0x0, 0x1],            bits : 12
    INT_DPCF           :    [0x0, 0x1],            bits : 13
    INT_RAWENC_END     :    [0x0, 0x1],            bits : 14
    INT_RAWENC_OUTOVFL :    [0x0, 0x1],            bits : 15
    INT_ACTEND         :    [0x0, 0x1],            bits : 16
    INT_CROPEND        :    [0x0, 0x1],            bits : 17
*/
#define RC_ENGINE_INTERRUPT_OFS 0x000c
REGDEF_BEGIN(RC_ENGINE_INTERRUPT)
REGDEF_BIT(INT_VD,        1)
REGDEF_BIT(INT_BP1,        1)
REGDEF_BIT(INT_BP2,        1)
REGDEF_BIT(INT_BP3,        1)
REGDEF_BIT(INT_ACTST,        1)
REGDEF_BIT(INT_CRPST,        1)
REGDEF_BIT(INT_DRAM_IN_OUT_ERR,        1)
REGDEF_BIT(INT_DRAM_OUT0_END,        1)
REGDEF_BIT(INT_DRAM_OUT1_END,        1)
REGDEF_BIT(INT_DRAM_OUT2_END,        1)
REGDEF_BIT(INT_DRAM_OUT3_END,        1)
REGDEF_BIT(INT_DRAM_OUT4_END,        1)
REGDEF_BIT(INT_DRAM_OUT5_END,        1)
REGDEF_BIT(INT_DPCF,        1)
REGDEF_BIT(INT_RAWENC_END,        1)
REGDEF_BIT(INT_RAWENC_OUTOVFL,        1)
REGDEF_BIT(INT_ACTEND,        1)
REGDEF_BIT(INT_CRPEND,        1)
REGDEF_BIT(,		  1)
REGDEF_BIT(INT_MD_HIT,        1)
REGDEF_END(RC_ENGINE_INTERRUPT)


/*
    PX_CNT            :    [0x0, 0x3fff],         bits : 13_0
    VSYNC             :    [0x0, 0x1],            bits : 16
    HSYNC             :    [0x0, 0x1],            bits : 17
    CCIR601_CURR_FIELD:    [0x0, 0x1],            bits : 18
    BUFF_IFE_OVFL     :    [0x0, 0x1],            bits : 23
    DRAM_IN1_UDFL     :    [0x0, 0x1],            bits : 24
    DRAM_IN2_UDFL     :    [0x0, 0x1],            bits : 25
    DRAM_OUT0_OVFL    :    [0x0, 0x1],            bits : 26
    DRAM_OUT1_OVFL    :    [0x0, 0x1],            bits : 27
    DRAM_OUT2_OVFL    :    [0x0, 0x1],            bits : 28
    DRAM_OUT3_OVFL    :    [0x0, 0x1],            bits : 29
    DRAM_OUT4_OVFL    :    [0x0, 0x1],            bits : 30
    DRAM_OUT5_OVFL    :    [0x0, 0x1],            bits : 31
*/
#define R10_ENGINE_STATUS_OFS 0x0010
REGDEF_BEGIN(R10_ENGINE_STATUS)
REGDEF_BIT(PX_CNT,        14)
REGDEF_BIT(,		2)
REGDEF_BIT(VSYNC,        1)
REGDEF_BIT(HSYNC,        1)
REGDEF_BIT(CCIR601_CURR_FIELD,        1)
REGDEF_BIT(,        4)
REGDEF_BIT(DRAM_OUT0_ERR,        1)
REGDEF_BIT(DRAM_IN1_UDFL,        1)
REGDEF_BIT(DRAM_IN2_UDFL,        1)
REGDEF_BIT(DRAM_OUT0_OVFL,        1)
REGDEF_BIT(DRAM_OUT1_OVFL,        1)
REGDEF_BIT(DRAM_OUT2_OVFL,        1)
REGDEF_BIT(CCIR656_HEADER_HIT,    1)
REGDEF_END(R10_ENGINE_STATUS)


/*
    CURR_DP_X:    [0x0, 0xffff],            bits : 15_0
    CURR_DP_Y:    [0x0, 0xffff],            bits : 31_16
*/
#define R14_ENGINE_STATUS_OFS 0x0014
REGDEF_BEGIN(R14_ENGINE_STATUS)
REGDEF_BIT(CURR_DP_X,        16)
REGDEF_BIT(CURR_DP_Y,        16)
REGDEF_END(R14_ENGINE_STATUS)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R18_ENGINE_STATUS_OFS 0x0018
REGDEF_BEGIN(R18_ENGINE_STATUS)
REGDEF_BIT(,                 4)
REGDEF_BIT(CHECKSUM_EN,		 1)
REGDEF_BIT(,		         7)
REGDEF_BIT(PXCLK_IN,		 1)
REGDEF_BIT(SIECLK_IN,		 1)
REGDEF_END(R18_ENGINE_STATUS)


/*
    SRC_WIDTH :    [0x0, 0x3fff],            bits : 13_0
    SRC_HEIGHT:    [0x0, 0x3fff],            bits : 29_16
*/
#define R1C_ENGINE_TIMING_OFS 0x001c
REGDEF_BEGIN(R1C_ENGINE_TIMING)
REGDEF_BIT(SRC_WIDTH,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(SRC_HEIGHT,        14)
REGDEF_END(R1C_ENGINE_TIMING)


/*
    BP1:    [0x0, 0x3fff],            bits : 13_0
    BP2:    [0x0, 0x3fff],            bits : 29_16
*/
#define R20_ENGINE_TIMING_OFS 0x0020
REGDEF_BEGIN(R20_ENGINE_TIMING)
REGDEF_BIT(BP1,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(BP2,        14)
REGDEF_END(R20_ENGINE_TIMING)


/*
    BP3:    [0x0, 0x3fff],            bits : 13_0
*/
#define R24_ENGINE_TIMING_OFS 0x0024
REGDEF_BEGIN(R24_ENGINE_TIMING)
REGDEF_BIT(BP3,        14)
REGDEF_END(R24_ENGINE_TIMING)


/*
    ACT_STX   :    [0x0, 0x3fff],         bits : 13_0
    ACT_STY   :    [0x0, 0x3fff],         bits : 29_16
*/
#define R28_ENGINE_TIMING_OFS 0x0028
REGDEF_BEGIN(R28_ENGINE_TIMING)
REGDEF_BIT(ACT_STX,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(ACT_STY,        14)
REGDEF_END(R28_ENGINE_TIMING)


/*
    ACT_SZX:    [0x0, 0x3fff],            bits : 13_0
    ACT_SZY:    [0x0, 0x3fff],            bits : 29_16
*/
#define R2C_ENGINE_TIMING_OFS 0x002c
REGDEF_BEGIN(R2C_ENGINE_TIMING)
REGDEF_BIT(ACT_SZX,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(ACT_SZY,        14)
REGDEF_END(R2C_ENGINE_TIMING)


/*
    ACT_CFAPAT :    [0x0, 0x2],            bits : 1_0
    CROP_CFAPAT:    [0x0, 0x2],            bits : 3_2
    CA_CFAPAT  :    [0x0, 0x2],            bits : 5_4
    VA_CFAPAT  :    [0x0, 0x2],            bits : 7_6
*/
#define R30_ENGINE_CFA_PATTERN_OFS 0x0030
REGDEF_BEGIN(R30_RESERVED)
REGDEF_BIT(ACT_CFAPAT,      3)
REGDEF_BIT(           ,     1)
REGDEF_BIT(CROP_CFAPAT,      3)
REGDEF_BIT(           ,      1)
REGDEF_BIT(CA_CFAPAT,      3)
REGDEF_BIT(         ,       1)
REGDEF_BIT(DPC_CFAPAT,      3)
REGDEF_END(R30_ENGINE_CFA_PATTERN)


/*
    CRP_STX   :    [0x0, 0x3fff],            bits : 13_0
    CRP_STY   :    [0x0, 0x3fff],            bits : 29_16
*/
#define R34_ENGINE_TIMING_OFS 0x0034
REGDEF_BEGIN(R34_ENGINE_TIMING)
REGDEF_BIT(CRP_STX,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(CRP_STY,        14)
REGDEF_END(R34_ENGINE_TIMING)


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
    PIX_PATE_CNT:    [0x0, 0xffff],            bits : 15_0
    PIX_PATE_MAX:    [0x0, 0xffff],            bits : 31_16
*/
#define R3C_ENGINE_STATUS_OFS 0x003c
REGDEF_BEGIN(R3C_ENGINE_STATUS)
REGDEF_BIT(PIX_PATE_CNT,        16)
REGDEF_BIT(PIX_PATE_MAX,        16)
REGDEF_END(R3C_ENGINE_STATUS)


/*
    DRAM_IN0_SAI     :    [0x0, 0x7ffffff],            bits : 28_2
    DRAM_IN0_PACK_BUS:    [0x0, 0x3],            bits : 31_30
*/
#define R40_ENGINE_DRAM_OFS 0x0040
REGDEF_BEGIN(R40_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_IN0_SAI,        29)

REGDEF_END(R40_ENGINE_DRAM)


/*
    DRAM_IN0_OFSO:    [0x0, 0x3fff],            bits : 15_2
*/
#define R44_ENGINE_TIMING_DELAY_OFS 0x0044
REGDEF_BEGIN(R44_ENGINE_TIMING_DELAY)
REGDEF_BIT(VD_DELAY,         2)
REGDEF_BIT(HD_DELAY,         2)
REGDEF_END(R44_ENGINE_TIMING_DELAY)


/*
    DRAM_IN1_SAI:    [0x0, 0x7ffffff],            bits : 28_2
*/
#define R48_ENGINE_DRAM_OFS 0x0048
REGDEF_BEGIN(R48_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_IN1_SAI,        29)
REGDEF_END(R48_ENGINE_DRAM)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4C_ENGINE_DRAM_OFS 0x004c
REGDEF_BEGIN(R4C_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_IN1_OFSO,       14)
REGDEF_BIT(DRAM_IN1_PACK_BUS,    2)
REGDEF_END(R4C_ENGINE_DRAM)


/*
    DRAM_IN2_SAI:    [0x0, 0x7ffffff],            bits : 28_2
*/
#define R50_ENGINE_DRAM_OFS 0x0050
REGDEF_BEGIN(R50_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_IN2_SAI,        29)
REGDEF_END(R50_ENGINE_DRAM)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R54_RESERVED_OFS 0x0054
REGDEF_BEGIN(R54_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R54_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R58_RESERVED_OFS 0x0058
REGDEF_BEGIN(R58_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R58_RESERVED)


/*
    DRAM_OUT0_SAO0  :    [0x0, 0x1fffffff],      bits : 30_2
*/
#define R5C_ENGINE_DRAM_OFS 0x005c
REGDEF_BEGIN(R5C_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_OUT0_SAO,        29)
REGDEF_END(R5C_ENGINE_DRAM)


/*
    DRAM_OUT0_OFSO    :    [0x0, 0x3fff],         bits : 15_2
    DRAM_OUT0_PACK_BUS:    [0x0, 0x3],            bits : 17_16
    DRAM_OUT0_HFLIP   :    [0x0, 0x1],            bits : 18
    DRAM_OUT0_VFLIP   :    [0x0, 0x1],            bits : 19
    DRAM_OUT0_BUFNUM  :    [0x0, 0x7],            bits : 22_20
    DRAM_OUT0_SOURCE  :    [0x0, 0x1],            bits : 24
*/
#define R60_ENGINE_DRAM_OFS 0x0060
REGDEF_BEGIN(R60_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_OUT0_OFSO,        14)
REGDEF_BIT(DRAM_OUT0_PACK_BUS,         2)
REGDEF_BIT(DRAM_OUT0_HFLIP,         1)
REGDEF_BIT(DRAM_OUT0_VFLIP,         1)
REGDEF_BIT(DRAM_OUT0_RINGBUF_EN,       1)
REGDEF_BIT(,	   7)
REGDEF_BIT(DRAM_OUT0_BITDEPTH_SEL,	   2)
REGDEF_BIT(DRAM_OUT0_PACK_FORMAT,	   1)
REGDEF_END(R60_ENGINE_DRAM)


/*
    DRAM_OUT0_FRMOFSO  :    [0x0, 0x2fffffff],            bits : 31_2
*/
#define R64_ENGINE_DRAM_OFS 0x0064
REGDEF_BEGIN(R64_ENGINE_DRAM)
REGDEF_BIT(DRAM_OUT0_RINGBUF_LEN,        11)
REGDEF_END(R64_ENGINE_DRAM)


/*
    DRAM_OUT1_SAO0  :    [0x0, 0x1fffffff],            bits : 30_2
*/
#define R68_ENGINE_DRAM_OFS 0x0068
REGDEF_BEGIN(R68_ENGINE_DRAM)
REGDEF_BIT(,        2)
REGDEF_BIT(DRAM_OUT1_SAO,        29)
REGDEF_END(R68_ENGINE_DRAM)


/*
    DRAM_OUT1_OFSO    :    [0x0, 0x3fff],         bits : 15_2
    DRAM_OUT1_HFLIP   :    [0x0, 0x1],            bits : 18
    DRAM_OUT1_VFLIP   :    [0x0, 0x1],            bits : 19
    DRAM_OUT1_BUFNUM  :    [0x0, 0x7],            bits : 22_20
*/
#define R6C_ENGINE_DRAM_OFS 0x006c
REGDEF_BEGIN(R6C_ENGINE_DRAM)
REGDEF_BIT(,        2)
REGDEF_BIT(DRAM_OUT1_OFSO,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(DRAM_OUT1_HFLIP,        1)
REGDEF_BIT(DRAM_OUT1_VFLIP,        1)
REGDEF_END(R6C_ENGINE_DRAM)


/*
    DRAM_OUT1_FRMOFSO  :    [0x0, 0x2fffffff],            bits : 31_2
*/
#define R70_ENGINE_DRAM_OFS 0x0070
REGDEF_BEGIN(R70_ENGINE_DRAM)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R70_ENGINE_DRAM)


/*
    DRAM_OUT2_SAO0  :    [0x0, 0x1fffffff],            bits : 30_2
*/
#define R74_ENGINE_DRAM_OFS 0x0074
REGDEF_BEGIN(R74_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_OUT2_SAO,        29)
REGDEF_END(R74_ENGINE_DRAM)


/*
    DRAM_OUT2_OFSO    :    [0x0, 0x3fff],            bits : 15_2
    DRAM_OUT2_BUFNUM  :    [0x0, 0x7],               bits : 22_20
*/
#define R78_ENGINE_DRAM_OFS 0x0078
REGDEF_BEGIN(R78_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_OUT2_OFSO,        14)
REGDEF_END(R78_ENGINE_DRAM)


/*
    DRAM_OUT2_FRMOFSO  :    [0x0, 0x2fffffff],            bits : 31_2
*/
#define R7C_ENGINE_DRAM_OFS 0x007c
REGDEF_BEGIN(R7C_ENGINE_DRAM)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7C_ENGINE_DRAM)


/*
    DRAM_OUT3_SAO0  :    [0x0, 0x1fffffff],            bits : 30_2
*/
#define R80_ENGINE_DRAM_OFS 0x0080
REGDEF_BEGIN(R80_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_OUT3_SAO,         29)
REGDEF_END(R80_ENGINE_DRAM)


/*
    DRAM_OUT3_OFSO    :    [0x0, 0x3fff],         bits : 15_2
    DRAM_OUT3_PACK_BUS:    [0x0, 0x3],            bits : 17_16
    DRAM_OUT3_BUFNUM  :    [0x0, 0x1],            bits : 22_20
    DRAM_OUT3_SOURCE  :    [0x0, 0x1],            bits : 24
*/
#define R84_ENGINE_DRAM_OFS 0x0084
REGDEF_BEGIN(R84_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_OUT3_OFSO,        14)
REGDEF_BIT(DRAM_OUT3_PACK_BUS,         2)
REGDEF_BIT(DRAM_OUT3_HFLIP,         1)
REGDEF_BIT(DRAM_OUT3_VFLIP,         1)
REGDEF_BIT(,         4)
REGDEF_BIT(DRAM_OUT3_SOURCE,         2)
REGDEF_END(R84_ENGINE_DRAM)


/*
    DRAM_OUT3_FRMOFSO  :    [0x0, 0x2fffffff],      bits : 31_2
*/
#define R88_ENGINE_DRAM_OFS 0x0088
REGDEF_BEGIN(R88_ENGINE_DRAM)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R88_ENGINE_DRAM)


/*
    DRAM_OUT4_SAO0  :    [0x0, 0x1fffffff],      bits : 30_2
*/
#define R8C_ENGINE_DRAM_OFS 0x008c
REGDEF_BEGIN(R8C_ENGINE_DRAM)
REGDEF_BIT(,        2)
REGDEF_BIT(DRAM_OUT4_SAO,        29)
REGDEF_END(R8C_ENGINE_DRAM)


/*
    DRAM_OUT4_OFSO    :    [0x0, 0x3fff],         bits : 15_2
    DRAM_OUT4_BUFNUM  :    [0x0, 0x7],            bits : 22_20
    DRAM_OUT4_SOURCE  :    [0x0, 0x1],            bits : 24
*/
#define R90_ENGINE_DRAM_OFS 0x0090
REGDEF_BEGIN(R90_ENGINE_DRAM)
REGDEF_BIT(,        2)
REGDEF_BIT(DRAM_OUT4_OFSO,        14)
REGDEF_END(R90_ENGINE_DRAM)


/*
    DRAM_OUT4_FRMOFSO  :    [0x0, 0x2fffffff],            bits : 31_2
*/
#define R94_ENGINE_DRAM_OFS 0x0094
REGDEF_BEGIN(R94_ENGINE_DRAM)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R94_ENGINE_DRAM)


/*
    DRAM_OUT5_SAO:    [0x0, 0x1fffffff],            bits : 30_2
*/
#define R98_ENGINE_DRAM_OFS 0x0098
REGDEF_BEGIN(R98_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_OUT5_SAO,        29)
REGDEF_END(R98_ENGINE_DRAM)


/*
    DRAM_OUT5_OFSO  :    [0x0, 0x7ffffff],            bits : 30_2
    DRAM_OUT5_BUBNUM:    [0x0, 0x7],                  bits : 22_20
*/
#define R9C_ENGINE_DRAM_OFS 0x009c
REGDEF_BEGIN(R9C_ENGINE_DRAM)
REGDEF_BIT(,         2)
REGDEF_BIT(DRAM_OUT5_OFSO,        14)
REGDEF_BIT(,         4)
REGDEF_BIT(DRAM_OUT5_BUFNUM,         3)
REGDEF_END(R9C_ENGINE_DRAM)


/*
    DRAM_OUT5_FRMOFSO  :    [0x0, 0x2fffffff],            bits : 31_2
*/
#define RA0_ENGINE_DRAM_OFS 0x00a0
REGDEF_BEGIN(RA0_ENGINE_DRAM)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(RA0_ENGINE_DRAM)

/*
    OUT0_CURR_DRAM_BUF_OID      :    [0x0, 0x7],            bits : 2_0
    OUT1_CURR_DRAM_BUF_OID      :    [0x0, 0x7],            bits : 6_4
    OUT2_CURR_DRAM_BUF_OID      :    [0x0, 0x7],            bits :10_8
    OUT3_CURR_DRAM_BUF_OID      :    [0x0, 0x7],            bits :14_12
    OUT4_CURR_DRAM_BUF_OID      :    [0x0, 0x7],            bits :18_16
    OUT5_CURR_DRAM_BUF_OID      :    [0x0, 0x7],            bits :22_20
*/
#define RA4_ENGINE_DRAM_OFS 0x00a4
REGDEF_BEGIN(RA4_ENGINE_DRAM)
REGDEF_BIT(,        28)
REGDEF_BIT(DRAM_OUT0_SINGLE_EN, 1)
REGDEF_BIT(DRAM_OUT1_SINGLE_EN, 1)
REGDEF_BIT(DRAM_OUT2_SINGLE_EN, 1)
REGDEF_BIT(,         1)
REGDEF_END(RA4_ENGINE_DRAM)

/*
    DRAM_BURST_LENGTH_OUT0      :    [0x0, 0x7],            bits : 2_0
    DRAM_BURST_LENGTH_OUT1      :    [0x0, 0x1],            bits :   4
    DRAM_BURST_LENGTH_OUT2      :    [0x0, 0x1],            bits :   6
    DRAM_BURST_LENGTH_OUT3      :    [0x0, 0x3],            bits : 9_8
    DRAM_BURST_LENGTH_OUT4      :    [0x0, 0x1],            bits :  10
    DRAM_BURST_LENGTH_OUT5      :    [0x0, 0x1],            bits :  12
    DRAM_BURST_LENGTH_IN0       :    [0x0, 0x1],            bits :  14
    DRAM_BURST_LENGTH_IN1       :    [0x0, 0x1],            bits :  16
    DRAM_BURST_LENGTH_IN2       :    [0x0, 0x1],            bits :  18
    DRAM_OUT0_BURST_MIN_INTERVAL:    [0x0, 0x3ff],          bits :29_20
*/
#define RA8_ENGINE_DRAM_OFS 0x00a8
REGDEF_BEGIN(RA8_ENGINE_DRAM)
REGDEF_BIT(DRAM_BURST_LENGTH_OUT0,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(DRAM_BURST_LENGTH_OUT1,        2)
REGDEF_BIT(DRAM_BURST_LENGTH_OUT2,        2)
REGDEF_BIT(DRAM_BURST_LENGTH_OUT3,        8)
REGDEF_BIT(DRAM_BURST_LENGTH_IN1,         2)
REGDEF_BIT(DRAM_BURST_LENGTH_IN2,         2)
REGDEF_BIT(DRAM_OUT0_BURST_MIN_INTERVAL,       10)
REGDEF_END(RA8_ENGINE_DRAM)


/*
    DVI_FORMAT         :    [0x0, 0x1],            bits : 0
    DVI_MODE           :    [0x0, 0x1],            bits : 1
    CCIR656_ACT_SEL    :    [0x0, 0x1],            bits : 2
    FIELD_EN           :    [0x0, 0x1],            bits : 3
    FIELD_SEL          :    [0x0, 0x1],            bits : 4
    HD_IN_SWAP         :    [0x0, 0x1],            bits : 5
    OUT_SWAP           :    [0x0, 0x3],            bits : 7_6
    OUT_SPLIT          :    [0x0, 0x1],            bits : 8
    OUT_YUV420         :    [0x0, 0x1],            bits : 9
    PARAL_DATA_PERIOD  :    [0x0, 0x3],            bits : 13_12
    PARAL_DATA_INDEX   :    [0x0, 0x3],            bits : 15_14
    PARAL_CH_ID        :    [0x0, 0xF],            bits : 19_16
    CCIR656_AUTO_ALIGH :    [0x0, 0x1],            bits : 30
    CCIR656_VD_MODE    :    [0x0, 0x1],            bits : 31
*/
#define RAC_BASIC_DVI_OFS 0x00ac
REGDEF_BEGIN(RAC_BASIC_DVI)
REGDEF_BIT(DVI_FORMAT,        1)
REGDEF_BIT(DVI_MODE,        1)
REGDEF_BIT(CCIR656_ACT_SEL,        1)
REGDEF_BIT(FIELD_EN,        1)
REGDEF_BIT(FIELD_SEL,        1)
REGDEF_BIT(HD_IN_SWAP,        1)
REGDEF_BIT(OUT_SWAP,        2)
REGDEF_BIT(OUT_SPLIT,        1)
REGDEF_BIT(OUT_YUV420,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(PARAL_DATA_PERIOD,      2)
REGDEF_BIT(PARAL_DATA_INDEX,       2)
REGDEF_BIT(PARAL_CH_ID,            4)
REGDEF_BIT(,       10)
REGDEF_BIT(CCIR656_AUTO_ALIGN,     1)
REGDEF_BIT(CCIR656_VD_MODE,        1)
REGDEF_END(RAC_BASIC_DVI)


/*
    PATGEN_MODE:    [0x0, 0x7],            bits : 2_0
    PATGEN_VAL :    [0x0, 0xfff],          bits : 27_16
*/
#define RB0_BASIC_PATGEN_OFS 0x00b0
REGDEF_BEGIN(RB0_BASIC_PATGEN)
REGDEF_BIT(PATGEN_MODE,        3)
REGDEF_BIT(,        13)
REGDEF_BIT(PATGEN_VAL,        12)
REGDEF_END(RB0_BASIC_PATGEN)


/*
    OBDT_X:    [0x0, 0x3fff],            bits : 13_0
    OBDT_Y:    [0x0, 0x3fff],            bits : 29_16
*/
#define RB4_BASIC_OB_OFS 0x00b4
REGDEF_BEGIN(RB4_BASIC_OB)
REGDEF_BIT(OBDT_X,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(OBDT_Y,        14)
REGDEF_END(RB4_BASIC_OB)


/*
    OBDT_WIDTH :    [0x0, 0x3fff],            bits : 13_0
    OBDT_HEIGHT:    [0x0, 0xf]   ,            bits : 19_16
*/
#define RB8_BASIC_OB_OFS 0x00b8
REGDEF_BEGIN(RB8_BASIC_OB)
REGDEF_BIT(OBDT_WIDTH,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(OBDT_HEIGHT,         4)
REGDEF_END(RB8_BASIC_OB)


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
    OBDT_THRES:    [0x0, 0xfff],            bits : 11_0
    OBDT_XDIV :    [0x0, 0xff] ,            bits : 23_16
*/
#define RC0_BASIC_OB_OFS 0x00c0
REGDEF_BEGIN(RC0_BASIC_OB)
REGDEF_BIT(OBDT_THRES,        12)
REGDEF_BIT(,         4)
REGDEF_BIT(OBDT_XDIV,         8)
REGDEF_END(RC0_BASIC_OB)


/*
    OBDT_AVER:    [0x0, 0xfff],            bits : 11_0
    OBDT_SCNT:    [0x0, 0x1ff],            bits : 24_16
*/
#define RC4_BASIC_OB_OFS 0x00c4
REGDEF_BEGIN(RC4_BASIC_OB)
REGDEF_BIT(OBDT_AVER,        12)
REGDEF_BIT(,         4)
REGDEF_BIT(OBDT_SCNT,         9)
REGDEF_END(RC4_BASIC_OB)


/*
    CCIR656_WIDTH:    [0x0, 0xffff],            bits : 15_0
    CCIR656_HEIGHT:   [0x0, 0xffff],            bits : 31_16
*/
#define RC8_BASIC_CCIR_OFS 0x00c8
REGDEF_BEGIN(RC8_BASIC_CCIR)
REGDEF_BIT(CCIR656_WIDTH,        16)
REGDEF_BIT(CCIR656_HEIGHT,       16)
REGDEF_END(RC8_BASIC_CCIR)


/*
    DEF_FACT:    [0x0, 0x3],            bits : 1_0
*/
#define RCC_BASIC_DEFECT_OFS 0x00cc
REGDEF_BEGIN(RCC_BASIC_DEFECT)
REGDEF_BIT(DEF_FACT,        2)
REGDEF_END(RCC_BASIC_DEFECT)


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
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R134_VD_INFO_OFS 0x0134
REGDEF_BEGIN(R134_VD_INFO)
REGDEF_BIT(VD_TIME_COUNT_THR,        32)
REGDEF_END(R134_VD_INFO)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R138_HD_INFO_OFS 0x0138
REGDEF_BEGIN(R138_HD_INFO)
REGDEF_BIT(HD_TIME_COUNT_THR,        32)
REGDEF_END(R138_HD_INFO)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R13C_VD_INTEV_OFS 0x013c
REGDEF_BEGIN(R13C_VD_INTEV)
REGDEF_BIT(VD_INTERVAL_COUNT_LOWER_THR,        32)
REGDEF_END(R13C_VD_INTEV)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R140_VD_INTEV_OFS 0x0140
REGDEF_BEGIN(R140_VD_INTEV)
REGDEF_BIT(VD_INTERVAL_COUNT_UPPER_THR,        32)
REGDEF_END(R140_VD_INTEV)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R144_HD_INTEV_OFS 0x0144
REGDEF_BEGIN(R144_HD_INTEV)
REGDEF_BIT(HD_INTERVAL_COUNT_LOWER_THR,        32)
REGDEF_END(R144_HD_INTEV)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R148_HD_INTEV_OFS 0x0148
REGDEF_BEGIN(R148_HD_INTEV)
REGDEF_BIT(HD_INTERVAL_COUNT_UPPER_THR,        32)
REGDEF_END(R148_HD_INTEV)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R14C_PXCLK_INFO_OFS 0x014c
REGDEF_BEGIN(R14C_PXCLK_INFO)
REGDEF_BIT(PXCLK_PIX_CNT,        16)
REGDEF_BIT(PXCLK_PIX_MAX_CNT,    16)
REGDEF_END(R14C_PXCLK_INFO)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R150_CLK_INFO_OFS 0x0150
REGDEF_BEGIN(R150_CLK_INFO)
REGDEF_BIT(SIECLK_PIX_CNT,        16)
REGDEF_BIT(SIECLK_PIX_MAX_CNT,        16)
REGDEF_END(R150_CLK_INFO)


/*
    MAP_SIZESEL   :    [0x0, 0x3],            bits : 1_0
    MAP_DTHR_EN   :    [0x0, 0x1],            bits : 4
    MAP_DTHR_RST  :    [0x0, 0x1],            bits : 5
    MAP_DTHR_LEVEL:    [0x0, 0x7],            bits : 10_8
    MAP_SHIFT     :    [0x0, 0xf],            bits : 19_16
*/
#define R154_BASIC_ECS_OFS 0x0154
REGDEF_BEGIN(R154_BASIC_ECS)
REGDEF_BIT(MAP_SIZESEL,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(MAP_DTHR_EN,        1)
REGDEF_BIT(MAP_DTHR_RST,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(MAP_DTHR_LEVEL,        3)
REGDEF_BIT(,        5)
REGDEF_BIT(MAP_SHIFT,        4)
REGDEF_END(R154_BASIC_ECS)


/*
    MAP_HSCL:    [0x0, 0x7fff],            bits : 13_0
    MAP_VSCL:    [0x0, 0x7fff],            bits : 29_16
*/
#define R158_BASIC_ECS_OFS 0x0158
REGDEF_BEGIN(R158_BASIC_ECS)
REGDEF_BIT(MAP_HSCL,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(MAP_VSCL,        14)
REGDEF_END(R158_BASIC_ECS)


/*
    DGAIN_GAIN:    [0x0, 0x3ff],            bits : 9_0
*/
#define R15C_BASIC_DGAIN_OFS 0x015c
REGDEF_BEGIN(R15C_BASIC_DGAIN)
REGDEF_BIT(DGAIN_GAIN,        16)
REGDEF_END(R15C_BASIC_DGAIN)

/*
    BS_H_IV    :    [0x0, 0x1fffff],            bits : 20_0
    BS_SRC_INTP:    [0x0, 0x1]     ,            bits : 31
*/
#define R160_BASIC_BSH_OFS 0x0160
REGDEF_BEGIN(R160_BASIC_BSH)
REGDEF_BIT(BS_H_IV,        21)
REGDEF_BIT(,        10)
REGDEF_BIT(BS_SRC_INTP,        1)
REGDEF_END(R160_BASIC_BSH)

#if 0
/*  528 only
    LN_CNT    :    [0x0, 0x3fff],            bits : 13_0
*/
#define R160_DEBUG_OFS 0x0160
REGDEF_BEGIN(R160_DEBUG)
REGDEF_BIT(LN_CNT,        14)
REGDEF_BIT(,		 2)
REGDEF_BIT(LN_CNT_MAX,        14)
REGDEF_END(R160_DEBUG)
#endif

/*
    BS_H_SV:    [0x0, 0xfffff],            bits : 19_0
*/
#define R164_BASIC_BSH_OFS 0x0164
REGDEF_BEGIN(R164_BASIC_BSH)
REGDEF_BIT(BS_H_SV,        20)
REGDEF_END(R164_BASIC_BSH)


/*
    BS_H_BV_R:    [0x0, 0x1fffff],            bits : 20_0
*/
#define R168_BASIC_BSH_OFS 0x0168
REGDEF_BEGIN(R168_BASIC_BSH)
REGDEF_BIT(BS_H_BV_R,        21)
REGDEF_END(R168_BASIC_BSH)


/*
    BS_H_BV_B:    [0x0, 0x1fffff],            bits : 20_0
*/
#define R16C_BASIC_BSH_OFS 0x016c
REGDEF_BEGIN(R16C_BASIC_BSH)
REGDEF_BIT(BS_H_BV_B,        21)
REGDEF_END(R16C_BASIC_BSH)


/*
    BS_H_OUTSIZE   :    [0x0, 0x1fff],            bits : 12_0
    BS_H_ACC_DIV_M0:    [0x0, 0x3ff],            bits : 25_16
    BS_H_ACC_DIV_S0:    [0x0, 0x7]  ,            bits : 30_28
*/
#define R170_BASIC_BSH_OFS 0x0170
REGDEF_BEGIN(R170_BASIC_BSH)
REGDEF_BIT(BS_H_OUTSIZE,        13)
REGDEF_BIT(,         3)
REGDEF_BIT(BS_H_ACC_DIV_M0,        10)
REGDEF_BIT(,         2)
REGDEF_BIT(BS_H_ACC_DIV_S0,         3)
REGDEF_END(R170_BASIC_BSH)


/*
    BS_V_IV:    [0x0, 0xfffff],            bits : 19_0
*/
#define R174_BASIC_BSV_OFS 0x0174
REGDEF_BEGIN(R174_BASIC_BSV)
REGDEF_BIT(BS_V_IV,        20)
REGDEF_END(R174_BASIC_BSV)


/*
    BS_V_SV:    [0x0, 0x7ffff],            bits : 18_0
*/
#define R178_BASIC_BSV_OFS 0x0178
REGDEF_BEGIN(R178_BASIC_BSV)
REGDEF_BIT(BS_V_SV,        19)
REGDEF_END(R178_BASIC_BSV)


/*
    BS_V_BV_R:    [0x0, 0x1fffff],            bits : 20_0
*/
#define R17C_BASIC_BSV_OFS 0x017c
REGDEF_BEGIN(R17C_BASIC_BSV)
REGDEF_BIT(BS_V_BV_R,        21)
REGDEF_END(R17C_BASIC_BSV)

#if 0
/*  for 520 only
    CSI_HD_CNT:        [0x0, 0xffff],            bits : 15_0
    CSI_LN_END_CNT:    [0x0, 0xffff],            bits : 31_16
*/
#define R180_DEBUG_OFS 0x0180
REGDEF_BEGIN(R180_DEBUG)
REGDEF_BIT(CSI_HD_CNT,        16)
REGDEF_BIT(CSI_LN_END_CNT,        16)
REGDEF_END(R180_DEBUG)


/*  for 520 only
    CSI_PXL_CNT   :    [0x0, 0xffff],          bits : 15_0
    
*/
#define R184_DEBUG_OFS 0x0184
REGDEF_BEGIN(R184_DEBUG)
REGDEF_BIT(CSI_PXL_CNT,        16)
REGDEF_END(R184_DEBUG)
#endif

/*
    BS_V_BV_B:    [0x0, 0x1fffff],            bits : 20_0
*/
#define R180_BASIC_BSV_OFS 0x0180
REGDEF_BEGIN(R180_BASIC_BSV)
REGDEF_BIT(BS_V_BV_B,        21)
REGDEF_END(R180_BASIC_BSV)


/*
    BS_V_OUTSIZE   :    [0x0, 0xfff],          bits : 12_0
    BS_V_ACC_IN_S  :    [0x0, 0x3],            bits : 15_14
    BS_V_ACC_DIV_M0:    [0x0, 0x3ff],          bits : 25_16
    BS_V_ACC_DIV_S0:    [0x0, 0x7],            bits : 30_28
*/
#define R184_BASIC_BSV_OFS 0x0184
REGDEF_BEGIN(R184_BASIC_BSV)
REGDEF_BIT(BS_V_OUTSIZE,        13)
REGDEF_BIT(,        1)
REGDEF_BIT(BS_V_ACC_IN_S,        2)
REGDEF_BIT(BS_V_ACC_DIV_M0,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S0,        3)
REGDEF_END(R184_BASIC_BSV)


/*
    BS_V_ACC_DIV_M_RB0:    [0x0, 0x3ff],            bits : 9_0
    BS_V_ACC_DIV_S_RB0:    [0x0, 0xf],            bits : 15_12
    BS_V_ACC_DIV_M_RB1:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S_RB1:    [0x0, 0xf],            bits : 31_28
*/
#define R188_BASIC_BSV_OFS 0x0188
REGDEF_BEGIN(R188_BASIC_BSV)
REGDEF_BIT(BS_V_ACC_DIV_M_RB0,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB0,        4)
REGDEF_BIT(BS_V_ACC_DIV_M_RB1,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB1,        4)
REGDEF_END(R188_BASIC_BSV)


/*
    BS_V_ACC_DIV_M_RB2:    [0x0, 0x3ff],            bits : 9_0
    BS_V_ACC_DIV_S_RB2:    [0x0, 0xf],            bits : 15_12
    BS_V_ACC_DIV_M_RB3:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S_RB3:    [0x0, 0xf],            bits : 31_28
*/
#define R18C_BASIC_BSV_OFS 0x018c
REGDEF_BEGIN(R18C_BASIC_BSV)
REGDEF_BIT(BS_V_ACC_DIV_M_RB2,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB2,        4)
REGDEF_BIT(BS_V_ACC_DIV_M_RB3,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB3,        4)
REGDEF_END(R18C_BASIC_BSV)

#if 0
/*  for 528 only
    CSI_HD_CNT:        [0x0, 0xffff],            bits :15_0
    CSI_LN_END_CNT:    [0x0, 0xffff],            bits : 15_12
*/
#define R190_DEBUG_OFS 0x0190
REGDEF_BEGIN(R190_DEBUG)
REGDEF_BIT(CSI_HD_CNT,            16)
REGDEF_BIT(CSI_LN_END_CNT,        16)
REGDEF_END(R190_DEBUG)


/*  for 528 only
    CSI_PXL_CNT:    [0x0, 0xffff],            bits : 15_0
*/
#define R194_DEBUG_OFS 0x0194
REGDEF_BEGIN(R194_DEBUG)
REGDEF_BIT(CSI_PXL_CNT,        16)
REGDEF_END(R194_DEBUG)
#endif

/*
    BS_V_ACC_DIV_M_RB4:    [0x0, 0x3ff],            bits : 9_0
    BS_V_ACC_DIV_S_RB4:    [0x0, 0xf],            bits : 15_12
    BS_V_ACC_DIV_M_RB5:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S_RB5:    [0x0, 0xf],            bits : 31_28
*/
#define R190_BASIC_BSV_OFS 0x0190
REGDEF_BEGIN(R190_BASIC_BSV)
REGDEF_BIT(BS_V_ACC_DIV_M_RB4,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB4,        4)
REGDEF_BIT(BS_V_ACC_DIV_M_RB5,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB5,        4)
REGDEF_END(R190_BASIC_BSV)


/*
    BS_V_ACC_DIV_M_RB6:    [0x0, 0x3ff],            bits : 9_0
    BS_V_ACC_DIV_S_RB6:    [0x0, 0xf],            bits : 15_12
    BS_V_ACC_DIV_M_RB7:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S_RB7:    [0x0, 0xf],            bits : 31_28
*/
#define R194_BASIC_BSV_OFS 0x0194
REGDEF_BEGIN(R194_BASIC_BSV)
REGDEF_BIT(BS_V_ACC_DIV_M_RB6,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB6,        4)
REGDEF_BIT(BS_V_ACC_DIV_M_RB7,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB7,        4)
REGDEF_END(R194_BASIC_BSV)


/*
    BS_V_ACC_DIV_M_RB8:    [0x0, 0x3ff],            bits : 9_0
    BS_V_ACC_DIV_S_RB8:    [0x0, 0xf],            bits : 15_12
    BS_V_ACC_DIV_M_RB9:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S_RB9:    [0x0, 0xf],            bits : 31_28
*/
#define R198_BASIC_BSV_OFS 0x0198
REGDEF_BEGIN(R198_BASIC_BSV)
REGDEF_BIT(BS_V_ACC_DIV_M_RB8,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB8,        4)
REGDEF_BIT(BS_V_ACC_DIV_M_RB9,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB9,        4)
REGDEF_END(R198_BASIC_BSV)


/*
    BS_V_ACC_DIV_M_RB10:    [0x0, 0x3ff],            bits : 9_0
    BS_V_ACC_DIV_S_RB10:    [0x0, 0xf],            bits : 15_12
    BS_V_ACC_DIV_M_RB11:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S_RB11:    [0x0, 0xf],            bits : 31_28
*/
#define R19C_BASIC_BSV_OFS 0x019c
REGDEF_BEGIN(R19C_BASIC_BSV)
REGDEF_BIT(BS_V_ACC_DIV_M_RB10,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB10,        4)
REGDEF_BIT(BS_V_ACC_DIV_M_RB11,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB11,        4)
REGDEF_END(R19C_BASIC_BSV)


/*
    BS_V_ACC_DIV_M_RB12:    [0x0, 0x3ff],            bits : 9_0
    BS_V_ACC_DIV_S_RB12:    [0x0, 0xf],            bits : 15_12
    BS_V_ACC_DIV_M_RB13:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S_RB13:    [0x0, 0xf],            bits : 31_28
*/
#define R1A0_BASIC_BSV_OFS 0x01a0
REGDEF_BEGIN(R1A0_BASIC_BSV)
REGDEF_BIT(BS_V_ACC_DIV_M_RB12,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB12,        4)
REGDEF_BIT(BS_V_ACC_DIV_M_RB13,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB13,        4)
REGDEF_END(R1A0_BASIC_BSV)


/*
    BS_V_ACC_DIV_M_RB14:    [0x0, 0x3ff],            bits : 9_0
    BS_V_ACC_DIV_S_RB14:    [0x0, 0xf],            bits : 15_12
    BS_V_ACC_DIV_M_RB15:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S_RB15:    [0x0, 0xf],            bits : 31_28
*/
#define R1A4_BASIC_BSV_OFS 0x01a4
REGDEF_BEGIN(R1A4_BASIC_BSV)
REGDEF_BIT(BS_V_ACC_DIV_M_RB14,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB14,        4)
REGDEF_BIT(BS_V_ACC_DIV_M_RB15,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S_RB15,        4)
REGDEF_END(R1A4_BASIC_BSV)


/*
    BS_H_ACC_DIV_M1:    [0x0, 0x3ff],            bits : 9_0
    BS_H_ACC_DIV_S1:    [0x0, 0x7],            bits : 14_12
    BS_V_ACC_DIV_M1:    [0x0, 0x3ff],            bits : 25_16
    BS_V_ACC_DIV_S1:    [0x0, 0x7],            bits : 30_28
*/
#define R1A8_BASIC_BSV_OFS 0x01a8
REGDEF_BEGIN(R1A8_BASIC_BSV)
REGDEF_BIT(BS_H_ACC_DIV_M1,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_H_ACC_DIV_S1,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(BS_V_ACC_DIV_M1,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(BS_V_ACC_DIV_S1,        3)
REGDEF_END(R1A8_BASIC_BSV)


/*
    YOUT_SMPL_X_SFACT:    [0x0, 0xffff],            bits : 15_0
    YOUT_SMPL_Y_SFACT:    [0x0, 0xffff],            bits : 31_16
*/
#define R1AC_BASIC_CCIR_DBG_OFS 0x01ac
REGDEF_BEGIN(R1AC_BASIC_CCIR_DBG)
REGDEF_BIT(CCIR656_HEADER,      16)
REGDEF_END(R1AC_BASIC_CCIR_DBG)


/*
    YOUT_WIN_SZX       :    [0x0, 0xff],           bits : 7_0
    YOUT_WIN_SZY       :    [0x0, 0xff],           bits : 15_8
    YOUT_ACCUM_SHIFT   :    [0x0, 0xf],            bits : 19_16
    YOUT_ACCUM_GAIN    :    [0x0, 0xff],           bits : 27_20
*/
#define R1B0_BASIC_CCIR_DBG_OFS 0x01b0
REGDEF_BEGIN(R1B0_BASIC_CCIR_DBG)
REGDEF_BIT(CCIR656_HEADER_STATUS,        32)
REGDEF_END(R1B0_BASIC_CCIR_DBG)


/*
    CGAIN_RGAIN:    [0x0, 0x3ff],            bits : 9_0
    CGAIN_GRGAIN:    [0x0, 0x3ff],            bits : 25_16
*/
#define R1B4_BASIC_CG_OFS 0x01b4
REGDEF_BEGIN(R1B4_BASIC_CG)
REGDEF_BIT(CGAIN_RGAIN,    10)
REGDEF_BIT(,        6)
REGDEF_BIT(CGAIN_GRGAIN,    10)
REGDEF_END(R1B4_BASIC_CG)


/*
    CGAIN_GBGAIN    :    [0x0, 0x3ff],          bits : 9_0
    CGAIN_BGAIN    :    [0x0, 0x3ff],          bits : 25_16
*/
#define R1B8_BASIC_CG_OFS 0x01b8
REGDEF_BEGIN(R1B8_BASIC_CG)
REGDEF_BIT(CGAIN_GBGAIN,   10)
REGDEF_BIT(,        6)
REGDEF_BIT(CGAIN_BGAIN,    10)
REGDEF_END(R1B8_BASIC_CG)


/*
    CGAIN_IRGAIN    :    [0x0, 0x3ff],          bits : 25_16
    CGAIN_LEVEL_SEL:    [0x0, 0x1],            bits : 28
*/
#define R1BC_BASIC_CG_OFS 0x01bc
REGDEF_BEGIN(R1BC_BASIC_CG)
REGDEF_BIT(CGAIN_IRGAIN, 10)
REGDEF_BIT(,         6)
REGDEF_BIT(CGAIN_LEVEL_SEL,  1)
REGDEF_END(R1BC_BASIC_CG)


/*
    GRID_LINE_VAL2  :    [0x0, 0xfff],            bits : 11_0
    GRID_LINE_VAL3  :    [0x0, 0xfff],            bits : 27_16
    GRID_LINE_SHFTBS:    [0x0, 0x7],              bits : 30_28
*/
#define R1C0_BASIC_GRIDLINE_OFS 0x01c0
REGDEF_BEGIN(R1C0_BASIC_GRIDLINE)
REGDEF_BIT(GRID_LINE_VAL2, 12)
REGDEF_BIT(,        4)
REGDEF_BIT(GRID_LINE_VAL3, 12)
REGDEF_BIT(GRID_LINE_SHFTBS, 3)
REGDEF_END(R1C0_BASIC_GRIDLINE)


/*
    DVS_POS_CODE:      [0x0, 0xff],            bits : 7_0
    DVS_NEG_CODE:      [0x0, 0xff],            bits : 15_8
    DVS_NOCHG_CODE:    [0x0, 0xff],            bits : 23_16
*/
#define R1C4_BASIC_DVS_OFS 0x01c4
	REGDEF_BEGIN(R1C4_BASIC_DVS)
	REGDEF_BIT(DVS_POS_CODE, 8)
	REGDEF_BIT(DVS_NEG_CODE, 8)
	REGDEF_BIT(DVS_NOCHG_CODE, 8)
	REGDEF_END(R1C4_BASIC_DVS)



/*
    YOUT_WIN_NUMX:    [0x0, 0x7f],            bits : 6_0
    YOUT_WIN_NUMY:    [0x0, 0x7f],            bits : 14_8
*/
#define R1C8_BASIC_DVS_OFS 0x01c8
REGDEF_BEGIN(R1C8_BASIC_DVS)
REGDEF_BIT(DVS_TIMESTAMP, 	   32)
REGDEF_END(R1C8_BASIC_DVS)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R1CC_DVS_OFS 0x01cc
REGDEF_BEGIN(R1CC_DVS)
REGDEF_BIT(DVS_OUT_W,		 14)
REGDEF_BIT(,		2)
REGDEF_BIT(DVS_OUT_H,		 14)
REGDEF_END(R1CC_DVS)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R1D0_RESERVED_OFS 0x01d0
REGDEF_BEGIN(R1D0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R1D0_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R1D4_RESERVED_OFS 0x01d4
REGDEF_BEGIN(R1D4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R1D4_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R1D8_RESERVED_OFS 0x01d8
REGDEF_BEGIN(R1D8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R1D8_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R1DC_RESERVED_OFS 0x01dc
REGDEF_BEGIN(R1DC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R1DC_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R1E0_ENGINE_STATUS_OFS 0x01e0
REGDEF_BEGIN(R1E0_ENGINE_STATUS)
REGDEF_BIT(SIE_VD_TIME,        32)
REGDEF_END(R1E0_ENGINE_STATUS)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R1E4_ENGINE_STATUS_OFS 0x01e4
REGDEF_BEGIN(R1E4_ENGINE_STATUS)
REGDEF_BIT(SIE_HD_TIME, 	   32)
REGDEF_END(R1E4_ENGINE_STATUS)

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
    LA_CG_RGAIN       :    [0x0, 0x3ff],            bits : 9_0
    LA_CG_GGAIN       :    [0x0, 0x3ff],            bits : 25_16
*/
#define R1EC_STCS_LA_CG_OFS 0x01ec
REGDEF_BEGIN(R1EC_STCS_LA_CG)
REGDEF_BIT(LA_CG_RGAIN,        10)
REGDEF_BIT(,         6)
REGDEF_BIT(LA_CG_GGAIN,        10)
REGDEF_END(R1EC_STCS_LA_CG)


/*
    LA_CG_BGAIN       :    [0x0, 0x3ff],            bits : 9_0
    LA_CG_IRGAIN      :    [0x0, 0x3ff],            bits : 25_16
*/
#define R1F0_STCS_LA_CG_OFS 0x01f0
REGDEF_BEGIN(R1F0_STCS_LA_CG)
REGDEF_BIT(LA_CG_BGAIN,        10)
REGDEF_BIT(,         6)
REGDEF_BIT(LA_CG_IRGAIN,        10)
REGDEF_END(R1F0_STCS_LA_CG)


/*
    VA_CG_GAIN0       :    [0x0, 0x3ff],            bits : 9_0
    VA_CG_GAIN1       :    [0x0, 0x3ff],            bits : 25_16
*/
#define R1F4_STCS_LA_IR_OFS 0x01f4
REGDEF_BEGIN(R1F4_STCS_LA_IR)
REGDEF_BIT(LA_IRSUB_RWET,         8)
REGDEF_BIT(LA_IRSUB_GWET,         8)
REGDEF_BIT(LA_IRSUB_BWET,         8)
REGDEF_END(R1F4_STCS_LA_IR)

/*
    VA_CG_GAIN2       :    [0x0, 0x3ff],            bits : 9_0
    VA_CG_GAIN3       :    [0x0, 0x3ff],            bits : 25_16
*/
#define R1F8_STCS_VA_CG_OFS 0x01f8
REGDEF_BEGIN(R1F8_STCS_VA_CG)
REGDEF_BIT(VA_CG_GAIN2,        10)
REGDEF_BIT(,         6)
REGDEF_BIT(VA_CG_GAIN3,        10)
REGDEF_END(R1F8_STCS_VA_CG)


/*
    VIG_X       :    [0x0, 0x3fff],            bits : 13_0
    VIG_Y       :    [0x0, 0x3fff],            bits : 29_16
    VIG_TAB_GAIN:    [0x0, 0x3],            bits : 31_30
*/
#define R1FC_STCS_VIG_OFS 0x01fc
REGDEF_BEGIN(R1FC_STCS_VIG)
REGDEF_BIT(VIG_X,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(VIG_Y,        14)
REGDEF_BIT(VIG_TAB_GAIN,         2)
REGDEF_END(R1FC_STCS_VIG)


/*
    VIG_T:    [0x0, 0x3ff],            bits : 9_0
*/
#define R200_STCS_VIG_OFS 0x0200
REGDEF_BEGIN(R200_STCS_VIG)
REGDEF_BIT(VIG_T,        10)
REGDEF_END(R200_STCS_VIG)


/*
    VIG_XDIV:    [0x0, 0xfff],            bits : 11_0
    VIG_YDIV:    [0x0, 0xfff],            bits : 27_16
*/
#define R204_STCS_VIG_OFS 0x0204
REGDEF_BEGIN(R204_STCS_VIG)
REGDEF_BIT(VIG_XDIV,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(VIG_YDIV,        12)
REGDEF_END(R204_STCS_VIG)


/*
    VIG_LUT00:    [0x0, 0x3ff],            bits : 9_0
    VIG_LUT01:    [0x0, 0x3ff],            bits : 19_10
    VIG_LUT02:    [0x0, 0x3ff],            bits : 29_20
*/
#define R208_STCS_VIG_OFS 0x0208
REGDEF_BEGIN(R208_STCS_VIG)
REGDEF_BIT(VIG_LUT00,        10)
REGDEF_BIT(VIG_LUT01,        10)
REGDEF_BIT(VIG_LUT02,        10)
REGDEF_END(R208_STCS_VIG)


/*
    VIG_LUT03:    [0x0, 0x3ff],            bits : 9_0
    VIG_LUT04:    [0x0, 0x3ff],            bits : 19_10
    VIG_LUT05:    [0x0, 0x3ff],            bits : 29_20
*/
#define R20C_STCS_VIG_OFS 0x020c
REGDEF_BEGIN(R20C_STCS_VIG)
REGDEF_BIT(VIG_LUT03,        10)
REGDEF_BIT(VIG_LUT04,        10)
REGDEF_BIT(VIG_LUT05,        10)
REGDEF_END(R20C_STCS_VIG)


/*
    VIG_LUT06:    [0x0, 0x3ff],            bits : 9_0
    VIG_LUT07:    [0x0, 0x3ff],            bits : 19_10
    VIG_LUT08:    [0x0, 0x3ff],            bits : 29_20
*/
#define R210_STCS_VIG_OFS 0x0210
REGDEF_BEGIN(R210_STCS_VIG)
REGDEF_BIT(VIG_LUT06,        10)
REGDEF_BIT(VIG_LUT07,        10)
REGDEF_BIT(VIG_LUT08,        10)
REGDEF_END(R210_STCS_VIG)


/*
    VIG_LUT09:    [0x0, 0x3ff],            bits : 9_0
    VIG_LUT10:    [0x0, 0x3ff],            bits : 19_10
    VIG_LUT11:    [0x0, 0x3ff],            bits : 29_20
*/
#define R214_STCS_VIG_OFS 0x0214
REGDEF_BEGIN(R214_STCS_VIG)
REGDEF_BIT(VIG_LUT09,        10)
REGDEF_BIT(VIG_LUT10,        10)
REGDEF_BIT(VIG_LUT11,        10)
REGDEF_END(R214_STCS_VIG)


/*
    VIG_LUT12:    [0x0, 0x3ff],            bits : 9_0
    VIG_LUT13:    [0x0, 0x3ff],            bits : 19_10
    VIG_LUT14:    [0x0, 0x3ff],            bits : 29_20
*/
#define R218_STCS_VIG_OFS 0x0218
REGDEF_BEGIN(R218_STCS_VIG)
REGDEF_BIT(VIG_LUT12,        10)
REGDEF_BIT(VIG_LUT13,        10)
REGDEF_BIT(VIG_LUT14,        10)
REGDEF_END(R218_STCS_VIG)


/*
    VIG_LUT15:    [0x0, 0x3ff],            bits : 9_0
    VIG_LUT16:    [0x0, 0x3ff],            bits : 19_10
*/
#define R21C_STCS_VIG_OFS 0x021c
REGDEF_BEGIN(R21C_STCS_VIG)
REGDEF_BIT(VIG_LUT15,        10)
REGDEF_BIT(VIG_LUT16,        10)
REGDEF_END(R21C_STCS_VIG)


/*
    CA_CROP_STX:    [0x0, 0x3fff],            bits : 13_0
    CA_CROP_STY:    [0x0, 0x3fff],            bits : 29_16
*/
#define R220_STCS_CA_OFS 0x0220
REGDEF_BEGIN(R220_STCS_CA)
REGDEF_BIT(CA_CROP_STX,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(CA_CROP_STY,        14)
REGDEF_END(R220_STCS_CA)


/*
    SMPL_X_FACT:    [0x0, 0x1fff],            bits : 12_0
    SMPL_Y_FACT:    [0x0, 0x1fff],            bits : 28_16
*/
#define R224_STCS_CA_OFS 0x0224
REGDEF_BEGIN(R224_STCS_CA)
REGDEF_BIT(SMPL_X_FACT,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(SMPL_Y_FACT,        14)
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
    CA_TH_G_LOWER:    [0x0, 0xfff],            bits : 11_0
    CA_TH_G_UPPER:    [0x0, 0xfff],            bits : 27_16
*/
#define R22C_STCS_CA_OFS 0x022c
REGDEF_BEGIN(R22C_STCS_CA)
REGDEF_BIT(CA_TH_G_LOWER,        12)
REGDEF_BIT(,         4)
REGDEF_BIT(CA_TH_G_UPPER,        12)
REGDEF_END(R22C_STCS_CA)


/*
    CA_TH_RG_LOWER:    [0x0, 0xfff],            bits : 11_0
    CA_TH_RG_UPPER:    [0x0, 0xfff],            bits : 27_16
*/
#define R230_STCS_CA_OFS 0x0230
REGDEF_BEGIN(R230_STCS_CA)
REGDEF_BIT(CA_TH_RG_LOWER,        12)
REGDEF_BIT(,         4)
REGDEF_BIT(CA_TH_RG_UPPER,        12)
REGDEF_END(R230_STCS_CA)


/*
    CA_TH_BG_LOWER:    [0x0, 0xfff],            bits : 11_0
    CA_TH_BG_UPPER:    [0x0, 0xfff],            bits : 27_16
*/
#define R234_STCS_CA_OFS 0x0234
REGDEF_BEGIN(R234_STCS_CA)
REGDEF_BIT(CA_TH_BG_LOWER,        12)
REGDEF_BIT(,         4)
REGDEF_BIT(CA_TH_BG_UPPER,        12)
REGDEF_END(R234_STCS_CA)


/*
    CA_TH_PG_LOWER:    [0x0, 0xfff],            bits : 11_0
    CA_TH_PG_UPPER:    [0x0, 0xfff],            bits : 27_16
*/
#define R238_STCS_CA_OFS 0x0238
REGDEF_BEGIN(R238_STCS_CA)
REGDEF_BIT(CA_TH_PG_LOWER,        12)
REGDEF_BIT(,         4)
REGDEF_BIT(CA_TH_PG_UPPER,        12)
REGDEF_END(R238_STCS_CA)


/*
    CA_WIN_SZX:    [0x0, 0xff],            bits : 7_0
    CA_WIN_SZY:    [0x0, 0xff],            bits : 15_8
*/
#define R23C_STCS_CA_OFS 0x023c
REGDEF_BEGIN(R23C_STCS_CA)
REGDEF_BIT(CA_WIN_SZX,        8)
REGDEF_BIT(CA_WIN_SZY,        8)
REGDEF_END(R23C_STCS_CA)


/*
    CA_WIN_NUMX:    [0x0, 0x1f],            bits : 4_0
    CA_WIN_NUMY:    [0x0, 0x1f],            bits : 12_8
*/
#define R240_STCS_CA_OFS 0x0240
REGDEF_BEGIN(R240_STCS_CA)
REGDEF_BIT(CA_WIN_NUMX,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(CA_WIN_NUMY,        5)
REGDEF_END(R240_STCS_CA)


/*
    CA_IRSUB_RWET:    [0x0, 0xff],            bits : 7_0
    CA_IRSUB_GWET:    [0x0, 0xff],            bits : 15_8
    CA_IRSUB_BWET:    [0x0, 0xff],            bits : 23_16
*/
#define R244_STCS_CA_IR_OFS 0x0244
REGDEF_BEGIN(R244_STCS_CA_IR)
REGDEF_BIT(CA_IRSUB_RWET,      8)
REGDEF_BIT(CA_IRSUB_GWET,      8)
REGDEF_BIT(CA_IRSUB_BWET,      8)
REGDEF_END(R244_STCS_CA_IR)


/*
    LA_CROP_STX:    [0x0, 0x7ff],            bits : 9_0
    LA_CROP_STY:    [0x0, 0x7ff],            bits : 19_10
*/
#define R248_STCS_LA_OFS 0x0248
REGDEF_BEGIN(R248_STCS_LA)
REGDEF_BIT(LA_CROP_STX,        11)
REGDEF_BIT(,         5)
REGDEF_BIT(LA_CROP_STY,        11)
REGDEF_END(R248_STCS_LA)


/*
    LA_CROP_SZX:    [0x0, 0x7ff],            bits : 10_0
    LA_CROP_SZY:    [0x0, 0x7ff],            bits : 26_16
    LA_RGB2Y_WET:   [0x0, 0xf],              bits : 31_28
*/
#define R24C_STCS_LA_OFS 0x024c
REGDEF_BEGIN(R24C_STCS_LA)
REGDEF_BIT(LA_CROP_SZX,        11)
REGDEF_BIT(,         5)
REGDEF_BIT(LA_CROP_SZY,        11)
REGDEF_BIT(,         1)
REGDEF_BIT(LA_RGB2Y_WET,         4)
REGDEF_END(R24C_STCS_LA)


/*
    STCS_GAMMA_1_TBL0:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL1:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL2:    [0x0, 0x3ff],            bits : 29_20
*/
#define R250_STCS_GAMMA_OFS 0x0250
REGDEF_BEGIN(R250_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL0,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL1,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL2,        10)
REGDEF_END(R250_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL3:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL4:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL5:    [0x0, 0x3ff],            bits : 29_20
*/
#define R254_STCS_GAMMA_OFS 0x0254
REGDEF_BEGIN(R254_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL3,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL4,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL5,        10)
REGDEF_END(R254_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL6:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL7:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL8:    [0x0, 0x3ff],            bits : 29_20
*/
#define R258_STCS_GAMMA_OFS 0x0258
REGDEF_BEGIN(R258_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL6,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL7,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL8,        10)
REGDEF_END(R258_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL9 :    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL10:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL11:    [0x0, 0x3ff],            bits : 29_20
*/
#define R25C_STCS_GAMMA_OFS 0x025c
REGDEF_BEGIN(R25C_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL9,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL10,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL11,        10)
REGDEF_END(R25C_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL12:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL13:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL14:    [0x0, 0x3ff],            bits : 29_20
*/
#define R260_STCS_GAMMA_OFS 0x0260
REGDEF_BEGIN(R260_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL12,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL13,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL14,        10)
REGDEF_END(R260_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL15:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL16:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL17:    [0x0, 0x3ff],            bits : 29_20
*/
#define R264_STCS_GAMMA_OFS 0x0264
REGDEF_BEGIN(R264_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL15,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL16,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL17,        10)
REGDEF_END(R264_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL18:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL19:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL20:    [0x0, 0x3ff],            bits : 29_20
*/
#define R268_STCS_GAMMA_OFS 0x0268
REGDEF_BEGIN(R268_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL18,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL19,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL20,        10)
REGDEF_END(R268_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL21:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL22:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL23:    [0x0, 0x3ff],            bits : 29_20
*/
#define R26C_STCS_GAMMA_OFS 0x026c
REGDEF_BEGIN(R26C_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL21,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL22,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL23,        10)
REGDEF_END(R26C_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL24:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL25:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL26:    [0x0, 0x3ff],            bits : 29_20
*/
#define R270_STCS_GAMMA_OFS 0x0270
REGDEF_BEGIN(R270_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL24,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL25,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL26,        10)
REGDEF_END(R270_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL27:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL28:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL29:    [0x0, 0x3ff],            bits : 29_20
*/
#define R274_STCS_GAMMA_OFS 0x0274
REGDEF_BEGIN(R274_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL27,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL28,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL29,        10)
REGDEF_END(R274_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL30:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL31:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL32:    [0x0, 0x3ff],            bits : 29_20
*/
#define R278_STCS_GAMMA_OFS 0x0278
REGDEF_BEGIN(R278_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL30,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL31,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL32,        10)
REGDEF_END(R278_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL33:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL34:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL35:    [0x0, 0x3ff],            bits : 29_20
*/
#define R27C_STCS_GAMMA_OFS 0x027c
REGDEF_BEGIN(R27C_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL33,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL34,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL35,        10)
REGDEF_END(R27C_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL36:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL37:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL38:    [0x0, 0x3ff],            bits : 29_20
*/
#define R280_STCS_GAMMA_OFS 0x0280
REGDEF_BEGIN(R280_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL36,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL37,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL38,        10)
REGDEF_END(R280_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL39:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL40:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL41:    [0x0, 0x3ff],            bits : 29_20
*/
#define R284_STCS_GAMMA_OFS 0x0284
REGDEF_BEGIN(R284_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL39,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL40,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL41,        10)
REGDEF_END(R284_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL42:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL43:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL44:    [0x0, 0x3ff],            bits : 29_20
*/
#define R288_STCS_GAMMA_OFS 0x0288
REGDEF_BEGIN(R288_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL42,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL43,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL44,        10)
REGDEF_END(R288_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL45:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL46:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL47:    [0x0, 0x3ff],            bits : 29_20
*/
#define R28C_STCS_GAMMA_OFS 0x028c
REGDEF_BEGIN(R28C_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL45,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL46,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL47,        10)
REGDEF_END(R28C_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL48:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL49:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL50:    [0x0, 0x3ff],            bits : 29_20
*/
#define R290_STCS_GAMMA_OFS 0x0290
REGDEF_BEGIN(R290_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL48,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL49,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL50,        10)
REGDEF_END(R290_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL51:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL52:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL53:    [0x0, 0x3ff],            bits : 29_20
*/
#define R294_STCS_GAMMA_OFS 0x0294
REGDEF_BEGIN(R294_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL51,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL52,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL53,        10)
REGDEF_END(R294_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL54:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL55:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL56:    [0x0, 0x3ff],            bits : 29_20
*/
#define R298_STCS_GAMMA_OFS 0x0298
REGDEF_BEGIN(R298_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL54,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL55,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL56,        10)
REGDEF_END(R298_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL57:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL58:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL59:    [0x0, 0x3ff],            bits : 29_20
*/
#define R29C_STCS_GAMMA_OFS 0x029c
REGDEF_BEGIN(R29C_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL57,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL58,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL59,        10)
REGDEF_END(R29C_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL60:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL61:    [0x0, 0x3ff],            bits : 19_10
    STCS_GAMMA_1_TBL62:    [0x0, 0x3ff],            bits : 29_20
*/
#define R2A0_STCS_GAMMA_OFS 0x02a0
REGDEF_BEGIN(R2A0_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL60,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL61,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL62,        10)
REGDEF_END(R2A0_STCS_GAMMA)


/*
    STCS_GAMMA_1_TBL63:    [0x0, 0x3ff],            bits : 9_0
    STCS_GAMMA_1_TBL64:    [0x0, 0x3ff],            bits : 19_10
*/
#define R2A4_STCS_GAMMA_OFS 0x02a4
REGDEF_BEGIN(R2A4_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_1_TBL63,        10)
REGDEF_BIT(STCS_GAMMA_1_TBL64,        10)
REGDEF_END(R2A4_STCS_GAMMA)


/*
    STCS_CA_OB_OFS:    [0x0, 0xFff],            bits : 11_0
    STCS_LA_OB_OFS:    [0x0, 0xFff],            bits : 23_12
*/
#define R2A8_STCS_OB_OFS 0x02a8
REGDEF_BEGIN(R2A8_RESERVED)
REGDEF_BIT(STCS_CA_OB_OFS,        12)
REGDEF_BIT(STCS_LA_OB_OFS,        12)
REGDEF_END(R2A8_STCS_OB)


/*
    LA_WIN_SZX:    [0x0, 0xff],            bits : 7_0
    LA_WIN_SZY:    [0x0, 0xff],            bits : 15_8
*/
#define R2AC_STCS_LA_OFS 0x02ac
REGDEF_BEGIN(R2AC_STCS_LA)
REGDEF_BIT(LA_WIN_SZX,        8)
REGDEF_BIT(LA_WIN_SZY,        8)
REGDEF_END(R2AC_STCS_LA)


/*
    LA_WIN_NUMX:    [0x0, 0x1f],            bits : 4_0
    LA_WIN_NUMY:    [0x0, 0x1f],            bits : 12_8
    LA1_WIN_SUM:    [0x0, 0x3ffff],            bits : 31_14
*/
#define R2B0_STCS_LA_OFS 0x02b0
REGDEF_BEGIN(R2B0_STCS_LA)
REGDEF_BIT(LA_WIN_NUMX,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(LA_WIN_NUMY,        5)
REGDEF_BIT(,        1)
REGDEF_BIT(LA1_WIN_SUM,        18)
REGDEF_END(R2B0_STCS_LA)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2B4_STCS_LA_OFS 0x02b4
REGDEF_BEGIN(R2B4_STCS_LA)
REGDEF_BIT(LA_TH_Y1_LOWER,        8)
REGDEF_BIT(LA_TH_Y1_UPPER,        8)
REGDEF_BIT(LA_TH_Y2_LOWER,        8)
REGDEF_BIT(LA_TH_Y2_UPPER,        8)
REGDEF_END(R2B4_STCS_LA)


/*
    HISTO_Y_RSLT_tmp0:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2B8_STCS_HISTO_OFS 0x02b8
REGDEF_BEGIN(R2B8_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp0,        32)
REGDEF_END(R2B8_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp1:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2BC_STCS_HISTO_OFS 0x02bc
REGDEF_BEGIN(R2BC_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp1,        32)
REGDEF_END(R2BC_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp2:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2C0_STCS_HISTO_OFS 0x02c0
REGDEF_BEGIN(R2C0_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp2,        32)
REGDEF_END(R2C0_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp3:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2C4_STCS_HISTO_OFS 0x02c4
REGDEF_BEGIN(R2C4_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp3,        32)
REGDEF_END(R2C4_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp4:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2C8_STCS_HISTO_OFS 0x02c8
REGDEF_BEGIN(R2C8_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp4,        32)
REGDEF_END(R2C8_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp5:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2CC_STCS_HISTO_OFS 0x02cc
REGDEF_BEGIN(R2CC_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp5,        32)
REGDEF_END(R2CC_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp6:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2D0_STCS_HISTO_OFS 0x02d0
REGDEF_BEGIN(R2D0_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp6,        32)
REGDEF_END(R2D0_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp7:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2D4_STCS_HISTO_OFS 0x02d4
REGDEF_BEGIN(R2D4_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp7,        32)
REGDEF_END(R2D4_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp8:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2D8_STCS_HISTO_OFS 0x02d8
REGDEF_BEGIN(R2D8_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp8,        32)
REGDEF_END(R2D8_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp9:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2DC_STCS_HISTO_OFS 0x02dc
REGDEF_BEGIN(R2DC_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp9,        32)
REGDEF_END(R2DC_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp10:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2E0_STCS_HISTO_OFS 0x02e0
REGDEF_BEGIN(R2E0_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp10,        32)
REGDEF_END(R2E0_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp11:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2E4_STCS_HISTO_OFS 0x02e4
REGDEF_BEGIN(R2E4_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp11,        32)
REGDEF_END(R2E4_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp12:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2E8_STCS_HISTO_OFS 0x02e8
REGDEF_BEGIN(R2E8_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp12,        32)
REGDEF_END(R2E8_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp13:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2EC_STCS_HISTO_OFS 0x02ec
REGDEF_BEGIN(R2EC_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp13,        32)
REGDEF_END(R2EC_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp14:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2F0_STCS_HISTO_OFS 0x02f0
REGDEF_BEGIN(R2F0_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp14,        32)
REGDEF_END(R2F0_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp15:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2F4_STCS_HISTO_OFS 0x02f4
REGDEF_BEGIN(R2F4_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp15,        32)
REGDEF_END(R2F4_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp16:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2F8_STCS_HISTO_OFS 0x02f8
REGDEF_BEGIN(R2F8_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp16,        32)
REGDEF_END(R2F8_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp17:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R2FC_STCS_HISTO_OFS 0x02fc
REGDEF_BEGIN(R2FC_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp17,        32)
REGDEF_END(R2FC_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp18:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R300_STCS_HISTO_OFS 0x0300
REGDEF_BEGIN(R300_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp18,        32)
REGDEF_END(R300_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp19:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R304_STCS_HISTO_OFS 0x0304
REGDEF_BEGIN(R304_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp19,        32)
REGDEF_END(R304_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp20:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R308_STCS_HISTO_OFS 0x0308
REGDEF_BEGIN(R308_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp20,        32)
REGDEF_END(R308_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp21:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R30C_STCS_HISTO_OFS 0x030c
REGDEF_BEGIN(R30C_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp21,        32)
REGDEF_END(R30C_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp22:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R310_STCS_HISTO_OFS 0x0310
REGDEF_BEGIN(R310_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp22,        32)
REGDEF_END(R310_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp23:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R314_STCS_HISTO_OFS 0x0314
REGDEF_BEGIN(R314_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp23,        32)
REGDEF_END(R314_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp24:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R318_STCS_HISTO_OFS 0x0318
REGDEF_BEGIN(R318_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp24,        32)
REGDEF_END(R318_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp25:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R31C_STCS_HISTO_OFS 0x031c
REGDEF_BEGIN(R31C_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp25,        32)
REGDEF_END(R31C_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp26:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R320_STCS_HISTO_OFS 0x0320
REGDEF_BEGIN(R320_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp26,        32)
REGDEF_END(R320_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp27:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R324_STCS_HISTO_OFS 0x0324
REGDEF_BEGIN(R324_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp27,        32)
REGDEF_END(R324_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp28:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R328_STCS_HISTO_OFS 0x0328
REGDEF_BEGIN(R328_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp28,        32)
REGDEF_END(R328_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp29:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R32C_STCS_HISTO_OFS 0x032c
REGDEF_BEGIN(R32C_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp29,        32)
REGDEF_END(R32C_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp30:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R330_STCS_HISTO_OFS 0x0330
REGDEF_BEGIN(R330_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp30,        32)
REGDEF_END(R330_STCS_HISTO)


/*
    HISTO_Y_RSLT_tmp31:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R334_STCS_HISTO_OFS 0x0334
REGDEF_BEGIN(R334_STCS_HISTO)
REGDEF_BIT(HISTO_Y_RSLT_tmp31,        32)
REGDEF_END(R334_STCS_HISTO)


/*
    VA_CROP_STX:    [0x0, 0x3fff],            bits : 13_0
    VA_CROP_STY:    [0x0, 0x3fff],            bits : 29_16
*/
#define R338_STCS_VA_OFS 0x0338
REGDEF_BEGIN(R338_STCS_VA)
REGDEF_BIT(VA_CROP_STX,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(VA_CROP_STY,        14)
REGDEF_END(R338_STCS_VA)


/*
    VA_CROP_SZX:    [0x0, 0x1fff],         bits : 13_0
    VA_RCCB2Y_SEL   [0x0, 0x1],            bits : 14
    VA_CROP_SZY:    [0x0, 0x1fff],         bits : 29_16
    VA_RGBIR2Y_SEL  [0x0, 0x3],            bits : 31_30
*/
#define R33C_STCS_VA_OFS 0x033c
REGDEF_BEGIN(R33C_STCS_VA)
REGDEF_BIT(VA_CROP_SZX,        14)
REGDEF_BIT(VA_RCCB2Y_SEL,       1)
REGDEF_BIT(,         1)
REGDEF_BIT(VA_CROP_SZY,        14)
REGDEF_BIT(VA_RGBIR2Y_SEL,      2)
REGDEF_END(R33C_STCS_VA)


/*
    LN_CNT      :    [0x0, 0x3fff],            bits : 13_0
    LN_CNT_MAX  :    [0x0, 0x3fff],            bits : 29_16
*/
#define R340_DEBUG_OFS 0x0340
REGDEF_BEGIN(R340_DBG)
REGDEF_BIT(LN_CNT,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(LN_CNT_MAX,		  14)
REGDEF_BIT(,		2)
REGDEF_END(R340_DEBUG)


/*
    VDETGV1A      :    [0x0, 0x1f],            bits : 4_0
    VDETGV1_BCD_OP:    [0x0, 0x1],            bits : 7
    VDETGV1B      :    [0x0, 0x1f],            bits : 12_8
    VDETGV1C      :    [0x0, 0xf],            bits : 19_16
    VDETGV1D      :    [0x0, 0xf],            bits : 23_20
    VDETGV1_FSIZE :    [0x0, 0x3],            bits : 25_24
    VDETGV1_DIV   :    [0x0, 0xf],            bits : 31_28
*/
#define R344_STCS_VA_OFS 0x0344
REGDEF_BEGIN(R344_STCS_VA)
REGDEF_BIT(VDETGV1A,        5)
REGDEF_BIT(,        2)
REGDEF_BIT(VDETGV1_BCD_OP,        1)
REGDEF_BIT(VDETGV1B,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(VDETGV1C,        4)
REGDEF_BIT(VDETGV1D,        4)
REGDEF_BIT(VDETGV1_FSIZE,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(VDETGV1_DIV,        4)
REGDEF_END(R344_STCS_VA)


/*
    VDETGH2A      :    [0x0, 0x1f],            bits : 4_0
    VDETGH2_BCD_OP:    [0x0, 0x1],            bits : 7
    VDETGH2B      :    [0x0, 0x1f],            bits : 12_8
    VDETGH2C      :    [0x0, 0xf],            bits : 19_16
    VDETGH2D      :    [0x0, 0xf],            bits : 23_20
    VDETGH2_FSIZE :    [0x0, 0x3],            bits : 25_24
    VDETGH2_EXTEND:    [0x0, 0x1],            bits : 26
    VDETGH2_DIV   :    [0x0, 0xf],            bits : 31_28
*/
#define R348_STCS_VA_OFS 0x0348
REGDEF_BEGIN(R348_STCS_VA)
REGDEF_BIT(VDETGH2A,        5)
REGDEF_BIT(,        2)
REGDEF_BIT(VDETGH2_BCD_OP,        1)
REGDEF_BIT(VDETGH2B,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(VDETGH2C,        4)
REGDEF_BIT(VDETGH2D,        4)
REGDEF_BIT(VDETGH2_FSIZE,        2)
REGDEF_BIT(VDETGH2_EXTEND,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(VDETGH2_DIV,        4)
REGDEF_END(R348_STCS_VA)


/*
    VDETGV2A      :    [0x0, 0x1f],            bits : 4_0
    VDETGV2_BCD_OP:    [0x0, 0x1],            bits : 7
    VDETGV2B      :    [0x0, 0x1f],            bits : 12_8
    VDETGV2C      :    [0x0, 0xf],            bits : 19_16
    VDETGV2D      :    [0x0, 0xf],            bits : 23_20
    VDETGV2_FSIZE :    [0x0, 0x3],            bits : 25_24
    VDETGV2_DIV   :    [0x0, 0xf],            bits : 31_28
*/
#define R34C_STCS_VA_OFS 0x034c
REGDEF_BEGIN(R34C_STCS_VA)
REGDEF_BIT(VDETGV2A,        5)
REGDEF_BIT(,        2)
REGDEF_BIT(VDETGV2_BCD_OP,        1)
REGDEF_BIT(VDETGV2B,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(VDETGV2C,        4)
REGDEF_BIT(VDETGV2D,        4)
REGDEF_BIT(VDETGV2_FSIZE,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(VDETGV2_DIV,        4)
REGDEF_END(R34C_STCS_VA)


/*
    VACC_OUTSEL:    [0x0, 0x1],            bits : 0
*/
#define R350_STCS_VA_OFS 0x0350
REGDEF_BEGIN(R350_STCS_VA)
REGDEF_BIT(VACC_OUTSEL,        1)
REGDEF_END(R350_STCS_VA)


/*
    VA_G1HTHL:    [0x0, 0xff],            bits : 7_0
    VA_G1HTHH:    [0x0, 0xff],            bits : 15_8
    VA_G1VTHL:    [0x0, 0xff],            bits : 23_16
    VA_G1VTHH:    [0x0, 0xff],            bits : 31_24
*/
#define R354_STCS_VA_OFS 0x0354
REGDEF_BEGIN(R354_STCS_VA)
REGDEF_BIT(VA_G1HTHL,        8)
REGDEF_BIT(VA_G1HTHH,        8)
REGDEF_BIT(VA_G1VTHL,        8)
REGDEF_BIT(VA_G1VTHH,        8)
REGDEF_END(R354_STCS_VA)


/*
    VA_G2HTHL:    [0x0, 0xff],            bits : 7_0
    VA_G2HTHH:    [0x0, 0xff],            bits : 15_8
    VA_G2VTHL:    [0x0, 0xff],            bits : 23_16
    VA_G2VTHH:    [0x0, 0xff],            bits : 31_24
*/
#define R358_STCS_VA_OFS 0x0358
REGDEF_BEGIN(R358_STCS_VA)
REGDEF_BIT(VA_G2HTHL,        8)
REGDEF_BIT(VA_G2HTHH,        8)
REGDEF_BIT(VA_G2VTHL,        8)
REGDEF_BIT(VA_G2VTHH,        8)
REGDEF_END(R358_STCS_VA)


/*
    VA_WIN_SZX         :    [0x0, 0xff],            bits : 7_0
    VA_WIN_SZY         :    [0x0, 0xff],            bits : 15_8
    VA_G1_LINE_MAX_MODE:    [0x0, 0x1],            bits : 16
    VA_G2_LINE_MAX_MODE:    [0x0, 0x1],            bits : 17
*/
#define R35C_STCS_VA_OFS 0x035c
REGDEF_BEGIN(R35C_STCS_VA)
REGDEF_BIT(VA_WIN_SZX,        8)
REGDEF_BIT(VA_WIN_SZY,        8)
REGDEF_BIT(VA_G1_LINE_MAX_MODE,        1)
REGDEF_BIT(VA_G2_LINE_MAX_MODE,        1)
REGDEF_END(R35C_STCS_VA)


/*
    VA_WIN_NUMX :    [0x0, 0xf],            bits : 3_0
    VA_WIN_NUMY :    [0x0, 0xf],            bits : 7_4
    VA_WIN_SKIPX:    [0x0, 0x3f],            bits : 21_16
    VA_WIN_SKIPY:    [0x0, 0x3f],            bits : 29_24
*/
#define R360_STCS_VA_OFS 0x0360
REGDEF_BEGIN(R360_STCS_VA)
REGDEF_BIT(VA_WIN_NUMX,        4)
REGDEF_BIT(VA_WIN_NUMY,        4)
REGDEF_BIT(,        8)
REGDEF_BIT(VA_WIN_SKIPX,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(VA_WIN_SKIPY,        6)
REGDEF_END(R360_STCS_VA)


/*
    CSI_HD_CNT     :    [0x0, 0xFFFF],            bits : 15_0
    CSI_LN_END_CNT :    [0x0, 0xFFFF],            bits : 31_16
*/
#define R364_DEBUG_OFS 0x0364
REGDEF_BEGIN(R364_DEBUG)
REGDEF_BIT(CSI_HD_CNT,        16)
REGDEF_BIT(CSI_LN_END_CNT,    16)
REGDEF_END(R364_DEBUG)


/*
    CSI_PXL_CNT:    [0x0, 0xFFFF],            bits : 15_0
*/
#define R368_DEBUG_OFS 0x0368
REGDEF_BEGIN(R368_DEBUG)
REGDEF_BIT(CSI_PXL_CNT,        16)
REGDEF_END(R368_DEBUG)


/*
    WIN1_VAEN:    [0x0, 0x1],            bits : 0
    WIN1_STX :    [0x0, 0xfff],            bits : 13_2
    WIN1_STY :    [0x0, 0xfff],            bits : 27_16
*/
#define R36C_STCS_VA_OFS 0x036c
REGDEF_BEGIN(R36C_STCS_VA)
REGDEF_BIT(WIN1_VAEN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(WIN1_STX,        12)
REGDEF_BIT(,        2)
REGDEF_BIT(WIN1_STY,        12)
REGDEF_END(R36C_STCS_VA)


/*
    WIN1_G1_LINE_MAX_MODE:    [0x0, 0x1],            bits : 0
    WIN1_G2_LINE_MAX_MODE:    [0x0, 0x1],            bits : 1
    WIN1_HSZ             :    [0x0, 0x1ff],            bits : 12_4
    WIN1_VSZ             :    [0x0, 0x1ff],            bits : 24_16
*/
#define R370_STCS_VA_OFS 0x0370
REGDEF_BEGIN(R370_STCS_VA)
REGDEF_BIT(WIN1_G1_LINE_MAX_MODE,        1)
REGDEF_BIT(WIN1_G2_LINE_MAX_MODE,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(WIN1_HSZ,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(WIN1_VSZ,        9)
REGDEF_END(R370_STCS_VA)


/*
    WIN2_VAEN:    [0x0, 0x1],            bits : 0
    WIN2_STX :    [0x0, 0xfff],            bits : 13_2
    WIN2_STY :    [0x0, 0xfff],            bits : 27_16
*/
#define R374_STCS_VA_OFS 0x0374
REGDEF_BEGIN(R374_STCS_VA)
REGDEF_BIT(WIN2_VAEN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(WIN2_STX,        12)
REGDEF_BIT(,        2)
REGDEF_BIT(WIN2_STY,        12)
REGDEF_END(R374_STCS_VA)


/*
    WIN2_G1_LINE_MAX_MODE:    [0x0, 0x1],            bits : 0
    WIN2_G2_LINE_MAX_MODE:    [0x0, 0x1],            bits : 1
    WIN2_HSZ             :    [0x0, 0x1ff],            bits : 12_4
    WIN2_VSZ             :    [0x0, 0x1ff],            bits : 24_16
*/
#define R378_STCS_VA_OFS 0x0378
REGDEF_BEGIN(R378_STCS_VA)
REGDEF_BIT(WIN2_G1_LINE_MAX_MODE,        1)
REGDEF_BIT(WIN2_G2_LINE_MAX_MODE,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(WIN2_HSZ,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(WIN2_VSZ,        9)
REGDEF_END(R378_STCS_VA)


/*
    WIN3_VAEN:    [0x0, 0x1],            bits : 0
    WIN3_STX :    [0x0, 0xfff],            bits : 13_2
    WIN3_STY :    [0x0, 0xfff],            bits : 27_16
*/
#define R37C_STCS_VA_OFS 0x037c
REGDEF_BEGIN(R37C_STCS_VA)
REGDEF_BIT(WIN3_VAEN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(WIN3_STX,        12)
REGDEF_BIT(,        2)
REGDEF_BIT(WIN3_STY,        12)
REGDEF_END(R37C_STCS_VA)


/*
    WIN3_G1_LINE_MAX_MODE:    [0x0, 0x1],            bits : 0
    WIN3_G2_LINE_MAX_MODE:    [0x0, 0x1],            bits : 1
    WIN3_HSZ             :    [0x0, 0x1ff],            bits : 12_4
    WIN3_VSZ             :    [0x0, 0x1ff],            bits : 24_16
*/
#define R380_STCS_VA_OFS 0x0380
REGDEF_BEGIN(R380_STCS_VA)
REGDEF_BIT(WIN3_G1_LINE_MAX_MODE,        1)
REGDEF_BIT(WIN3_G2_LINE_MAX_MODE,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(WIN3_HSZ,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(WIN3_VSZ,        9)
REGDEF_END(R380_STCS_VA)


/*
    WIN4_VAEN:    [0x0, 0x1],            bits : 0
    WIN4_STX :    [0x0, 0xfff],            bits : 13_2
    WIN4_STY :    [0x0, 0xfff],            bits : 27_16
*/
#define R384_STCS_VA_OFS 0x0384
REGDEF_BEGIN(R384_STCS_VA)
REGDEF_BIT(WIN4_VAEN,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(WIN4_STX,        12)
REGDEF_BIT(,        2)
REGDEF_BIT(WIN4_STY,        12)
REGDEF_END(R384_STCS_VA)


/*
    WIN4_G1_LINE_MAX_MODE:    [0x0, 0x1],            bits : 0
    WIN4_G2_LINE_MAX_MODE:    [0x0, 0x1],            bits : 1
    WIN4_HSZ             :    [0x0, 0x1ff],            bits : 12_4
    WIN4_VSZ             :    [0x0, 0x1ff],            bits : 24_16
*/
#define R388_STCS_VA_OFS 0x0388
REGDEF_BEGIN(R388_STCS_VA)
REGDEF_BIT(WIN4_G1_LINE_MAX_MODE,        1)
REGDEF_BIT(WIN4_G2_LINE_MAX_MODE,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(WIN4_HSZ,        9)
REGDEF_BIT(,        3)
REGDEF_BIT(WIN4_VSZ,        9)
REGDEF_END(R388_STCS_VA)


/*
    VA_WIN0G1H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN0G1V:    [0x0, 0xffff],            bits : 31_16
*/
#define R38C_STCS_VA_OFS 0x038c
REGDEF_BEGIN(R38C_STCS_VA)
REGDEF_BIT(VA_WIN0G1H,        16)
REGDEF_BIT(VA_WIN0G1V,        16)
REGDEF_END(R38C_STCS_VA)


/*
    VA_WIN0G2H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN0G2V:    [0x0, 0xffff],            bits : 31_16
*/
#define R390_STCS_VA_OFS 0x0390
REGDEF_BEGIN(R390_STCS_VA)
REGDEF_BIT(VA_WIN0G2H,        16)
REGDEF_BIT(VA_WIN0G2V,        16)
REGDEF_END(R390_STCS_VA)


/*
    VA_WIN1G1H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN1G1V:    [0x0, 0xffff],            bits : 31_16
*/
#define R394_STCS_VA_OFS 0x0394
REGDEF_BEGIN(R394_STCS_VA)
REGDEF_BIT(VA_WIN1G1H,        16)
REGDEF_BIT(VA_WIN1G1V,        16)
REGDEF_END(R394_STCS_VA)


/*
    VA_WIN1G2H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN1G2V:    [0x0, 0xffff],            bits : 31_16
*/
#define R398_STCS_VA_OFS 0x0398
REGDEF_BEGIN(R398_STCS_VA)
REGDEF_BIT(VA_WIN1G2H,        16)
REGDEF_BIT(VA_WIN1G2V,        16)
REGDEF_END(R398_STCS_VA)


/*
    VA_WIN2G1H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN2G1V:    [0x0, 0xffff],            bits : 31_16
*/
#define R39C_STCS_VA_OFS 0x039c
REGDEF_BEGIN(R39C_STCS_VA)
REGDEF_BIT(VA_WIN2G1H,        16)
REGDEF_BIT(VA_WIN2G1V,        16)
REGDEF_END(R39C_STCS_VA)


/*
    VA_WIN2G2H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN2G2V:    [0x0, 0xffff],            bits : 31_16
*/
#define R3A0_STCS_VA_OFS 0x03a0
REGDEF_BEGIN(R3A0_STCS_VA)
REGDEF_BIT(VA_WIN2G2H,        16)
REGDEF_BIT(VA_WIN2G2V,        16)
REGDEF_END(R3A0_STCS_VA)


/*
    VA_WIN3G1H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN3G1V:    [0x0, 0xffff],            bits : 31_16
*/
#define R3A4_STCS_VA_OFS 0x03a4
REGDEF_BEGIN(R3A4_STCS_VA)
REGDEF_BIT(VA_WIN3G1H,        16)
REGDEF_BIT(VA_WIN3G1V,        16)
REGDEF_END(R3A4_STCS_VA)


/*
    VA_WIN3G2H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN3G2V:    [0x0, 0xffff],            bits : 31_16
*/
#define R3A8_STCS_VA_OFS 0x03a8
REGDEF_BEGIN(R3A8_STCS_VA)
REGDEF_BIT(VA_WIN3G2H,        16)
REGDEF_BIT(VA_WIN3G2V,        16)
REGDEF_END(R3A8_STCS_VA)


/*
    VA_WIN4G1H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN4G1V:    [0x0, 0xffff],            bits : 31_16
*/
#define R3AC_STCS_VA_OFS 0x03ac
REGDEF_BEGIN(R3AC_STCS_VA)
REGDEF_BIT(VA_WIN4G1H,        16)
REGDEF_BIT(VA_WIN4G1V,        16)
REGDEF_END(R3AC_STCS_VA)


/*
    VA_WIN4G2H:    [0x0, 0xffff],            bits : 15_0
    VA_WIN4G2V:    [0x0, 0xffff],            bits : 31_16
*/
#define R3B0_STCS_VA_OFS 0x03b0
REGDEF_BEGIN(R3B0_STCS_VA)
REGDEF_BIT(VA_WIN4G2H,        16)
REGDEF_BIT(VA_WIN4G2V,        16)
REGDEF_END(R3B0_STCS_VA)


/*
    ETHLOW :    [0x0, 0x3ff],            bits : 11_2
    ETHMID :    [0x0, 0x3ff],            bits : 21_12
    ETHHIGH:    [0x0, 0x3ff],            bits : 31_22
*/
#define R3B4_STCS_ETH_OFS 0x03b4
REGDEF_BEGIN(R3B4_STCS_ETH)
REGDEF_BIT(,        2)
REGDEF_BIT(ETHLOW,        10)
REGDEF_BIT(ETHMID,        10)
REGDEF_BIT(ETHHIGH,        10)
REGDEF_END(R3B4_STCS_ETH)


/*
    STCS_GAMMA_2_TBL0:    [0x0, 0xff],            bits : 7_0
    STCS_GAMMA_2_TBL1:    [0x0, 0xff],            bits : 15_8
    STCS_GAMMA_2_TBL2:    [0x0, 0xff],            bits : 23_16
    STCS_GAMMA_2_TBL3:    [0x0, 0xff],            bits : 31_24
*/
#define R3B8_STCS_GAMMA_OFS 0x03b8
REGDEF_BEGIN(R3B8_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_2_TBL0,        8)
REGDEF_BIT(STCS_GAMMA_2_TBL1,        8)
REGDEF_BIT(STCS_GAMMA_2_TBL2,        8)
REGDEF_BIT(STCS_GAMMA_2_TBL3,        8)
REGDEF_END(R3B8_STCS_GAMMA)


/*
    STCS_GAMMA_2_TBL4:    [0x0, 0xff],            bits : 7_0
    STCS_GAMMA_2_TBL5:    [0x0, 0xff],            bits : 15_8
    STCS_GAMMA_2_TBL6:    [0x0, 0xff],            bits : 23_16
    STCS_GAMMA_2_TBL7:    [0x0, 0xff],            bits : 31_24
*/
#define R3BC_STCS_GAMMA_OFS 0x03bc
REGDEF_BEGIN(R3BC_STCS_GAMMA)
REGDEF_BIT(STCS_GAMMA_2_TBL4,        8)
REGDEF_BIT(STCS_GAMMA_2_TBL5,        8)
REGDEF_BIT(STCS_GAMMA_2_TBL6,        8)
REGDEF_BIT(STCS_GAMMA_2_TBL7,        8)
REGDEF_END(R3BC_STCS_GAMMA)


/*
    STCS_GAMMA_2_TBL8 :    [0x0, 0xff],            bits : 7_0
    STCS_GAMMA_2_TBL9 :    [0x0, 0xff],            bits : 15_8
    STCS_GAMMA_2_TBL10:    [0x0, 0xff],            bits : 23_16
    STCS_GAMMA_2_TBL11:    [0x0, 0xff],            bits : 31_24
*/
#define R3C0_STCS_MD_OFS 0x03c0
REGDEF_BEGIN(R3C0_STCS_MD)
REGDEF_BIT(MD_Y_SEL,           1)
REGDEF_BIT(,		3)
REGDEF_BIT(MD_SUM_FRMS,        4)
REGDEF_BIT(MD_BLKDIFF_THR,    23)
REGDEF_END(R3C0_STCS_MD)


/*
    STCS_GAMMA_2_TBL12:    [0x0, 0xff],            bits : 7_0
    STCS_GAMMA_2_TBL13:    [0x0, 0xff],            bits : 15_8
    STCS_GAMMA_2_TBL14:    [0x0, 0xff],            bits : 23_16
    STCS_GAMMA_2_TBL15:    [0x0, 0xff],            bits : 31_24
*/
#define R3C4_STCS_MD_OFS 0x03c4
REGDEF_BEGIN(R3C4_STCS_MD)
REGDEF_BIT(MD_TOTAL_BLKDIFF_THR,        32)
REGDEF_END(R3C4_STCS_MD)


/*
    STCS_GAMMA_2_TBL16:    [0x0, 0xff],            bits : 7_0
    STCS_GAMMA_2_TBL17:    [0x0, 0xff],            bits : 15_8
    STCS_GAMMA_2_TBL18:    [0x0, 0xff],            bits : 23_16
    STCS_GAMMA_2_TBL19:    [0x0, 0xff],            bits : 31_24
*/
#define R3C8_STCS_MD_OFS 0x03c8
REGDEF_BEGIN(R3C8_STCS_MD)
REGDEF_BIT(MD_BLKDIFF_CNT,		      11)
REGDEF_BIT(,        1)
REGDEF_BIT(MD_BLKDIFF_CNT_THR,        10)
REGDEF_END(R3C8_STCS_MD)


/*
    STCS_GAMMA_2_TBL20:    [0x0, 0xff],            bits : 7_0
    STCS_GAMMA_2_TBL21:    [0x0, 0xff],            bits : 15_8
    STCS_GAMMA_2_TBL22:    [0x0, 0xff],            bits : 23_16
    STCS_GAMMA_2_TBL23:    [0x0, 0xff],            bits : 31_24
*/
#define R3CC_STCS_MD_OFS 0x03cc
REGDEF_BEGIN(R3CC_STCS_MD)
REGDEF_BIT(MD_TOTAL_BLKDIFF,        32)
REGDEF_END(R3CC_STCS_MD)


/*
    STCS_GAMMA_2_TBL24:    [0x0, 0xff],            bits : 7_0
    STCS_GAMMA_2_TBL25:    [0x0, 0xff],            bits : 15_8
    STCS_GAMMA_2_TBL26:    [0x0, 0xff],            bits : 23_16
    STCS_GAMMA_2_TBL27:    [0x0, 0xff],            bits : 31_24
*/
#define R3D0_STCS_MD_OFS 0x03d0
REGDEF_BEGIN(R3D0_STCS_MD)
REGDEF_BIT(MD_VALID_REG_MODE,        1)
REGDEF_END(R3D0_STCS_MD)


/*
    STCS_GAMMA_2_TBL28:    [0x0, 0xff],            bits : 7_0
    STCS_GAMMA_2_TBL29:    [0x0, 0xff],            bits : 15_8
    STCS_GAMMA_2_TBL30:    [0x0, 0xff],            bits : 23_16
    STCS_GAMMA_2_TBL31:    [0x0, 0xff],            bits : 31_24
*/
#define R3D4_STCS_MD_OFS 0x03d4
REGDEF_BEGIN(R3D4_STCS_MD)
REGDEF_BIT(MD_VALID_REG0_31,        32)
REGDEF_END(R3D4_STCS_MD)


/*
    STCS_GAMMA_2_TBL32:    [0x0, 0xff],            bits : 7_0
*/
#define R3D8_STCS_MD_OFS 0x03d8
REGDEF_BEGIN(R3D8_STCS_MD)
REGDEF_BIT(MD_VALID_REG32_63,        32)
REGDEF_END(R3D8_STCS_MD)


/*
    ETH_OUTFMT  :    [0x0, 0x1],            bits : 0
    ETH_OUTSEL  :    [0x0, 0x1],            bits : 1
    ETH_HOUT_SEL:    [0x0, 0x1],            bits : 2
    ETH_VOUT_SEL:    [0x0, 0x1],            bits : 3
    PFPC_MODE   :    [0x0, 0x1],            bits : 4
*/
#define R3DC_RESERVED_OFS 0x03dc
REGDEF_BEGIN(R3DC_RESERVED)
REGDEF_BIT(RESERVED,		32)
REGDEF_END(R3DC_RESERVED)


/*
    PFPC_TBL_GAIN0_3:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R3E0_RESERVED_OFS 0x03e0
REGDEF_BEGIN(R3E0_RESERVED)
REGDEF_BIT(RESERVED,		32)
REGDEF_END(R3E0_RESERVED)


/*
    PFPC_TBL_GAIN4_7:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R3E4_MD_OFS 0x03e4
REGDEF_BEGIN(R3E4_MD)
REGDEF_BIT(MD_THRES_RSLT_REG0_31,        32)
REGDEF_END(R3E4_MD)


/*
    PFPC_TBL_GAIN8_11:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R3E8_MD_OFS 0x03e8
REGDEF_BEGIN(R3E8_MD)
REGDEF_BIT(MD_THRES_RSLT_REG32_63,		 32)
REGDEF_END(R3E8_MD)


/*
    PFPC_TBL_GAIN12_15:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R3EC_MD_OFS 0x03ec
REGDEF_BEGIN(R3EC_MD)
REGDEF_BIT(MD_THRES_RSLT_REG64_95,		 32)
REGDEF_END(R3EC_MD)


/*
    PFPC_TBL_GAIN16_19:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R3F0_ADVANCED_PFPC_OFS 0x03f0
REGDEF_BEGIN(R3F0_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_GAIN16_19,        32)
REGDEF_END(R3F0_ADVANCED_PFPC)


/*
    PFPC_TBL_GAIN20_23:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R3F4_ADVANCED_PFPC_OFS 0x03f4
REGDEF_BEGIN(R3F4_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_GAIN20_23,        32)
REGDEF_END(R3F4_ADVANCED_PFPC)


/*
    PFPC_TBL_GAIN24_27:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R3F8_ADVANCED_PFPC_OFS 0x03f8
REGDEF_BEGIN(R3F8_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_GAIN24_27,        32)
REGDEF_END(R3F8_ADVANCED_PFPC)


/*
    PFPC_TBL_GAIN28_31:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R3FC_ADVANCED_PFPC_OFS 0x03fc
REGDEF_BEGIN(R3FC_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_GAIN28_31,        32)
REGDEF_END(R3FC_ADVANCED_PFPC)


/*
    PFPC_TBL_OFS0_3:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R400_ADVANCED_PFPC_OFS 0x0400
REGDEF_BEGIN(R400_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_OFS0_3,        32)
REGDEF_END(R400_ADVANCED_PFPC)


/*
    PFPC_TBL_OFS4_7:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R404_ADVANCED_PFPC_OFS 0x0404
REGDEF_BEGIN(R404_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_OFS4_7,        32)
REGDEF_END(R404_ADVANCED_PFPC)


/*
    PFPC_TBL_OFS8_11:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R408_ADVANCED_PFPC_OFS 0x0408
REGDEF_BEGIN(R408_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_OFS8_11,        32)
REGDEF_END(R408_ADVANCED_PFPC)


/*
    PFPC_TBL_OFS12_15:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R40C_ADVANCED_PFPC_OFS 0x040c
REGDEF_BEGIN(R40C_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_OFS12_15,        32)
REGDEF_END(R40C_ADVANCED_PFPC)


/*
    PFPC_TBL_OFS16_19:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R410_ADVANCED_PFPC_OFS 0x0410
REGDEF_BEGIN(R410_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_OFS16_19,        32)
REGDEF_END(R410_ADVANCED_PFPC)


/*
    PFPC_TBL_OFS20_23:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R414_ADVANCED_PFPC_OFS 0x0414
REGDEF_BEGIN(R414_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_OFS20_23,        32)
REGDEF_END(R414_ADVANCED_PFPC)


/*
    PFPC_TBL_OFS24_27:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R418_ADVANCED_PFPC_OFS 0x0418
REGDEF_BEGIN(R418_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_OFS24_27,        32)
REGDEF_END(R418_ADVANCED_PFPC)


/*
    PFPC_TBL_OFS28_31:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R41C_ADVANCED_PFPC_OFS 0x041c
REGDEF_BEGIN(R41C_ADVANCED_PFPC)
REGDEF_BIT(PFPC_TBL_OFS28_31,        32)
REGDEF_END(R41C_ADVANCED_PFPC)


/*
    OBF_TOP_X:    [0x0, 0x3fff],            bits : 13_0
    OBF_TOP_Y:    [0x0, 0xffff],            bits : 31_16
*/
#define R420_ADVANCED_OBF_OFS 0x0420
REGDEF_BEGIN(R420_ADVANCED_OBF)
REGDEF_BIT(OBF_TOP_X,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(OBF_TOP_Y,        14)
REGDEF_END(R420_ADVANCED_OBF)


/*
    OBF_TOP_INTVL:    [0x0, 0x3ff],            bits : 9_0
    OBF_THRES    :    [0x0, 0xfff],            bits : 21_10
    OBF_NF       :    [0x0, 0x3],            bits : 23_22
*/
#define R424_ADVANCED_OBF_OFS 0x0424
REGDEF_BEGIN(R424_ADVANCED_OBF)
REGDEF_BIT(OBF_TOP_INTVL,        10)
REGDEF_BIT(OBF_THRES,        12)
REGDEF_BIT(OBF_NF,        2)
REGDEF_END(R424_ADVANCED_OBF)


/*
    OBF_LEFT_X:    [0x0, 0x3fff],            bits : 13_0
    OBF_LEFT_Y:    [0x0, 0xffff],            bits : 31_16
*/
#define R428_ADVANCED_OBF_OFS 0x0428
REGDEF_BEGIN(R428_ADVANCED_OBF)
REGDEF_BIT(OBF_LEFT_X,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(OBF_LEFT_Y,        14)
REGDEF_END(R428_ADVANCED_OBF)


/*
    OBF_LEFT_INTVL:    [0x0, 0x3ff],            bits : 9_0
*/
#define R42C_ADVANCED_OBF_OFS 0x042c
REGDEF_BEGIN(R42C_ADVANCED_OBF)
REGDEF_BIT(OBF_LEFT_INTVL,        10)
REGDEF_END(R42C_ADVANCED_OBF)


/*
    OBF_BOT_Y  :    [0x0, 0x1fff],            bits : 13_0
    OBF_RIGHT_X:    [0x0, 0x3fff],            bits : 29_16
*/
#define R430_ADVANCED_OBF_OFS 0x0430
REGDEF_BEGIN(R430_ADVANCED_OBF)
REGDEF_BIT(OBF_BOT_Y,        14)
REGDEF_BIT(,         2)
REGDEF_BIT(OBF_RIGHT_X,        14)
REGDEF_END(R430_ADVANCED_OBF)


/*
    OBF_CNT:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R434_ADVANCED_OBF_OFS 0x0434
REGDEF_BEGIN(R434_ADVANCED_OBF)
REGDEF_BIT(OBF_CNT,        32)
REGDEF_END(R434_ADVANCED_OBF)


/*
    OBF_TOP0_3:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R438_ADVANCED_OBF_OFS 0x0438
REGDEF_BEGIN(R438_ADVANCED_OBF)
REGDEF_BIT(OBF_TOP0_3,        32)
REGDEF_END(R438_ADVANCED_OBF)


/*
    OBF_TOP4_7:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R43C_ADVANCED_OBF_OFS 0x043c
REGDEF_BEGIN(R43C_ADVANCED_OBF)
REGDEF_BIT(OBF_TOP4_7,        32)
REGDEF_END(R43C_ADVANCED_OBF)


/*
    OBF_LEFT0_3:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R440_ADVANCED_OBF_OFS 0x0440
REGDEF_BEGIN(R440_ADVANCED_OBF)
REGDEF_BIT(OBF_LEFT0_3,        32)
REGDEF_END(R440_ADVANCED_OBF)


/*
    OBF_LEFT4_7:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R444_ADVANCED_OBF_OFS 0x0444
REGDEF_BEGIN(R444_ADVANCED_OBF)
REGDEF_BIT(OBF_LEFT4_7,        32)
REGDEF_END(R444_ADVANCED_OBF)


/*
    OBF_BOT0_3:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R448_ADVANCED_OBF_OFS 0x0448
REGDEF_BEGIN(R448_ADVANCED_OBF)
REGDEF_BIT(OBF_BOT0_3,        32)
REGDEF_END(R448_ADVANCED_OBF)


/*
    OBF_BOT4_7:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R44C_ADVANCED_OBF_OFS 0x044c
REGDEF_BEGIN(R44C_ADVANCED_OBF)
REGDEF_BIT(OBF_BOT4_7,        32)
REGDEF_END(R44C_ADVANCED_OBF)


/*
    OBF_RIGHT0_3:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R450_ADVANCED_OBF_OFS 0x0450
REGDEF_BEGIN(R450_ADVANCED_OBF)
REGDEF_BIT(OBF_RIGHT0_3,        32)
REGDEF_END(R450_ADVANCED_OBF)


/*
    OBF_RIGHT4_7:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R454_ADVANCED_OBF_OFS 0x0454
REGDEF_BEGIN(R454_ADVANCED_OBF)
REGDEF_BIT(OBF_RIGHT4_7,        32)
REGDEF_END(R454_ADVANCED_OBF)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R458_RESERVED_OFS 0x0458
REGDEF_BEGIN(R458_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R458_RESERVED)


/*
    OBP_HSCL :    [0x0, 0x3fff],            bits : 13_0
    OBP_VSCL :    [0x0, 0x3fff],            bits : 29_16
    OBP_SHIFT:    [0x0, 0x3],            bits : 31_30
*/
#define R45C_ADVANCED_OBP_OFS 0x045c
REGDEF_BEGIN(R45C_ADVANCED_OBP)
REGDEF_BIT(OBP_HSCL,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(OBP_VSCL,        14)
REGDEF_BIT(OBP_SHIFT,        2)
REGDEF_END(R45C_ADVANCED_OBP)


/*
    OBP_TBL_tmp0:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R460_ADVANCED_OBP_OFS 0x0460
REGDEF_BEGIN(R460_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp0,        32)
REGDEF_END(R460_ADVANCED_OBP)


/*
    OBP_TBL_tmp1:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R464_ADVANCED_OBP_OFS 0x0464
REGDEF_BEGIN(R464_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp1,        32)
REGDEF_END(R464_ADVANCED_OBP)


/*
    OBP_TBL_tmp2:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R468_ADVANCED_OBP_OFS 0x0468
REGDEF_BEGIN(R468_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp2,        32)
REGDEF_END(R468_ADVANCED_OBP)


/*
    OBP_TBL_tmp3:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R46C_ADVANCED_OBP_OFS 0x046c
REGDEF_BEGIN(R46C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp3,        32)
REGDEF_END(R46C_ADVANCED_OBP)


/*
    OBP_TBL_tmp4:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R470_ADVANCED_OBP_OFS 0x0470
REGDEF_BEGIN(R470_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp4,        32)
REGDEF_END(R470_ADVANCED_OBP)


/*
    OBP_TBL_tmp5:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R474_ADVANCED_OBP_OFS 0x0474
REGDEF_BEGIN(R474_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp5,        32)
REGDEF_END(R474_ADVANCED_OBP)


/*
    OBP_TBL_tmp6:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R478_ADVANCED_OBP_OFS 0x0478
REGDEF_BEGIN(R478_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp6,        32)
REGDEF_END(R478_ADVANCED_OBP)


/*
    OBP_TBL_tmp7:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R47C_ADVANCED_OBP_OFS 0x047c
REGDEF_BEGIN(R47C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp7,        32)
REGDEF_END(R47C_ADVANCED_OBP)


/*
    OBP_TBL_tmp8:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R480_ADVANCED_OBP_OFS 0x0480
REGDEF_BEGIN(R480_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp8,        32)
REGDEF_END(R480_ADVANCED_OBP)


/*
    OBP_TBL_tmp9:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R484_ADVANCED_OBP_OFS 0x0484
REGDEF_BEGIN(R484_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp9,        32)
REGDEF_END(R484_ADVANCED_OBP)


/*
    OBP_TBL_tmp10:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R488_ADVANCED_OBP_OFS 0x0488
REGDEF_BEGIN(R488_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp10,        32)
REGDEF_END(R488_ADVANCED_OBP)


/*
    OBP_TBL_tmp11:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R48C_ADVANCED_OBP_OFS 0x048c
REGDEF_BEGIN(R48C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp11,        32)
REGDEF_END(R48C_ADVANCED_OBP)


/*
    OBP_TBL_tmp12:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R490_ADVANCED_OBP_OFS 0x0490
REGDEF_BEGIN(R490_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp12,        32)
REGDEF_END(R490_ADVANCED_OBP)


/*
    OBP_TBL_tmp13:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R494_ADVANCED_OBP_OFS 0x0494
REGDEF_BEGIN(R494_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp13,        32)
REGDEF_END(R494_ADVANCED_OBP)


/*
    OBP_TBL_tmp14:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R498_ADVANCED_OBP_OFS 0x0498
REGDEF_BEGIN(R498_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp14,        32)
REGDEF_END(R498_ADVANCED_OBP)


/*
    OBP_TBL_tmp15:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R49C_ADVANCED_OBP_OFS 0x049c
REGDEF_BEGIN(R49C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp15,        32)
REGDEF_END(R49C_ADVANCED_OBP)


/*
    OBP_TBL_tmp16:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4A0_ADVANCED_OBP_OFS 0x04a0
REGDEF_BEGIN(R4A0_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp16,        32)
REGDEF_END(R4A0_ADVANCED_OBP)


/*
    OBP_TBL_tmp17:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4A4_ADVANCED_OBP_OFS 0x04a4
REGDEF_BEGIN(R4A4_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp17,        32)
REGDEF_END(R4A4_ADVANCED_OBP)


/*
    OBP_TBL_tmp18:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4A8_ADVANCED_OBP_OFS 0x04a8
REGDEF_BEGIN(R4A8_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp18,        32)
REGDEF_END(R4A8_ADVANCED_OBP)


/*
    OBP_TBL_tmp19:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4AC_ADVANCED_OBP_OFS 0x04ac
REGDEF_BEGIN(R4AC_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp19,        32)
REGDEF_END(R4AC_ADVANCED_OBP)


/*
    OBP_TBL_tmp20:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4B0_ADVANCED_OBP_OFS 0x04b0
REGDEF_BEGIN(R4B0_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp20,        32)
REGDEF_END(R4B0_ADVANCED_OBP)


/*
    OBP_TBL_tmp21:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4B4_ADVANCED_OBP_OFS 0x04b4
REGDEF_BEGIN(R4B4_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp21,        32)
REGDEF_END(R4B4_ADVANCED_OBP)


/*
    OBP_TBL_tmp22:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4B8_ADVANCED_OBP_OFS 0x04b8
REGDEF_BEGIN(R4B8_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp22,        32)
REGDEF_END(R4B8_ADVANCED_OBP)


/*
    OBP_TBL_tmp23:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4BC_ADVANCED_OBP_OFS 0x04bc
REGDEF_BEGIN(R4BC_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp23,        32)
REGDEF_END(R4BC_ADVANCED_OBP)


/*
    OBP_TBL_tmp24:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4C0_ADVANCED_OBP_OFS 0x04c0
REGDEF_BEGIN(R4C0_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp24,        32)
REGDEF_END(R4C0_ADVANCED_OBP)


/*
    OBP_TBL_tmp25:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4C4_ADVANCED_OBP_OFS 0x04c4
REGDEF_BEGIN(R4C4_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp25,        32)
REGDEF_END(R4C4_ADVANCED_OBP)


/*
    OBP_TBL_tmp26:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4C8_ADVANCED_OBP_OFS 0x04c8
REGDEF_BEGIN(R4C8_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp26,        32)
REGDEF_END(R4C8_ADVANCED_OBP)


/*
    OBP_TBL_tmp27:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4CC_ADVANCED_OBP_OFS 0x04cc
REGDEF_BEGIN(R4CC_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp27,        32)
REGDEF_END(R4CC_ADVANCED_OBP)


/*
    OBP_TBL_tmp28:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4D0_ADVANCED_OBP_OFS 0x04d0
REGDEF_BEGIN(R4D0_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp28,        32)
REGDEF_END(R4D0_ADVANCED_OBP)


/*
    OBP_TBL_tmp29:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4D4_ADVANCED_OBP_OFS 0x04d4
REGDEF_BEGIN(R4D4_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp29,        32)
REGDEF_END(R4D4_ADVANCED_OBP)


/*
    OBP_TBL_tmp30:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4D8_ADVANCED_OBP_OFS 0x04d8
REGDEF_BEGIN(R4D8_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp30,        32)
REGDEF_END(R4D8_ADVANCED_OBP)


/*
    OBP_TBL_tmp31:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4DC_ADVANCED_OBP_OFS 0x04dc
REGDEF_BEGIN(R4DC_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp31,        32)
REGDEF_END(R4DC_ADVANCED_OBP)


/*
    OBP_TBL_tmp32:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4E0_ADVANCED_OBP_OFS 0x04e0
REGDEF_BEGIN(R4E0_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp32,        32)
REGDEF_END(R4E0_ADVANCED_OBP)


/*
    OBP_TBL_tmp33:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4E4_ADVANCED_OBP_OFS 0x04e4
REGDEF_BEGIN(R4E4_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp33,        32)
REGDEF_END(R4E4_ADVANCED_OBP)


/*
    OBP_TBL_tmp34:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4E8_ADVANCED_OBP_OFS 0x04e8
REGDEF_BEGIN(R4E8_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp34,        32)
REGDEF_END(R4E8_ADVANCED_OBP)


/*
    OBP_TBL_tmp35:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4EC_ADVANCED_OBP_OFS 0x04ec
REGDEF_BEGIN(R4EC_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp35,        32)
REGDEF_END(R4EC_ADVANCED_OBP)


/*
    OBP_TBL_tmp36:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4F0_ADVANCED_OBP_OFS 0x04f0
REGDEF_BEGIN(R4F0_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp36,        32)
REGDEF_END(R4F0_ADVANCED_OBP)


/*
    OBP_TBL_tmp37:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4F4_ADVANCED_OBP_OFS 0x04f4
REGDEF_BEGIN(R4F4_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp37,        32)
REGDEF_END(R4F4_ADVANCED_OBP)


/*
    OBP_TBL_tmp38:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4F8_ADVANCED_OBP_OFS 0x04f8
REGDEF_BEGIN(R4F8_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp38,        32)
REGDEF_END(R4F8_ADVANCED_OBP)


/*
    OBP_TBL_tmp39:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R4FC_ADVANCED_OBP_OFS 0x04fc
REGDEF_BEGIN(R4FC_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp39,        32)
REGDEF_END(R4FC_ADVANCED_OBP)


/*
    OBP_TBL_tmp40:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R500_ADVANCED_OBP_OFS 0x0500
REGDEF_BEGIN(R500_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp40,        32)
REGDEF_END(R500_ADVANCED_OBP)


/*
    OBP_TBL_tmp41:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R504_ADVANCED_OBP_OFS 0x0504
REGDEF_BEGIN(R504_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp41,        32)
REGDEF_END(R504_ADVANCED_OBP)


/*
    OBP_TBL_tmp42:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R508_ADVANCED_OBP_OFS 0x0508
REGDEF_BEGIN(R508_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp42,        32)
REGDEF_END(R508_ADVANCED_OBP)


/*
    OBP_TBL_tmp43:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R50C_ADVANCED_OBP_OFS 0x050c
REGDEF_BEGIN(R50C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp43,        32)
REGDEF_END(R50C_ADVANCED_OBP)


/*
    OBP_TBL_tmp44:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R510_ADVANCED_OBP_OFS 0x0510
REGDEF_BEGIN(R510_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp44,        32)
REGDEF_END(R510_ADVANCED_OBP)


/*
    OBP_TBL_tmp45:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R514_ADVANCED_OBP_OFS 0x0514
REGDEF_BEGIN(R514_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp45,        32)
REGDEF_END(R514_ADVANCED_OBP)


/*
    OBP_TBL_tmp46:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R518_ADVANCED_OBP_OFS 0x0518
REGDEF_BEGIN(R518_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp46,        32)
REGDEF_END(R518_ADVANCED_OBP)


/*
    OBP_TBL_tmp47:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R51C_ADVANCED_OBP_OFS 0x051c
REGDEF_BEGIN(R51C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp47,        32)
REGDEF_END(R51C_ADVANCED_OBP)


/*
    OBP_TBL_tmp48:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R520_ADVANCED_OBP_OFS 0x0520
REGDEF_BEGIN(R520_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp48,        32)
REGDEF_END(R520_ADVANCED_OBP)


/*
    OBP_TBL_tmp49:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R524_ADVANCED_OBP_OFS 0x0524
REGDEF_BEGIN(R524_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp49,        32)
REGDEF_END(R524_ADVANCED_OBP)


/*
    OBP_TBL_tmp50:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R528_ADVANCED_OBP_OFS 0x0528
REGDEF_BEGIN(R528_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp50,        32)
REGDEF_END(R528_ADVANCED_OBP)


/*
    OBP_TBL_tmp51:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R52C_ADVANCED_OBP_OFS 0x052c
REGDEF_BEGIN(R52C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp51,        32)
REGDEF_END(R52C_ADVANCED_OBP)


/*
    OBP_TBL_tmp52:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R530_ADVANCED_OBP_OFS 0x0530
REGDEF_BEGIN(R530_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp52,        32)
REGDEF_END(R530_ADVANCED_OBP)


/*
    OBP_TBL_tmp53:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R534_ADVANCED_OBP_OFS 0x0534
REGDEF_BEGIN(R534_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp53,        32)
REGDEF_END(R534_ADVANCED_OBP)


/*
    OBP_TBL_tmp54:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R538_ADVANCED_OBP_OFS 0x0538
REGDEF_BEGIN(R538_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp54,        32)
REGDEF_END(R538_ADVANCED_OBP)


/*
    OBP_TBL_tmp55:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R53C_ADVANCED_OBP_OFS 0x053c
REGDEF_BEGIN(R53C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp55,        32)
REGDEF_END(R53C_ADVANCED_OBP)


/*
    OBP_TBL_tmp56:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R540_ADVANCED_OBP_OFS 0x0540
REGDEF_BEGIN(R540_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp56,        32)
REGDEF_END(R540_ADVANCED_OBP)


/*
    OBP_TBL_tmp57:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R544_ADVANCED_OBP_OFS 0x0544
REGDEF_BEGIN(R544_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp57,        32)
REGDEF_END(R544_ADVANCED_OBP)


/*
    OBP_TBL_tmp58:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R548_ADVANCED_OBP_OFS 0x0548
REGDEF_BEGIN(R548_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp58,        32)
REGDEF_END(R548_ADVANCED_OBP)


/*
    OBP_TBL_tmp59:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R54C_ADVANCED_OBP_OFS 0x054c
REGDEF_BEGIN(R54C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp59,        32)
REGDEF_END(R54C_ADVANCED_OBP)


/*
    OBP_TBL_tmp60:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R550_ADVANCED_OBP_OFS 0x0550
REGDEF_BEGIN(R550_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp60,        32)
REGDEF_END(R550_ADVANCED_OBP)


/*
    OBP_TBL_tmp61:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R554_ADVANCED_OBP_OFS 0x0554
REGDEF_BEGIN(R554_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp61,        32)
REGDEF_END(R554_ADVANCED_OBP)


/*
    OBP_TBL_tmp62:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R558_ADVANCED_OBP_OFS 0x0558
REGDEF_BEGIN(R558_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp62,        32)
REGDEF_END(R558_ADVANCED_OBP)


/*
    OBP_TBL_tmp63:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R55C_ADVANCED_OBP_OFS 0x055c
REGDEF_BEGIN(R55C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp63,        32)
REGDEF_END(R55C_ADVANCED_OBP)


/*
    OBP_TBL_tmp64:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R560_ADVANCED_OBP_OFS 0x0560
REGDEF_BEGIN(R560_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp64,        32)
REGDEF_END(R560_ADVANCED_OBP)


/*
    OBP_TBL_tmp65:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R564_ADVANCED_OBP_OFS 0x0564
REGDEF_BEGIN(R564_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp65,        32)
REGDEF_END(R564_ADVANCED_OBP)


/*
    OBP_TBL_tmp66:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R568_ADVANCED_OBP_OFS 0x0568
REGDEF_BEGIN(R568_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp66,        32)
REGDEF_END(R568_ADVANCED_OBP)


/*
    OBP_TBL_tmp67:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R56C_ADVANCED_OBP_OFS 0x056c
REGDEF_BEGIN(R56C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp67,        32)
REGDEF_END(R56C_ADVANCED_OBP)


/*
    OBP_TBL_tmp68:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R570_ADVANCED_OBP_OFS 0x0570
REGDEF_BEGIN(R570_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp68,        32)
REGDEF_END(R570_ADVANCED_OBP)


/*
    OBP_TBL_tmp69:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R574_ADVANCED_OBP_OFS 0x0574
REGDEF_BEGIN(R574_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp69,        32)
REGDEF_END(R574_ADVANCED_OBP)


/*
    OBP_TBL_tmp70:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R578_ADVANCED_OBP_OFS 0x0578
REGDEF_BEGIN(R578_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp70,        32)
REGDEF_END(R578_ADVANCED_OBP)


/*
    OBP_TBL_tmp71:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R57C_ADVANCED_OBP_OFS 0x057c
REGDEF_BEGIN(R57C_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp71,        32)
REGDEF_END(R57C_ADVANCED_OBP)


/*
    OBP_TBL_tmp72:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R580_ADVANCED_OBP_OFS 0x0580
REGDEF_BEGIN(R580_ADVANCED_OBP)
REGDEF_BIT(OBP_TBL_tmp72,        32)
REGDEF_END(R580_ADVANCED_OBP)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R584_RESERVED_OFS 0x0584
REGDEF_BEGIN(R584_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R584_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R588_RESERVED_OFS 0x0588
REGDEF_BEGIN(R588_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R588_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R58C_RESERVED_OFS 0x058c
REGDEF_BEGIN(R58C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R58C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R590_RESERVED_OFS 0x0590
REGDEF_BEGIN(R590_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R590_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R594_RESERVED_OFS 0x0594
REGDEF_BEGIN(R594_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R594_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R598_RESERVED_OFS 0x0598
REGDEF_BEGIN(R598_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R598_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R59C_RESERVED_OFS 0x059c
REGDEF_BEGIN(R59C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R59C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5A0_RESERVED_OFS 0x05a0
REGDEF_BEGIN(R5A0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5A0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5A4_RESERVED_OFS 0x085a4
REGDEF_BEGIN(R5A4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5A4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5A8_RESERVED_OFS 0x05a8
REGDEF_BEGIN(R5A8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5A8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5AC_RESERVED_OFS 0x05ac
REGDEF_BEGIN(R5AC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5AC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5B0_RESERVED_OFS 0x05b0
REGDEF_BEGIN(R5B0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5B0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5B4_RESERVED_OFS 0x05b4
REGDEF_BEGIN(R5B4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5B4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5B8_RESERVED_OFS 0x05b8
REGDEF_BEGIN(R5B8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5B8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5BC_RESERVED_OFS 0x05bc
REGDEF_BEGIN(R5BC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5BC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5C0_RESERVED_OFS 0x05c0
REGDEF_BEGIN(R5C0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5C0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5C4_RESERVED_OFS 0x05c4
REGDEF_BEGIN(R5C4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5C4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5C8_RESERVED_OFS 0x05c8
REGDEF_BEGIN(R5C8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5C8_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5CC_RESERVED_OFS 0x05cc
REGDEF_BEGIN(R5CC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5CC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5D0_RESERVED_OFS 0x05d0
REGDEF_BEGIN(R5D0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5D0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5D4_RESERVED_OFS 0x05d4
REGDEF_BEGIN(R5D4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5D4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5D8_RESERVED_OFS 0x05d8
REGDEF_BEGIN(R5D8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5D8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5DC_RESERVED_OFS 0x05dc
REGDEF_BEGIN(R5DC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5DC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5E0_RESERVED_OFS 0x05e0
REGDEF_BEGIN(R5E0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5E0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5E4_RESERVED_OFS 0x05e4
REGDEF_BEGIN(R5E4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5E4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5E8_RESERVED_OFS 0x05e8
REGDEF_BEGIN(R5E8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5E8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5EC_RESERVED_OFS 0x05ec
REGDEF_BEGIN(R5EC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5EC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5F0_RESERVED_OFS 0x05f0
REGDEF_BEGIN(R5F0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5F0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5F4_RESERVED_OFS 0x05f4
REGDEF_BEGIN(R5F4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5F4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5F8_RESERVED_OFS 0x05f8
REGDEF_BEGIN(R5F8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5F8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5FC_RESERVED_OFS 0x05fc
REGDEF_BEGIN(R5FC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5FC_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R600_RESERVED_OFS 0x0600
REGDEF_BEGIN(R600_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R600_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R604_RESERVED_OFS 0x0604
REGDEF_BEGIN(R604_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R604_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R608_RESERVED_OFS 0x0608
REGDEF_BEGIN(R608_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R608_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R60C_RESERVED_OFS 0x060c
REGDEF_BEGIN(R60C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R60C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R610_RESERVED_OFS 0x0610
REGDEF_BEGIN(R610_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R610_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R614_RESERVED_OFS 0x0614
REGDEF_BEGIN(R614_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R614_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R618_RESERVED_OFS 0x0618
REGDEF_BEGIN(R618_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R618_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R61C_RESERVED_OFS 0x061c
REGDEF_BEGIN(R61C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R61C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R620_RESERVED_OFS 0x0620
REGDEF_BEGIN(R620_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R620_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R624_RESERVED_OFS 0x0624
REGDEF_BEGIN(R624_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R624_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R628_RESERVED_OFS 0x0628
REGDEF_BEGIN(R628_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R628_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R62C_RESERVED_OFS 0x062c
REGDEF_BEGIN(R62C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R62C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R630_RESERVED_OFS 0x0630
REGDEF_BEGIN(R630_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R630_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R634_RESERVED_OFS 0x0634
REGDEF_BEGIN(R634_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R634_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R638_RESERVED_OFS 0x0638
REGDEF_BEGIN(R638_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R638_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R63C_RESERVED_OFS 0x063c
REGDEF_BEGIN(R63C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R63C_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R640_RESERVED_OFS 0x0640
REGDEF_BEGIN(R640_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R640_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R644_RESERVED_OFS 0x0644
REGDEF_BEGIN(R644_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R644_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R648_RESERVED_OFS 0x0648
REGDEF_BEGIN(R648_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R648_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R64C_RESERVED_OFS 0x064c
REGDEF_BEGIN(R64C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R64C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R650_RESERVED_OFS 0x0650
REGDEF_BEGIN(R650_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R650_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R654_RESERVED_OFS 0x0654
REGDEF_BEGIN(R654_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R654_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R658_RESERVED_OFS 0x0658
REGDEF_BEGIN(R658_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R658_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R65C_RESERVED_OFS 0x065c
REGDEF_BEGIN(R65C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R65C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R660_RESERVED_OFS 0x0660
REGDEF_BEGIN(R660_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R660_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R664_RESERVED_OFS 0x0664
REGDEF_BEGIN(R664_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R664_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R668_RESERVED_OFS 0x0668
REGDEF_BEGIN(R668_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R668_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R66C_RESERVED_OFS 0x066c
REGDEF_BEGIN(R66C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R66C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R670_RESERVED_OFS 0x0670
REGDEF_BEGIN(R670_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R670_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R674_RESERVED_OFS 0x0674
REGDEF_BEGIN(R674_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R674_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R678_RESERVED_OFS 0x0678
REGDEF_BEGIN(R678_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R678_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R67C_RESERVED_OFS 0x067c
REGDEF_BEGIN(R67C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R67C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R680_RESERVED_OFS 0x0680
REGDEF_BEGIN(R680_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R680_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R684_RESERVED_OFS 0x0684
REGDEF_BEGIN(R684_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R684_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R688_RESERVED_OFS 0x0688
REGDEF_BEGIN(R688_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R688_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R68C_RESERVED_OFS 0x068c
REGDEF_BEGIN(R68C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R68C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R690_RESERVED_OFS 0x0690
REGDEF_BEGIN(R690_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R690_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R694_RESERVED_OFS 0x0694
REGDEF_BEGIN(R694_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R694_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R698_RESERVED_OFS 0x0698
REGDEF_BEGIN(R698_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R698_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R69C_RESERVED_OFS 0x069c
REGDEF_BEGIN(R69C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R69C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6A0_RESERVED_OFS 0x06a0
REGDEF_BEGIN(R6A0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6A0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6A4_RESERVED_OFS 0x06a4
REGDEF_BEGIN(R6A4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6A4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6A8_RESERVED_OFS 0x06a8
REGDEF_BEGIN(R6A8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6A8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6AC_RESERVED_OFS 0x06ac
REGDEF_BEGIN(R6AC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6AC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6B0_RESERVED_OFS 0x06b0
REGDEF_BEGIN(R6B0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6B0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6B4_RESERVED_OFS 0x06b4
REGDEF_BEGIN(R6B4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6B4_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6B8_RESERVED_OFS 0x06b8
REGDEF_BEGIN(R6B8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6B8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6BC_RESERVED_OFS 0x06bc
REGDEF_BEGIN(R6BC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6BC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6C0_RESERVED_OFS 0x06c0
REGDEF_BEGIN(R6C0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6C0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6C4_RESERVED_OFS 0x06c4
REGDEF_BEGIN(R6C4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6C4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6C8_RESERVED_OFS 0x06c8
REGDEF_BEGIN(R6C8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6C8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6CC_RESERVED_OFS 0x06cc
REGDEF_BEGIN(R6CC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6CC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6D0_RESERVED_OFS 0x06d0
REGDEF_BEGIN(R6D0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6D0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6D4_RESERVED_OFS 0x06d4
REGDEF_BEGIN(R6D4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6D4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6D8_RESERVED_OFS 0x06d8
REGDEF_BEGIN(R6D8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6D8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6DC_RESERVED_OFS 0x06dc
REGDEF_BEGIN(R6DC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6DC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6E0_RESERVED_OFS 0x06e0
REGDEF_BEGIN(R6E0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6E0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6E4_RESERVED_OFS 0x06e4
REGDEF_BEGIN(R6E4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6E4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6E8_RESERVED_OFS 0x06e8
REGDEF_BEGIN(R6E8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6E8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6EC_RESERVED_OFS 0x06ec
REGDEF_BEGIN(R6EC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6EC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6F0_RESERVED_OFS 0x06f0
REGDEF_BEGIN(R6F0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6F0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6F4_RESERVED_OFS 0x06f4
REGDEF_BEGIN(R6F4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6F4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6F8_RESERVED_OFS 0x06f8
REGDEF_BEGIN(R6F8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6F8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R6FC_RESERVED_OFS 0x06fc
REGDEF_BEGIN(R6FC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R6FC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R700_RESERVED_OFS 0x0700
REGDEF_BEGIN(R700_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R700_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R704_RESERVED_OFS 0x0704
REGDEF_BEGIN(R704_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R704_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R708_RESERVED_OFS 0x0708
REGDEF_BEGIN(R708_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R708_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R70C_RESERVED_OFS 0x070c
REGDEF_BEGIN(R70C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R70C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R710_RESERVED_OFS 0x0710
REGDEF_BEGIN(R710_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R710_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R714_RESERVED_OFS 0x0714
REGDEF_BEGIN(R714_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R714_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R718_RESERVED_OFS 0x0718
REGDEF_BEGIN(R718_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R718_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R71C_RESERVED_OFS 0x071c
REGDEF_BEGIN(R71C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R71C_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R720_RESERVED_OFS 0x0720
REGDEF_BEGIN(R720_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R720_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R724_RESERVED_OFS 0x0724
REGDEF_BEGIN(R724_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R724_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R728_RESERVED_OFS 0x0728
REGDEF_BEGIN(R728_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R728_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R72C_RESERVED_OFS 0x072c
REGDEF_BEGIN(R72C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R72C_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R730_RESERVED_OFS 0x0730
REGDEF_BEGIN(R730_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R730_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R734_RESERVED_OFS 0x0734
REGDEF_BEGIN(R734_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R734_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R738_RESERVED_OFS 0x0738
REGDEF_BEGIN(R738_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R738_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R73C_RESERVED_OFS 0x073c
REGDEF_BEGIN(R73C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R73C_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R740_RESERVED_OFS 0x0740
REGDEF_BEGIN(R740_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R740_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R744_RESERVED_OFS 0x0744
REGDEF_BEGIN(R744_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R744_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R748_RESERVED_OFS 0x0748
REGDEF_BEGIN(R748_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R748_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R74C_RESERVED_OFS 0x074c
REGDEF_BEGIN(R74C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R74C_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R750_RESERVED_OFS 0x0750
REGDEF_BEGIN(R750_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R750_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R754_RESERVED_OFS 0x0754
REGDEF_BEGIN(R754_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R754_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R758_RESERVED_OFS 0x0758
REGDEF_BEGIN(R758_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R758_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R75C_RESERVED_OFS 0x075c
REGDEF_BEGIN(R75C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R75C_RESERVED)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R760_RESERVED_OFS 0x0760
REGDEF_BEGIN(R760_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R760_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R764_RESERVED_OFS 0x0764
REGDEF_BEGIN(R764_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R764_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R768_RESERVED_OFS 0x0768
REGDEF_BEGIN(R768_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R768_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R76C_RESERVED_OFS 0x076c
REGDEF_BEGIN(R76C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R76C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R770_RESERVED_OFS 0x0770
REGDEF_BEGIN(R770_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R770_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R774_RESERVED_OFS 0x0774
REGDEF_BEGIN(R774_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R774_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R778_RESERVED_OFS 0x0778
REGDEF_BEGIN(R778_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R778_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R77C_RESERVED_OFS 0x077c
REGDEF_BEGIN(R77C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R77C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R780_RESERVED_OFS 0x0780
REGDEF_BEGIN(R780_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R780_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R784_RESERVED_OFS 0x0784
REGDEF_BEGIN(R784_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R784_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R788_RESERVED_OFS 0x0788
REGDEF_BEGIN(R788_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R788_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R78C_RESERVED_OFS 0x078c
REGDEF_BEGIN(R78C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R78C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R790_RESERVED_OFS 0x0790
REGDEF_BEGIN(R790_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R790_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R794_RESERVED_OFS 0x0794
REGDEF_BEGIN(R794_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R794_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R798_RESERVED_OFS 0x0798
REGDEF_BEGIN(R798_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R798_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R79C_RESERVED_OFS 0x079c
REGDEF_BEGIN(R79C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R79C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7A0_RESERVED_OFS 0x07a0
REGDEF_BEGIN(R7A0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7A0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7A4_RESERVED_OFS 0x07a4
REGDEF_BEGIN(R7A4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7A4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7A8_RESERVED_OFS 0x07a8
REGDEF_BEGIN(R7A8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7A8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7AC_RESERVED_OFS 0x07ac
REGDEF_BEGIN(R7AC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7AC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7B0_RESERVED_OFS 0x07b0
REGDEF_BEGIN(R7B0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7B0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7B4_RESERVED_OFS 0x07b4
REGDEF_BEGIN(R7B4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7B4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7B8_RESERVED_OFS 0x07b8
REGDEF_BEGIN(R7B8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7B8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7BC_RESERVED_OFS 0x07bc
REGDEF_BEGIN(R7BC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7BC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7C0_RESERVED_OFS 0x07c0
REGDEF_BEGIN(R7C0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7C0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7C04_RESERVED_OFS 0x07c4
REGDEF_BEGIN(R7C4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7C4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7C8_RESERVED_OFS 0x07c8
REGDEF_BEGIN(R7C8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7C8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7CC_RESERVED_OFS 0x07cc
REGDEF_BEGIN(R7CC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7CC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7D0_RESERVED_OFS 0x07d0
REGDEF_BEGIN(R7D0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7D0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7D4_RESERVED_OFS 0x07d4
REGDEF_BEGIN(R7D4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7D4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7D8_RESERVED_OFS 0x07d8
REGDEF_BEGIN(R7D8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7D8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7DC_RESERVED_OFS 0x07dc
REGDEF_BEGIN(R7DC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7DC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7E0_RESERVED_OFS 0x07e0
REGDEF_BEGIN(R7E0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7E0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7E4_RESERVED_OFS 0x07e4
REGDEF_BEGIN(R7E4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7E4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7E8_RESERVED_OFS 0x07e8
REGDEF_BEGIN(R7E8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7E8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7EC_RESERVED_OFS 0x07ec
REGDEF_BEGIN(R7EC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7EC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7F0_RESERVED_OFS 0x07f0
REGDEF_BEGIN(R7F0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7F0_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7F4_RESERVED_OFS 0x07f4
REGDEF_BEGIN(R7F4_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7F4_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7F8_RESERVED_OFS 0x07f8
REGDEF_BEGIN(R7F8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7F8_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R7FC_RESERVED_OFS 0x07fc
REGDEF_BEGIN(R7FC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R7FC_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R800_RESERVED_OFS 0x0800
REGDEF_BEGIN(R800_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R800_RESERVED)

/*
    STRP_SUP_FMT:          [0x0, 0x1],             bits :  0_0
    GAMMA_EN    :          [0x0, 0x1],             bits :  4_4
    ENCROUNDING_SEL:       [0x0, 0x1],             bits :  8_8
    ENCROUNDING_PNINIT:    [0x0, 0x7fff],          bits : 26_12
*/
#define R804_RAWENC_CTRL_OFS 0x0804
REGDEF_BEGIN(R804_RAWENC_CTRL)
REGDEF_BIT(STRP_SUP_FMT,              1)
REGDEF_BIT(,              3)
REGDEF_BIT(GAMMA_EN,              1)
REGDEF_END(R804_RAWENC_CTRL)


/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R808_RESERVED_OFS 0x0808
REGDEF_BEGIN(R808_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R808_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R80C_RESERVED_OFS 0x080c
REGDEF_BEGIN(R80C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R80C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R810_RESERVED_OFS 0x0810
REGDEF_BEGIN(R810_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R810_RESERVED)

/*
    LFN_CSB_TBL_NUM_SEL:    [0x0, 0x3],            bits : 1_0
    BRC_SEGBITNO:           [0x0, 0xff],           bits :11_4
    DIST_CAL_TYPE:          [0x0, 0x1],            bits :12_12
    DIST_DIFF_WT:           [0x0, 0x7],            bits :18_16
    DIST_SUM_WT:            [0x0, 0x7],            bits :22_20
    DIST_SHIFT:             [0x0, 0x7],            bits :26_24
*/
#define R814_RAWENC_BRC_OFS 0x0814
REGDEF_BEGIN(R814_RAWENC_BRC)
REGDEF_BIT(BRC_SEGBITNO,       3)
REGDEF_END(R814_RAWENC_BRC)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R818_RESERVED_OFS 0x0818
REGDEF_BEGIN(R818_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R818_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R81C_RESERVED_OFS 0x081c
REGDEF_BEGIN(R81C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R81C_RESERVED)

/*
    RESERVED:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R820_RESERVED_OFS 0x0820
REGDEF_BEGIN(R820_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R820_RESERVED)

/*
    GAMMA_TBL0:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL1:             [0x0, 0xfff],          bits :27_16
*/
#define R824_RAWENC_GAMMA_OFS 0x0824
REGDEF_BEGIN(R824_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL0,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL1,         12)
REGDEF_END(R824_RAWENC_GAMMA)

/*
    GAMMA_TBL2:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL3:             [0x0, 0xfff],          bits :27_16
*/
#define R828_RAWENC_GAMMA_OFS 0x0828
REGDEF_BEGIN(R828_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL2,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL3,         12)
REGDEF_END(R828_RAWENC_GAMMA)

/*
    GAMMA_TBL4:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL5:             [0x0, 0xfff],          bits :27_16
*/
#define R82C_RAWENC_GAMMA_OFS 0x082c
REGDEF_BEGIN(R82C_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL4,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL5,         12)
REGDEF_END(R82C_RAWENC_GAMMA)

/*
    GAMMA_TBL6:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL7:             [0x0, 0xfff],          bits :27_16
*/
#define R830_RAWENC_GAMMA_OFS 0x0830
REGDEF_BEGIN(R830_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL6,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL7,         12)
REGDEF_END(R830_RAWENC_GAMMA)

/*
    GAMMA_TBL8:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL9:             [0x0, 0xfff],          bits :27_16
*/
#define R834_RAWENC_GAMMA_OFS 0x0834
REGDEF_BEGIN(R834_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL8,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL9,         12)
REGDEF_END(R834_RAWENC_GAMMA)

/*
    GAMMA_TBL10:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL11:             [0x0, 0xfff],          bits :27_16
*/
#define R838_RAWENC_GAMMA_OFS 0x0838
REGDEF_BEGIN(R838_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL10,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL11,         12)
REGDEF_END(R838_RAWENC_GAMMA)

/*
    GAMMA_TBL12:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL13:             [0x0, 0xfff],          bits :27_16
*/
#define R83C_RAWENC_GAMMA_OFS 0x083c
REGDEF_BEGIN(R83C_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL12,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL13,         12)
REGDEF_END(R83C_RAWENC_GAMMA)

/*
    GAMMA_TBL14:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL15:             [0x0, 0xfff],          bits :27_16
*/
#define R840_RAWENC_GAMMA_OFS 0x0840
REGDEF_BEGIN(R840_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL14,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL15,         12)
REGDEF_END(R840_RAWENC_GAMMA)

/*
    GAMMA_TBL16:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL17:             [0x0, 0xfff],          bits :27_16
*/
#define R844_RAWENC_GAMMA_OFS 0x0844
REGDEF_BEGIN(R844_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL16,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL17,         12)
REGDEF_END(R844_RAWENC_GAMMA)

/*
    GAMMA_TBL18:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL19:             [0x0, 0xfff],          bits :27_16
*/
#define R848_RAWENC_GAMMA_OFS 0x0848
REGDEF_BEGIN(R848_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL18,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL19,         12)
REGDEF_END(R848_RAWENC_GAMMA)

/*
    GAMMA_TBL20:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL21:             [0x0, 0xfff],          bits :27_16
*/
#define R84C_RAWENC_GAMMA_OFS 0x084c
REGDEF_BEGIN(R84C_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL20,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL21,         12)
REGDEF_END(R84C_RAWENC_GAMMA)

/*
    GAMMA_TBL22:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL23:             [0x0, 0xfff],          bits :27_16
*/
#define R850_RAWENC_GAMMA_OFS 0x0850
REGDEF_BEGIN(R850_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL22,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL23,         12)
REGDEF_END(R850_RAWENC_GAMMA)

/*
    GAMMA_TBL24:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL25:             [0x0, 0xfff],          bits :27_16
*/
#define R854_RAWENC_GAMMA_OFS 0x0854
REGDEF_BEGIN(R854_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL24,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL25,         12)
REGDEF_END(R854_RAWENC_GAMMA)

/*
    GAMMA_TBL26:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL27:             [0x0, 0xfff],          bits :27_16
*/
#define R858_RAWENC_GAMMA_OFS 0x0858
REGDEF_BEGIN(R858_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL26,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL27,         12)
REGDEF_END(R858_RAWENC_GAMMA)

/*
    GAMMA_TBL28:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL29:             [0x0, 0xfff],          bits :27_16
*/
#define R85C_RAWENC_GAMMA_OFS 0x085c
REGDEF_BEGIN(R85C_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL28,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL29,         12)
REGDEF_END(R85C_RAWENC_GAMMA)

/*
    GAMMA_TBL30:             [0x0, 0xfff],          bits :11_0
    GAMMA_TBL31:             [0x0, 0xfff],          bits :27_16
*/
#define R860_RAWENC_GAMMA_OFS 0x0860
REGDEF_BEGIN(R860_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL30,         12)
REGDEF_BIT(,          4)
REGDEF_BIT(GAMMA_TBL31,         12)
REGDEF_END(R860_RAWENC_GAMMA)

/*
    GAMMA_TBL32:             [0x0, 0xfff],          bits :11_0
*/
#define R864_RAWENC_GAMMA_OFS 0x0864
REGDEF_BEGIN(R864_RAWENC_GAMMA)
REGDEF_BIT(GAMMA_TBL32,         12)
REGDEF_END(R864_RAWENC_GAMMA)

/*
    BRC_LNCNT_LFN0:          [0x0, 0x1fff],         bits :12_0
    BRC_LNCNT_LFN1:          [0x0, 0x1fff],         bits :28_16
*/
#define R868_RAWENC_DCT_QTBL_IDX_OFS 0x0868
REGDEF_BEGIN(R868_RAWENC_DCT_QTBL_IDX)
REGDEF_BIT(DCT_QTBL0_IDX,         5)
REGDEF_BIT(             ,         3)
REGDEF_BIT(DCT_QTBL1_IDX,         5)
REGDEF_BIT(             ,         3)
REGDEF_BIT(DCT_QTBL2_IDX,         5)
REGDEF_BIT(             ,         3)
REGDEF_BIT(DCT_QTBL3_IDX,         5)
REGDEF_END(R868_RAWENC_DCT_QTBL_IDX)

/*
    BRC_LNCNT_LFN2:          [0x0, 0x1fff],         bits :12_0
    BRC_LNCNT_LFN3:          [0x0, 0x1fff],         bits :28_16
*/
#define R86C_RAWENC_DCT_QTBL_IDX_OFS 0x086c
REGDEF_BEGIN(R86C_RAWENC_DCT_QTBL_IDX)
REGDEF_BIT(DCT_QTBL4_IDX,         5)
REGDEF_BIT(             ,         3)
REGDEF_BIT(DCT_QTBL5_IDX,         5)
REGDEF_BIT(             ,         3)
REGDEF_BIT(DCT_QTBL6_IDX,         5)
REGDEF_BIT(             ,         3)
REGDEF_BIT(DCT_QTBL7_IDX,         5)
REGDEF_END(R86C_RAWENC_DCT_QTBL_IDX)

/*
    BRC_LNCNT_LFN4:          [0x0, 0x1fff],         bits :12_0
    BRC_LNCNT_LFN5:          [0x0, 0x1fff],         bits :28_16
*/
#define R870_RAWENC_DCT_LEVEL_TH_OFS 0x0870
REGDEF_BEGIN(R870_RAWENC_DCT_LEVEL_TH)
REGDEF_BIT(DCT_LEVEL_TH0,         8)
REGDEF_BIT(DCT_LEVEL_TH1,         8)
REGDEF_BIT(DCT_LEVEL_TH2,         8)
REGDEF_BIT(DCT_LEVEL_TH3,         8)
REGDEF_END(R870_RAWENC_DCT_LEVEL_TH)


typedef struct {
	//0x0000
	T_R0_ENGINE_CONTROL
	SIE_Register_0000;

	//0x0004
	T_R4_ENGINE_FUNCTION
	SIE_Register_0004;

	//0x0008
	T_R8_ENGINE_INTERRUPT
	SIE_Register_0008;

	//0x000c
	T_RC_ENGINE_INTERRUPT
	SIE_Register_000c;

	//0x0010
	T_R10_ENGINE_STATUS
	SIE_Register_0010;

	//0x0014
	T_R14_ENGINE_STATUS
	SIE_Register_0014;

	//0x0018
	T_R18_ENGINE_STATUS
	SIE_Register_0018;

	//0x001c
	T_R1C_ENGINE_TIMING
	SIE_Register_001c;

	//0x0020
	T_R20_ENGINE_TIMING
	SIE_Register_0020;

	//0x0024
	T_R24_ENGINE_TIMING
	SIE_Register_0024;

	//0x0028
	T_R28_ENGINE_TIMING
	SIE_Register_0028;

	//0x002c
	T_R2C_ENGINE_TIMING
	SIE_Register_002c;

	//0x0030
	T_R30_ENGINE_CFA_PATTERN
	SIE_Register_0030;

	//0x0034
	T_R34_ENGINE_TIMING
	SIE_Register_0034;

	//0x0038
	T_R38_ENGINE_TIMING
	SIE_Register_0038;

	//0x003c
	T_R3C_ENGINE_STATUS
	SIE_Register_003c;

	//0x0040
	T_R40_ENGINE_DRAM
	SIE_Register_0040;

	//0x0044
	T_R44_ENGINE_TIMING_DELAY
	SIE_Register_0044;

	//0x0048
	T_R48_ENGINE_DRAM
	SIE_Register_0048;

	//0x004c
	T_R4C_ENGINE_DRAM
	SIE_Register_004c;

	//0x0050
	T_R50_ENGINE_DRAM
	SIE_Register_0050;

	//0x0054
	T_R54_RESERVED
	SIE_Register_0054;

	//0x0058
	T_R58_RESERVED
	SIE_Register_0058;

	//0x005c
	T_R5C_ENGINE_DRAM
	SIE_Register_005c;

	//0x0060
	T_R60_ENGINE_DRAM
	SIE_Register_0060;

	//0x0064
	T_R64_ENGINE_DRAM
	SIE_Register_0064;

	//0x0068
	T_R68_ENGINE_DRAM
	SIE_Register_0068;

	//0x006c
	T_R6C_ENGINE_DRAM
	SIE_Register_006c;

	//0x0070
	T_R70_ENGINE_DRAM
	SIE_Register_0070;

	//0x0074
	T_R74_ENGINE_DRAM
	SIE_Register_0074;

	//0x0078
	T_R78_ENGINE_DRAM
	SIE_Register_0078;

	//0x007c
	T_R7C_ENGINE_DRAM
	SIE_Register_007c;

	//0x0080
	T_R80_ENGINE_DRAM
	SIE_Register_0080;

	//0x0084
	T_R84_ENGINE_DRAM
	SIE_Register_0084;

	//0x0088
	T_R88_ENGINE_DRAM
	SIE_Register_0088;

	//0x008c
	T_R8C_ENGINE_DRAM
	SIE_Register_008c;

	//0x0090
	T_R90_ENGINE_DRAM
	SIE_Register_0090;

	//0x0094
	T_R94_ENGINE_DRAM
	SIE_Register_0094;

	//0x0098
	T_R98_ENGINE_DRAM
	SIE_Register_0098;

	//0x009c
	T_R9C_ENGINE_DRAM
	SIE_Register_009c;

	//0x00a0
	T_RA0_ENGINE_DRAM
	SIE_Register_00a0;

	//0x00a4
	T_RA4_ENGINE_DRAM
	SIE_Register_00a4;

	//0x00a8
	T_RA8_ENGINE_DRAM
	SIE_Register_00a8;

	//0x00ac
	T_RAC_BASIC_DVI
	SIE_Register_00ac;

	//0x00b0
	T_RB0_BASIC_PATGEN
	SIE_Register_00b0;

	//0x00b4
	T_RB4_BASIC_OB
	SIE_Register_00b4;

	//0x00b8
	T_RB8_BASIC_OB
	SIE_Register_00b8;

	//0x00bc
	T_RBC_BASIC_OB
	SIE_Register_00bc;

	//0x00c0
	T_RC0_BASIC_OB
	SIE_Register_00c0;

	//0x00c4
	T_RC4_BASIC_OB
	SIE_Register_00c4;

	//0x00c8
	T_RC8_BASIC_CCIR
	SIE_Register_00c8;

	//0x00cc
	T_RCC_BASIC_DEFECT
	SIE_Register_00cc;

	//0x00d0
	T_RD0_DECOMP
	SIE_Register_00d0;

	//0x00d4
	T_RD4_DECOMP
	SIE_Register_00d4;

	//0x00d8
	T_RD8_DECOMP
	SIE_Register_00d8;

	//0x00dc
	T_RDC_DECOMP
	SIE_Register_00dc;

	//0x00e0
	T_RE0_DECOMP
	SIE_Register_00e0;

	//0x00e4
	T_RE4_DECOMP
	SIE_Register_00e4;

	//0x00e8
	T_RE8_DECOMP
	SIE_Register_00e8;

	//0x00ec
	T_REC_DECOMP
	SIE_Register_00ec;

	//0x00f0
	T_RF0_DECOMP
	SIE_Register_00f0;

	//0x00f4
	T_RF4_DECOMP
	SIE_Register_00f4;

	//0x00f8
	T_RF8_DECOMP
	SIE_Register_00f8;

	//0x00fc
	T_RFC_DECOMP
	SIE_Register_00fc;

	//0x0100
	T_R100_DECOMP
	SIE_Register_0100;

	//0x0104
	T_R104_DECOMP
	SIE_Register_0104;

	//0x0108
	T_R108_COMP
	SIE_Register_0108;

	//0x010c
	T_R10C_COMP
	SIE_Register_010c;

	//0x0110
	T_R110_COMP
	SIE_Register_0110;

	//0x0114
	T_R114_COMP
	SIE_Register_0114;

	//0x0118
	T_R118_COMP
	SIE_Register_0118;

	//0x011c
	T_R11C_COMP
	SIE_Register_011c;

	//0x0120
	T_R120_COMP
	SIE_Register_0120;

	//0x0124
	T_R124_COMP
	SIE_Register_0124;

	//0x0128
	T_R128_COMP
	SIE_Register_0128;

	//0x012c
	T_R12C_COMP
	SIE_Register_012c;

	//0x0130
	T_R130_COMP
	SIE_Register_0130;

	//0x0134
	T_R134_VD_INFO
	SIE_Register_0134;

	//0x0138
	T_R138_HD_INFO
	SIE_Register_0138;

	//0x013c
	T_R13C_VD_INTEV
	SIE_Register_013c;

	//0x0140
	T_R140_VD_INTEV
	SIE_Register_0140;

	//0x0144
	T_R144_HD_INTEV
	SIE_Register_0144;

	//0x0148
	T_R148_HD_INTEV
	SIE_Register_0148;

	//0x014c
	T_R14C_PXCLK_INFO
	SIE_Register_014c;

	//0x0150
	T_R150_CLK_INFO
	SIE_Register_0150;

	//0x0154
	T_R154_BASIC_ECS
	SIE_Register_0154;

	//0x0158
	T_R158_BASIC_ECS
	SIE_Register_0158;

	//0x015c
	T_R15C_BASIC_DGAIN
	SIE_Register_015c;
#if 0
	//0x0160
	T_R160_DEBUG
	SIE_Register_0160;

	//0x0164
	T_R164_BASIC_BSH
	SIE_Register_0164;
#endif

	//0x0160
	T_R160_BASIC_BSH
	SIE_Register_0160;

	//0x0164
	T_R164_BASIC_BSH
	SIE_Register_0164;

	//0x0168
	T_R168_BASIC_BSH
	SIE_Register_0168;

	//0x016c
	T_R16C_BASIC_BSH
	SIE_Register_016c;

	//0x0170
	T_R170_BASIC_BSH
	SIE_Register_0170;

	//0x0174
	T_R174_BASIC_BSV
	SIE_Register_0174;

	//0x0178
	T_R178_BASIC_BSV
	SIE_Register_0178;

	//0x017c
	T_R17C_BASIC_BSV
	SIE_Register_017c;
#if 0
	//0x0180
	T_R180_DEBUG
	SIE_Register_0180;

	//0x0184
	T_R184_DEBUG
	SIE_Register_0184;
#endif

	//0x0180
	T_R180_BASIC_BSV
	SIE_Register_0180;

	//0x0184
	T_R184_BASIC_BSV
	SIE_Register_0184;
		

	//0x0188
	T_R188_BASIC_BSV
	SIE_Register_0188;

	//0x018c
	T_R18C_BASIC_BSV
	SIE_Register_018c;

#if 0
	//0x0190
	T_R190_DEBUG
	SIE_Register_0190;

	//0x0194
	T_R194_DEBUG
	SIE_Register_0194;
#endif

	//0x0190
	T_R190_BASIC_BSV
	SIE_Register_0190;

	//0x0194
	T_R194_BASIC_BSV
	SIE_Register_0194;

	//0x0198
	T_R198_BASIC_BSV
	SIE_Register_0198;

	//0x019c
	T_R19C_BASIC_BSV
	SIE_Register_019c;

	//0x01a0
	T_R1A0_BASIC_BSV
	SIE_Register_01a0;

	//0x01a4
	T_R1A4_BASIC_BSV
	SIE_Register_01a4;

	//0x01a8
	T_R1A8_BASIC_BSV
	SIE_Register_01a8;

	//0x01ac
	T_R1AC_BASIC_CCIR_DBG
	SIE_Register_01ac;

	//0x01b0
	T_R1B0_BASIC_CCIR_DBG
	SIE_Register_01b0;

	//0x01b4
	T_R1B4_BASIC_CG
	SIE_Register_01b4;

	//0x01b8
	T_R1B8_BASIC_CG
	SIE_Register_01b8;

	//0x01bc
	T_R1BC_BASIC_CG
	SIE_Register_01bc;

	//0x01c0
	T_R1C0_BASIC_GRIDLINE
	SIE_Register_01c0;

	//0x01c4
	T_R1C4_BASIC_DVS
	SIE_Register_01c4;

	//0x01c8
	T_R1C8_BASIC_DVS
	SIE_Register_01c8;

	//0x01cc
	T_R1CC_DVS
	SIE_DVS_01cc;

	//0x01d0
	T_R1D0_RESERVED
	SIE_Register_01d0;

	//0x01d4
	T_R1D4_RESERVED
	SIE_Register_01d4;

	//0x01d8
	T_R1D8_RESERVED
	SIE_Register_01d8;

	//0x01dc
	T_R1DC_RESERVED
	SIE_Register_01dc;

	//0x01e0
	T_R1E0_ENGINE_STATUS
	SIE_Register_01e0;

	//0x01e4
	T_R1E4_ENGINE_STATUS
	SIE_Register_01e4;

	//0x01e8
	T_R1E8_STCS
	SIE_Register_01e8;

	//0x01ec
	T_R1EC_STCS_LA_CG
	SIE_Register_01ec;

	//0x01f0
	T_R1F0_STCS_LA_CG
	SIE_Register_01f0;

	//0x01f4
	T_R1F4_STCS_LA_IR
	SIE_Register_01f4;

	//0x01f8
	T_R1F8_STCS_VA_CG
	SIE_Register_01f8;

	//0x01fc
	T_R1FC_STCS_VIG
	SIE_Register_01fc;

	//0x0200
	T_R200_STCS_VIG
	SIE_Register_0200;

	//0x0204
	T_R204_STCS_VIG
	SIE_Register_0204;

	//0x0208
	T_R208_STCS_VIG
	SIE_Register_0208;

	//0x020c
	T_R20C_STCS_VIG
	SIE_Register_020c;

	//0x0210
	T_R210_STCS_VIG
	SIE_Register_0210;

	//0x0214
	T_R214_STCS_VIG
	SIE_Register_0214;

	//0x0218
	T_R218_STCS_VIG
	SIE_Register_0218;

	//0x021c
	T_R21C_STCS_VIG
	SIE_Register_021c;

	//0x0220
	T_R220_STCS_CA
	SIE_Register_0220;

	//0x0224
	T_R224_STCS_CA
	SIE_Register_0224;

	//0x0228
	T_R228_STCS_CA
	SIE_Register_0228;

	//0x022c
	T_R22C_STCS_CA
	SIE_Register_022c;

	//0x0230
	T_R230_STCS_CA
	SIE_Register_0230;

	//0x0234
	T_R234_STCS_CA
	SIE_Register_0234;

	//0x0238
	T_R238_STCS_CA
	SIE_Register_0238;

	//0x023c
	T_R23C_STCS_CA
	SIE_Register_023c;

	//0x0240
	T_R240_STCS_CA
	SIE_Register_0240;

	//0x0244
	T_R244_STCS_CA_IR
	SIE_Register_0244;

	//0x0248
	T_R248_STCS_LA
	SIE_Register_0248;

	//0x024c
	T_R24C_STCS_LA
	SIE_Register_024c;

	//0x0250
	T_R250_STCS_GAMMA
	SIE_Register_0250;

	//0x0254
	T_R254_STCS_GAMMA
	SIE_Register_0254;

	//0x0258
	T_R258_STCS_GAMMA
	SIE_Register_0258;

	//0x025c
	T_R25C_STCS_GAMMA
	SIE_Register_025c;

	//0x0260
	T_R260_STCS_GAMMA
	SIE_Register_0260;

	//0x0264
	T_R264_STCS_GAMMA
	SIE_Register_0264;

	//0x0268
	T_R268_STCS_GAMMA
	SIE_Register_0268;

	//0x026c
	T_R26C_STCS_GAMMA
	SIE_Register_026c;

	//0x0270
	T_R270_STCS_GAMMA
	SIE_Register_0270;

	//0x0274
	T_R274_STCS_GAMMA
	SIE_Register_0274;

	//0x0278
	T_R278_STCS_GAMMA
	SIE_Register_0278;

	//0x027c
	T_R27C_STCS_GAMMA
	SIE_Register_027c;

	//0x0280
	T_R280_STCS_GAMMA
	SIE_Register_0280;

	//0x0284
	T_R284_STCS_GAMMA
	SIE_Register_0284;

	//0x0288
	T_R288_STCS_GAMMA
	SIE_Register_0288;

	//0x028c
	T_R28C_STCS_GAMMA
	SIE_Register_028c;

	//0x0290
	T_R290_STCS_GAMMA
	SIE_Register_0290;

	//0x0294
	T_R294_STCS_GAMMA
	SIE_Register_0294;

	//0x0298
	T_R298_STCS_GAMMA
	SIE_Register_0298;

	//0x029c
	T_R29C_STCS_GAMMA
	SIE_Register_029c;

	//0x02a0
	T_R2A0_STCS_GAMMA
	SIE_Register_02a0;

	//0x02a4
	T_R2A4_STCS_GAMMA
	SIE_Register_02a4;

	//0x02a8
	T_R2A8_STCS_OB
	SIE_Register_02a8;

	//0x02ac
	T_R2AC_STCS_LA
	SIE_Register_02ac;

	//0x02b0
	T_R2B0_STCS_LA
	SIE_Register_02b0;

	//0x02b4
	T_R2B4_STCS_LA
	SIE_Register_02b4;

	//0x02b8
	T_R2B8_STCS_HISTO
	SIE_Register_02b8;

	//0x02bc
	T_R2BC_STCS_HISTO
	SIE_Register_02bc;

	//0x02c0
	T_R2C0_STCS_HISTO
	SIE_Register_02c0;

	//0x02c4
	T_R2C4_STCS_HISTO
	SIE_Register_02c4;

	//0x02c8
	T_R2C8_STCS_HISTO
	SIE_Register_02c8;

	//0x02cc
	T_R2CC_STCS_HISTO
	SIE_Register_02cc;

	//0x02d0
	T_R2D0_STCS_HISTO
	SIE_Register_02d0;

	//0x02d4
	T_R2D4_STCS_HISTO
	SIE_Register_02d4;

	//0x02d8
	T_R2D8_STCS_HISTO
	SIE_Register_02d8;

	//0x02dc
	T_R2DC_STCS_HISTO
	SIE_Register_02dc;

	//0x02e0
	T_R2E0_STCS_HISTO
	SIE_Register_02e0;

	//0x02e4
	T_R2E4_STCS_HISTO
	SIE_Register_02e4;

	//0x02e8
	T_R2E8_STCS_HISTO
	SIE_Register_02e8;

	//0x02ec
	T_R2EC_STCS_HISTO
	SIE_Register_02ec;

	//0x02f0
	T_R2F0_STCS_HISTO
	SIE_Register_02f0;

	//0x02f4
	T_R2F4_STCS_HISTO
	SIE_Register_02f4;

	//0x02f8
	T_R2F8_STCS_HISTO
	SIE_Register_02f8;

	//0x02fc
	T_R2FC_STCS_HISTO
	SIE_Register_02fc;

	//0x0300
	T_R300_STCS_HISTO
	SIE_Register_0300;

	//0x0304
	T_R304_STCS_HISTO
	SIE_Register_0304;

	//0x0308
	T_R308_STCS_HISTO
	SIE_Register_0308;

	//0x030c
	T_R30C_STCS_HISTO
	SIE_Register_030c;

	//0x0310
	T_R310_STCS_HISTO
	SIE_Register_0310;

	//0x0314
	T_R314_STCS_HISTO
	SIE_Register_0314;

	//0x0318
	T_R318_STCS_HISTO
	SIE_Register_0318;

	//0x031c
	T_R31C_STCS_HISTO
	SIE_Register_031c;

	//0x0320
	T_R320_STCS_HISTO
	SIE_Register_0320;

	//0x0324
	T_R324_STCS_HISTO
	SIE_Register_0324;

	//0x0328
	T_R328_STCS_HISTO
	SIE_Register_0328;

	//0x032c
	T_R32C_STCS_HISTO
	SIE_Register_032c;

	//0x0330
	T_R330_STCS_HISTO
	SIE_Register_0330;

	//0x0334
	T_R334_STCS_HISTO
	SIE_Register_0334;

	//0x0338
	T_R338_STCS_VA
	SIE_Register_0338;

	//0x033c
	T_R33C_STCS_VA
	SIE_Register_033c;

	//0x0340
	T_R340_DEBUG
	SIE_Register_0340;

	//0x0344
	T_R344_STCS_VA
	SIE_Register_0344;

	//0x0348
	T_R348_STCS_VA
	SIE_Register_0348;

	//0x034c
	T_R34C_STCS_VA
	SIE_Register_034c;

	//0x0350
	T_R350_STCS_VA
	SIE_Register_0350;

	//0x0354
	T_R354_STCS_VA
	SIE_Register_0354;

	//0x0358
	T_R358_STCS_VA
	SIE_Register_0358;

	//0x035c
	T_R35C_STCS_VA
	SIE_Register_035c;

	//0x0360
	T_R360_STCS_VA
	SIE_Register_0360;

	//0x0364
	T_R364_DEBUG
	SIE_Register_0364;

	//0x0368
	T_R368_DEBUG
	SIE_Register_0368;

	//0x036c
	T_R36C_STCS_VA
	SIE_Register_036c;

	//0x0370
	T_R370_STCS_VA
	SIE_Register_0370;

	//0x0374
	T_R374_STCS_VA
	SIE_Register_0374;

	//0x0378
	T_R378_STCS_VA
	SIE_Register_0378;

	//0x037c
	T_R37C_STCS_VA
	SIE_Register_037c;

	//0x0380
	T_R380_STCS_VA
	SIE_Register_0380;

	//0x0384
	T_R384_STCS_VA
	SIE_Register_0384;

	//0x0388
	T_R388_STCS_VA
	SIE_Register_0388;

	//0x038c
	T_R38C_STCS_VA
	SIE_Register_038c;

	//0x0390
	T_R390_STCS_VA
	SIE_Register_0390;

	//0x0394
	T_R394_STCS_VA
	SIE_Register_0394;

	//0x0398
	T_R398_STCS_VA
	SIE_Register_0398;

	//0x039c
	T_R39C_STCS_VA
	SIE_Register_039c;

	//0x03a0
	T_R3A0_STCS_VA
	SIE_Register_03a0;

	//0x03a4
	T_R3A4_STCS_VA
	SIE_Register_03a4;

	//0x03a8
	T_R3A8_STCS_VA
	SIE_Register_03a8;

	//0x03ac
	T_R3AC_STCS_VA
	SIE_Register_03ac;

	//0x03b0
	T_R3B0_STCS_VA
	SIE_Register_03b0;

	//0x03b4
	T_R3B4_STCS_ETH
	SIE_Register_03b4;

	//0x03b8
	T_R3B8_STCS_GAMMA
	SIE_Register_03b8;

	//0x03bc
	T_R3BC_STCS_GAMMA
	SIE_Register_03bc;

	//0x03c0
	T_R3C0_STCS_MD
	SIE_Register_03c0;

	//0x03c4
	T_R3C4_STCS_MD
	SIE_Register_03c4;

	//0x03c8
	T_R3C8_STCS_MD
	SIE_Register_03c8;

	//0x03cc
	T_R3CC_STCS_MD
	SIE_Register_03cc;

	//0x03d0
	T_R3D0_STCS_MD
	SIE_Register_03d0;

	//0x03d4
	T_R3D4_STCS_MD
	SIE_Register_03d4;

	//0x03d8
	T_R3D8_STCS_MD
	SIE_Register_03d8;

	//0x03dc
	T_R3DC_RESERVED
	SIE_Register_03dc;

	//0x03e0
	T_R3E0_RESERVED
	SIE_Register_03e0;

	//0x03e4
	T_R3E4_MD
	SIE_Register_03e4;

	//0x03e8
	T_R3E8_MD
	SIE_Register_03e8;

	//0x03ec
	T_R3EC_MD
	SIE_Register_03ec;

	//0x03f0
	T_R3F0_ADVANCED_PFPC
	SIE_Register_03f0;

	//0x03f4
	T_R3F4_ADVANCED_PFPC
	SIE_Register_03f4;

	//0x03f8
	T_R3F8_ADVANCED_PFPC
	SIE_Register_03f8;

	//0x03fc
	T_R3FC_ADVANCED_PFPC
	SIE_Register_03fc;

	//0x0400
	T_R400_ADVANCED_PFPC
	SIE_Register_0400;

	//0x0404
	T_R404_ADVANCED_PFPC
	SIE_Register_0404;

	//0x0408
	T_R408_ADVANCED_PFPC
	SIE_Register_0408;

	//0x040c
	T_R40C_ADVANCED_PFPC
	SIE_Register_040c;

	//0x0410
	T_R410_ADVANCED_PFPC
	SIE_Register_0410;

	//0x0414
	T_R414_ADVANCED_PFPC
	SIE_Register_0414;

	//0x0418
	T_R418_ADVANCED_PFPC
	SIE_Register_0418;

	//0x041c
	T_R41C_ADVANCED_PFPC
	SIE_Register_041c;

	//0x0420
	T_R420_ADVANCED_OBF
	SIE_Register_0420;

	//0x0424
	T_R424_ADVANCED_OBF
	SIE_Register_0424;

	//0x0428
	T_R428_ADVANCED_OBF
	SIE_Register_0428;

	//0x042c
	T_R42C_ADVANCED_OBF
	SIE_Register_042c;

	//0x0430
	T_R430_ADVANCED_OBF
	SIE_Register_0430;

	//0x0434
	T_R434_ADVANCED_OBF
	SIE_Register_0434;

	//0x0438
	T_R438_ADVANCED_OBF
	SIE_Register_0438;

	//0x043c
	T_R43C_ADVANCED_OBF
	SIE_Register_043c;

	//0x0440
	T_R440_ADVANCED_OBF
	SIE_Register_0440;

	//0x0444
	T_R444_ADVANCED_OBF
	SIE_Register_0444;

	//0x0448
	T_R448_ADVANCED_OBF
	SIE_Register_0448;

	//0x044c
	T_R44C_ADVANCED_OBF
	SIE_Register_044c;

	//0x0450
	T_R450_ADVANCED_OBF
	SIE_Register_0450;

	//0x0454
	T_R454_ADVANCED_OBF
	SIE_Register_0454;

	//0x0458
	T_R458_RESERVED
	SIE_Register_0458;

	//0x045c
	T_R45C_ADVANCED_OBP
	SIE_Register_045c;

	//0x0460
	T_R460_ADVANCED_OBP
	SIE_Register_0460;

	//0x0464
	T_R464_ADVANCED_OBP
	SIE_Register_0464;

	//0x0468
	T_R468_ADVANCED_OBP
	SIE_Register_0468;

	//0x046c
	T_R46C_ADVANCED_OBP
	SIE_Register_046c;

	//0x0470
	T_R470_ADVANCED_OBP
	SIE_Register_0470;

	//0x0474
	T_R474_ADVANCED_OBP
	SIE_Register_0474;

	//0x0478
	T_R478_ADVANCED_OBP
	SIE_Register_0478;

	//0x047c
	T_R47C_ADVANCED_OBP
	SIE_Register_047c;

	//0x0480
	T_R480_ADVANCED_OBP
	SIE_Register_0480;

	//0x0484
	T_R484_ADVANCED_OBP
	SIE_Register_0484;

	//0x0488
	T_R488_ADVANCED_OBP
	SIE_Register_0488;

	//0x048c
	T_R48C_ADVANCED_OBP
	SIE_Register_048c;

	//0x0490
	T_R490_ADVANCED_OBP
	SIE_Register_0490;

	//0x0494
	T_R494_ADVANCED_OBP
	SIE_Register_0494;

	//0x0498
	T_R498_ADVANCED_OBP
	SIE_Register_0498;

	//0x049c
	T_R49C_ADVANCED_OBP
	SIE_Register_049c;

	//0x04a0
	T_R4A0_ADVANCED_OBP
	SIE_Register_04a0;

	//0x04a4
	T_R4A4_ADVANCED_OBP
	SIE_Register_04a4;

	//0x04a8
	T_R4A8_ADVANCED_OBP
	SIE_Register_04a8;

	//0x04ac
	T_R4AC_ADVANCED_OBP
	SIE_Register_04ac;

	//0x04b0
	T_R4B0_ADVANCED_OBP
	SIE_Register_04b0;

	//0x04b4
	T_R4B4_ADVANCED_OBP
	SIE_Register_04b4;

	//0x04b8
	T_R4B8_ADVANCED_OBP
	SIE_Register_04b8;

	//0x04bc
	T_R4BC_ADVANCED_OBP
	SIE_Register_04bc;

	//0x04c0
	T_R4C0_ADVANCED_OBP
	SIE_Register_04c0;

	//0x04c4
	T_R4C4_ADVANCED_OBP
	SIE_Register_04c4;

	//0x04c8
	T_R4C8_ADVANCED_OBP
	SIE_Register_04c8;

	//0x04cc
	T_R4CC_ADVANCED_OBP
	SIE_Register_04cc;

	//0x04d0
	T_R4D0_ADVANCED_OBP
	SIE_Register_04d0;

	//0x04d4
	T_R4D4_ADVANCED_OBP
	SIE_Register_04d4;

	//0x04d8
	T_R4D8_ADVANCED_OBP
	SIE_Register_04d8;

	//0x04dc
	T_R4DC_ADVANCED_OBP
	SIE_Register_04dc;

	//0x04e0
	T_R4E0_ADVANCED_OBP
	SIE_Register_04e0;

	//0x04e4
	T_R4E4_ADVANCED_OBP
	SIE_Register_04e4;

	//0x04e8
	T_R4E8_ADVANCED_OBP
	SIE_Register_04e8;

	//0x04ec
	T_R4EC_ADVANCED_OBP
	SIE_Register_04ec;

	//0x04f0
	T_R4F0_ADVANCED_OBP
	SIE_Register_04f0;

	//0x04f4
	T_R4F4_ADVANCED_OBP
	SIE_Register_04f4;

	//0x04f8
	T_R4F8_ADVANCED_OBP
	SIE_Register_04f8;

	//0x04fc
	T_R4FC_ADVANCED_OBP
	SIE_Register_04fc;

	//0x0500
	T_R500_ADVANCED_OBP
	SIE_Register_0500;

	//0x0504
	T_R504_ADVANCED_OBP
	SIE_Register_0504;

	//0x0508
	T_R508_ADVANCED_OBP
	SIE_Register_0508;

	//0x050c
	T_R50C_ADVANCED_OBP
	SIE_Register_050c;

	//0x0510
	T_R510_ADVANCED_OBP
	SIE_Register_0510;

	//0x0514
	T_R514_ADVANCED_OBP
	SIE_Register_0514;

	//0x0518
	T_R518_ADVANCED_OBP
	SIE_Register_0518;

	//0x051c
	T_R51C_ADVANCED_OBP
	SIE_Register_051c;

	//0x0520
	T_R520_ADVANCED_OBP
	SIE_Register_0520;

	//0x0524
	T_R524_ADVANCED_OBP
	SIE_Register_0524;

	//0x0528
	T_R528_ADVANCED_OBP
	SIE_Register_0528;

	//0x052c
	T_R52C_ADVANCED_OBP
	SIE_Register_052c;

	//0x0530
	T_R530_ADVANCED_OBP
	SIE_Register_0530;

	//0x0534
	T_R534_ADVANCED_OBP
	SIE_Register_0534;

	//0x0538
	T_R538_ADVANCED_OBP
	SIE_Register_0538;

	//0x053c
	T_R53C_ADVANCED_OBP
	SIE_Register_053c;

	//0x0540
	T_R540_ADVANCED_OBP
	SIE_Register_0540;

	//0x0544
	T_R544_ADVANCED_OBP
	SIE_Register_0544;

	//0x0548
	T_R548_ADVANCED_OBP
	SIE_Register_0548;

	//0x054c
	T_R54C_ADVANCED_OBP
	SIE_Register_054c;

	//0x0550
	T_R550_ADVANCED_OBP
	SIE_Register_0550;

	//0x0554
	T_R554_ADVANCED_OBP
	SIE_Register_0554;

	//0x0558
	T_R558_ADVANCED_OBP
	SIE_Register_0558;

	//0x055c
	T_R55C_ADVANCED_OBP
	SIE_Register_055c;

	//0x0560
	T_R560_ADVANCED_OBP
	SIE_Register_0560;

	//0x0564
	T_R564_ADVANCED_OBP
	SIE_Register_0564;

	//0x0568
	T_R568_ADVANCED_OBP
	SIE_Register_0568;

	//0x056c
	T_R56C_ADVANCED_OBP
	SIE_Register_056c;

	//0x0570
	T_R570_ADVANCED_OBP
	SIE_Register_0570;

	//0x0574
	T_R574_ADVANCED_OBP
	SIE_Register_0574;

	//0x0578
	T_R578_ADVANCED_OBP
	SIE_Register_0578;

	//0x057c
	T_R57C_ADVANCED_OBP
	SIE_Register_057c;

	//0x0580
	T_R580_ADVANCED_OBP
	SIE_Register_0580;

	//0x0584
	T_R584_RESERVED
	SIE_Register_0584;

	//0x0588
	T_R588_RESERVED
	SIE_Register_0588;

	//0x058c
	T_R58C_RESERVED
	SIE_Register_058c;

	//0x0590
	T_R590_RESERVED
	SIE_Register_0590;

	//0x0594
	T_R594_RESERVED
	SIE_Register_0594;

	//0x0598
	T_R598_RESERVED
	SIE_Register_0598;

	//0x059c
	T_R59C_RESERVED
	SIE_Register_059c;

	//0x05a0
	T_R5A0_RESERVED
	SIE_Register_05a0;

	//0x05a4
	T_R5A4_RESERVED
	SIE_Register_05a4;

	//0x05a8
	T_R5A8_RESERVED
	SIE_Register_05a8;

	//0x05ac
	T_R5AC_RESERVED
	SIE_Register_05ac;

	//0x05b0
	T_R5B0_RESERVED
	SIE_Register_05b0;

	//0x05b4
	T_R5B0_RESERVED
	SIE_Register_05b4;

	//0x05b8
	T_R5B0_RESERVED
	SIE_Register_05b8;

	//0x05bc
	T_R5B0_RESERVED
	SIE_Register_05bc;

	//0x05c0
	T_R5C0_RESERVED
	SIE_Register_05c0;

	//0x05c4
	T_R54_RESERVED
	SIE_Register_054;

	//0x05c8
	T_R5C8_RESERVED
	SIE_Register_05c8;

	//0x05cc
	T_R5CC_RESERVED
	SIE_Register_05cc;

	//0x05d0
	T_R5D0_RESERVED
	SIE_Register_05d0;

	//0x05d4
	T_R5D4_RESERVED
	SIE_Register_05d4;

	//0x05d8
	T_R5D8_RESERVED
	SIE_Register_05d8;

	//0x05dc
	T_R5DC_RESERVED
	SIE_Register_05dc;

	//0x05e0
	T_R5E0_RESERVED
	SIE_Register_05e0;

	//0x05e4
	T_R5E4_RESERVED
	SIE_Register_05e4;

	//0x05e8
	T_R5E8_RESERVED
	SIE_Register_05e8;

	//0x05ec
	T_R5EC_RESERVED
	SIE_Register_05ec;

	//0x05f0
	T_R5F0_RESERVED
	SIE_Register_05f0;

	//0x05f4
	T_R5F4_RESERVED
	SIE_Register_05f4;

	//0x05f8
	T_R5F8_RESERVED
	SIE_Register_05f8;

	//0x05fc
	T_R5FC_RESERVED
	SIE_Register_05fc;

	//0x0600
	T_R600_RESERVED
	SIE_Register_0600;

	//0x0604
	T_R604_RESERVED
	SIE_Register_0604;

	//0x0608
	T_R608_RESERVED
	SIE_Register_0608;

	//0x060c
	T_R60C_RESERVED
	SIE_Register_060c;

	//0x0610
	T_R610_RESERVED
	SIE_Register_0610;

	//0x0614
	T_R614_RESERVED
	SIE_Register_0614;

	//0x0618
	T_R618_RESERVED
	SIE_Register_0618;

	//0x061c
	T_R61C_RESERVED
	SIE_Register_061c;

	//0x0620
	T_R620_RESERVED
	SIE_Register_0620;

	//0x0624
	T_R624_RESERVED
	SIE_Register_0624;

	//0x0628
	T_R628_RESERVED
	SIE_Register_0628;

	//0x062c
	T_R62C_RESERVED
	SIE_Register_062c;

	//0x0630
	T_R630_RESERVED
	SIE_Register_0630;

	//0x0634
	T_R634_RESERVED
	SIE_Register_0634;

	//0x0638
	T_R638_RESERVED
	SIE_Register_0638;

	//0x063c
	T_R63C_RESERVED
	SIE_Register_063c;

	//0x0640
	T_R640_RESERVED
	SIE_Register_0640;

	//0x0644
	T_R644_RESERVED
	SIE_Register_0644;

	//0x0648
	T_R648_RESERVED
	SIE_Register_0648;

	//0x064c
	T_R64C_RESERVED
	SIE_Register_064c;

	//0x0650
	T_R650_RESERVED
	SIE_Register_0650;

	//0x0654
	T_R654_RESERVED
	SIE_Register_0654;

	//0x0658
	T_R658_RESERVED
	SIE_Register_0658;

	//0x065c
	T_R65C_RESERVED
	SIE_Register_065c;

	//0x0660
	T_R660_RESERVED
	SIE_Register_0660;

	//0x0664
	T_R664_RESERVED
	SIE_Register_0664;

	//0x0668
	T_R668_RESERVED
	SIE_Register_0668;

	//0x066c
	T_R66C_RESERVED
	SIE_Register_066c;

	//0x0670
	T_R670_RESERVED
	SIE_Register_0670;

	//0x0674
	T_R674_RESERVED
	SIE_Register_0674;

	//0x0678
	T_R678_RESERVED
	SIE_Register_0678;

	//0x067c
	T_R67C_RESERVED
	SIE_Register_067c;

	//0x0680
	T_R680_RESERVED
	SIE_Register_0680;

	//0x0684
	T_R684_RESERVED
	SIE_Register_0684;

	//0x0688
	T_R688_RESERVED
	SIE_Register_0688;

	//0x068c
	T_R68C_RESERVED
	SIE_Register_068c;

	//0x0690
	T_R690_RESERVED
	SIE_Register_0690;

	//0x0694
	T_R694_RESERVED
	SIE_Register_0694;

	//0x0698
	T_R698_RESERVED
	SIE_Register_0698;

	//0x069c
	T_R69C_RESERVED
	SIE_Register_069c;

	//0x06a0
	T_R6A0_RESERVED
	SIE_Register_06a0;

	//0x06a4
	T_R6A4_RESERVED
	SIE_Register_06a4;

	//0x06a8
	T_R6A8_RESERVED
	SIE_Register_06a8;

	//0x06ac
	T_R6AC_RESERVED
	SIE_Register_06ac;

	//0x06b4
	T_R6B4_RESERVED
	SIE_Register_06b4;

	//0x06b8
	T_R6B8_RESERVED
	SIE_Register_06b8;

	//0x06bc
	T_R6BC_RESERVED
	SIE_Register_06bc;

	//0x06c0
	T_R6C0_RESERVED
	SIE_Register_06c0;

	//0x06c4
	T_R6C4_RESERVED
	SIE_Register_06c4;

	//0x06c8
	T_R6C8_RESERVED
	SIE_Register_06c8;

	//0x06cc
	T_R6CC_RESERVED
	SIE_Register_06cc;

	//0x06d0
	T_R6D0_RESERVED
	SIE_Register_06d0;

	//0x06d4
	T_R6D4_RESERVED
	SIE_Register_06d4;

	//0x06d8
	T_R6D8_RESERVED
	SIE_Register_06d8;

	//0x06dc
	T_R6DC_RESERVED
	SIE_Register_06dc;

	//0x06e0
	T_R6E0_RESERVED
	SIE_Register_06e0;

	//0x06e4
	T_R6E4_RESERVED
	SIE_Register_06e4;

	//0x06e8
	T_R6E8_RESERVED
	SIE_Register_06e8;

	//0x06ec
	T_R6EC_RESERVED
	SIE_Register_06ec;

	//0x06f0
	T_R6F0_RESERVED
	SIE_Register_06f0;

	//0x06f4
	T_R6F4_RESERVED
	SIE_Register_06f4;

	//0x06f8
	T_R6F8_RESERVED
	SIE_Register_06f8;

	//0x06fc
	T_R6FC_RESERVED
	SIE_Register_06fc;

	//0x0700
	T_R700_RESERVED
	SIE_Register_0700;

	//0x0704
	T_R704_RESERVED
	SIE_Register_0704;


	//0x0708
	T_R708_RESERVED
	SIE_Register_0708;

	//0x070c
	T_R70C_RESERVED
	SIE_Register_070c;

	//0x0700
	T_R710_RESERVED
	SIE_Register_0710;

	//0x0714
	T_R714_RESERVED
	SIE_Register_0714;

	//0x0718
	T_R718_RESERVED
	SIE_Register_0718;

	//0x071c
	T_R71C_RESERVED
	SIE_Register_071c;

	//0x0720
	T_R720_RESERVED
	SIE_Register_0720;

	//0x0724
	T_R724_RESERVED
	SIE_Register_0724;

	//0x0728
	T_R728_RESERVED
	SIE_Register_0728;

	//0x072c
	T_R72C_RESERVED
	SIE_Register_072c;

	//0x0730
	T_R730_RESERVED
	SIE_Register_0730;

	//0x0734
	T_R734_RESERVED
	SIE_Register_0734;

	//0x0738
	T_R738_RESERVED
	SIE_Register_0738;

	//0x073c
	T_R73C_RESERVED
	SIE_Register_073c;

	//0x0740
	T_R740_RESERVED
	SIE_Register_0740;

	//0x0744
	T_R744_RESERVED
	SIE_Register_0744;

	//0x0748
	T_R748_RESERVED
	SIE_Register_0748;

	//0x074c
	T_R74C_RESERVED
	SIE_Register_074c;

	//0x0750
	T_R750_RESERVED
	SIE_Register_0750;

	//0x0754
	T_R754_RESERVED
	SIE_Register_0754;

	//0x0758
	T_R758_RESERVED
	SIE_Register_0758;

	//0x075c
	T_R75C_RESERVED
	SIE_Register_075c;

	//0x0760
	T_R760_RESERVED
	SIE_Register_0760;

	//0x0764
	T_R764_RESERVED
	SIE_Register_0764;

	//0x0768
	T_R768_RESERVED
	SIE_Register_0768;

	//0x076c
	T_R76C_RESERVED
	SIE_Register_07c;

	//0x0770
	T_R770_RESERVED
	SIE_Register_0770;

	//0x0774
	T_R774_RESERVED
	SIE_Register_0774;

	//0x0778
	T_R778_RESERVED
	SIE_Register_0778;

	//0x077c
	T_R77C_RESERVED
	SIE_Register_077c;

	//0x0780
	T_R780_RESERVED
	SIE_Register_0780;

	//0x0784
	T_R784_RESERVED
	SIE_Register_0784;

	//0x0788
	T_R788_RESERVED
	SIE_Register_0788;

	//0x078c
	T_R78C_RESERVED
	SIE_Register_078c;

	//0x0790
	T_R790_RESERVED
	SIE_Register_0790;

	//0x0794
	T_R794_RESERVED
	SIE_Register_0794;

	//0x0798
	T_R798_RESERVED
	SIE_Register_0798;

	//0x079c
	T_R79C_RESERVED
	SIE_Register_079c;

	//0x07a0
	T_R7A0_RESERVED
	SIE_Register_07a0;

	//0x07a4
	T_R7A4_RESERVED
	SIE_Register_07a4;

	//0x07a8
	T_R7A8_RESERVED
	SIE_Register_07a8;

	//0x07ac
	T_R7AC_RESERVED
	SIE_Register_07ac;

	//0x07b0
	T_R7B0_RESERVED
	SIE_Register_07b0;

	//0x07b4
	T_R7B4_RESERVED
	SIE_Register_07b4;

	//0x07b8
	T_R7B8_RESERVED
	SIE_Register_07b8;

	//0x07bc
	T_R7BC_RESERVED
	SIE_Register_07bc;

	//0x07c0
	T_R7C0_RESERVED
	SIE_Register_07c0;

	//0x07c4
	T_R7C4_RESERVED
	SIE_Register_07c4;

	//0x07c8
	T_R7C8_RESERVED
	SIE_Register_07c8;

	//0x07cc
	T_R7CC_RESERVED
	SIE_Register_07cc;

	//0x07d0
	T_R7D0_RESERVED
	SIE_Register_07d0;

	//0x07d4
	T_R7D4_RESERVED
	SIE_Register_07d4;

	//0x07d8
	T_R7D8_RESERVED
	SIE_Register_07d8;

	//0x07dc
	T_R7DC_RESERVED
	SIE_Register_07dc;

	//0x07e0
	T_R7E0_RESERVED
	SIE_Register_07e0;

	//0x07e4
	T_R7E4_RESERVED
	SIE_Register_07e4;

	//0x07e8
	T_R7E8_RESERVED
	SIE_Register_07e8;

	//0x07ec
	T_R7EC_RESERVED
	SIE_Register_07ec;

	//0x07f0
	T_R7F0_RESERVED
	SIE_Register_07f0;

	//0x07f4
	T_R7F4_RESERVED
	SIE_Register_07f4;

	//0x07f8
	T_R7F8_RESERVED
	SIE_Register_07f8;

	//0x07fc
	T_R7FC_RESERVED
	SIE_Register_07fc;

	//0x0800
	T_R800_RESERVED
	SIE_Register_0800;

	//0x0804
	T_R804_RAWENC_CTRL
	SIE_Register_0804;

	//0x0808
	T_R808_RESERVED
	SIE_Register_0808;

	//0x080c
	T_R80C_RESERVED
	SIE_Register_080C;

	//0x0810
	T_R810_RESERVED
	SIE_Register_0810;

	//0x0814
	T_R814_RAWENC_BRC
	SIE_Register_0814;

	//0x0818
	T_R818_RESERVED
	SIE_Register_0818;

	//0x081c
	T_R81C_RESERVED
	SIE_Register_081c;

	//0x0820
	T_R820_RESERVED
	SIE_Register_0820;

	//0x0824
	T_R824_RAWENC_GAMMA
	SIE_Register_0824;

	//0x0828
	T_R828_RAWENC_GAMMA
	SIE_Register_0828;

	//0x082C
	T_R82C_RAWENC_GAMMA
	SIE_Register_082C;

	//0x0830
	T_R830_RAWENC_GAMMA
	SIE_Register_0830;

	//0x0834
	T_R834_RAWENC_GAMMA
	SIE_Register_0834;

	//0x0838
	T_R838_RAWENC_GAMMA
	SIE_Register_0838;

	//0x083C
	T_R83C_RAWENC_GAMMA
	SIE_Register_083C;

	//0x0840
	T_R840_RAWENC_GAMMA
	SIE_Register_0840;

	//0x0844
	T_R844_RAWENC_GAMMA
	SIE_Register_0844;

	//0x0848
	T_R848_RAWENC_GAMMA
	SIE_Register_0848;

	//0x084C
	T_R84C_RAWENC_GAMMA
	SIE_Register_084C;

	//0x0850
	T_R850_RAWENC_GAMMA
	SIE_Register_0850;

	//0x0854
	T_R854_RAWENC_GAMMA
	SIE_Register_0854;

	//0x0858
	T_R858_RAWENC_GAMMA
	SIE_Register_0858;

	//0x085C
	T_R85C_RAWENC_GAMMA
	SIE_Register_085C;

	//0x0860
	T_R860_RAWENC_GAMMA
	SIE_Register_0860;

	//0x0864
	T_R864_RAWENC_GAMMA
	SIE_Register_0864;

	//0x0868
	T_R868_RAWENC_DCT_QTBL_IDX
	SIE_Register_0868;

	//0x086C
	T_R86C_RAWENC_DCT_QTBL_IDX
	SIE_Register_086C;

	//0x0870
	T_R870_RAWENC_DCT_LEVEL_TH
	SIE_Register_0870;

} NT96680_SIE_REG_STRUCT;

//This file should be included only once//#endif




#endif
