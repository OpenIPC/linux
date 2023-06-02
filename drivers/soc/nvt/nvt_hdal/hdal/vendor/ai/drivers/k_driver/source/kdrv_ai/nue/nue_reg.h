#ifndef _NUE_REG_H_
#define _NUE_REG_H_

#ifdef __cplusplus
extern "C" {
#endif


//#include "DrvCommon.h"
//#include "mach/RegMarco.h"
#if defined(__FREERTOS)
#include "rcw_macro.h"
#else
#include "mach/rcw_macro.h"
#endif
//#include "rcw_macro.h"
//#include "..\\IPP_AP_Driver_Common.h"

/*
    NUE_RST  :    [0x0, 0x1],			bits : 0
    NUE_START:    [0x0, 0x1],			bits : 1
    LL_FIRE  :    [0x0, 0x1],			bits : 28
*/
#define NUE_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(NUE_CONTROL_REGISTER)
    REGDEF_BIT(NUE_RST  ,        1)
    REGDEF_BIT(NUE_START,        1)
    REGDEF_BIT(         ,        26)
    REGDEF_BIT(LL_FIRE  ,        1)
REGDEF_END(NUE_CONTROL_REGISTER)


/*
    NUE_MODE      :    [0x0, 0x7],			bits : 2_0
    IN_SIGNEDNESS :    [0x0, 0x1],			bits : 4
    OUT_SIGNEDNESS:    [0x0, 0x1],			bits : 8
    IN_BIT_DEPTH  :    [0x0, 0x1],			bits : 12
    OUT_BIT_DEPTH :    [0x0, 0x1],			bits : 16
*/
#define NUE_MODE_REGISTER0_OFS 0x0004
REGDEF_BEGIN(NUE_MODE_REGISTER0)
    REGDEF_BIT(NUE_MODE      ,        3)
    REGDEF_BIT(              ,        1)
    REGDEF_BIT(IN_SIGNEDNESS ,        1)
    REGDEF_BIT(              ,        3)
    REGDEF_BIT(OUT_SIGNEDNESS,        1)
    REGDEF_BIT(              ,        3)
    REGDEF_BIT(IN_BIT_DEPTH  ,        1)
    REGDEF_BIT(              ,        3)
    REGDEF_BIT(OUT_BIT_DEPTH ,        1)
REGDEF_END(NUE_MODE_REGISTER0)


/*
    SVM_KERNEL1_MODE          :    [0x0, 0x3],			bits : 1_0
    SVM_KERNEL2_MODE          :    [0x0, 0x1],			bits : 2
    PERMUTE_MODE              :    [0x0, 0x1],			bits : 3
    SVM_RESULT_MODE           :    [0x0, 0x3],			bits : 5_4
    SVM_DMAOEN                :    [0x0, 0x1],			bits : 6
    SVM_DMAO_PATH             :    [0x0, 0x3],			bits : 9_8
    SVM_AR_MODE               :    [0x0, 0x1],			bits : 10
    SVM_RHO_MODE              :    [0x0, 0x1],			bits : 11
    SVM_LUT_MODE              :    [0x0, 0x1],			bits : 12
    SVM_KERNEL2_EN            :    [0x0, 0x1],			bits : 16
    SVM_CAL_EN                :    [0x0, 0x1],			bits : 17
    SVM_INTERLACE_EN          :    [0x0, 0x1],			bits : 18
    FCD_VLC_EN                :    [0x0, 0x1],			bits : 20
    FCD_QUANTIZATION_EN       :    [0x0, 0x1],			bits : 21
    FCD_SPARSE_EN             :    [0x0, 0x1],			bits : 22
    FCD_QUANTIZATION_KMEANS   :    [0x0, 0x1],			bits : 23
    FCD_KQ_TBL_UPDATE_DISABLE :    [0x0, 0x1],			bits : 24
    RELU_EN                   :    [0x0, 0x1],			bits : 25
    SVM_INTERMEDIATE_IN_EN    :    [0x0, 0x1],			bits : 26
    SVM_SCIENTIFIC_NOTATION_EN:    [0x0, 0x1],			bits : 27
*/
#define NUE_MODE_REGISTER1_OFS 0x0008
REGDEF_BEGIN(NUE_MODE_REGISTER1)
    REGDEF_BIT(SVM_KERNEL1_MODE          ,        2)
    REGDEF_BIT(SVM_KERNEL2_MODE          ,        1)
    REGDEF_BIT(PERMUTE_MODE              ,        1)
    REGDEF_BIT(SVM_RESULT_MODE           ,        2)
    REGDEF_BIT(SVM_DMAOEN                ,        1)
    REGDEF_BIT(                          ,        1)
    REGDEF_BIT(SVM_DMAO_PATH             ,        2)
    REGDEF_BIT(SVM_AR_MODE               ,        1)
    REGDEF_BIT(SVM_RHO_MODE              ,        1)
    REGDEF_BIT(SVM_LUT_MODE              ,        1)
    REGDEF_BIT(                          ,        3)
    REGDEF_BIT(SVM_KERNEL2_EN            ,        1)
    REGDEF_BIT(SVM_CAL_EN                ,        1)
    REGDEF_BIT(SVM_INTERLACE_EN          ,        1)
    REGDEF_BIT(                          ,        1)
    REGDEF_BIT(FCD_VLC_EN                ,        1)
    REGDEF_BIT(FCD_QUANTIZATION_EN       ,        1)
    REGDEF_BIT(FCD_SPARSE_EN             ,        1)
    REGDEF_BIT(FCD_QUANTIZATION_KMEANS   ,        1)
    REGDEF_BIT(FCD_KQ_TBL_UPDATE_DISABLE ,        1)
    REGDEF_BIT(RELU_EN                   ,        1)
    REGDEF_BIT(SVM_INTERMEDIATE_IN_EN    ,        1)
    REGDEF_BIT(SVM_SCIENTIFIC_NOTATION_EN,        1)
REGDEF_END(NUE_MODE_REGISTER1)


/*
    DRAM_SAI0:    [0x0, 0xffffffff],			bits : 31_0
*/
#define DMA_TO_NUE_REGISTER0_OFS 0x000c
REGDEF_BEGIN(DMA_TO_NUE_REGISTER0)
    REGDEF_BIT(DRAM_SAI0,        32)
REGDEF_END(DMA_TO_NUE_REGISTER0)


/*
    DRAM_SAI1:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_NUE_REGISTER1_OFS 0x0010
REGDEF_BEGIN(DMA_TO_NUE_REGISTER1)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_SAI1,        30)
REGDEF_END(DMA_TO_NUE_REGISTER1)


/*
    DRAM_SAISV:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_NUE_REGISTER2_OFS 0x0014
REGDEF_BEGIN(DMA_TO_NUE_REGISTER2)
    REGDEF_BIT(          ,        2)
    REGDEF_BIT(DRAM_SAISV,        30)
REGDEF_END(DMA_TO_NUE_REGISTER2)


/*
    DRAM_SAIALPHA:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_NUE_REGISTER3_OFS 0x0018
REGDEF_BEGIN(DMA_TO_NUE_REGISTER3)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(DRAM_SAIALPHA,        30)
REGDEF_END(DMA_TO_NUE_REGISTER3)


/*
    DRAM_SAIRHO:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_NUE_REGISTER4_OFS 0x001c
REGDEF_BEGIN(DMA_TO_NUE_REGISTER4)
    REGDEF_BIT(           ,        2)
    REGDEF_BIT(DRAM_SAIRHO,        30)
REGDEF_END(DMA_TO_NUE_REGISTER4)


#define NUE_RESERVED_OFS_0 0x0020
REGDEF_BEGIN(NUE_RESERVED_0)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_0) //0x0020


/*
    DRAM_SAILL:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_NUE_REGISTER6_OFS 0x0024
REGDEF_BEGIN(DMA_TO_NUE_REGISTER6)
    REGDEF_BIT(          ,        2)
    REGDEF_BIT(DRAM_SAILL,        30)
REGDEF_END(DMA_TO_NUE_REGISTER6)


/*
    DRAM_SAIKQ:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DMA_TO_NUE_REGISTER7_OFS 0x0028
REGDEF_BEGIN(DMA_TO_NUE_REGISTER7)
    REGDEF_BIT(          ,        2)
    REGDEF_BIT(DRAM_SAIKQ,        30)
REGDEF_END(DMA_TO_NUE_REGISTER7)


/*
    DRAM_SAOR:    [0x0, 0xffffffff],			bits : 31_0
*/
#define NUE_TO_DMA_RESULT_REGISTER0_OFS 0x002c
REGDEF_BEGIN(NUE_TO_DMA_RESULT_REGISTER0)
    REGDEF_BIT(DRAM_SAOR,        32)
REGDEF_END(NUE_TO_DMA_RESULT_REGISTER0)


/*
    DRAM_OFSI:    [0x0, 0x3ffff],			bits : 19_2
*/
#define INPUT_FEATURE_LINE_OFFSET_REGISTER_OFS 0x0030
REGDEF_BEGIN(INPUT_FEATURE_LINE_OFFSET_REGISTER)
    REGDEF_BIT(         ,        2)
    REGDEF_BIT(DRAM_OFSI,        18)
REGDEF_END(INPUT_FEATURE_LINE_OFFSET_REGISTER)


/*
    DRAM_OFSO:    [0x0, 0xfffff],			bits : 19_0
*/
#define OUTPUT_FEATURE_LINE_OFFSET_REGISTER_OFS 0x0034
REGDEF_BEGIN(OUTPUT_FEATURE_LINE_OFFSET_REGISTER)
    REGDEF_BIT(DRAM_OFSO,        20)
REGDEF_END(OUTPUT_FEATURE_LINE_OFFSET_REGISTER)


/*
    INTE_FRM_END                  :    [0x0, 0x1],			bits : 0
    INTE_DMAIN0END                :    [0x0, 0x1],			bits : 1
    INTE_DMAIN1END                :    [0x0, 0x1],			bits : 2
    INTE_DMAIN2END                :    [0x0, 0x1],			bits : 3
    INTE_LLEND                    :    [0x0, 0x1],			bits : 8
    INTE_LLERROR                  :    [0x0, 0x1],			bits : 9
    INTE_LLJOBEND                 :    [0x0, 0x1],			bits : 10
    INTE_CNN_OCCUPY_APP_TO_APP_ERR:    [0x0, 0x1],			bits : 12
    INTE_CNN_OCCUPY_APP_TO_LL_ERR :    [0x0, 0x1],			bits : 13
    INTE_CNN_OCCUPY_LL_TO_APP_ERR :    [0x0, 0x1],			bits : 14
    INTE_CNN_OCCUPY_LL_TO_LL_ERR  :    [0x0, 0x1],			bits : 15
    INTE_FCD_DECODE_DONE          :    [0x0, 0x1],			bits : 16
    INTE_FCD_VLC_DEC_ERR          :    [0x0, 0x1],			bits : 17
    INTE_FCD_BS_SIZE_ERR          :    [0x0, 0x1],			bits : 18
    INTE_FCD_SPARSE_DATA_ERR      :    [0x0, 0x1],			bits : 19
    INTE_FCD_SPARSE_INDEX_ERR     :    [0x0, 0x1],			bits : 20
    INTE_CHECKSUM_MISMATCH        :    [0x0, 0x1],			bits : 25
*/
#define NUE_INTERRUPT_ENABLE_REGISTER_OFS 0x0038
REGDEF_BEGIN(NUE_INTERRUPT_ENABLE_REGISTER)
    REGDEF_BIT(INTE_FRM_END                  ,        1)
    REGDEF_BIT(INTE_DMAIN0END                ,        1)
    REGDEF_BIT(INTE_DMAIN1END                ,        1)
    REGDEF_BIT(INTE_DMAIN2END                ,        1)
    REGDEF_BIT(                              ,        4)
    REGDEF_BIT(INTE_LLEND                    ,        1)
    REGDEF_BIT(INTE_LLERROR                  ,        1)
    REGDEF_BIT(INTE_LLJOBEND                 ,        1)
    REGDEF_BIT(                              ,        1)
    REGDEF_BIT(INTE_CNN_OCCUPY_APP_TO_APP_ERR,        1)
    REGDEF_BIT(INTE_CNN_OCCUPY_APP_TO_LL_ERR ,        1)
    REGDEF_BIT(INTE_CNN_OCCUPY_LL_TO_APP_ERR ,        1)
    REGDEF_BIT(INTE_CNN_OCCUPY_LL_TO_LL_ERR  ,        1)
    REGDEF_BIT(INTE_FCD_DECODE_DONE          ,        1)
    REGDEF_BIT(INTE_FCD_VLC_DEC_ERR          ,        1)
    REGDEF_BIT(INTE_FCD_BS_SIZE_ERR          ,        1)
    REGDEF_BIT(INTE_FCD_SPARSE_DATA_ERR      ,        1)
    REGDEF_BIT(INTE_FCD_SPARSE_INDEX_ERR     ,        1)
    REGDEF_BIT(                              ,        4)
    REGDEF_BIT(INTE_CHECKSUM_MISMATCH        ,        1)
REGDEF_END(NUE_INTERRUPT_ENABLE_REGISTER)


/*
    INT_FRM_END                  :    [0x0, 0x1],			bits : 0
    INT_DMAIN0END                :    [0x0, 0x1],			bits : 1
    INT_DMAIN1END                :    [0x0, 0x1],			bits : 2
    INT_DMAIN2END                :    [0x0, 0x1],			bits : 3
    INT_LLEND                    :    [0x0, 0x1],			bits : 8
    INT_LLERROR                  :    [0x0, 0x1],			bits : 9
    INT_LLJOBEND                 :    [0x0, 0x1],			bits : 10
    INT_CNN_OCCUPY_APP_TO_APP_ERR:    [0x0, 0x1],			bits : 12
    INT_CNN_OCCUPY_APP_TO_LL_ERR :    [0x0, 0x1],			bits : 13
    INT_CNN_OCCUPY_LL_TO_APP_ERR :    [0x0, 0x1],			bits : 14
    INT_CNN_OCCUPY_LL_TO_LL_ERR  :    [0x0, 0x1],			bits : 15
    INT_FCD_DECODE_DONE          :    [0x0, 0x1],			bits : 16
    INT_FCD_VLC_DEC_ERR          :    [0x0, 0x1],			bits : 17
    INT_FCD_BS_SIZE_ERR          :    [0x0, 0x1],			bits : 18
    INT_FCD_SPARSE_DATA_ERR      :    [0x0, 0x1],			bits : 19
    INT_FCD_SPARSE_INDEX_ERR     :    [0x0, 0x1],			bits : 20
    INT_CHECKSUM_MISMATCH        :    [0x0, 0x1],			bits : 25
*/
#define NUE_INTERRUPT_STATUS_REGISTER_OFS 0x003c
REGDEF_BEGIN(NUE_INTERRUPT_STATUS_REGISTER)
    REGDEF_BIT(INT_FRM_END                  ,        1)
    REGDEF_BIT(INT_DMAIN0END                ,        1)
    REGDEF_BIT(INT_DMAIN1END                ,        1)
    REGDEF_BIT(INT_DMAIN2END                ,        1)
    REGDEF_BIT(                             ,        4)
    REGDEF_BIT(INT_LLEND                    ,        1)
    REGDEF_BIT(INT_LLERROR                  ,        1)
    REGDEF_BIT(INT_LLJOBEND                 ,        1)
    REGDEF_BIT(                             ,        1)
    REGDEF_BIT(INT_CNN_OCCUPY_APP_TO_APP_ERR,        1)
    REGDEF_BIT(INT_CNN_OCCUPY_APP_TO_LL_ERR ,        1)
    REGDEF_BIT(INT_CNN_OCCUPY_LL_TO_APP_ERR ,        1)
    REGDEF_BIT(INT_CNN_OCCUPY_LL_TO_LL_ERR  ,        1)
    REGDEF_BIT(INT_FCD_DECODE_DONE          ,        1)
    REGDEF_BIT(INT_FCD_VLC_DEC_ERR          ,        1)
    REGDEF_BIT(INT_FCD_BS_SIZE_ERR          ,        1)
    REGDEF_BIT(INT_FCD_SPARSE_DATA_ERR      ,        1)
    REGDEF_BIT(INT_FCD_SPARSE_INDEX_ERR     ,        1)
    REGDEF_BIT(                             ,        4)
    REGDEF_BIT(INT_CHECKSUM_MISMATCH        ,        1)
REGDEF_END(NUE_INTERRUPT_STATUS_REGISTER)


/*
    NUE_WIDTH :    [0x0, 0x3fff],			bits : 13_0
    NUE_HEIGHT:    [0x0, 0x1fff],			bits : 28_16
*/
#define INPUT_SIZE_REGISTER0_OFS 0x0040
REGDEF_BEGIN(INPUT_SIZE_REGISTER0)
    REGDEF_BIT(NUE_WIDTH ,        14)
    REGDEF_BIT(          ,        2)
    REGDEF_BIT(NUE_HEIGHT,        13)
REGDEF_END(INPUT_SIZE_REGISTER0)


/*
    NUE_SVM_INSIZE:    [0x0, 0x3fff],			bits : 13_0
    NUE_CHANNEL   :    [0x0, 0xfff],			bits : 27_16
*/
#define INPUT_SIZE_REGISTER1_OFS 0x0044
REGDEF_BEGIN(INPUT_SIZE_REGISTER1)
    REGDEF_BIT(NUE_SVM_INSIZE,        14)
    REGDEF_BIT(              ,        2)
    REGDEF_BIT(NUE_CHANNEL   ,        12)
REGDEF_END(INPUT_SIZE_REGISTER1)


/*
    OBJ_NUM:    [0x0, 0x7f],			bits : 6_0
    IN_RFH :    [0x0, 0x1],			bits : 16
*/
#define INPUT_SIZE_REGISTER2_OFS 0x0048
REGDEF_BEGIN(INPUT_SIZE_REGISTER2)
    REGDEF_BIT(OBJ_NUM,        7)
    REGDEF_BIT(       ,        9)
    REGDEF_BIT(IN_RFH ,        1)
REGDEF_END(INPUT_SIZE_REGISTER2)


/*
    KER1_GV      :    [0x0, 0xfff],			bits : 11_0
    KER1_GV_SHIFT:    [0x0, 0xf],			bits : 19_16
    KER2_DEGREE  :    [0x0, 0x7],			bits : 22_20
*/
#define KERNEL_REGISTER0_OFS 0x004c
REGDEF_BEGIN(KERNEL_REGISTER0)
    REGDEF_BIT(KER1_GV      ,        12)
    REGDEF_BIT(             ,        4)
    REGDEF_BIT(KER1_GV_SHIFT,        4)
    REGDEF_BIT(KER2_DEGREE  ,        3)
REGDEF_END(KERNEL_REGISTER0)


/*
    KER1_COEF    :    [0x0, 0x1fffff],			bits : 20_0
    KER1_FT_SHIFT:    [0x0, 0xf],			bits : 27_24
*/
#define KERNEL_REGISTER1_OFS 0x0050
REGDEF_BEGIN(KERNEL_REGISTER1)
    REGDEF_BIT(KER1_COEF    ,        21)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(KER1_FT_SHIFT,        4)
REGDEF_END(KERNEL_REGISTER1)


/*
    RELU_LEAKY_VAL       :    [0x0, 0x7ff],			bits : 10_0
    RELU_LEAKY_SHIFT     :    [0x0, 0xf],			bits : 15_12
    RELU_SHIFT_SIGNEDNESS:    [0x0, 0x1],			bits : 16
    RELU_SHIFT           :    [0x0, 0xf],			bits : 23_20
*/
#define RELU_REGISTER0_OFS 0x0054
REGDEF_BEGIN(RELU_REGISTER0)
    REGDEF_BIT(RELU_LEAKY_VAL       ,        11)
    REGDEF_BIT(                     ,        1)
    REGDEF_BIT(RELU_LEAKY_SHIFT     ,        4)
    REGDEF_BIT(RELU_SHIFT_SIGNEDNESS,        1)
    REGDEF_BIT(                     ,        3)
    REGDEF_BIT(RELU_SHIFT           ,        4)
REGDEF_END(RELU_REGISTER0)


/*
    RHO_REG    :    [0x0, 0xfff],			bits : 11_0
    RHO_FMT    :    [0x0, 0xf],			bits : 19_16
    ALPHA_SHIFT:    [0x0, 0xf],			bits : 23_20
*/
#define RESULT_REGISTER0_OFS 0x0058
REGDEF_BEGIN(RESULT_REGISTER0)
    REGDEF_BIT(RHO_REG    ,        12)
    REGDEF_BIT(           ,        4)
    REGDEF_BIT(RHO_FMT    ,        4)
    REGDEF_BIT(ALPHA_SHIFT,        4)
REGDEF_END(RESULT_REGISTER0)


#define NUE_RESERVED_OFS_1 0x005C
REGDEF_BEGIN(NUE_RESERVED_1)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_1) //0x005C


