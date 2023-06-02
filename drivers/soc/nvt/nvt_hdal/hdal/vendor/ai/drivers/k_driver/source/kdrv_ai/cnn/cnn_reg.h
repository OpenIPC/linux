#ifndef _CNN_NT98560_H_
#define _CNN_NT98560_H_

#include "cnn_platform.h"

#define _CNN_REG_BASE_ADDR       0xf0cb0000

#define CNN_REG_ADDR(ofs)        (_CNN_REG_BASE_ADDR+(ofs))
#define CNN_SETREG(ofs, value)   OUTW((_CNN_REG_BASE_ADDR + ofs), value)
#define CNN_GETREG(ofs)          INW(_CNN_REG_BASE_ADDR + ofs)

/*
    CNN_SW_RST:    [0x0, 0x1],			bits : 0
    CNN_START :    [0x0, 0x1],			bits : 1
    LL_FIRE   :    [0x0, 0x1],			bits : 28
*/
#define CNN_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(CNN_CONTROL_REGISTER)
    REGDEF_BIT(CNN_SW_RST,        1)
    REGDEF_BIT(CNN_START ,        1)
    REGDEF_BIT(          ,        26)
    REGDEF_BIT(LL_FIRE   ,        1)
REGDEF_END(CNN_CONTROL_REGISTER)


/*
    CNN_MODE                 :    [0x0, 0x3],			bits : 1_0
    CNN_ELT_MODE             :    [0x0, 0x3],			bits : 3_2
    CNN_ACT_MODE             :    [0x0, 0x1],			bits : 4
    CNN_POOL_MODE            :    [0x0, 0x1],			bits : 5
    CNN_LUT_MODE             :    [0x0, 0x1],			bits : 6
    CNN_BNSCALE_EN           :    [0x0, 0x1],			bits : 8
    CNN_ACT_EN               :    [0x0, 0x1],			bits : 9
    CNN_POOL_EN              :    [0x0, 0x1],			bits : 10
    CNN_ELTWISE_EN           :    [0x0, 0x1],			bits : 11
    CNN_CONV_EN              :    [0x0, 0x1],			bits : 12
    CNN_PREACT_EN            :    [0x0, 0x1],			bits : 13
    FCD_VLC_EN               :    [0x0, 0x1],			bits : 16
    FCD_QUANTIZATION_EN      :    [0x0, 0x1],			bits : 17
    FCD_SPARSE_EN            :    [0x0, 0x1],			bits : 18
    FCD_QUANTIZATION_KMEANS  :    [0x0, 0x1],			bits : 19
    FCD_KQ_TBL_UPDATE_DISABLE:    [0x0, 0x1],			bits : 20
*/
#define CNN_MODE_REGISTER_OFS 0x0004
REGDEF_BEGIN(CNN_MODE_REGISTER)
    REGDEF_BIT(CNN_MODE                 ,        2)
    REGDEF_BIT(CNN_ELT_MODE             ,        2)
    REGDEF_BIT(CNN_ACT_MODE             ,        1)
    REGDEF_BIT(CNN_POOL_MODE            ,        1)
    REGDEF_BIT(CNN_LUT_MODE             ,        1)
    REGDEF_BIT(                         ,        1)
    REGDEF_BIT(CNN_BNSCALE_EN           ,        1)
    REGDEF_BIT(CNN_ACT_EN               ,        1)
    REGDEF_BIT(CNN_POOL_EN              ,        1)
    REGDEF_BIT(CNN_ELTWISE_EN           ,        1)
    REGDEF_BIT(CNN_CONV_EN              ,        1)
    REGDEF_BIT(CNN_PREACT_EN            ,        1)
    REGDEF_BIT(                         ,        2)
    REGDEF_BIT(FCD_VLC_EN               ,        1)
    REGDEF_BIT(FCD_QUANTIZATION_EN      ,        1)
    REGDEF_BIT(FCD_SPARSE_EN            ,        1)
    REGDEF_BIT(FCD_QUANTIZATION_KMEANS  ,        1)
    REGDEF_BIT(FCD_KQ_TBL_UPDATE_DISABLE,        1)
REGDEF_END(CNN_MODE_REGISTER)


/*
    INTE_FRM_DONE                 :    [0x0, 0x1],			bits : 0
    INTE_SRC_ILLEGALSIZE          :    [0x0, 0x1],			bits : 2
    INTE_CONVOUT_ILLEGALSIZE      :    [0x0, 0x1],			bits : 3
    INTE_LLEND                    :    [0x0, 0x1],			bits : 8
    INTE_LLERROR                  :    [0x0, 0x1],			bits : 9
    INTE_LLJOBEND                 :    [0x0, 0x1],			bits : 10
    INTE_NUE_OCCUPY_APP_TO_APP_ERR:    [0x0, 0x1],			bits : 12
    INTE_NUE_OCCUPY_APP_TO_LL_ERR :    [0x0, 0x1],			bits : 13
    INTE_NUE_OCCUPY_LL_TO_APP_ERR :    [0x0, 0x1],			bits : 14
    INTE_NUE_OCCUPY_LL_TO_LL_ERR  :    [0x0, 0x1],			bits : 15
    INTE_FCD_DECODE_DONE          :    [0x0, 0x1],			bits : 16
    INTE_FCD_VLC_DEC_ERR          :    [0x0, 0x1],			bits : 17
    INTE_FCD_BS_SIZE_ERR          :    [0x0, 0x1],			bits : 18
    INTE_FCD_SPARSE_DATA_ERR      :    [0x0, 0x1],			bits : 19
    INTE_FCD_SPARSE_INDEX_ERR     :    [0x0, 0x1],			bits : 20
    INTS_CHECKSUM_MISMATCH0       :    [0x0, 0x1],			bits : 24
    INTS_CHECKSUM_MISMATCH1       :    [0x0, 0x1],			bits : 25
    INTS_CHECKSUM_MISMATCH2       :    [0x0, 0x1],			bits : 26
*/
#define CNN_INTERRUPT_ENABLE_REGISTER_OFS 0x0008
REGDEF_BEGIN(CNN_INTERRUPT_ENABLE_REGISTER)
    REGDEF_BIT(INTE_FRM_DONE                 ,        1)
    REGDEF_BIT(                              ,        1)
    REGDEF_BIT(INTE_SRC_ILLEGALSIZE          ,        1)
    REGDEF_BIT(INTE_CONVOUT_ILLEGALSIZE      ,        1)
    REGDEF_BIT(                              ,        4)
    REGDEF_BIT(INTE_LLEND                    ,        1)
    REGDEF_BIT(INTE_LLERROR                  ,        1)
    REGDEF_BIT(INTE_LLJOBEND                 ,        1)
    REGDEF_BIT(                              ,        1)
    REGDEF_BIT(INTE_NUE_OCCUPY_APP_TO_APP_ERR,        1)
    REGDEF_BIT(INTE_NUE_OCCUPY_APP_TO_LL_ERR ,        1)
    REGDEF_BIT(INTE_NUE_OCCUPY_LL_TO_APP_ERR ,        1)
    REGDEF_BIT(INTE_NUE_OCCUPY_LL_TO_LL_ERR  ,        1)
    REGDEF_BIT(INTE_FCD_DECODE_DONE          ,        1)
    REGDEF_BIT(INTE_FCD_VLC_DEC_ERR          ,        1)
    REGDEF_BIT(INTE_FCD_BS_SIZE_ERR          ,        1)
    REGDEF_BIT(INTE_FCD_SPARSE_DATA_ERR      ,        1)
    REGDEF_BIT(INTE_FCD_SPARSE_INDEX_ERR     ,        1)
    REGDEF_BIT(                              ,        3)
    REGDEF_BIT(INTS_CHECKSUM_MISMATCH0       ,        1)
    REGDEF_BIT(INTS_CHECKSUM_MISMATCH1       ,        1)
    REGDEF_BIT(INTS_CHECKSUM_MISMATCH2       ,        1)
REGDEF_END(CNN_INTERRUPT_ENABLE_REGISTER)


/*
    INTS_FRM_DONE                 :    [0x0, 0x1],			bits : 0
    INTS_SRC_ILLEGALSIZE          :    [0x0, 0x1],			bits : 2
    INTS_CONVOUT_ILLEGALSIZE      :    [0x0, 0x1],			bits : 3
    INTS_LLEND                    :    [0x0, 0x1],			bits : 8
    INTS_LLERROR                  :    [0x0, 0x1],			bits : 9
    INTS_LLJOBEND                 :    [0x0, 0x1],			bits : 10
    INTS_NUE_OCCUPY_APP_TO_APP_ERR:    [0x0, 0x1],			bits : 12
    INTS_NUE_OCCUPY_APP_TO_LL_ERR :    [0x0, 0x1],			bits : 13
    INTS_NUE_OCCUPY_LL_TO_APP_ERR :    [0x0, 0x1],			bits : 14
    INTS_NUE_OCCUPY_LL_TO_LL_ERR  :    [0x0, 0x1],			bits : 15
    INTS_FCD_DECODE_DONE          :    [0x0, 0x1],			bits : 16
    INTS_FCD_VLC_DEC_ERR          :    [0x0, 0x1],			bits : 17
    INTS_FCD_BS_SIZE_ERR          :    [0x0, 0x1],			bits : 18
    INTS_FCD_SPARSE_DATA_ERR      :    [0x0, 0x1],			bits : 19
    INTS_FCD_SPARSE_INDEX_ERR     :    [0x0, 0x1],			bits : 20
    INTS_CHECKSUM_MISMATCH0       :    [0x0, 0x1],			bits : 24
    INTS_CHECKSUM_MISMATCH1       :    [0x0, 0x1],			bits : 25
    INTS_CHECKSUM_MISMATCH2       :    [0x0, 0x1],			bits : 26
*/
#define CNN_INTERRUPT_STATUS_REGISTER_OFS 0x000c
REGDEF_BEGIN(CNN_INTERRUPT_STATUS_REGISTER)
    REGDEF_BIT(INTS_FRM_DONE                 ,        1)
    REGDEF_BIT(                              ,        1)
    REGDEF_BIT(INTS_SRC_ILLEGALSIZE          ,        1)
    REGDEF_BIT(INTS_CONVOUT_ILLEGALSIZE      ,        1)
    REGDEF_BIT(                              ,        4)
    REGDEF_BIT(INTS_LLEND                    ,        1)
    REGDEF_BIT(INTS_LLERROR                  ,        1)
    REGDEF_BIT(INTS_LLJOBEND                 ,        1)
    REGDEF_BIT(                              ,        1)
    REGDEF_BIT(INTS_NUE_OCCUPY_APP_TO_APP_ERR,        1)
    REGDEF_BIT(INTS_NUE_OCCUPY_APP_TO_LL_ERR ,        1)
    REGDEF_BIT(INTS_NUE_OCCUPY_LL_TO_APP_ERR ,        1)
    REGDEF_BIT(INTS_NUE_OCCUPY_LL_TO_LL_ERR  ,        1)
    REGDEF_BIT(INTS_FCD_DECODE_DONE          ,        1)
    REGDEF_BIT(INTS_FCD_VLC_DEC_ERR          ,        1)
    REGDEF_BIT(INTS_FCD_BS_SIZE_ERR          ,        1)
    REGDEF_BIT(INTS_FCD_SPARSE_DATA_ERR      ,        1)
    REGDEF_BIT(INTS_FCD_SPARSE_INDEX_ERR     ,        1)
    REGDEF_BIT(                              ,        3)
    REGDEF_BIT(INTS_CHECKSUM_MISMATCH0       ,        1)
    REGDEF_BIT(INTS_CHECKSUM_MISMATCH1       ,        1)
    REGDEF_BIT(INTS_CHECKSUM_MISMATCH2       ,        1)
