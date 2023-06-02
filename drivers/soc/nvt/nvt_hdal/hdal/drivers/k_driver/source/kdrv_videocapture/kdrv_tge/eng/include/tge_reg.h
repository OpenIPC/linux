/*
    TGE register related header file.

    @file       tge_reg.h
    @ingroup    mIIPPTGE

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _TGE_NT96660_H_
#define _TGE_NT96660_H_


#include "tge_platform.h"
#if 0
//#define _TGE_REG_BASE_ADDR       IOADDR_TGE_REG_BASE         //(0xC0CC0000) in IOAddress.h
extern UINT32 _TGE_REG_BASE_ADDR;
#define TGE_REG_ADDR(ofs)        (_TGE_REG_BASE_ADDR+(ofs))
#define TGE_SETREG(ofs, value)   iowrite32(value, (void*)(_TGE_REG_BASE_ADDR + ofs))
#define TGE_GETREG(ofs)          ioread32((void*)(_TGE_REG_BASE_ADDR + ofs))
#endif
/*
    TGE_RST              :    [0x0, 0x1],            bits : 0
    FLSH_MSH_LOAD        :    [0x0, 0x1],            bits : 1
    VD_LOAD              :    [0x0, 0x1],            bits : 8
    VD2_LOAD             :    [0x0, 0x1],            bits : 9
    VD3_LOAD             :    [0x0, 0x1],            bits :10
    VD4_LOAD             :    [0x0, 0x1],            bits :11
    VD5_LOAD             :    [0x0, 0x1],            bits :12
    VD6_LOAD             :    [0x0, 0x1],            bits :13
    VD7_LOAD             :    [0x0, 0x1],            bits :14
    VD8_LOAD             :    [0x0, 0x1],            bits :15
    VD_RST               :    [0x0, 0x1],            bits :16
    VD2_RST              :    [0x0, 0x1],            bits :17
    VD3_RST              :    [0x0, 0x1],            bits :18
    VD4_RST              :    [0x0, 0x1],            bits :19
    VD5_RST              :    [0x0, 0x1],            bits :20
    VD6_RST              :    [0x0, 0x1],            bits :21
    VD7_RST              :    [0x0, 0x1],            bits :22
    VD8_RST              :    [0x0, 0x1],            bits :23
    MODE_SEL             :    [0x0, 0x1],            bits :24
    MODE2_SEL            :    [0x0, 0x1],            bits :25
    MODE3_SEL            :    [0x0, 0x1],            bits :26
    MODE4_SEL            :    [0x0, 0x1],            bits :27
    MODE5_SEL            :    [0x0, 0x1],            bits :28
    MODE6_SEL            :    [0x0, 0x1],            bits :29
    MODE7_SEL            :    [0x0, 0x1],            bits :30
    MODE8_SEL            :    [0x0, 0x1],            bits :31
*/
#define R0_ENGINE_CONTROL_OFS 0x0000
REGDEF_BEGIN(R0_ENGINE_CONTROL)
REGDEF_BIT(TGE_RST,        1)
REGDEF_BIT(FLSH_MSH_LOAD,        1)
REGDEF_BIT(,        6)
REGDEF_BIT(VD_LOAD,        1)
REGDEF_BIT(VD2_LOAD,        1)
REGDEF_BIT(VD3_LOAD,        1)
REGDEF_BIT(VD4_LOAD,        1)
REGDEF_BIT(VD5_LOAD,        1)
REGDEF_BIT(VD6_LOAD,        1)
REGDEF_BIT(VD7_LOAD,        1)
REGDEF_BIT(VD8_LOAD,        1)
REGDEF_BIT(VD_RST,        1)
REGDEF_BIT(VD2_RST,        1)
REGDEF_BIT(VD3_RST,        1)
REGDEF_BIT(VD4_RST,        1)
REGDEF_BIT(VD5_RST,        1)
REGDEF_BIT(VD6_RST,        1)
REGDEF_BIT(VD7_RST,        1)
REGDEF_BIT(VD8_RST,        1)
REGDEF_BIT(MODE_SEL,        1)
REGDEF_BIT(MODE2_SEL,        1)
REGDEF_BIT(MODE3_SEL,        1)
REGDEF_BIT(MODE4_SEL,        1)
REGDEF_BIT(MODE5_SEL,        1)
REGDEF_BIT(MODE6_SEL,        1)
REGDEF_BIT(MODE7_SEL,        1)
REGDEF_BIT(MODE8_SEL,        1)
REGDEF_END(R0_ENGINE_CONTROL)



/*
    VD_PAUSE             :    [0x0, 0x1],            bits : 0_0
    HD_PAUSE             :    [0x0, 0x1],            bits : 1_1
    VD2_PAUSE            :    [0x0, 0x1],            bits : 2_2
    HD2_PAUSE            :    [0x0, 0x1],            bits : 3_3
    VD3_PAUSE            :    [0x0, 0x1],            bits : 4_4
    HD3_PAUSE            :    [0x0, 0x1],            bits : 5_5
    VD4_PAUSE            :    [0x0, 0x1],            bits : 6_6
    HD4_PAUSE            :    [0x0, 0x1],            bits : 7_7
    VD5_PAUSE            :    [0x0, 0x1],            bits : 8_8
    HD5_PAUSE            :    [0x0, 0x1],            bits : 9_9
    VD6_PAUSE            :    [0x0, 0x1],            bits :10_10
    HD6_PAUSE            :    [0x0, 0x1],            bits :11_11
    VD7_PAUSE            :    [0x0, 0x1],            bits :12_12
    HD7_PAUSE            :    [0x0, 0x1],            bits :13_13
    VD8_PAUSE            :    [0x0, 0x1],            bits :14_14
    HD8_PAUSE            :    [0x0, 0x1],            bits :15_15
    VD_HD_IN_SEL         :    [0x0, 0x1],            bits :16_16
    VD2_HD2_IN_SEL       :    [0x0, 0x1],            bits :17_17
    VD3_HD3_IN_SEL       :    [0x0, 0x1],            bits :18_18
    VD4_HD4_IN_SEL       :    [0x0, 0x1],            bits :19_19
    SN_XVS_OUTSWAP       :    [0x0, 0x1],            bits :20_20
    SN2_XVS_OUTSWAP      :    [0x0, 0x1],            bits :21_21
    SN3_XVS_OUTSWAP      :    [0x0, 0x1],            bits :22_22
    SN4_XVS_OUTSWAP      :    [0x0, 0x1],            bits :23_23
    FLSH_MSH_VD_HD_IN_SEL:    [0x0, 0x1],           bits :29_28
*/
#define R4_ENGINE_CONTROL_OFS 0x0004
REGDEF_BEGIN(R4_ENGINE_CONTROL)
REGDEF_BIT(VD_PAUSE,        1)
REGDEF_BIT(HD_PAUSE,        1)
REGDEF_BIT(VD2_PAUSE,        1)
REGDEF_BIT(HD2_PAUSE,        1)
REGDEF_BIT(VD3_PAUSE,        1)
REGDEF_BIT(HD3_PAUSE,        1)
REGDEF_BIT(VD4_PAUSE,        1)
REGDEF_BIT(HD4_PAUSE,        1)
REGDEF_BIT(VD5_PAUSE,        1)
REGDEF_BIT(HD5_PAUSE,        1)
REGDEF_BIT(VD6_PAUSE,        1)
REGDEF_BIT(HD6_PAUSE,        1)
REGDEF_BIT(VD7_PAUSE,        1)
REGDEF_BIT(HD7_PAUSE,        1)
REGDEF_BIT(VD8_PAUSE,        1)
REGDEF_BIT(HD8_PAUSE,        1)
REGDEF_BIT(VD_HD_IN_SEL,        1)
REGDEF_BIT(VD2_HD2_IN_SEL,        1)
REGDEF_BIT(VD3_HD3_IN_SEL,        1)
REGDEF_BIT(VD4_HD4_IN_SEL,        1)
REGDEF_BIT(SN_XVS_OUTSWAP,        1)
REGDEF_BIT(SN2_XVS_OUTSWAP,        1)
REGDEF_BIT(SN3_XVS_OUTSWAP,        1)
REGDEF_BIT(SN4_XVS_OUTSWAP,        1)
REGDEF_BIT(,        4)
REGDEF_BIT(FLSH_MSH_VD_HD_IN_SEL,      2)
REGDEF_END(R4_ENGINE_CONTROL)