/*
    REORGANIZE_SHIFT_SIGNEDNESS:    [0x0, 0x1],			bits : 16
    REORGANIZE_SHIFT           :    [0x0, 0x7],			bits : 22_20
*/
#define REORGANIZE_REGISTER0_OFS 0x0060
REGDEF_BEGIN(REORGANIZE_REGISTER0)
    REGDEF_BIT(                           ,        16)
    REGDEF_BIT(REORGANIZE_SHIFT_SIGNEDNESS,        1)
    REGDEF_BIT(                           ,        3)
    REGDEF_BIT(REORGANIZE_SHIFT           ,        3)
REGDEF_END(REORGANIZE_REGISTER0)


/*
    Result0:    [0x0, 0xffff],			bits : 15_0
    Result1:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER0_OFS 0x0064
REGDEF_BEGIN(SVM_RESULT_REGISTER0)
    REGDEF_BIT(Result0,        16)
    REGDEF_BIT(Result1,        16)
REGDEF_END(SVM_RESULT_REGISTER0)


/*
    Result2:    [0x0, 0xffff],			bits : 15_0
    Result3:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER1_OFS 0x0068
REGDEF_BEGIN(SVM_RESULT_REGISTER1)
    REGDEF_BIT(Result2,        16)
    REGDEF_BIT(Result3,        16)
REGDEF_END(SVM_RESULT_REGISTER1)


/*
    Result4:    [0x0, 0xffff],			bits : 15_0
    Result5:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER2_OFS 0x006c
REGDEF_BEGIN(SVM_RESULT_REGISTER2)
    REGDEF_BIT(Result4,        16)
    REGDEF_BIT(Result5,        16)
REGDEF_END(SVM_RESULT_REGISTER2)


/*
    Result6:    [0x0, 0xffff],			bits : 15_0
    Result7:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER3_OFS 0x0070
REGDEF_BEGIN(SVM_RESULT_REGISTER3)
    REGDEF_BIT(Result6,        16)
    REGDEF_BIT(Result7,        16)
REGDEF_END(SVM_RESULT_REGISTER3)


/*
    Result8:    [0x0, 0xffff],			bits : 15_0
    Result9:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER4_OFS 0x0074
REGDEF_BEGIN(SVM_RESULT_REGISTER4)
    REGDEF_BIT(Result8,        16)
    REGDEF_BIT(Result9,        16)
REGDEF_END(SVM_RESULT_REGISTER4)


/*
    Result10:    [0x0, 0xffff],			bits : 15_0
    Result11:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER5_OFS 0x0078
REGDEF_BEGIN(SVM_RESULT_REGISTER5)
    REGDEF_BIT(Result10,        16)
    REGDEF_BIT(Result11,        16)
REGDEF_END(SVM_RESULT_REGISTER5)


/*
    Result12:    [0x0, 0xffff],			bits : 15_0
    Result13:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER6_OFS 0x007c
REGDEF_BEGIN(SVM_RESULT_REGISTER6)
    REGDEF_BIT(Result12,        16)
    REGDEF_BIT(Result13,        16)
REGDEF_END(SVM_RESULT_REGISTER6)


/*
    Result14:    [0x0, 0xffff],			bits : 15_0
    Result15:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER7_OFS 0x0080
REGDEF_BEGIN(SVM_RESULT_REGISTER7)
    REGDEF_BIT(Result14,        16)
    REGDEF_BIT(Result15,        16)
REGDEF_END(SVM_RESULT_REGISTER7)


/*
    Result16:    [0x0, 0xffff],			bits : 15_0
    Result17:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER8_OFS 0x0084
REGDEF_BEGIN(SVM_RESULT_REGISTER8)
    REGDEF_BIT(Result16,        16)
    REGDEF_BIT(Result17,        16)
REGDEF_END(SVM_RESULT_REGISTER8)


/*
    Result18:    [0x0, 0xffff],			bits : 15_0
    Result19:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER9_OFS 0x0088
REGDEF_BEGIN(SVM_RESULT_REGISTER9)
    REGDEF_BIT(Result18,        16)
    REGDEF_BIT(Result19,        16)
REGDEF_END(SVM_RESULT_REGISTER9)


/*
    Result20:    [0x0, 0xffff],			bits : 15_0
    Result21:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER10_OFS 0x008c
REGDEF_BEGIN(SVM_RESULT_REGISTER10)
    REGDEF_BIT(Result20,        16)
    REGDEF_BIT(Result21,        16)
REGDEF_END(SVM_RESULT_REGISTER10)


/*
    Result22:    [0x0, 0xffff],			bits : 15_0
    Result23:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER11_OFS 0x0090
REGDEF_BEGIN(SVM_RESULT_REGISTER11)
    REGDEF_BIT(Result22,        16)
    REGDEF_BIT(Result23,        16)
REGDEF_END(SVM_RESULT_REGISTER11)


/*
    Result24:    [0x0, 0xffff],			bits : 15_0
    Result25:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER12_OFS 0x0094
REGDEF_BEGIN(SVM_RESULT_REGISTER12)
    REGDEF_BIT(Result24,        16)
    REGDEF_BIT(Result25,        16)
REGDEF_END(SVM_RESULT_REGISTER12)


/*
    Result26:    [0x0, 0xffff],			bits : 15_0
    Result27:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER13_OFS 0x0098
REGDEF_BEGIN(SVM_RESULT_REGISTER13)
    REGDEF_BIT(Result26,        16)
    REGDEF_BIT(Result27,        16)
REGDEF_END(SVM_RESULT_REGISTER13)


/*
    Result28:    [0x0, 0xffff],			bits : 15_0
    Result29:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER14_OFS 0x009c
REGDEF_BEGIN(SVM_RESULT_REGISTER14)
    REGDEF_BIT(Result28,        16)
    REGDEF_BIT(Result29,        16)
REGDEF_END(SVM_RESULT_REGISTER14)


/*
    Result30:    [0x0, 0xffff],			bits : 15_0
    Result31:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER15_OFS 0x00a0
REGDEF_BEGIN(SVM_RESULT_REGISTER15)
    REGDEF_BIT(Result30,        16)
    REGDEF_BIT(Result31,        16)
REGDEF_END(SVM_RESULT_REGISTER15)


/*
    Result32:    [0x0, 0xffff],			bits : 15_0
    Result33:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER16_OFS 0x00a4
REGDEF_BEGIN(SVM_RESULT_REGISTER16)
    REGDEF_BIT(Result32,        16)
    REGDEF_BIT(Result33,        16)
REGDEF_END(SVM_RESULT_REGISTER16)


/*
    Result34:    [0x0, 0xffff],			bits : 15_0
    Result35:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER17_OFS 0x00a8
REGDEF_BEGIN(SVM_RESULT_REGISTER17)
    REGDEF_BIT(Result34,        16)
    REGDEF_BIT(Result35,        16)
REGDEF_END(SVM_RESULT_REGISTER17)


/*
    Result36:    [0x0, 0xffff],			bits : 15_0
    Result37:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER18_OFS 0x00ac
REGDEF_BEGIN(SVM_RESULT_REGISTER18)
    REGDEF_BIT(Result36,        16)
    REGDEF_BIT(Result37,        16)
REGDEF_END(SVM_RESULT_REGISTER18)


/*
    Result38:    [0x0, 0xffff],			bits : 15_0
    Result39:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER19_OFS 0x00b0
REGDEF_BEGIN(SVM_RESULT_REGISTER19)
    REGDEF_BIT(Result38,        16)
    REGDEF_BIT(Result39,        16)
REGDEF_END(SVM_RESULT_REGISTER19)


/*
    Result40:    [0x0, 0xffff],			bits : 15_0
    Result41:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER20_OFS 0x00b4
REGDEF_BEGIN(SVM_RESULT_REGISTER20)
    REGDEF_BIT(Result40,        16)
    REGDEF_BIT(Result41,        16)
REGDEF_END(SVM_RESULT_REGISTER20)


/*
    Result42:    [0x0, 0xffff],			bits : 15_0
    Result43:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER21_OFS 0x00b8
REGDEF_BEGIN(SVM_RESULT_REGISTER21)
    REGDEF_BIT(Result42,        16)
    REGDEF_BIT(Result43,        16)
REGDEF_END(SVM_RESULT_REGISTER21)


/*
    Result44:    [0x0, 0xffff],			bits : 15_0
    Result45:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER22_OFS 0x00bc
REGDEF_BEGIN(SVM_RESULT_REGISTER22)
    REGDEF_BIT(Result44,        16)
    REGDEF_BIT(Result45,        16)
REGDEF_END(SVM_RESULT_REGISTER22)


/*
    Result46:    [0x0, 0xffff],			bits : 15_0
    Result47:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER23_OFS 0x00c0
REGDEF_BEGIN(SVM_RESULT_REGISTER23)
    REGDEF_BIT(Result46,        16)
    REGDEF_BIT(Result47,        16)
REGDEF_END(SVM_RESULT_REGISTER23)


/*
    Result48:    [0x0, 0xffff],			bits : 15_0
    Result49:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER24_OFS 0x00c4
REGDEF_BEGIN(SVM_RESULT_REGISTER24)
    REGDEF_BIT(Result48,        16)
    REGDEF_BIT(Result49,        16)
REGDEF_END(SVM_RESULT_REGISTER24)


/*
    Result50:    [0x0, 0xffff],			bits : 15_0
    Result51:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER25_OFS 0x00c8
REGDEF_BEGIN(SVM_RESULT_REGISTER25)
    REGDEF_BIT(Result50,        16)
    REGDEF_BIT(Result51,        16)
REGDEF_END(SVM_RESULT_REGISTER25)


/*
    Result52:    [0x0, 0xffff],			bits : 15_0
    Result53:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER26_OFS 0x00cc
REGDEF_BEGIN(SVM_RESULT_REGISTER26)
    REGDEF_BIT(Result52,        16)
    REGDEF_BIT(Result53,        16)
REGDEF_END(SVM_RESULT_REGISTER26)


/*
    Result54:    [0x0, 0xffff],			bits : 15_0
    Result55:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER27_OFS 0x00d0
REGDEF_BEGIN(SVM_RESULT_REGISTER27)
    REGDEF_BIT(Result54,        16)
    REGDEF_BIT(Result55,        16)
REGDEF_END(SVM_RESULT_REGISTER27)


/*
    Result56:    [0x0, 0xffff],			bits : 15_0
    Result57:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER28_OFS 0x00d4
REGDEF_BEGIN(SVM_RESULT_REGISTER28)
    REGDEF_BIT(Result56,        16)
    REGDEF_BIT(Result57,        16)
REGDEF_END(SVM_RESULT_REGISTER28)


/*
    Result58:    [0x0, 0xffff],			bits : 15_0
    Result59:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER29_OFS 0x00d8
REGDEF_BEGIN(SVM_RESULT_REGISTER29)
    REGDEF_BIT(Result58,        16)
    REGDEF_BIT(Result59,        16)
REGDEF_END(SVM_RESULT_REGISTER29)


/*
    Result60:    [0x0, 0xffff],			bits : 15_0
    Result61:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER30_OFS 0x00dc
REGDEF_BEGIN(SVM_RESULT_REGISTER30)
    REGDEF_BIT(Result60,        16)
    REGDEF_BIT(Result61,        16)
REGDEF_END(SVM_RESULT_REGISTER30)


/*
    Result62:    [0x0, 0xffff],			bits : 15_0
    Result63:    [0x0, 0xffff],			bits : 31_16
*/
#define SVM_RESULT_REGISTER31_OFS 0x00e0
REGDEF_BEGIN(SVM_RESULT_REGISTER31)
    REGDEF_BIT(Result62,        16)
    REGDEF_BIT(Result63,        16)