REGDEF_END(CNN_INTERRUPT_STATUS_REGISTER)


/*
    DRAM_SAI0:    [0x0, 0xffffffff],			bits : 31_0
*/
#define INPUT_CHANNEL_REGISTER_0_OFS 0x0010
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_0)
    REGDEF_BIT(DRAM_SAI0,        32)
REGDEF_END(INPUT_CHANNEL_REGISTER_0)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define INPUT_CHANNEL_REGISTER_1_OFS 0x0014
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_1)
    REGDEF_BIT(reserved,        32)
REGDEF_END(INPUT_CHANNEL_REGISTER_1)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define INPUT_CHANNEL_REGISTER_2_OFS 0x0018
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_2)
    REGDEF_BIT(reserved,        32)
REGDEF_END(INPUT_CHANNEL_REGISTER_2)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define INPUT_CHANNEL_REGISTER_3_OFS 0x001c
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_3)
    REGDEF_BIT(reserved,        32)
REGDEF_END(INPUT_CHANNEL_REGISTER_3)


/*
    DRAM_OFSI0:    [0x0, 0x1fffffff],			bits : 28_0
*/
#define INPUT_CHANNEL_REGISTER_4_OFS 0x0020
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_4)
    REGDEF_BIT(DRAM_OFSI0,        29)
REGDEF_END(INPUT_CHANNEL_REGISTER_4)


/*
    DRAM_OFSI1:    [0x0, 0x1fffffff],			bits : 28_0
*/
#define INPUT_CHANNEL_REGISTER_5_OFS 0x0024
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_5)
    REGDEF_BIT(DRAM_OFSI1,        29)
REGDEF_END(INPUT_CHANNEL_REGISTER_5)


/*
    DRAM_OFSI2:    [0x0, 0x1fffffff],			bits : 28_0
*/
#define INPUT_CHANNEL_REGISTER_6_OFS 0x0028
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_6)
    REGDEF_BIT(DRAM_OFSI2,        29)
REGDEF_END(INPUT_CHANNEL_REGISTER_6)


/*
    DRAM_OFSI3:    [0x0, 0x1fffffff],			bits : 28_0
*/
#define INPUT_CHANNEL_REGISTER_7_OFS 0x002c
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_7)
    REGDEF_BIT(DRAM_OFSI3,        29)
REGDEF_END(INPUT_CHANNEL_REGISTER_7)


/*
    DRAM_SAI4:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define INPUT_CHANNEL_REGISTER_8_OFS 0x0030
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_8)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI4,        30)
REGDEF_END(INPUT_CHANNEL_REGISTER_8)


/*
    DRAM_SAI5:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define INPUT_CHANNEL_REGISTER_9_OFS 0x0034
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_9)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI5,        30)
REGDEF_END(INPUT_CHANNEL_REGISTER_9)


/*
    DRAM_SAI6:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define INPUT_CHANNEL_REGISTER_10_OFS 0x0038
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_10)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI6,        30)
REGDEF_END(INPUT_CHANNEL_REGISTER_10)


/*
    DRAM_SAI7:    [0x0, 0xffffffff],			bits : 31_0
*/
#define INPUT_CHANNEL_REGISTER_11_OFS 0x003c
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_11)
    REGDEF_BIT(DRAM_SAI7,        32)
REGDEF_END(INPUT_CHANNEL_REGISTER_11)


/*
    DRAM_SAI8:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define INPUT_CHANNEL_REGISTER_12_OFS 0x0040
REGDEF_BEGIN(INPUT_CHANNEL_REGISTER_12)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI8,        30)
REGDEF_END(INPUT_CHANNEL_REGISTER_12)


/*
    DRAM_SAO0:    [0x0, 0xffffffff],			bits : 31_0
*/
#define OUTPUT_CHANNEL_REGISTER_0_OFS 0x0044
REGDEF_BEGIN(OUTPUT_CHANNEL_REGISTER_0)
    REGDEF_BIT(DRAM_SAO0,        32)
REGDEF_END(OUTPUT_CHANNEL_REGISTER_0)


/*
    DRAM_SAO1:    [0x0, 0xffffffff],			bits : 31_0
*/
#define OUTPUT_CHANNEL_REGISTER_1_OFS 0x0048
REGDEF_BEGIN(OUTPUT_CHANNEL_REGISTER_1)
    REGDEF_BIT(DRAM_SAO1,        32)
REGDEF_END(OUTPUT_CHANNEL_REGISTER_1)


/*
    DRAM_OFSO0:    [0x0, 0x1fffffff],			bits : 28_0
*/
#define OUTPUT_CHANNEL_REGISTER_2_OFS 0x004c
REGDEF_BEGIN(OUTPUT_CHANNEL_REGISTER_2)
    REGDEF_BIT(DRAM_OFSO0,        29)
REGDEF_END(OUTPUT_CHANNEL_REGISTER_2)


/*
    DRAM_OFSO1:    [0x0, 0x1fffffff],			bits : 28_0
*/
#define OUTPUT_CHANNEL_REGISTER_3_OFS 0x0050
REGDEF_BEGIN(OUTPUT_CHANNEL_REGISTER_3)
    REGDEF_BIT(DRAM_OFSO1,        29)
REGDEF_END(OUTPUT_CHANNEL_REGISTER_3)


/*
    DRAM_OFSO2:    [0x0, 0x1fffffff],			bits : 28_0
*/
#define OUTPUT_CHANNEL_REGISTER_4_OFS 0x0054
REGDEF_BEGIN(OUTPUT_CHANNEL_REGISTER_4)
    REGDEF_BIT(DRAM_OFSO2,        29)
REGDEF_END(OUTPUT_CHANNEL_REGISTER_4)


/*
    DRAM_OFSO3:    [0x0, 0x1fffffff],			bits : 28_0
*/
#define OUTPUT_CHANNEL_REGISTER_5_OFS 0x0058
REGDEF_BEGIN(OUTPUT_CHANNEL_REGISTER_5)
    REGDEF_BIT(DRAM_OFSO3,        29)
REGDEF_END(OUTPUT_CHANNEL_REGISTER_5)


/*
    WIDTH :    [0x0, 0x3ff],			bits : 9_0
    HEIGHT:    [0x0, 0x3ff],			bits : 21_12
*/
#define INPUT_SIZE_REGISTER0_OFS 0x005c
REGDEF_BEGIN(INPUT_SIZE_REGISTER0)
    REGDEF_BIT(WIDTH ,        10)
    REGDEF_BIT(      ,        2)
    REGDEF_BIT(HEIGHT,        10)
REGDEF_END(INPUT_SIZE_REGISTER0)


/*
    CHANNEL  :    [0x0, 0xfff],			bits : 11_0
    BATCH_NUM:    [0x0, 0x7f],			bits : 18_12
*/
#define INPUT_SIZE_REGISTER1_OFS 0x0060
REGDEF_BEGIN(INPUT_SIZE_REGISTER1)
    REGDEF_BIT(CHANNEL  ,        12)
    REGDEF_BIT(BATCH_NUM,        7)
REGDEF_END(INPUT_SIZE_REGISTER1)


