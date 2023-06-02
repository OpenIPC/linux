/*
    Protected header for PLL module

    PLL Configuration module protected header file.

    @file       pll_protected.h
    @ingroup    mIDrvSys_CG
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _PLL_PROTECTED_H
#define _PLL_PROTECTED_H

#include <kwrap/nvt_type.h>
#include "plat/pll.h"

/*
    @addtogroup mIDrvSys_CG
*/
//@{

#define PLL_CLKSEL_R1_OFFSET        32    //0x14
#define PLL_CLKSEL_R2_OFFSET        64    //0x18
#define PLL_CLKSEL_R3_OFFSET        96    //0x1C
#define PLL_CLKSEL_R4_OFFSET        128   //0x20
#define PLL_CLKSEL_R5_OFFSET        160   //0x24
#define PLL_CLKSEL_R6_OFFSET        192   //0x28
#define PLL_CLKSEL_R7_OFFSET        224   //0x2C
#define PLL_CLKSEL_R8_OFFSET        256   //0x30
#define PLL_CLKSEL_R9_OFFSET        288   //0x34
#define PLL_CLKSEL_R10_OFFSET       320   //0x38
#define PLL_CLKSEL_R11_OFFSET       352   //0x3C
#define PLL_CLKSEL_R12_OFFSET       384   //0x40
#define PLL_CLKSEL_R13_OFFSET       416   //0x44
#define PLL_CLKSEL_R14_OFFSET       448   //0x48
#define PLL_CLKSEL_R15_OFFSET       480   //0x4C
#define PLL_CLKSEL_R16_OFFSET       512   //0x50
#define PLL_CLKSEL_R17_OFFSET       544   //0x54
#define PLL_CLKSEL_R18_OFFSET       576   //0x58
#define PLL_CLKSEL_R19_OFFSET       608   //0x5C
#define PLL_CLKSEL_R20_OFFSET       640   //0x60
#define PLL_CLKSEL_R21_OFFSET       672   //0x64
#define PLL_CLKSEL_R22_OFFSET       704   //0x68
#define PLL_CLKSEL_R23_OFFSET       736   //0x6C
#define PLL_CLKSEL_R24_OFFSET       768   //0x70


/*
    Clock select ID

    Clock select ID for pll_set_clock_rate() & pll_get_clock_rate().
*/
typedef enum {
	//0x10
	//System Clock Rate bit definition
	PLL_CLKSEL_CPU =            0,                          //< Clock Select Module ID: CPU
	PLL_CLKSEL_APB =            4,                          //< Clock Select Module ID: APB
	PLL_CLKSEL_DMA2 =           6,                          //< Clock Select Module ID: DMA2
	PLL_CLKSEL_DSP2 =           8,                          //< Clock Select Module ID: DSP2
	PLL_CLKSEL_DMA_ARBT =       10,                         //< Clock select Module ID: DMA Arbiter
	PLL_CLKSEL_DSP_IOP =        12,                         //< Clock Select Module ID: DSP IOP
	PLL_CLKSEL_DSP_EDP =        14,                         //< Clock Select Module ID: DSP EDP
	PLL_CLKSEL_DSP_EPP =        16,                         //< Clock Select Module ID: DSP EPP
	PLL_CLKSEL_DSP_EDAP =       18,                         //< Clock Select Module ID: DSP EDAP
	PLL_CLKSEL_DSP  =           20,                         //< Clock Select Module ID: DSP
	PLL_CLKSEL_DSP2_IOP =       22,                         //< Clock Select Module ID: DSP2 IOP
	PLL_CLKSEL_DSP2_EDP =       24,                         //< Clock Select Module ID: DSP2 EDP
	PLL_CLKSEL_DSP2_EPP =       26,                         //< Clock Select Module ID: DSP2 EPP
	PLL_CLKSEL_DSP2_EDAP =      28,                         //< Clock Select Module ID: DSP2 EDAP

	//IPP Clock Rate 0 bit definition
	PLL_CLKSEL_SDE =            PLL_CLKSEL_R1_OFFSET + 0,   //< Clock Select Module ID: SDE
	PLL_CLKSEL_IPE =            PLL_CLKSEL_R1_OFFSET + 4,   //< Clock Select Module ID: IPE
	PLL_CLKSEL_DIS =            PLL_CLKSEL_R1_OFFSET + 8,   //< Clock Select Module ID: DIS
	PLL_CLKSEL_IME =            PLL_CLKSEL_R1_OFFSET + 12,  //< Clock Select Module ID: IME
	PLL_CLKSEL_FDE =            PLL_CLKSEL_R1_OFFSET + 16,  //< Clock Select Module ID: FDE
	PLL_CLKSEL_IVE =            PLL_CLKSEL_R1_OFFSET + 20,  //< Clock Select Module ID: IVE
	PLL_CLKSEL_ISE =            PLL_CLKSEL_R1_OFFSET + 24,  //< Clock Select Module ID: SIE
	PLL_CLKSEL_DCE =            PLL_CLKSEL_R1_OFFSET + 28,  //< Clock Select Module ID: DCE

	//IPP Clock Rate 1 bit definition
	PLL_CLKSEL_IFE =            PLL_CLKSEL_R2_OFFSET + 0,   //< Clock Select Module ID: IFE
	PLL_CLKSEL_IFE2 =           PLL_CLKSEL_R2_OFFSET + 4,   //< Clock Select Module ID: IFE2
	PLL_CLKSEL_SVM =            PLL_CLKSEL_R2_OFFSET + 8,   //< Clock Select Module ID: SVM
	PLL_CLKSEL_RHE =            PLL_CLKSEL_R2_OFFSET + 12,  //< Clock select Module ID: RHE
	PLL_CLKSEL_ISE2 =           PLL_CLKSEL_R2_OFFSET + 16,  //< Clock select Module ID: ISE2
	PLL_CLKSEL_SIE_MCLKINV =    PLL_CLKSEL_R2_OFFSET + 19,  //< Clock Select Module ID: SIE MCLK output Invert
	PLL_CLKSEL_RDE =            PLL_CLKSEL_R2_OFFSET + 20,  //< Backward compatible
	PLL_CLKSEL_TGE =            PLL_CLKSEL_R2_OFFSET + 22,  //< Clock Select Module ID: TGE
	PLL_CLKSEL_SIE2_MCLKINV =   PLL_CLKSEL_R2_OFFSET + 24,  //< Clock Select Module ID: SIE2 MCLK output Invert
	PLL_CLKSEL_RHE_CLK2 =       PLL_CLKSEL_R2_OFFSET + 26,  //< Backward compatible
	PLL_CLKSEL_DRE =            PLL_CLKSEL_R2_OFFSET + 28,  //< Clock Select Module ID: DRE
	PLL_CLKSEL_TGE2 =           PLL_CLKSEL_R2_OFFSET + 30,  //< Clock Select Module ID: TGE2

	//CODEC Clock Rate bit definition 0x1C
	PLL_CLKSEL_JPEG =           PLL_CLKSEL_R3_OFFSET + 0,   //< Clock Select Module ID: JPEG
	PLL_CLKSEL_H26X =           PLL_CLKSEL_R3_OFFSET + 4,   //< Clock Select Module ID: H.264
	PLL_CLKSEL_GRAPHIC =        PLL_CLKSEL_R3_OFFSET + 8,   //< Clock Select Module ID: GRAPHIC
	PLL_CLKSEL_GRAPHIC2 =       PLL_CLKSEL_R3_OFFSET + 12,  //< Clock Select Module ID: GRAPHIC2
	PLL_CLKSEL_AFFINE =         PLL_CLKSEL_R3_OFFSET + 16,  //< Clock Select Module ID: AFFINE
	PLL_CLKSEL_HWCPY =          PLL_CLKSEL_R3_OFFSET + 20,  //< Clock Select Module ID: HWCPY
	PLL_CLKSEL_ROTATE =         PLL_CLKSEL_R3_OFFSET + 24,  //< Clock Select Module ID: CRYPTO
	PLL_CLKSEL_CRYPTO =         PLL_CLKSEL_R3_OFFSET + 31,  //< Clock Select Module ID: CRYPTO

	//Peripheral Clock Rate bit definition 0x20
	PLL_CLKSEL_SDIO =           PLL_CLKSEL_R4_OFFSET + 4,   //< Clock Select Module ID: SDIO
	PLL_CLKSEL_SDIO2 =          PLL_CLKSEL_R4_OFFSET + 8,   //< Clock Select Module ID: SDIO2
	PLL_CLKSEL_IDE_CLKSRC =     PLL_CLKSEL_R4_OFFSET + 16,  //< Clock Select Module ID: IDE clock source
	PLL_CLKSEL_IDE2_CLKSRC =    PLL_CLKSEL_R4_OFFSET + 18,  //< Clock Select Module ID: IDE2 clock source
	PLL_CLKSEL_DSI_LPSRC  =     PLL_CLKSEL_R4_OFFSET + 20,  //< Clock Select Module ID: CSI LP clock source
	PLL_CLKSEL_MI_CLKSRC  =     PLL_CLKSEL_R4_OFFSET + 22,  //< Clock Select Module ID: MI clock source
	PLL_CLKSEL_HDMI_ADO_CLKMUX = PLL_CLKSEL_R4_OFFSET + 24, //< Clock Select Module ID: HDMI Audio clock mux
	PLL_CLKSEL_ADO_CLKSEL =     PLL_CLKSEL_R4_OFFSET + 28,  //< Clock Select Module ID: Audio clock source
	PLL_CLKSEL_HDMI_ADO_CLKSEL = PLL_CLKSEL_R4_OFFSET + 30, //< Clock Select Module ID: HDMI Audio clock source

	//Peripheral Clock Rate bit definition 1 0x24
	PLL_CLKSEL_SDIO3 =          PLL_CLKSEL_R5_OFFSET + 0,   //< Clock Select Module ID: SDIO3
	PLL_CLKSEL_TSMUX =          PLL_CLKSEL_R5_OFFSET + 4,   //< Clock Select Module ID: TSMUX
	PLL_CLKSEL_SP =             PLL_CLKSEL_R5_OFFSET + 8,   //< Clock Select Module ID: SP
	PLL_CLKSEL_SP2 =            PLL_CLKSEL_R5_OFFSET + 10,  //< Clock Select Module ID: SP2
	PLL_CLKSEL_CANBUS =         PLL_CLKSEL_R5_OFFSET + 12,  //< Clock Select Module ID: CANBUS
	PLL_CLKSEL_ETH_REFCLK_INV  = PLL_CLKSEL_R5_OFFSET + 14, //< Clock Select Module ID: ETH reference clock invert
	PLL_CLKSEL_VX1_PIXCLK_INV  = PLL_CLKSEL_R5_OFFSET + 15, //< Clock Select Module ID: VX1 PIXCLK invert
	PLL_CLKSEL_VX1_PIXCLK2_INV = PLL_CLKSEL_R5_OFFSET + 16, //< Clock Select Module ID: VX1 PIXCLK2 invert
	PLL_CLKSEL_VX12_PIXCLK_INV = PLL_CLKSEL_R5_OFFSET + 17, //< Clock Select Module ID: VX12 PIXCLK invert
	PLL_CLKSEL_VX12_PIXCLK2_INV = PLL_CLKSEL_R5_OFFSET + 18, //< Clock Select Module ID: VX12 PIXCLK2 invert
	PLL_CLKSEL_ETH_REVMII_OPT_INV   = PLL_CLKSEL_R5_OFFSET + 19, //< Clock Select Module ID: ETH REVMII_OPT invert
	PLL_CLKSEL_ETH_REVMII_OPT2_INV  = PLL_CLKSEL_R5_OFFSET + 20, //< Clock Select Module ID: ETH REVMII_OPT2 invert
	PLL_CLKSEL_TSDEMUX          = PLL_CLKSEL_R5_OFFSET + 21, //< Clock Select Module ID: TSDEMUX
	PLL_CLKSEL_SLVSEC_D0CK_INV  = PLL_CLKSEL_R5_OFFSET + 23, //< Clock Select Module ID: SLVS-EC DataLane 0 Clock invert
	PLL_CLKSEL_SLVSEC_D1CK_INV  = PLL_CLKSEL_R5_OFFSET + 24, //< Clock Select Module ID: SLVS-EC DataLane 1 Clock invert
	PLL_CLKSEL_SLVSEC_D2CK_INV  = PLL_CLKSEL_R5_OFFSET + 25, //< Clock Select Module ID: SLVS-EC DataLane 2 Clock invert
	PLL_CLKSEL_SLVSEC_D3CK_INV  = PLL_CLKSEL_R5_OFFSET + 26, //< Clock Select Module ID: SLVS-EC DataLane 3 Clock invert
	PLL_CLKSEL_SLVSEC_D4CK_INV  = PLL_CLKSEL_R5_OFFSET + 27, //< Clock Select Module ID: SLVS-EC DataLane 4 Clock invert
	PLL_CLKSEL_SLVSEC_D5CK_INV  = PLL_CLKSEL_R5_OFFSET + 28, //< Clock Select Module ID: SLVS-EC DataLane 5 Clock invert
	PLL_CLKSEL_SLVSEC_D6CK_INV  = PLL_CLKSEL_R5_OFFSET + 29, //< Clock Select Module ID: SLVS-EC DataLane 6 Clock invert
	PLL_CLKSEL_SLVSEC_D7CK_INV  = PLL_CLKSEL_R5_OFFSET + 30, //< Clock Select Module ID: SLVS-EC DataLane 7 Clock invert

	//Peripheral Clock Rate bit definition 2 0x28
	PLL_CLKSEL_MIPI_LVDS =      PLL_CLKSEL_R6_OFFSET + 0,   //< Clock Select Module ID: MIPI/LVDS
	PLL_CLKSEL_MIPI_LVDS2 =     PLL_CLKSEL_R6_OFFSET + 1,   //< Clock Select Module ID: MIPI/LVDS2
	PLL_CLKSEL_MIPI_LVDS3 =     PLL_CLKSEL_R6_OFFSET + 2,   //< Clock Select Module ID: MIPI/LVDS3
	PLL_CLKSEL_MIPI_LVDS4 =     PLL_CLKSEL_R6_OFFSET + 3,   //< Clock Select Module ID: MIPI/LVDS3
	PLL_CLKSEL_MIPI_LVDS5 =     PLL_CLKSEL_R6_OFFSET + 4,   //< Clock Select Module ID: MIPI/LVDS3
	PLL_CLKSEL_MIPI_LVDS6 =     PLL_CLKSEL_R6_OFFSET + 5,   //< Clock Select Module ID: MIPI/LVDS3
	PLL_CLKSEL_MIPI_LVDS7 =     PLL_CLKSEL_R6_OFFSET + 6,   //< Clock Select Module ID: MIPI/LVDS3
	PLL_CLKSEL_MIPI_LVDS8 =     PLL_CLKSEL_R6_OFFSET + 7,   //< Clock Select Module ID: MIPI/LVDS3
	PLL_CLKSEL_LVDS_CLKPHASE =  PLL_CLKSEL_R6_OFFSET + 8,   //< Clock Select Module ID: MIPI/LVDS CLK Phase
	PLL_CLKSEL_LVDS2_CLKPHASE = PLL_CLKSEL_R6_OFFSET + 9,   //< Clock Select Module ID: MIPI2/LVDS2 CLK Phase
	PLL_CLKSEL_LVDS3_CLKPHASE = PLL_CLKSEL_R6_OFFSET + 10,  //< Clock Select Module ID: MIPI3/LVDS3 CLK Phase
	PLL_CLKSEL_LVDS4_CLKPHASE = PLL_CLKSEL_R6_OFFSET + 11,  //< Clock Select Module ID: MIPI4/LVDS4 CLK Phase
	PLL_CLKSEL_LVDS5_CLKPHASE = PLL_CLKSEL_R6_OFFSET + 12,  //< Clock Select Module ID: MIPI5/LVDS5 CLK Phase
	PLL_CLKSEL_LVDS6_CLKPHASE = PLL_CLKSEL_R6_OFFSET + 13,  //< Clock Select Module ID: MIPI6/LVDS6 CLK Phase
	PLL_CLKSEL_LVDS7_CLKPHASE = PLL_CLKSEL_R6_OFFSET + 14,  //< Clock Select Module ID: MIPI7/LVDS7 CLK Phase
	PLL_CLKSEL_LVDS8_CLKPHASE = PLL_CLKSEL_R6_OFFSET + 15,  //< Clock Select Module ID: MIPI8/LVDS8 CLK Phase
	PLL_CLKSEL_SLVSEC =         PLL_CLKSEL_R6_OFFSET + 16,  //< Clock Select Module ID: SLVSEC
	PLL_CLKSEL_U3PCLK_INV     = PLL_CLKSEL_R6_OFFSET + 20,  //< Clock Select Module ID: USB3 PIPE Clock invert
	PLL_CLKSEL_U3UCLK_INV     = PLL_CLKSEL_R6_OFFSET + 21,  //< Clock Select Module ID: USB3 UClock invert
	PLL_CLKSEL_U2UCLK_INV     = PLL_CLKSEL_R6_OFFSET + 22,  //< Clock Select Module ID: USB2 UClock invert
	PLL_CLKSEL_U3REFCLK_SEL   = PLL_CLKSEL_R6_OFFSET + 23,  //< Clock Select Module ID: USB3 Ref-Clock select
	PLL_CLKSEL_SIE_IO_PXCLK   = PLL_CLKSEL_R6_OFFSET + 24,  //< Clock Select Module ID: SIE IO PXCLK select
	PLL_CLKSEL_SIE2_IO_PXCLK  = PLL_CLKSEL_R6_OFFSET + 25,  //< Clock Select Module ID: SIE2 IO PXCLK select
	PLL_CLKSEL_SIE4_IO_PXCLK  = PLL_CLKSEL_R6_OFFSET + 27,  //< Clock Select Module ID: SIE4 IO PXCLK select
	PLL_CLKSEL_SIE5_IO_PXCLK  = PLL_CLKSEL_R6_OFFSET + 28,  //< Clock Select Module ID: SIE5 IO PXCLK select
	PLL_CLKSEL_SIE7_IO_PXCLK  = PLL_CLKSEL_R6_OFFSET + 30,  //< Clock Select Module ID: SIE7 IO PXCLK select
	PLL_CLKSEL_SIE8_IO_PXCLK  = PLL_CLKSEL_R6_OFFSET + 31,  //< Clock Select Module ID: SIE8 IO PXCLK select

	//Peripheral Clock Rate bit definition 3 0x2C
	PLL_CLKSEL_SIE_CLKSRC =     PLL_CLKSEL_R7_OFFSET + 0,   //< Clock Select Module ID: SIE CLK source
	PLL_CLKSEL_SIE2_CLKSRC =    PLL_CLKSEL_R7_OFFSET + 3,   //< Clock Select Module ID: SIE2 CLK source
	PLL_CLKSEL_SIE3_CLKSRC =    PLL_CLKSEL_R7_OFFSET + 6,   //< Clock Select Module ID: SIE3 CLK source
	PLL_CLKSEL_SIE4_CLKSRC =    PLL_CLKSEL_R7_OFFSET + 9,   //< Clock Select Module ID: SIE3 CLK source
	PLL_CLKSEL_SIE_MCLKSRC =    PLL_CLKSEL_R7_OFFSET + 12,  //< Clock Select Module ID: SIE MCLK source
	PLL_CLKSEL_SIE_MCLK2SRC =   PLL_CLKSEL_R7_OFFSET + 14,  //< Clock Select Module ID: SIE MCLK2 source
	PLL_CLKSEL_SIE_PXCLKSRC =   PLL_CLKSEL_R7_OFFSET + 16,  //< Clock Select Module ID: SIE PX CLK source
	PLL_CLKSEL_SIE2_PXCLKSRC =  PLL_CLKSEL_R7_OFFSET + 18,  //< Clock Select Module ID: SIE2 PX CLK source
	PLL_CLKSEL_SIE3_PXCLKSRC =  PLL_CLKSEL_R7_OFFSET + 20,  //< Clock Select Module ID: SIE3 PX CLK source
	PLL_CLKSEL_SIE4_PXCLKSRC =  PLL_CLKSEL_R7_OFFSET + 22,  //< Clock Select Module ID: SIE4 PX CLK source
	PLL_CLKSEL_SIE5_PXCLKSRC =  PLL_CLKSEL_R7_OFFSET + 24,  //< Clock Select Module ID: SIE5 PX CLK source
	PLL_CLKSEL_SIE6_PXCLKSRC =  PLL_CLKSEL_R7_OFFSET + 26,  //< Clock Select Module ID: SIE6 PX CLK source
	PLL_CLKSEL_SIE7_PXCLKSRC =  PLL_CLKSEL_R7_OFFSET + 28,  //< Clock Select Module ID: SIE7 PX CLK source
	PLL_CLKSEL_SIE8_PXCLKSRC =  PLL_CLKSEL_R7_OFFSET + 30,  //< Clock Select Module ID: SIE8 PX CLK source


	//IPP Clock Divider bit definition 0x30
	PLL_CLKSEL_SIE_MCLKDIV =    PLL_CLKSEL_R8_OFFSET + 0,   //< Clock Select Module ID: SIE MCLK divider
	PLL_CLKSEL_SIE_MCLK2DIV =   PLL_CLKSEL_R8_OFFSET + 8,   //< Clock Select Module ID: SIE MCLK2 divider
	PLL_CLKSEL_SIE_CLKDIV =     PLL_CLKSEL_R8_OFFSET + 16,  //< Clock Select Module ID: SIE CLK divider
	PLL_CLKSEL_SIE2_CLKDIV =    PLL_CLKSEL_R8_OFFSET + 24,  //< Clock Select Module ID: SIE2 CLK divider


	//IPP Clock Divider bit definition 0x34
	PLL_CLKSEL_SIE3_CLKDIV =    PLL_CLKSEL_R9_OFFSET + 0,   //< Clock Select Module ID: IDE clock divider
	PLL_CLKSEL_SIE4_CLKDIV =    PLL_CLKSEL_R9_OFFSET + 8,   //< Clock Select Module ID: IDE clock divider
	PLL_CLKSEL_SIE5_CLKDIV =    PLL_CLKSEL_R9_OFFSET + 16,  //< Clock Select Module ID: IDE clock divider
	PLL_CLKSEL_SIE6_CLKDIV =    PLL_CLKSEL_R9_OFFSET + 20,  //< Clock Select Module ID: IDE clock divider
	PLL_CLKSEL_SIE7_CLKDIV =    PLL_CLKSEL_R9_OFFSET + 24,  //< Clock Select Module ID: IDE clock divider
	PLL_CLKSEL_SIE8_CLKDIV =    PLL_CLKSEL_R9_OFFSET + 28,  //< Clock Select Module ID: IDE clock divider

	//Video Clock Divider bit definition 0x38
	PLL_CLKSEL_MI_CLKDIV   =    PLL_CLKSEL_R10_OFFSET + 16, //< Clock Select Module ID: MI clock divider


	//Peripheral Clock Divider bit definition 0x3C
	PLL_CLKSEL_SP_CLKDIV    =   PLL_CLKSEL_R11_OFFSET + 0,  //< Clock Select Module ID: Special clock divider
	PLL_CLKSEL_CEC_CLKDIV   =   PLL_CLKSEL_R11_OFFSET + 8,  //< Clock Select Module ID: CEC clock divider
	PLL_CLKSEL_ADO_CLKDIV   =   PLL_CLKSEL_R11_OFFSET + 16, //< Clock Select Module ID: audio clock divider
	PLL_CLKSEL_HDMI_ADO_CLKDIV = PLL_CLKSEL_R11_OFFSET + 24, //< Clock Select Module ID: HDMI audio clock divider

	//SDIO Clock Divider bit definition 0x40
	PLL_CLKSEL_SDIO_CLKDIV =    PLL_CLKSEL_R12_OFFSET + 0,  //< Clock Select Module ID: SDIO clock divider
	PLL_CLKSEL_SDIO2_CLKDIV =   PLL_CLKSEL_R12_OFFSET + 12, //< Clock Select Module ID: SDIO2 clock divider
	PLL_CLKSEL_NAND_CLKDIV =    PLL_CLKSEL_R12_OFFSET + 24, //< Clock Select Module ID: NAND clock divider

	//Peripheral Clock Divider bit definition 1 0x44
	PLL_CLKSEL_SDIO3_CLKDIV =   PLL_CLKSEL_R13_OFFSET + 0,  //< Clock Select Module ID: SDIO3 clock divider
	PLL_CLKSEL_CLASSD_CLKDIV =   PLL_CLKSEL_R13_OFFSET + 16, //< Clock Select Module ID: EAC classD clock divider
	PLL_CLKSEL_SP2_CLKDIV    =  PLL_CLKSEL_R13_OFFSET + 24, //< Clock Select Module ID: Special clock2 divider

	//SPI Clock Divider bit definition 0x48
	PLL_CLKSEL_SPI_CLKDIV =     PLL_CLKSEL_R14_OFFSET + 0,  //< Clock Select Module ID: SPI clock divider
	PLL_CLKSEL_SPI2_CLKDIV =    PLL_CLKSEL_R14_OFFSET + 16, //< Clock Select Module ID: SPI2 clock divider

	//SPI Clock Divider bit definition 1 0x4C
	PLL_CLKSEL_SPI3_CLKDIV =    PLL_CLKSEL_R15_OFFSET + 0,  //< Clock Select Module ID: SPI3 clock divider
	PLL_CLKSEL_SPI4_CLKDIV =    PLL_CLKSEL_R15_OFFSET + 16, //< Clock Select Module ID: SPI4 clock divider

	//Video Clock Divider bit definition 0x6C
	PLL_CLKSEL_IDE_CLKDIV =         PLL_CLKSEL_R23_OFFSET + 0,   //< Clock Select Module ID: IDE clock divider
	PLL_CLKSEL_IDE2_CLKDIV =        PLL_CLKSEL_R23_OFFSET + 8,   //< Clock Select Module ID: IDE2 clock divider
	PLL_CLKSEL_IDE_OUTIF_CLKDIV =   PLL_CLKSEL_R23_OFFSET + 16,  //< Clock Select Module ID: IDE out interface clock divider
	PLL_CLKSEL_IDE2_OUTIF_CLKDIV =  PLL_CLKSEL_R23_OFFSET + 24,  //< Clock Select Module ID: IDE2 out interface clock divider

	//CSI LP Clock Selection bit definition 0x70
	PLL_CLKSEL_LPD0_LPCLK =         PLL_CLKSEL_R24_OFFSET + 0,   //< Clock Select Module ID: LP D0
	PLL_CLKSEL_LPD1_LPCLK =         PLL_CLKSEL_R24_OFFSET + 2,   //< Clock Select Module ID: LP D1
	PLL_CLKSEL_LPD2_LPCLK =         PLL_CLKSEL_R24_OFFSET + 4,   //< Clock Select Module ID: LP D2
	PLL_CLKSEL_LPD3_LPCLK =         PLL_CLKSEL_R24_OFFSET + 6,   //< Clock Select Module ID: LP D3
	PLL_CLKSEL_LPD4_LPCLK =         PLL_CLKSEL_R24_OFFSET + 8,   //< Clock Select Module ID: LP D4
	PLL_CLKSEL_LPD5_LPCLK =         PLL_CLKSEL_R24_OFFSET + 10,  //< Clock Select Module ID: LP D5
	PLL_CLKSEL_LPD6_LPCLK =         PLL_CLKSEL_R24_OFFSET + 12,  //< Clock Select Module ID: LP D6
	PLL_CLKSEL_LPD7_LPCLK =         PLL_CLKSEL_R24_OFFSET + 14,  //< Clock Select Module ID: LP D7

	ENUM_DUMMY4WORD(PLL_CLKSEL)
} PLL_CLKSEL;