REGDEF_END(SVM_RESULT_REGISTER31)


/*
    INDATA_BURST_MODE :    [0x0, 0x3],			bits : 1_0
    OUTDATA_BURST_MODE:    [0x0, 0x3],			bits : 3_2
    DEBUGPORTSEL      :    [0x0, 0x3],			bits : 5_4
    CYCLE_COUNT_EN    :    [0x0, 0x1],			bits : 24
    CHECKSUM_EN       :    [0x0, 0x1],			bits : 29
    PERMUTE_STRIPE_EN :    [0x0, 0x1],			bits : 30
*/
#define DEBUG_DESIGN_REGISTER_OFS 0x00e4
REGDEF_BEGIN(DEBUG_DESIGN_REGISTER)
    REGDEF_BIT(INDATA_BURST_MODE ,        2)
    REGDEF_BIT(OUTDATA_BURST_MODE,        2)
    REGDEF_BIT(DEBUGPORTSEL      ,        2)
    REGDEF_BIT(                  ,        18)
    REGDEF_BIT(CYCLE_COUNT_EN    ,        1)
    REGDEF_BIT(                  ,        4)
    REGDEF_BIT(CHECKSUM_EN       ,        1)
    REGDEF_BIT(PERMUTE_STRIPE_EN ,        1)