/*
    IN_ISIMAGE        :    [0x0, 0x1],			bits : 0
    IN_IS_HSTRIPE     :    [0x0, 0x1],			bits : 1
    IN_IS_VSTRIPE     :    [0x0, 0x1],			bits : 2
    IN_IS_CHSTRIPE    :    [0x0, 0x1],			bits : 3
    IN_SIGNEDNESS     :    [0x0, 0x1],			bits : 4
    IN_BITDEPTH       :    [0x0, 0x1],			bits : 5
    OUT0_SIGNEDNESS   :    [0x0, 0x1],			bits : 6
    OUT1_SIGNEDNESS   :    [0x0, 0x1],			bits : 7
    OUT0_EN           :    [0x0, 0x1],			bits : 8
    OUT0_BITDEPTH     :    [0x0, 0x1],			bits : 9
    OUT0_MODE         :    [0x0, 0x3],			bits : 11_10
    OUT1_BITDEPTH     :    [0x0, 0x1],			bits : 12
    OUT1_EN           :    [0x0, 0x1],			bits : 13
    POOLING_SIGNEDNESS:    [0x0, 0x1],			bits : 14
    OUT0_OFFSET       :    [0x0, 0x7],			bits : 18_16
    IN_REMAINSRC_EN   :    [0x0, 0x1],			bits : 20
    IN_FLIP_EN        :    [0x0, 0x1],			bits : 21
    IN_INTERLACE_EN   :    [0x0, 0x1],			bits : 22
*/
#define INPUT_OUTPUT_REGISTER_OFS 0x0064
REGDEF_BEGIN(INPUT_OUTPUT_REGISTER)
    REGDEF_BIT(IN_ISIMAGE        ,        1)
    REGDEF_BIT(IN_IS_HSTRIPE     ,        1)
    REGDEF_BIT(IN_IS_VSTRIPE     ,        1)
    REGDEF_BIT(IN_IS_CHSTRIPE    ,        1)
    REGDEF_BIT(IN_SIGNEDNESS     ,        1)
    REGDEF_BIT(IN_BITDEPTH       ,        1)
    REGDEF_BIT(OUT0_SIGNEDNESS   ,        1)
    REGDEF_BIT(OUT1_SIGNEDNESS   ,        1)
    REGDEF_BIT(OUT0_EN           ,        1)
    REGDEF_BIT(OUT0_BITDEPTH     ,        1)
    REGDEF_BIT(OUT0_MODE         ,        2)
    REGDEF_BIT(OUT1_BITDEPTH     ,        1)
    REGDEF_BIT(OUT1_EN           ,        1)
    REGDEF_BIT(POOLING_SIGNEDNESS,        1)
    REGDEF_BIT(                  ,        1)
    REGDEF_BIT(OUT0_OFFSET       ,        3)
    REGDEF_BIT(                  ,        1)
    REGDEF_BIT(IN_REMAINSRC_EN   ,        1)
    REGDEF_BIT(IN_FLIP_EN        ,        1)
    REGDEF_BIT(IN_INTERLACE_EN   ,        1)
REGDEF_END(INPUT_OUTPUT_REGISTER)


/*
    CONV_KERSEL   :    [0x0, 0xf],			bits : 3_0
    CONV_STRIDE   :    [0x0, 0x3],			bits : 5_4
    CONV_SETNUM   :    [0x0, 0xfff],			bits : 19_8
    CONV_SHIFT_ACC:    [0x0, 0x7],			bits : 22_20
*/
#define CONVOLUTION_REGISTER0_OFS 0x0068
REGDEF_BEGIN(CONVOLUTION_REGISTER0)
    REGDEF_BIT(CONV_KERSEL   ,        4)
    REGDEF_BIT(CONV_STRIDE   ,        2)
    REGDEF_BIT(              ,        2)
    REGDEF_BIT(CONV_SETNUM   ,        12)
    REGDEF_BIT(CONV_SHIFT_ACC,        3)
REGDEF_END(CONVOLUTION_REGISTER0)


/*
    CONV_TOP_PAD   :    [0x0, 0x1],			bits : 0
    CONV_BOTTOM_PAD:    [0x0, 0x1],			bits : 1
    CONV_LEFT_PAD  :    [0x0, 0x1],			bits : 2
    CONV_RIGHT_PAD :    [0x0, 0x1],			bits : 3
    CONV_SHIFT_B   :    [0x0, 0x1f],			bits : 12_8
*/
#define CONVOLUTION_REGISTER1_OFS 0x006c
REGDEF_BEGIN(CONVOLUTION_REGISTER1)
    REGDEF_BIT(CONV_TOP_PAD   ,        1)
    REGDEF_BIT(CONV_BOTTOM_PAD,        1)
    REGDEF_BIT(CONV_LEFT_PAD  ,        1)
    REGDEF_BIT(CONV_RIGHT_PAD ,        1)
    REGDEF_BIT(               ,        4)
    REGDEF_BIT(CONV_SHIFT_B   ,        5)
REGDEF_END(CONVOLUTION_REGISTER1)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CONVOLUTION_REGISTER2_OFS 0x0070
REGDEF_BEGIN(CONVOLUTION_REGISTER2)
    REGDEF_BIT(reserved,        32)
REGDEF_END(CONVOLUTION_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CONVOLUTION_REGISTER3_OFS 0x0074
REGDEF_BEGIN(CONVOLUTION_REGISTER3)
    REGDEF_BIT(reserved,        32)
REGDEF_END(CONVOLUTION_REGISTER3)


/*
    BATCHNORM_SHIFT_M:    [0x0, 0x1f],			bits : 4_0
    SCALE_SHIFT_B    :    [0x0, 0x1f],			bits : 12_8
    SCALE_SHIFT_ALPHA:    [0x0, 0x1f],			bits : 25_21
*/
#define BNSCALE_REGISTER0_OFS 0x0078
REGDEF_BEGIN(BNSCALE_REGISTER0)
    REGDEF_BIT(BATCHNORM_SHIFT_M,        5)
    REGDEF_BIT(                 ,        3)
    REGDEF_BIT(SCALE_SHIFT_B    ,        5)
    REGDEF_BIT(                 ,        8)
    REGDEF_BIT(SCALE_SHIFT_ALPHA,        5)
REGDEF_END(BNSCALE_REGISTER0)


#define CNN_RESERVED_OFS_0 0x007C
REGDEF_BEGIN(CNN_RESERVED_0)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_0) //0x007C


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define BNSCALE_REGISTER2_OFS 0x0080
REGDEF_BEGIN(BNSCALE_REGISTER2)
    REGDEF_BIT(reserved,        32)
REGDEF_END(BNSCALE_REGISTER2)


/*
    ELTWISE_INPUT_SIGN     :    [0x0, 0x1],			bits : 0
    ELTWISE_INPUT_BIT_DEPTH:    [0x0, 0x1],			bits : 1
    ELTWISE_SHIFT0         :    [0x0, 0x1f],			bits : 8_4
    ELTWISE_SHIFT1         :    [0x0, 0x1f],			bits : 16_12
    ELTWISE_OUTSHF         :    [0x0, 0x1f],			bits : 24_20
*/
#define ELTWISE_REGISTER0_OFS 0x0084
REGDEF_BEGIN(ELTWISE_REGISTER0)
    REGDEF_BIT(ELTWISE_INPUT_SIGN     ,        1)
    REGDEF_BIT(ELTWISE_INPUT_BIT_DEPTH,        1)
    REGDEF_BIT(                       ,        2)
    REGDEF_BIT(ELTWISE_SHIFT0         ,        5)
    REGDEF_BIT(                       ,        3)
    REGDEF_BIT(ELTWISE_SHIFT1         ,        5)
    REGDEF_BIT(                       ,        3)
    REGDEF_BIT(ELTWISE_OUTSHF         ,        5)
REGDEF_END(ELTWISE_REGISTER0)


/*
    ELTWISE_COEFF0:    [0x0, 0xffff],			bits : 15_0
    ELTWISE_COEFF1:    [0x0, 0xffff],			bits : 31_16
*/
#define ELTWISE_REGISTER1_OFS 0x0088
REGDEF_BEGIN(ELTWISE_REGISTER1)
    REGDEF_BIT(ELTWISE_COEFF0,        16)
    REGDEF_BIT(ELTWISE_COEFF1,        16)
REGDEF_END(ELTWISE_REGISTER1)


/*
    PRE_RELU_LEAKY_VAL  :    [0x0, 0x7ff],			bits : 10_0
    PRE_RELU_LEAKY_SHIFT:    [0x0, 0xf],			bits : 15_12
    PRE_RELU_NEGATION   :    [0x0, 0x1],			bits : 16
*/
#define ACTIVATION_REGISTER0_OFS 0x008c
REGDEF_BEGIN(ACTIVATION_REGISTER0)
    REGDEF_BIT(PRE_RELU_LEAKY_VAL  ,        11)
    REGDEF_BIT(                    ,        1)
    REGDEF_BIT(PRE_RELU_LEAKY_SHIFT,        4)
    REGDEF_BIT(PRE_RELU_NEGATION   ,        1)
REGDEF_END(ACTIVATION_REGISTER0)


/*
    RELU_LEAKY_VAL0  :    [0x0, 0x7ff],			bits : 10_0
    RELU_LEAKY_SHIFT0:    [0x0, 0xf],			bits : 15_12
    RELU_NEGATION0   :    [0x0, 0x1],			bits : 16
*/
#define ACTIVATION_REGISTER1_OFS 0x0090
REGDEF_BEGIN(ACTIVATION_REGISTER1)
    REGDEF_BIT(RELU_LEAKY_VAL0  ,        11)
    REGDEF_BIT(                 ,        1)
    REGDEF_BIT(RELU_LEAKY_SHIFT0,        4)
    REGDEF_BIT(RELU_NEGATION0   ,        1)
REGDEF_END(ACTIVATION_REGISTER1)


/*
    RELU_LEAKY_VAL1  :    [0x0, 0x7ff],			bits : 10_0
    RELU_LEAKY_SHIFT1:    [0x0, 0xf],			bits : 15_12
    RELU_NEGATION1   :    [0x0, 0x1],			bits : 16
*/
#define ACTIVATION_REGISTER2_OFS 0x0094
REGDEF_BEGIN(ACTIVATION_REGISTER2)
    REGDEF_BIT(RELU_LEAKY_VAL1  ,        11)
    REGDEF_BIT(                 ,        1)
    REGDEF_BIT(RELU_LEAKY_SHIFT1,        4)
    REGDEF_BIT(RELU_NEGATION1   ,        1)
REGDEF_END(ACTIVATION_REGISTER2)


/*
    POOL_KERNEL      :    [0x0, 0x1],			bits : 0
    POOL_OUT_CAL_TYPE:    [0x0, 0x1],			bits : 1
    POOL_STRIDE      :    [0x0, 0x1],			bits : 4
    POOL_KERSIZE     :    [0x0, 0x3],			bits : 9_8
    POOL_TOP_PAD     :    [0x0, 0x1],			bits : 12
    POOL_BOTTOM_PAD  :    [0x0, 0x1],			bits : 13
    POOL_LEFT_PAD    :    [0x0, 0x1],			bits : 14
    POOL_RIGHT_PAD   :    [0x0, 0x1],			bits : 15
*/
#define POOL_REGISTER0_OFS 0x0098
REGDEF_BEGIN(POOL_REGISTER0)
    REGDEF_BIT(POOL_KERNEL      ,        1)
    REGDEF_BIT(POOL_OUT_CAL_TYPE,        1)
    REGDEF_BIT(                 ,        2)
    REGDEF_BIT(POOL_STRIDE      ,        1)
    REGDEF_BIT(                 ,        3)
    REGDEF_BIT(POOL_KERSIZE     ,        2)
    REGDEF_BIT(                 ,        2)
    REGDEF_BIT(POOL_TOP_PAD     ,        1)
    REGDEF_BIT(POOL_BOTTOM_PAD  ,        1)
    REGDEF_BIT(POOL_LEFT_PAD    ,        1)
    REGDEF_BIT(POOL_RIGHT_PAD   ,        1)