#define PLL_CLKSEL_CPU2  (PLL_CLKSEL_R24_OFFSET + 0)                      //< Backward compatible
//#define PLL_CLKSEL_DSP   (PLL_CLKSEL_R24_OFFSET + 0)                      //< Backward compatible
#define PLL_CLKSEL_VX1SL (PLL_CLKSEL_R24_OFFSET + 0)                      //< Backward compatible
#define PLL_CLKSEL_NAND  (PLL_CLKSEL_R24_OFFSET + 0)                      //< Backward compatible
#define PLL_CLKSEL_SPI5_CLKDIV (PLL_CLKSEL_R24_OFFSET + 0)                      //< Backward compatible
#define PLL_CLKSEL_OCP   (PLL_CLKSEL_R24_OFFSET + 0)                      //< Backward compatible
#define PLL_CLKSEL_CNN   (PLL_CLKSEL_SDE)


/*
    @name   CPU clock rate value

    CPU clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CPU).
*/
//@{
#define PLL_CLKSEL_CPU_80         (0x00 << PLL_CLKSEL_CPU)      //< Select CPU clock 80MHz
#define PLL_CLKSEL_CPU_PLL8       (0x01 << PLL_CLKSEL_CPU)      //< Select CPU clock PLL8 (for CPU)
//@}
#define PLL_CLKSEL_CPU_240        (0x02 << PLL_CLKSEL_CPU)      //< Backward compatible
#define PLL_CLKSEL_CPU_480        (0x03 << PLL_CLKSEL_CPU)      //< Backward compatible

/*
    @name   APB clock rate value

    APB clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_APB).
*/
//@{
#define PLL_CLKSEL_APB_80         (0x00 << PLL_CLKSEL_APB)      //< Select APB 80MHz
#define PLL_CLKSEL_APB_120        (0x01 << PLL_CLKSEL_APB)      //< Select APB 120MHz
#define PLL_CLKSEL_APB_160        (0x02 << PLL_CLKSEL_APB)      //< Select APB 160MHz
#define PLL_CLKSEL_APB_240        (0x03 << PLL_CLKSEL_APB)      //< Select APB 240MHz
//@}

/*
    @name   DMA2 clock rate value

    DMA2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DMA2).
*/
//@{
#define PLL_CLKSEL_DMA2_PLL3      (0x00 << PLL_CLKSEL_DMA2)     //< Select DMA2 PLL3
#define PLL_CLKSEL_DMA2_PLL14     (0x01 << PLL_CLKSEL_DMA2)     //< Select DMA2 PLL14
//@}


/*
    @name   OCP clock rate value

    OCP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_OCP).
*/
//@{
//@}
#define PLL_CLKSEL_OCP_CPUCLK     (0x00)      //< Backward compatible
#define PLL_CLKSEL_OCP_CPUCLK_D2  (0x00)      //< Backward compatible
#define PLL_CLKSEL_AHB_CPUCLK     (PLL_CLKSEL_OCP_CPUCLK)       //< Backward compatible
#define PLL_CLKSEL_AHB_CPUCLK_D2  (PLL_CLKSEL_OCP_CPUCLK_D2)    //< Backward compatible


/*
    @name   CPU2 clock rate value

    CPU2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CPU2).
*/
//@{
//@}
#define PLL_CLKSEL_CPU2_240       (0x00)     //< Backward compatible
#define PLL_CLKSEL_CPU2_PLL9      (0x01)     //< Backward compatible


/*
    @name   DSP clock rate value

    DSP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP).
*/
//@{
#define PLL_CLKSEL_DSP_PLL9       (0x00 << PLL_CLKSEL_DSP)      //< Select DSP PLL9
#define PLL_CLKSEL_DSP_PLL10      (0x01 << PLL_CLKSEL_DSP)      //< Select DSP PLL10
#define PLL_CLKSEL_DSP_480        (0x02 << PLL_CLKSEL_DSP)      //< Select DSP 480
#define PLL_CLKSEL_DSP_PLL13      (0x03 << PLL_CLKSEL_DSP)      //< Select DSP PLL13
//@}
#define PLL_CLKSEL_DSP_240        (PLL_CLKSEL_DSP_PLL9)      //< Backward compatible




/*
    @name   DSP2 clock rate value

    DSP2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP2).
*/
//@{
#define PLL_CLKSEL_DSP2_PLL9      (0x00 << PLL_CLKSEL_DSP2)      //< Select DSP2 PLL9
#define PLL_CLKSEL_DSP2_PLL10     (0x01 << PLL_CLKSEL_DSP2)      //< Select DSP2 PLL10
#define PLL_CLKSEL_DSP2_480       (0x02 << PLL_CLKSEL_DSP2)      //< Select DSP2 480
#define PLL_CLKSEL_DSP2_PLL13     (0x03 << PLL_CLKSEL_DSP2)      //< Select DSP2 PLL13
//@}

/*
    @name   DMA Arbiter clock rate value

    DMA Arbiter clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DMA_ARRT).
*/
//@{
#define PLL_CLKSEL_DMA_ARBT_PLL3  (0x00)      //< Select DMA Arbiter PLL3
#define PLL_CLKSEL_DMA_ARBT_PLL14 (0x01)      //< Select DMA Arbiter PLL14
//@}


/*
    @name   DSP IOP clock rate value

    DSP IOP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP_IOP).
*/
//@{
#define PLL_CLKSEL_DSP_IOP_D2     (0x00 << PLL_CLKSEL_DSP_IOP)      //< Select DSP IOP D2
#define PLL_CLKSEL_DSP_IOP_D4     (0x01 << PLL_CLKSEL_DSP_IOP)      //< Select DSP IOP D4
#define PLL_CLKSEL_DSP_IOP_D6     (0x02 << PLL_CLKSEL_DSP_IOP)      //< Select DSP IOP D6
#define PLL_CLKSEL_DSP_IOP_D8     (0x03 << PLL_CLKSEL_DSP_IOP)      //< Select DSP IOP D8
//@}