REGDEF_END(DEBUG_DESIGN_REGISTER)


/*
    AT_TABLE_UPDATE:    [0x0, 0x1],			bits : 0
    AT_W_SHIFT     :    [0x0, 0x7],			bits : 6_4
*/
#define ANCHOR_TRANSFORM_REGISTER0_OFS 0x00e8
REGDEF_BEGIN(ANCHOR_TRANSFORM_REGISTER0)
    REGDEF_BIT(AT_TABLE_UPDATE,        1)
    REGDEF_BIT(               ,        3)
    REGDEF_BIT(AT_W_SHIFT     ,        3)
REGDEF_END(ANCHOR_TRANSFORM_REGISTER0)


/*
    SF_IN_SHIFT_SIGNEDNESS :    [0x0, 0x1],			bits : 0
    SF_IN_SHIFT            :    [0x0, 0xf],			bits : 7_4
    SF_OUT_SHIFT_SIGNEDNESS:    [0x0, 0x1],			bits : 8
    SF_OUT_SHIFT           :    [0x0, 0xf],			bits : 15_12
    SF_OBJECT_NUM          :    [0x0, 0xff],			bits : 23_16
    SF_SET_NUM             :    [0x0, 0x7f],			bits : 30_24
*/
#define SOFTMAX_REGISTER0_OFS 0x00ec
REGDEF_BEGIN(SOFTMAX_REGISTER0)
    REGDEF_BIT(SF_IN_SHIFT_SIGNEDNESS ,        1)
    REGDEF_BIT(                       ,        3)
    REGDEF_BIT(SF_IN_SHIFT            ,        4)
    REGDEF_BIT(SF_OUT_SHIFT_SIGNEDNESS,        1)
    REGDEF_BIT(                       ,        3)
    REGDEF_BIT(SF_OUT_SHIFT           ,        4)
    REGDEF_BIT(SF_OBJECT_NUM          ,        8)
    REGDEF_BIT(SF_SET_NUM             ,        7)
REGDEF_END(SOFTMAX_REGISTER0)


/*
    IN_SHIFT_DIR:    [0x0, 0x1],			bits : 0
    IN_SHIFT    :    [0x0, 0x1f],			bits : 8_4
    IN_SCALE    :    [0x0, 0xffff],			bits : 27_12
*/
#define SCALE_SHIFT_REGISTER0_OFS 0x00f0
REGDEF_BEGIN(SCALE_SHIFT_REGISTER0)
    REGDEF_BIT(IN_SHIFT_DIR,        1)
    REGDEF_BIT(            ,        3)
    REGDEF_BIT(IN_SHIFT    ,        5)
    REGDEF_BIT(            ,        3)
    REGDEF_BIT(IN_SCALE    ,        16)
REGDEF_END(SCALE_SHIFT_REGISTER0)


/*
    FCD_ENC_BIT_LENGTH:    [0x0, 0xffffffff],			bits : 31_0
*/
#define COMPRESSION_REGISTER0_OFS 0x00f4
REGDEF_BEGIN(COMPRESSION_REGISTER0)
    REGDEF_BIT(FCD_ENC_BIT_LENGTH,        32)
REGDEF_END(COMPRESSION_REGISTER0)