REGDEF_END(POOL_REGISTER0)


/*
    POOL_SHIFT_DIR   :    [0x0, 0x1],			bits : 0
    POOL_SHIFT       :    [0x0, 0x7],			bits : 6_4
    POOL_AVE_DIV_TYPE:    [0x0, 0x1],			bits : 7
    POOL_AVE_MUL     :    [0x0, 0x7ff],			bits : 18_8
    POOL_AVE_SHIFT   :    [0x0, 0x1f],			bits : 24_20
*/
#define POOL_REGISTER1_OFS 0x009c
REGDEF_BEGIN(POOL_REGISTER1)
    REGDEF_BIT(POOL_SHIFT_DIR   ,        1)
    REGDEF_BIT(                 ,        3)
    REGDEF_BIT(POOL_SHIFT       ,        3)
    REGDEF_BIT(POOL_AVE_DIV_TYPE,        1)
    REGDEF_BIT(POOL_AVE_MUL     ,        11)
    REGDEF_BIT(                 ,        1)
    REGDEF_BIT(POOL_AVE_SHIFT   ,        5)
REGDEF_END(POOL_REGISTER1)


/*
    DECONV_TOP_PAD   :    [0x0, 0x3],			bits : 1_0
    DECONV_BOTTOM_PAD:    [0x0, 0x3],			bits : 3_2
    DECONV_LEFT_PAD  :    [0x0, 0x3],			bits : 5_4
    DECONV_RIGHT_PAD :    [0x0, 0x3],			bits : 7_6
    DECONV_PAD_VALUE :    [0x0, 0xffff],			bits : 23_8
    DECONV_STRIDE    :    [0x0, 0x3],			bits : 25_24
*/
#define DECONV_REGISTER0_OFS 0x00a0
REGDEF_BEGIN(DECONV_REGISTER0)
    REGDEF_BIT(DECONV_TOP_PAD   ,        2)
    REGDEF_BIT(DECONV_BOTTOM_PAD,        2)
    REGDEF_BIT(DECONV_LEFT_PAD  ,        2)
    REGDEF_BIT(DECONV_RIGHT_PAD ,        2)
    REGDEF_BIT(DECONV_PAD_VALUE ,        16)
    REGDEF_BIT(DECONV_STRIDE    ,        2)
REGDEF_END(DECONV_REGISTER0)


/*
    SCALE_UP_RATE      :    [0x0, 0x3],			bits : 1_0
    SCALE_UP_TOP_PAD   :    [0x0, 0x3],			bits : 3_2
    SCALE_UP_BOTTOM_PAD:    [0x0, 0x3],			bits : 5_4
    SCALE_UP_LEFT_PAD  :    [0x0, 0x3],			bits : 7_6
    SCALE_UP_RIGHT_PAD :    [0x0, 0x3],			bits : 9_8
    SCALE_UP_PAD_VALUE :    [0x0, 0xffff],			bits : 27_12
*/
#define SCALEUP_REGISTER0_OFS 0x00a4
REGDEF_BEGIN(SCALEUP_REGISTER0)
    REGDEF_BIT(SCALE_UP_RATE      ,        2)
    REGDEF_BIT(SCALE_UP_TOP_PAD   ,        2)
    REGDEF_BIT(SCALE_UP_BOTTOM_PAD,        2)
    REGDEF_BIT(SCALE_UP_LEFT_PAD  ,        2)
    REGDEF_BIT(SCALE_UP_RIGHT_PAD ,        2)
    REGDEF_BIT(                   ,        2)
    REGDEF_BIT(SCALE_UP_PAD_VALUE ,        16)
REGDEF_END(SCALEUP_REGISTER0)


/*
    IN_SHIFT_DIR:    [0x0, 0x1],			bits : 0
    IN_SHIFT    :    [0x0, 0x1f],			bits : 8_4
    IN_SCALE    :    [0x0, 0xffff],			bits : 27_12
*/
#define SCALESHIFT_REGISTER0_OFS 0x00a8
REGDEF_BEGIN(SCALESHIFT_REGISTER0)
    REGDEF_BIT(IN_SHIFT_DIR,        1)
    REGDEF_BIT(            ,        3)
    REGDEF_BIT(IN_SHIFT    ,        5)
    REGDEF_BIT(            ,        3)
    REGDEF_BIT(IN_SCALE    ,        16)
REGDEF_END(SCALESHIFT_REGISTER0)


/*
    CONV_OUT_SHIFT_DIR:    [0x0, 0x1],			bits : 0
    CONV_OUT_SHIFT    :    [0x0, 0x1f],			bits : 8_4
    CONV_OUT_SCALE    :    [0x0, 0xffff],			bits : 27_12
*/
#define SCALESHIFT_REGISTER1_OFS 0x00ac
REGDEF_BEGIN(SCALESHIFT_REGISTER1)
    REGDEF_BIT(CONV_OUT_SHIFT_DIR,        1)
    REGDEF_BIT(                  ,        3)
    REGDEF_BIT(CONV_OUT_SHIFT    ,        5)
    REGDEF_BIT(                  ,        3)
    REGDEF_BIT(CONV_OUT_SCALE    ,        16)
REGDEF_END(SCALESHIFT_REGISTER1)


/*
    ELTWISE_OUT_SHIFT_DIR:    [0x0, 0x1],			bits : 0
    ELTWISE_OUT_SHIFT    :    [0x0, 0x1f],			bits : 8_4
    ELTWISE_OUT_SCALE    :    [0x0, 0xffff],			bits : 27_12
*/
#define SCALESHIFT_REGISTER2_OFS 0x00b0
REGDEF_BEGIN(SCALESHIFT_REGISTER2)
    REGDEF_BIT(ELTWISE_OUT_SHIFT_DIR,        1)
    REGDEF_BIT(                     ,        3)
    REGDEF_BIT(ELTWISE_OUT_SHIFT    ,        5)
    REGDEF_BIT(                     ,        3)
    REGDEF_BIT(ELTWISE_OUT_SCALE    ,        16)
REGDEF_END(SCALESHIFT_REGISTER2)


/*
    OUT0_SHIFT_DIR:    [0x0, 0x1],			bits : 0
    OUT0_SHIFT    :    [0x0, 0x1f],			bits : 8_4
    OUT0_SCALE    :    [0x0, 0xffff],			bits : 27_12
*/
#define SCALESHIFT_REGISTER3_OFS 0x00b4
REGDEF_BEGIN(SCALESHIFT_REGISTER3)
    REGDEF_BIT(OUT0_SHIFT_DIR,        1)
    REGDEF_BIT(              ,        3)
    REGDEF_BIT(OUT0_SHIFT    ,        5)
    REGDEF_BIT(              ,        3)
    REGDEF_BIT(OUT0_SCALE    ,        16)
REGDEF_END(SCALESHIFT_REGISTER3)


/*
    ACT_OUT1_SHIFT_DIR:    [0x0, 0x1],			bits : 0
    ACT_OUT1_SHIFT    :    [0x0, 0x1f],			bits : 8_4
    ACT_OUT1_SCALE    :    [0x0, 0xffff],			bits : 27_12
*/
#define SCALESHIFT_REGISTER4_OFS 0x00b8
REGDEF_BEGIN(SCALESHIFT_REGISTER4)
    REGDEF_BIT(ACT_OUT1_SHIFT_DIR,        1)
    REGDEF_BIT(                  ,        3)
    REGDEF_BIT(ACT_OUT1_SHIFT    ,        5)
    REGDEF_BIT(                  ,        3)
    REGDEF_BIT(ACT_OUT1_SCALE    ,        16)
REGDEF_END(SCALESHIFT_REGISTER4)


/*
    POOL_OUT_SHIFT_DIR:    [0x0, 0x1],			bits : 0
    POOL_OUT_SHIFT    :    [0x0, 0x1f],			bits : 8_4
    POOL_OUT_SCALE    :    [0x0, 0xffff],			bits : 27_12
*/
#define SCALESHIFT_REGISTER5_OFS 0x00bc
REGDEF_BEGIN(SCALESHIFT_REGISTER5)
    REGDEF_BIT(POOL_OUT_SHIFT_DIR,        1)
    REGDEF_BIT(                  ,        3)
    REGDEF_BIT(POOL_OUT_SHIFT    ,        5)
    REGDEF_BIT(                  ,        3)
    REGDEF_BIT(POOL_OUT_SCALE    ,        16)
REGDEF_END(SCALESHIFT_REGISTER5)


/*
    CNN_ENG_CYCLE:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CYCLE_COUNT_REGISTER0_OFS 0x00c0
REGDEF_BEGIN(CYCLE_COUNT_REGISTER0)
    REGDEF_BIT(CNN_ENG_CYCLE,        32)
REGDEF_END(CYCLE_COUNT_REGISTER0)


/*
    CNN_LL_CYCLE:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CYCLE_COUNT_REGISTER1_OFS 0x00c4
REGDEF_BEGIN(CYCLE_COUNT_REGISTER1)
    REGDEF_BIT(CNN_LL_CYCLE,        32)
REGDEF_END(CYCLE_COUNT_REGISTER1)


/*
    CNN_WAIT_DMA_CYCLE:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CYCLE_COUNT_REGISTER2_OFS 0x00c8
REGDEF_BEGIN(CYCLE_COUNT_REGISTER2)
    REGDEF_BIT(CNN_WAIT_DMA_CYCLE,        32)
REGDEF_END(CYCLE_COUNT_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define RESERVED_REGISTER3_OFS 0x00cc
REGDEF_BEGIN(RESERVED_REGISTER3)
    REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED_REGISTER3)


/*
    reserved:    [0x0, 0xffffffff],			bits : 31_0
*/
#define RESERVED_REGISTER4_OFS 0x00d0
REGDEF_BEGIN(RESERVED_REGISTER4)
    REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED_REGISTER4)


