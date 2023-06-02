#ifndef _ISE_ENG_INT_REG_H_
#define _ISE_ENG_INT_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcw_macro.h"

typedef struct
{

  union
  {
    struct
    {
      unsigned ise_rst                 : 1;		// bits : 0
      unsigned ise_start               : 1;		// bits : 1
      unsigned                         : 26;
      unsigned ise_ll_fire             : 1;		// bits : 28
      unsigned ise_ll_terminate        : 1;		// bits : 29
    } bit;
    UINT32 word;
  } ise_register_0; // 0x0000

  union
  {
    struct
    {
      unsigned ise_io_fmt               : 4;		// bits : 3_0
      unsigned                          : 4;		// bits : 7_4
      unsigned ise_scl_method           : 2;		// bits : 9_8
      unsigned                          : 2;
      unsigned ise_argb_outmode         : 2;		// bits : 13_12
      unsigned                          : 2;
      unsigned ise_in_brt_lenth         : 1;		// bits : 16
      unsigned ise_out_brt_lenth        : 1;		// bits : 17
      unsigned ise_ovlap_mode           : 1;		// bits : 18
    } bit;
    UINT32 word;
  } ise_register_1; // 0x0004

  union
  {
    struct
    {
      unsigned ise_inte_ll_end         : 1;		// bits : 0
      unsigned ise_inte_ll_err         : 1;		// bits : 1
      unsigned                         : 29;
      unsigned ise_inte_frm_end        : 1;		// bits : 31
    } bit;
    UINT32 word;
  } ise_register_2; // 0x0008

  union
  {
    struct
    {
      unsigned ise_ints_ll_end         : 1;		// bits : 0
      unsigned ise_ints_ll_err         : 1;		// bits : 1
      unsigned                         : 29;
      unsigned ise_ints_frm_end        : 1;		// bits : 31
    } bit;
    UINT32 word;
  } ise_register_3; // 0x000c

  union
  {
    struct
    {
      unsigned ise_h_size        : 16;		// bits : 15_0
      unsigned ise_v_size        : 16;		// bits : 31_16
    } bit;
    UINT32 word;
  } ise_register_4; // 0x0010

  union
  {
    struct
    {
      unsigned ise_st_size           : 9;		// bits : 8_0
      unsigned                       : 7;
      unsigned ise_ovlap_size        : 2;		// bits : 17_16
    } bit;
    UINT32 word;
  } ise_register_5; // 0x0014

  union
  {
    struct
    {
      unsigned                 : 2;
      unsigned ise_dram_ofsi        : 18;		// bits : 19_2
    } bit;
    UINT32 word;
  } ise_register_6; // 0x0018

  union
  {
    struct
    {
    } bit;
    UINT32 word;
  } ise_register_7; // 0x001c

  union
  {
    struct
    {
      unsigned ise_dram_sai        : 32;		// bits : 31_0
    } bit;
    UINT32 word;
  } ise_register_8; // 0x0020

  union
  {
    struct
    {
    } bit;
    UINT32 word;
  } ise_register_9; // 0x0024

  union
  {
    struct
    {
    } bit;
    UINT32 word;
  } ise_register_10; // 0x0028

  union
  {
    struct
    {
      unsigned ise_dram_ll_sai        : 32;		// bits : 31_0
    } bit;
    UINT32 word;
  } ise_register_11; // 0x002c

  union
  {
    struct
    {
      unsigned h_ud              : 1;		// bits : 0
      unsigned v_ud              : 1;		// bits : 1
      unsigned h_dnrate          : 5;		// bits : 6_2
      unsigned v_dnrate          : 5;		// bits : 11_7
      unsigned                   : 3;
      unsigned h_filtmode        : 1;		// bits : 15
      unsigned h_filtcoef        : 6;		// bits : 21_16
      unsigned v_filtmode        : 1;		// bits : 22
      unsigned v_filtcoef        : 6;		// bits : 28_23
    } bit;
    UINT32 word;
  } ise_register_12; // 0x0030

  union
  {
    struct
    {
      unsigned h_sfact        : 16;		// bits : 15_0
      unsigned v_sfact        : 16;		// bits : 31_16
    } bit;
    UINT32 word;
  } ise_register_13; // 0x0034

  union
  {
    struct
    {
      unsigned h_osize        : 16;		// bits : 15_0
      unsigned v_osize        : 16;		// bits : 31_16
    } bit;
    UINT32 word;
  } ise_register_14; // 0x0038

  union
  {
    struct
    {
      unsigned ise_hsft_int        : 16;		// bits : 15_0
      unsigned ise_vsft_int        : 16;		// bits : 31_16
    } bit;
    UINT32 word;
  } ise_register_15; // 0x003c

  union
  {
    struct
    {
      unsigned ise_hsft_dec        : 16;		// bits : 15_0
      unsigned ise_vsft_dec        : 16;		// bits : 31_16
    } bit;
    UINT32 word;
  } ise_register_16; // 0x0040

  union
  {
    struct
    {
      unsigned isd_h_base        : 13;		// bits : 12_0
      unsigned                   : 3;
      unsigned isd_v_base        : 13;		// bits : 28_16
    } bit;
    UINT32 word;
  } ise_register_17; // 0x0044

  union
  {
    struct
    {
      unsigned isd_h_sfact0        : 13;		// bits : 12_0
      unsigned                     : 3;
      unsigned isd_v_sfact0        : 13;		// bits : 28_16
    } bit;
    UINT32 word;
  } ise_register_18; // 0x0048

  union
  {
    struct
    {
      unsigned isd_h_sfact1        : 13;		// bits : 12_0
      unsigned                     : 3;
      unsigned isd_v_sfact1        : 13;		// bits : 28_16
    } bit;
    UINT32 word;
  } ise_register_19; // 0x004c

  union
  {
    struct
    {
      unsigned isd_h_sfact2        : 13;		// bits : 12_0
      unsigned                     : 3;
      unsigned isd_v_sfact2        : 13;		// bits : 28_16
    } bit;
    UINT32 word;
  } ise_register_20; // 0x0050

  union
  {
    struct
    {
      unsigned ise_isd_mode              : 1;		// bits : 0
      unsigned                           : 2;
      unsigned ise_isd_h_coef_num        : 5;		// bits : 7_3
      unsigned ise_isd_v_coef_num        : 5;		// bits : 12_8
    } bit;
    UINT32 word;
  } ise_register_21; // 0x0054

  union
  {
    struct
    {
      unsigned ise_isd_h_coef0        : 8;		// bits : 7_0
      unsigned ise_isd_h_coef1        : 8;		// bits : 15_8
      unsigned ise_isd_h_coef2        : 8;		// bits : 23_16
      unsigned ise_isd_h_coef3        : 8;		// bits : 31_24
    } bit;
    UINT32 word;
  } ise_register_22; // 0x0058

  union
  {
    struct
    {
      unsigned ise_isd_h_coef4        : 8;		// bits : 7_0
      unsigned ise_isd_h_coef5        : 8;		// bits : 15_8
      unsigned ise_isd_h_coef6        : 8;		// bits : 23_16
      unsigned ise_isd_h_coef7        : 8;		// bits : 31_24
    } bit;
    UINT32 word;
  } ise_register_23; // 0x005c

  union
  {
    struct
    {
      unsigned ise_isd_h_coef8         : 8;		// bits : 7_0
      unsigned ise_isd_h_coef9         : 8;		// bits : 15_8
      unsigned ise_isd_h_coef10        : 8;		// bits : 23_16
      unsigned ise_isd_h_coef11        : 8;		// bits : 31_24
    } bit;
    UINT32 word;
  } ise_register_24; // 0x0060

  union
  {
    struct
    {
      unsigned ise_isd_h_coef12        : 8;		// bits : 7_0
      unsigned ise_isd_h_coef13        : 8;		// bits : 15_8
      unsigned ise_isd_h_coef14        : 8;		// bits : 23_16
      unsigned ise_isd_h_coef15        : 8;		// bits : 31_24
    } bit;
    UINT32 word;
  } ise_register_25; // 0x0064

  union
  {
    struct
    {
      unsigned ise_isd_h_coef16        : 12;		// bits : 11_0
    } bit;
    UINT32 word;
  } ise_register_26; // 0x0068

  union
  {
    struct
    {
      unsigned ise_isd_h_coef_msb0        : 4;		// bits : 3_0
      unsigned ise_isd_h_coef_msb1        : 4;		// bits : 7_4
      unsigned ise_isd_h_coef_msb2        : 4;		// bits : 11_8
      unsigned ise_isd_h_coef_msb3        : 4;		// bits : 15_12
      unsigned ise_isd_h_coef_msb4        : 4;		// bits : 19_16
      unsigned ise_isd_h_coef_msb5        : 4;		// bits : 23_20
      unsigned ise_isd_h_coef_msb6        : 4;		// bits : 27_24
      unsigned ise_isd_h_coef_msb7        : 4;		// bits : 31_28
    } bit;
    UINT32 word;
  } ise_register_27; // 0x006c

  union
  {
    struct
    {
      unsigned ise_isd_h_coef_msb8         : 4;		// bits : 3_0
      unsigned ise_isd_h_coef_msb9         : 4;		// bits : 7_4
      unsigned ise_isd_h_coef_msb10        : 4;		// bits : 11_8
      unsigned ise_isd_h_coef_msb11        : 4;		// bits : 15_12
      unsigned ise_isd_h_coef_msb12        : 4;		// bits : 19_16
      unsigned ise_isd_h_coef_msb13        : 4;		// bits : 23_20
      unsigned ise_isd_h_coef_msb14        : 4;		// bits : 27_24
      unsigned ise_isd_h_coef_msb15        : 4;		// bits : 31_28
    } bit;
    UINT32 word;
  } ise_register_28; // 0x0070

  union
  {
    struct
    {
      unsigned ise_isd_v_coef0        : 8;		// bits : 7_0
      unsigned ise_isd_v_coef1        : 8;		// bits : 15_8
      unsigned ise_isd_v_coef2        : 8;		// bits : 23_16
      unsigned ise_isd_v_coef3        : 8;		// bits : 31_24
    } bit;
    UINT32 word;
  } ise_register_29; // 0x0074

  union
  {
    struct
    {
      unsigned ise_isd_v_coef4        : 8;		// bits : 7_0
      unsigned ise_isd_v_coef5        : 8;		// bits : 15_8
      unsigned ise_isd_v_coef6        : 8;		// bits : 23_16
      unsigned ise_isd_v_coef7        : 8;		// bits : 31_24
    } bit;
    UINT32 word;
  } ise_register_30; // 0x0078

  union
  {
    struct
    {
      unsigned ise_isd_v_coef8         : 8;		// bits : 7_0
      unsigned ise_isd_v_coef9         : 8;		// bits : 15_8
      unsigned ise_isd_v_coef10        : 8;		// bits : 23_16
      unsigned ise_isd_v_coef11        : 8;		// bits : 31_24
    } bit;
    UINT32 word;
  } ise_register_31; // 0x007c

  union
  {
    struct
    {
      unsigned ise_isd_v_coef12        : 8;		// bits : 7_0
      unsigned ise_isd_v_coef13        : 8;		// bits : 15_8
      unsigned ise_isd_v_coef14        : 8;		// bits : 23_16
      unsigned ise_isd_v_coef15        : 8;		// bits : 31_24
    } bit;
    UINT32 word;
  } ise_register_32; // 0x0080

  union
  {
    struct
    {
      unsigned ise_isd_v_coef16        : 12;		// bits : 11_0
    } bit;
    UINT32 word;
  } ise_register_33; // 0x0084

  union
  {
    struct
    {
      unsigned ise_isd_v_coef_msb0        : 4;		// bits : 3_0
      unsigned ise_isd_v_coef_msb1        : 4;		// bits : 7_4
      unsigned ise_isd_v_coef_msb2        : 4;		// bits : 11_8
      unsigned ise_isd_v_coef_msb3        : 4;		// bits : 15_12
      unsigned ise_isd_v_coef_msb4        : 4;		// bits : 19_16
      unsigned ise_isd_v_coef_msb5        : 4;		// bits : 23_20
      unsigned ise_isd_v_coef_msb6        : 4;		// bits : 27_24
      unsigned ise_isd_v_coef_msb7        : 4;		// bits : 31_28
    } bit;
    UINT32 word;
  } ise_register_34; // 0x0088

  union
  {
    struct
    {
      unsigned ise_isd_v_coef_msb8         : 4;		// bits : 3_0
      unsigned ise_isd_v_coef_msb9         : 4;		// bits : 7_4
      unsigned ise_isd_v_coef_msb10        : 4;		// bits : 11_8
      unsigned ise_isd_v_coef_msb11        : 4;		// bits : 15_12
      unsigned ise_isd_v_coef_msb12        : 4;		// bits : 19_16
      unsigned ise_isd_v_coef_msb13        : 4;		// bits : 23_20
      unsigned ise_isd_v_coef_msb14        : 4;		// bits : 27_24
      unsigned ise_isd_v_coef_msb15        : 4;		// bits : 31_28
    } bit;
    UINT32 word;
  } ise_register_35; // 0x008c

  union
  {
    struct
    {
      unsigned                 : 2;
      unsigned ise_dram_ofso        : 18;		// bits : 19_2
    } bit;
    UINT32 word;
  } ise_register_36; // 0x0090

  union
  {
    struct
    {
    } bit;
    UINT32 word;
  } ise_register_37; // 0x0094

  union
  {
    struct
    {
      unsigned ise_dram_sao        : 32;		// bits : 31_0
    } bit;
    UINT32 word;
  } ise_register_38; // 0x0098

  union
  {
    struct
    {
    } bit;
    UINT32 word;
  } ise_register_39; // 0x009c

  union
  {
    struct
    {
      unsigned ise_isd_h_coef_all        : 17;		// bits : 16_0
    } bit;
    UINT32 word;
  } ise_register_40; // 0x00a0

  union
  {
    struct
    {
      unsigned ise_isd_h_coef_half        : 17;		// bits : 16_0
    } bit;
    UINT32 word;
  } ise_register_41; // 0x00a4

  union
  {
    struct
    {
      unsigned ise_isd_v_coef_all        : 17;		// bits : 16_0
    } bit;
    UINT32 word;
  } ise_register_42; // 0x00a8

  union
  {
    struct
    {
      unsigned ise_isd_v_coef_half        : 17;		// bits : 16_0
    } bit;
    UINT32 word;
  } ise_register_43; // 0x00ac

} NT98560_ISE_REG_STRUCT;