/*
    FCD_VLC_CODE0 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID0:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER1_OFS 0x00f8
REGDEF_BEGIN(COMPRESSION_REGISTER1)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE0 ,        23)
    REGDEF_BIT(FCD_VLC_VALID0,        1)
REGDEF_END(COMPRESSION_REGISTER1)


/*
    FCD_VLC_CODE1 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID1:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER2_OFS 0x00fc
REGDEF_BEGIN(COMPRESSION_REGISTER2)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE1 ,        23)
    REGDEF_BIT(FCD_VLC_VALID1,        1)
REGDEF_END(COMPRESSION_REGISTER2)


/*
    FCD_VLC_CODE2 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID2:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER3_OFS 0x0100
REGDEF_BEGIN(COMPRESSION_REGISTER3)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE2 ,        23)
    REGDEF_BIT(FCD_VLC_VALID2,        1)
REGDEF_END(COMPRESSION_REGISTER3)


/*
    FCD_VLC_CODE3 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID3:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER4_OFS 0x0104
REGDEF_BEGIN(COMPRESSION_REGISTER4)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE3 ,        23)
    REGDEF_BIT(FCD_VLC_VALID3,        1)
REGDEF_END(COMPRESSION_REGISTER4)


/*
    FCD_VLC_CODE4 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID4:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER5_OFS 0x0108
REGDEF_BEGIN(COMPRESSION_REGISTER5)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE4 ,        23)
    REGDEF_BIT(FCD_VLC_VALID4,        1)
REGDEF_END(COMPRESSION_REGISTER5)


/*
    FCD_VLC_CODE5 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID5:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER6_OFS 0x010c
REGDEF_BEGIN(COMPRESSION_REGISTER6)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE5 ,        23)
    REGDEF_BIT(FCD_VLC_VALID5,        1)
REGDEF_END(COMPRESSION_REGISTER6)


/*
    FCD_VLC_CODE6 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID6:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER7_OFS 0x0110
REGDEF_BEGIN(COMPRESSION_REGISTER7)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE6 ,        23)
    REGDEF_BIT(FCD_VLC_VALID6,        1)
REGDEF_END(COMPRESSION_REGISTER7)


/*
    FCD_VLC_CODE7 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID7:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER8_OFS 0x0114
REGDEF_BEGIN(COMPRESSION_REGISTER8)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE7 ,        23)
    REGDEF_BIT(FCD_VLC_VALID7,        1)
REGDEF_END(COMPRESSION_REGISTER8)


/*
    FCD_VLC_CODE8 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID8:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER9_OFS 0x0118
REGDEF_BEGIN(COMPRESSION_REGISTER9)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE8 ,        23)
    REGDEF_BIT(FCD_VLC_VALID8,        1)
REGDEF_END(COMPRESSION_REGISTER9)


/*
    FCD_VLC_CODE9 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID9:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER10_OFS 0x011c
REGDEF_BEGIN(COMPRESSION_REGISTER10)
    REGDEF_BIT(              ,        8)
    REGDEF_BIT(FCD_VLC_CODE9 ,        23)
    REGDEF_BIT(FCD_VLC_VALID9,        1)
REGDEF_END(COMPRESSION_REGISTER10)


/*
    FCD_VLC_CODE10 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID10:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER11_OFS 0x0120
REGDEF_BEGIN(COMPRESSION_REGISTER11)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE10 ,        23)
    REGDEF_BIT(FCD_VLC_VALID10,        1)
REGDEF_END(COMPRESSION_REGISTER11)


/*
    FCD_VLC_CODE11 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID11:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER12_OFS 0x0124
REGDEF_BEGIN(COMPRESSION_REGISTER12)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE11 ,        23)
    REGDEF_BIT(FCD_VLC_VALID11,        1)
REGDEF_END(COMPRESSION_REGISTER12)


/*
    FCD_VLC_CODE12 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID12:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER13_OFS 0x0128
REGDEF_BEGIN(COMPRESSION_REGISTER13)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE12 ,        23)
    REGDEF_BIT(FCD_VLC_VALID12,        1)
REGDEF_END(COMPRESSION_REGISTER13)


/*
    FCD_VLC_CODE13 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID13:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER14_OFS 0x012c
REGDEF_BEGIN(COMPRESSION_REGISTER14)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE13 ,        23)
    REGDEF_BIT(FCD_VLC_VALID13,        1)
REGDEF_END(COMPRESSION_REGISTER14)


/*
    FCD_VLC_CODE14 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID14:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER15_OFS 0x0130
REGDEF_BEGIN(COMPRESSION_REGISTER15)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE14 ,        23)
    REGDEF_BIT(FCD_VLC_VALID14,        1)
REGDEF_END(COMPRESSION_REGISTER15)


/*
    FCD_VLC_CODE15 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID15:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER16_OFS 0x0134
REGDEF_BEGIN(COMPRESSION_REGISTER16)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE15 ,        23)
    REGDEF_BIT(FCD_VLC_VALID15,        1)
REGDEF_END(COMPRESSION_REGISTER16)


/*
    FCD_VLC_CODE16 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID16:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER17_OFS 0x0138
REGDEF_BEGIN(COMPRESSION_REGISTER17)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE16 ,        23)
    REGDEF_BIT(FCD_VLC_VALID16,        1)
REGDEF_END(COMPRESSION_REGISTER17)


/*
    FCD_VLC_CODE17 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID17:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER18_OFS 0x013c
REGDEF_BEGIN(COMPRESSION_REGISTER18)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE17 ,        23)
    REGDEF_BIT(FCD_VLC_VALID17,        1)
REGDEF_END(COMPRESSION_REGISTER18)


/*
    FCD_VLC_CODE18 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID18:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER19_OFS 0x0140
REGDEF_BEGIN(COMPRESSION_REGISTER19)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE18 ,        23)
    REGDEF_BIT(FCD_VLC_VALID18,        1)
REGDEF_END(COMPRESSION_REGISTER19)


/*
    FCD_VLC_CODE19 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID19:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER20_OFS 0x0144
REGDEF_BEGIN(COMPRESSION_REGISTER20)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE19 ,        23)
    REGDEF_BIT(FCD_VLC_VALID19,        1)
REGDEF_END(COMPRESSION_REGISTER20)


/*
    FCD_VLC_CODE20 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID20:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER21_OFS 0x0148
REGDEF_BEGIN(COMPRESSION_REGISTER21)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE20 ,        23)
    REGDEF_BIT(FCD_VLC_VALID20,        1)
REGDEF_END(COMPRESSION_REGISTER21)


/*
    FCD_VLC_CODE21 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID21:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER22_OFS 0x014c
REGDEF_BEGIN(COMPRESSION_REGISTER22)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE21 ,        23)
    REGDEF_BIT(FCD_VLC_VALID21,        1)
REGDEF_END(COMPRESSION_REGISTER22)


/*
    FCD_VLC_CODE22 :    [0x0, 0x7fffff],			bits : 30_8
    FCD_VLC_VALID22:    [0x0, 0x1],			bits : 31
*/
#define COMPRESSION_REGISTER23_OFS 0x0150
REGDEF_BEGIN(COMPRESSION_REGISTER23)
    REGDEF_BIT(               ,        8)
    REGDEF_BIT(FCD_VLC_CODE22 ,        23)
    REGDEF_BIT(FCD_VLC_VALID22,        1)
REGDEF_END(COMPRESSION_REGISTER23)


/*
    FCD_VLC_OFFSET0:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET1:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER24_OFS 0x0154
REGDEF_BEGIN(COMPRESSION_REGISTER24)
    REGDEF_BIT(FCD_VLC_OFFSET0,        16)
    REGDEF_BIT(FCD_VLC_OFFSET1,        16)
REGDEF_END(COMPRESSION_REGISTER24)


/*
    FCD_VLC_OFFSET2:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET3:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER25_OFS 0x0158
REGDEF_BEGIN(COMPRESSION_REGISTER25)
    REGDEF_BIT(FCD_VLC_OFFSET2,        16)
    REGDEF_BIT(FCD_VLC_OFFSET3,        16)
REGDEF_END(COMPRESSION_REGISTER25)


/*
    FCD_VLC_OFFSET4:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET5:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER26_OFS 0x015c
REGDEF_BEGIN(COMPRESSION_REGISTER26)
    REGDEF_BIT(FCD_VLC_OFFSET4,        16)
    REGDEF_BIT(FCD_VLC_OFFSET5,        16)
REGDEF_END(COMPRESSION_REGISTER26)


/*
    FCD_VLC_OFFSET6:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET7:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER27_OFS 0x0160
REGDEF_BEGIN(COMPRESSION_REGISTER27)
    REGDEF_BIT(FCD_VLC_OFFSET6,        16)
    REGDEF_BIT(FCD_VLC_OFFSET7,        16)
REGDEF_END(COMPRESSION_REGISTER27)


/*
    FCD_VLC_OFFSET8:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET9:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER28_OFS 0x0164
REGDEF_BEGIN(COMPRESSION_REGISTER28)
    REGDEF_BIT(FCD_VLC_OFFSET8,        16)
    REGDEF_BIT(FCD_VLC_OFFSET9,        16)
REGDEF_END(COMPRESSION_REGISTER28)


/*
    FCD_VLC_OFFSET10:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET11:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER29_OFS 0x0168
REGDEF_BEGIN(COMPRESSION_REGISTER29)
    REGDEF_BIT(FCD_VLC_OFFSET10,        16)
    REGDEF_BIT(FCD_VLC_OFFSET11,        16)
REGDEF_END(COMPRESSION_REGISTER29)


/*
    FCD_VLC_OFFSET12:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET13:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER30_OFS 0x016c
REGDEF_BEGIN(COMPRESSION_REGISTER30)
    REGDEF_BIT(FCD_VLC_OFFSET12,        16)
    REGDEF_BIT(FCD_VLC_OFFSET13,        16)
REGDEF_END(COMPRESSION_REGISTER30)


/*
    FCD_VLC_OFFSET14:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET15:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER31_OFS 0x0170
REGDEF_BEGIN(COMPRESSION_REGISTER31)
    REGDEF_BIT(FCD_VLC_OFFSET14,        16)
    REGDEF_BIT(FCD_VLC_OFFSET15,        16)
REGDEF_END(COMPRESSION_REGISTER31)


/*
    FCD_VLC_OFFSET16:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET17:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER32_OFS 0x0174
REGDEF_BEGIN(COMPRESSION_REGISTER32)
    REGDEF_BIT(FCD_VLC_OFFSET16,        16)
    REGDEF_BIT(FCD_VLC_OFFSET17,        16)
REGDEF_END(COMPRESSION_REGISTER32)


/*
    FCD_VLC_OFFSET18:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET19:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER33_OFS 0x0178
REGDEF_BEGIN(COMPRESSION_REGISTER33)
    REGDEF_BIT(FCD_VLC_OFFSET18,        16)
    REGDEF_BIT(FCD_VLC_OFFSET19,        16)
REGDEF_END(COMPRESSION_REGISTER33)


/*
    FCD_VLC_OFFSET20:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET21:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER34_OFS 0x017c
REGDEF_BEGIN(COMPRESSION_REGISTER34)
    REGDEF_BIT(FCD_VLC_OFFSET20,        16)
    REGDEF_BIT(FCD_VLC_OFFSET21,        16)
REGDEF_END(COMPRESSION_REGISTER34)


/*
    FCD_VLC_OFFSET22:    [0x0, 0xffff],			bits : 15_0
    FCD_VLC_OFFSET23:    [0x0, 0xffff],			bits : 31_16
*/
#define COMPRESSION_REGISTER35_OFS 0x0180
REGDEF_BEGIN(COMPRESSION_REGISTER35)
    REGDEF_BIT(FCD_VLC_OFFSET22,        16)
    REGDEF_BIT(FCD_VLC_OFFSET23,        16)