/*
    FCD_ENC_BIT_LENGTH:    [0x0, 0xffffffff],			bits : 31_0
*/
#define COMPRESSION_REGISTER0_OFS 0x00d4
REGDEF_BEGIN(COMPRESSION_REGISTER0)
    REGDEF_BIT(FCD_ENC_BIT_LENGTH,        32)
REGDEF_END(COMPRESSION_REGISTER0)


/*
    FCD_VLC_CODE0 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID0:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER1_OFS 0x00d8
REGDEF_BEGIN(COMPRESSION_REGISTER1)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE0 ,        23)
    REGDEF_BIT(FCD_VLC_VALID0,        1)
REGDEF_END(COMPRESSION_REGISTER1)


/*
    FCD_VLC_CODE1 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID1:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER2_OFS 0x00dc
REGDEF_BEGIN(COMPRESSION_REGISTER2)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE1 ,        23)
    REGDEF_BIT(FCD_VLC_VALID1,        1)
REGDEF_END(COMPRESSION_REGISTER2)


/*
    FCD_VLC_CODE2 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID2:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER3_OFS 0x00e0
REGDEF_BEGIN(COMPRESSION_REGISTER3)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE2 ,        23)
    REGDEF_BIT(FCD_VLC_VALID2,        1)
REGDEF_END(COMPRESSION_REGISTER3)


/*
    FCD_VLC_CODE3 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID3:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER4_OFS 0x00e4
REGDEF_BEGIN(COMPRESSION_REGISTER4)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE3 ,        23)
    REGDEF_BIT(FCD_VLC_VALID3,        1)
REGDEF_END(COMPRESSION_REGISTER4)


/*
    FCD_VLC_CODE4 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID4:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER5_OFS 0x00e8
REGDEF_BEGIN(COMPRESSION_REGISTER5)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE4 ,        23)
    REGDEF_BIT(FCD_VLC_VALID4,        1)
REGDEF_END(COMPRESSION_REGISTER5)


/*
    FCD_VLC_CODE5 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID5:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER6_OFS 0x00ec
REGDEF_BEGIN(COMPRESSION_REGISTER6)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE5 ,        23)
    REGDEF_BIT(FCD_VLC_VALID5,        1)
REGDEF_END(COMPRESSION_REGISTER6)


/*
    FCD_VLC_CODE6 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID6:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER7_OFS 0x00f0
REGDEF_BEGIN(COMPRESSION_REGISTER7)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE6 ,        23)
    REGDEF_BIT(FCD_VLC_VALID6,        1)
REGDEF_END(COMPRESSION_REGISTER7)


/*
    FCD_VLC_CODE7 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID7:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER8_OFS 0x00f4
REGDEF_BEGIN(COMPRESSION_REGISTER8)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE7 ,        23)
    REGDEF_BIT(FCD_VLC_VALID7,        1)
REGDEF_END(COMPRESSION_REGISTER8)


/*
    FCD_VLC_CODE8 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID8:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER9_OFS 0x00f8
REGDEF_BEGIN(COMPRESSION_REGISTER9)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE8 ,        23)
    REGDEF_BIT(FCD_VLC_VALID8,        1)
REGDEF_END(COMPRESSION_REGISTER9)


/*
    FCD_VLC_CODE9 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID9:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER10_OFS 0x00fc
REGDEF_BEGIN(COMPRESSION_REGISTER10)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE9 ,        23)
    REGDEF_BIT(FCD_VLC_VALID9,        1)
REGDEF_END(COMPRESSION_REGISTER10)


/*
    FCD_VLC_CODE10 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID10:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER11_OFS 0x0100
REGDEF_BEGIN(COMPRESSION_REGISTER11)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE10 ,        23)
    REGDEF_BIT(FCD_VLC_VALID10,        1)
REGDEF_END(COMPRESSION_REGISTER11)


/*
    FCD_VLC_CODE11 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID11:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER12_OFS 0x0104
REGDEF_BEGIN(COMPRESSION_REGISTER12)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE11 ,        23)
    REGDEF_BIT(FCD_VLC_VALID11,        1)
REGDEF_END(COMPRESSION_REGISTER12)


/*
    FCD_VLC_CODE12 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID12:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER13_OFS 0x0108
REGDEF_BEGIN(COMPRESSION_REGISTER13)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE12 ,        23)
    REGDEF_BIT(FCD_VLC_VALID12,        1)
REGDEF_END(COMPRESSION_REGISTER13)


/*
    FCD_VLC_CODE13 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID13:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER14_OFS 0x010c
REGDEF_BEGIN(COMPRESSION_REGISTER14)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE13 ,        23)
    REGDEF_BIT(FCD_VLC_VALID13,        1)
REGDEF_END(COMPRESSION_REGISTER14)


/*
    FCD_VLC_CODE14 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID14:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER15_OFS 0x0110
REGDEF_BEGIN(COMPRESSION_REGISTER15)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE14 ,        23)
    REGDEF_BIT(FCD_VLC_VALID14,        1)
REGDEF_END(COMPRESSION_REGISTER15)


/*
    FCD_VLC_CODE15 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID15:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER16_OFS 0x0114
REGDEF_BEGIN(COMPRESSION_REGISTER16)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE15 ,        23)
    REGDEF_BIT(FCD_VLC_VALID15,        1)
REGDEF_END(COMPRESSION_REGISTER16)


/*
    FCD_VLC_CODE16 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID16:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER17_OFS 0x0118
REGDEF_BEGIN(COMPRESSION_REGISTER17)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE16 ,        23)
    REGDEF_BIT(FCD_VLC_VALID16,        1)
REGDEF_END(COMPRESSION_REGISTER17)


/*
    FCD_VLC_CODE17 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID17:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER18_OFS 0x011c
REGDEF_BEGIN(COMPRESSION_REGISTER18)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE17 ,        23)
    REGDEF_BIT(FCD_VLC_VALID17,        1)
REGDEF_END(COMPRESSION_REGISTER18)


/*
    FCD_VLC_CODE18 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID18:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER19_OFS 0x0120
REGDEF_BEGIN(COMPRESSION_REGISTER19)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE18 ,        23)
    REGDEF_BIT(FCD_VLC_VALID18,        1)
REGDEF_END(COMPRESSION_REGISTER19)


/*
    FCD_VLC_CODE19 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID19:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER20_OFS 0x0124
REGDEF_BEGIN(COMPRESSION_REGISTER20)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE19 ,        23)
    REGDEF_BIT(FCD_VLC_VALID19,        1)
REGDEF_END(COMPRESSION_REGISTER20)


/*
    FCD_VLC_CODE20 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID20:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER21_OFS 0x0128
REGDEF_BEGIN(COMPRESSION_REGISTER21)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE20 ,        23)
    REGDEF_BIT(FCD_VLC_VALID20,        1)
REGDEF_END(COMPRESSION_REGISTER21)


/*
    FCD_VLC_CODE21 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID21:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER22_OFS 0x012c
REGDEF_BEGIN(COMPRESSION_REGISTER22)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE21 ,        23)
    REGDEF_BIT(FCD_VLC_VALID21,        1)
REGDEF_END(COMPRESSION_REGISTER22)


/*
    FCD_VLC_CODE22 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID22:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER23_OFS 0x0130
REGDEF_BEGIN(COMPRESSION_REGISTER23)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE22 ,        23)
    REGDEF_BIT(FCD_VLC_VALID22,        1)
REGDEF_END(COMPRESSION_REGISTER23)


/*
    FCD_VLC_INDEX0:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX1:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER24_OFS 0x0134
REGDEF_BEGIN(COMPRESSION_REGISTER24)
    REGDEF_BIT(FCD_VLC_INDEX0,        16)
    REGDEF_BIT(FCD_VLC_INDEX1,        16)
REGDEF_END(COMPRESSION_REGISTER24)


/*
    FCD_VLC_INDEX2:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX3:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER25_OFS 0x0138
REGDEF_BEGIN(COMPRESSION_REGISTER25)
    REGDEF_BIT(FCD_VLC_INDEX2,        16)
    REGDEF_BIT(FCD_VLC_INDEX3,        16)
REGDEF_END(COMPRESSION_REGISTER25)


/*
    FCD_VLC_INDEX4:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX5:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER26_OFS 0x013c
REGDEF_BEGIN(COMPRESSION_REGISTER26)
    REGDEF_BIT(FCD_VLC_INDEX4,        16)
    REGDEF_BIT(FCD_VLC_INDEX5,        16)
REGDEF_END(COMPRESSION_REGISTER26)


/*
    FCD_VLC_INDEX6:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX7:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER27_OFS 0x0140
REGDEF_BEGIN(COMPRESSION_REGISTER27)
    REGDEF_BIT(FCD_VLC_INDEX6,        16)
    REGDEF_BIT(FCD_VLC_INDEX7,        16)
REGDEF_END(COMPRESSION_REGISTER27)


/*
    FCD_VLC_INDEX8:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX9:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER28_OFS 0x0144
REGDEF_BEGIN(COMPRESSION_REGISTER28)
    REGDEF_BIT(FCD_VLC_INDEX8,        16)
    REGDEF_BIT(FCD_VLC_INDEX9,        16)
REGDEF_END(COMPRESSION_REGISTER28)


/*
    FCD_VLC_INDEX10:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX11:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER29_OFS 0x0148
REGDEF_BEGIN(COMPRESSION_REGISTER29)
    REGDEF_BIT(FCD_VLC_INDEX10,        16)
    REGDEF_BIT(FCD_VLC_INDEX11,        16)
REGDEF_END(COMPRESSION_REGISTER29)


/*
    FCD_VLC_INDEX12:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX13:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER30_OFS 0x014c
REGDEF_BEGIN(COMPRESSION_REGISTER30)
    REGDEF_BIT(FCD_VLC_INDEX12,        16)
    REGDEF_BIT(FCD_VLC_INDEX13,        16)
REGDEF_END(COMPRESSION_REGISTER30)


/*
    FCD_VLC_INDEX14:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX15:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER31_OFS 0x0150
REGDEF_BEGIN(COMPRESSION_REGISTER31)
    REGDEF_BIT(FCD_VLC_INDEX14,        16)
    REGDEF_BIT(FCD_VLC_INDEX15,        16)
REGDEF_END(COMPRESSION_REGISTER31)


/*
    FCD_VLC_INDEX16:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX17:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER32_OFS 0x0154
REGDEF_BEGIN(COMPRESSION_REGISTER32)
    REGDEF_BIT(FCD_VLC_INDEX16,        16)
    REGDEF_BIT(FCD_VLC_INDEX17,        16)
REGDEF_END(COMPRESSION_REGISTER32)


/*
    FCD_VLC_INDEX18:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX19:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER33_OFS 0x0158
REGDEF_BEGIN(COMPRESSION_REGISTER33)
    REGDEF_BIT(FCD_VLC_INDEX18,        16)
    REGDEF_BIT(FCD_VLC_INDEX19,        16)
REGDEF_END(COMPRESSION_REGISTER33)


/*
    FCD_VLC_INDEX20:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX21:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER34_OFS 0x015c
REGDEF_BEGIN(COMPRESSION_REGISTER34)
    REGDEF_BIT(FCD_VLC_INDEX20,        16)
    REGDEF_BIT(FCD_VLC_INDEX21,        16)
REGDEF_END(COMPRESSION_REGISTER34)


/*
    FCD_VLC_INDEX22:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_INDEX23:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER35_OFS 0x0160
REGDEF_BEGIN(COMPRESSION_REGISTER35)
    REGDEF_BIT(FCD_VLC_INDEX22,        16)
    REGDEF_BIT(FCD_VLC_INDEX23,        16)
REGDEF_END(COMPRESSION_REGISTER35)


/*
    LL_TABLE_IDX0:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX1:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX2:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX3:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER0_OFS 0x0164
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
#define LL_FRAME_REGISTER1_OFS 0x0168
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
#define LL_FRAME_REGISTER2_OFS 0x016c
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
#define LL_FRAME_REGISTER3_OFS 0x0170
REGDEF_BEGIN(LL_FRAME_REGISTER3)
    REGDEF_BIT(LL_TABLE_IDX12,        8)
    REGDEF_BIT(LL_TABLE_IDX13,        8)
    REGDEF_BIT(LL_TABLE_IDX14,        8)
    REGDEF_BIT(LL_TABLE_IDX15,        8)
REGDEF_END(LL_FRAME_REGISTER3)


/*
    PARAM_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define PARAMETER_CHECKSUM_OFS 0x0174
REGDEF_BEGIN(PARAMETER_CHECKSUM)
    REGDEF_BIT(PARAM_CHKSUM,        32)
REGDEF_END(PARAMETER_CHECKSUM)


/*
    CONV_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CONVOLUTION_CHECKSUM_OFS 0x0178
REGDEF_BEGIN(CONVOLUTION_CHECKSUM)
    REGDEF_BIT(CONV_CHKSUM,        32)
REGDEF_END(CONVOLUTION_CHECKSUM)


/*
    PREACT_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define PRE_ACT_CHECKSUM_OFS 0x017c
REGDEF_BEGIN(PRE_ACT_CHECKSUM)
    REGDEF_BIT(PREACT_CHKSUM,        32)
REGDEF_END(PRE_ACT_CHECKSUM)


/*
    BNSCALE_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define BN_SCALE_CHECKSUM_OFS 0x0180
REGDEF_BEGIN(BN_SCALE_CHECKSUM)
    REGDEF_BIT(BNSCALE_CHKSUM,        32)
REGDEF_END(BN_SCALE_CHECKSUM)


/*
    ELTWISE_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define ELTWISE_CHECKSUM_OFS 0x0184
REGDEF_BEGIN(ELTWISE_CHECKSUM)
    REGDEF_BIT(ELTWISE_CHKSUM,        32)
REGDEF_END(ELTWISE_CHECKSUM)


/*
    ACT0_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define ACT0_CHECKSUM_OFS 0x0188
REGDEF_BEGIN(ACT0_CHECKSUM)
    REGDEF_BIT(ACT0_CHKSUM,        32)
REGDEF_END(ACT0_CHECKSUM)


/*
    ACT1_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define ACT1_CHECKSUM_OFS 0x018c
REGDEF_BEGIN(ACT1_CHECKSUM)
    REGDEF_BIT(ACT1_CHKSUM,        32)
REGDEF_END(ACT1_CHECKSUM)


/*
    POOL_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define POOLING_CHECKSUM_OFS 0x0190
REGDEF_BEGIN(POOLING_CHECKSUM)
    REGDEF_BIT(POOL_CHKSUM,        32)
REGDEF_END(POOLING_CHECKSUM)


/*
    DECONV_SCALEUP_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define DECONVOLUTION_SCALE_UP_CHECKSUM_OFS 0x0194
REGDEF_BEGIN(DECONVOLUTION_SCALE_UP_CHECKSUM)
    REGDEF_BIT(DECONV_SCALEUP_CHKSUM,        32)
REGDEF_END(DECONVOLUTION_SCALE_UP_CHECKSUM)


/*
    SAI0_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define INPUT_CHECKSUM_REGISTER0_OFS 0x0198
REGDEF_BEGIN(INPUT_CHECKSUM_REGISTER0)
    REGDEF_BIT(SAI0_CHKSUM,        32)
REGDEF_END(INPUT_CHECKSUM_REGISTER0)


/*
    ELTWISE_IN_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define INPUT_CHECKSUM_REGISTER1_OFS 0x019c
REGDEF_BEGIN(INPUT_CHECKSUM_REGISTER1)
    REGDEF_BIT(ELTWISE_IN_CHKSUM,        32)
REGDEF_END(INPUT_CHECKSUM_REGISTER1)


/*
    BNSCALE_IN_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define INPUT_CHECKSUM_REGISTER2_OFS 0x01a0
REGDEF_BEGIN(INPUT_CHECKSUM_REGISTER2)
    REGDEF_BIT(BNSCALE_IN_CHKSUM,        32)
REGDEF_END(INPUT_CHECKSUM_REGISTER2)


/*
    OUT0_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define OUTPUT_CHECKSUM_REGISTER0_OFS 0x01a4
REGDEF_BEGIN(OUTPUT_CHECKSUM_REGISTER0)
    REGDEF_BIT(OUT0_CHKSUM,        32)
REGDEF_END(OUTPUT_CHECKSUM_REGISTER0)


/*
    OUT1_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define OUTPUT_CHECKSUM_REGISTER1_OFS 0x01a8
REGDEF_BEGIN(OUTPUT_CHECKSUM_REGISTER1)
    REGDEF_BIT(OUT1_CHKSUM,        32)
REGDEF_END(OUTPUT_CHECKSUM_REGISTER1)


/*
    LBW_CTRL_CHKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CHECKSUM_OFS 0x01ac
REGDEF_BEGIN(CHECKSUM)
    REGDEF_BIT(LBW_CTRL_CHKSUM,        32)
REGDEF_END(CHECKSUM)


#define CNN_RESERVED_OFS_1 0x01B0
REGDEF_BEGIN(CNN_RESERVED_1)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_1) //0x01B0


#define CNN_RESERVED_OFS_2 0x01B4
REGDEF_BEGIN(CNN_RESERVED_2)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_2) //0x01B4


#define CNN_RESERVED_OFS_3 0x01B8
REGDEF_BEGIN(CNN_RESERVED_3)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_3) //0x01B8


#define CNN_RESERVED_OFS_4 0x01BC
REGDEF_BEGIN(CNN_RESERVED_4)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_4) //0x01BC


#define CNN_RESERVED_OFS_5 0x01C0
REGDEF_BEGIN(CNN_RESERVED_5)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_5) //0x01C0


#define CNN_RESERVED_OFS_6 0x01C4
REGDEF_BEGIN(CNN_RESERVED_6)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_6) //0x01C4


#define CNN_RESERVED_OFS_7 0x01C8
REGDEF_BEGIN(CNN_RESERVED_7)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_7) //0x01C8


#define CNN_RESERVED_OFS_8 0x01CC
REGDEF_BEGIN(CNN_RESERVED_8)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_8) //0x01CC


#define CNN_RESERVED_OFS_9 0x01D0
REGDEF_BEGIN(CNN_RESERVED_9)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_9) //0x01D0


#define CNN_RESERVED_OFS_10 0x01D4
REGDEF_BEGIN(CNN_RESERVED_10)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_10) //0x01D4


#define CNN_RESERVED_OFS_11 0x01D8
REGDEF_BEGIN(CNN_RESERVED_11)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_11) //0x01D8


#define CNN_RESERVED_OFS_12 0x01DC
REGDEF_BEGIN(CNN_RESERVED_12)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_12) //0x01DC


#define CNN_RESERVED_OFS_13 0x01E0
REGDEF_BEGIN(CNN_RESERVED_13)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_13) //0x01E0


#define CNN_RESERVED_OFS_14 0x01E4
REGDEF_BEGIN(CNN_RESERVED_14)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_14) //0x01E4


#define CNN_RESERVED_OFS_15 0x01E8
REGDEF_BEGIN(CNN_RESERVED_15)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_15) //0x01E8


#define CNN_RESERVED_OFS_16 0x01EC
REGDEF_BEGIN(CNN_RESERVED_16)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_16) //0x01EC


#define CNN_RESERVED_OFS_17 0x01F0
REGDEF_BEGIN(CNN_RESERVED_17)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_17) //0x01F0


#define CNN_RESERVED_OFS_18 0x01F4
REGDEF_BEGIN(CNN_RESERVED_18)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_18) //0x01F4


/*
    LL_TERMINATE:    [0x0, 0x1],			bits : 0
*/
#define LL_TERMINATE_RESISTER0_OFS 0x01f8
REGDEF_BEGIN(LL_TERMINATE_RESISTER0)
    REGDEF_BIT(LL_TERMINATE,        1)