/*
    @name   DSP EPP clock rate value

    DSP EPP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP_EPP).
*/
//@{
#define PLL_CLKSEL_DSP_EPP_D1     (0x00 << PLL_CLKSEL_DSP_EPP)      //< Select DSP EPP D1
#define PLL_CLKSEL_DSP_EPP_D2     (0x01 << PLL_CLKSEL_DSP_EPP)      //< Select DSP EPP D2
#define PLL_CLKSEL_DSP_EPP_D3     (0x02 << PLL_CLKSEL_DSP_EPP)      //< Select DSP EPP D3
//@}

/*
    @name   DSP EDP clock rate value

    DSP EDP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP_EDP).
*/
//@{
#define PLL_CLKSEL_DSP_EDP_D1     (0x00 << PLL_CLKSEL_DSP_EDP)      //< Select DSP EDP D1
#define PLL_CLKSEL_DSP_EDP_D2     (0x01 << PLL_CLKSEL_DSP_EDP)      //< Select DSP EDP D2
#define PLL_CLKSEL_DSP_EDP_D3     (0x02 << PLL_CLKSEL_DSP_EDP)      //< Select DSP EDP D3
//@}


/*
    @name   DSP EDAP clock rate value

    DSP EDAP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP_EDAP).
*/
//@{
#define PLL_CLKSEL_DSP_EDAP_D1     (0x00 << PLL_CLKSEL_DSP_EDAP)      //< Select DSP EDAP D1
#define PLL_CLKSEL_DSP_EDAP_D2     (0x01 << PLL_CLKSEL_DSP_EDAP)      //< Select DSP EDAP D2
#define PLL_CLKSEL_DSP_EDAP_D3     (0x02 << PLL_CLKSEL_DSP_EDAP)      //< Select DSP EDAP D3
#define PLL_CLKSEL_DSP_EDAP_D4     (0x03 << PLL_CLKSEL_DSP_EDAP)      //< Select DSP EDAP D4
//@}


/*
    @name   DSP2 IOP clock rate value

    DSP2 IOP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP2_IOP).
*/
//@{
#define PLL_CLKSEL_DSP2_IOP_D2     (0x00 << PLL_CLKSEL_DSP2_IOP)      //< Select DSP2 IOP D2
#define PLL_CLKSEL_DSP2_IOP_D4     (0x01 << PLL_CLKSEL_DSP2_IOP)      //< Select DSP2 IOP D4
#define PLL_CLKSEL_DSP2_IOP_D6     (0x02 << PLL_CLKSEL_DSP2_IOP)      //< Select DSP2 IOP D6
#define PLL_CLKSEL_DSP2_IOP_D8     (0x03 << PLL_CLKSEL_DSP2_IOP)      //< Select DSP2 IOP D8
//@}

/*
    @name   DSP2 EPP clock rate value

    DSP2 EPP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP2_EPP).
*/
//@{
#define PLL_CLKSEL_DSP2_EPP_D1     (0x00 << PLL_CLKSEL_DSP2_EPP)      //< Select DSP2 EPP D1
#define PLL_CLKSEL_DSP2_EPP_D2     (0x01 << PLL_CLKSEL_DSP2_EPP)      //< Select DSP2 EPP D2
#define PLL_CLKSEL_DSP2_EPP_D3     (0x02 << PLL_CLKSEL_DSP2_EPP)      //< Select DSP2 EPP D3
//@}

/*
    @name   DSP2 EDP clock rate value

    DSP2 EDP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP2_EDP).
*/
//@{
#define PLL_CLKSEL_DSP2_EDP_D1     (0x00 << PLL_CLKSEL_DSP2_EDP)      //< Select DSP2 EDP D1
#define PLL_CLKSEL_DSP2_EDP_D2     (0x01 << PLL_CLKSEL_DSP2_EDP)      //< Select DSP2 EDP D2
#define PLL_CLKSEL_DSP2_EDP_D3     (0x02 << PLL_CLKSEL_DSP2_EDP)      //< Select DSP2 EDP D3
//@}

/*
    @name   DSP2 EDAP clock rate value

    DSP2 EDAP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSP2_EDAP).
*/
//@{
#define PLL_CLKSEL_DSP2_EDAP_D1    (0x00 << PLL_CLKSEL_DSP2_EDAP)      //< Select DSP2 EDAP D1
#define PLL_CLKSEL_DSP2_EDAP_D2    (0x01 << PLL_CLKSEL_DSP2_EDAP)      //< Select DSP2 EDAP D2
#define PLL_CLKSEL_DSP2_EDAP_D3    (0x02 << PLL_CLKSEL_DSP2_EDAP)      //< Select DSP2 EDAP D3
#define PLL_CLKSEL_DSP2_EDAP_D4    (0x03 << PLL_CLKSEL_DSP2_EDAP)      //< Select DSP2 EDAP D4
//@}


/*
    @name   DMA clock rate value

    DMA clock rate value

    @note OBSOLETED. For backward compatible.
*/
//@{
//@}
#define PLL_CLKSEL_DMA_240        (0x04 << 0)                   //< Backward compatible
#define PLL_CLKSEL_DMA_160        (0x05 << 0)                   //< Backward compatible
#define PLL_CLKSEL_DMA_120        (0x06 << 0)                   //< Backward compatible
#define PLL_CLKSEL_DMA_96         (0x07 << 0)                   //< Backward compatible
#define PLL_CLKSEL_DMA_PLL3       (0x01 << 0)                   //< Backward compatible
#define PLL_CLKSEL_DMA_PLL2_D2    (0x02 << 0)                   //< Backward compatible
#define PLL_CLKSEL_DMA_PLL2_D3    (0x03 << 0)                   //< Backward compatible


/*
    @name   PRE clock rate value

    PRE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_PRE).
*/
//@{
//@}
#define PLL_CLKSEL_PRE_48         (0x00) //< Backward compatible
#define PLL_CLKSEL_PRE_60         (0x01) //< Backward compatible
#define PLL_CLKSEL_PRE_80         (0x02) //< Backward compatible
#define PLL_CLKSEL_PRE_120        (0x03) //< Backward compatible
#define PLL_CLKSEL_PRE_160        (0x04) //< Backward compatible
#define PLL_CLKSEL_PRE_192        (0x05) //< Backward compatible
#define PLL_CLKSEL_PRE_240        (0x06) //< Backward compatible
#define PLL_CLKSEL_PRE_PLL6       (0x07) //< Backward compatible
#define PLL_CLKSEL_PRE_96         (PLL_CLKSEL_PRE_80) //< Backward compatible


/*
    @name   SDE clock rate value

    SDE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SDE).
*/
//@{
#define PLL_CLKSEL_SDE_240        (0x00 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET)) //< Select SDE clock as 240MHz
#define PLL_CLKSEL_SDE_480        (0x01 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET)) //< Select SDE clock as 480MHz
#define PLL_CLKSEL_SDE_PLL6       (0x02 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET)) //< Select SDE clock as PLL6
#define PLL_CLKSEL_SDE_PLL13      (0x03 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET)) //< Select SDE clock as PLL13


/*
    @name   IPE clock rate value

    IPE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IPE).
*/
//@{
#define PLL_CLKSEL_IPE_240        (0x00 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as 240MHz
#define PLL_CLKSEL_IPE_480        (0x01 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as 480MHz
#define PLL_CLKSEL_IPE_PLL6       (0x02 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as PLL6
#define PLL_CLKSEL_IPE_PLL13      (0x03 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as PLL13
//@}
#define PLL_CLKSEL_IPE_48         (0x04 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IPE_60         (0x05 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IPE_80         (0x06 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IPE_120        (0x07 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IPE_160        (0x08 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IPE_192        (0x09 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible

/*
    @name   DIS clock rate value

    DIS clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DIS).
*/
//@{
#define PLL_CLKSEL_DIS_240        (0x00 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Select DIS clock as 240MHz
#define PLL_CLKSEL_DIS_480        (0x01 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Select DIS clock as 480MHz
#define PLL_CLKSEL_DIS_PLL6       (0x02 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Select DIS clock as PLL6
#define PLL_CLKSEL_DIS_PLL13      (0x03 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as PLL13
//@}
#define PLL_CLKSEL_DIS_48         (0x04 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DIS_60         (0x05 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DIS_80         (0x06 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DIS_120        (0x07 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DIS_160        (0x08 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DIS_192        (0x09 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible

/*
    @name   IME clock rate value

    IME clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IME).
*/
//@{
#define PLL_CLKSEL_IME_240        (0x00 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Select IME clock as 240MHz
#define PLL_CLKSEL_IME_480        (0x01 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Select IME clock as 480MHz
#define PLL_CLKSEL_IME_PLL6       (0x02 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Select IME clock as PLL6
#define PLL_CLKSEL_IME_PLL13      (0x03 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Select IME clock as PLL13
//@}
#define PLL_CLKSEL_IME_48         (0x04 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IME_60         (0x05 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IME_80         (0x06 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IME_120        (0x07 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IME_160        (0x08 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IME_192        (0x09 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible


/*
    @name   FDE clock rate value

    FDE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_FDE).
*/
//@{
#define PLL_CLKSEL_FDE_240        (0x00 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Select FDE clock as 240MHz
#define PLL_CLKSEL_FDE_480        (0x01 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Select FDE clock as 480MHz
#define PLL_CLKSEL_FDE_PLL6       (0x02 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Select FDE clock as PLL6
#define PLL_CLKSEL_FDE_PLL13      (0x03 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Select FDE clock as PLL13
//@}
#define PLL_CLKSEL_FDE_48         (0x04 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_FDE_60         (0x05 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_FDE_80         (0x06 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_FDE_120        (0x07 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_FDE_160        (0x08 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_FDE_192        (0x09 << (PLL_CLKSEL_FDE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible

/*
    @name   IVE clock rate value

    IVE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IVE).
*/
//@{
#define PLL_CLKSEL_IVE_240        (0x00 << (PLL_CLKSEL_IVE - PLL_CLKSEL_R1_OFFSET)) //< Select IVE clock as 240MHz
#define PLL_CLKSEL_IVE_480        (0x01 << (PLL_CLKSEL_IVE - PLL_CLKSEL_R1_OFFSET)) //< Select IVE clock as 480MHz
#define PLL_CLKSEL_IVE_PLL6       (0x02 << (PLL_CLKSEL_IVE - PLL_CLKSEL_R1_OFFSET)) //< Select IVE clock as PLL6
#define PLL_CLKSEL_IVE_PLL13      (0x03 << (PLL_CLKSEL_IVE - PLL_CLKSEL_R1_OFFSET)) //< Select IVE clock as PLL13
//@}


/*
    @name   ISE clock rate value

    ISE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ISE).
*/
//@{
#define PLL_CLKSEL_ISE_240        (0x00 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Select ISE clock as 240MHz
#define PLL_CLKSEL_ISE_480        (0x01 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Select ISE clock as 480MHz
#define PLL_CLKSEL_ISE_PLL6       (0x02 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Select ISE clock as PLL6
#define PLL_CLKSEL_ISE_PLL13      (0x03 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Select ISE clock as PLL13
//@}
#define PLL_CLKSEL_ISE_48         (0x04 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_ISE_60         (0x05 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_ISE_80         (0x06 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_ISE_120        (0x07 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_ISE_160        (0x08 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_ISE_192        (0x09 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible

/*
    @name   ISE2 clock rate value

    ISE2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ISE2).
*/
//@{
#define PLL_CLKSEL_ISE2_240       (0x00 << (PLL_CLKSEL_ISE2 - PLL_CLKSEL_R2_OFFSET)) //< Select ISE2 clock as 240MHz
#define PLL_CLKSEL_ISE2_480       (0x01 << (PLL_CLKSEL_ISE2 - PLL_CLKSEL_R2_OFFSET)) //< Select ISE2 clock as 480MHz
#define PLL_CLKSEL_ISE2_PLL6      (0x02 << (PLL_CLKSEL_ISE2 - PLL_CLKSEL_R2_OFFSET)) //< Select ISE2 clock as PLL6
#define PLL_CLKSEL_ISE2_PLL13     (0x03 << (PLL_CLKSEL_ISE2 - PLL_CLKSEL_R2_OFFSET)) //< Select ISE2 clock as PLL13


/*
    @name   DCE clock rate value

    DCE clock rate value

    This is for pll_set_clock_rate(PLL_CLKSEL_DCE).
*/
//@{
#define PLL_CLKSEL_DCE_240        (0x00 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Select DCE clock as 240MHz
#define PLL_CLKSEL_DCE_480        (0x01 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Select DCE clock as 480MHz
#define PLL_CLKSEL_DCE_PLL6       (0x02 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Select DCE clock as PLL6
#define PLL_CLKSEL_DCE_PLL13      (0x03 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Select DCE clock as PLL13
//@}
#define PLL_CLKSEL_DCE_48         (0x04 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DCE_60         (0x05 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DCE_80         (0x06 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DCE_120        (0x07 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DCE_160        (0x08 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_DCE_192        (0x09 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Backward compatible


/*
    @name   IFE clock rate value

    IFE clock rate value

    This is for pll_set_clock_rate(PLL_CLKSEL_IFE).
*/
//@{
#define PLL_CLKSEL_IFE_240        (0x00 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Select IFE clock as 240MHz
#define PLL_CLKSEL_IFE_480        (0x01 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Select IFE clock as 480MHz
#define PLL_CLKSEL_IFE_PLL6       (0x02 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Select IFE clock as PLL6
#define PLL_CLKSEL_IFE_PLL13      (0x03 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Select IFE clock as PLL13
//@}
#define PLL_CLKSEL_IFE_48         (0x04 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IFE_60         (0x05 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IFE_80         (0x06 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IFE_120        (0x07 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IFE_160        (0x08 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_IFE_192        (0x09 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Backward compatible

/*
    @name   IFE2 clock rate value

    IFE2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IFE2).
*/
//@{
#define PLL_CLKSEL_IFE2_240       (0x00 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Select IFE2 clock as 240MHz
#define PLL_CLKSEL_IFE2_480       (0x01 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Select IFE2 clock as 480MHz
#define PLL_CLKSEL_IFE2_PLL6      (0x02 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Select IFE2 clock as PLL6
#define PLL_CLKSEL_IFE2_PLL13     (0x03 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Select IFE2 clock as PLL13
//@}
#define PLL_CLKSEL_IFE2_48        (0x04 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_IFE2_60        (0x05 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_IFE2_80        (0x06 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_IFE2_120       (0x07 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_IFE2_160       (0x08 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_IFE2_192       (0x09 << (PLL_CLKSEL_IFE2 - PLL_CLKSEL_R2_OFFSET))  //< Backward compatible


/*
    @name   SVM clock rate value

    SVM clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SVM).
*/
//@{
#define PLL_CLKSEL_SVM_240       (0x00 << (PLL_CLKSEL_SVM - PLL_CLKSEL_R2_OFFSET))  //< Select IFE2 clock as 240MHz
#define PLL_CLKSEL_SVM_480       (0x01 << (PLL_CLKSEL_SVM - PLL_CLKSEL_R2_OFFSET))  //< Select IFE2 clock as 480MHz
#define PLL_CLKSEL_SVM_PLL6      (0x02 << (PLL_CLKSEL_SVM - PLL_CLKSEL_R2_OFFSET))  //< Select IFE2 clock as PLL6
#define PLL_CLKSEL_SVM_PLL13     (0x03 << (PLL_CLKSEL_SVM - PLL_CLKSEL_R2_OFFSET))  //< Select IFE2 clock as PLL13
//@}

/*
    @name   RHE clock rate value

    RHE clock rate value

    This is for pll_set_clock_rate(PLL_CLKSEL_RHE).
*/
//@{
#define PLL_CLKSEL_RHE_240        (0x00 << (PLL_CLKSEL_RHE - PLL_CLKSEL_R2_OFFSET)) //< Select RHE clock as 240MHz
#define PLL_CLKSEL_RHE_480        (0x01 << (PLL_CLKSEL_RHE - PLL_CLKSEL_R2_OFFSET)) //< Select RHE clock as 480MHz
#define PLL_CLKSEL_RHE_PLL6       (0x02 << (PLL_CLKSEL_RHE - PLL_CLKSEL_R2_OFFSET)) //< Select RHE clock as PLL6
#define PLL_CLKSEL_RHE_PLL13      (0x03 << (PLL_CLKSEL_RHE - PLL_CLKSEL_R2_OFFSET)) //< Select RHE clock as PLL13
//@}
#define PLL_CLKSEL_RHE_192        (0x04 << (PLL_CLKSEL_RHE - PLL_CLKSEL_R2_OFFSET)) //< Backward compatible