REGDEF_END(COMPRESSION_REGISTER35)


/*
    LL_TABLE_INDEX0:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_INDEX1:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_INDEX2:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_INDEX3:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER0_OFS 0x0184
REGDEF_BEGIN(LL_FRAME_REGISTER0)
    REGDEF_BIT(LL_TABLE_INDEX0,        8)
    REGDEF_BIT(LL_TABLE_INDEX1,        8)
    REGDEF_BIT(LL_TABLE_INDEX2,        8)
    REGDEF_BIT(LL_TABLE_INDEX3,        8)
REGDEF_END(LL_FRAME_REGISTER0)


/*
    LL_TABLE_INDEX4:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_INDEX5:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_INDEX6:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_INDEX7:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER1_OFS 0x0188
REGDEF_BEGIN(LL_FRAME_REGISTER1)
    REGDEF_BIT(LL_TABLE_INDEX4,        8)
    REGDEF_BIT(LL_TABLE_INDEX5,        8)
    REGDEF_BIT(LL_TABLE_INDEX6,        8)
    REGDEF_BIT(LL_TABLE_INDEX7,        8)
REGDEF_END(LL_FRAME_REGISTER1)


/*
    LL_TABLE_INDEX8 :    [0x0, 0xff],			bits : 7_0
    LL_TABLE_INDEX9 :    [0x0, 0xff],			bits : 15_8
    LL_TABLE_INDEX10:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_INDEX11:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER2_OFS 0x018c
REGDEF_BEGIN(LL_FRAME_REGISTER2)
    REGDEF_BIT(LL_TABLE_INDEX8 ,        8)
    REGDEF_BIT(LL_TABLE_INDEX9 ,        8)
    REGDEF_BIT(LL_TABLE_INDEX10,        8)
    REGDEF_BIT(LL_TABLE_INDEX11,        8)
REGDEF_END(LL_FRAME_REGISTER2)


/*
    LL_TABLE_INDEX12:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_INDEX13:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_INDEX14:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_INDEX15:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER3_OFS 0x0190
REGDEF_BEGIN(LL_FRAME_REGISTER3)
    REGDEF_BIT(LL_TABLE_INDEX12,        8)
    REGDEF_BIT(LL_TABLE_INDEX13,        8)
    REGDEF_BIT(LL_TABLE_INDEX14,        8)
    REGDEF_BIT(LL_TABLE_INDEX15,        8)
REGDEF_END(LL_FRAME_REGISTER3)


#define NUE_RESERVED_OFS_2 0x0194
REGDEF_BEGIN(NUE_RESERVED_2)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_2) //0x0194


#define NUE_RESERVED_OFS_3 0x0198
REGDEF_BEGIN(NUE_RESERVED_3)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_3) //0x0198


#define NUE_RESERVED_OFS_4 0x019C
REGDEF_BEGIN(NUE_RESERVED_4)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_4) //0x019C


#define NUE_RESERVED_OFS_5 0x01A0
REGDEF_BEGIN(NUE_RESERVED_5)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_5) //0x01A0


#define NUE_RESERVED_OFS_6 0x01A4
REGDEF_BEGIN(NUE_RESERVED_6)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_6) //0x01A4


#define NUE_RESERVED_OFS_7 0x01A8
REGDEF_BEGIN(NUE_RESERVED_7)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_7) //0x01A8


/*
    DMA_DISABLE:    [0x0, 0x1],			bits : 0
    NUE_IDLE   :    [0x0, 0x1],			bits : 15
*/
#define DMA_DISABLE_REGISTER0_OFS 0x01ac
REGDEF_BEGIN(DMA_DISABLE_REGISTER0)
    REGDEF_BIT(DMA_DISABLE,        1)
    REGDEF_BIT(           ,        14)
    REGDEF_BIT(NUE_IDLE   ,        1)
REGDEF_END(DMA_DISABLE_REGISTER0)


#define NUE_RESERVED_OFS_8 0x01B0
REGDEF_BEGIN(NUE_RESERVED_8)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_8) //0x01B0


#define NUE_RESERVED_OFS_9 0x01B4
REGDEF_BEGIN(NUE_RESERVED_9)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_9) //0x01B4


#define NUE_RESERVED_OFS_10 0x01B8
REGDEF_BEGIN(NUE_RESERVED_10)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_10) //0x01B8


#define NUE_RESERVED_OFS_11 0x01BC
REGDEF_BEGIN(NUE_RESERVED_11)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_11) //0x01BC


#define NUE_RESERVED_OFS_12 0x01C0
REGDEF_BEGIN(NUE_RESERVED_12)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_12) //0x01C0


/*
    PERMUTE_SHIFT_SIGNEDNESS:    [0x0, 0x1],			bits : 0
    PERMUTE_SHIFT           :    [0x0, 0x7],			bits : 6_4
*/
#define PERMUTE_REGISTER0_OFS 0x01c4
REGDEF_BEGIN(PERMUTE_REGISTER0)
    REGDEF_BIT(PERMUTE_SHIFT_SIGNEDNESS,        1)
    REGDEF_BIT(                        ,        3)
    REGDEF_BIT(PERMUTE_SHIFT           ,        3)
REGDEF_END(PERMUTE_REGISTER0)


/*
    LL_TERMINATE:    [0x0, 0x1],			bits : 0
*/
#define LL_TERMINATE_RESISTER0_OFS 0x01c8
REGDEF_BEGIN(LL_TERMINATE_RESISTER0)
    REGDEF_BIT(LL_TERMINATE,        1)
REGDEF_END(LL_TERMINATE_RESISTER0)


/*
    LL_BASE_ADDR:    [0x0, 0xffffffff],			bits : 31_0
*/
#define LL_BASE_ADDRESS_RESISTER0_OFS 0x01cc
REGDEF_BEGIN(LL_BASE_ADDRESS_RESISTER0)
    REGDEF_BIT(LL_BASE_ADDR,        32)
REGDEF_END(LL_BASE_ADDRESS_RESISTER0)


#define NUE_RESERVED_OFS_13 0x01D0
REGDEF_BEGIN(NUE_RESERVED_13)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_13) //0x01D0


#define NUE_RESERVED_OFS_14 0x01D4
REGDEF_BEGIN(NUE_RESERVED_14)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_14) //0x01D4


#define NUE_RESERVED_OFS_15 0x01D8
REGDEF_BEGIN(NUE_RESERVED_15)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_15) //0x01D8


#define NUE_RESERVED_OFS_16 0x01DC
REGDEF_BEGIN(NUE_RESERVED_16)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_16) //0x01DC


#define NUE_RESERVED_OFS_17 0x01E0
REGDEF_BEGIN(NUE_RESERVED_17)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_17) //0x01E0


#define NUE_RESERVED_OFS_18 0x01E4
REGDEF_BEGIN(NUE_RESERVED_18)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_18) //0x01E4


#define NUE_RESERVED_OFS_19 0x01E8
REGDEF_BEGIN(NUE_RESERVED_19)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_19) //0x01E8


/*
    NUE_CODE_VERSION:    [0x0, 0xffffffff],			bits : 31_0
*/
#define VERSION_RESISTER0_OFS 0x01ec
REGDEF_BEGIN(VERSION_RESISTER0)
    REGDEF_BIT(NUE_CODE_VERSION,        32)
REGDEF_END(VERSION_RESISTER0)


#define NUE_RESERVED_OFS_20 0x01F0
REGDEF_BEGIN(NUE_RESERVED_20)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_20) //0x01F0