REGDEF_END(LL_TERMINATE_RESISTER0)


/*
    DMA_DISABLE:    [0x0, 0x1],			bits : 0
    CNN_IDLE   :    [0x0, 0x1],			bits : 15
*/
#define DMA_DISABLE_REGISTER0_OFS 0x01fc
REGDEF_BEGIN(DMA_DISABLE_REGISTER0)
    REGDEF_BIT(DMA_DISABLE,        1)
    REGDEF_BIT(           ,        14)
    REGDEF_BIT(CNN_IDLE   ,        1)
REGDEF_END(DMA_DISABLE_REGISTER0)


/*
    CHECK_SUM_ENABLE :    [0x0, 0x1],			bits : 0
    BN_DMA_DISABLE   :    [0x0, 0x1],			bits : 1
    IN0_BURST_LENGTH :    [0x0, 0x3],			bits : 3_2
    IN1_BURST_LENGTH :    [0x0, 0x3],			bits : 5_4
    IN2_BURST_LENGTH :    [0x0, 0x3],			bits : 9_8
    OUT0_BURST_LENGTH:    [0x0, 0x3],			bits : 11_10
    OUT1_BURST_LENGTH:    [0x0, 0x3],			bits : 13_12
    DBG_PORT_SEL     :    [0x0, 0x3],			bits : 15_14
    CYCLE_COUNT_EN   :    [0x0, 0x1],			bits : 16
*/
#define DESIGN_DEBUG_REGISTER0_OFS 0x0200
REGDEF_BEGIN(DESIGN_DEBUG_REGISTER0)
    REGDEF_BIT(CHECK_SUM_ENABLE ,        1)
    REGDEF_BIT(BN_DMA_DISABLE   ,        1)
    REGDEF_BIT(IN0_BURST_LENGTH ,        2)
    REGDEF_BIT(IN1_BURST_LENGTH ,        2)
    REGDEF_BIT(                 ,        2)
    REGDEF_BIT(IN2_BURST_LENGTH ,        2)
    REGDEF_BIT(OUT0_BURST_LENGTH,        2)
    REGDEF_BIT(OUT1_BURST_LENGTH,        2)
    REGDEF_BIT(DBG_PORT_SEL     ,        2)
    REGDEF_BIT(CYCLE_COUNT_EN   ,        1)