/*
    ise_rst         :    [0x0, 0x1],            bits : 0
    ise_start       :    [0x0, 0x1],            bits : 1
    ise_ll_fire     :    [0x0, 0x1],            bits : 28
*/
#define ISE_ENGINE_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(ISE_ENGINE_CONTROL_REGISTER)
REGDEF_BIT(ise_rst,        1)
REGDEF_BIT(ise_start,        1)
REGDEF_BIT(,        26)
REGDEF_BIT(ise_ll_fire,        1)
REGDEF_END(ISE_ENGINE_CONTROL_REGISTER)


/*
    ise_in_fmt       :    [0x0, 0xf],           bits : 3_0
    ISE_OUT_FMT      :    [0x0, 0xf],           bits : 7_4
    ise_scl_method   :    [0x0, 0x3],           bits : 9_8
    ise_argb_outmode :    [0x0, 0x3],           bits : 13_12
    ise_in_brt_lenth :    [0x0, 0x1],           bits : 16
    ise_out_brt_lenth:    [0x0, 0x1],           bits : 17
    ise_ovlap_mode   :    [0x0, 0x1],           bits : 18
*/
#define ISE_INPUT_CONTROL_REGISTER0_OFS 0x0004
REGDEF_BEGIN(ISE_INPUT_CONTROL_REGISTER0)
REGDEF_BIT(ise_in_fmt,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(ise_scl_method,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_argb_outmode,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_in_brt_lenth,        1)
REGDEF_BIT(ise_out_brt_lenth,        1)
REGDEF_BIT(ise_ovlap_mode,        1)
REGDEF_END(ISE_INPUT_CONTROL_REGISTER0)


/*
    ise_inte_ll_end :    [0x0, 0x1],            bits : 0
    ise_inte_ll_err :    [0x0, 0x1],            bits : 1
    ise_inte_frm_end:    [0x0, 0x1],            bits : 31
*/
#define ISE_INTERRUPT_ENABLE_REGISTER_OFS 0x0008
REGDEF_BEGIN(ISE_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(ise_inte_ll_end,        1)
REGDEF_BIT(ise_inte_ll_err,        1)
REGDEF_BIT(,        29)
REGDEF_BIT(ise_inte_frm_end,        1)
REGDEF_END(ISE_INTERRUPT_ENABLE_REGISTER)


/*
    ise_ints_ll_end :    [0x0, 0x1],            bits : 0
    ise_ints_ll_err :    [0x0, 0x1],            bits : 1
    ise_ints_frm_end:    [0x0, 0x1],            bits : 31
*/
#define ISE_INTERRUPT_STATUS_REGISTER_OFS 0x000c
REGDEF_BEGIN(ISE_INTERRUPT_STATUS_REGISTER)
REGDEF_BIT(ise_ints_ll_end,        1)
REGDEF_BIT(ise_ints_ll_err,        1)
REGDEF_BIT(,        29)
REGDEF_BIT(ise_ints_frm_end,        1)
REGDEF_END(ISE_INTERRUPT_STATUS_REGISTER)


/*
    ise_h_size:    [0x0, 0xffff],           bits : 15_0
    ise_v_size:    [0x0, 0xffff],           bits : 31_16
*/
#define ISE_INPUT_IMAGE_SIZE_REGISTER0_OFS 0x0010
REGDEF_BEGIN(ISE_INPUT_IMAGE_SIZE_REGISTER0)
REGDEF_BIT(ise_h_size,        16)
REGDEF_BIT(ise_v_size,        16)
REGDEF_END(ISE_INPUT_IMAGE_SIZE_REGISTER0)


/*
    ise_st_size   :    [0x0, 0x1ff],            bits : 8_0
    ise_ovlap_size:    [0x0, 0x3],          bits : 17_16
*/
#define ISE_INPUT_STRIPE_SIZE_REGISTER0_OFS 0x0014
REGDEF_BEGIN(ISE_INPUT_STRIPE_SIZE_REGISTER0)
REGDEF_BIT(ise_st_size,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(ise_ovlap_size,        2)
REGDEF_END(ISE_INPUT_STRIPE_SIZE_REGISTER0)


/*
    ise_dram_ofsi:    [0x0, 0x3ffff],           bits : 19_2
*/
#define ISE_INPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0018
REGDEF_BEGIN(ISE_INPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_dram_ofsi,        18)
REGDEF_END(ISE_INPUT_DMA_LINEOFFSET_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RESERVED0_OFS 0x001c
REGDEF_BEGIN(RESERVED0)
REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED0)


/*
    ise_dram_sai:    [0x0, 0xffffffff],         bits : 31_0
*/
#define ISE_INPUT_DMA_CHANNEL_REGISTER0_OFS 0x0020
REGDEF_BEGIN(ISE_INPUT_DMA_CHANNEL_REGISTER0)
REGDEF_BIT(ise_dram_sai,        32)
REGDEF_END(ISE_INPUT_DMA_CHANNEL_REGISTER0)

/*
    ise_dmach_idle :    [0x0, 0x1],          bits : 0
    ise_dmach_dis  :    [0x0, 0x1],          bits : 1
*/
#define ISE_DMA_DISABLE_REGISTER_OFS 0x0024
REGDEF_BEGIN(ISE_DMA_DISABLE_REGISTER)
REGDEF_BIT(ise_dmach_idle,        1)
REGDEF_BIT(ise_dmach_dis,		  1)
REGDEF_END(ISE_DMA_DISABLE_REGISTER)

/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define ISE_LINKED_LIST_CONTROL_REGISTER0_OFS 0x0028
REGDEF_BEGIN(ISE_LINKED_LIST_CONTROL_REGISTER0)
REGDEF_BIT(reserved,        29)
REGDEF_BIT(ise_ll_terminate,        1)
REGDEF_END(ISE_LINKED_LIST_CONTROL_REGISTER0)


/*
    ise_dram_ll_sai:    [0x0, 0xffffffff],          bits : 31_0
*/
#define ISE_INPUT_DMA_CHANNEL_REGISTER3_OFS 0x002c
REGDEF_BEGIN(ISE_INPUT_DMA_CHANNEL_REGISTER3)
REGDEF_BIT(ise_dram_ll_sai,        32)
REGDEF_END(ISE_INPUT_DMA_CHANNEL_REGISTER3)


/*
    h_ud      :    [0x0, 0x1],          bits : 0
    v_ud      :    [0x0, 0x1],          bits : 1
    h_dnrate  :    [0x0, 0x1f],         bits : 6_2
    v_dnrate  :    [0x0, 0x1f],         bits : 11_7
    h_filtmode:    [0x0, 0x1],          bits : 15
    h_filtcoef:    [0x0, 0x3f],         bits : 21_16
    v_filtmode:    [0x0, 0x1],          bits : 22
    v_filtcoef:    [0x0, 0x3f],         bits : 28_23
*/
#define ISE_OUTPUT_CONTROL_REGISTER0_OFS 0x0030
REGDEF_BEGIN(ISE_OUTPUT_CONTROL_REGISTER0)
REGDEF_BIT(h_ud,        1)
REGDEF_BIT(v_ud,        1)
REGDEF_BIT(h_dnrate,        5)
REGDEF_BIT(v_dnrate,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(h_filtmode,        1)
REGDEF_BIT(h_filtcoef,        6)
REGDEF_BIT(v_filtmode,        1)
REGDEF_BIT(v_filtcoef,        6)
REGDEF_END(ISE_OUTPUT_CONTROL_REGISTER0)


/*
    h_sfact:    [0x0, 0xffff],          bits : 15_0
    v_sfact:    [0x0, 0xffff],          bits : 31_16
*/
#define ISE_OUTPUT_CONTROL_REGISTER1_OFS 0x0034
REGDEF_BEGIN(ISE_OUTPUT_CONTROL_REGISTER1)
REGDEF_BIT(h_sfact,        16)
REGDEF_BIT(v_sfact,        16)
REGDEF_END(ISE_OUTPUT_CONTROL_REGISTER1)


/*
    h_osize:    [0x0, 0xffff],          bits : 15_0
    v_osize:    [0x0, 0xffff],          bits : 31_16
*/
#define ISE_OUTPUT_CONTROL_REGISTER2_OFS 0x0038
REGDEF_BEGIN(ISE_OUTPUT_CONTROL_REGISTER2)
REGDEF_BIT(h_osize,        16)
REGDEF_BIT(v_osize,        16)
REGDEF_END(ISE_OUTPUT_CONTROL_REGISTER2)


/*
    ise_hsft_int:    [0x0, 0xffff],         bits : 15_0
    ise_vsft_int:    [0x0, 0xffff],         bits : 31_16
*/
#define ISE_SCALING_START_OFFSET_REGISTER0_OFS 0x003c
REGDEF_BEGIN(ISE_SCALING_START_OFFSET_REGISTER0)
REGDEF_BIT(ise_hsft_int,        16)
REGDEF_BIT(ise_vsft_int,        16)
REGDEF_END(ISE_SCALING_START_OFFSET_REGISTER0)


/*
    ise_hsft_dec:    [0x0, 0xffff],         bits : 15_0
    ise_vsft_dec:    [0x0, 0xffff],         bits : 31_16
*/
#define ISE_SCALING_START_OFFSET_REGISTER1_OFS 0x0040
REGDEF_BEGIN(ISE_SCALING_START_OFFSET_REGISTER1)
REGDEF_BIT(ise_hsft_dec,        16)
REGDEF_BIT(ise_vsft_dec,        16)
REGDEF_END(ISE_SCALING_START_OFFSET_REGISTER1)


/*
    isd_h_base:    [0x0, 0x1fff],           bits : 12_0
    isd_v_base:    [0x0, 0x1fff],           bits : 28_16
*/
#define ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER3_OFS 0x0044
REGDEF_BEGIN(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER3)
REGDEF_BIT(isd_h_base,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_base,        13)
REGDEF_END(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER3)


/*
    isd_h_sfact0:    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact0:    [0x0, 0x1fff],         bits : 28_16
*/
#define ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER0_OFS 0x0048
REGDEF_BEGIN(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER0)
REGDEF_BIT(isd_h_sfact0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact0,        13)
REGDEF_END(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER0)


/*
    isd_h_sfact1:    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact1:    [0x0, 0x1fff],         bits : 28_16
*/
#define ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER1_OFS 0x004c
REGDEF_BEGIN(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER1)
REGDEF_BIT(isd_h_sfact1,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact1,        13)
REGDEF_END(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER1)


/*
    isd_h_sfact2:    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact2:    [0x0, 0x1fff],         bits : 28_16
*/
#define ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER2_OFS 0x0050
REGDEF_BEGIN(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER2)
REGDEF_BIT(isd_h_sfact2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact2,        13)
REGDEF_END(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER2)


/*
    ise_isd_mode      :    [0x0, 0x1],          bits : 0
    ise_isd_h_coef_num:    [0x0, 0x1f],         bits : 7_3
    ise_isd_v_coef_num:    [0x0, 0x1f],         bits : 12_8
*/
#define ISE_INTEGRATION_SCALE_USER_COEFFICIENTS_REGISTER0_OFS 0x0054
REGDEF_BEGIN(ISE_INTEGRATION_SCALE_USER_COEFFICIENTS_REGISTER0)
REGDEF_BIT(ise_isd_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_isd_h_coef_num,        5)
REGDEF_BIT(ise_isd_v_coef_num,        5)
REGDEF_END(ISE_INTEGRATION_SCALE_USER_COEFFICIENTS_REGISTER0)


/*
    ise_isd_h_coef0:    [0x0, 0xff],            bits : 7_0
    ise_isd_h_coef1:    [0x0, 0xff],            bits : 15_8
    ise_isd_h_coef2:    [0x0, 0xff],            bits : 23_16
    ise_isd_h_coef3:    [0x0, 0xff],            bits : 31_24
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER0_OFS 0x0058
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER0)
REGDEF_BIT(ise_isd_h_coef0,        8)
REGDEF_BIT(ise_isd_h_coef1,        8)
REGDEF_BIT(ise_isd_h_coef2,        8)
REGDEF_BIT(ise_isd_h_coef3,        8)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER0)


/*
    ise_isd_h_coef4:    [0x0, 0xff],            bits : 7_0
    ise_isd_h_coef5:    [0x0, 0xff],            bits : 15_8
    ise_isd_h_coef6:    [0x0, 0xff],            bits : 23_16
    ise_isd_h_coef7:    [0x0, 0xff],            bits : 31_24
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER1_OFS 0x005c
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER1)
REGDEF_BIT(ise_isd_h_coef4,        8)
REGDEF_BIT(ise_isd_h_coef5,        8)
REGDEF_BIT(ise_isd_h_coef6,        8)
REGDEF_BIT(ise_isd_h_coef7,        8)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER1)


/*
    ise_isd_h_coef8 :    [0x0, 0xff],           bits : 7_0
    ise_isd_h_coef9 :    [0x0, 0xff],           bits : 15_8
    ise_isd_h_coef10:    [0x0, 0xff],           bits : 23_16
    ise_isd_h_coef11:    [0x0, 0xff],           bits : 31_24
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER2_OFS 0x0060
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER2)
REGDEF_BIT(ise_isd_h_coef8,        8)
REGDEF_BIT(ise_isd_h_coef9,        8)
REGDEF_BIT(ise_isd_h_coef10,        8)
REGDEF_BIT(ise_isd_h_coef11,        8)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER2)


/*
    ise_isd_h_coef12:    [0x0, 0xff],           bits : 7_0
    ise_isd_h_coef13:    [0x0, 0xff],           bits : 15_8
    ise_isd_h_coef14:    [0x0, 0xff],           bits : 23_16
    ise_isd_h_coef15:    [0x0, 0xff],           bits : 31_24
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER3_OFS 0x0064
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER3)
REGDEF_BIT(ise_isd_h_coef12,        8)
REGDEF_BIT(ise_isd_h_coef13,        8)
REGDEF_BIT(ise_isd_h_coef14,        8)
REGDEF_BIT(ise_isd_h_coef15,        8)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER3)


/*
    ise_isd_h_coef16:    [0x0, 0xfff],          bits : 11_0
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER4_OFS 0x0068
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER4)
REGDEF_BIT(ise_isd_h_coef16,        12)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER4)


/*
    ise_isd_h_coef_msb0:    [0x0, 0xf],         bits : 3_0
    ise_isd_h_coef_msb1:    [0x0, 0xf],         bits : 7_4
    ise_isd_h_coef_msb2:    [0x0, 0xf],         bits : 11_8
    ise_isd_h_coef_msb3:    [0x0, 0xf],         bits : 15_12
    ise_isd_h_coef_msb4:    [0x0, 0xf],         bits : 19_16
    ise_isd_h_coef_msb5:    [0x0, 0xf],         bits : 23_20
    ise_isd_h_coef_msb6:    [0x0, 0xf],         bits : 27_24
    ise_isd_h_coef_msb7:    [0x0, 0xf],         bits : 31_28
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER5_OFS 0x006c
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER5)
REGDEF_BIT(ise_isd_h_coef_msb0,        4)
REGDEF_BIT(ise_isd_h_coef_msb1,        4)
REGDEF_BIT(ise_isd_h_coef_msb2,        4)
REGDEF_BIT(ise_isd_h_coef_msb3,        4)
REGDEF_BIT(ise_isd_h_coef_msb4,        4)
REGDEF_BIT(ise_isd_h_coef_msb5,        4)
REGDEF_BIT(ise_isd_h_coef_msb6,        4)
REGDEF_BIT(ise_isd_h_coef_msb7,        4)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER5)


/*
    ise_isd_h_coef_msb8 :    [0x0, 0xf],            bits : 3_0
    ise_isd_h_coef_msb9 :    [0x0, 0xf],            bits : 7_4
    ise_isd_h_coef_msb10:    [0x0, 0xf],            bits : 11_8
    ise_isd_h_coef_msb11:    [0x0, 0xf],            bits : 15_12
    ise_isd_h_coef_msb12:    [0x0, 0xf],            bits : 19_16
    ise_isd_h_coef_msb13:    [0x0, 0xf],            bits : 23_20
    ise_isd_h_coef_msb14:    [0x0, 0xf],            bits : 27_24
    ise_isd_h_coef_msb15:    [0x0, 0xf],            bits : 31_28
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER6_OFS 0x0070
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER6)
REGDEF_BIT(ise_isd_h_coef_msb8,        4)
REGDEF_BIT(ise_isd_h_coef_msb9,        4)
REGDEF_BIT(ise_isd_h_coef_msb10,        4)
REGDEF_BIT(ise_isd_h_coef_msb11,        4)
REGDEF_BIT(ise_isd_h_coef_msb12,        4)
REGDEF_BIT(ise_isd_h_coef_msb13,        4)
REGDEF_BIT(ise_isd_h_coef_msb14,        4)
REGDEF_BIT(ise_isd_h_coef_msb15,        4)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER6)


/*
    ise_isd_v_coef0:    [0x0, 0xff],            bits : 7_0
    ise_isd_v_coef1:    [0x0, 0xff],            bits : 15_8
    ise_isd_v_coef2:    [0x0, 0xff],            bits : 23_16
    ise_isd_v_coef3:    [0x0, 0xff],            bits : 31_24
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER0_OFS 0x0074
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER0)
REGDEF_BIT(ise_isd_v_coef0,        8)
REGDEF_BIT(ise_isd_v_coef1,        8)
REGDEF_BIT(ise_isd_v_coef2,        8)
REGDEF_BIT(ise_isd_v_coef3,        8)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER0)


/*
    ise_isd_v_coef4:    [0x0, 0xff],            bits : 7_0
    ise_isd_v_coef5:    [0x0, 0xff],            bits : 15_8
    ise_isd_v_coef6:    [0x0, 0xff],            bits : 23_16
    ise_isd_v_coef7:    [0x0, 0xff],            bits : 31_24
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER1_OFS 0x0078
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER1)
REGDEF_BIT(ise_isd_v_coef4,        8)
REGDEF_BIT(ise_isd_v_coef5,        8)
REGDEF_BIT(ise_isd_v_coef6,        8)
REGDEF_BIT(ise_isd_v_coef7,        8)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER1)


/*
    ise_isd_v_coef8 :    [0x0, 0xff],           bits : 7_0
    ise_isd_v_coef9 :    [0x0, 0xff],           bits : 15_8
    ise_isd_v_coef10:    [0x0, 0xff],           bits : 23_16
    ise_isd_v_coef11:    [0x0, 0xff],           bits : 31_24
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER2_OFS 0x007c
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER2)
REGDEF_BIT(ise_isd_v_coef8,        8)
REGDEF_BIT(ise_isd_v_coef9,        8)
REGDEF_BIT(ise_isd_v_coef10,        8)
REGDEF_BIT(ise_isd_v_coef11,        8)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER2)


/*
    ise_isd_v_coef12:    [0x0, 0xff],           bits : 7_0
    ise_isd_v_coef13:    [0x0, 0xff],           bits : 15_8
    ise_isd_v_coef14:    [0x0, 0xff],           bits : 23_16
    ise_isd_v_coef15:    [0x0, 0xff],           bits : 31_24
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER3_OFS 0x0080
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER3)
REGDEF_BIT(ise_isd_v_coef12,        8)
REGDEF_BIT(ise_isd_v_coef13,        8)
REGDEF_BIT(ise_isd_v_coef14,        8)
REGDEF_BIT(ise_isd_v_coef15,        8)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER3)


/*
    ise_isd_v_coef16:    [0x0, 0xfff],          bits : 11_0
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER4_OFS 0x0084
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER4)
REGDEF_BIT(ise_isd_v_coef16,        12)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER4)


/*
    ise_isd_v_coef_msb0:    [0x0, 0xf],         bits : 3_0
    ise_isd_v_coef_msb1:    [0x0, 0xf],         bits : 7_4
    ise_isd_v_coef_msb2:    [0x0, 0xf],         bits : 11_8
    ise_isd_v_coef_msb3:    [0x0, 0xf],         bits : 15_12
    ise_isd_v_coef_msb4:    [0x0, 0xf],         bits : 19_16
    ise_isd_v_coef_msb5:    [0x0, 0xf],         bits : 23_20
    ise_isd_v_coef_msb6:    [0x0, 0xf],         bits : 27_24
    ise_isd_v_coef_msb7:    [0x0, 0xf],         bits : 31_28
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER5_OFS 0x0088
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER5)
REGDEF_BIT(ise_isd_v_coef_msb0,        4)
REGDEF_BIT(ise_isd_v_coef_msb1,        4)
REGDEF_BIT(ise_isd_v_coef_msb2,        4)
REGDEF_BIT(ise_isd_v_coef_msb3,        4)
REGDEF_BIT(ise_isd_v_coef_msb4,        4)
REGDEF_BIT(ise_isd_v_coef_msb5,        4)
REGDEF_BIT(ise_isd_v_coef_msb6,        4)
REGDEF_BIT(ise_isd_v_coef_msb7,        4)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER5)


/*
    ise_isd_v_coef_msb8 :    [0x0, 0xf],            bits : 3_0
    ise_isd_v_coef_msb9 :    [0x0, 0xf],            bits : 7_4
    ise_isd_v_coef_msb10:    [0x0, 0xf],            bits : 11_8
    ise_isd_v_coef_msb11:    [0x0, 0xf],            bits : 15_12
    ise_isd_v_coef_msb12:    [0x0, 0xf],            bits : 19_16
    ise_isd_v_coef_msb13:    [0x0, 0xf],            bits : 23_20
    ise_isd_v_coef_msb14:    [0x0, 0xf],            bits : 27_24
    ise_isd_v_coef_msb15:    [0x0, 0xf],            bits : 31_28
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER6_OFS 0x008c
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER6)
REGDEF_BIT(ise_isd_v_coef_msb8,        4)
REGDEF_BIT(ise_isd_v_coef_msb9,        4)
REGDEF_BIT(ise_isd_v_coef_msb10,        4)
REGDEF_BIT(ise_isd_v_coef_msb11,        4)
REGDEF_BIT(ise_isd_v_coef_msb12,        4)
REGDEF_BIT(ise_isd_v_coef_msb13,        4)
REGDEF_BIT(ise_isd_v_coef_msb14,        4)
REGDEF_BIT(ise_isd_v_coef_msb15,        4)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER6)


/*
    ISE_OFSO:    [0x0, 0x3ffff],            bits : 19_2
*/
#define ISE_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0090
REGDEF_BEGIN(ISE_OUTPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_dram_ofso,        18)
REGDEF_END(ISE_OUTPUT_DMA_LINEOFFSET_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RESERVED3_OFS 0x0094
REGDEF_BEGIN(RESERVED3)
REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED3)


/*
    ise_dram_sao:    [0x0, 0xffffffff],         bits : 31_0
*/
#define ISE_OUTPUT_DMA_CHANNEL_REGISTER0_OFS 0x0098
REGDEF_BEGIN(ISE_OUTPUT_DMA_CHANNEL_REGISTER0)
REGDEF_BIT(ise_dram_sao,        32)
REGDEF_END(ISE_OUTPUT_DMA_CHANNEL_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RESERVED4_OFS 0x009c
REGDEF_BEGIN(RESERVED4)
REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED4)


/*
    ise_isd_h_coef_all:    [0x0, 0x1ffff],          bits : 16_0
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER7_OFS 0x00a0
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER7)
REGDEF_BIT(ise_isd_h_coef_all,        17)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER7)


/*
    ise_isd_h_coef_half:    [0x0, 0x1ffff],         bits : 16_0
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER8_OFS 0x00a4
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER8)
REGDEF_BIT(ise_isd_h_coef_half,        17)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER8)


/*
    ise_isd_v_coef_all:    [0x0, 0x1ffff],          bits : 16_0
*/
#define ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER7_OFS 0x00a8
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER7)
REGDEF_BIT(ise_isd_v_coef_all,        17)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER7)


/*
    ise_isd_v_coef_half:    [0x0, 0x1ffff],         bits : 16_0
*/
#define ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER8_OFS 0x00ac
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER8)
REGDEF_BIT(ise_isd_v_coef_half,        17)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER8)


#endif