#define NUE_RESERVED_OFS_21 0x01F4
REGDEF_BEGIN(NUE_RESERVED_21)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_21) //0x01F4


#define NUE_RESERVED_OFS_22 0x01F8
REGDEF_BEGIN(NUE_RESERVED_22)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_22) //0x01F8


/*
    NUE_ENG_CYCLE:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CYCLE_COUNT_RESISTER0_OFS 0x01fc
REGDEF_BEGIN(CYCLE_COUNT_RESISTER0)
    REGDEF_BIT(NUE_ENG_CYCLE,        32)
REGDEF_END(CYCLE_COUNT_RESISTER0)


/*
    NUE_LL_CYCLE:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CYCLE_COUNT_RESISTER1_OFS 0x0200
REGDEF_BEGIN(CYCLE_COUNT_RESISTER1)
    REGDEF_BIT(NUE_LL_CYCLE,        32)
REGDEF_END(CYCLE_COUNT_RESISTER1)


/*
    NUE_WAIT_DMA_CYCLE:    [0x0, 0xffffffff],			bits : 31_0
*/
#define CYCLE_COUNT_RESISTER2_OFS 0x0204
REGDEF_BEGIN(CYCLE_COUNT_RESISTER2)
    REGDEF_BIT(NUE_WAIT_DMA_CYCLE,        32)
REGDEF_END(CYCLE_COUNT_RESISTER2)


/*
    NUE_LLC_ERR_CMD_CNT:    [0x0, 0xffffffff],			bits : 31_0
*/
#define LLC_DEBUG_RESISTER0_OFS 0x0208
REGDEF_BEGIN(LLC_DEBUG_RESISTER0)
    REGDEF_BIT(NUE_LLC_ERR_CMD_CNT,        32)
REGDEF_END(LLC_DEBUG_RESISTER0)


/*
    NUE_LLC_ERR_CMD_ADDR:    [0x0, 0xffffffff],			bits : 31_0
*/
#define LLC_DEBUG_RESISTER1_OFS 0x020c
REGDEF_BEGIN(LLC_DEBUG_RESISTER1)
    REGDEF_BIT(NUE_LLC_ERR_CMD_ADDR,        32)
REGDEF_END(LLC_DEBUG_RESISTER1)


#define NUE_RESERVED_OFS_23 0x0210
REGDEF_BEGIN(NUE_RESERVED_23)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_23) //0x0210


#define NUE_RESERVED_OFS_24 0x0214
REGDEF_BEGIN(NUE_RESERVED_24)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_24) //0x0214


#define NUE_RESERVED_OFS_25 0x0218
REGDEF_BEGIN(NUE_RESERVED_25)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_25) //0x0218


/*
    NUE_OUTPUT_CHECKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define OUTPUT_CHECKSUM_REGISTER0_OFS 0x021c
REGDEF_BEGIN(OUTPUT_CHECKSUM_REGISTER0)
    REGDEF_BIT(NUE_OUTPUT_CHECKSUM,        32)
REGDEF_END(OUTPUT_CHECKSUM_REGISTER0)


#define NUE_RESERVED_OFS_26 0x0220
REGDEF_BEGIN(NUE_RESERVED_26)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_26) //0x0220


#define NUE_RESERVED_OFS_27 0x0224
REGDEF_BEGIN(NUE_RESERVED_27)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_27) //0x0224


#define NUE_RESERVED_OFS_28 0x0228
REGDEF_BEGIN(NUE_RESERVED_28)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_28) //0x0228


/*
    NUE_COMPARE_CHECKSUM:    [0x0, 0xffffffff],			bits : 31_0
*/
#define COMPARE_CHECKSUM_REGISTER0_OFS 0x022c
REGDEF_BEGIN(COMPARE_CHECKSUM_REGISTER0)
    REGDEF_BIT(NUE_COMPARE_CHECKSUM,        32)
REGDEF_END(COMPARE_CHECKSUM_REGISTER0)


#define NUE_RESERVED_OFS_29 0x0230
REGDEF_BEGIN(NUE_RESERVED_29)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_29) //0x0230


#define NUE_RESERVED_OFS_30 0x0234
REGDEF_BEGIN(NUE_RESERVED_30)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_30) //0x0234


#define NUE_RESERVED_OFS_31 0x0238
REGDEF_BEGIN(NUE_RESERVED_31)
    REGDEF_BIT(RESERVED,        32)
REGDEF_END(NUE_RESERVED_31) //0x0238


/*
    NUE_CHKSUM_MISMATCH_IDX_CNT:    [0x0, 0xffff],			bits : 15_0
*/
#define CHECKSUM_MISMATCH_COUNTER_REGISTER0_OFS 0x023c
REGDEF_BEGIN(CHECKSUM_MISMATCH_COUNTER_REGISTER0)
    REGDEF_BIT(NUE_CHKSUM_MISMATCH_IDX_CNT,        16)
REGDEF_END(CHECKSUM_MISMATCH_COUNTER_REGISTER0)