REGDEF_END(DESIGN_DEBUG_REGISTER0)


/*
    LL_BASE_ADDR:    [0x0, 0xffffffff],			bits : 31_0
*/
#define LL_BASE_ADDRESS_REGISTER0_OFS 0x0204
REGDEF_BEGIN(LL_BASE_ADDRESS_REGISTER0)
    REGDEF_BIT(LL_BASE_ADDR,        32)
REGDEF_END(LL_BASE_ADDRESS_REGISTER0)


#define CNN_RESERVED_OFS_19 0x0208
REGDEF_BEGIN(CNN_RESERVED_19)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_19) //0x0208


/*
    CNN_LLC_ERR_CMD_CNT:    [0x0, 0xffffffff],			bits : 31_0
*/
#define LLC_DEBUG_RESISTER_0_OFS 0x020c
REGDEF_BEGIN(LLC_DEBUG_RESISTER_0)
    REGDEF_BIT(CNN_LLC_ERR_CMD_CNT,        32)
REGDEF_END(LLC_DEBUG_RESISTER_0)


/*
    CNN_LLC_ERR_CMD_ADDR:    [0x0, 0xffffffff],			bits : 31_0
*/
#define LLC_DEBUG_RESISTER_1_OFS 0x0210
REGDEF_BEGIN(LLC_DEBUG_RESISTER_1)
    REGDEF_BIT(CNN_LLC_ERR_CMD_ADDR,        32)
REGDEF_END(LLC_DEBUG_RESISTER_1)


#define CNN_RESERVED_OFS_20 0x0214
REGDEF_BEGIN(CNN_RESERVED_20)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_20) //0x0214


#define CNN_RESERVED_OFS_21 0x0218
REGDEF_BEGIN(CNN_RESERVED_21)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_21) //0x0218


#define CNN_RESERVED_OFS_22 0x021C
REGDEF_BEGIN(CNN_RESERVED_22)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(CNN_RESERVED_22) //0x021C


/*
    CNN_COMPARE_CHECKSUM0:    [0x0, 0xffffffff],			bits : 31_0
*/
#define COMPARE_CHECKSUM_REGISTER_0_OFS 0x0220
REGDEF_BEGIN(COMPARE_CHECKSUM_REGISTER_0)
    REGDEF_BIT(CNN_COMPARE_CHECKSUM0,        32)
REGDEF_END(COMPARE_CHECKSUM_REGISTER_0)


/*
    CNN_COMPARE_CHECKSUM1:    [0x0, 0xffffffff],			bits : 31_0
*/
#define COMPARE_CHECKSUM_REGISTER_1_OFS 0x0224
REGDEF_BEGIN(COMPARE_CHECKSUM_REGISTER_1)
    REGDEF_BIT(CNN_COMPARE_CHECKSUM1,        32)
REGDEF_END(COMPARE_CHECKSUM_REGISTER_1)


/*
    CNN_COMPARE_CHECKSUM2:    [0x0, 0xffffffff],			bits : 31_0
*/
#define COMPARE_CHECKSUM_REGISTER_2_OFS 0x0228
REGDEF_BEGIN(COMPARE_CHECKSUM_REGISTER_2)
    REGDEF_BIT(CNN_COMPARE_CHECKSUM2,        32)
REGDEF_END(COMPARE_CHECKSUM_REGISTER_2)


/*
    CNN_CHKSUM_MISMATCH_IDX_CNT:    [0x0, 0xffff],			bits : 15_0
    reserved                   :    [0x0, 0xffff],			bits : 31_16
*/
#define CHECKSUM_DEBUG_COUNTER_RESISTER_0_OFS 0x022c
REGDEF_BEGIN(CHECKSUM_DEBUG_COUNTER_RESISTER_0)
    REGDEF_BIT(CNN_CHKSUM_MISMATCH_IDX_CNT,        16)
    REGDEF_BIT(reserved                   ,        16)
REGDEF_END(CHECKSUM_DEBUG_COUNTER_RESISTER_0)