/*
    @name   CNN clock rate value

    CNN clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CNN).
*/
//@{
#define PLL_CLKSEL_CNN_240       (0x00 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET))  //< Select CNN clock as 240MHz
#define PLL_CLKSEL_CNN_480       (0x01 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET))  //< Select CNN clock as 480MHz
#define PLL_CLKSEL_CNN_PLL6      (0x02 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET))  //< Select CNN clock as PLL6
#define PLL_CLKSEL_CNN_PLL13     (0x03 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET))  //< Select CNN clock as PLL13
//@}


/*
    @name   RDE clock rate value

    RDE clock rate value

    This is for pll_set_clock_rate(PLL_CLKSEL_RDE).
*/
//@{
//@}
#define PLL_CLKSEL_RDE_120        (0x00) //< Backward compatible
#define PLL_CLKSEL_RDE_160        (0x01) //< Backward compatible
#define PLL_CLKSEL_RDE_192        (0x02) //< Backward compatible
#define PLL_CLKSEL_RDE_240        (0x03) //< Backward compatible
#define PLL_CLKSEL_RDE_48         (0x04) //< Backward compatible
#define PLL_CLKSEL_RDE_60         (0x05) //< Backward compatible
#define PLL_CLKSEL_RDE_80         (0x06) //< Backward compatible
#define PLL_CLKSEL_RDE_PLL6       (0x07) //< Backward compatible
#define PLL_CLKSEL_RDE_PLL13      (0x08) //< Backward compatible


/*
    @name   RHE clock rate value

    RHE clock rate value

    This is for pll_set_clock_rate(PLL_CLKSEL_RHE_CLK2).
*/
//@{
//@}
#define PLL_CLKSEL_RHE_CLK2_120        (0x00) //< Backward compatible
#define PLL_CLKSEL_RHE_CLK2_160        (0x01) //< Backward compatible
#define PLL_CLKSEL_RHE_CLK2_192        (0x02) //< Backward compatible
#define PLL_CLKSEL_RHE_CLK2_240        (0x03) //< Backward compatible

/*
    @name   DRE clock rate value

    DRE clock rate value

    This is for pll_set_clock_rate(PLL_CLKSEL_DRE).
*/
//@{
#define PLL_CLKSEL_DRE_240        (0x00 << (PLL_CLKSEL_DRE - PLL_CLKSEL_R2_OFFSET)) //< Select DRE clock as 240MHz
#define PLL_CLKSEL_DRE_480        (0x01 << (PLL_CLKSEL_DRE - PLL_CLKSEL_R2_OFFSET)) //< Select DRE clock as 480MHz
#define PLL_CLKSEL_DRE_PLL6       (0x02 << (PLL_CLKSEL_DRE - PLL_CLKSEL_R2_OFFSET)) //< Select DRE clock as PLL6
#define PLL_CLKSEL_DRE_PLL13      (0x03 << (PLL_CLKSEL_DRE - PLL_CLKSEL_R2_OFFSET)) //< Select DRE clock as PLL13
//@}
#define PLL_CLKSEL_DRE_192        (0x04 << (PLL_CLKSEL_DRE - PLL_CLKSEL_R2_OFFSET)) //< Select DRE clock as 192MHz


/*
    @name   JPEG clock rate value

    JPEG clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_JPEG).
*/
//@{
#define PLL_CLKSEL_JPEG_240       (0x00 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Select JPEG clock as 240MHz
#define PLL_CLKSEL_JPEG_480       (0x01 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Select JPEG clock as 480MHz
#define PLL_CLKSEL_JPEG_PLL6      (0x02 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Select JPEG clock as PLL6
#define PLL_CLKSEL_JPEG_PLL13     (0x03 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Select JPEG clock as PLL13
//@}
#define PLL_CLKSEL_JPEG_80        (0x04 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_JPEG_120       (0x05 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_JPEG_160       (0x06 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible

/*
    @name H264 clock rate value

    H264 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_H264).
*/
//@{
#define PLL_CLKSEL_H26X_192       (0x00 << (PLL_CLKSEL_H26X - PLL_CLKSEL_R3_OFFSET)) //< Select H.264 clock as 192MHz
#define PLL_CLKSEL_H26X_240       (0x01 << (PLL_CLKSEL_H26X - PLL_CLKSEL_R3_OFFSET)) //< Select H.264 clock as 240MHz
#define PLL_CLKSEL_H26X_PLL6      (0x02 << (PLL_CLKSEL_H26X - PLL_CLKSEL_R3_OFFSET)) //< Select H.264 clock as PLL6 (for IPP)
#define PLL_CLKSEL_H26X_PLL13     (0x03 << (PLL_CLKSEL_H26X - PLL_CLKSEL_R3_OFFSET)) //< Select H.264 clock as PLL13 (for IPP)
//@}
#define PLL_CLKSEL_H26X_120       (0x04 << (PLL_CLKSEL_H26X - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_H26X_160       (0x05 << (PLL_CLKSEL_H26X - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible

/*
    @name   Graphic clock rate value

    Graphic clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_GRAPHIC).
*/
//@{
#define PLL_CLKSEL_GRAPHIC_240    (0x00 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Select Graphic clock as 240Mhz
#define PLL_CLKSEL_GRAPHIC_480    (0x01 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Select Graphic clock as 480Mhz
#define PLL_CLKSEL_GRAPHIC_PLL6   (0x02 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Select Graphic clock as PLL6
#define PLL_CLKSEL_GRAPHIC_PLL13  (0x03 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Select Graphic clock as PLL13
//@}
#define PLL_CLKSEL_GRAPHIC_80     (0x04 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_GRAPHIC_120    (0x05 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_GRAPHIC_160    (0x06 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_GRAPHIC_192    (0x07 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Backward compatible

/*
    @name   Graphic2 clock rate value

    Graphic2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_GRAPHIC2).
*/
//@{
#define PLL_CLKSEL_GRAPHIC2_240   (0x00 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Select Graphic2 clock as 240Mhz
#define PLL_CLKSEL_GRAPHIC2_480   (0x01 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Select Graphic2 clock as 480Mhz
#define PLL_CLKSEL_GRAPHIC2_PLL6  (0x02 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Select Graphic2 clock as PLL6
#define PLL_CLKSEL_GRAPHIC2_PLL13 (0x03 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Select Graphic2 clock as PLL13
//@}
#define PLL_CLKSEL_GRAPHIC2_80    (0x04 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_GRAPHIC2_120   (0x05 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_GRAPHIC2_160   (0x06 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_GRAPHIC2_192   (0x07 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Backward compatible

/*
    @name   Affine clock rate value

    Affine clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_AFFINE).
*/
//@{
#define PLL_CLKSEL_AFFINE_160   (0x00 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET))    //< Select Affine clock as 160Mhz
#define PLL_CLKSEL_AFFINE_240   (0x01 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET))    //< Select Affine clock as 240Mhz
#define PLL_CLKSEL_AFFINE_PLL6  (0x02 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET))    //< Select Affine clock as PLL6
#define PLL_CLKSEL_AFFINE_PLL13 (0x03 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET))    //< Select Affine clock as PLL13
//@}
#define PLL_CLKSEL_AFFINE_80    (0x04 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_AFFINE_120   (0x05 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_AFFINE_192   (0x07 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET))    //< Backward compatible

/*
    @name   HWCPY clock rate value

    HWCPY clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_HWCPY).
*/
//@{
#define PLL_CLKSEL_HWCPY_240   (0x00 << (PLL_CLKSEL_HWCPY - PLL_CLKSEL_R3_OFFSET))    //< Select HWCPY clock as 240Mhz
#define PLL_CLKSEL_HWCPY_480   (0x01 << (PLL_CLKSEL_HWCPY - PLL_CLKSEL_R3_OFFSET))    //< Select HWCPY clock as 480Mhz
#define PLL_CLKSEL_HWCPY_PLL6  (0x02 << (PLL_CLKSEL_HWCPY - PLL_CLKSEL_R3_OFFSET))    //< Select HWCPY clock as PLL6
#define PLL_CLKSEL_HWCPY_PLL13 (0x03 << (PLL_CLKSEL_HWCPY - PLL_CLKSEL_R3_OFFSET))    //< Select HWCPY clock as PLL13
//@}

/*
    @name   ROTATE clock rate value

    ROTATE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ROTATE).
*/
//@{
#define PLL_CLKSEL_ROTATE_240   (0x00 << (PLL_CLKSEL_ROTATE - PLL_CLKSEL_R3_OFFSET))    //< Select ROTATE clock as 240Mhz
#define PLL_CLKSEL_ROTATE_480   (0x01 << (PLL_CLKSEL_ROTATE - PLL_CLKSEL_R3_OFFSET))    //< Select ROTATE clock as 480Mhz
#define PLL_CLKSEL_ROTATE_PLL6  (0x02 << (PLL_CLKSEL_ROTATE - PLL_CLKSEL_R3_OFFSET))    //< Select ROTATE clock as PLL6
#define PLL_CLKSEL_ROTATE_PLL13 (0x03 << (PLL_CLKSEL_ROTATE - PLL_CLKSEL_R3_OFFSET))    //< Select ROTATE clock as PLL13
//@}


/*
    @name   Crypto clock rate value

    Crypto clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CRYPTO).
*/
//@{
#define PLL_CLKSEL_CRYPTO_240   (0x00 << (PLL_CLKSEL_CRYPTO - PLL_CLKSEL_R3_OFFSET))    //< Select Crypto clock as 2400Mhz
#define PLL_CLKSEL_CRYPTO_480   (0x01 << (PLL_CLKSEL_CRYPTO - PLL_CLKSEL_R3_OFFSET))    //< Select Crypto clock as 480Mhz
//@}
#define PLL_CLKSEL_CRYPTO_80    (0x02 << (PLL_CLKSEL_CRYPTO - PLL_CLKSEL_R3_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_CRYPTO_160   (0x03 << (PLL_CLKSEL_CRYPTO - PLL_CLKSEL_R3_OFFSET))    //< Backward compatible



/*
    @name   VX1 sub-link clock rate value

    VX1 sub-link clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_VX1SL).
*/
//@{
#define PLL_CLKSEL_VX1SL_80    (0x00 << (PLL_CLKSEL_VX1SL - PLL_CLKSEL_R24_OFFSET))    //< Select VX1 sub-link clock as 80Mhz
//@}
#define PLL_CLKSEL_VX1SL_40    (0x01 << (PLL_CLKSEL_VX1SL - PLL_CLKSEL_R24_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_VX1SL_48    (0x02 << (PLL_CLKSEL_VX1SL - PLL_CLKSEL_R24_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_VX1SL_60    (0x03 << (PLL_CLKSEL_VX1SL - PLL_CLKSEL_R24_OFFSET))    //< Backward compatible

/*
    @name   VX1 pixel clock invert value

    VX1 pixel clock invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_VX1_PIXCLK_INV).
*/
//@{
#define PLL_CLKSEL_VX1_PIXCLKINV_DIS  (0x00 << (PLL_CLKSEL_VX1_PIXCLK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select VX1 PIXCLK not invert
#define PLL_CLKSEL_VX1_PIXCLKINV_EN   (0x01 << (PLL_CLKSEL_VX1_PIXCLK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select VX1 PIXCLK invert
//@}

/*
    @name   VX1 pixel2 clock invert value

    VX1 pixel2 clock invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_VX1_PIXCLK2_INV).
*/
//@{
#define PLL_CLKSEL_VX1_PIXCLK2INV_DIS  (0x00 << (PLL_CLKSEL_VX1_PIXCLK2_INV - PLL_CLKSEL_R5_OFFSET))  //< Select VX1 PIXCLK2 not invert
#define PLL_CLKSEL_VX1_PIXCLK2INV_EN   (0x01 << (PLL_CLKSEL_VX1_PIXCLK2_INV - PLL_CLKSEL_R5_OFFSET))  //< Select VX1 PIXCLK2 invert
//@}

/*
    @name   VX12 pixel clock invert value

    VX12 pixel clock invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_VX12_PIXCLK_INV).
*/
//@{
#define PLL_CLKSEL_VX12_PIXCLKINV_DIS  (0x00 << (PLL_CLKSEL_VX12_PIXCLK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select VX12 PIXCLK not invert
#define PLL_CLKSEL_VX12_PIXCLKINV_EN   (0x01 << (PLL_CLKSEL_VX12_PIXCLK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select VX12 PIXCLK invert
//@}

/*
    @name   VX12 pixel2 clock invert value

    VX12 pixel2 clock invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_VX12_PIXCLK2_INV).
*/
//@{
#define PLL_CLKSEL_VX12_PIXCLK2INV_DIS  (0x00 << (PLL_CLKSEL_VX12_PIXCLK2_INV - PLL_CLKSEL_R5_OFFSET))  //< Select VX12 PIXCLK2 not invert
#define PLL_CLKSEL_VX12_PIXCLK2INV_EN   (0x01 << (PLL_CLKSEL_VX12_PIXCLK2_INV - PLL_CLKSEL_R5_OFFSET))  //< Select VX12 PIXCLK2 invert
//@}



/*
    @name MIPI/LVDS clock rate value

    MIPI/LVDS clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS clock as 120Mhz
//@}
#define PLL_CLKSEL_MIPI_LVDS_80  (0x00 << (PLL_CLKSEL_MIPI_LVDS - PLL_CLKSEL_R6_OFFSET))     //< Backward compatible


/*
    @name MIPI/LVDS2 clock rate value

    MIPI/LVDS2 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS2).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS2_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS2 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS2 clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS2_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS2 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS2 clock as 120Mhz
//@}

/*
    @name MIPI/LVDS3 clock rate value

    MIPI/LVDS3 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS3).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS3_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS3 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS3 clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS3_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS3 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS3 clock as 120Mhz
//@}

/*
    @name MIPI/LVDS4 clock rate value

    MIPI/LVDS4 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS4).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS4_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS4 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS4 clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS4_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS4 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS4 clock as 120Mhz
//@}

/*
    @name MIPI/LVDS5 clock rate value

    MIPI/LVDS5 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS5).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS5_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS5 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS5 clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS5_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS5 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS5 clock as 120Mhz
//@}

/*
    @name MIPI/LVDS6 clock rate value

    MIPI/LVDS6 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS6).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS6_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS6 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS6 clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS6_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS6 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS6 clock as 120Mhz
//@}

/*
    @name MIPI/LVDS7 clock rate value

    MIPI/LVDS7 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS7).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS7_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS7 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS7 clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS7_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS7 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS7 clock as 120Mhz
//@}

/*
    @name MIPI/LVDS8 clock rate value

    MIPI/LVDS8 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS8).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS8_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS8 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS8 clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS8_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS8 - PLL_CLKSEL_R6_OFFSET))     //< Select MIPI/LVDS8 clock as 120Mhz
//@}



/*
    @name   NAND clock rate value

    NAND clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_NAND).
*/
//@{
#define PLL_CLKSEL_NAND_48          (0x00)  //< Backward compatible
#define PLL_CLKSEL_NAND_60          (0x01)  //< Backward compatible
#define PLL_CLKSEL_NAND_96          (0x02)  //< Backward compatible
#define PLL_CLKSEL_NAND_PLL4        (0x03)  //< Backward compatible
//@}

/*
    @name   SDIO clock rate value

    SDIO clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SDIO).
*/
//@{
#define PLL_CLKSEL_SDIO_192         (0x00 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Select SDIO clock as 192Mhz
#define PLL_CLKSEL_SDIO_480         (0x01 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Select SDIO clock as 480Mhz
#define PLL_CLKSEL_SDIO_PLL2        (0x02 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Select SDIO clock as PLL2
#define PLL_CLKSEL_SDIO_PLL4        (0x03 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Select SDIO clock as PLL4
//@}
#define PLL_CLKSEL_SDIO_48          (0x04 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_SDIO_60          (0x05 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_SDIO_80          (0x06 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_SDIO_96          (0x07 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Backward compatible

/*
    @name   SDIO2 clock rate value

    SDIO2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SDIO2).
*/
//@{
#define PLL_CLKSEL_SDIO2_192        (0x00 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Select SDIO2 clock as 192Mhz
#define PLL_CLKSEL_SDIO2_480        (0x01 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Select SDIO2 clock as 480Mhz
#define PLL_CLKSEL_SDIO2_PLL2       (0x02 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Select SDIO2 clock as PLL2
#define PLL_CLKSEL_SDIO2_PLL4       (0x03 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Select SDIO2 clock as PLL4
//@}
#define PLL_CLKSEL_SDIO2_40         (0x04 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SDIO2_48         (0x05 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SDIO2_60         (0x06 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SDIO2_80         (0x07 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SDIO2_96         (0x08 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Backward compatible