typedef struct
{
    //0x0000
    T_NUE_CONTROL_REGISTER 
    NUE_Register_0000;

    //0x0004
    T_NUE_MODE_REGISTER0 
    NUE_Register_0004;

    //0x0008
    T_NUE_MODE_REGISTER1 
    NUE_Register_0008;

    //0x000c
    T_DMA_TO_NUE_REGISTER0 
    NUE_Register_000c;

    //0x0010
    T_DMA_TO_NUE_REGISTER1 
    NUE_Register_0010;

    //0x0014
    T_DMA_TO_NUE_REGISTER2 
    NUE_Register_0014;

    //0x0018
    T_DMA_TO_NUE_REGISTER3 
    NUE_Register_0018;

    //0x001c
    T_DMA_TO_NUE_REGISTER4 
    NUE_Register_001c;

    //0x0020
    T_NUE_RESERVED_0 
    NUE_Register_0020;

    //0x0024
    T_DMA_TO_NUE_REGISTER6 
    NUE_Register_0024;

    //0x0028
    T_DMA_TO_NUE_REGISTER7 
    NUE_Register_0028;

    //0x002c
    T_NUE_TO_DMA_RESULT_REGISTER0 
    NUE_Register_002c;

    //0x0030
    T_INPUT_FEATURE_LINE_OFFSET_REGISTER 
    NUE_Register_0030;

    //0x0034
    T_OUTPUT_FEATURE_LINE_OFFSET_REGISTER 
    NUE_Register_0034;

    //0x0038
    T_NUE_INTERRUPT_ENABLE_REGISTER 
    NUE_Register_0038;

    //0x003c
    T_NUE_INTERRUPT_STATUS_REGISTER 
    NUE_Register_003c;

    //0x0040
    T_INPUT_SIZE_REGISTER0 
    NUE_Register_0040;

    //0x0044
    T_INPUT_SIZE_REGISTER1 
    NUE_Register_0044;

    //0x0048
    T_INPUT_SIZE_REGISTER2 
    NUE_Register_0048;

    //0x004c
    T_KERNEL_REGISTER0 
    NUE_Register_004c;

    //0x0050
    T_KERNEL_REGISTER1 
    NUE_Register_0050;

    //0x0054
    T_RELU_REGISTER0 
    NUE_Register_0054;

    //0x0058
    T_RESULT_REGISTER0 
    NUE_Register_0058;

    //0x005c
    T_NUE_RESERVED_1 
    NUE_Register_005c;

    //0x0060
    T_REORGANIZE_REGISTER0 
    NUE_Register_0060;

    //0x0064
    T_SVM_RESULT_REGISTER0 
    NUE_Register_0064;

    //0x0068
    T_SVM_RESULT_REGISTER1 
    NUE_Register_0068;

    //0x006c
    T_SVM_RESULT_REGISTER2 
    NUE_Register_006c;

    //0x0070
    T_SVM_RESULT_REGISTER3 
    NUE_Register_0070;

    //0x0074
    T_SVM_RESULT_REGISTER4 
    NUE_Register_0074;

    //0x0078
    T_SVM_RESULT_REGISTER5 
    NUE_Register_0078;

    //0x007c
    T_SVM_RESULT_REGISTER6 
    NUE_Register_007c;

    //0x0080
    T_SVM_RESULT_REGISTER7 
    NUE_Register_0080;

    //0x0084
    T_SVM_RESULT_REGISTER8 
    NUE_Register_0084;

    //0x0088
    T_SVM_RESULT_REGISTER9 
    NUE_Register_0088;

    //0x008c
    T_SVM_RESULT_REGISTER10 
    NUE_Register_008c;

    //0x0090
    T_SVM_RESULT_REGISTER11 
    NUE_Register_0090;

    //0x0094
    T_SVM_RESULT_REGISTER12 
    NUE_Register_0094;

    //0x0098
    T_SVM_RESULT_REGISTER13 
    NUE_Register_0098;

    //0x009c
    T_SVM_RESULT_REGISTER14 
    NUE_Register_009c;

    //0x00a0
    T_SVM_RESULT_REGISTER15 
    NUE_Register_00a0;

    //0x00a4
    T_SVM_RESULT_REGISTER16 
    NUE_Register_00a4;

    //0x00a8
    T_SVM_RESULT_REGISTER17 
    NUE_Register_00a8;

    //0x00ac
    T_SVM_RESULT_REGISTER18 
    NUE_Register_00ac;

    //0x00b0
    T_SVM_RESULT_REGISTER19 
    NUE_Register_00b0;

    //0x00b4
    T_SVM_RESULT_REGISTER20 
    NUE_Register_00b4;

    //0x00b8
    T_SVM_RESULT_REGISTER21 
    NUE_Register_00b8;

    //0x00bc
    T_SVM_RESULT_REGISTER22 
    NUE_Register_00bc;

    //0x00c0
    T_SVM_RESULT_REGISTER23 
    NUE_Register_00c0;

    //0x00c4
    T_SVM_RESULT_REGISTER24 
    NUE_Register_00c4;

    //0x00c8
    T_SVM_RESULT_REGISTER25 
    NUE_Register_00c8;

    //0x00cc
    T_SVM_RESULT_REGISTER26 
    NUE_Register_00cc;

    //0x00d0
    T_SVM_RESULT_REGISTER27 
    NUE_Register_00d0;

    //0x00d4
    T_SVM_RESULT_REGISTER28 
    NUE_Register_00d4;

    //0x00d8
    T_SVM_RESULT_REGISTER29 
    NUE_Register_00d8;

    //0x00dc
    T_SVM_RESULT_REGISTER30 
    NUE_Register_00dc;

    //0x00e0
    T_SVM_RESULT_REGISTER31 
    NUE_Register_00e0;

    //0x00e4
    T_DEBUG_DESIGN_REGISTER 
    NUE_Register_00e4;

    //0x00e8
    T_ANCHOR_TRANSFORM_REGISTER0 
    NUE_Register_00e8;

    //0x00ec
    T_SOFTMAX_REGISTER0 
    NUE_Register_00ec;

    //0x00f0
    T_SCALE_SHIFT_REGISTER0 
    NUE_Register_00f0;

    //0x00f4
    T_COMPRESSION_REGISTER0 
    NUE_Register_00f4;

    //0x00f8
    T_COMPRESSION_REGISTER1 
    NUE_Register_00f8;

    //0x00fc
    T_COMPRESSION_REGISTER2 
    NUE_Register_00fc;

    //0x0100
    T_COMPRESSION_REGISTER3 
    NUE_Register_0100;

    //0x0104
    T_COMPRESSION_REGISTER4 
    NUE_Register_0104;

    //0x0108
    T_COMPRESSION_REGISTER5 
    NUE_Register_0108;

    //0x010c
    T_COMPRESSION_REGISTER6 
    NUE_Register_010c;

    //0x0110
    T_COMPRESSION_REGISTER7 
    NUE_Register_0110;

    //0x0114
    T_COMPRESSION_REGISTER8 
    NUE_Register_0114;

    //0x0118
    T_COMPRESSION_REGISTER9 
    NUE_Register_0118;

    //0x011c
    T_COMPRESSION_REGISTER10 
    NUE_Register_011c;

    //0x0120
    T_COMPRESSION_REGISTER11 
    NUE_Register_0120;

    //0x0124
    T_COMPRESSION_REGISTER12 
    NUE_Register_0124;

    //0x0128
    T_COMPRESSION_REGISTER13 
    NUE_Register_0128;

    //0x012c
    T_COMPRESSION_REGISTER14 
    NUE_Register_012c;

    //0x0130
    T_COMPRESSION_REGISTER15 
    NUE_Register_0130;

    //0x0134
    T_COMPRESSION_REGISTER16 
    NUE_Register_0134;

    //0x0138
    T_COMPRESSION_REGISTER17 
    NUE_Register_0138;

    //0x013c
    T_COMPRESSION_REGISTER18 
    NUE_Register_013c;

    //0x0140
    T_COMPRESSION_REGISTER19 
    NUE_Register_0140;

    //0x0144
    T_COMPRESSION_REGISTER20 
    NUE_Register_0144;

    //0x0148
    T_COMPRESSION_REGISTER21 
    NUE_Register_0148;

    //0x014c
    T_COMPRESSION_REGISTER22 
    NUE_Register_014c;

    //0x0150
    T_COMPRESSION_REGISTER23 
    NUE_Register_0150;

    //0x0154
    T_COMPRESSION_REGISTER24 
    NUE_Register_0154;

    //0x0158
    T_COMPRESSION_REGISTER25 
    NUE_Register_0158;

    //0x015c
    T_COMPRESSION_REGISTER26 
    NUE_Register_015c;

    //0x0160
    T_COMPRESSION_REGISTER27 
    NUE_Register_0160;

    //0x0164
    T_COMPRESSION_REGISTER28 
    NUE_Register_0164;

    //0x0168
    T_COMPRESSION_REGISTER29 
    NUE_Register_0168;

    //0x016c
    T_COMPRESSION_REGISTER30 
    NUE_Register_016c;

    //0x0170
    T_COMPRESSION_REGISTER31 
    NUE_Register_0170;

    //0x0174
    T_COMPRESSION_REGISTER32 
    NUE_Register_0174;

    //0x0178
    T_COMPRESSION_REGISTER33 
    NUE_Register_0178;

    //0x017c
    T_COMPRESSION_REGISTER34 
    NUE_Register_017c;

    //0x0180
    T_COMPRESSION_REGISTER35 
    NUE_Register_0180;

    //0x0184
    T_LL_FRAME_REGISTER0 
    NUE_Register_0184;

    //0x0188
    T_LL_FRAME_REGISTER1 
    NUE_Register_0188;

    //0x018c
    T_LL_FRAME_REGISTER2 
    NUE_Register_018c;

    //0x0190
    T_LL_FRAME_REGISTER3 
    NUE_Register_0190;

    //0x0194
    T_NUE_RESERVED_2 
    NUE_Register_0194;

    //0x0198
    T_NUE_RESERVED_3 
    NUE_Register_0198;

    //0x019c
    T_NUE_RESERVED_4 
    NUE_Register_019c;

    //0x01a0
    T_NUE_RESERVED_5 
    NUE_Register_01a0;

    //0x01a4
    T_NUE_RESERVED_6 
    NUE_Register_01a4;

    //0x01a8
    T_NUE_RESERVED_7 
    NUE_Register_01a8;

    //0x01ac
    T_DMA_DISABLE_REGISTER0 
    NUE_Register_01ac;

    //0x01b0
    T_NUE_RESERVED_8 
    NUE_Register_01b0;

    //0x01b4
    T_NUE_RESERVED_9 
    NUE_Register_01b4;

    //0x01b8
    T_NUE_RESERVED_10 
    NUE_Register_01b8;

    //0x01bc
    T_NUE_RESERVED_11 
    NUE_Register_01bc;

    //0x01c0
    T_NUE_RESERVED_12 
    NUE_Register_01c0;

    //0x01c4
    T_PERMUTE_REGISTER0 
    NUE_Register_01c4;

    //0x01c8
    T_LL_TERMINATE_RESISTER0 
    NUE_Register_01c8;

    //0x01cc
    T_LL_BASE_ADDRESS_RESISTER0 
    NUE_Register_01cc;

    //0x01d0
    T_NUE_RESERVED_13 
    NUE_Register_01d0;

    //0x01d4
    T_NUE_RESERVED_14 
    NUE_Register_01d4;

    //0x01d8
    T_NUE_RESERVED_15 
    NUE_Register_01d8;

    //0x01dc
    T_NUE_RESERVED_16 
    NUE_Register_01dc;

    //0x01e0
    T_NUE_RESERVED_17 
    NUE_Register_01e0;

    //0x01e4
    T_NUE_RESERVED_18 
    NUE_Register_01e4;

    //0x01e8
    T_NUE_RESERVED_19 
    NUE_Register_01e8;

    //0x01ec
    T_VERSION_RESISTER0 
    NUE_Register_01ec;

    //0x01f0
    T_NUE_RESERVED_20 
    NUE_Register_01f0;

    //0x01f4
    T_NUE_RESERVED_21 
    NUE_Register_01f4;

    //0x01f8
    T_NUE_RESERVED_22 
    NUE_Register_01f8;

    //0x01fc
    T_CYCLE_COUNT_RESISTER0 
    NUE_Register_01fc;

    //0x0200
    T_CYCLE_COUNT_RESISTER1 
    NUE_Register_0200;

    //0x0204
    T_CYCLE_COUNT_RESISTER2 
    NUE_Register_0204;

    //0x0208
    T_LLC_DEBUG_RESISTER0 
    NUE_Register_0208;

    //0x020c
    T_LLC_DEBUG_RESISTER1 
    NUE_Register_020c;

    //0x0210
    T_NUE_RESERVED_23 
    NUE_Register_0210;

    //0x0214
    T_NUE_RESERVED_24 
    NUE_Register_0214;

    //0x0218
    T_NUE_RESERVED_25 
    NUE_Register_0218;

    //0x021c
    T_OUTPUT_CHECKSUM_REGISTER0 
    NUE_Register_021c;

    //0x0220
    T_NUE_RESERVED_26 
    NUE_Register_0220;

    //0x0224
    T_NUE_RESERVED_27 
    NUE_Register_0224;

    //0x0228
    T_NUE_RESERVED_28 
    NUE_Register_0228;

    //0x022c
    T_COMPARE_CHECKSUM_REGISTER0 
    NUE_Register_022c;

    //0x0230
    T_NUE_RESERVED_29 
    NUE_Register_0230;

    //0x0234
    T_NUE_RESERVED_30 
    NUE_Register_0234;

    //0x0238
    T_NUE_RESERVED_31 
    NUE_Register_0238;

    //0x023c
    T_CHECKSUM_MISMATCH_COUNTER_REGISTER0 
    NUE_Register_023c;

} NT98560_NUE_REG_STRUCT;


#ifdef __cplusplus
}
#endif

#endif
