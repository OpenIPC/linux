
/******************************************************************************
 *-----------------------------------------------------------------------------
 *
 *    Copyright (c) 2011 MStar Semiconductor, Inc.  All rights reserved.
 *
 *-----------------------------------------------------------------------------
 * FILE NAME      gop_c2_reg.h
 * DESCRIPTION
 *          Including some MACRO needed in msb250xfb.c
 *                    Defined CLRREG16(), SETREG16()
 *          (refer to include/asm-arm/arch-msb25xx/io.h)
 *                    Defined used MASK and setting value
 *                    Defined CHIPTOP, OP2, VOP, GOP Base Addres in Physical Address
 *                    and theirs offset    (drvop2.h and drvgop.h)
 *                    Declared Boot_splash array. the image content shown in boot
 *          (boot_splasy.h)
 *            refering sources list
 *                    drvop2.h and drvgop.h
 *              boot_splasy.h
 *
 * AUTHOR         Chun Fan
 *
 * HISTORY
 *                2008/05/06    Chun    initial version
 *                2008/10/02    Chun    add MSB251x code, add CONFIG_ARCH_MSB251X
 *                                      add some IP base address Macro
 *                                      BK_LPLL_BASE, BK_DAC_BASE,
 *                                      and OP2_DITH_REG_BASE
 *
 *        linux/drivers/video/msb250xfb.h    --    msb250xfb frame buffer device
 ******************************************************************************/



#define GET_REG8_ADDR(x, y)     (x+(y)*2)
#define GET_REG16_ADDR(x, y)    (x+(y)*4)



/* ========================================================================= */
/* Define HW base address */
#define COLUMBUS_BASE_REG_RIU_PA            (0x1F000000)
#define CLKG0_REG_BASE_PA                   (0x100B00)

#define RIU_BASE                    (0xA0000000)


#define CEDRIC_BASE_REG_GOP_PA     GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, 0x080F80)
#define CHICAGO_BASE_REG_GOP_PA    GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, 0x001A80)

// same as cedric
#define mdrv_BASE_REG_GOP_PA CEDRIC_BASE_REG_GOP_PA

#define mdrv_BASE_REG_CHIPTOP_PA   GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, 0x080580)

#define mdrv_BASE_REG_GOP00_PA          GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x123000/2))
#define mdrv_BASE_REG_GOP01_PA          GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x123000/2))
#define mdrv_BASE_REG_GOP02_PA          GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x123000/2))

#define mdrv_REG_GOP31_7E	            GET_REG16_ADDR(mdrv_BASE_REG_GOP31_PA,0x7E)
#define mdrv_REG_GOP31_7F	            GET_REG16_ADDR(mdrv_BASE_REG_GOP31_PA,0x7F)
#define mdrv_REG_GOP01_WIN              GET_REG16_ADDR(mdrv_BASE_REG_GOP01_PA,0x00)
#define mdrv_REG_GOP31_WIN              GET_REG16_ADDR(mdrv_BASE_REG_GOP31_PA,0x00)
#define mdrv_REG_GOP01_ALPHA            GET_REG16_ADDR(mdrv_BASE_REG_GOP01_PA,0x0A)
#define mdrv_REG_GOP01_7F	            GET_REG16_ADDR(mdrv_BASE_REG_GOP01_PA,0x7F)
#define mdrv_REG_GOP31_ALPHA            GET_REG16_ADDR(mdrv_BASE_REG_GOP31_PA,0x0A)

#define     mdrv_GWin0_Disable   (0xFFFE)
#define		mdrv_GWIN0_Enable	(0x1)

#define mdrv_ENABLE_CONST_ALPHA	(~(0x1<<14))
//fomrat define

#define  RGB1555_Blink 0x0000
#define  RGB565  0x0010
#define  ARGB4444  0x0020
#define  _2266     0x0030
#define  _8Bit_Palette 0x0040

#define  RGBA8888  0x0050

#define  ARGB1555  0x0060

#define  ARGB8888  0x0070

#define  RGB1555_UV7Y8 0x0080
#define  UV8Y8 0x0090
#define  RGBA5551  0x00A0
#define  RGBA4444  0x00B0

//------------------------------------------------------------------------------
// GOP
//------------------------------------------------------------------------------
// Bank 0
// REG_GOP_00
#define GOP_SOFT_RESET            0x0001
#define GOP_VS_INV                0x0002
#define GOP_HS_INV                0x0004
#define GOP_DISPLAY_MODE          0x0008
#define GOP_FIELD_INV             0x0010
#define GOP_TEST_PATTENR_MODE     0x0040
#define GOP_5541_EN               0x0080  // RGB5541 alpha mask mode enable, only for RGB1555 Data type
#define GOP_REG_OUTPUT_READY      0x0100

#define GOP_GENSHOT_FAST          0x0200
#define GOP_YUVOUT                0x0400
#define GOP_RGB_Transparent_color_enable              0x0800

#define GOP_HS_MASK               0x4000
#define GOP_ALPHA_INV             0x8000

#define GOP_ALPHA_MASK            0x7F00
#define GOP_ALPHA                 0x3F00
#define GOP_PIXEL_ALPHA_EN        0x4000

#define GWIN_display_mode_progress 0x0008
#define GWIN_display_mode_interlace 0x0000


// REG_GOP_01
#define GOP_DST_IP_MAIN           0x0000
#define GOP_DST_IP_SUB            0x0001
#define GOP_DST_OP                0x0002
#define GOP_DST_MVOP              0x0003
#define GOP_DST_SC1_IP_MAIN       0x0004
#define GOP_DST_SC1_OP            0x0005