/*
    VD_PHASE             :    [0x0, 0x1],            bits : 0_0
    HD_PHASE             :    [0x0, 0x1],            bits : 1_1
    VD_INV               :    [0x0, 0x1],            bits : 2_2
    HD_INV               :    [0x0, 0x1],            bits : 3_3
    VD2_PHASE            :    [0x0, 0x1],            bits : 4_4
    HD2_PHASE            :    [0x0, 0x1],            bits : 5_5
    VD2_INV              :    [0x0, 0x1],            bits : 6_6
    HD2_INV              :    [0x0, 0x1],            bits : 7_7
    VD3_PHASE            :    [0x0, 0x1],            bits : 8_4
    HD3_PHASE            :    [0x0, 0x1],            bits : 9_5
    VD3_INV              :    [0x0, 0x1],            bits :10_10
    HD3_INV              :    [0x0, 0x1],            bits :11_11
    VD4_PHASE            :    [0x0, 0x1],            bits :12_12
    HD4_PHASE            :    [0x0, 0x1],            bits :13_13
    VD4_INV              :    [0x0, 0x1],            bits :14_14
    HD4_INV              :    [0x0, 0x1],            bits :15_15
    VD5_PHASE            :    [0x0, 0x1],            bits :16_16
    HD5_PHASE            :    [0x0, 0x1],            bits :17_17
    VD5_INV              :    [0x0, 0x1],            bits :18_18
    HD5_INV              :    [0x0, 0x1],            bits :19_19
    VD6_PHASE            :    [0x0, 0x1],            bits :20_20
    HD6_PHASE            :    [0x0, 0x1],            bits :21_21
    VD6_INV              :    [0x0, 0x1],            bits :22_22
    HD6_INV              :    [0x0, 0x1],            bits :23_23
    VD7_PHASE            :    [0x0, 0x1],            bits :24_24
    HD7_PHASE            :    [0x0, 0x1],            bits :25_25
    VD7_INV              :    [0x0, 0x1],            bits :26_26
    HD7_INV              :    [0x0, 0x1],            bits :27_27
    VD8_PHASE            :    [0x0, 0x1],            bits :28_28
    HD8_PHASE            :    [0x0, 0x1],            bits :29_29
    VD8_INV              :    [0x0, 0x1],            bits :30_30
    HD8_INV              :    [0x0, 0x1],            bits :31_31
*/
#define R8_ENGINE_CONTROL_OFS 0x0008
REGDEF_BEGIN(R8_ENGINE_CONTROL)
REGDEF_BIT(VD_PHASE,        1)
REGDEF_BIT(HD_PHASE,        1)
REGDEF_BIT(VD_INV,        1)
REGDEF_BIT(HD_INV,        1)
REGDEF_BIT(VD2_PHASE,        1)
REGDEF_BIT(HD2_PHASE,        1)
REGDEF_BIT(VD2_INV,        1)
REGDEF_BIT(HD2_INV,        1)
REGDEF_BIT(VD3_PHASE,        1)
REGDEF_BIT(HD3_PHASE,        1)
REGDEF_BIT(VD3_INV,        1)
REGDEF_BIT(HD3_INV,        1)
REGDEF_BIT(VD4_PHASE,        1)
REGDEF_BIT(HD4_PHASE,        1)
REGDEF_BIT(VD4_INV,        1)
REGDEF_BIT(HD4_INV,        1)
REGDEF_BIT(VD5_PHASE,        1)
REGDEF_BIT(HD5_PHASE,        1)
REGDEF_BIT(VD5_INV,        1)
REGDEF_BIT(HD5_INV,        1)
REGDEF_BIT(VD6_PHASE,        1)
REGDEF_BIT(HD6_PHASE,        1)
REGDEF_BIT(VD6_INV,        1)
REGDEF_BIT(HD6_INV,        1)
REGDEF_BIT(VD7_PHASE,        1)
REGDEF_BIT(HD7_PHASE,        1)
REGDEF_BIT(VD7_INV,        1)
REGDEF_BIT(HD7_INV,        1)
REGDEF_BIT(VD8_PHASE,        1)
REGDEF_BIT(HD8_PHASE,        1)
REGDEF_BIT(VD8_INV,        1)
REGDEF_BIT(HD8_INV,        1)
REGDEF_END(R8_ENGINE_CONTROL)