/*
    @name   SDIO3 clock rate value

    SDIO3 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SDIO3).
*/
//@{
#define PLL_CLKSEL_SDIO3_192        (0x00 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Select SDIO3 clock as 192Mhz
#define PLL_CLKSEL_SDIO3_480        (0x01 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Select SDIO3 clock as 480Mhz
#define PLL_CLKSEL_SDIO3_PLL2       (0x02 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Select SDIO3 clock as PLL2
#define PLL_CLKSEL_SDIO3_PLL4       (0x03 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Select SDIO3 clock as PLL4
//@}
#define PLL_CLKSEL_SDIO3_60         (0x04 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SDIO3_80         (0x05 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SDIO3_96         (0x06 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Backward compatible

/*
    @name   TSMUX clock rate value

    TSMUX clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TSMUX).
*/
//@{
#define PLL_CLKSEL_TSMUX_240         (0x01 << (PLL_CLKSEL_TSMUX - PLL_CLKSEL_R5_OFFSET))  //< Select TSMUX clock as 240Mhz
#define PLL_CLKSEL_TSMUX_480         (0x02 << (PLL_CLKSEL_TSMUX - PLL_CLKSEL_R5_OFFSET))  //< Select TSMUX clock as 480Mhz
//@}
#define PLL_CLKSEL_TSMUX_80          (0x04 << (PLL_CLKSEL_TSMUX - PLL_CLKSEL_R5_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_TSMUX_PLL12       (0x05 << (PLL_CLKSEL_TSMUX - PLL_CLKSEL_R5_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_TSMUX_160         (0x06 << (PLL_CLKSEL_TSMUX - PLL_CLKSEL_R5_OFFSET))  //< Backward compatible



/*
    @name   SP clock rate value

    SP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SP).
*/
//@{
#define PLL_CLKSEL_SP_480            (0x00 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Select SP clock as 240Mhz
#define PLL_CLKSEL_SP_PLL4           (0x01 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Select SP clock as PLL4
#define PLL_CLKSEL_SP_PLL10          (0x02 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Select SP clock as PLL10
#define PLL_CLKSEL_SP_PLL14          (0x03 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Select SP clock as PLL14

//@}
#define PLL_CLKSEL_SP_240            (0x04 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_SP_PLL2           (0x05 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_SP_PLL5           (0x06 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Backward compatible

/*
    @name   SP2 clock rate value

    SP2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SP2).
*/
//@{
#define PLL_CLKSEL_SP2_480           (0x00 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET))  //< Select SP2 clock as 240Mhz
#define PLL_CLKSEL_SP2_PLL4          (0x01 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET))  //< Select SP2 clock as PLL4
#define PLL_CLKSEL_SP2_PLL10         (0x02 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET))  //< Select SP2 clock as PLL10
#define PLL_CLKSEL_SP2_PLL14         (0x03 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET))  //< Select SP2 clock as PLL14
//@}
#define PLL_CLKSEL_SP2_240           (0x04 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SP2_PLL2          (0x05 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SP2_PLL5          (0x06 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET)) //< Select SP2 clock as PLL5


/*
    @name   CANBUS clock rate value

    CANBUS clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CANBUS).
*/
//@{
#define PLL_CLKSEL_CANBUS_20         (0x00 << (PLL_CLKSEL_CANBUS - PLL_CLKSEL_R5_OFFSET))  //< Select CANBUS clock as 20Mhz
#define PLL_CLKSEL_CANBUS_40         (0x01 << (PLL_CLKSEL_CANBUS - PLL_CLKSEL_R5_OFFSET))  //< Select CANBUS clock as 40Mhz
#define PLL_CLKSEL_CANBUS_80         (0x02 << (PLL_CLKSEL_CANBUS - PLL_CLKSEL_R5_OFFSET))  //< Select CANBUS clock as 80Mhz
//@}


/*
    @name   SIE MCLK clock source value

    SIE MCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE_MCLKSRC_480  (0x00 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE MCLK clock source as 480 MHz (PLL1)
#define PLL_CLKSEL_SIE_MCLKSRC_PLL4 (0x01 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE MCLK clock source as PLL4
#define PLL_CLKSEL_SIE_MCLKSRC_PLL5 (0x02 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE MCLK clock source as PLL5
#define PLL_CLKSEL_SIE_MCLKSRC_PLL10 (0x03 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE MCLK clock source as PLL10
//@}
#define PLL_CLKSEL_SIE_MCLKSRC_PLL2_D2  (0x03 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R7_OFFSET))  //< Backward compatible

/*
    @name   SIE MCLK2 clock source value

    SIE MCLK2 clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC).
*/
//@{
#define PLL_CLKSEL_SIE_MCLK2SRC_480  (0x00 << (PLL_CLKSEL_SIE_MCLK2SRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE MCLK2 clock source as 480 MHz
#define PLL_CLKSEL_SIE_MCLK2SRC_PLL4 (0x01 << (PLL_CLKSEL_SIE_MCLK2SRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE MCLK2 clock source as PLL4
#define PLL_CLKSEL_SIE_MCLK2SRC_PLL5 (0x02 << (PLL_CLKSEL_SIE_MCLK2SRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE MCLK2 clock source as PLL5
#define PLL_CLKSEL_SIE_MCLK2SRC_PLL10 (0x03 << (PLL_CLKSEL_SIE_MCLK2SRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE MCLK2 clock source as PLL10
//@}

/*
    @name   TGE PLL CLK clock source value

    TGE PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TGE_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_TGE_PXCLKSRC_PXCLKPAD    (0x00 << (PLL_CLKSEL_TGE - PLL_CLKSEL_R2_OFFSET))  //< Select TGE PX CLK source as PXCLK PAD
#define PLL_CLKSEL_TGE_PXCLKSRC_MCLK        (0x01 << (PLL_CLKSEL_TGE - PLL_CLKSEL_R2_OFFSET))  //< Select TGE PX CLK source as PLL_CLKSEL_SIE_MCLKSRC / (PLL_SIE_MCLKDIV+1)
#define PLL_CLKSEL_TGE_PXCLKSRC_MCLK2       (0x02 << (PLL_CLKSEL_TGE - PLL_CLKSEL_R2_OFFSET))  //< Select TGE PX CLK source as PLL_CLKSEL_SIE_MCLKSRC2 / (PLL_SIE_MCLKDIV+1)
//@}

/*
    @name   TGE2 PLL CLK clock source value

    TGE2 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TGE_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_TGE2_PXCLKSRC_PXCLKPAD    (0x00 << (PLL_CLKSEL_TGE2 - PLL_CLKSEL_R2_OFFSET))  //< Select TGE2 PX CLK source as PXCLK PAD
#define PLL_CLKSEL_TGE2_PXCLKSRC_MCLK        (0x01 << (PLL_CLKSEL_TGE2 - PLL_CLKSEL_R2_OFFSET))  //< Select TGE2 PX CLK source as PLL_CLKSEL_SIE_MCLKSRC / (PLL_SIE_MCLKDIV+1)
#define PLL_CLKSEL_TGE2_PXCLKSRC_MCLK2       (0x02 << (PLL_CLKSEL_TGE2 - PLL_CLKSEL_R2_OFFSET))  //< Select TGE2 PX CLK source as PLL_CLKSEL_SIE_MCLKSRC2 / (PLL_SIE_MCLKDIV+1)
//@}


/*
    @name   SIE IO PXCLK clock source value

    SIE IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_IO_PXCLK).
*/
//@{
#define PLL_CLKSEL_SIE_IO_PXCLK_PXCLK    (0x00 << (PLL_CLKSEL_SIE_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE PX CLK source as PXCLK PAD
//@}
#define PLL_CLKSEL_SIE_IO_PXCLK_PXCLK3   (0x00 << (PLL_CLKSEL_SIE_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Backward compatible


/*
    @name   SIE2 IO PXCLK clock source value

    SIE2 IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE2_IO_PXCLK).
*/
//@{
#define PLL_CLKSEL_SIE2_IO_PXCLK_PXCLK2  (0x00 << (PLL_CLKSEL_SIE2_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE2 PX CLK source as PXCLK2 PAD
#define PLL_CLKSEL_SIE2_IO_PXCLK_PXCLK1  (0x01 << (PLL_CLKSEL_SIE2_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE2 PX CLK source as PXCLK1 PAD
//@}
#define PLL_CLKSEL_SIE2_IO_PXCLK_PXCLK3  (0x01 << (PLL_CLKSEL_SIE2_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Backward compatible


/*
    @name   SIE4 IO PXCLK clock source value

    SIE4 IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE4_IO_PXCLK).
*/
//@{
#define PLL_CLKSEL_SIE4_IO_PXCLK_PXCLK4  (0x00 << (PLL_CLKSEL_SIE4_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE4 PX CLK source as PXCLK4 PAD
#define PLL_CLKSEL_SIE4_IO_PXCLK_PXCLK3  (0x01 << (PLL_CLKSEL_SIE4_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE4 PX CLK source as PXCLK3 PAD
//@}


/*
    @name   SIE5 IO PXCLK clock source value

    SIE5 IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE5_IO_PXCLK).
*/
//@{
#define PLL_CLKSEL_SIE5_IO_PXCLK_PXCLK5  (0x00 << (PLL_CLKSEL_SIE5_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE5 PX CLK source as PXCLK5 PAD
#define PLL_CLKSEL_SIE5_IO_PXCLK_PXCLK6  (0x01 << (PLL_CLKSEL_SIE5_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE5 PX CLK source as PXCLK6 PAD
//@}

/*
    @name   SIE7 IO PXCLK clock source value

    SIE7 IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE7_IO_PXCLK).
*/
//@{
#define PLL_CLKSEL_SIE7_IO_PXCLK_PXCLK7  (0x00 << (PLL_CLKSEL_SIE7_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE7 PX CLK source as PXCLK7 PAD
//@}
#define PLL_CLKSEL_SIE7_IO_PXCLK_PXCLK6  (0x00 << (PLL_CLKSEL_SIE7_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Backward compatible


/*
    @name   SIE8 IO PXCLK clock source value

    SIE8 IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE8_IO_PXCLK).
*/
//@{
#define PLL_CLKSEL_SIE8_IO_PXCLK_PXCLK8  (0x00 << (PLL_CLKSEL_SIE8_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE8 PX CLK source as PXCLK8 PAD
#define PLL_CLKSEL_SIE8_IO_PXCLK_PXCLK7  (0x01 << (PLL_CLKSEL_SIE8_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Select SIE8 PX CLK source as PXCLK7 PAD
//@}
#define PLL_CLKSEL_SIE8_IO_PXCLK_PXCLK6  (0x01 << (PLL_CLKSEL_SIE8_IO_PXCLK - PLL_CLKSEL_R6_OFFSET))  //< Backward compatible



/*
    @name   SIE PLL CLK clock source value

    SIE PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE_PXCLKSRC_PXCLKPAD    (0x00 << (PLL_CLKSEL_SIE_PXCLKSRC - PLL_CLKSEL_R7_OFFSET))  //< Select SIE PX CLK source as PXCLK PAD
#define PLL_CLKSEL_SIE_PXCLKSRC_MCLK        (0x01 << (PLL_CLKSEL_SIE_PXCLKSRC - PLL_CLKSEL_R7_OFFSET))  //< Select SIE PX CLK source as PLL_CLKSEL_SIE_MCLKSRC / (PLL_SIE_MCLKDIV+1)
//@}

/*
    @name   SIE2 PLL CLK clock source value

    SIE2 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE2_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE2_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE2_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE2 PX CLK source as PXCLK2 PAD
#define PLL_CLKSEL_SIE2_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE2_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE2 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)
//@}
#define PLL_CLKSEL_SIE2_VX1_1X              (0x02 << (PLL_CLKSEL_SIE2_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Backward compatible
#define PLL_CLKSEL_SIE2_VX1_2X              (0x03 << (PLL_CLKSEL_SIE2_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Backward compatible

/*
    @name   SIE3 PLL CLK clock source value

    SIE3 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE3_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE3_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE3_PXCLKSRC - PLL_CLKSEL_R7_OFFSET))  //< Select SIE3 PX CLK source as PXCLK3 PAD
#define PLL_CLKSEL_SIE3_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE3_PXCLKSRC - PLL_CLKSEL_R7_OFFSET))  //< Select SIE3 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)
#define PLL_CLKSEL_SIE3_VX1_1X              (0x02 << (PLL_CLKSEL_SIE3_PXCLKSRC - PLL_CLKSEL_R7_OFFSET))  //< Select SIE3 PX CLK source as VX1 1X pixel clock
#define PLL_CLKSEL_SIE3_VX1_2X              (0x03 << (PLL_CLKSEL_SIE3_PXCLKSRC - PLL_CLKSEL_R7_OFFSET))  //< Select SIE3 PX CLK source as VX1 2X pixel clock
//@}

/*
    @name   SIE4 PLL CLK clock source value

    SIE4 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE4_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE4_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE4_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE4 PX CLK source as PXCLK2 PAD
#define PLL_CLKSEL_SIE4_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE4_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE4 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)
#define PLL_CLKSEL_SIE4_VX1_1X              (0x02 << (PLL_CLKSEL_SIE4_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE4 PX CLK source as VX1 1X pixel clock
#define PLL_CLKSEL_SIE4_VX1_2X              (0x03 << (PLL_CLKSEL_SIE4_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE4 PX CLK source as VX1 2X pixel clock
//@}

/*
    @name   SIE5 PLL CLK clock source value

    SIE5 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE5_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE5_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE5_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE5 PX CLK source as PXCLK2 PAD
#define PLL_CLKSEL_SIE5_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE5_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE5 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)
#define PLL_CLKSEL_SIE5_VX1_1X              (0x02 << (PLL_CLKSEL_SIE5_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE5 PX CLK source as VX1 1X pixel clock
#define PLL_CLKSEL_SIE5_VX1_2X              (0x03 << (PLL_CLKSEL_SIE5_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE5 PX CLK source as VX1 2X pixel clock
//@}

/*
    @name   SIE6 PLL CLK clock source value

    SIE6 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE6_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE6_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE6_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE6 PX CLK source as PXCLK2 PAD
#define PLL_CLKSEL_SIE6_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE6_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE6 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)
#define PLL_CLKSEL_SIE6_VX1_1X              (0x02 << (PLL_CLKSEL_SIE6_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE6 PX CLK source as VX1 1X pixel clock
#define PLL_CLKSEL_SIE6_VX1_2X              (0x03 << (PLL_CLKSEL_SIE6_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE6 PX CLK source as VX1 2X pixel clock
//@}

/*
    @name   SIE7 PLL CLK clock source value

    SIE7 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE7_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE7_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE7_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE7 PX CLK source as PXCLK2 PAD
#define PLL_CLKSEL_SIE7_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE7_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE7 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)

/*
    @name   SIE8 PLL CLK clock source value

    SIE8 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE8_PLLCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE8_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE8_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE8 PX CLK source as PXCLK2 PAD
#define PLL_CLKSEL_SIE8_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE8_PXCLKSRC - PLL_CLKSEL_R7_OFFSET)) //< Select SIE8 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)


/*
    @name   SIE CLK clock source value

    SIE CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_CLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE_CLKSRC_480       (0x00 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Select SIE CLK clock source as 480 MHz
#define PLL_CLKSEL_SIE_CLKSRC_PLL2      (0x01 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Select SIE CLK clock source as PLL2
#define PLL_CLKSEL_SIE_CLKSRC_PLL5      (0x02 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Select SIE CLK clock source as PLL5
#define PLL_CLKSEL_SIE_CLKSRC_PLL14     (0x03 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Select SIE CLK clock source as PLL14
//@}
#define PLL_CLKSEL_SIE_CLKSRC_PLL6      (0x04 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_SIE_CLKSRC_PLL13     (0x05 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_SIE_CLKSRC_PLL7      (0x06 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_SIE_CLKSRC_PLL12     (0x07 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_SIE_CLKSRC_192       (0x08 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R7_OFFSET))    //< Backward compatible

/*
    @name   SIE2 CLK clock source value

    SIE2 CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE2_CLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE2_CLKSRC_480      (0x00 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE2 CLK clock source as 480 MHz
#define PLL_CLKSEL_SIE2_CLKSRC_PLL2     (0x01 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE2 CLK clock source as PLL2
#define PLL_CLKSEL_SIE2_CLKSRC_PLL5     (0x02 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE2 CLK clock source as PLL5
#define PLL_CLKSEL_SIE2_CLKSRC_PLL14    (0x03 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE2 CLK clock source as PLL14
//@}
#define PLL_CLKSEL_SIE2_CLKSRC_PLL6     (0x01 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_SIE2_CLKSRC_PLL13    (0x03 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_SIE2_CLKSRC_PLL7     (0x04 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_SIE2_CLKSRC_PLL12    (0x05 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_SIE2_CLKSRC_192      (0x06 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Backward compatible

/*
    @name   SIE3 CLK clock source value

    SIE3 CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE3_CLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE3_CLKSRC_480      (0x00 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE3 CLK clock source as 480 MHz
#define PLL_CLKSEL_SIE3_CLKSRC_PLL2     (0x01 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE3 CLK clock source as PLL2
#define PLL_CLKSEL_SIE3_CLKSRC_PLL5     (0x02 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE3 CLK clock source as PLL5
#define PLL_CLKSEL_SIE3_CLKSRC_PLL14    (0x03 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE3 CLK clock source as PLL14
//@}

/*
    @name   SIE4 CLK clock source value

    SIE4 CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE4_CLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE4_CLKSRC_480      (0x00 << (PLL_CLKSEL_SIE4_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE4 CLK clock source as 480 MHz
#define PLL_CLKSEL_SIE4_CLKSRC_PLL2     (0x01 << (PLL_CLKSEL_SIE4_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE4 CLK clock source as PLL2
#define PLL_CLKSEL_SIE4_CLKSRC_PLL5     (0x02 << (PLL_CLKSEL_SIE4_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE4 CLK clock source as PLL5
#define PLL_CLKSEL_SIE4_CLKSRC_PLL14    (0x03 << (PLL_CLKSEL_SIE4_CLKSRC - PLL_CLKSEL_R7_OFFSET))   //< Select SIE4 CLK clock source as PLL14
//@}


/*
    @name   SIE MCLK clock Inverted Output

    Invert output phase of SIE MCLK

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKINV).
*/
//@{
#define PLL_CLKSEL_SIE_MCLKINV_DIS      (0x00 << (PLL_CLKSEL_SIE_MCLKINV - PLL_CLKSEL_R2_OFFSET))  //< Select SIE MCLK normal output
#define PLL_CLKSEL_SIE_MCLKINV_EN       (0x01 << (PLL_CLKSEL_SIE_MCLKINV - PLL_CLKSEL_R2_OFFSET))  //< Select SIE MCLK inverted output
//@}