#define GOP_DMA_LEN_8             0x0000
#define GOP_DMA_LEN_16            0x0004
#define GOP_DMA_LEN_32            0x0008
#define GOP_DMA_LEN_ALL           0x000C
#define GOP_MIU_TSH_1             0x0010
#define GOP_REGDMA_INTERVAL_START 0x0200 //cedric:0x0300
#define GOP_REGDMA_INTERVAL_END   0x4000 //cedirc:0x5000

// REG_GOP_02
#define GOP_BLINK_DISABLE         0x0000
#define GOP_BLINK_ENABLE          0x8000

// REG_GOP_06
#define GOP_REGDMA_END            0x0034

// REG_GOP_07
#define GOP_REGDMA_STR            0x0000

//REG_GOP_0F


#define AMBER3_GOP_HSYNC_PIPE_DLY        0x00E5
#define CEDRIC_GOP_HSYNC_PIPE_DLY        0x0070
#define AMBER3_GOP_HSYNC_PIPE_DLY        0x00E5
#define CEDRIC_GOP_HSYNC_PIPE_DLY        0x0070
#define CHICAGO_GOP_HSYNC_PIPE_DLY       0x000B

#define GOP_HSYNC_PIPE_DLY_FOR_IP_SUB    0x0080
#define GOP_HSYNC_PIPE_DLY_FOR_SC1_OP    0x00A0
#define  HSYNC_PIPE_DLY_FOR_SC0_2GOP     0x0044



// inifinity settings
#define INIFINITY_GOP_HSYNC_PIPE_DLY 0x0000
#define GOP_HSYNC_PIPE_DLY INIFINITY_GOP_HSYNC_PIPE_DLY



#define GOP_HSYNC_PIPE_DLY_FOR_IP_SUB  0x0080

// REG_GOP_20
#define GOP_GWIN_HIGH_PRI_0       0x0000
#define GOP_GWIN_HIGH_PRI_1       0x0001



#define GOP_STRETCH_HST 0x0000


// REG_GOP_34
#define GOP_STRETCH_VST 0x0000

// REG_GOP_35
#define GOP_STRETCH_HRATIO 0x1000

// REG_GOP_36
#define GOP_STRETCH_VRATIO 0x1000

// REG_GOP_38
#define GOP_STRETCH_HINI 0x0000

// REG_GOP_39
#define GOP_STRETCH_VINI 0x0000

// REG_GOP_7E
#define GOP_GOPG0_MUX_MASK 0x0007
#define GOP_GOPG0_MUX_4G 0x0000
#define GOP_GOPG0_MUX_2G 0x0001

#define GOP_Layer0_MUX_MASK 0x0007
#define GOP_Layer0_Sel_GOP0 0x0000
#define GOP_Layer0_Sel_GOP1 0x0001
#define GOP_Layer0_Sel_GOP2 0x0002
#define GOP_Layer0_Sel_GOP3 0x0003
#define GOP_Layer0_Sel_GOP_Null 0x0004

#define GOP_Layer1_MUX_MASK (0x0007)<<3
#define GOP_Layer1_Sel_GOP0 (0x0000)<<3
#define GOP_Layer1_Sel_GOP1 (0x0001)<<3
#define GOP_Layer1_Sel_GOP2 (0x0002)<<3
#define GOP_Layer1_Sel_GOP3 (0x0003)<<3
#define GOP_Layer1_Sel_GOP_Null (0x0004)<<3

#define GOP_Layer2_MUX_MASK (0x0007)<<6
#define GOP_Layer2_Sel_GOP0 (0x0000)<<6
#define GOP_Layer2_Sel_GOP1 (0x0001)<<6
#define GOP_Layer2_Sel_GOP2 (0x0002)<<6
#define GOP_Layer2_Sel_GOP3 (0x0003)<<6
#define GOP_Layer2_Sel_GOP_Null (0x0004)<<6

#define GOP_Layer3_MUX_MASK (0x0007)<<9
#define GOP_Layer3_Sel_GOP0 (0x0000)<<9
#define GOP_Layer3_Sel_GOP1 (0x0001)<<9
#define GOP_Layer3_Sel_GOP2 (0x0002)<<9
#define GOP_Layer3_Sel_GOP3  (0x0003)<<9
#define GOP_Layer3_Sel_GOP_Null (0x0004)<<9





// REG_GOP_7F
#define GOP_BANK_G0_0           0x0000
#define GOP_BANK_G0_1           0x0001
#define GOP_BANK_G1_0           0x0003
#define GOP_BANK_G1_1           0x0004
#define GOP_BANK_G2_0           0x0006
#define GOP_BANK_G2_1           0x0007
#define GOP_BANK_G3_0           0x0009
#define GOP_BANK_G3_1           0x000A
#define GOP_BANK_GPD            0x0006

#define GOP_BANK_DOUBLE_WR      0x0100
#define GOP_BANK_FORCE_WR       0x0200

#define GOP_BANK_DOUBLE_WR_G0   0x0400
#define GOP_BANK_DOUBLE_WR_G1   0x0403
#define GOP_BANK_DOUBLE_WR_G2   0x0406
#define GOP_BANK_DOUBLE_WR_G3   0x0409
#define GOP_BANK_DOUBLE_WR_MSK  0x0400
#define GOP_BANK_SEL_MSK        0x000F
// GOP0 Bank 1
// REG_GOP_00
#define GOP0_GWin0_Enable           0x0001

// REG_GOP_20
#define GOP0_GWin1_Enable           0x0001




// Chip Device ID
#define DEVICE_COLUMBUS2            (0x2E)
#define DEVICE_AMBER3               (0x47)
#define DEVICE_CEDRIC               (0x50)
#define DEVICE_CHICAGO              (0x70)


