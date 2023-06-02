#ifndef _NUE2_NT98520_H_
#define _NUE2_NT98520_H_

#define _NUE2_REG_BASE_ADDR 0xF0D50000

typedef struct
{

  union
  {
    struct
    {
      unsigned NUE2_RST          : 1;		// bits : 0
      unsigned NUE2_START        : 1;		// bits : 1
      unsigned                   : 26;
      unsigned LL_FIRE           : 1;		// bits : 28
    } Bit;
    UINT32 Word;
  } NUE2_Register_0; // 0x0000

  union
  {
    struct
    {
      unsigned NUE2_YUV2RGB_EN            : 1;		// bits : 0
      unsigned NUE2_SUB_EN                : 1;		// bits : 1
      unsigned NUE2_PAD_EN                : 1;		// bits : 2
      unsigned NUE2_HSV_EN                : 1;		// bits : 3
      unsigned NUE2_ROTATE_EN             : 1;		// bits : 4
      unsigned NUE2_YCC_ENC_EN            : 1;		// bits : 5
      unsigned                            : 6;
      unsigned NUE2_IN_FMT                : 2;		// bits : 13_12
      unsigned NUE2_OUT_SIGNEDNESS        : 1;		// bits : 14
      unsigned NUE2_SUB_MODE              : 1;		// bits : 15
      unsigned NUE2_HSV_OUT_MODE          : 1;		// bits : 16
      unsigned NUE2_ROTATE_MODE           : 2;		// bits : 18_17
    } Bit;
    UINT32 Word;
  } NUE2_Register_1; // 0x0004

  union
  {
    struct
    {
      unsigned DRAM_SAI0        : 32;		// bits : 31_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_2; // 0x0008

  union
  {
    struct
    {
      unsigned DRAM_SAI1        : 32;		// bits : 31_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_3; // 0x000c

  union
  {
    struct
    {
      unsigned DRAM_SAI2        : 32;		// bits : 31_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_4; // 0x0010

  union
  {
    struct
    {
      unsigned DRAM_SAILL        : 32;		// bits : 31_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_5; // 0x0014

  union
  {
    struct
    {
      unsigned DRAM_SAO0        : 32;		// bits : 31_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_6; // 0x0018

  union
  {
    struct
    {
      unsigned DRAM_SAO1        : 32;		// bits : 31_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_7; // 0x001c

  union
  {
    struct
    {
      unsigned DRAM_SAO2        : 31;		// bits : 30_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_8; // 0x0020

  union
  {
    struct
    {
      unsigned DRAM_OFSI0        : 17;		// bits : 16_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_9; // 0x0024

  union
  {
    struct
    {
      unsigned DRAM_OFSI1        : 17;		// bits : 16_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_10; // 0x0028

  union
  {
    struct
    {
      unsigned DRAM_OFSI2        : 17;		// bits : 16_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_11; // 0x002c

  union
  {
    struct
    {
      unsigned DRAM_OFSO0        : 17;		// bits : 16_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_12; // 0x0030

  union
  {
    struct
    {
      unsigned DRAM_OFSO1        : 17;		// bits : 16_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_13; // 0x0034

  union
  {
    struct
    {
      unsigned DRAM_OFSO2        : 17;		// bits : 16_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_14; // 0x0038

  union
  {
    struct
    {
      unsigned INTE_FRM_END            : 1;		// bits : 0
      unsigned INTE_DMAIN0END          : 1;		// bits : 1
      unsigned INTE_DMAIN1END          : 1;		// bits : 2
      unsigned INTE_DMAIN2END          : 1;		// bits : 3
      unsigned INTE_ENC_OUTOVFL        : 1;		// bits : 4
      unsigned INTE_SCL_ERR            : 1;		// bits : 5
      unsigned INTE_SUB_ERR            : 1;		// bits : 6
      unsigned                         : 1;
      unsigned INTE_LLEND              : 1;		// bits : 8
      unsigned INTE_LLERROR            : 1;		// bits : 9
    } Bit;
    UINT32 Word;
  } NUE2_Register_15; // 0x003c

  union
  {
    struct
    {
      unsigned INT_FRM_END            : 1;		// bits : 0
      unsigned INT_DMAIN0END          : 1;		// bits : 1
      unsigned INT_DMAIN1END          : 1;		// bits : 2
      unsigned INT_DMAIN2END          : 1;		// bits : 3
      unsigned INT_ENC_OUTOVFL        : 1;		// bits : 4
      unsigned INT_SCL_ERR            : 1;		// bits : 5
      unsigned INT_SUB_ERR            : 1;		// bits : 6
      unsigned                        : 1;
      unsigned INT_LLEND              : 1;		// bits : 8
      unsigned INT_LLERROR            : 1;		// bits : 9
    } Bit;
    UINT32 Word;
  } NUE2_Register_16; // 0x0040

  union
  {
    struct
    {
      unsigned NUE2_IN_WIDTH         : 11;		// bits : 10_0
      unsigned                       : 5;
      unsigned NUE2_IN_HEIGHT        : 11;		// bits : 26_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_17; // 0x0044

  union
  {
    struct
    {
      unsigned NUE2_H_DNRATE          : 7;		// bits : 6_0
      unsigned                        : 1;
      unsigned NUE2_V_DNRATE          : 7;		// bits : 14_8
      unsigned                        : 1;
      unsigned NUE2_H_FILTMODE        : 1;		// bits : 16
      unsigned NUE2_H_FILTCOEF        : 6;		// bits : 22_17
      unsigned                        : 1;
      unsigned NUE2_V_FILTMODE        : 1;		// bits : 24
      unsigned NUE2_V_FILTCOEF        : 6;		// bits : 30_25
    } Bit;
    UINT32 Word;
  } NUE2_Register_18; // 0x0048

  union
  {
    struct
    {
      unsigned NUE2_H_SFACT        : 16;		// bits : 15_0
      unsigned NUE2_V_SFACT        : 16;		// bits : 31_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_19; // 0x004c

  union
  {
    struct
    {
      unsigned NUE2_INI_H_DNRATE        : 7;		// bits : 6_0
      unsigned                          : 8;
      unsigned NUE2_INI_H_SFACT         : 17;		// bits : 31_15
    } Bit;
    UINT32 Word;
  } NUE2_Register_20; // 0x0050

  union
  {
    struct
    {
	  unsigned NUE2_FINAL_H_DNRATE : 7;		// bits : 6_0
	  unsigned : 8;
	  unsigned NUE2_FINAL_H_SFACT : 17;		// bits : 31_15
      //unsigned NUE2_FINAL_H_DNRATE        : 16;		// bits : 15_0
      //unsigned NUE2_FINAL_H_SFACT         : 16;		// bits : 31_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_21; // 0x0054

  union
  {
    struct
    {
      unsigned NUE2_H_SCL_SIZE        : 11;		// bits : 10_0
      unsigned                        : 5;
      unsigned NUE2_V_SCL_SIZE        : 11;		// bits : 26_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_22; // 0x0058

  union
  {
    struct
    {
      unsigned NUE2_SUB_IN_WIDTH         : 11;		// bits : 10_0
      unsigned                           : 5;
      unsigned NUE2_SUB_IN_HEIGHT        : 11;		// bits : 26_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_23; // 0x005c

  union
  {
    struct
    {
      unsigned NUE2_SUB_COEF_0        : 8;		// bits : 7_0
      unsigned NUE2_SUB_COEF_1        : 8;		// bits : 15_8
      unsigned NUE2_SUB_COEF_2        : 8;		// bits : 23_16
      unsigned NUE2_SUB_DUP           : 2;		// bits : 25_24
      unsigned                        : 2;
      unsigned NUE2_SUB_SHF           : 2;		// bits : 29_28
    } Bit;
    UINT32 Word;
  } NUE2_Register_24; // 0x0060

  union
  {
    struct
    {
      unsigned NUE2_PAD_CROP_X        : 11;		// bits : 10_0
      unsigned                        : 5;
      unsigned NUE2_PAD_CROP_Y        : 11;		// bits : 26_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_25; // 0x0064

  union
  {
    struct
    {
      unsigned NUE2_PAD_CROP_WIDTH         : 12;		// bits : 11_0
      unsigned                             : 4;
      unsigned NUE2_PAD_CROP_HEIGHT        : 12;		// bits : 27_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_26; // 0x0068

  union
  {
    struct
    {
      unsigned NUE2_PAD_OUT_X        : 16;		// bits : 15_0
      unsigned NUE2_PAD_OUT_Y        : 16;		// bits : 31_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_27; // 0x006c

  union
  {
    struct
    {
      unsigned NUE2_PAD_OUT_WIDTH         : 12;		// bits : 11_0
      unsigned                            : 4;
      unsigned NUE2_PAD_OUT_HEIGHT        : 12;		// bits : 27_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_28; // 0x0070

  union
  {
    struct
    {
      unsigned NUE2_PAD_VAL_0        : 8;		// bits : 7_0
      unsigned NUE2_PAD_VAL_1        : 8;		// bits : 15_8
      unsigned NUE2_PAD_VAL_2        : 8;		// bits : 23_16
    } Bit;
    UINT32 Word;
  } NUE2_Register_29; // 0x0074

  union
  {
    struct
    {
      unsigned NUE2_HUE_SFT        : 7;		// bits : 6_0
    } Bit;
    UINT32 Word;
  } NUE2_Register_30; // 0x0078

  union
  {
    struct
    {
      unsigned INDATA_BURST_MODE        : 2;		// bits : 1_0
      unsigned OUTRST_BURST_MODE        : 2;		// bits : 3_2
      unsigned DEBUGPORTSEL             : 2;		// bits : 5_4
    } Bit;
    UINT32 Word;
  } NUE2_Register_31; // 0x007c

  union
  {
    struct
    {
      unsigned LL_TABLE_IDX0        : 8;		// bits : 7_0
      unsigned LL_TABLE_IDX1        : 8;		// bits : 15_8
      unsigned LL_TABLE_IDX2        : 8;		// bits : 23_16
      unsigned LL_TABLE_IDX3        : 8;		// bits : 31_24
    } Bit;
    UINT32 Word;
  } NUE2_Register_32; // 0x0080

  union
  {
    struct
    {
      unsigned LL_TABLE_IDX4        : 8;		// bits : 7_0
      unsigned LL_TABLE_IDX5        : 8;		// bits : 15_8
      unsigned LL_TABLE_IDX6        : 8;		// bits : 23_16
      unsigned LL_TABLE_IDX7        : 8;		// bits : 31_24
    } Bit;
    UINT32 Word;
  } NUE2_Register_33; // 0x0084

  union
  {
    struct
    {
      unsigned LL_TABLE_IDX8         : 8;		// bits : 7_0
      unsigned LL_TABLE_IDX9         : 8;		// bits : 15_8
      unsigned LL_TABLE_IDX10        : 8;		// bits : 23_16
      unsigned LL_TABLE_IDX11        : 8;		// bits : 31_24
    } Bit;
    UINT32 Word;
  } NUE2_Register_34; // 0x0088

  union
  {
    struct
    {
      unsigned LL_TABLE_IDX12        : 8;		// bits : 7_0
      unsigned LL_TABLE_IDX13        : 8;		// bits : 15_8
      unsigned LL_TABLE_IDX14        : 8;		// bits : 23_16
      unsigned LL_TABLE_IDX15        : 8;		// bits : 31_24
    } Bit;
    UINT32 Word;
  } NUE2_Register_35; // 0x008c

  union
  {
    struct
    {
      unsigned LL_TERMINATE        : 1;		// bits : 0
    } Bit;
    UINT32 Word;
  } NUE2_Register_36; // 0x0090

  union
  {
    struct
    {
      unsigned DMA_DISABLE        : 1;		// bits : 0
      unsigned                    : 14;
      unsigned NUE2_IDLE          : 1;		// bits : 15
    } Bit;
    UINT32 Word;
  } NUE2_Register_37; // 0x0090




} NT98520_NUE2_REG_STRUCT;


#endif