/*
    @name   SIE2 MCLK clock Inverted Output

    Invert output phase of SIE2 MCLK

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE2_MCLKINV).
*/
//@{
#define PLL_CLKSEL_SIE2_MCLKINV_DIS     (0x00 << (PLL_CLKSEL_SIE2_MCLKINV - PLL_CLKSEL_R2_OFFSET)) //< Select SIE2 MCLK normal output
#define PLL_CLKSEL_SIE2_MCLKINV_EN      (0x01 << (PLL_CLKSEL_SIE2_MCLKINV - PLL_CLKSEL_R2_OFFSET)) //< Select SIE2 MCLK inverted output
//@}


/*
    @name   DSI PHY clock source value

    DSI PHY clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSI_CLKSRC).
*/
//@{
#define PLL_CLKSEL_DSI_CLKSRC_480   (0x00 << 0/*(PLL_CLKSEL_DSI_CLKSRC - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_DSI_CLKSRC_PLL2  (0x01 << 0/*(PLL_CLKSEL_DSI_CLKSRC - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_DSI_CLKSRC_PLL4  (0x02 << 0/*(PLL_CLKSEL_DSI_CLKSRC - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_DSI_CLKSRC_PLL7  (0x03 << 0/*(PLL_CLKSEL_DSI_CLKSRC - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
//@}

/*
    @name   DSI LP clock source value

    DSI LP clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DSI_LPSRC).
*/
//@{
#define PLL_CLKSEL_DSI_LPSRC_60   (0x00 << (PLL_CLKSEL_DSI_LPSRC - PLL_CLKSEL_R4_OFFSET))    //< Select DSI LP clock as 60MHz
#define PLL_CLKSEL_DSI_LPSRC_80   (0x01 << (PLL_CLKSEL_DSI_LPSRC - PLL_CLKSEL_R4_OFFSET))    //< Select DSI LP clock as 80MHz
//@}


/*
    @name   MI clock source value

    MI clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_MI_CLKSRC).
*/
//@{
#define PLL_CLKSEL_MI_CLKSRC_192   (0x00 << (PLL_CLKSEL_MI_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select MI clock as 192MHz
#define PLL_CLKSEL_MI_CLKSRC_240   (0x01 << (PLL_CLKSEL_MI_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select MI clock as 240MHz
//@}


/*
    @name   IDE clock source value

    IDE clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IDE_CLKSRC).
*/
//@{
#define PLL_CLKSEL_IDE_CLKSRC_480   (0x00 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as 480 MHz
#define PLL_CLKSEL_IDE_CLKSRC_PLL2  (0x01 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as PLL2 (for video/HDMI)
#define PLL_CLKSEL_IDE_CLKSRC_PLL4  (0x02 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as PLL4 (for SSPLL)
#define PLL_CLKSEL_IDE_CLKSRC_PLL14 (0x03 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as PLL114
//@}
#define PLL_CLKSEL_IDE_CLKSRC_PLL1  (PLL_CLKSEL_IDE_CLKSRC_480)                                 //< Backward compatible
#define PLL_CLKSEL_IDE_CLKSRC_PLL7  (0x03 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Backward compatible
#define PLL_CLKSEL_IDE_CLKSRC_PLL12 (0x03 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Backward compatible


/*
    @name   IDE2 clock source value

    IDE2 clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IDE2_CLKSRC).
*/
//@{
#define PLL_CLKSEL_IDE2_CLKSRC_480   (0x00 << (PLL_CLKSEL_IDE2_CLKSRC - PLL_CLKSEL_R4_OFFSET))  //< Select IDE2 clock source as 480 MHz
#define PLL_CLKSEL_IDE2_CLKSRC_PLL2  (0x01 << (PLL_CLKSEL_IDE2_CLKSRC - PLL_CLKSEL_R4_OFFSET))  //< Select IDE2 clock source as PLL2 (for video/HDMI)
#define PLL_CLKSEL_IDE2_CLKSRC_PLL4  (0x02 << (PLL_CLKSEL_IDE2_CLKSRC - PLL_CLKSEL_R4_OFFSET))  //< Select IDE2 clock source as PLL4 (for SSPLL)
#define PLL_CLKSEL_IDE2_CLKSRC_PLL14 (0x03 << (PLL_CLKSEL_IDE2_CLKSRC - PLL_CLKSEL_R4_OFFSET))  //< Select IDE2 clock source as PLL14
//@}
#define PLL_CLKSEL_IDE2_CLKSRC_PLL1  (PLL_CLKSEL_IDE2_CLKSRC_480)                               //< Backward compatible
#define PLL_CLKSEL_IDE2_CLKSRC_PLL7  (0x03 << (PLL_CLKSEL_IDE2_CLKSRC - PLL_CLKSEL_R4_OFFSET))  //< Backward compatible
#define PLL_CLKSEL_IDE2_CLKSRC_PLL12 (0x03 << (PLL_CLKSEL_IDE2_CLKSRC - PLL_CLKSEL_R4_OFFSET))  //< Backward compatible

/*
    @name   ETH reference clock invert value

    ETH reference clock invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ETH_REFCLK_INV).
*/
//@{
#define PLL_CLKSEL_ETH_REFCLK_DIS    (0x00 << (PLL_CLKSEL_ETH_REFCLK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select ETH reference clock not invert
#define PLL_CLKSEL_ETH_REFCLK_EN     (0x01 << (PLL_CLKSEL_ETH_REFCLK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select ETH reference clock invert
//@}

/*
    @name   ETH REVMII OPT invert value

    ETH REVMII OPT invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ETH_REVMII_OPT_INV).
*/
//@{
#define PLL_CLKSEL_ETH_REVMII_OPT_DIS    (0x00 << (PLL_CLKSEL_ETH_REVMII_OPT_INV - PLL_CLKSEL_R5_OFFSET))  //< Select ETH REVMII OPT not invert
#define PLL_CLKSEL_ETH_REVMII_OPT_EN     (0x01 << (PLL_CLKSEL_ETH_REVMII_OPT_INV - PLL_CLKSEL_R5_OFFSET))  //< Select ETH REVMII OPT invert
//@}

/*
    @name   ETH REVMII OPT2 invert value

    ETH REVMII OPT2 invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ETH_REVMII_OPT2_INV).
*/
//@{
#define PLL_CLKSEL_ETH_REVMII_OPT2_DIS    (0x00 << (PLL_CLKSEL_ETH_REVMII_OPT2_INV - PLL_CLKSEL_R5_OFFSET))  //< Select ETH REVMII OPT2 not invert
#define PLL_CLKSEL_ETH_REVMII_OPT2_EN     (0x01 << (PLL_CLKSEL_ETH_REVMII_OPT2_INV - PLL_CLKSEL_R5_OFFSET))  //< Select ETH REVMII OPT2 invert
//@}

/*
    @name   SLVS-EC DataLane Clock invert value

    SLVS-EC DataLane Clock invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SLVSEC_D"0~7"CK_INV).
*/
//@{
#define PLL_CLKSEL_SLVSEC_DxCK_INVDIS       (0x00 << (PLL_CLKSEL_SLVSEC_D0CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock signal not invert
#define PLL_CLKSEL_SLVSEC_D0CK_INVEN        (0x01 << (PLL_CLKSEL_SLVSEC_D0CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock 0 signal invert
#define PLL_CLKSEL_SLVSEC_D1CK_INVEN        (0x01 << (PLL_CLKSEL_SLVSEC_D1CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock 0 signal invert
#define PLL_CLKSEL_SLVSEC_D2CK_INVEN        (0x01 << (PLL_CLKSEL_SLVSEC_D2CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock 0 signal invert
#define PLL_CLKSEL_SLVSEC_D3CK_INVEN        (0x01 << (PLL_CLKSEL_SLVSEC_D3CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock 0 signal invert
#define PLL_CLKSEL_SLVSEC_D4CK_INVEN        (0x01 << (PLL_CLKSEL_SLVSEC_D4CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock 0 signal invert
#define PLL_CLKSEL_SLVSEC_D5CK_INVEN        (0x01 << (PLL_CLKSEL_SLVSEC_D5CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock 0 signal invert
#define PLL_CLKSEL_SLVSEC_D6CK_INVEN        (0x01 << (PLL_CLKSEL_SLVSEC_D6CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock 0 signal invert
#define PLL_CLKSEL_SLVSEC_D7CK_INVEN        (0x01 << (PLL_CLKSEL_SLVSEC_D7CK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select SLVS-EC DataLane Clock 0 signal invert
//@}

/*
    @name   TSDEMUX Clock selection

    TSDEMUX Clock selection

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TSDEMUX).
*/
//@{
#define PLL_CLKSEL_TSDEMUX_240         (0x00 << (PLL_CLKSEL_TSDEMUX - PLL_CLKSEL_R5_OFFSET))  //< Select TSDEMUX clock as 240Mhz
#define PLL_CLKSEL_TSDEMUX_480         (0x01 << (PLL_CLKSEL_TSDEMUX - PLL_CLKSEL_R5_OFFSET))  //< Select TSDEMUX clock as 480Mhz
//@}
#define PLL_CLKSEL_TSDEMUX_160         (0x04 << (PLL_CLKSEL_TSDEMUX - PLL_CLKSEL_R5_OFFSET))  //< Backward compatible


/*
    @name   SIE MCLK clock divider

    SIE MCLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKDIV).
*/
//@{
#define PLL_SIE_MCLKDIV(x)          ((x) << (PLL_CLKSEL_SIE_MCLKDIV - PLL_CLKSEL_R8_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKDIV)
//@}

/*
    @name   SIE MCLK2 clock divider

    SIE MCLK2 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2DIV).
*/
//@{
#define PLL_SIE_MCLK2DIV(x)         ((x) << (PLL_CLKSEL_SIE_MCLK2DIV - PLL_CLKSEL_R8_OFFSET))   //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2DIV)
//@}

/*
    @name   SIE CLK clock divider

    SIE CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE_CLKDIV).
*/
//@{
#define PLL_SIE_CLKDIV(x)           ((x) << (PLL_CLKSEL_SIE_CLKDIV - PLL_CLKSEL_R8_OFFSET))         //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE_CLKDIV)
//@}

/*
    @name   SIE2 CLK clock divider

    SIE2 CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE2_CLKDIV).
*/
//@{
#define PLL_SIE2_CLKDIV(x)          ((x) << (PLL_CLKSEL_SIE2_CLKDIV - PLL_CLKSEL_R8_OFFSET))        //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE2_PLLCLKDIV)
//@}

/*
    @name   SIE3 CLK clock divider

    SIE3 CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE3_CLKDIV).
*/
//@{
#define PLL_SIE3_CLKDIV(x)          ((x) << (PLL_CLKSEL_SIE3_CLKDIV - PLL_CLKSEL_R9_OFFSET))        //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE3_PLLCLKDIV)
//@}

/*
    @name   SIE4 CLK clock divider

    SIE4 CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE4_CLKDIV).
*/
//@{
#define PLL_SIE4_CLKDIV(x)          ((x) << (PLL_CLKSEL_SIE4_CLKDIV - PLL_CLKSEL_R9_OFFSET))        //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE4_PLLCLKDIV)
//@}

/*
    @name   SIE5 CLK clock divider

    SIE5 CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE5_CLKDIV).
*/
//@{
#define PLL_SIE5_CLKDIV(x)          ((x) << (PLL_CLKSEL_SIE5_CLKDIV - PLL_CLKSEL_R9_OFFSET))        //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE5_PLLCLKDIV)
//@}

/*
    @name   SIE6 CLK clock divider

    SIE6 CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE6_CLKDIV).
*/
//@{
#define PLL_SIE6_CLKDIV(x)          ((x) << (PLL_CLKSEL_SIE6_CLKDIV - PLL_CLKSEL_R9_OFFSET))        //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE6_PLLCLKDIV)
//@}

/*
    @name   SIE7 CLK clock divider

    SIE7 CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE7_CLKDIV).
*/
//@{
#define PLL_SIE7_CLKDIV(x)          ((x) << (PLL_CLKSEL_SIE7_CLKDIV - PLL_CLKSEL_R9_OFFSET))        //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE7_PLLCLKDIV)
//@}

/*
    @name   SIE8 CLK clock divider

    SIE8 CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE8_CLKDIV).
*/
//@{
#define PLL_SIE8_CLKDIV(x)          ((x) << (PLL_CLKSEL_SIE8_CLKDIV - PLL_CLKSEL_R9_OFFSET))        //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE8_PLLCLKDIV)
//@}

/*
    @name   IDE clock divider

    IDE clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_IDE_CLKDIV).
*/
//@{
#define PLL_IDE_CLKDIV(x)           ((x) << (PLL_CLKSEL_IDE_CLKDIV - PLL_CLKSEL_R23_OFFSET))     //< Used for pll_set_clock_rate(PLL_CLKSEL_IDE_CLKDIV)
//@}

/*
    @name   IDE2 clock divider

    IDE2 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_IDE2_CLKDIV).
*/
//@{
#define PLL_IDE2_CLKDIV(x)          ((x) << (PLL_CLKSEL_IDE2_CLKDIV - PLL_CLKSEL_R23_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_IDE2_CLKDIV)
//@}

/*
    @name   IDE OUTIF clock divider

    IDE OUTIF clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_IDE_OUTIF_CLKDIV).
*/
//@{
#define PLL_IDE_OUTIF_CLKDIV(x)     ((x) << (PLL_CLKSEL_IDE_OUTIF_CLKDIV - PLL_CLKSEL_R23_OFFSET))     //< Used for pll_set_clock_rate(PLL_CLKSEL_IDE_OUTIF_CLKDIV)
//@}

/*
    @name   IDE2 OUTIF clock divider

    IDE2 OUTIF clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_IDE2_OUTIF_CLKDIV).
*/
//@{
#define PLL_IDE2_OUTIF_CLKDIV(x)    ((x) << (PLL_CLKSEL_IDE2_OUTIF_CLKDIV - PLL_CLKSEL_R23_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_IDE2_OUTIF_CLKDIV)
//@}


/*
    @name   MI clock divider

    MI clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_MI_CLKDIV)
*/
//@{
#define PLL_MI_CLKDIV(x)            ((x) << (PLL_CLKSEL_MI_CLKDIV - PLL_CLKSEL_R10_OFFSET))      //< This if for pll_set_clock_rate(PLL_CLKSEL_MI_CLKDIV)
//@}