/*
    VDTOSIE_SRC          :    [0x0, 0x1],            bits : 0_0
    VDTOSIE2_SRC         :    [0x0, 0x1],            bits : 4_4
*/
#define RC_ENGINE_CONTROL_OFS 0x000c
REGDEF_BEGIN(RC_ENGINE_CONTROL)
REGDEF_BIT(VDTOSIE_SRC,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(VDTOSIE3_SRC,        1)
REGDEF_END(RC_ENGINE_CONTROL)


/*
    FLSH_EXT_TRG_INV     :    [0x0, 0x1],            bits : 0_0
    MSH_EXT_TRG_INV      :    [0x0, 0x1],            bits : 1_1
    FLSH_CTRL_INV        :    [0x0, 0x1],            bits : 2_2
    MSH_A_CTRL_INV       :    [0x0, 0x1],            bits : 3_3
    MSH_B_CTRL_INV       :    [0x0, 0x1],            bits : 4_4
    MSH_A_SWAP           :    [0x0, 0x1],            bits : 5_5
    MSH_B_SWAP           :    [0x0, 0x1],            bits : 6_6
    MSH_A_CTRL_MODE      :    [0x0, 0x1],            bits : 7_7
    MSH_B_CTRL_MODE      :    [0x0, 0x1],            bits : 8_8
*/
#define R10_ENGINE_CONTROL_OFS 0x0010
REGDEF_BEGIN(R10_ENGINE_CONTROL)
REGDEF_BIT(FLSH_EXT_TRG_INV,        1)
REGDEF_BIT(MSH_EXT_TRG_INV,        1)
REGDEF_BIT(FLSH_CTRL_INV,        1)
REGDEF_BIT(MSH_A_CTRL_INV,        1)
REGDEF_BIT(MSH_B_CTRL_INV,        1)
REGDEF_BIT(MSH_A_SWAP,        1)
REGDEF_BIT(MSH_B_SWAP,        1)
REGDEF_BIT(MSH_A_CTRL_MODE,        1)
REGDEF_BIT(MSH_B_CTRL_MODE,        1)
REGDEF_END(R10_ENGINE_CONTROL)


/*
    INTE_VD              :    [0x0, 0x1],            bits : 0_0
    INTE_VD2             :    [0x0, 0x1],            bits : 1_1
    INTE_VD3             :    [0x0, 0x1],            bits : 2_2
    INTE_VD4             :    [0x0, 0x1],            bits : 3_3
    INTE_VD5             :    [0x0, 0x1],            bits : 4_4
    INTE_VD6             :    [0x0, 0x1],            bits : 5_5
    INTE_VD7             :    [0x0, 0x1],            bits : 6_6
    INTE_VD8             :    [0x0, 0x1],            bits : 7_7
    INTE_VD_BP           :    [0x0, 0x1],            bits : 8_8
    INTE_VD2_BP          :    [0x0, 0x1],            bits : 9_9
    INTE_VD3_BP          :    [0x0, 0x1],            bits :10_10
    INTE_VD4_BP          :    [0x0, 0x1],            bits :11_11
    INTE_VD5_BP          :    [0x0, 0x1],            bits :12_12
    INTE_VD6_BP          :    [0x0, 0x1],            bits :13_13
    INTE_VD7_BP          :    [0x0, 0x1],            bits :14_14
    INTE_VD8_BP          :    [0x0, 0x1],            bits :15_15
    INTE_FLSH_TRG        :    [0x0, 0x1],            bits :16_16
    INTE_MSHA_CLOSE_TRG  :    [0x0, 0x1],            bits :17_17
    INTE_MSHA_OPEN_TRG   :    [0x0, 0x1],            bits :18_18
    INTE_MSHB_CLOSE_TRG  :    [0x0, 0x1],            bits :19_19
    INTE_MSHB_OPEN_TRG   :    [0x0, 0x1],            bits :20_20
    INTE_FLSH_END        :    [0x0, 0x1],            bits :21_21
    INTE_MSHA_CLOSE_END  :    [0x0, 0x1],            bits :22_22
    INTE_MSHA_OPEN_END   :    [0x0, 0x1],            bits :23_23
    INTE_MSHB_CLOSE_END  :    [0x0, 0x1],            bits :24_24
    INTE_MSHB_OPEN_END   :    [0x0, 0x1],            bits :25_25
*/
#define R14_INTERRUPT_EN_OFS 0x0014
REGDEF_BEGIN(R14_INTERRUPT_EN)
REGDEF_BIT(INTE_VD,        1)
REGDEF_BIT(INTE_VD2,        1)
REGDEF_BIT(INTE_VD3,        1)
REGDEF_BIT(INTE_VD4,        1)
REGDEF_BIT(INTE_VD5,        1)
REGDEF_BIT(INTE_VD6,        1)
REGDEF_BIT(INTE_VD7,        1)
REGDEF_BIT(INTE_VD8,        1)
REGDEF_BIT(INTE_VD_BP,        1)
REGDEF_BIT(INTE_VD2_BP,        1)
REGDEF_BIT(INTE_VD3_BP,        1)
REGDEF_BIT(INTE_VD4_BP,        1)
REGDEF_BIT(INTE_VD5_BP,        1)
REGDEF_BIT(INTE_VD6_BP,        1)
REGDEF_BIT(INTE_VD7_BP,        1)
REGDEF_BIT(INTE_VD8_BP,        1)
REGDEF_BIT(INTE_FLSH_TRG,        1)
REGDEF_BIT(INTE_MSHA_CLOSE_TRG,        1)
REGDEF_BIT(INTE_MSHA_OPEN_TRG,        1)
REGDEF_BIT(INTE_MSHB_CLOSE_TRG,        1)
REGDEF_BIT(INTE_MSHB_OPEN_TRG,        1)
REGDEF_BIT(INTE_FLSH_END,        1)
REGDEF_BIT(INTE_MSHA_CLOSE_END,        1)
REGDEF_BIT(INTE_MSHA_OPEN_END,        1)
REGDEF_BIT(INTE_MSHB_CLOSE_END,        1)
REGDEF_BIT(INTE_MSHB_OPEN_END,        1)
REGDEF_END(R14_INTERRUPT_EN)


/*
    INT_VD               :    [0x0, 0x1],            bits : 0_0
    INT_VD2              :    [0x0, 0x1],            bits : 1_1
    INT_VD3              :    [0x0, 0x1],            bits : 2_2
    INT_VD4              :    [0x0, 0x1],            bits : 3_3
    INT_VD5              :    [0x0, 0x1],            bits : 4_4
    INT_VD6              :    [0x0, 0x1],            bits : 5_5
    INT_VD7              :    [0x0, 0x1],            bits : 6_6
    INT_VD8              :    [0x0, 0x1],            bits : 7_7
    INT_VD_BP            :    [0x0, 0x1],            bits : 8_8
    INT_VD2_BP           :    [0x0, 0x1],            bits : 9_9
    INT_VD3_BP           :    [0x0, 0x1],            bits :10_10
    INT_VD4_BP           :    [0x0, 0x1],            bits :11_11
    INT_VD5_BP           :    [0x0, 0x1],            bits :12_12
    INT_VD6_BP           :    [0x0, 0x1],            bits :13_13
    INT_VD7_BP           :    [0x0, 0x1],            bits :14_14
    INT_VD8_BP           :    [0x0, 0x1],            bits :15_15
    INT_FLSH_TRG         :    [0x0, 0x1],            bits :16_16
    INT_MSHA_CLOSE_TRG   :    [0x0, 0x1],            bits :17_17
    INT_MSHA_OPEN_TRG    :    [0x0, 0x1],            bits :18_18
    INT_MSHB_CLOSE_TRG   :    [0x0, 0x1],            bits :19_19
    INT_MSHB_OPEN_TRG    :    [0x0, 0x1],            bits :20_20
    INT_FLSH_END         :    [0x0, 0x1],            bits :21_21
    INT_MSHA_CLOSE_END   :    [0x0, 0x1],            bits :22_22
    INT_MSHA_OPEN_END    :    [0x0, 0x1],            bits :23_23
    INT_MSHB_CLOSE_END   :    [0x0, 0x1],            bits :24_24
    INT_MSHB_OPEN_END    :    [0x0, 0x1],            bits :25_25
*/
#define R18_INTERRUPT_OFS 0x0018
REGDEF_BEGIN(R18_INTERRUPT)
REGDEF_BIT(INT_VD,        1)
REGDEF_BIT(INT_VD2,        1)
REGDEF_BIT(INT_VD3,        1)
REGDEF_BIT(INT_VD4,        1)
REGDEF_BIT(INT_VD5,        1)
REGDEF_BIT(INT_VD6,        1)
REGDEF_BIT(INT_VD7,        1)
REGDEF_BIT(INT_VD8,        1)
REGDEF_BIT(INT_VD_BP,        1)
REGDEF_BIT(INT_VD2_BP,        1)
REGDEF_BIT(INT_VD3_BP,        1)
REGDEF_BIT(INT_VD4_BP,        1)
REGDEF_BIT(INT_VD5_BP,        1)
REGDEF_BIT(INT_VD6_BP,        1)
REGDEF_BIT(INT_VD7_BP,        1)
REGDEF_BIT(INT_VD8_BP,        1)
REGDEF_BIT(INT_FLSH_TRG,        1)
REGDEF_BIT(INT_MSHA_CLOSE_TRG,        1)
REGDEF_BIT(INT_MSHA_OPEN_TRG,        1)
REGDEF_BIT(INT_MSHB_CLOSE_TRG,        1)
REGDEF_BIT(INT_MSHB_OPEN_TRG,        1)
REGDEF_BIT(INT_FLSH_END,        1)
REGDEF_BIT(INT_MSHA_CLOSE_END,        1)
REGDEF_BIT(INT_MSHA_OPEN_END,        1)
REGDEF_BIT(INT_MSHB_CLOSE_END,        1)
REGDEF_BIT(INT_MSHB_OPEN_END,        1)
REGDEF_END(R18_INTERRUPT)


/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R1C_RESERVED_OFS 0x001c
REGDEF_BEGIN(R1C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R1C_RESERVED)


/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R20_RESERVED_OFS 0x0020
REGDEF_BEGIN(R20_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R20_RESERVED)


/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R24_RESERVED_OFS 0x0024
REGDEF_BEGIN(R24_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R24_RESERVED)


/*
    VD_PERIOD:           [0x0, 0xfffffff],           bits : 27_0
*/
#define R28_VERTICAL_SYNC_OFS 0x0028
REGDEF_BEGIN(R28_VERTICAL_SYNC)
REGDEF_BIT(VD_PERIOD,       28)
REGDEF_END(R28_VERTICAL_SYNC)


/*
    VD_ASSERT:           [0x0, 0xfffffff],           bits : 27_0
*/
#define R2C_VERTICAL_SYNC_OFS 0x002c
REGDEF_BEGIN(R2C_VERTICAL_SYNC)
REGDEF_BIT(VD_ASSERT,       28)
REGDEF_END(R2C_VERTICAL_SYNC)


/*
    VD_FRONTBLANK:       [0x0, 0xfffffff],           bits : 27_0
*/
#define R30_VERTICAL_SYNC_OFS 0x0030
REGDEF_BEGIN(R30_VERTICAL_SYNC)
REGDEF_BIT(VD_FRONTBLANK,       28)
REGDEF_END(R30_VERTICAL_SYNC)


/*
    HD_PERIOD:           [0x0, 0xffff],              bits : 15_0
    HD_ASSERT:           [0x0, 0xffff],              bits : 31_16
*/
#define R34_HORIZONTAL_SYNC_OFS 0x0034
REGDEF_BEGIN(R34_HORIZONTAL_SYNC)
REGDEF_BIT(HD_PERIOD,       16)
REGDEF_BIT(HD_ASSERT,       16)
REGDEF_END(R34_HORIZONTAL_SYNC)


/*
    HD_COUNT:            [0x0, 0x7ffff],             bits : 18_0
*/
#define R38_HORIZONTAL_SYNC_OFS 0x0038
REGDEF_BEGIN(R38_HORIZONTAL_SYNC)
REGDEF_BIT(HD_COUNT,       19)
REGDEF_END(R38_HORIZONTAL_SYNC)


/*
    VD_BP:               [0x0, 0xfffffff],           bits : 27_0
*/
#define R3C_BREAKPOINT_OFS 0x003c
REGDEF_BEGIN(R3C_BREAKPOINT)
REGDEF_BIT(VD_BP,       28)
REGDEF_END(R3C_BREAKPOINT)

/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R40_RESERVED_OFS 0x0040
REGDEF_BEGIN(R40_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R40_RESERVED)

/*
    VD2_PERIOD:          [0x0, 0xfffffff],           bits : 27_0
*/
#define R44_VERTICAL_SYNC_OFS 0x0044
REGDEF_BEGIN(R44_VERTICAL_SYNC)
REGDEF_BIT(VD2_PERIOD,       28)
REGDEF_END(R44_VERTICAL_SYNC)


/*
    VD2_ASSERT:          [0x0, 0xfffffff],           bits : 27_0
*/
#define R48_VERTICAL_SYNC_OFS 0x0048
REGDEF_BEGIN(R48_VERTICAL_SYNC)
REGDEF_BIT(VD2_ASSERT,       28)
REGDEF_END(R48_VERTICAL_SYNC)


/*
    VD2_FRONTBLANK:      [0x0, 0xfffffff],           bits : 27_0
*/
#define R4C_VERTICAL_SYNC_OFS 0x004c
REGDEF_BEGIN(R4C_VERTICAL_SYNC)
REGDEF_BIT(VD2_FRONTBLANK,       28)
REGDEF_END(R4C_VERTICAL_SYNC)


/*
    HD2_PERIOD:          [0x0, 0xffff],              bits : 15_0
    HD2_ASSERT:          [0x0, 0xffff],              bits : 31_16
*/
#define R50_HORIZONTAL_SYNC_OFS 0x0050
REGDEF_BEGIN(R50_HORIZONTAL_SYNC)
REGDEF_BIT(HD2_PERIOD,       16)
REGDEF_BIT(HD2_ASSERT,       16)
REGDEF_END(R50_HORIZONTAL_SYNC)


/*
    HD2_COUNT:           [0x0, 0x7ffff],             bits : 18_0
*/
#define R54_HORIZONTAL_SYNC_OFS 0x0054
REGDEF_BEGIN(R54_HORIZONTAL_SYNC)
REGDEF_BIT(HD2_COUNT,       19)
REGDEF_END(R54_HORIZONTAL_SYNC)


/*
    VD2_BP:              [0x0, 0xfffffff],           bits : 27_0
*/
#define R58_BREAKPOINT_OFS 0x0058
REGDEF_BEGIN(R58_BREAKPOINT)
REGDEF_BIT(VD2_BP,       28)
REGDEF_END(R58_BREAKPOINT)

/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R5C_RESERVED_OFS 0x005c
REGDEF_BEGIN(R5C_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R5C_RESERVED)


/*
    VD3_PERIOD:          [0x0, 0xfffffff],           bits : 27_0
*/
#define R60_VERTICAL_SYNC_OFS 0x0060
REGDEF_BEGIN(R60_VERTICAL_SYNC)
REGDEF_BIT(VD3_PERIOD,       28)
REGDEF_END(R60_VERTICAL_SYNC)


/*
    VD3_ASSERT:          [0x0, 0xfffffff],           bits : 27_0
*/
#define R64_VERTICAL_SYNC_OFS 0x0064
REGDEF_BEGIN(R64_VERTICAL_SYNC)
REGDEF_BIT(VD3_ASSERT,       28)
REGDEF_END(R64_VERTICAL_SYNC)


/*
    VD3_FRONTBLANK:      [0x0, 0xfffffff],           bits : 27_0
*/
#define R68_VERTICAL_SYNC_OFS 0x0068
REGDEF_BEGIN(R68_VERTICAL_SYNC)
REGDEF_BIT(VD3_FRONTBLANK,       28)
REGDEF_END(R68_VERTICAL_SYNC)


/*
    HD3_PERIOD:          [0x0, 0xffff],              bits : 15_0
    HD3_ASSERT:          [0x0, 0xffff],              bits : 31_16
*/
#define R6C_HORIZONTAL_SYNC_OFS 0x006c
REGDEF_BEGIN(R6C_HORIZONTAL_SYNC)
REGDEF_BIT(HD3_PERIOD,       16)
REGDEF_BIT(HD3_ASSERT,       16)
REGDEF_END(R6C_HORIZONTAL_SYNC)


/*
    HD3_COUNT:           [0x0, 0x7ffff],             bits : 18_0
*/
#define R70_HORIZONTAL_SYNC_OFS 0x0070
REGDEF_BEGIN(R70_HORIZONTAL_SYNC)
REGDEF_BIT(HD3_COUNT,       19)
REGDEF_END(R70_HORIZONTAL_SYNC)


/*
    VD3_BP:              [0x0, 0xfffffff],           bits : 27_0
*/
#define R74_BREAKPOINT_OFS 0x0074
REGDEF_BEGIN(R74_BREAKPOINT)
REGDEF_BIT(VD3_BP,       28)
REGDEF_END(R74_BREAKPOINT)

/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R78_RESERVED_OFS 0x0078
REGDEF_BEGIN(R78_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R78_RESERVED)


/*
    VD4_PERIOD:          [0x0, 0xfffffff],           bits : 27_0
*/
#define R7C_VERTICAL_SYNC_OFS 0x007c
REGDEF_BEGIN(R7C_VERTICAL_SYNC)
REGDEF_BIT(VD4_PERIOD,       28)
REGDEF_END(R7C_VERTICAL_SYNC)


/*
    VD4_ASSERT:          [0x0, 0xfffffff],           bits : 27_0
*/
#define R80_VERTICAL_SYNC_OFS 0x0080
REGDEF_BEGIN(R80_VERTICAL_SYNC)
REGDEF_BIT(VD4_ASSERT,       28)
REGDEF_END(R80_VERTICAL_SYNC)


/*
    VD4_FRONTBLANK:      [0x0, 0xfffffff],           bits : 27_0
*/
#define R84_VERTICAL_SYNC_OFS 0x0084
REGDEF_BEGIN(R84_VERTICAL_SYNC)
REGDEF_BIT(VD4_FRONTBLANK,       28)
REGDEF_END(R84_VERTICAL_SYNC)


/*
    HD4_PERIOD:          [0x0, 0xffff],              bits : 15_0
    HD4_ASSERT:          [0x0, 0xffff],              bits : 31_16
*/
#define R88_HORIZONTAL_SYNC_OFS 0x0088
REGDEF_BEGIN(R88_HORIZONTAL_SYNC)
REGDEF_BIT(HD4_PERIOD,       16)
REGDEF_BIT(HD4_ASSERT,       16)
REGDEF_END(R88_HORIZONTAL_SYNC)


/*
    HD4_COUNT:           [0x0, 0x7ffff],             bits : 18_0
*/
#define R8C_HORIZONTAL_SYNC_OFS 0x008c
REGDEF_BEGIN(R8C_HORIZONTAL_SYNC)
REGDEF_BIT(HD4_COUNT,       19)
REGDEF_END(R8C_HORIZONTAL_SYNC)


/*
    VD4_BP:              [0x0, 0xfffffff],           bits : 27_0
*/
#define R90_BREAKPOINT_OFS 0x0090
REGDEF_BEGIN(R90_BREAKPOINT)
REGDEF_BIT(VD4_BP,       28)
REGDEF_END(R90_BREAKPOINT)

/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R94_RESERVED_OFS 0x0094
REGDEF_BEGIN(R94_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R94_RESERVED)

/*
    VD5_PERIOD:          [0x0, 0xfffffff],           bits : 27_0
*/
#define R98_VERTICAL_SYNC_OFS 0x0098
REGDEF_BEGIN(R98_VERTICAL_SYNC)
REGDEF_BIT(VD5_PERIOD,       28)
REGDEF_END(R98_VERTICAL_SYNC)


/*
    VD5_ASSERT:          [0x0, 0xfffffff],           bits : 27_0
*/
#define R9C_VERTICAL_SYNC_OFS 0x009c
REGDEF_BEGIN(R9C_VERTICAL_SYNC)
REGDEF_BIT(VD5_ASSERT,       28)
REGDEF_END(R9C_VERTICAL_SYNC)


/*
    VD5_FRONTBLANK:      [0x0, 0xfffffff],           bits : 27_0
*/
#define RA0_VERTICAL_SYNC_OFS 0x00a0
REGDEF_BEGIN(RA0_VERTICAL_SYNC)
REGDEF_BIT(VD5_FRONTBLANK,       28)
REGDEF_END(RA0_VERTICAL_SYNC)


/*
    HD5_PERIOD:          [0x0, 0xffff],              bits : 15_0
    HD5_ASSERT:          [0x0, 0xffff],              bits : 31_16
*/
#define RA4_HORIZONTAL_SYNC_OFS 0x00a4
REGDEF_BEGIN(RA4_HORIZONTAL_SYNC)
REGDEF_BIT(HD5_PERIOD,       16)
REGDEF_BIT(HD5_ASSERT,       16)
REGDEF_END(RA4_HORIZONTAL_SYNC)


/*
    HD5_COUNT:           [0x0, 0x7ffff],             bits : 18_0
*/
#define RA8_HORIZONTAL_SYNC_OFS 0x00a8
REGDEF_BEGIN(RA8_HORIZONTAL_SYNC)
REGDEF_BIT(HD5_COUNT,       19)
REGDEF_END(RA8_HORIZONTAL_SYNC)


/*
    VD5_BP:              [0x0, 0xfffffff],           bits : 27_0
*/
#define RAC_BREAKPOINT_OFS 0x00ac
REGDEF_BEGIN(RAC_BREAKPOINT)
REGDEF_BIT(VD5_BP,       28)
REGDEF_END(RAC_BREAKPOINT)

/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RB0_RESERVED_OFS 0x00b0
REGDEF_BEGIN(RB0_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(RB0_RESERVED)


/*
    VD6_PERIOD:          [0x0, 0xfffffff],           bits : 27_0
*/
#define RB4_VERTICAL_SYNC_OFS 0x00b4
REGDEF_BEGIN(RB4_VERTICAL_SYNC)
REGDEF_BIT(VD6_PERIOD,       28)
REGDEF_END(RB4_VERTICAL_SYNC)


/*
    VD6_ASSERT:          [0x0, 0xfffffff],           bits : 27_0
*/
#define RB8_VERTICAL_SYNC_OFS 0x00b8
REGDEF_BEGIN(RB8_VERTICAL_SYNC)
REGDEF_BIT(VD6_ASSERT,       28)
REGDEF_END(RB8_VERTICAL_SYNC)


/*
    VD6_FRONTBLANK:      [0x0, 0xfffffff],           bits : 27_0
*/
#define RBC_VERTICAL_SYNC_OFS 0x00bc
REGDEF_BEGIN(RBC_VERTICAL_SYNC)
REGDEF_BIT(VD6_FRONTBLANK,       28)
REGDEF_END(RBC_VERTICAL_SYNC)


/*
    HD6_PERIOD:          [0x0, 0xffff],              bits : 15_0
    HD6_ASSERT:          [0x0, 0xffff],              bits : 31_16
*/
#define RC0_HORIZONTAL_SYNC_OFS 0x00c0
REGDEF_BEGIN(RC0_HORIZONTAL_SYNC)
REGDEF_BIT(HD6_PERIOD,       16)
REGDEF_BIT(HD6_ASSERT,       16)
REGDEF_END(RC0_HORIZONTAL_SYNC)


/*
    HD6_COUNT:           [0x0, 0x7ffff],             bits : 18_0
*/
#define RC4_HORIZONTAL_SYNC_OFS 0x00c4
REGDEF_BEGIN(RC4_HORIZONTAL_SYNC)
REGDEF_BIT(HD6_COUNT,       19)
REGDEF_END(RC4_HORIZONTAL_SYNC)


/*
    VD6_BP:              [0x0, 0xfffffff],           bits : 27_0
*/
#define RC8_BREAKPOINT_OFS 0x00c8
REGDEF_BEGIN(RC8_BREAKPOINT)
REGDEF_BIT(VD6_BP,       28)
REGDEF_END(RC8_BREAKPOINT)

/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RCC_RESERVED_OFS 0x00cc
REGDEF_BEGIN(RCC_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(RCC_RESERVED)


/*
    VD7_PERIOD:          [0x0, 0xfffffff],           bits : 27_0
*/
#define RD0_VERTICAL_SYNC_OFS 0x00d0
REGDEF_BEGIN(RD0_VERTICAL_SYNC)
REGDEF_BIT(VD7_PERIOD,       28)
REGDEF_END(RD0_VERTICAL_SYNC)


/*
    VD7_ASSERT:          [0x0, 0xfffffff],           bits : 27_0
*/
#define RD4_VERTICAL_SYNC_OFS 0x00d4
REGDEF_BEGIN(RD4_VERTICAL_SYNC)
REGDEF_BIT(VD7_ASSERT,       28)
REGDEF_END(RD4_VERTICAL_SYNC)


/*
    VD7_FRONTBLANK:      [0x0, 0xfffffff],           bits : 27_0
*/
#define RD8_VERTICAL_SYNC_OFS 0x00d8
REGDEF_BEGIN(RD8_VERTICAL_SYNC)
REGDEF_BIT(VD7_FRONTBLANK,       28)
REGDEF_END(RD8_VERTICAL_SYNC)


/*
    HD7_PERIOD:          [0x0, 0xffff],              bits : 15_0
    HD7_ASSERT:          [0x0, 0xffff],              bits : 31_16
*/
#define RDC_HORIZONTAL_SYNC_OFS 0x00dc
REGDEF_BEGIN(RDC_HORIZONTAL_SYNC)
REGDEF_BIT(HD7_PERIOD,       16)
REGDEF_BIT(HD7_ASSERT,       16)
REGDEF_END(RDC_HORIZONTAL_SYNC)


/*
    HD7_COUNT:           [0x0, 0x7ffff],             bits : 18_0
*/
#define RE0_HORIZONTAL_SYNC_OFS 0x00e0
REGDEF_BEGIN(RE0_HORIZONTAL_SYNC)
REGDEF_BIT(HD7_COUNT,       19)
REGDEF_END(RE0_HORIZONTAL_SYNC)


/*
    VD7_BP:              [0x0, 0xfffffff],           bits : 27_0
*/
#define RE4_BREAKPOINT_OFS 0x00e4
REGDEF_BEGIN(RE4_BREAKPOINT)
REGDEF_BIT(VD7_BP,       28)
REGDEF_END(RE4_BREAKPOINT)

/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define RE8_RESERVED_OFS 0x00e8
REGDEF_BEGIN(RE8_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(RE8_RESERVED)


/*
    VD8_PERIOD:          [0x0, 0xfffffff],           bits : 27_0
*/
#define REC_VERTICAL_SYNC_OFS 0x00ec
REGDEF_BEGIN(REC_VERTICAL_SYNC)
REGDEF_BIT(VD8_PERIOD,       28)
REGDEF_END(REC_VERTICAL_SYNC)


/*
    VD8_ASSERT:          [0x0, 0xfffffff],           bits : 27_0
*/
#define RF0_VERTICAL_SYNC_OFS 0x00f0
REGDEF_BEGIN(RF0_VERTICAL_SYNC)
REGDEF_BIT(VD8_ASSERT,       28)
REGDEF_END(RF0_VERTICAL_SYNC)


/*
    VD8_FRONTBLANK:      [0x0, 0xfffffff],           bits : 27_0
*/
#define RF4_VERTICAL_SYNC_OFS 0x00f4
REGDEF_BEGIN(RF4_VERTICAL_SYNC)
REGDEF_BIT(VD8_FRONTBLANK,       28)
REGDEF_END(RF4_VERTICAL_SYNC)


/*
    HD8_PERIOD:          [0x0, 0xffff],              bits : 15_0
    HD8_ASSERT:          [0x0, 0xffff],              bits : 31_16
*/
#define RF8_HORIZONTAL_SYNC_OFS 0x00f8
REGDEF_BEGIN(RF8_HORIZONTAL_SYNC)
REGDEF_BIT(HD8_PERIOD,       16)
REGDEF_BIT(HD8_ASSERT,       16)
REGDEF_END(RF8_HORIZONTAL_SYNC)


/*
    HD8_COUNT:           [0x0, 0x7ffff],             bits : 18_0
*/
#define RFC_HORIZONTAL_SYNC_OFS 0x00fc
REGDEF_BEGIN(RFC_HORIZONTAL_SYNC)
REGDEF_BIT(HD8_COUNT,       19)
REGDEF_END(RFC_HORIZONTAL_SYNC)


/*
    VD8_BP:              [0x0, 0xfffffff],           bits : 27_0
*/
#define R100_BREAKPOINT_OFS 0x0100
REGDEF_BEGIN(R100_BREAKPOINT)
REGDEF_BIT(VD8_BP,       28)
REGDEF_END(R100_BREAKPOINT)

/*
    Reserved:    [0x0, 0xffffffff],            bits : 31_0
*/
#define R104_RESERVED_OFS 0x0104
REGDEF_BEGIN(R104_RESERVED)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(R104_RESERVED)


/*
    CLK_CNT     :              [0x0, 0xff],               bits : 7_0
    VD          :              [0x0, 0x1],                bits : 8_8
    HD          :              [0x0, 0x1],                bits : 9_9
    VD2         :              [0x0, 0x1],                bits :10_10
    HD2         :              [0x0, 0x1],                bits :11_11
    VD3         :              [0x0, 0x1],                bits :12_12
    HD3         :              [0x0, 0x1],                bits :13_13
    VD4         :              [0x0, 0x1],                bits :14_14
    HD4         :              [0x0, 0x1],                bits :15_15
    VD5         :              [0x0, 0x1],                bits :16_16
    HD5         :              [0x0, 0x1],                bits :17_17
    VD6         :              [0x0, 0x1],                bits :18_18
    HD6         :              [0x0, 0x1],                bits :19_19
    VD7         :              [0x0, 0x1],                bits :20_20
    HD7         :              [0x0, 0x1],                bits :21_21
    VD8         :              [0x0, 0x1],                bits :22_22
    HD8         :              [0x0, 0x1],                bits :23_23
    FLSH_EXT_TRG:              [0x0, 0x1],                bits :24_24
    MSH_EXT_TRG :              [0x0, 0x1],                bits :25_25
    FLSH_CTRL   :              [0x0, 0x1],                bits :26_26
    MES_CTRL_A0 :              [0x0, 0x1],                bits :27_27
    MES_CTRL_A1 :              [0x0, 0x1],                bits :28_28
    MES_CTRL_B0 :              [0x0, 0x1],                bits :29_29
    MES_CTRL_B1 :              [0x0, 0x1],                bits :30_30
*/
#define R108_STATUS_OFS 0x0108
REGDEF_BEGIN(R108_STATUS)
REGDEF_BIT(CLK_CNT,        8)
REGDEF_BIT(VD,        1)
REGDEF_BIT(HD,        1)
REGDEF_BIT(VD2,        1)
REGDEF_BIT(HD2,        1)
REGDEF_BIT(VD3,        1)
REGDEF_BIT(HD3,        1)
REGDEF_BIT(VD4,        1)
REGDEF_BIT(HD4,        1)
REGDEF_BIT(VD5,        1)
REGDEF_BIT(HD5,        1)
REGDEF_BIT(VD6,        1)
REGDEF_BIT(HD6,        1)
REGDEF_BIT(VD7,        1)
REGDEF_BIT(HD7,        1)
REGDEF_BIT(VD8,        1)
REGDEF_BIT(HD8,        1)
REGDEF_BIT(FLSH_EXT_TRG,        1)
REGDEF_BIT(MSH_EXT_TRG,        1)
REGDEF_BIT(FLSH_CTRL,        1)
REGDEF_BIT(MES_CTRL_A0,        1)
REGDEF_BIT(MES_CTRL_A1,        1)
REGDEF_BIT(MES_CTRL_B0,        1)
REGDEF_BIT(MES_CTRL_B1,        1)
REGDEF_END(R108_STATUS)


/*
    FLSH_IMD_TRG :    [0x0, 0x1],            bits : 0
    FLSH_WAT_TRG :    [0x0, 0x1],            bits : 1
    FLSH_WAT_MODE:    [0x0, 0x1],            bits : 2
    FLSH_ASSERT  :    [0x0, 0xfffff],        bits : 23_4
*/
#define R10C_FLASHLIGHT_OFS 0x010c
REGDEF_BEGIN(R10C_FLASHLIGHT)
REGDEF_BIT(FLSH_IMD_TRG,        1)
REGDEF_BIT(FLSH_WAT_TRG,        1)
REGDEF_BIT(FLSH_WAT_MODE,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(FLSH_ASSERT,        20)
REGDEF_END(R10C_FLASHLIGHT)


/*
    FLSH_DELAY:    [0x0, 0xfffffff],            bits : 27_0
*/
#define R110_FLASHLIGHT_OFS 0x0110
REGDEF_BEGIN(R110_FLASHLIGHT)
REGDEF_BIT(FLSH_DELAY,        28)
REGDEF_END(R110_FLASHLIGHT)


/*
    MSHA_CLOSE_IMD_TRG :    [0x0, 0x1],            bits : 0
    MSHA_CLOSE_WAT_TRG :    [0x0, 0x1],            bits : 1
    MSHA_CLOSE_WAT_MODE:    [0x0, 0x1],            bits : 2
    MSHA_CLOSE_ASSERT  :    [0x0, 0x7fffff],            bits : 26_4
*/
#define R114_MECHANICAL_SHUTTER_A_OFS 0x0114
REGDEF_BEGIN(R114_MECHANICAL_SHUTTER_A)
REGDEF_BIT(MSHA_CLOSE_IMD_TRG,        1)
REGDEF_BIT(MSHA_CLOSE_WAT_TRG,        1)
REGDEF_BIT(MSHA_CLOSE_WAT_MODE,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(MSHA_CLOSE_ASSERT,        23)
REGDEF_END(R114_MECHANICAL_SHUTTER_A)


/*
    MSHA_CLOSE_DELAY:    [0x0, 0xfffffff],            bits : 27_0
*/
#define R118_MECHANICAL_SHUTTER_A_OFS 0x0118
REGDEF_BEGIN(R118_MECHANICAL_SHUTTER_A)
REGDEF_BIT(MSHA_CLOSE_DELAY,        28)
REGDEF_END(R118_MECHANICAL_SHUTTER_A)


/*
    MSHA_OPEN_IMD_TRG :    [0x0, 0x1],            bits : 0
    MSHA_OPEN_WAT_TRG :    [0x0, 0x1],            bits : 1
    MSHA_OPEN_WAT_MODE:    [0x0, 0x1],            bits : 2
    MSHA_OPEN_ASSERT  :    [0x0, 0x7fffff],            bits : 26_4
*/
#define R11C_MECHANICAL_SHUTTER_A_OFS 0x011c
REGDEF_BEGIN(R11C_MECHANICAL_SHUTTER_A)
REGDEF_BIT(MSHA_OPEN_IMD_TRG,        1)
REGDEF_BIT(MSHA_OPEN_WAT_TRG,        1)
REGDEF_BIT(MSHA_OPEN_WAT_MODE,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(MSHA_OPEN_ASSERT,        23)
REGDEF_END(R11C_MECHANICAL_SHUTTER_A)


/*
    MSHA_OPEN_DELAY:    [0x0, 0xfffffff],            bits : 27_0
*/
#define R120_MECHANICAL_SHUTTER_A_OFS 0x0120
REGDEF_BEGIN(R120_MECHANICAL_SHUTTER_A)
REGDEF_BIT(MSHA_OPEN_DELAY,        28)
REGDEF_END(R120_MECHANICAL_SHUTTER_A)


/*
    MSHB_CLOSE_IMD_TRG :    [0x0, 0x1],            bits : 0
    MSHB_CLOSE_WAT_TRG :    [0x0, 0x1],            bits : 1
    MSHB_CLOSE_WAT_MODE:    [0x0, 0x1],            bits : 2
    MSHB_CLOSE_ASSERT  :    [0x0, 0x7fffff],            bits : 26_4
*/
#define R124_MECHANICAL_SHUTTER_B_OFS 0x0124
REGDEF_BEGIN(R124_MECHANICAL_SHUTTER_B)
REGDEF_BIT(MSHB_CLOSE_IMD_TRG,        1)
REGDEF_BIT(MSHB_CLOSE_WAT_TRG,        1)
REGDEF_BIT(MSHB_CLOSE_WAT_MODE,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(MSHB_CLOSE_ASSERT,        23)
REGDEF_END(R124_MECHANICAL_SHUTTER_B)


/*
    MSHB_CLOSE_DELAY:    [0x0, 0xfffffff],            bits : 27_0
*/
#define R128_MECHANICAL_SHUTTER_B_OFS 0x0128
REGDEF_BEGIN(R128_MECHANICAL_SHUTTER_B)
REGDEF_BIT(MSHB_CLOSE_DELAY,        28)
REGDEF_END(R128_MECHANICAL_SHUTTER_B)


/*
    MSHB_OPEN_IMD_TRG :    [0x0, 0x1],            bits : 0
    MSHB_OPEN_WAT_TRG :    [0x0, 0x1],            bits : 1
    MSHB_OPEN_WAT_MODE:    [0x0, 0x1],            bits : 2
    MSHB_OPEN_ASSERT  :    [0x0, 0x7fffff],            bits : 26_4
*/
#define R12C_MECHANICAL_SHUTTER_B_OFS 0x012c
REGDEF_BEGIN(R12C_MECHANICAL_SHUTTER_B)
REGDEF_BIT(MSHB_OPEN_IMD_TRG,        1)
REGDEF_BIT(MSHB_OPEN_WAT_TRG,        1)
REGDEF_BIT(MSHB_OPEN_WAT_MODE,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(MSHB_OPEN_ASSERT,        23)
REGDEF_END(R12C_MECHANICAL_SHUTTER_B)


/*
    MSHB_OPEN_DELAY:    [0x0, 0xfffffff],            bits : 27_0
*/
#define R130_MECHANICAL_SHUTTER_B_OFS 0x0130
REGDEF_BEGIN(R130_MECHANICAL_SHUTTER_B)
REGDEF_BIT(MSHB_OPEN_DELAY,        28)
REGDEF_END(R130_MECHANICAL_SHUTTER_B)


typedef struct {
	//0x0000
	T_R0_ENGINE_CONTROL
	TGE_Register_0000;

	//0x0004
	T_R4_ENGINE_CONTROL
	TGE_Register_0004;

	//0x0008
	T_R8_ENGINE_CONTROL
	TGE_Register_0008;

	//0x000c
	T_RC_ENGINE_CONTROL
	TGE_Register_000c;

	//0x0010
	T_R10_ENGINE_CONTROL
	TGE_Register_0010;

	//0x0014
	T_R14_INTERRUPT_EN
	TGE_Register_0014;

	//0x0018
	T_R18_INTERRUPT
	TGE_Register_0018;

	//0x001c
	T_R1C_RESERVED
	TGE_Register_001c;

	//0x0020
	T_R20_RESERVED
	TGE_Register_0020;

	//0x0024
	T_R24_RESERVED
	TGE_Register_0024;

	//0x0028
	T_R28_VERTICAL_SYNC
	TGE_Register_0028;

	//0x002c
	T_R2C_VERTICAL_SYNC
	TGE_Register_002c;

	//0x0030
	T_R30_VERTICAL_SYNC
	TGE_Register_0030;

	//0x0034
	T_R34_HORIZONTAL_SYNC
	TGE_Register_0034;

	//0x0038
	T_R38_HORIZONTAL_SYNC
	TGE_Register_0038;

	//0x003c
	T_R3C_BREAKPOINT
	TGE_Register_003c;

	//0x0040
	T_R40_RESERVED
	TGE_Register_0040;

	//0x0044
	T_R44_VERTICAL_SYNC
	TGE_Register_0044;

	//0x0048
	T_R48_VERTICAL_SYNC
	TGE_Register_0048;

	//0x004c
	T_R4C_VERTICAL_SYNC
	TGE_Register_004c;

	//0x0050
	T_R50_HORIZONTAL_SYNC
	TGE_Register_0050;

	//0x0054
	T_R54_HORIZONTAL_SYNC
	TGE_Register_0054;

	//0x0058
	T_R58_BREAKPOINT
	TGE_Register_0058;

	//0x005c
	T_R5C_RESERVED
	TGE_Register_005c;

	//0x0060
	T_R60_VERTICAL_SYNC
	TGE_Register_0060;

	//0x0064
	T_R64_VERTICAL_SYNC
	TGE_Register_0064;

	//0x0068
	T_R68_VERTICAL_SYNC
	TGE_Register_0068;

	//0x006c
	T_R6C_HORIZONTAL_SYNC
	TGE_Register_006c;

	//0x0070
	T_R70_HORIZONTAL_SYNC
	TGE_Register_0070;

	//0x0074
	T_R74_BREAKPOINT
	TGE_Register_0074;

	//0x0078
	T_R78_RESERVED
	TGE_Register_0078;

	//0x007c
	T_R7C_VERTICAL_SYNC
	TGE_Register_007c;

	//0x0080
	T_R80_VERTICAL_SYNC
	TGE_Register_0080;

	//0x0084
	T_R84_VERTICAL_SYNC
	TGE_Register_0084;

	//0x0088
	T_R88_HORIZONTAL_SYNC
	TGE_Register_0088;

	//0x008c
	T_R8C_HORIZONTAL_SYNC
	TGE_Register_008c;

	//0x0090
	T_R90_BREAKPOINT
	TGE_Register_0090;

	//0x0094
	T_R94_RESERVED
	TGE_Register_0094;

	//0x0098
	T_R98_VERTICAL_SYNC
	TGE_Register_0098;

	//0x009c
	T_R9C_VERTICAL_SYNC
	TGE_Register_009c;

	//0x00a0
	T_RA0_VERTICAL_SYNC
	TGE_Register_00a0;

	//0x00a4
	T_RA4_HORIZONTAL_SYNC
	TGE_Register_00a4;

	//0x00a8
	T_RA8_HORIZONTAL_SYNC
	TGE_Register_00a8;

	//0x00ac
	T_RAC_BREAKPOINT
	TGE_Register_00ac;

	//0x00b0
	T_RB0_RESERVED
	TGE_Register_00b0;

	//0x00b4
	T_RB4_VERTICAL_SYNC
	TGE_Register_00b4;

	//0x00b8
	T_RB8_VERTICAL_SYNC
	TGE_Register_00b8;

	//0x00bc
	T_RBC_VERTICAL_SYNC
	TGE_Register_00bc;

	//0x00c0
	T_RC0_HORIZONTAL_SYNC
	TGE_Register_00c0;

	//0x00c4
	T_RC4_HORIZONTAL_SYNC
	TGE_Register_00c4;

	//0x00c8
	T_RC8_BREAKPOINT
	TGE_Register_00c8;

	//0x00cc
	T_RCC_RESERVED
	TGE_Register_00cc;

	//0x00d0
	T_RD0_VERTICAL_SYNC
	TGE_Register_00d0;

	//0x00d4
	T_RD4_VERTICAL_SYNC
	TGE_Register_00d4;

	//0x00d8
	T_RD8_VERTICAL_SYNC
	TGE_Register_00d8;

	//0x00dc
	T_RDC_HORIZONTAL_SYNC
	TGE_Register_00dc;

	//0x00e0
	T_RE0_HORIZONTAL_SYNC
	TGE_Register_00e0;

	//0x00e4
	T_RE4_BREAKPOINT
	TGE_Register_00e4;

	//0x00e8
	T_RE8_RESERVED
	TGE_Register_00e8;

	//0x00ec
	T_REC_VERTICAL_SYNC
	TGE_Register_00ec;

	//0x00f0
	T_RF0_VERTICAL_SYNC
	TGE_Register_00f0;

	//0x00f4
	T_RF4_VERTICAL_SYNC
	TGE_Register_00f4;

	//0x00f8
	T_RF8_HORIZONTAL_SYNC
	TGE_Register_00f8;

	//0x00fc
	T_RFC_HORIZONTAL_SYNC
	TGE_Register_00fc;

	//0x0100
	T_R100_BREAKPOINT
	TGE_Register_0100;

	//0x0104
	T_R104_RESERVED
	TGE_Register_0104;

	//0x0108
	T_R108_STATUS
	TGE_Register_0108;

	//0x010c
	T_R10C_FLASHLIGHT
	TGE_Register_010c;

	//0x0110
	T_R110_FLASHLIGHT
	TGE_Register_0110;

	//0x0114
	T_R114_MECHANICAL_SHUTTER_A
	TGE_Register_0114;

	//0x0118
	T_R118_MECHANICAL_SHUTTER_A
	TGE_Register_0118;

	//0x011c
	T_R11C_MECHANICAL_SHUTTER_A
	TGE_Register_0011c;

	//0x0120
	T_R120_MECHANICAL_SHUTTER_A
	TGE_Register_0120;

	//0x0124
	T_R124_MECHANICAL_SHUTTER_B
	TGE_Register_0124;

	//0x0128
	T_R128_MECHANICAL_SHUTTER_B
	TGE_Register_0128;

	//0x012c
	T_R12C_MECHANICAL_SHUTTER_B
	TGE_Register_012c;

	//0x0130
	T_R130_MECHANICAL_SHUTTER_B
	TGE_Register_0130;

} NT96680_TGE_REG_STRUCT;

#endif