typedef struct
{
    //0x0000
    T_CNN_CONTROL_REGISTER 
    CNN_Register_0000;

    //0x0004
    T_CNN_MODE_REGISTER 
    CNN_Register_0004;

    //0x0008
    T_CNN_INTERRUPT_ENABLE_REGISTER 
    CNN_Register_0008;

    //0x000c
    T_CNN_INTERRUPT_STATUS_REGISTER 
    CNN_Register_000c;

    //0x0010
    T_INPUT_CHANNEL_REGISTER_0 
    CNN_Register_0010;

    //0x0014
    T_INPUT_CHANNEL_REGISTER_1 
    CNN_Register_0014;

    //0x0018
    T_INPUT_CHANNEL_REGISTER_2 
    CNN_Register_0018;

    //0x001c
    T_INPUT_CHANNEL_REGISTER_3 
    CNN_Register_001c;

    //0x0020
    T_INPUT_CHANNEL_REGISTER_4 
    CNN_Register_0020;

    //0x0024
    T_INPUT_CHANNEL_REGISTER_5 
    CNN_Register_0024;

    //0x0028
    T_INPUT_CHANNEL_REGISTER_6 
    CNN_Register_0028;

    //0x002c
    T_INPUT_CHANNEL_REGISTER_7 
    CNN_Register_002c;

    //0x0030
    T_INPUT_CHANNEL_REGISTER_8 
    CNN_Register_0030;

    //0x0034
    T_INPUT_CHANNEL_REGISTER_9 
    CNN_Register_0034;

    //0x0038
    T_INPUT_CHANNEL_REGISTER_10 
    CNN_Register_0038;

    //0x003c
    T_INPUT_CHANNEL_REGISTER_11 
    CNN_Register_003c;

    //0x0040
    T_INPUT_CHANNEL_REGISTER_12 
    CNN_Register_0040;

    //0x0044
    T_OUTPUT_CHANNEL_REGISTER_0 
    CNN_Register_0044;

    //0x0048
    T_OUTPUT_CHANNEL_REGISTER_1 
    CNN_Register_0048;

    //0x004c
    T_OUTPUT_CHANNEL_REGISTER_2 
    CNN_Register_004c;

    //0x0050
    T_OUTPUT_CHANNEL_REGISTER_3 
    CNN_Register_0050;

    //0x0054
    T_OUTPUT_CHANNEL_REGISTER_4 
    CNN_Register_0054;

    //0x0058
    T_OUTPUT_CHANNEL_REGISTER_5 
    CNN_Register_0058;

    //0x005c
    T_INPUT_SIZE_REGISTER0 
    CNN_Register_005c;

    //0x0060
    T_INPUT_SIZE_REGISTER1 
    CNN_Register_0060;

    //0x0064
    T_INPUT_OUTPUT_REGISTER 
    CNN_Register_0064;

    //0x0068
    T_CONVOLUTION_REGISTER0 
    CNN_Register_0068;

    //0x006c
    T_CONVOLUTION_REGISTER1 
    CNN_Register_006c;

    //0x0070
    T_CONVOLUTION_REGISTER2 
    CNN_Register_0070;

    //0x0074
    T_CONVOLUTION_REGISTER3 
    CNN_Register_0074;

    //0x0078
    T_BNSCALE_REGISTER0 
    CNN_Register_0078;

    //0x007c
    T_CNN_RESERVED_0 
    CNN_Register_007c;

    //0x0080
    T_BNSCALE_REGISTER2 
    CNN_Register_0080;

    //0x0084
    T_ELTWISE_REGISTER0 
    CNN_Register_0084;

    //0x0088
    T_ELTWISE_REGISTER1 
    CNN_Register_0088;

    //0x008c
    T_ACTIVATION_REGISTER0 
    CNN_Register_008c;

    //0x0090
    T_ACTIVATION_REGISTER1 
    CNN_Register_0090;

    //0x0094
    T_ACTIVATION_REGISTER2 
    CNN_Register_0094;

    //0x0098
    T_POOL_REGISTER0 
    CNN_Register_0098;

    //0x009c
    T_POOL_REGISTER1 
    CNN_Register_009c;

    //0x00a0
    T_DECONV_REGISTER0 
    CNN_Register_00a0;

    //0x00a4
    T_SCALEUP_REGISTER0 
    CNN_Register_00a4;

    //0x00a8
    T_SCALESHIFT_REGISTER0 
    CNN_Register_00a8;

    //0x00ac
    T_SCALESHIFT_REGISTER1 
    CNN_Register_00ac;

    //0x00b0
    T_SCALESHIFT_REGISTER2 
    CNN_Register_00b0;

    //0x00b4
    T_SCALESHIFT_REGISTER3 
    CNN_Register_00b4;

    //0x00b8
    T_SCALESHIFT_REGISTER4 
    CNN_Register_00b8;

    //0x00bc
    T_SCALESHIFT_REGISTER5 
    CNN_Register_00bc;

    //0x00c0
    T_CYCLE_COUNT_REGISTER0 
    CNN_Register_00c0;

    //0x00c4
    T_CYCLE_COUNT_REGISTER1 
    CNN_Register_00c4;

    //0x00c8
    T_CYCLE_COUNT_REGISTER2 
    CNN_Register_00c8;

    //0x00cc
    T_RESERVED_REGISTER3 
    CNN_Register_00cc;

    //0x00d0
    T_RESERVED_REGISTER4 
    CNN_Register_00d0;

    //0x00d4
    T_COMPRESSION_REGISTER0 
    CNN_Register_00d4;

    //0x00d8
    T_COMPRESSION_REGISTER1 
    CNN_Register_00d8;

    //0x00dc
    T_COMPRESSION_REGISTER2 
    CNN_Register_00dc;

    //0x00e0
    T_COMPRESSION_REGISTER3 
    CNN_Register_00e0;

    //0x00e4
    T_COMPRESSION_REGISTER4 
    CNN_Register_00e4;

    //0x00e8
    T_COMPRESSION_REGISTER5 
    CNN_Register_00e8;

    //0x00ec
    T_COMPRESSION_REGISTER6 
    CNN_Register_00ec;

    //0x00f0
    T_COMPRESSION_REGISTER7 
    CNN_Register_00f0;

    //0x00f4
    T_COMPRESSION_REGISTER8 
    CNN_Register_00f4;

    //0x00f8
    T_COMPRESSION_REGISTER9 
    CNN_Register_00f8;

    //0x00fc
    T_COMPRESSION_REGISTER10 
    CNN_Register_00fc;

    //0x0100
    T_COMPRESSION_REGISTER11 
    CNN_Register_0100;

    //0x0104
    T_COMPRESSION_REGISTER12 
    CNN_Register_0104;

    //0x0108
    T_COMPRESSION_REGISTER13 
    CNN_Register_0108;

    //0x010c
    T_COMPRESSION_REGISTER14 
    CNN_Register_010c;

    //0x0110
    T_COMPRESSION_REGISTER15 
    CNN_Register_0110;

    //0x0114
    T_COMPRESSION_REGISTER16 
    CNN_Register_0114;

    //0x0118
    T_COMPRESSION_REGISTER17 
    CNN_Register_0118;

    //0x011c
    T_COMPRESSION_REGISTER18 
    CNN_Register_011c;

    //0x0120
    T_COMPRESSION_REGISTER19 
    CNN_Register_0120;

    //0x0124
    T_COMPRESSION_REGISTER20 
    CNN_Register_0124;

    //0x0128
    T_COMPRESSION_REGISTER21 
    CNN_Register_0128;

    //0x012c
    T_COMPRESSION_REGISTER22 
    CNN_Register_012c;

    //0x0130
    T_COMPRESSION_REGISTER23 
    CNN_Register_0130;

    //0x0134
    T_COMPRESSION_REGISTER24 
    CNN_Register_0134;

    //0x0138
    T_COMPRESSION_REGISTER25 
    CNN_Register_0138;

    //0x013c
    T_COMPRESSION_REGISTER26 
    CNN_Register_013c;

    //0x0140
    T_COMPRESSION_REGISTER27 
    CNN_Register_0140;

    //0x0144
    T_COMPRESSION_REGISTER28 
    CNN_Register_0144;

    //0x0148
    T_COMPRESSION_REGISTER29 
    CNN_Register_0148;

    //0x014c
    T_COMPRESSION_REGISTER30 
    CNN_Register_014c;

    //0x0150
    T_COMPRESSION_REGISTER31 
    CNN_Register_0150;

    //0x0154
    T_COMPRESSION_REGISTER32 
    CNN_Register_0154;

    //0x0158
    T_COMPRESSION_REGISTER33 
    CNN_Register_0158;

    //0x015c
    T_COMPRESSION_REGISTER34 
    CNN_Register_015c;

    //0x0160
    T_COMPRESSION_REGISTER35 
    CNN_Register_0160;

    //0x0164
    T_LL_FRAME_REGISTER0 
    CNN_Register_0164;

    //0x0168
    T_LL_FRAME_REGISTER1 
    CNN_Register_0168;

    //0x016c
    T_LL_FRAME_REGISTER2 
    CNN_Register_016c;

    //0x0170
    T_LL_FRAME_REGISTER3 
    CNN_Register_0170;

    //0x0174
    T_PARAMETER_CHECKSUM 
    CNN_Register_0174;

    //0x0178
    T_CONVOLUTION_CHECKSUM 
    CNN_Register_0178;

    //0x017c
    T_PRE_ACT_CHECKSUM 
    CNN_Register_017c;

    //0x0180
    T_BN_SCALE_CHECKSUM 
    CNN_Register_0180;

    //0x0184
    T_ELTWISE_CHECKSUM 
    CNN_Register_0184;

    //0x0188
    T_ACT0_CHECKSUM 
    CNN_Register_0188;

    //0x018c
    T_ACT1_CHECKSUM 
    CNN_Register_018c;

    //0x0190
    T_POOLING_CHECKSUM 
    CNN_Register_0190;

    //0x0194
    T_DECONVOLUTION_SCALE_UP_CHECKSUM 
    CNN_Register_0194;

    //0x0198
    T_INPUT_CHECKSUM_REGISTER0 
    CNN_Register_0198;

    //0x019c
    T_INPUT_CHECKSUM_REGISTER1 
    CNN_Register_019c;

    //0x01a0
    T_INPUT_CHECKSUM_REGISTER2 
    CNN_Register_01a0;

    //0x01a4
    T_OUTPUT_CHECKSUM_REGISTER0 
    CNN_Register_01a4;

    //0x01a8
    T_OUTPUT_CHECKSUM_REGISTER1 
    CNN_Register_01a8;

    //0x01ac
    T_CHECKSUM 
    CNN_Register_01ac;

    //0x01b0
    T_CNN_RESERVED_1 
    CNN_Register_01b0;

    //0x01b4
    T_CNN_RESERVED_2 
    CNN_Register_01b4;

    //0x01b8
    T_CNN_RESERVED_3 
    CNN_Register_01b8;

    //0x01bc
    T_CNN_RESERVED_4 
    CNN_Register_01bc;

    //0x01c0
    T_CNN_RESERVED_5 
    CNN_Register_01c0;

    //0x01c4
    T_CNN_RESERVED_6 
    CNN_Register_01c4;

    //0x01c8
    T_CNN_RESERVED_7 
    CNN_Register_01c8;

    //0x01cc
    T_CNN_RESERVED_8 
    CNN_Register_01cc;

    //0x01d0
    T_CNN_RESERVED_9 
    CNN_Register_01d0;

    //0x01d4
    T_CNN_RESERVED_10 
    CNN_Register_01d4;

    //0x01d8
    T_CNN_RESERVED_11 
    CNN_Register_01d8;

    //0x01dc
    T_CNN_RESERVED_12 
    CNN_Register_01dc;

    //0x01e0
    T_CNN_RESERVED_13 
    CNN_Register_01e0;

    //0x01e4
    T_CNN_RESERVED_14 
    CNN_Register_01e4;

    //0x01e8
    T_CNN_RESERVED_15 
    CNN_Register_01e8;

    //0x01ec
    T_CNN_RESERVED_16 
    CNN_Register_01ec;

    //0x01f0
    T_CNN_RESERVED_17 
    CNN_Register_01f0;

    //0x01f4
    T_CNN_RESERVED_18 
    CNN_Register_01f4;

    //0x01f8
    T_LL_TERMINATE_RESISTER0 
    CNN_Register_01f8;

    //0x01fc
    T_DMA_DISABLE_REGISTER0 
    CNN_Register_01fc;

    //0x0200
    T_DESIGN_DEBUG_REGISTER0 
    CNN_Register_0200;

    //0x0204
    T_LL_BASE_ADDRESS_REGISTER0 
    CNN_Register_0204;

    //0x0208
    T_CNN_RESERVED_19 
    CNN_Register_0208;

    //0x020c
    T_LLC_DEBUG_RESISTER_0 
    CNN_Register_020c;

    //0x0210
    T_LLC_DEBUG_RESISTER_1 
    CNN_Register_0210;

    //0x0214
    T_CNN_RESERVED_20 
    CNN_Register_0214;

    //0x0218
    T_CNN_RESERVED_21 
    CNN_Register_0218;

    //0x021c
    T_CNN_RESERVED_22 
    CNN_Register_021c;

    //0x0220
    T_COMPARE_CHECKSUM_REGISTER_0 
    CNN_Register_0220;

    //0x0224
    T_COMPARE_CHECKSUM_REGISTER_1 
    CNN_Register_0224;

    //0x0228
    T_COMPARE_CHECKSUM_REGISTER_2 
    CNN_Register_0228;

    //0x022c
    T_CHECKSUM_DEBUG_COUNTER_RESISTER_0 
    CNN_Register_022c;

} NT98560_CNN_REG_STRUCT;

#endif