/*
    @name   DSI clock divider

    DSI clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_DSI_CLKDIV)
*/
//@{
#define PLL_DSI_CLKDIV(x)           ((x) << 0/*(PLL_CLKSEL_DSI_CLKDIV - PLL_CLKSEL_R9_OFFSET)*/)      //< Backward compatible
//@}

/*
    @name   SP clock divider

    Special clock divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SP_CLKDIV)
*/
//@{
#define PLL_SP_CLKDIV(x)            ((x) << (PLL_CLKSEL_SP_CLKDIV - PLL_CLKSEL_R11_OFFSET))     //< Used for pll_set_clock_rate(PLL_CLKSEL_SP_CLKDIV)
//@}

/*
    @name   SDIO clock divider

    SDIO clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SDIO_CLKDIV)
*/
//@{
#define PLL_SDIO_CLKDIV(x)          ((x) << (PLL_CLKSEL_SDIO_CLKDIV - PLL_CLKSEL_R12_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SDIO_CLKDIV)
//@}

/*
    @name   SDIO2 clock divider

    SDIO2 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SDIO2_CLKDIV)
*/
//@{
#define PLL_SDIO2_CLKDIV(x)         ((x) << (PLL_CLKSEL_SDIO2_CLKDIV - PLL_CLKSEL_R12_OFFSET))  //< This if for pll_set_clock_rate(PLL_CLKSEL_SDIO2_CLKDIV)
//@}


/*
    @name   NAND clock divider

    NAND clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_NAND_CLKDIV)
*/
//@{
#define PLL_NAND_CLKDIV(x)          ((x) << (PLL_CLKSEL_NAND_CLKDIV - PLL_CLKSEL_R12_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_NAND_CLKDIV)
//@}


/*
    @name   SDIO3 clock divider

    SDIO3 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SDIO3_CLKDIV)
*/
//@{
#define PLL_SDIO3_CLKDIV(x)         ((x) << (PLL_CLKSEL_SDIO3_CLKDIV - PLL_CLKSEL_R13_OFFSET))  //< This if for pll_set_clock_rate(PLL_CLKSEL_SDIO3_CLKDIV)
//@}

/*
    @name   SP2 clock divider

    Special clock2 divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SP2_CLKDIV)
*/
//@{
#define PLL_SP2_CLKDIV(x)           ((x) << (PLL_CLKSEL_SP2_CLKDIV - PLL_CLKSEL_R13_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_SP2_CLKDIV)
//@}


/*
    @name   SPI clock divider

    SPI clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI_CLKDIV)
*/
//@{
#define PLL_SPI_CLKDIV(x)           ((x) << (PLL_CLKSEL_SPI_CLKDIV - PLL_CLKSEL_R14_OFFSET))    //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI_CLKDIV)
//@}

/*
    @name   SPI2 clock divider

    SPI2 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI2_CLKDIV)
*/
//@{
#define PLL_SPI2_CLKDIV(x)          ((x) << (PLL_CLKSEL_SPI2_CLKDIV - PLL_CLKSEL_R14_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI2_CLKDIV)
//@}

/*
    @name   SPI3 clock divider

    SPI3 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI3_CLKDIV)
*/
//@{
#define PLL_SPI3_CLKDIV(x)          ((x) << (PLL_CLKSEL_SPI3_CLKDIV - PLL_CLKSEL_R15_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI3_CLKDIV)
//@}

/*
    @name   SPI4 clock divider

    SPI4 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI4_CLKDIV)
*/
//@{
#define PLL_SPI4_CLKDIV(x)          ((x) << (PLL_CLKSEL_SPI4_CLKDIV - PLL_CLKSEL_R15_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI4_CLKDIV)
//@}

/*
    @name   SPI5 clock divider

    SPI5 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI5_CLKDIV)
*/
//@{
#define PLL_SPI5_CLKDIV(x)          //< Backward compatible
//@}

/*
    @name   Audio internal clock mux value

    Audio internal clock mux value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ADO_CLKMUX).
*/
//@{
#define PLL_CLKSEL_ADO_CLKMUX_24P576    (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_12P288    (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_8P192     (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_6P144     (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_4P096     (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_3P072     (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_2P048     (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_11P2896   (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_5P6448    (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
#define PLL_CLKSEL_ADO_CLKMUX_2P8224    (0x00 << 0/*(PLL_CLKSEL_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET)*/)    //< Backward compatible
//@}

/*
    @name   HDMI Audio internal clock mux value

    HDMI Audio internal clock mux value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_HDMI_ADO_CLKMUX).
*/
//@{
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_24P576   (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_12P288   (0x02 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< HDMI Audio internal clock = 12.288 MHz
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_8P192    (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< HDMI Audio internal clock = 8.192 MHz
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_6P144    (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_4P096    (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_3P072    (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_2P048    (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_11P2896  (0x01 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< HDMI Audio internal clock = 11.2896 MHz
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_5P6448   (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< Backward compatible
#define PLL_CLKSEL_HDMI_ADO_CLKMUX_2P8224   (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKMUX - PLL_CLKSEL_R4_OFFSET))   //< Backward compatible
//@}

/*
    @name   Audio clock select

    Select source clock of audio system (DAI/EAC/audio MCLK)

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ADO_CLKSEL).
*/
//@{
#define PLL_CLKSEL_ADO_CLKSEL_CLKMUX    (0x00 << (PLL_CLKSEL_ADO_CLKSEL - PLL_CLKSEL_R4_OFFSET))    //< Audio clock = PLL_CLKSEL_ADO_CLKMUX
#define PLL_CLKSEL_ADO_CLKSEL_CLKDIV    (0x01 << (PLL_CLKSEL_ADO_CLKSEL - PLL_CLKSEL_R4_OFFSET))    //< Audio clock = PLL7 / (PLL_CLKSEL_ADO_CLKDIV+1)
//@}

/*
    @name   HDMI Audio clock select

    Select source clock of HDMI audio

    @note This is for pll_set_clock_rate(PLL_CLKSEL_HDMI_ADO_CLKSEL).
*/
//@{
#define PLL_CLKSEL_HDMI_ADO_CLKSEL_CLKMUX   (0x00 << (PLL_CLKSEL_HDMI_ADO_CLKSEL - PLL_CLKSEL_R4_OFFSET))   //< HDMI Audio clock = PLL_CLKSEL_HDMI_ADO_CLKMUX
#define PLL_CLKSEL_HDMI_ADO_CLKSEL_CLKDIV   (0x01 << (PLL_CLKSEL_HDMI_ADO_CLKSEL - PLL_CLKSEL_R4_OFFSET))   //< HDMI Audio clock = PLL7 / (PLL_CLKSEL_HDMI_ADO_CLKDIV+1)
//@}

/*
    @name   CEC clock divider

    CEC clock divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CEC_CLKDIV).
*/
//@{
#define PLL_CEC_CLKDIV(x)           ((x) << (PLL_CLKSEL_CEC_CLKDIV - PLL_CLKSEL_R11_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_CEC_CLKDIV)
//@}

/*
    @name   ADO clock divider

    ADO clock divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ADO_CLKDIV).
*/
//@{
#define PLL_ADO_CLKDIV(x)           ((x) << (PLL_CLKSEL_ADO_CLKDIV - PLL_CLKSEL_R11_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_ADO_CLKDIV)
//@}

/*
    @name   HDMI ADO clock divider

    HDMI ADO clock divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_HDMI_ADO_CLKDIV).
*/
//@{
#define PLL_HDMI_ADO_CLKDIV(x)      ((x) << (PLL_CLKSEL_HDMI_ADO_CLKDIV - PLL_CLKSEL_R11_OFFSET))   //< Used for pll_set_clock_rate(PLL_CLKSEL_HDMI_ADO_CLKDIV)
//@}

/*
    @name   EAC ClassD clock divider

    EAC ClassD clock divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CLASSD_CLKDIV).
*/
//@{
#define PLL_CLASSD_CLKDIV(x)           ((x) << (PLL_CLKSEL_CLASSD_CLKDIV - PLL_CLKSEL_R13_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_CLASSD_CLKDIV)
//@}


/*
    @name   LVDS Clock Phase Select

    LVDS Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS_CLKPHASE).
*/
//@{
#define PLL_LVDS_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS_CLKPHASE - PLL_CLKSEL_R6_OFFSET))    //< Select LVDS Clock Phase as Normal (non-invert)
#define PLL_LVDS_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS_CLKPHASE - PLL_CLKSEL_R6_OFFSET))    //< Select LVDS Clock Phase as Invert
//@}

/*
    @name   LVDS2 Clock Phase Select

    LVDS2 Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS2_CLKPHASE).
*/
//@{
#define PLL_LVDS2_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS2_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS2 Clock Phase as Normal (non-invert)
#define PLL_LVDS2_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS2_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS2 Clock Phase as Invert
//@}

/*
    @name   LVDS3 Clock Phase Select

    LVDS3 Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS3_CLKPHASE).
*/
//@{
#define PLL_LVDS3_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS3_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS3 Clock Phase as Normal (non-invert)
#define PLL_LVDS3_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS3_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS3 Clock Phase as Invert
//@}

/*
    @name   LVDS4 Clock Phase Select

    LVDS4 Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS4_CLKPHASE).
*/
//@{
#define PLL_LVDS4_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS4_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS4 Clock Phase as Normal (non-invert)
#define PLL_LVDS4_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS4_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS4 Clock Phase as Invert
//@}

/*
    @name   LVDS5 Clock Phase Select

    LVDS5 Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS5_CLKPHASE).
*/
//@{
#define PLL_LVDS5_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS5_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS5 Clock Phase as Normal (non-invert)
#define PLL_LVDS5_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS5_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS5 Clock Phase as Invert
//@}

/*
    @name   LVDS6 Clock Phase Select

    LVDS6 Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS6_CLKPHASE).
*/
//@{
#define PLL_LVDS6_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS6_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS6 Clock Phase as Normal (non-invert)
#define PLL_LVDS6_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS6_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS6 Clock Phase as Invert
//@}

/*
    @name   LVDS7 Clock Phase Select

    LVDS7 Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS7_CLKPHASE).
*/
//@{
#define PLL_LVDS7_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS7_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS7 Clock Phase as Normal (non-invert)
#define PLL_LVDS7_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS7_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS7 Clock Phase as Invert
//@}

/*
    @name   LVDS8 Clock Phase Select

    LVDS8 Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS8_CLKPHASE).
*/
//@{
#define PLL_LVDS8_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS8_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS8 Clock Phase as Normal (non-invert)
#define PLL_LVDS8_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS8_CLKPHASE - PLL_CLKSEL_R6_OFFSET))   //< Select LVDS8 Clock Phase as Invert
//@}

/*
    @name   CSI LP D0 Clock Select

    CSI LP D0 Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LPD0_LPCLK).
*/
//@{
#define PLL_CSI_LPD0_CLK_SEL_D0      (0x00 << (PLL_CLKSEL_LPD0_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D0 from LPCLK_D0PN
#define PLL_CSI_LPD0_CLK_SEL_D1      (0x01 << (PLL_CLKSEL_LPD0_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D0 from LPCLK_D1PN
#define PLL_CSI_LPD0_CLK_SEL_D2      (0x02 << (PLL_CLKSEL_LPD0_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D0 from LPCLK_D1PN
#define PLL_CSI_LPD0_CLK_SEL_D3      (0x03 << (PLL_CLKSEL_LPD0_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D0 from LPCLK_D1PN
//@}

/*
    @name   CSI LP D1 Clock Select

    CSI LP D1 Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LPD1_LPCLK).
*/
//@{
#define PLL_CSI_LPD1_CLK_SEL_D0      (0x00 << (PLL_CLKSEL_LPD1_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D1 from LPCLK_D0PN
#define PLL_CSI_LPD1_CLK_SEL_D1      (0x01 << (PLL_CLKSEL_LPD1_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D1 from LPCLK_D1PN
#define PLL_CSI_LPD1_CLK_SEL_D2      (0x02 << (PLL_CLKSEL_LPD1_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D1 from LPCLK_D1PN
#define PLL_CSI_LPD1_CLK_SEL_D3      (0x03 << (PLL_CLKSEL_LPD1_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D1 from LPCLK_D1PN
//@}

/*
    @name   CSI LP D2 Clock Select

    CSI LP D2 Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LPD2_LPCLK).
*/
//@{
#define PLL_CSI_LPD2_CLK_SEL_D0      (0x00 << (PLL_CLKSEL_LPD2_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D2 from LPCLK_D0PN
#define PLL_CSI_LPD2_CLK_SEL_D1      (0x01 << (PLL_CLKSEL_LPD2_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D2 from LPCLK_D1PN
#define PLL_CSI_LPD2_CLK_SEL_D2      (0x02 << (PLL_CLKSEL_LPD2_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D2 from LPCLK_D1PN
#define PLL_CSI_LPD2_CLK_SEL_D3      (0x03 << (PLL_CLKSEL_LPD2_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D2 from LPCLK_D1PN
//@}


/*
    @name   CSI LP D3 Clock Select

    CSI LP D3 Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LPD3_LPCLK).
*/
//@{
#define PLL_CSI_LPD3_CLK_SEL_D0      (0x00 << (PLL_CLKSEL_LPD3_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D3 from LPCLK_D0PN
#define PLL_CSI_LPD3_CLK_SEL_D1      (0x01 << (PLL_CLKSEL_LPD3_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D3 from LPCLK_D1PN
#define PLL_CSI_LPD3_CLK_SEL_D2      (0x02 << (PLL_CLKSEL_LPD3_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D3 from LPCLK_D1PN
#define PLL_CSI_LPD3_CLK_SEL_D3      (0x03 << (PLL_CLKSEL_LPD3_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D3 from LPCLK_D1PN
//@}

/*
    @name   CSI LP D4 Clock Select

    CSI LP D4 Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LPD4_LPCLK).
*/
//@{
#define PLL_CSI_LPD4_CLK_SEL_D4      (0x00 << (PLL_CLKSEL_LPD4_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D4 from LPCLK_D4PN
#define PLL_CSI_LPD4_CLK_SEL_D5      (0x01 << (PLL_CLKSEL_LPD4_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D4 from LPCLK_D5PN
#define PLL_CSI_LPD4_CLK_SEL_D6      (0x02 << (PLL_CLKSEL_LPD4_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D4 from LPCLK_D6PN
#define PLL_CSI_LPD4_CLK_SEL_D7      (0x03 << (PLL_CLKSEL_LPD4_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D4 from LPCLK_D7PN
//@}

/*
    @name   CSI LP D5 Clock Select

    CSI LP D5 Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LPD5_LPCLK).
*/
//@{
#define PLL_CSI_LPD5_CLK_SEL_D4      (0x00 << (PLL_CLKSEL_LPD5_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D5 from LPCLK_D4PN
#define PLL_CSI_LPD5_CLK_SEL_D5      (0x01 << (PLL_CLKSEL_LPD5_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D5 from LPCLK_D5PN
#define PLL_CSI_LPD5_CLK_SEL_D6      (0x02 << (PLL_CLKSEL_LPD5_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D5 from LPCLK_D6PN
#define PLL_CSI_LPD5_CLK_SEL_D7      (0x03 << (PLL_CLKSEL_LPD5_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D5 from LPCLK_D7PN
//@}

/*
    @name   CSI LP D6 Clock Select

    CSI LP D6 Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LPD6_LPCLK).
*/
//@{
#define PLL_CSI_LPD6_CLK_SEL_D4      (0x00 << (PLL_CLKSEL_LPD6_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D6 from LPCLK_D4PN
#define PLL_CSI_LPD6_CLK_SEL_D5      (0x01 << (PLL_CLKSEL_LPD6_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D6 from LPCLK_D5PN
#define PLL_CSI_LPD6_CLK_SEL_D6      (0x02 << (PLL_CLKSEL_LPD6_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D6 from LPCLK_D6PN
#define PLL_CSI_LPD6_CLK_SEL_D7      (0x03 << (PLL_CLKSEL_LPD6_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D6 from LPCLK_D7PN
//@}

/*
    @name   CSI LP D7 Clock Select

    CSI LP D7 Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LPD7_LPCLK).
*/
//@{
#define PLL_CSI_LPD7_CLK_SEL_D4      (0x00 << (PLL_CLKSEL_LPD7_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D7 from LPCLK_D4PN
#define PLL_CSI_LPD7_CLK_SEL_D5      (0x01 << (PLL_CLKSEL_LPD7_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D7 from LPCLK_D5PN
#define PLL_CSI_LPD7_CLK_SEL_D6      (0x02 << (PLL_CLKSEL_LPD7_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D7 from LPCLK_D6PN
#define PLL_CSI_LPD7_CLK_SEL_D7      (0x03 << (PLL_CLKSEL_LPD7_LPCLK - PLL_CLKSEL_R24_OFFSET))   //< Select CSI LP D7 from LPCLK_D7PN
//@}


/*
    @name   SLVSEC Deskew Clock Select

    SLVSEC Deskew Clock Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SLVSEC).
*/
//@{
#define PLL_SLVSEC_CLK_DATA0   (0x00 << (PLL_CLKSEL_SLVSEC - PLL_CLKSEL_R6_OFFSET))   //< Select SLVSEC Clock from data 0
#define PLL_SLVSEC_CLK_DATA1   (0x01 << (PLL_CLKSEL_SLVSEC - PLL_CLKSEL_R6_OFFSET))   //< Select SLVSEC Clock from data 1
#define PLL_SLVSEC_CLK_DATA2   (0x02 << (PLL_CLKSEL_SLVSEC - PLL_CLKSEL_R6_OFFSET))   //< Select SLVSEC Clock from data 2
#define PLL_SLVSEC_CLK_DATA3   (0x03 << (PLL_CLKSEL_SLVSEC - PLL_CLKSEL_R6_OFFSET))   //< Select SLVSEC Clock from data 3
#define PLL_SLVSEC_CLK_DATA4   (0x04 << (PLL_CLKSEL_SLVSEC - PLL_CLKSEL_R6_OFFSET))   //< Select SLVSEC Clock from data 4
#define PLL_SLVSEC_CLK_DATA5   (0x05 << (PLL_CLKSEL_SLVSEC - PLL_CLKSEL_R6_OFFSET))   //< Select SLVSEC Clock from data 5
#define PLL_SLVSEC_CLK_DATA6   (0x06 << (PLL_CLKSEL_SLVSEC - PLL_CLKSEL_R6_OFFSET))   //< Select SLVSEC Clock from data 6
#define PLL_SLVSEC_CLK_DATA7   (0x07 << (PLL_CLKSEL_SLVSEC - PLL_CLKSEL_R6_OFFSET))   //< Select SLVSEC Clock from data 7
//@}

/*
    @name   USB2/3 Clock Phase Inversion

    USB2/3 Clock Phase Inversion

    @note This is for pll_set_clock_rate(PLL_CLKSEL_U3PCLK_INV / PLL_CLKSEL_U3UCLK_INV / PLL_CLKSEL_U2UCLK_INV / PLL_CLKSEL_U3REFCLK_SEL).
*/
//@{
#define PLL_CLKSEL_U3PCLK_INV_DIS      (0x00 << (PLL_CLKSEL_U3PCLK_INV - PLL_CLKSEL_R6_OFFSET))  //< USB3 Pipe Clock Phase Normal
#define PLL_CLKSEL_U3PCLK_INV_EN       (0x01 << (PLL_CLKSEL_U3PCLK_INV - PLL_CLKSEL_R6_OFFSET))  //< USB3 Pipe Clock Phase Invert
#define PLL_CLKSEL_U3UCLK_INV_DIS      (0x00 << (PLL_CLKSEL_U3UCLK_INV - PLL_CLKSEL_R6_OFFSET))  //< USB3 UClock Phase Normal
#define PLL_CLKSEL_U3UCLK_INV_EN       (0x01 << (PLL_CLKSEL_U3UCLK_INV - PLL_CLKSEL_R6_OFFSET))  //< USB3 UClock Phase Invert
#define PLL_CLKSEL_U2UCLK_INV_DIS      (0x00 << (PLL_CLKSEL_U2UCLK_INV - PLL_CLKSEL_R6_OFFSET))  //< USB2 UClock Phase Normal
#define PLL_CLKSEL_U2UCLK_INV_EN       (0x01 << (PLL_CLKSEL_U2UCLK_INV - PLL_CLKSEL_R6_OFFSET))  //< USB2 UClock Phase Invert
#define PLL_CLKSEL_U3REFCLK_PLL12M     (0x00 << (PLL_CLKSEL_U3REFCLK_SEL - PLL_CLKSEL_R6_OFFSET))//< USB3 Refrence Clock from PLL12M
#define PLL_CLKSEL_U3REFCLK_UTANA      (0x01 << (PLL_CLKSEL_U3REFCLK_SEL - PLL_CLKSEL_R6_OFFSET))//< USB3 Refrence Clock from CK_UT_ANA
//@}


/*
    System Reset ID

    This is for pll_enable_system_reset() and pll_disable_system_reset().
*/
typedef enum {
	PLL_RSTN_DRAMC = 1, //< Reset SDRAM controller
	DRAMC2_RSTN = 2,    //< Reset SDRAM2 controller
	SIE_RSTN = 4,       //< Reset SIE controller
	SIE2_RSTN,          //< Reset SIE2 controller
	//5

	TGE_RSTN = 6,       //< Reset PRE controller
	IPE_RSTN,           //< Reset IPE controller
	DIS_RSTN,           //< Reset DIS controller
	IME_RSTN,           //< Reset IME controller
	FDE_RSTN,           //< Reset FDE controller
	//10

	ISE_RSTN,           //< Reset ISE controller
	IFE_RSTN = 13,      //< Reset IFE controller
	DCE_RSTN,           //< Reset DCE controller
	IFE2_RSTN,          //< Reset IFE2 controller
	//15

	IDE_RSTN = 16,      //< Reset IDE controller
	IDE2_RSTN,          //< Reset IDE2 controller
	TV_RSTN,            //< Reset TV controller
	MI_RSTN = 20,       //< Reset MI controller
	//20

	CEC_RSTN = 21,      //< Reset CEC controller
	HDMI_RSTN,          //< Reset HDMI controller
	CRYPTO_RSTN,        //< Reset CRYPTO controller
	H26X_RSTN,          //< Reset H264 controller
	AFFINE_RSTN,        //< Reset AFFINE controller
	//25


	JPG_RSTN = 26,      //< Reset JPG controller
	GRAPH_RSTN,         //< Reset Graphic controller
	GRAPH2_RSTN,        //< Reset Graphic2 controller
	DAI_RSTN,           //< Reset DAI controller
	EAC_RSTN,           //< Reset EAC controller
	//30

	NAND_RSTN = 32,     //< Reset xD/NAND controller
	SDIO_RSTN = 34,     //< Reset SDIO controller
	SDIO2_RSTN,         //< Reset SDIO2 controller
	//35

	I2C_RSTN = 36,      //< Reset I2C controller
	I2C2_RSTN,          //< Reset I2C2 controller
	SPI_RSTN,           //< Reset SPI controller
	SPI2_RSTN,          //< Reset SPI2 controller
	SPI3_RSTN,          //< Reset SPI3 controller
	//40

	SIF_RSTN = 41,      //< Reset SIF controller
	UART_RSTN,          //< Reset UART controller
	UART2_RSTN,         //< Reset UART2 controller
	REMOTE_RSTN,        //< Reset Remote controller
	ADC_RSTN,           //< Reset ADC controller
	//45

	SDIO3_RSTN,         //< Reset SDIO3 controller
	SPI4_RSTN,          //< Reset SPI4 controller

	RTC_RSTN = 48,      //< Reset RTC controller
	WDT_RSTN,           //< Reset WDT controller
	TMR_RSTN,           //< Reset Timer controller
	//50

	USB_RSTN = 51,      //< Reset USB controller

	UART3_RSTN = 54,    //< Reset UART3 controller
	UART4_RSTN,         //< Reset UART4 controller
	//55

	INTC_RSTN = 56,     //< Reset INTC controller
	GPIO_RSTN,          //< Reset GPIO controller
	PAD_RSTN,           //< Reset PAD controller
	TOP_RSTN,           //< Reset TOP controller
	EFUSE_RSTN,         //< Reset EFUSE controller
	ETH_RSTN,           //< Reset ETH controller
	//61

	I2C3_RSTN = 63,     //< Reset I2C3 controller

	MIPI_RSTN = 65,     //< Backward compatible
	MIPI_LVDS_RSTN = 65,//< Reset MIPI_LVDS controller
	MIPI2_RSTN = 66,    //< Backward compatible
	MIPI_LVDS2_RSTN = 66, //< Reset MIPI_LVDS2 controller
	MIPI_DSI_RSTN,      //< Reset MIPI DSI controller
	RDE_RSTN = 70,      //< Reset RDE controller


	PWM_RSTN = 72,      //< Reset PWM controller

	DRE_RSTN = 76,      //< Reset DRE controller
	TSMUX_RSTN,         //< Reset TSMUX controller
	Vx1_RSTN,           //< Reset Vx1 controller
	SIE3_RSTN,          //< Reset SIE3 controller
	SIE4_RSTN,          //< Reset SIE4 controller
	USB2_RSTN,          //< Reset USB2 controller
	CC_RSTN,            //< Reset CC controller
	RHE_RSTN,           //< Reset RHE controller
	SL_RSTN,            //< Reset SubLink controller
	SL_REG_RSTN,        //< Reset SubLink controller register
	//85

	SL_2_RSTN,          //< Reset SubLink2 controller
	SL_2_REG_RSTN,      //< Reset SubLink2 controller register
	Vx12_RSTN,          //< Reset Vx12 controller
	CANBUS_RSTN,        //< Reset CANBUS controller
	I2C4_RSTN,          //< Reset I2C4 controller
	I2C5_RSTN,          //< Reset I2C5 controller
	I2C6_RSTN,          //< Reset I2C6 controller
	I2C7_RSTN,          //< Reset I2C7 controller
	HWCPY_RSTN,         //< Reset HWCPY controller
	ROTATE_RSTN,        //< Reset ROTATE controller


	MIPI_LVDS3_RSTN = 116, //< Reset MIPI_LVDS3 controller
	MIPI_LVDS4_RSTN,       //< Reset MIPI_LVDS4 controller
	MIPI_LVDS5_RSTN,       //< Reset MIPI_LVDS5 controller
	MIPI_LVDS6_RSTN,       //< Reset MIPI_LVDS6 controller
	MIPI_LVDS7_RSTN,       //< Reset MIPI_LVDS7 controller
	MIPI_LVDS8_RSTN,       //< Reset MIPI_LVDS8 controller
	SIE5_RSTN,             //< Reset SIE5 controller
	SIE6_RSTN,             //< Reset SIE6 controller
	SIE7_RSTN,             //< Reset SIE7 controller
	SIE8_RSTN,             //< Reset SIE8 controller
	IVE_RSTN,              //< Reset IVE controller
	SDE_RSTN,              //< Reset SDE controller
	//127

	SVM_RSTN = 128,        //< Reset SVM controller
	CNN_RSTN,              //< Reset CNN controller
	ISE2_RSTN = 130,       //< Reset ISE2 controller
	DDRPHY_RSTN = 134,     //< Reset DDRPHY
	DDRPHY2_RSTN,          //< Reset DDRPHY2
	SLVSEC_RSTN = 138,     //< Reset SLVSEC
	DSP_RSTN,              //< Reset DSP
	DSP2_RSTN,             //< Reset DSP2
	//140

	TSDEMUX_RSTN = 148,    //< Reset TSDEMUX
	ETH_REVMII_RSTN = 149, //< Reset ETH_REVMII
	DRTC_RSTN = 150,       //< Reset DRTC

	DSP_NIC_RSTN = 160,    //< Reset DSP NIC
	DSP_APBTX_RSTN,        //< Reset DSP APBTX
	DSP_AXIB_EPP_RSTN,     //< Reset DSP AXIB EPP
	DSP_AXIB_EDP_RSTN,     //< Reset DSP AXIB EDP
	DSP2_NIC_RSTN = 168,   //< Reset DSP2 NIC
	DSP2_APBTX_RSTN,       //< Reset DSP2 APBTX
	DSP2_AXIB_EPP_RSTN,    //< Reset DSP2 AXIB EPP
	DSP2_AXIB_EDP_RSTN,    //< Reset DSP2 AXIB EDP
	DAP_AXIB_RSTN = 176,   //< Reset DAP AXIB
	USB_AXIB_RSTN = 180,   //< Reset USB AXIB
	ETH_AXIB_RSTN = 184,   //< Reset ETH AXIB

	CHIPPAR2_RSTN = 192,   //< Reset CHIP partition2
	CHIPPAR8_RSTN = 193,   //< Reset CHIP partition3

	ENUM_DUMMY4WORD(CG_RSTN)
} CG_RSTN;

#define PRE_RSTN  0               //< Backward compatible
#define LVDS_RSTN MIPI_LVDS_RSTN  //< Reset LVDS controller (Backward compatible)
#define MIPI_RSTN MIPI_LVDS_RSTN  //< Reset MIPI controller (Backward compatible)
#define CPU2_RSTN 191             //< Backward compatible
#define CPU2OCP_RSTN 191          //< Backward compatible
#define DSPOCP_RSTN 191           //< Backward compatible

/**
    Clock frequency select ID

    @note This is for pll_set_clock_freq().
*/
typedef enum {
	SIEMCLK_FREQ,           ///< SIE MCLK freq Select ID
	SIEMCLK2_FREQ,          ///< SIE MCLK2 freq Select ID
	SIECLK_FREQ,            ///< SIE CLK freq Select ID
	SIE2CLK_FREQ,           ///< SIE2 CLK freq Select ID
	SIE3CLK_FREQ,           ///< SIE3 CLK freq Select ID
	SIE4CLK_FREQ,           ///< SIE4 CLK freq Select ID
	SIE5CLK_FREQ,           ///< SIE5 CLK freq Select ID
	SIE6CLK_FREQ,           ///< SIE6 CLK freq Select ID
	SIE7CLK_FREQ,           ///< SIE7 CLK freq Select ID
	SIE8CLK_FREQ,           ///< SIE8 CLK freq Select ID
	IDECLK_FREQ,            ///< IDE CLK freq Select ID
	IDE2CLK_FREQ,           ///< IDE2 CLK freq Select ID
	MICLK_FREQ,             ///< MI freq Select ID
	DSICLK_FREQ,            ///< DSI freq Select ID
	SPCLK_FREQ,             ///< SP CLK Select ID
	CECCLK_FREQ,            ///< CEC CLK Select ID
	ADOCLK_FREQ,            ///< ADO CLK select ID
	HDMIADOCLK_FREQ,        ///< HDMI ADO CLK select ID
	SDIOCLK_FREQ,           ///< SDIO CLK select ID
	SDIO2CLK_FREQ,          ///< SDIO2 CLK select ID
	SPICLK_FREQ,            ///< SPI CLK select ID
	SPI2CLK_FREQ,           ///< SPI2 CLK select ID
	SPI3CLK_FREQ,           ///< SPI3 CLK select ID
	SPI4CLK_FREQ,           ///< SPI4 CLK select ID
	SPI5CLK_FREQ,           ///< SPI5 CLK select ID

	SPCLK2_FREQ,            ///< SP2 CLK Select ID
	IDEOUTIFCLK_FREQ,       ///< IDE output I/F CLK freq Select ID
	IDE2OUTIFCLK_FREQ,      ///< IDE2 output I/FCLK freq Select ID
	PLL_CLKFREQ_MAXNUM,
	ENUM_DUMMY4WORD(PLL_CLKFREQ)
} PLL_CLKFREQ;

//Backward Compatible
#define XD_CLK      (NAND_CLK)

extern void     pll_set_clock_rate(PLL_CLKSEL uiNum, UINT32 uiValue);
extern UINT32   pll_get_clock_rate(PLL_CLKSEL uiNum);

extern void     pll_set_pwm_clock_rate(UINT32 uiPWMNumber, UINT32 uiDiv);
extern void     pll_enable_clock(CG_EN Num);
extern void     pll_disable_clock(CG_EN Num);
extern ER       pll_set_clock_freq(PLL_CLKFREQ ClkID, UINT32 uiFreq);
extern ER       pll_get_clock_freq(PLL_CLKFREQ ClkID, UINT32 *pFreq);
extern void     pll_enable_system_reset(CG_RSTN Num);
extern void     pll_disable_system_reset(CG_RSTN Num);
extern void     pll_enable_sram_shut_down(CG_RSTN Num);
extern void     pll_disable_sram_shut_down(CG_RSTN Num);

extern ER 		pll_set_driver_pll(PLL_ID id, UINT32 ui_setting);

#define pll_setDrvPLL			pll_set_driver_pll
#define pll_setClockRate		pll_set_clock_rate
#define pll_getClockRate		pll_get_clock_rate
#define pll_enableClock			pll_enable_clock
#define pll_disableClock		pll_disable_clock
#define pll_setClockFreq		pll_set_clock_freq
#define pll_getClockFreq		pll_get_clock_freq
#define pll_enableSystemReset	pll_enable_system_reset
#define pll_disableSystemReset	pll_disable_system_reset

//@}

#endif


