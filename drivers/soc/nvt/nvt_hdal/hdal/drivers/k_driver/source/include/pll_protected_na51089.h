/*
    Protected header for PLL module

    PLL Configuration module protected header file.

    @file       pll_protected.h
    @ingroup    mIDrvSys_CG
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _PLL_PROTECTED_H
#define _PLL_PROTECTED_H

#include <kwrap/nvt_type.h>
#include "plat/pll.h"

#if defined(_NVT_FPGA_)
#ifndef _FPGA_PLL_OSC_
#define _FPGA_PLL_OSC_  24000000
#endif
#endif


/*
    @addtogroup mIDrvSys_CG
*/
//@{

#define PLL_CLKSEL_R1_OFFSET        32
#define PLL_CLKSEL_R2_OFFSET        64
#define PLL_CLKSEL_R3_OFFSET        96
#define PLL_CLKSEL_R4_OFFSET        128
#define PLL_CLKSEL_R5_OFFSET        160
#define PLL_CLKSEL_R6_OFFSET        192
#define PLL_CLKSEL_R7_OFFSET        224
#define PLL_CLKSEL_R8_OFFSET        256
#define PLL_CLKSEL_R9_OFFSET        288
#define PLL_CLKSEL_R10_OFFSET       320
#define PLL_CLKSEL_R11_OFFSET       352
#define PLL_CLKSEL_R12_OFFSET       384
#define PLL_CLKSEL_R13_OFFSET       416
#define PLL_CLKSEL_R14_OFFSET       448
#define PLL_CLKSEL_R15_OFFSET       480
#define PLL_CLKSEL_R16_OFFSET       512
#define PLL_CLKSEL_R17_OFFSET       544
#define PLL_CLKSEL_R18_OFFSET       576
#define PLL_CLKSEL_R19_OFFSET       608
#define PLL_CLKSEL_R20_OFFSET       640
#define PLL_CLKSEL_R21_OFFSET       672


/*
    Clock select ID

    Clock select ID for pll_set_clock_rate() & pll_get_clock_rate().
*/
typedef enum {
	//System Clock Rate bit definition
	PLL_CLKSEL_CPU =            0,                          //< Clock Select Module ID: CPU
	PLL_CLKSEL_APB =            8,                          //< Clock Select Module ID: APB
	PLL_CLKSEL_DMA_ARBT =       20,                         //< Clock select Module ID: DMA Arbiter
	PLL_CLKSEL_PD =             24,                         //< Clock select Module ID: PD
	PLL_CLKSEL_DDRPHY =         31,                         //< Clock Select Module ID: DDR-PHY

	//IPP Clock Rate 0 bit definition
	PLL_CLKSEL_CNN =            PLL_CLKSEL_R1_OFFSET + 0,   //< Clock Select Module ID: CNN
	PLL_CLKSEL_IVE =            PLL_CLKSEL_R1_OFFSET + 2,   //< Clock Select Module ID: IVE
	PLL_CLKSEL_IPE =            PLL_CLKSEL_R1_OFFSET + 4,   //< Clock Select Module ID: IPE
	PLL_CLKSEL_DIS =            PLL_CLKSEL_R1_OFFSET + 8,   //< Clock Select Module ID: DIS
	PLL_CLKSEL_IME =            PLL_CLKSEL_R1_OFFSET + 12,  //< Clock Select Module ID: IME
	PLL_CLKSEL_CNN2=            PLL_CLKSEL_R1_OFFSET + 16,  //< Clock Select Module ID: CNN2
	PLL_CLKSEL_SDE=             PLL_CLKSEL_R1_OFFSET + 18,  //< Clock Select Module ID: SDE
	PLL_CLKSEL_MDBC=			PLL_CLKSEL_R1_OFFSET + 20,  //< Clock Select Module ID: MDBC
	PLL_CLKSEL_VPE=		    	PLL_CLKSEL_R1_OFFSET + 22,  //< Clock Select Module ID: VPE
	PLL_CLKSEL_ISE =            PLL_CLKSEL_R1_OFFSET + 24,  //< Clock Select Module ID: SIE
	PLL_CLKSEL_DCE =            PLL_CLKSEL_R1_OFFSET + 28,  //< Clock Select Module ID: DCE

	//IPP Clock Rate 1 bit definition
	PLL_CLKSEL_IFE =            PLL_CLKSEL_R2_OFFSET + 0,   //< Clock Select Module ID: IFE
	PLL_CLKSEL_IFE2 =           PLL_CLKSEL_R2_OFFSET + 4,   //< Clock Select Module ID: IFE2
	PLL_CLKSEL_SIE_MCLKSRC =    PLL_CLKSEL_R2_OFFSET + 8,   //< Clock Select Module ID: SIE MCLK source
	PLL_CLKSEL_SIE_MCLK2SRC =   PLL_CLKSEL_R2_OFFSET + 10,  //< Clock Select Module ID: SIE MCLK2 source
	PLL_CLKSEL_SIE_MCLK3SRC =   PLL_CLKSEL_R2_OFFSET + 12,  //< Clock Select Module ID: SIE MCLK3 source
	PLL_CLKSEL_SIE_IO_PXCLKSRC =PLL_CLKSEL_R2_OFFSET + 14,
	PLL_CLKSEL_SIE2_IO_PXCLKSRC=PLL_CLKSEL_R2_OFFSET + 15,
	PLL_CLKSEL_SIE_PXCLKSRC =   PLL_CLKSEL_R2_OFFSET + 16,  //< Clock Select Module ID: SIE PX CLK source
	PLL_CLKSEL_SIE2_PXCLKSRC =  PLL_CLKSEL_R2_OFFSET + 17,  //< Clock Select Module ID: SIE2 PX CLK source
	PLL_CLKSEL_SIE3_PXCLKSRC =  PLL_CLKSEL_R2_OFFSET + 18,  //< Clock Select Module ID: SIE3 PX CLK source
	PLL_CLKSEL_SIE_MCLKINV =    PLL_CLKSEL_R2_OFFSET + 19,  //< Clock Select Module ID: SIE MCLK output Invert
	PLL_CLKSEL_SIE4_IO_PXCLKSRC=PLL_CLKSEL_R2_OFFSET + 20,
	PLL_CLKSEL_SIE4_PXCLKSRC =  PLL_CLKSEL_R2_OFFSET + 21,  //< Clock Select Module ID: SIE4 PX CLK source
	PLL_CLKSEL_TGE =            PLL_CLKSEL_R2_OFFSET + 22,  //< Clock Select Module ID: TGE
	PLL_CLKSEL_TGE2 =           PLL_CLKSEL_R2_OFFSET + 23,  //< Clock Select Module ID: TGE2
	PLL_CLKSEL_SIE_MCLK2INV =   PLL_CLKSEL_R2_OFFSET + 24,  //< Clock Select Module ID: SIE2 MCLK output Invert
	PLL_CLKSEL_SIE_MCLK3INV =   PLL_CLKSEL_R2_OFFSET + 25,  //< Clock Select Module ID: SIE3 MCLK output Invert
	PLL_CLKSEL_IFE_SRAM =       PLL_CLKSEL_R2_OFFSET + 26,  //< Clock Select Module ID: IFE SRAM source
	PLL_CLKSEL_CNN_SRAM =       PLL_CLKSEL_R2_OFFSET + 28,  //< Clock Select Module ID: CNN SRAM source
	PLL_CLKSEL_NUE2 =           PLL_CLKSEL_R2_OFFSET + 30,  //< Clock Select Module ID: NUE2

	//CODEC Clock Rate bit definition
	PLL_CLKSEL_JPEG =           PLL_CLKSEL_R3_OFFSET + 0,   //< Clock Select Module ID: JPEG
	PLL_CLKSEL_VENC =           PLL_CLKSEL_R3_OFFSET + 4,   //< Clock Select Module ID: VENC
	PLL_CLKSEL_GRAPHIC =        PLL_CLKSEL_R3_OFFSET + 8,   //< Clock Select Module ID: GRAPHIC
	PLL_CLKSEL_AFFINE =         PLL_CLKSEL_R3_OFFSET + 10,  //< Clock Select Module ID: AFFINE
	PLL_CLKSEL_GRAPHIC2 =       PLL_CLKSEL_R3_OFFSET + 12,  //< Clock Select Module ID: GRAPHIC2
	PLL_CLKSEL_SIE3_CLKSRC =    PLL_CLKSEL_R3_OFFSET + 16,  //< Clock Select Module ID: SIE3 CLK source
	PLL_CLKSEL_CRYPTO =         PLL_CLKSEL_R3_OFFSET + 20,  //< Clock Select Module ID: CRYPTO
	PLL_CLKSEL_RSA =            PLL_CLKSEL_R3_OFFSET + 22,  //< Clock Select Module ID: RSA
	PLL_CLKSEL_SIE_CLKSRC =     PLL_CLKSEL_R3_OFFSET + 24,  //< Clock Select Module ID: SIE CLK source
	PLL_CLKSEL_SIE2_CLKSRC =    PLL_CLKSEL_R3_OFFSET + 28,  //< Clock Select Module ID: SIE2 CLK source

	//Peripheral Clock Rate bit definition
	PLL_CLKSEL_SDIO =           PLL_CLKSEL_R4_OFFSET + 4,   //< Clock Select Module ID: SDIO
	PLL_CLKSEL_SDIO2 =          PLL_CLKSEL_R4_OFFSET + 8,   //< Clock Select Module ID: SDIO2
	PLL_CLKSEL_MIPI_LVDS =      PLL_CLKSEL_R4_OFFSET + 12,  //< Clock Select Module ID: MIPI
	PLL_CLKSEL_LVDS_CLKPHASE =  PLL_CLKSEL_R4_OFFSET + 13,  //< Clock Select Module ID: MIPI/LVDS CLK Phase
	PLL_CLKSEL_LVDS2_CLKPHASE = PLL_CLKSEL_R4_OFFSET + 14,  //< Clock Select Module ID: MIPI2/LVDS2 CLK Phase
	PLL_CLKSEL_MIPI_LVDS2 =     PLL_CLKSEL_R4_OFFSET + 15,  //< Clock Select Module ID: MIPI2
	PLL_CLKSEL_IDE_CLKSRC =     PLL_CLKSEL_R4_OFFSET + 16,  //< Clock Select Module ID: IDE clock source
	PLL_CLKSEL_DSI_LPSRC =      PLL_CLKSEL_R4_OFFSET + 20,  //< Clock Select Module ID: DSI LP Source
	PLL_CLKSEL_ADO_MCLKSEL =    PLL_CLKSEL_R4_OFFSET + 22,  //< Clock Select Module ID: AUDIO MCLK SEL
	PLL_CLKSEL_ETH =            PLL_CLKSEL_R4_OFFSET + 24,  //< Clock Select Module ID: Ethernet

	//Peripheral Clock Rate bit definition 1
	PLL_CLKSEL_SDIO3 =          PLL_CLKSEL_R5_OFFSET + 0,   //< Clock Select Module ID: SDIO3
	PLL_CLKSEL_TSE =            PLL_CLKSEL_R5_OFFSET + 4,   //< Clock Select Module ID: TSE
	PLL_CLKSEL_SP =             PLL_CLKSEL_R5_OFFSET + 8,   //< Clock Select Module ID: SP
	PLL_CLKSEL_SP2 =            PLL_CLKSEL_R5_OFFSET + 10,  //< Clock Select Module ID: SP2
	PLL_CLKSEL_ETH_REFCLK_INV = PLL_CLKSEL_R5_OFFSET + 14, //< Clock Select Module ID: ETH reference clock invert
	PLL_CLKSEL_HASH =           PLL_CLKSEL_R5_OFFSET + 16, //< Clock Select Module ID: HASH
	PLL_CLKSEL_TRNG =           PLL_CLKSEL_R5_OFFSET + 18, //< Clock Select Module ID: TRNG
	PLL_CLKSEL_TRNGRO_CLKSRC =  PLL_CLKSEL_R5_OFFSET + 19, //< Clock Select Module ID: TRNG RO CLK source
	PLL_CLKSEL_DRTC =           PLL_CLKSEL_R5_OFFSET + 20, //< Clock Select Module ID: DRTC
	PLL_CLKSEL_REMOTE =         PLL_CLKSEL_R5_OFFSET + 21, //< Clock Select Module ID: Remote
	PLL_CLKSEL_ADC_PD =         PLL_CLKSEL_R5_OFFSET + 24, //< Clock Select Module ID: ADC @ PowerDown 1/2/3
	PLL_CLKSEL_ETHPHY_CLKSRC =  PLL_CLKSEL_R5_OFFSET + 26, //< Clock Select Module ID: ETH PHY Reference Clock Source

	//Peripheral Clock Mux Register
	PLL_CLKSEL_CSILPCLK_D0 =    PLL_CLKSEL_R6_OFFSET + 0,  ///< MIPI CSI LP CLK SRC D0
	PLL_CLKSEL_CSILPCLK_D1 =    PLL_CLKSEL_R6_OFFSET + 2,  ///< MIPI CSI LP CLK SRC D1
	PLL_CLKSEL_CSILPCLK_D2 =    PLL_CLKSEL_R6_OFFSET + 4,  ///< MIPI CSI LP CLK SRC D2
	PLL_CLKSEL_CSILPCLK_D3 =    PLL_CLKSEL_R6_OFFSET + 6,  ///< MIPI CSI LP CLK SRC D3

	//IPP Clock Rate 2
	PLL_CLKSEL_SIE4_CLKSRC =    PLL_CLKSEL_R7_OFFSET + 0,  //< Clock Select Module ID: SIE4 CLK source
	PLL_CLKSEL_SIE5_CLKSRC =    PLL_CLKSEL_R7_OFFSET + 4,  //< Clock Select Module ID: SIE5 CLK source
	PLL_CLKSEL_SIE4_CLKDIV =    PLL_CLKSEL_R7_OFFSET + 16, //< Clock Select Module ID: SIE4 CLK divider
	PLL_CLKSEL_SIE5_CLKDIV =    PLL_CLKSEL_R7_OFFSET + 24, //< Clock Select Module ID: SIE5 CLK divider

	//IPP Clock Divider bit definition
	PLL_CLKSEL_SIE_MCLKDIV =    PLL_CLKSEL_R8_OFFSET + 0,   //< Clock Select Module ID: SIE MCLK divider
	PLL_CLKSEL_SIE_MCLK2DIV =   PLL_CLKSEL_R8_OFFSET + 8,   //< Clock Select Module ID: SIE MCLK2 divider
	PLL_CLKSEL_SIE_CLKDIV =     PLL_CLKSEL_R8_OFFSET + 16,  //< Clock Select Module ID: SIE CLK divider
	PLL_CLKSEL_SIE2_CLKDIV =    PLL_CLKSEL_R8_OFFSET + 24,  //< Clock Select Module ID: SIE2 CLK divider

	//Video Clock Divider bit definition
	PLL_CLKSEL_IDE_CLKDIV =     PLL_CLKSEL_R9_OFFSET + 0,   //< Clock Select Module ID: IDE clock divider
	PLL_CLKSEL_IDE_OUTIF_CLKDIV = PLL_CLKSEL_R9_OFFSET + 8, //< Clock Select Module ID: IDE Output Interface clock divider
	PLL_CLKSEL_SIE_MCLK3DIV =   PLL_CLKSEL_R9_OFFSET + 16,  //< Clock Select Module ID: SIE MCLK3 divider
	PLL_CLKSEL_TRNG_CLKDIV =    PLL_CLKSEL_R9_OFFSET + 24,  //< Clock Select Module ID: TRNG clock divider

	//Peripheral Clock Divider bit definition
	PLL_CLKSEL_SP_CLKDIV    =   PLL_CLKSEL_R10_OFFSET + 0,  //< Clock Select Module ID: Special clock divider
	PLL_CLKSEL_SIE3_CLKDIV =    PLL_CLKSEL_R10_OFFSET + 8,  //< Clock Select Module ID: SIE3 CLK divider
	PLL_CLKSEL_ADO_CLKDIV   =   PLL_CLKSEL_R10_OFFSET + 16, //< Clock Select Module ID: audio clock divider
	PLL_CLKSEL_ADO_OSR_CLKDIV = PLL_CLKSEL_R10_OFFSET + 24, //< Clock Select Module ID: audio OSR clock divider

	//SDIO Clock Divider bit definition
	PLL_CLKSEL_SDIO_CLKDIV =    PLL_CLKSEL_R11_OFFSET + 0,  //< Clock Select Module ID: SDIO clock divider
	PLL_CLKSEL_SDIO2_CLKDIV =   PLL_CLKSEL_R11_OFFSET + 16, //< Clock Select Module ID: SDIO2 clock divider

	//Peripheral Clock Divider bit definition 1
	PLL_CLKSEL_SDIO3_CLKDIV =   PLL_CLKSEL_R12_OFFSET + 0,  //< Clock Select Module ID: SDIO3 clock divider
	PLL_CLKSEL_NAND_CLKDIV  =   PLL_CLKSEL_R12_OFFSET + 12, //< Clock Select Module ID: SDIO3 clock divider
	PLL_CLKSEL_SP2_CLKDIV   =   PLL_CLKSEL_R12_OFFSET + 24, //< Clock Select Module ID: Special clock2 divider

	//SPI Clock Divider bit definition
	PLL_CLKSEL_SPI_CLKDIV =     PLL_CLKSEL_R13_OFFSET + 0,  //< Clock Select Module ID: SPI clock divider
	PLL_CLKSEL_SPI2_CLKDIV =    PLL_CLKSEL_R13_OFFSET + 16, //< Clock Select Module ID: SPI2 clock divider

	//SPI Clock Divider bit definition 1
	PLL_CLKSEL_SPI3_CLKDIV =    PLL_CLKSEL_R14_OFFSET + 0,  //< Clock Select Module ID: SPI3 clock divider
	PLL_CLKSEL_SPI4_CLKDIV =    PLL_CLKSEL_R14_OFFSET + 16, //< Clock Select Module ID: SPI4 clock divider

	// UART Clock Divider bit definition
	PLL_CLKSEL_UART2_CLKDIV =   PLL_CLKSEL_R15_OFFSET + 0,  //< Clock Select Module ID: UART2 clock divider
	PLL_CLKSEL_UART3_CLKDIV =   PLL_CLKSEL_R15_OFFSET + 8,  //< Clock Select Module ID: UART3 clock divider
	PLL_CLKSEL_UART4_CLKDIV =   PLL_CLKSEL_R15_OFFSET + 16, //< Clock Select Module ID: UART4 clock divider
	PLL_CLKSEL_UART5_CLKDIV =   PLL_CLKSEL_R15_OFFSET + 24, //< Clock Select Module ID: UART5 clock divider

	// TRNG RO Clock bit definition
	PLL_CLKSEL_TRNG_RO_CLKDIV = PLL_CLKSEL_R20_OFFSET + 0,  //< Clock Select Module ID: TRNG RO clock divider
	PLL_CLKSEL_TRNG_RO_DELAY =  PLL_CLKSEL_R20_OFFSET + 8,  //< Clock Select Module ID: TRNG RO Delay
	PLL_CLKSEL_UART6_CLKDIV =   PLL_CLKSEL_R20_OFFSET + 16, //< Clock Select Module ID: UART6 clock divider
	PLL_CLKSEL_ADO_MCLKDIV =    PLL_CLKSEL_R20_OFFSET + 24, //< Clock Select Module ID: ADO MCLK clock divider

	//RO32K Clock Divider bit definition 2
	PLL_CLKSEL_RO32K_CLKDIV =    PLL_CLKSEL_R21_OFFSET + 0,  //< Clock Select Module ID: SPI5 clock divider


	PLL_CLKSEL_MAX_ITEM,									// magic number for pll checking usage.
	ENUM_DUMMY4WORD(PLL_CLKSEL)
} PLL_CLKSEL;

#define PLL_CLKSEL_H26X			PLL_CLKSEL_H265


/*
    @name   CPU clock rate value

    CPU clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CPU).
*/
//@{
#define PLL_CLKSEL_CPU_80         (0x00 << PLL_CLKSEL_CPU)      //< Select CPU clock 80MHz
#define PLL_CLKSEL_CPU_PLL8       (0x01 << PLL_CLKSEL_CPU)      //< Select CPU clock PLL8 (for CPU)
#define PLL_CLKSEL_CPU_APLL       PLL_CLKSEL_CPU_PLL8           //< Select CPU clock APLL
#define PLL_CLKSEL_CPU_480        (0x02 << PLL_CLKSEL_CPU)      //< Select CPU clock 480MHz
//@}

/*
    @name   APB clock rate value

    APB clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_APB).
*/
//@{
#define PLL_CLKSEL_APB_48         (0x00 << PLL_CLKSEL_APB)      //< Select APB  48MHz
#define PLL_CLKSEL_APB_60         (0x01 << PLL_CLKSEL_APB)      //< Select APB  60MHz
#define PLL_CLKSEL_APB_80         (0x02 << PLL_CLKSEL_APB)      //< Select APB  80MHz
#define PLL_CLKSEL_APB_120        (0x03 << PLL_CLKSEL_APB)      //< Select APB 120MHz
//@}

/*
    @name   Power Down mode source clock selection

    Power Down mode source clock selection
*/
//@{
#define PLL_CLKSEL_PD_RTC        (0x00 << PLL_CLKSEL_PD)      //< Select PD source RTC
#define PLL_CLKSEL_PD_RINGOSC    (0x01 << PLL_CLKSEL_PD)      //< Select PD source RingOSC
//@}

/*
    @name   DMA Arbiter clock rate value

    DMA Arbiter clock rate value

    @note This is for pll_setClockRate(PLL_CLKSEL_DMA_ARRT).
*/
//@{
#define PLL_CLKSEL_DMA_ARBT_DDR1  (0x00)      //< Select DMA Arbiter DDR1 PHY
#define PLL_CLKSEL_DMA_ARBT_DDR2  (0x01)      //< Select DMA Arbiter DDR2 PHY
//@}

/*
    @name   DDR-PHY clock rate value

    DDR-PHY clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DDRPHY).
*/
//@{
#define PLL_CLKSEL_DDRPHY_PLL3    (0x00 << PLL_CLKSEL_DDRPHY)      //< Select DDRPHY clock from PLL3
#define PLL_CLKSEL_DDRPHY_ETHPHY  (0x01 << PLL_CLKSEL_DDRPHY)      //< Select DDRPHY clock from Ethernet PHY
//@}

/*
    @name   CNN clock rate value

    CNN clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CNN).
*/
//@{
#define PLL_CLKSEL_CNN_240        (0x00 << (PLL_CLKSEL_CNN - PLL_CLKSEL_R1_OFFSET)) //< Select CNN clock as 240MHz
#define PLL_CLKSEL_CNN_320        (0x01 << (PLL_CLKSEL_CNN - PLL_CLKSEL_R1_OFFSET)) //< Select CNN clock as 320MHz
#define PLL_CLKSEL_CNN_PLL12      (0x02 << (PLL_CLKSEL_CNN - PLL_CLKSEL_R1_OFFSET)) //< Select CNN clock as PLL12
#define PLL_CLKSEL_CNN_PLL9       (0x03 << (PLL_CLKSEL_CNN - PLL_CLKSEL_R1_OFFSET)) //< Select CNN clock as PLL9
//@}

/*
    @name   IVE clock rate value

    IVE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IVE).
*/
//@{
#define PLL_CLKSEL_IVE_240        (0x00 << (PLL_CLKSEL_IVE - PLL_CLKSEL_R1_OFFSET)) //< Select IVE clock as 192MHz
#define PLL_CLKSEL_IVE_320        (0x01 << (PLL_CLKSEL_IVE - PLL_CLKSEL_R1_OFFSET)) //< Select IVE clock as 320MHz
#define PLL_CLKSEL_IVE_480        (0x02 << (PLL_CLKSEL_IVE - PLL_CLKSEL_R1_OFFSET)) //< Select IVE clock as 480MHz
//@}

/*
    @name   IPE clock rate value

    IPE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IPE).
*/
//@{
#define PLL_CLKSEL_IPE_192        (0x00 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as 192MHz
#define PLL_CLKSEL_IPE_240        (0x01 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as 240MHz
#define PLL_CLKSEL_IPE_96         (0x02 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as 96MHz
#define PLL_CLKSEL_IPE_PLL12      (0x03 << (PLL_CLKSEL_IPE - PLL_CLKSEL_R1_OFFSET)) //< Select IPE clock as PLL12
//@}

/*
    @name   DIS clock rate value

    DIS clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DIS).
*/
//@{
#define PLL_CLKSEL_DIS_192        (0x00 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Select DIS clock as 192MHz
#define PLL_CLKSEL_DIS_240        (0x01 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Select DIS clock as 240MHz
#define PLL_CLKSEL_DIS_96         (0x02 << (PLL_CLKSEL_DIS - PLL_CLKSEL_R1_OFFSET)) //< Select DIS clock as 96MHz
//@}

/*
    @name   IME clock rate value

    IME clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IME).
*/
//@{
#define PLL_CLKSEL_IME_192        (0x00 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Select IME clock as 192MHz
#define PLL_CLKSEL_IME_240        (0x01 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Select IME clock as 240MHz
#define PLL_CLKSEL_IME_96         (0x02 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Select IME clock as 96MHz
#define PLL_CLKSEL_IME_PLL12      (0x03 << (PLL_CLKSEL_IME - PLL_CLKSEL_R1_OFFSET)) //< Select IME clock as PLL12
//@}

/*
    @name   CNN2 clock rate value

    CNN2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CNN2).
*/
//@{
#define PLL_CLKSEL_CNN2_240        (0x00 << (PLL_CLKSEL_CNN2 - PLL_CLKSEL_R1_OFFSET)) //< Select CNN2 clock as 192MHz
#define PLL_CLKSEL_CNN2_320        (0x01 << (PLL_CLKSEL_CNN2 - PLL_CLKSEL_R1_OFFSET)) //< Select CNN2 clock as 320MHz
#define PLL_CLKSEL_CNN2_480        (0x02 << (PLL_CLKSEL_CNN2 - PLL_CLKSEL_R1_OFFSET)) //< Select CNN2 clock as 480MHz
#define PLL_CLKSEL_CNN2_PLL10      (0x03 << (PLL_CLKSEL_CNN2 - PLL_CLKSEL_R1_OFFSET)) //< Select CNN2 clock as PLL10
//@}

/*
    @name   MDBC clock rate value

    MDBC clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_MDBC).
*/
//@{
#define PLL_CLKSEL_MDBC_192        (0x00 << (PLL_CLKSEL_MDBC - PLL_CLKSEL_R1_OFFSET)) //< Select MDBC clock as 192MHz
#define PLL_CLKSEL_MDBC_240        (0x01 << (PLL_CLKSEL_MDBC - PLL_CLKSEL_R1_OFFSET)) //< Select MDBC clock as 240MHz
#define PLL_CLKSEL_MDBC_96         (0x02 << (PLL_CLKSEL_MDBC - PLL_CLKSEL_R1_OFFSET)) //< Select MDBC clock as 96MHz
//@}


/*
    @name   ISE clock rate value

    ISE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ISE).
*/
//@{
#define PLL_CLKSEL_ISE_240        (0x00 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Select ISE clock as 192MHz
#define PLL_CLKSEL_ISE_320        (0x01 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Select ISE clock as 320MHz
#define PLL_CLKSEL_ISE_480        (0x02 << (PLL_CLKSEL_ISE - PLL_CLKSEL_R1_OFFSET)) //< Select ISE clock as 480MHz
//@}

/*
    @name   DCE clock rate value

    DCE clock rate value

    This is for pll_set_clock_rate(PLL_CLKSEL_DCE).
*/
//@{
#define PLL_CLKSEL_DCE_192        (0x00 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Select DCE clock as 192MHz
#define PLL_CLKSEL_DCE_240        (0x01 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Select DCE clock as 240MHz
#define PLL_CLKSEL_DCE_96         (0x02 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Select DCE clock as 96MHz
#define PLL_CLKSEL_DCE_PLL12      (0x03 << (PLL_CLKSEL_DCE - PLL_CLKSEL_R1_OFFSET)) //< Select DCE clock as PLL12
//@}

/*
    @name   IFE clock rate value

    IFE clock rate value

    This is for pll_set_clock_rate(PLL_CLKSEL_IFE).
*/
//@{
#define PLL_CLKSEL_IFE_192        (0x00 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Select IFE clock as 192MHz
#define PLL_CLKSEL_IFE_240        (0x01 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Select IFE clock as 240MHz
#define PLL_CLKSEL_IFE_96         (0x02 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Select IFE clock as 96MHz
#define PLL_CLKSEL_IFE_PLL12      (0x03 << (PLL_CLKSEL_IFE - PLL_CLKSEL_R2_OFFSET)) //< Select IFE clock as PLL12
//@}


/*
    @name   SIE MCLK clock source value

    SIE MCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE_MCLKSRC_480		(0x00 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R2_OFFSET))   //< Select SIE MCLK clock source as 480 MHz (PLL1)
#define PLL_CLKSEL_SIE_MCLKSRC_PLL5		(0x01 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R2_OFFSET))   //< Select SIE MCLK clock source as PLL5 (for sensor1)
#define PLL_CLKSEL_SIE_MCLKSRC_320	        (0x02 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R2_OFFSET))   //< Select SIE MCLK clock source as 320MHz
#define PLL_CLKSEL_SIE_MCLKSRC_PLL12	        (0x03 << (PLL_CLKSEL_SIE_MCLKSRC - PLL_CLKSEL_R2_OFFSET))   //< Select SIE MCLK clock source as PLL12 (for sensor2)
//@}

/*
    @name   SIE MCLK2 clock source value

    SIE MCLK2 clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC).
*/
//@{
#define PLL_CLKSEL_SIE_MCLK2SRC_480		(0x00 << (PLL_CLKSEL_SIE_MCLK2SRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE MCLK2 clock source as 480 MHz
#define PLL_CLKSEL_SIE_MCLK2SRC_PLL5            (0x01 << (PLL_CLKSEL_SIE_MCLK2SRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE MCLK2 clock source as PLL5 (for sensor1)
#define PLL_CLKSEL_SIE_MCLK2SRC_320             (0x02 << (PLL_CLKSEL_SIE_MCLK2SRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE MCLK2 clock source as 320MHz
#define PLL_CLKSEL_SIE_MCLK2SRC_PLL12           (0x03 << (PLL_CLKSEL_SIE_MCLK2SRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE MCLK2 clock source as PLL12 (for sensor2)
//@}

/*
    @name   SIE MCLK3 clock source value

    SIE MCLK3 clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC).
*/
//@{
#define PLL_CLKSEL_SIE_MCLK3SRC_480             (0x00 << (PLL_CLKSEL_SIE_MCLK3SRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE MCLK3 clock source as 480 MHz
#define PLL_CLKSEL_SIE_MCLK3SRC_PLL5            (0x01 << (PLL_CLKSEL_SIE_MCLK3SRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE MCLK3 clock source as PLL5 (for sensor1)
#define PLL_CLKSEL_SIE_MCLK3SRC_320             (0x02 << (PLL_CLKSEL_SIE_MCLK3SRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE MCLK3 clock source as 320MHz
#define PLL_CLKSEL_SIE_MCLK3SRC_PLL12           (0x03 << (PLL_CLKSEL_SIE_MCLK3SRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE MCLK3 clock source as PLL12 (for sensor2)
//@}

/*
    @name   SIE IO PXCLK clock source value

    SIE IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_IO_PXCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE_IO_PXCLKSRC_PXCLKPAD    (0x00 << (PLL_CLKSEL_SIE_IO_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))
#define PLL_CLKSEL_SIE_IO_PXCLKSRC_PXCLKPAD2   (0x01 << (PLL_CLKSEL_SIE_IO_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))
//@}

/*
    @name   SIE2 IO PXCLK clock source value

    SIE2 IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE2_IO_PXCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE2_IO_PXCLKSRC_PXCLKPAD2  (0x00 << (PLL_CLKSEL_SIE2_IO_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))
#define PLL_CLKSEL_SIE2_IO_PXCLKSRC_PXCLKPAD   (0x01 << (PLL_CLKSEL_SIE2_IO_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))
//@}

/*
    @name   SIE PLL CLK clock source value

    SIE PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_PXCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE_PXCLKSRC_PXCLKPAD    (0x00 << (PLL_CLKSEL_SIE_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))  //< Select SIE PX CLK source as PXCLK PAD
#define PLL_CLKSEL_SIE_PXCLKSRC_MCLK        (0x01 << (PLL_CLKSEL_SIE_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))  //< Select SIE PX CLK source as PLL_CLKSEL_SIE_MCLKSRC / (PLL_SIE_MCLKDIV+1)
//@}


/*
    @name   SIE2 PLL CLK clock source value

    SIE2 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE2_PXCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE2_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE2_PXCLKSRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE2 PX CLK source as PXCLK2 PAD
#define PLL_CLKSEL_SIE2_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE2_PXCLKSRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE2 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)
//@}

/*
    @name   SIE3 PXCLK source value

    SIE3 PXCLK source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE3_PXCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE3_PXCLKSRC_CCIR2_PXCLK (0x00 << (PLL_CLKSEL_SIE3_PXCLKSRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE3 PX CLK source as PXCLK3 PAD
#define PLL_CLKSEL_SIE3_PXCLKSRC_MCLK        (0x01 << (PLL_CLKSEL_SIE3_PXCLKSRC - PLL_CLKSEL_R2_OFFSET)) //< Select SIE3 PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLK2DIV+1)
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
    @name   SIE4 IO PXCLK clock source value

    SIE4 IO PXCLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE4_IO_PXCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE4_IO_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE4_IO_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))
#define PLL_CLKSEL_SIE4_IO_PXCLKSRC_PXCLKPAD2  (0x01 << (PLL_CLKSEL_SIE4_IO_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))
//@}

/*
    @name   SIE4 PLL CLK clock source value

    SIE4 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE4_PXCLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE4_PXCLKSRC_PXCLKPAD   (0x00 << (PLL_CLKSEL_SIE4_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))
#define PLL_CLKSEL_SIE4_PXCLKSRC_MCLK       (0x01 << (PLL_CLKSEL_SIE4_PXCLKSRC - PLL_CLKSEL_R2_OFFSET))
//@}

/*
    @name   TGE PLL CLK clock source value

    TGE PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TGE).
*/
//@{
#define PLL_CLKSEL_TGE_PXCLKSRC_PXCLKPAD    (0x00 << (PLL_CLKSEL_TGE - PLL_CLKSEL_R2_OFFSET))  //< Select TGE PX CLK source as PXCLK PAD
#define PLL_CLKSEL_TGE_PXCLKSRC_MCLK        (0x01 << (PLL_CLKSEL_TGE - PLL_CLKSEL_R2_OFFSET))  //< Select TGE PX CLK source as PLL_CLKSEL_SIE_MCLKSRC / (PLL_SIE_MCLKDIV+1)
//@}

/*
    @name   TGE2 PLL CLK clock source value

    TGE2 PLL CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TGE2).
*/
//@{
#define PLL_CLKSEL_TGE2_PXCLKSRC_PXCLKPAD    (0x00 << (PLL_CLKSEL_TGE2 - PLL_CLKSEL_R2_OFFSET))  //< Select TGE PX CLK source as PXCLK PAD
#define PLL_CLKSEL_TGE2_PXCLKSRC_MCLK2       (0x01 << (PLL_CLKSEL_TGE2 - PLL_CLKSEL_R2_OFFSET))  //< Select TGE PX CLK source as PLL_CLKSEL_SIE_MCLK2SRC / (PLL_SIE_MCLKDIV+1)
//@}

/*
    @name   SIE MCLK2 clock Inverted Output

    Invert output phase of SIE MCLK2

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2INV).
*/
//@{
#define PLL_CLKSEL_SIE_MCLK2INV_DIS     (0x00 << (PLL_CLKSEL_SIE_MCLK2INV - PLL_CLKSEL_R2_OFFSET)) //< Select SIE2 MCLK normal output
#define PLL_CLKSEL_SIE_MCLK2INV_EN      (0x01 << (PLL_CLKSEL_SIE_MCLK2INV - PLL_CLKSEL_R2_OFFSET)) //< Select SIE2 MCLK inverted output
//@}

/*
    @name   SIE MCLK3 clock Inverted Output

    Invert output phase of SIE MCLK3

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3INV).
*/
//@{
#define PLL_CLKSEL_SIE_MCLK3INV_DIS     (0x00 << (PLL_CLKSEL_SIE_MCLK3INV - PLL_CLKSEL_R2_OFFSET)) //< Select SIE2 MCLK normal output
#define PLL_CLKSEL_SIE_MCLK3INV_EN      (0x01 << (PLL_CLKSEL_SIE_MCLK3INV - PLL_CLKSEL_R2_OFFSET)) //< Select SIE2 MCLK inverted output
//@}

/*
    @name   IFE Shared SRAM clock selection

    SRAM clock selection of IFE
*/
//@{
#define PLL_CLKSEL_IFE_SRAM_CPU         (0x00 << (PLL_CLKSEL_IFE_SRAM - PLL_CLKSEL_R2_OFFSET)) //< Select IFE SRAM clock from CPU
#define PLL_CLKSEL_IFE_SRAM_IFE         (0x01 << (PLL_CLKSEL_IFE_SRAM - PLL_CLKSEL_R2_OFFSET)) //< Select IFE SRAM clock from IFE
//@}

/*
    @name   CNN Shared SRAM clock selection

    SRAM clock selection of IFE
*/
//@{
#define PLL_CLKSEL_CNN_SRAM_CNN         (0x00 << (PLL_CLKSEL_CNN_SRAM - PLL_CLKSEL_R2_OFFSET)) //< Select CNN SRAM clock from CNN
#define PLL_CLKSEL_CNN_SRAM_DCE         (0x01 << (PLL_CLKSEL_CNN_SRAM - PLL_CLKSEL_R2_OFFSET)) //< Select CNN SRAM clock from DCE
#define PLL_CLKSEL_CNN_SRAM_NUE         (0x03 << (PLL_CLKSEL_CNN_SRAM - PLL_CLKSEL_R2_OFFSET)) //< Select CNN SRAM clock from CNN
//@}


/*
    @name   NUE2 clock rate value

    NUE2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_NUE2).
*/
//@{
#define PLL_CLKSEL_NUE2_240        (0x00 << (PLL_CLKSEL_NUE2 - PLL_CLKSEL_R2_OFFSET)) //< Select NUE2 clock as 240MHz
#define PLL_CLKSEL_NUE2_320        (0x01 << (PLL_CLKSEL_NUE2 - PLL_CLKSEL_R2_OFFSET)) //< Select NUE2 clock as 320MHz
#define PLL_CLKSEL_NUE2_480        (0x02 << (PLL_CLKSEL_NUE2 - PLL_CLKSEL_R2_OFFSET)) //< Select NUE2 clock as 480MHz
//@}

/*
    @name   JPEG clock rate value

    JPEG clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_JPEG).
*/
//@{
#define PLL_CLKSEL_JPEG_240       (0x00 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Select JPEG clock as 240MHz
#define PLL_CLKSEL_JPEG_320       (0x01 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Select JPEG clock as 320MHz
#define PLL_CLKSEL_JPEG_480       (0x02 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Select JPEG clock as 480MHz
#define PLL_CLKSEL_JPEG_PLL9      (0x03 << (PLL_CLKSEL_JPEG - PLL_CLKSEL_R3_OFFSET)) //< Select JPEG clock as PLL9
//@}


/*
    @name H26x clock rate value

    H265 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_H265).
*/
//@{
#define PLL_CLKSEL_VENC_240       (0x00 << (PLL_CLKSEL_VENC - PLL_CLKSEL_R3_OFFSET)) //< Select H.265 clock as 240MHz
#define PLL_CLKSEL_VENC_320       (0x01 << (PLL_CLKSEL_VENC - PLL_CLKSEL_R3_OFFSET)) //< Select H.265 clock as 320MHz
#define PLL_CLKSEL_VENC_96        (0x02 << (PLL_CLKSEL_VENC - PLL_CLKSEL_R3_OFFSET)) //< Select H.264 clock as 96NHz
#define PLL_CLKSEL_VENC_PLL12     (0x03 << (PLL_CLKSEL_VENC - PLL_CLKSEL_R3_OFFSET)) //< Select H.264 clock as PLL12 (for Sensor2)
//@}

/*
    @name   Graphic clock rate value

    Graphic clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_GRAPHIC).
*/
//@{
#define PLL_CLKSEL_GRAPHIC_240      (0x00 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Select Graphic clock as 240Mhz
#define PLL_CLKSEL_GRAPHIC_320      (0x01 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Select Graphic clock as 320Mhz
#define PLL_CLKSEL_GRAPHIC_480      (0x02 << (PLL_CLKSEL_GRAPHIC - PLL_CLKSEL_R3_OFFSET)) //< Select Graphic clock as 480Mhz
//@}

/*
    @name   Affine clock rate value

    Affine clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_AFFINE).
*/
//@{
#define PLL_CLKSEL_AFFINE_240      (0x00 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET)) //< Select Affine clock as 240Mhz
#define PLL_CLKSEL_AFFINE_320      (0x01 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET)) //< Select Affine clock as 320Mhz
#define PLL_CLKSEL_AFFINE_480      (0x02 << (PLL_CLKSEL_AFFINE - PLL_CLKSEL_R3_OFFSET)) //< Select Affine clock as 480Mhz
//@}


/*
    @name   Graphic2 clock rate value

    Graphic2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_GRAPHIC2).
*/
//@{
#define PLL_CLKSEL_GRAPHIC2_240     (0x00 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Select Graphic2 clock as 240Mhz
#define PLL_CLKSEL_GRAPHIC2_320     (0x01 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Select Graphic2 clock as 320Mhz
#define PLL_CLKSEL_GRAPHIC2_480     (0x02 << (PLL_CLKSEL_GRAPHIC2 - PLL_CLKSEL_R3_OFFSET))  //< Select Graphic2 clock as 480Mhz
//@}

/*
    @name   UVCP clock rate value

    UVCP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_UVCP).
*/
//@{
#define PLL_CLKSEL_UVCP_240     (0x00 << (PLL_CLKSEL_UVCP - PLL_CLKSEL_R3_OFFSET))  //< Select UVCP clock as 240Mhz
#define PLL_CLKSEL_UVCP_320     (0x01 << (PLL_CLKSEL_UVCP - PLL_CLKSEL_R3_OFFSET))  //< Select UVCP clock as 320Mhz
#define PLL_CLKSEL_UVCP_120     (0x02 << (PLL_CLKSEL_UVCP - PLL_CLKSEL_R3_OFFSET))  //< Select UVCP clock as 120Mhz
//@}

/*
    @name   SIE3 CLK clock source value

    SIE3 CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE3_CLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE3_CLKSRC_480      (0x00 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE3 CLK clock source as 480 MHz
#define PLL_CLKSEL_SIE3_CLKSRC_PLL5     (0x02 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE3 CLK clock source as PLL5 (for sensor1)
#define PLL_CLKSEL_SIE3_CLKSRC_PLL12    (0x04 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE3 CLK clock source as PLL12 (for sensor2)
#define PLL_CLKSEL_SIE3_CLKSRC_320      (0x05 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE3 CLK clock source as 320Mhz
#define PLL_CLKSEL_SIE3_CLKSRC_192      (0x06 << (PLL_CLKSEL_SIE3_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE3 CLK clock source as 192 MHz
//@}

/*
    @name   Crypto clock rate value

    Crypto clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CRYPTO).
*/
//@{
#define PLL_CLKSEL_CRYPTO_240   (0x00 << (PLL_CLKSEL_CRYPTO - PLL_CLKSEL_R3_OFFSET))    //< Select Crypto clock as 240Mhz
#define PLL_CLKSEL_CRYPTO_320   (0x01 << (PLL_CLKSEL_CRYPTO - PLL_CLKSEL_R3_OFFSET))    //< Select Crypto clock as 320Mhz
#define PLL_CLKSEL_CRYPTO_PLL9  (0x03 << (PLL_CLKSEL_CRYPTO - PLL_CLKSEL_R3_OFFSET))    //< Select Crypto clock as PLL9
//@}

/*
    @name   RSA clock rate value

    RSA clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_RSA).
*/
//@{
#define PLL_CLKSEL_RSA_240   (0x00 << (PLL_CLKSEL_RSA - PLL_CLKSEL_R3_OFFSET))    //< Select RSA clock as 240Mhz
#define PLL_CLKSEL_RSA_320   (0x01 << (PLL_CLKSEL_RSA - PLL_CLKSEL_R3_OFFSET))    //< Select RSA clock as 320Mhz
#define PLL_CLKSEL_RSA_PLL9  (0x03 << (PLL_CLKSEL_RSA - PLL_CLKSEL_R3_OFFSET))    //< Select RSA clock as PLL9
//@}


/*
    @name   SIE CLK clock source value

    SIE CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE_CLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE_CLKSRC_480       (0x00 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R3_OFFSET))    //< Select SIE CLK clock source as 480 MHz
#define PLL_CLKSEL_SIE_CLKSRC_PLL5      (0x02 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R3_OFFSET))    //< Select SIE CLK clock source as PLL5 (for sensor1)
#define PLL_CLKSEL_SIE_CLKSRC_PLL12     (0x04 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE CLK clock source as PLL12 (for sensor2)
#define PLL_CLKSEL_SIE_CLKSRC_320       (0x05 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R3_OFFSET))    //< Select SIE CLK clock source as 320Mhz
#define PLL_CLKSEL_SIE_CLKSRC_192       (0x06 << (PLL_CLKSEL_SIE_CLKSRC - PLL_CLKSEL_R3_OFFSET))    //< Select SIE CLK clock source as 192 MHz
//@}

/*
    @name   SIE2 CLK clock source value

    SIE2 CLK clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SIE2_CLKSRC).
*/
//@{
#define PLL_CLKSEL_SIE2_CLKSRC_480      (0x00 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE2 CLK clock source as 480 MHz
#define PLL_CLKSEL_SIE2_CLKSRC_PLL5     (0x02 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE2 CLK clock source as PLL5 (for sensor1)
#define PLL_CLKSEL_SIE2_CLKSRC_PLL12    (0x04 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE2 CLK clock source as PLL12 (for sensor2)
#define PLL_CLKSEL_SIE2_CLKSRC_320      (0x05 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE2 CLK clock source as 320Mhz
#define PLL_CLKSEL_SIE2_CLKSRC_192      (0x06 << (PLL_CLKSEL_SIE2_CLKSRC - PLL_CLKSEL_R3_OFFSET))   //< Select SIE2 CLK clock source as 192 MHz
//@}

/*
    @name   SDIO clock rate value

    SDIO clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SDIO).
*/
//@{
#define PLL_CLKSEL_SDIO_192          (0x00 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Select SDIO clock as 192Mhz
#define PLL_CLKSEL_SDIO_480          (0x01 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Select SDIO clock as 480Mhz
#define PLL_CLKSEL_SDIO_320          (0x02 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Select SDIO clock as 320Mhz
#define PLL_CLKSEL_SDIO_PLL9         (0x03 << (PLL_CLKSEL_SDIO - PLL_CLKSEL_R4_OFFSET))  //< Select SDIO clock as PLL9
//@}

/*
    @name   SDIO2 clock rate value

    SDIO2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SDIO2).
*/
//@{
#define PLL_CLKSEL_SDIO2_192         (0x00 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Select SDIO2 clock as 192Mhz
#define PLL_CLKSEL_SDIO2_480         (0x01 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Select SDIO2 clock as 480Mhz
#define PLL_CLKSEL_SDIO2_320         (0x02 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Select SDIO2 clock as 320Mhz
#define PLL_CLKSEL_SDIO2_PLL9        (0x03 << (PLL_CLKSEL_SDIO2 - PLL_CLKSEL_R4_OFFSET)) //< Select SDIO2 clock as PLL9
//@}

/*
    @name MIPI/LVDS clock rate value

    MIPI/LVDS clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS - PLL_CLKSEL_R4_OFFSET))     //< Select MIPI/LVDS clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS - PLL_CLKSEL_R4_OFFSET))     //< Select MIPI/LVDS clock as 120Mhz
//@}

/*
    @name   LVDS Clock Phase Select

    LVDS Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS_CLKPHASE).
*/
//@{
#define PLL_LVDS_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS_CLKPHASE - PLL_CLKSEL_R4_OFFSET))    //< Select LVDS Clock Phase as Normal (non-invert)
#define PLL_LVDS_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS_CLKPHASE - PLL_CLKSEL_R4_OFFSET))    //< Select LVDS Clock Phase as Invert
//@}

/*
    @name   LVDS2 Clock Phase Select

    LVDS2 Clock Phase Select

    @note This is for pll_set_clock_rate(PLL_CLKSEL_LVDS2_CLKPHASE).
*/
//@{
#define PLL_LVDS2_CLK_PHASE_NORMAL   (0x00 << (PLL_CLKSEL_LVDS2_CLKPHASE - PLL_CLKSEL_R4_OFFSET))   //< Select LVDS2 Clock Phase as Normal (non-invert)
#define PLL_LVDS2_CLK_PHASE_INVERT   (0x01 << (PLL_CLKSEL_LVDS2_CLKPHASE - PLL_CLKSEL_R4_OFFSET))   //< Select LVDS2 Clock Phase as Invert
//@}


/*
    @name MIPI/LVDS2 clock rate value

    MIPI/LVDS2 clock rate value

    @note This is used for pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS2).
*/
//@{
#define PLL_CLKSEL_MIPI_LVDS2_60  (0x00 << (PLL_CLKSEL_MIPI_LVDS2 - PLL_CLKSEL_R4_OFFSET))     //< Select MIPI/LVDS2 clock as 60Mhz
#define PLL_CLKSEL_MIPI_LVDS2_120 (0x01 << (PLL_CLKSEL_MIPI_LVDS2 - PLL_CLKSEL_R4_OFFSET))     //< Select MIPI/LVDS2 clock as 120Mhz
//@}

/*
    @name   IDE clock source value

    IDE clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IDE_CLKSRC).
*/
//@{
#define PLL_CLKSEL_IDE_CLKSRC_480   (0x00 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as 480 MHz
#define PLL_CLKSEL_IDE_CLKSRC_PLL6  (0x01 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as PLL6 (for IDE/ETH)
#define PLL_CLKSEL_IDE_CLKSRC_192   (0x02 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as 192MHz
#define PLL_CLKSEL_IDE_CLKSRC_PLL9  (0x03 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as PLL9 (for IDE/ETH backup)
#define PLL_CLKSEL_IDE_CLKSRC_PLL11 (0x04 << (PLL_CLKSEL_IDE_CLKSRC - PLL_CLKSEL_R4_OFFSET))    //< Select IDE clock source as PLL11 (for DSI)
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
    @name   Audio MCLK clock source

    Audio MCLK clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ADO_MCLKSEL).
*/
//@{
#define PLL_CLKSEL_ADO_MCLKSEL_256FS   (0x00 << (PLL_CLKSEL_ADO_MCLKSEL - PLL_CLKSEL_R4_OFFSET))
#define PLL_CLKSEL_ADO_MCLKSEL_PLL7    (0x01 << (PLL_CLKSEL_ADO_MCLKSEL - PLL_CLKSEL_R4_OFFSET))
//@}


/*
    @name   Ethernet clock source

    Ethernet clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ETH).
*/
//@{
#define PLL_CLKSEL_ETH_PLL6   (0x00 << (PLL_CLKSEL_ETH - PLL_CLKSEL_R4_OFFSET))    //< Select ETH clock as PLL6
#define PLL_CLKSEL_ETH_PLL9   (0x01 << (PLL_CLKSEL_ETH - PLL_CLKSEL_R4_OFFSET))    //< Select ETH clock as PLL9
//@}

/*
    @name   SDIO3 clock rate value

    SDIO3 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SDIO3).
*/
//@{
#define PLL_CLKSEL_SDIO3_192         (0x00 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Select SDIO3 clock as 192Mhz
#define PLL_CLKSEL_SDIO3_480         (0x01 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Select SDIO3 clock as 480Mhz
#define PLL_CLKSEL_SDIO3_320         (0x02 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Select SDIO3 clock as 320Mhz
#define PLL_CLKSEL_SDIO3_PLL9        (0x03 << (PLL_CLKSEL_SDIO3 - PLL_CLKSEL_R5_OFFSET)) //< Select SDIO3 clock as PLL9
//@}

/*
    @name   TSE clock rate value

    TSE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TSMUX).
*/
//@{
#define PLL_CLKSEL_TSE_240          (0x00 << (PLL_CLKSEL_TSE - PLL_CLKSEL_R5_OFFSET))  //< Select TSE clock as 240Mhz
#define PLL_CLKSEL_TSE_320          (0x01 << (PLL_CLKSEL_TSE - PLL_CLKSEL_R5_OFFSET))  //< Select TSE clock as 320Mhz
#define PLL_CLKSEL_TSE_PLL9         (0x03 << (PLL_CLKSEL_TSE - PLL_CLKSEL_R5_OFFSET))  //< Select TSE clock as PLL9
//@}


/*
    @name   SP clock rate value

    SP clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SP).
*/
//@{
#define PLL_CLKSEL_SP_480            (0x00 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Select SP clock as 480Mhz
#define PLL_CLKSEL_SP_PLL5           (0x02 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Select SP clock as PLL5
#define PLL_CLKSEL_SP_PLL6           (0x03 << (PLL_CLKSEL_SP - PLL_CLKSEL_R5_OFFSET))  //< Select SP clock as PLL6
//@}

/*
    @name   SP2 clock rate value

    SP2 clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SP2).
*/
//@{
#define PLL_CLKSEL_SP2_480           (0x00 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET)) //< Select SP2 clock as 240Mhz
#define PLL_CLKSEL_SP2_PLL5          (0x02 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET)) //< Select SP2 clock as PLL5
#define PLL_CLKSEL_SP2_PLL6          (0x03 << (PLL_CLKSEL_SP2 - PLL_CLKSEL_R5_OFFSET)) //< Select SP2 clock as PLL6
//@}

/*
    @name   ETH reference clock invert value

    ETH reference clock invert value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ETH_REFCLK_INV).
*/
//@{
#define PLL_CLKSEL_ETH_REFCLK_INV_DIS    (0x00 << (PLL_CLKSEL_ETH_REFCLK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select ETH reference clock not invert
#define PLL_CLKSEL_ETH_REFCLK_INV_EN     (0x01 << (PLL_CLKSEL_ETH_REFCLK_INV - PLL_CLKSEL_R5_OFFSET))  //< Select ETH reference clock invert
//@}


/*
    @name   HASH clock rate value

    HASH clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_HASH).
*/
//@{
#define PLL_CLKSEL_HASH_240         (0x00 << (PLL_CLKSEL_HASH - PLL_CLKSEL_R5_OFFSET)) //< Select HASH clock as 240Mhz
#define PLL_CLKSEL_HASH_320         (0x01 << (PLL_CLKSEL_HASH - PLL_CLKSEL_R5_OFFSET)) //< Select HASH clock as 320Mhz
#define PLL_CLKSEL_HASH_PLL9        (0x03 << (PLL_CLKSEL_HASH - PLL_CLKSEL_R5_OFFSET)) //< Select HASH clock as PLL9
//@}


/*
    @name   TRNG clock rate value

    TRNG clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TRNG).
*/
//@{
#define PLL_CLKSEL_TRNG_160         (0x00 << (PLL_CLKSEL_TRNG - PLL_CLKSEL_R5_OFFSET)) //< Select TRNG clock as 160Mhz
#define PLL_CLKSEL_TRNG_240         (0x01 << (PLL_CLKSEL_TRNG - PLL_CLKSEL_R5_OFFSET)) //< Select TRNG clock as 240Mhz
//@}


/*
    @name   TRNG RO clock source value

    TRNG RO clock source value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_TRNGRO_CLKSRC).
*/
//@{
#define PLL_CLKSEL_TRNGRO_CLKSRC_ROSC     (0x00 << (PLL_CLKSEL_TRNGRO_CLKSRC- PLL_CLKSEL_R5_OFFSET)) //< Select TRNG ROSC clock from Ring OSC macro
#define PLL_CLKSEL_TRNGRO_CLKSRC_PLL9     (0x01 << (PLL_CLKSEL_TRNGRO_CLKSRC - PLL_CLKSEL_R5_OFFSET)) //< Select TRNG ROSC clock from PLL9
//@}


/*
    @name   DRTC clock rate value

    DRTC clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_DRTC).
*/
//@{
#define PLL_CLKSEL_DRTC_OSC         (0x00 << (PLL_CLKSEL_DRTC - PLL_CLKSEL_R5_OFFSET)) //< Select DRTC clock source from OSC 10/12 MHz
#define PLL_CLKSEL_DRTC_RTC         (0x01 << (PLL_CLKSEL_DRTC - PLL_CLKSEL_R5_OFFSET)) //< Select DRTC clock source from RTC 32.768 KHz
//@}

/*
    @name   REMOTE clock rate value

    REMOTE clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_REMOTE).
*/
//@{
#define PLL_CLKSEL_REMOTE_RTC         (0x00 << (PLL_CLKSEL_REMOTE - PLL_CLKSEL_R5_OFFSET)) //< Select REMOTE clock source from RTC 32.768 KHz
#define PLL_CLKSEL_REMOTE_OSC         (0x01 << (PLL_CLKSEL_REMOTE - PLL_CLKSEL_R5_OFFSET)) //< Select REMOTE clock source from OSC 12 MHz generated 32 KHz
#define PLL_CLKSEL_REMOTE_IOCLK       (0x02 << (PLL_CLKSEL_REMOTE - PLL_CLKSEL_R5_OFFSET)) //< Select REMOTE clock source from rmt_io_clk / P_GPIO_9
#define PLL_CLKSEL_REMOTE_3           (0x03 << (PLL_CLKSEL_REMOTE - PLL_CLKSEL_R5_OFFSET)) //< Select REMOTE clock source from 3 MHz
//@}

/*
    @name   ADC PowerDown clock rate value

    DC PowerDown clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ADC_PD).
*/
//@{
#define PLL_CLKSEL_ADC_PD_RTC         (0x00 << (PLL_CLKSEL_ADC_PD - PLL_CLKSEL_R5_OFFSET)) //< Select ADC_PD clock source from RTC 32.768 KHz
#define PLL_CLKSEL_ADC_PD_OSC         (0x01 << (PLL_CLKSEL_ADC_PD - PLL_CLKSEL_R5_OFFSET)) //< Select ADC_PD clock source from OSC 12 MHz generated 32 KHz
//@}

/*
    @name   Ethernet PHY Reference clock rate value

    Ethernet PHY Reference clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ETHPHY_CLKSRC).
*/
//@{
#define PLL_CLKSEL_ETHPHY_CLKSRC_PLL16         (0x00 << (PLL_CLKSEL_ETHPHY_CLKSRC - PLL_CLKSEL_R5_OFFSET))
#define PLL_CLKSEL_ETHPHY_CLKSRC_SPCLK         (0x01 << (PLL_CLKSEL_ETHPHY_CLKSRC - PLL_CLKSEL_R5_OFFSET))
#define PLL_CLKSEL_ETHPHY_CLKSRC_SP2CLK        (0x02 << (PLL_CLKSEL_ETHPHY_CLKSRC - PLL_CLKSEL_R5_OFFSET))
//@}

/*
    @name   IP ACLK clock rate value

    IP ACLK clock rate value

    @note This is for pll_set_clock_rate(PLL_CLKSEL_IPACLK_CLKSRC).
*/
//@{
#define PLL_CLKSEL_IPACLK_CLKSRC_240           (0x00 << (PLL_CLKSEL_IPACLK_CLKSRC - PLL_CLKSEL_R5_OFFSET))
#define PLL_CLKSEL_IPACLK_CLKSRC_120           (0x01 << (PLL_CLKSEL_IPACLK_CLKSRC - PLL_CLKSEL_R5_OFFSET))
//@}

/*
    @name   MIPI CSI D0 LP Clock Source

    MIPI CSI DataLane-0 LP Clock Source

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CSILPCLK_D0).
*/
//@{
#define PLL_CLKSEL_CSILPCLK_D0_SRC_HSI0 (0x00 << (PLL_CLKSEL_CSILPCLK_D0 - PLL_CLKSEL_R6_OFFSET)) //< CSI D0 LP CLK SRC from HSI-D0
#define PLL_CLKSEL_CSILPCLK_D0_SRC_HSI1 (0x01 << (PLL_CLKSEL_CSILPCLK_D0 - PLL_CLKSEL_R6_OFFSET)) //< CSI D0 LP CLK SRC from HSI-D1
#define PLL_CLKSEL_CSILPCLK_D0_SRC_HSI2 (0x02 << (PLL_CLKSEL_CSILPCLK_D0 - PLL_CLKSEL_R6_OFFSET)) //< CSI D0 LP CLK SRC from HSI-D2
#define PLL_CLKSEL_CSILPCLK_D0_SRC_HSI3 (0x03 << (PLL_CLKSEL_CSILPCLK_D0 - PLL_CLKSEL_R6_OFFSET)) //< CSI D0 LP CLK SRC from HSI-D3
//@}

/*
    @name   MIPI CSI D1 LP Clock Source

    MIPI CSI DataLane-1 LP Clock Source

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CSILPCLK_D1).
*/
//@{
#define PLL_CLKSEL_CSILPCLK_D1_SRC_HSI0 (0x00 << (PLL_CLKSEL_CSILPCLK_D1 - PLL_CLKSEL_R6_OFFSET)) //< CSI D1 LP CLK SRC from HSI-D0
#define PLL_CLKSEL_CSILPCLK_D1_SRC_HSI1 (0x01 << (PLL_CLKSEL_CSILPCLK_D1 - PLL_CLKSEL_R6_OFFSET)) //< CSI D1 LP CLK SRC from HSI-D1
#define PLL_CLKSEL_CSILPCLK_D1_SRC_HSI2 (0x02 << (PLL_CLKSEL_CSILPCLK_D1 - PLL_CLKSEL_R6_OFFSET)) //< CSI D1 LP CLK SRC from HSI-D2
#define PLL_CLKSEL_CSILPCLK_D1_SRC_HSI3 (0x03 << (PLL_CLKSEL_CSILPCLK_D1 - PLL_CLKSEL_R6_OFFSET)) //< CSI D1 LP CLK SRC from HSI-D3
//@}

/*
    @name   MIPI CSI D2 LP Clock Source

    MIPI CSI DataLane-2 LP Clock Source

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CSILPCLK_D2).
*/
//@{
#define PLL_CLKSEL_CSILPCLK_D2_SRC_HSI0 (0x00 << (PLL_CLKSEL_CSILPCLK_D2 - PLL_CLKSEL_R6_OFFSET)) //< CSI D2 LP CLK SRC from HSI-D0
#define PLL_CLKSEL_CSILPCLK_D2_SRC_HSI1 (0x01 << (PLL_CLKSEL_CSILPCLK_D2 - PLL_CLKSEL_R6_OFFSET)) //< CSI D2 LP CLK SRC from HSI-D1
#define PLL_CLKSEL_CSILPCLK_D2_SRC_HSI2 (0x02 << (PLL_CLKSEL_CSILPCLK_D2 - PLL_CLKSEL_R6_OFFSET)) //< CSI D2 LP CLK SRC from HSI-D2
#define PLL_CLKSEL_CSILPCLK_D2_SRC_HSI3 (0x03 << (PLL_CLKSEL_CSILPCLK_D2 - PLL_CLKSEL_R6_OFFSET)) //< CSI D2 LP CLK SRC from HSI-D3
//@}

/*
    @name   MIPI CSI D3 LP Clock Source

    MIPI CSI DataLane-3 LP Clock Source

    @note This is for pll_set_clock_rate(PLL_CLKSEL_CSILPCLK_D3).
*/
//@{
#define PLL_CLKSEL_CSILPCLK_D3_SRC_HSI0 (0x00 << (PLL_CLKSEL_CSILPCLK_D3 - PLL_CLKSEL_R6_OFFSET)) //< CSI D3 LP CLK SRC from HSI-D0
#define PLL_CLKSEL_CSILPCLK_D3_SRC_HSI1 (0x01 << (PLL_CLKSEL_CSILPCLK_D3 - PLL_CLKSEL_R6_OFFSET)) //< CSI D3 LP CLK SRC from HSI-D1
#define PLL_CLKSEL_CSILPCLK_D3_SRC_HSI2 (0x02 << (PLL_CLKSEL_CSILPCLK_D3 - PLL_CLKSEL_R6_OFFSET)) //< CSI D3 LP CLK SRC from HSI-D2
#define PLL_CLKSEL_CSILPCLK_D3_SRC_HSI3 (0x03 << (PLL_CLKSEL_CSILPCLK_D3 - PLL_CLKSEL_R6_OFFSET)) //< CSI D3 LP CLK SRC from HSI-D3
//@}

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
    @name   IDE clock divider

    IDE clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_IDE_CLKDIV).
*/
//@{
#define PLL_IDE_CLKDIV(x)           ((x) << (PLL_CLKSEL_IDE_CLKDIV - PLL_CLKSEL_R9_OFFSET))     //< Used for pll_set_clock_rate(PLL_CLKSEL_IDE_CLKDIV)
//@}

/*
    @name   IDE Output Interface clock divider

    IDE Output Interface clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_IDE_OUTIF_CLKDIV).
*/
//@{
#define PLL_IDE_OUTIF_CLKDIV(x)          ((x) << (PLL_CLKSEL_IDE_OUTIF_CLKDIV - PLL_CLKSEL_R9_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_IDE_OUTIF_CLKDIV)
//@}

/*
    @name   SIE MCLK3 clock divider

    SIE MCLK3 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3DIV).
*/
//@{
#define PLL_SIE_MCLK3DIV(x)         ((x) << (PLL_CLKSEL_SIE_MCLK3DIV - PLL_CLKSEL_R9_OFFSET))   //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3DIV)
//@}

/*
    @name   TRNG clock divider

    TRNG clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_TRNG_CLKDIV).
*/
//@{
#define PLL_TRNG_CLKDIV(x)          ((x) << (PLL_CLKSEL_TRNG_CLKDIV - PLL_CLKSEL_R9_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_TRNG_CLKDIV)
//@}

/*
    @name   SP clock divider

    Special clock divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SP_CLKDIV)
*/
//@{
#define PLL_SP_CLKDIV(x)            ((x) << (PLL_CLKSEL_SP_CLKDIV - PLL_CLKSEL_R10_OFFSET))     //< Used for pll_set_clock_rate(PLL_CLKSEL_SP_CLKDIV)
//@}

/*
    @name   SIE3 CLK clock divider

    SIE3 CLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SIE3_CLKDIV).
*/
//@{
#define PLL_SIE3_CLKDIV(x)          ((x) << (PLL_CLKSEL_SIE3_CLKDIV - PLL_CLKSEL_R10_OFFSET))        //< Used for pll_set_clock_rate(PLL_CLKSEL_SIE3_PLLCLKDIV)
//@}

/*
    @name   ADO clock divider

    ADO clock divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ADO_CLKDIV).
*/
//@{
#define PLL_ADO_CLKDIV(x)           ((x) << (PLL_CLKSEL_ADO_CLKDIV - PLL_CLKSEL_R10_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_ADO_CLKDIV)
//@}

/*
    @name   ADO OSR clock divider

    ADO OSR clock divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_ADO_OSR_CLKDIV).
*/
//@{
#define PLL_ADO_OSR_CLKDIV(x)           ((x) << (PLL_CLKSEL_ADO_OSR_CLKDIV - PLL_CLKSEL_R10_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_ADO_CLKDIV)
//@}

/*
    @name   SDIO clock divider

    SDIO clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SDIO_CLKDIV)
*/
//@{
#define PLL_SDIO_CLKDIV(x)          ((x) << (PLL_CLKSEL_SDIO_CLKDIV - PLL_CLKSEL_R11_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SDIO_CLKDIV)
//@}

/*
    @name   SDIO2 clock divider

    SDIO2 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SDIO2_CLKDIV)
*/
//@{
#define PLL_SDIO2_CLKDIV(x)         ((x) << (PLL_CLKSEL_SDIO2_CLKDIV - PLL_CLKSEL_R11_OFFSET))  //< This if for pll_set_clock_rate(PLL_CLKSEL_SDIO2_CLKDIV)
//@}

/*
    @name   SDIO3 clock divider

    SDIO3 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SDIO3_CLKDIV)
*/
//@{
#define PLL_SDIO3_CLKDIV(x)         ((x) << (PLL_CLKSEL_SDIO3_CLKDIV - PLL_CLKSEL_R12_OFFSET))  //< This if for pll_set_clock_rate(PLL_CLKSEL_SDIO3_CLKDIV)
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
    @name   SP2 clock divider

    Special clock2 divider

    @note This is for pll_set_clock_rate(PLL_CLKSEL_SP2_CLKDIV)
*/
//@{
#define PLL_SP2_CLKDIV(x)           ((x) << (PLL_CLKSEL_SP2_CLKDIV - PLL_CLKSEL_R12_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_SP2_CLKDIV)
//@}


/*
    @name   SPI clock divider

    SPI clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI_CLKDIV)
*/
//@{
#define PLL_SPI_CLKDIV(x)           ((x) << (PLL_CLKSEL_SPI_CLKDIV - PLL_CLKSEL_R13_OFFSET))    //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI_CLKDIV)
//@}

/*
    @name   SPI2 clock divider

    SPI2 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI2_CLKDIV)
*/
//@{
#define PLL_SPI2_CLKDIV(x)          ((x) << (PLL_CLKSEL_SPI2_CLKDIV - PLL_CLKSEL_R13_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI2_CLKDIV)
//@}

/*
    @name   SPI3 clock divider

    SPI3 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI3_CLKDIV)
*/
//@{
#define PLL_SPI3_CLKDIV(x)          ((x) << (PLL_CLKSEL_SPI3_CLKDIV - PLL_CLKSEL_R14_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI3_CLKDIV)
//@}

/*
    @name   SPI4 clock divider

    SPI4 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI4_CLKDIV)
*/
//@{
#define PLL_SPI4_CLKDIV(x)          ((x) << (PLL_CLKSEL_SPI4_CLKDIV - PLL_CLKSEL_R14_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI4_CLKDIV)
//@}

/*
    @name   UART2 clock divider

    UART2 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_UART2_CLKDIV)
*/
//@{
#define PLL_UART2_CLKDIV(x)          ((x) << (PLL_CLKSEL_UART2_CLKDIV - PLL_CLKSEL_R15_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_UART2_CLKDIV)
//@}

/*
    @name   UART3 clock divider

    UART3 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_UART3_CLKDIV)
*/
//@{
#define PLL_UART3_CLKDIV(x)          ((x) << (PLL_CLKSEL_UART3_CLKDIV - PLL_CLKSEL_R15_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_UART3_CLKDIV)
//@}

/*
    @name   UART4 clock divider

    UART4 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_UART4_CLKDIV)
*/
//@{
#define PLL_UART4_CLKDIV(x)          ((x) << (PLL_CLKSEL_UART4_CLKDIV - PLL_CLKSEL_R15_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_UART4_CLKDIV)
//@}

/*
    @name   UART5 clock divider

    UART5 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_UART5_CLKDIV)
*/
//@{
#define PLL_UART5_CLKDIV(x)          ((x) << (PLL_CLKSEL_UART5_CLKDIV - PLL_CLKSEL_R15_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_UART5_CLKDIV)
//@}

/*
    @name   TRNG RO clock divider

    TRNG RO clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_TRNG_RO_CLKDIV).
*/
//@{
#define PLL_TRNG_RO_CLKDIV(x)          ((x) << (PLL_CLKSEL_TRNG_RO_CLKDIV - PLL_CLKSEL_R20_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_TRNG_RO_CLKDIV)
//@}

/*
    @name   TRNG RO delay

    TRNG RO delay

    @note This if for pll_set_clock_rate(PLL_CLKSEL_TRNG_RO_DELAY).
*/
//@{
#define PLL_TRNG_RO_DELAY(x)          ((x) << (PLL_CLKSEL_TRNG_RO_DELAY - PLL_CLKSEL_R20_OFFSET))    //< Used for pll_set_clock_rate(PLL_CLKSEL_TRNG_RO_DELAY)
//@}

/*
    @name   UART6 clock divider

    UART6 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_UART6_CLKDIV)
*/
//@{
#define PLL_UART6_CLKDIV(x)          ((x) << (PLL_CLKSEL_UART6_CLKDIV - PLL_CLKSEL_R20_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_UART6_CLKDIV)
//@}

/*
    @name   AUDIO MCLK clock divider

    AUDIO MCLK clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_ADO_MCLKDIV)
*/
//@{
#define PLL_ADO_MCLKDIV(x)          ((x) << (PLL_CLKSEL_ADO_MCLKDIV - PLL_CLKSEL_R20_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_UART6_CLKDIV)
//@}

/*
    @name   SPI5 clock divider

    SPI5 clock divider

    @note This if for pll_set_clock_rate(PLL_CLKSEL_SPI5_CLKDIV)
*/
//@{
#define PLL_SPI5_CLKDIV(x)          ((x) << (PLL_CLKSEL_SPI5_CLKDIV - PLL_CLKSEL_R21_OFFSET))   //< This if for pll_set_clock_rate(PLL_CLKSEL_SPI5_CLKDIV)
//@}


/*
    System Reset ID

    This is for pll_enable_system_reset() and pll_disable_system_reset().
*/
typedef enum {
	PLL_RSTN_DRAMC          = 1,            //< Reset SDRAM controller
	AXI_BRG_IP_RSTN         = 2,            //< Reset AXI Bridge controller
	SIE_RSTN                = 4,            //< Reset SIE controller
	SIE2_RSTN,              /*5*/           //< Reset SIE2 controller
	TGE_RSTN,               /*6*/           //< Reset PRE controller
	IPE_RSTN,               /*7*/           //< Reset IPE controller
	DIS_RSTN,               /*8*/           //< Reset DIS controller
	IME_RSTN,               /*9*/           //< Reset IME controller
	ISE_RSTN                = 11,           //< Reset ISE controller
	IVE_RSTN                = 12,           //< Reset IVE controller
	IFE_RSTN                = 13,           //< Reset IFE controller
	DCE_RSTN,               /*14*/          //< Reset DCE controller
	IDE_RSTN                = 16,           //< Reset IDE controller
	CRYPTO_RSTN             = 23,           //< Reset CRYPTO controller
	VENC_RSTN,              /*24*/          //< Reset H265 controller
	AFFINE_RSTN,            /*25*/          //< Reset AFFINE controller
	JPG_RSTN                = 26,           //< Reset JPG controller
	GRAPH_RSTN,             /*27*/          //< Reset Graphic controller
	GRAPH2_RSTN,            /*28*/          //< Reset Graphic2 controller
	DAI_RSTN,               /*29*/          //< Reset DAI controller
	EAC_RSTN,               /*30*/          //< Reset EAC controller

	NAND_RSTN               = 32+ 0,        //< Reset xD/NAND controller
	SDIO_RSTN               = 32+ 2,        //< Reset SDIO controller
	SDIO2_RSTN,             /*32+ 3*/       //< Reset SDIO2 controller
	I2C_RSTN,               /*32+ 4*/       //< Reset I2C controller
	I2C2_RSTN,              /*32+ 5*/       //< Reset I2C2 controller
	SPI_RSTN,               /*32+ 6*/       //< Reset SPI controller
	SPI2_RSTN,              /*32+ 7*/       //< Reset SPI2 controller
	SPI3_RSTN,              /*32+ 8*/       //< Reset SPI3 controller
	SIF_RSTN,               /*32+ 9*/       //< Reset SIF controller
	UART_RSTN,              /*32+ 10*/      //< Reset UART controller
	UART2_RSTN,             /*32+ 11*/      //< Reset UART2 controller
	REMOTE_RSTN,            /*32+ 12*/      //< Reset Remote controller
	ADC_RSTN,               /*32+ 13*/      //< Reset ADC controller
	SDIO3_RSTN,             /*32+ 14*/      //< Reset SDIO3 controller
	RTC_RSTN                = 32+ 16,       //< Reset RTC controller
	WDT_RSTN,               /*32+ 17*/      //< Reset WDT controller
	TMR_RSTN,               /*32+ 18*/      //< Reset Timer controller
	USB_RSTN,               /*32+ 19*/      //< Reset USB controller
	UVCP_RSTN               = 32+ 21,       //< Reset UART3 controller
	UART3_RSTN              = 32+ 22,       //< Reset UART3 controller
	INTC_RSTN               = 32+ 24,       //< Reset INTC controller
	GPIO_RSTN,              /*32+ 25*/      //< Reset GPIO controller
	PAD_RSTN,               /*32+ 26*/      //< Reset PAD controller
	TOP_RSTN,               /*32+ 27*/      //< Reset TOP controller
	EFUSE_RSTN,             /*32+ 28*/      //< Reset EFUSE controller
	ETH_RSTN,               /*32+ 29*/      //< Reset ETH controller
	ETH_GLUE_RSTN,          /*32+ 30*/      //< Reset ETH Glue
	I2C3_RSTN               = 32+ 31,       //< Reset I2C3 controller

	MIPI_LVDS_RSTN          = 64+ 1,        //< Reset MIPI_LVDS controller
	MIPI_LVDS2_RSTN,        /*64+ 2*/       //< Reset MIPI_LVDS2 controller
	MIPI_DSI_RSTN,          /*64+ 3*/       //< Reset MIPI DSI controller
	PWM_RSTN                = 64+ 8,        //< Reset PWM controller
	SIE3_RSTN,              /*64+ 9*/       //< Reset SIE3 controller
	TSE_RSTN                = 64+ 13,       //< Reset TSMUX controller
	NUE2_RSTN               = 64+ 15,       //< Reset NUE2 controller
	MDBC_RSTN,              /*64+ 16*/      //< Reset MDBC controller
	CNN_RSTN,               /*64+ 17*/      //< Reset CNN controller
	SDP_RSTN                = 64+ 20,	//< Reset SDP controller
	DRTC_RSTN               = 64+ 22,       //< Reset DRTC controller
	SENPHY_RSTN             = 64+ 24,       //< Reset SENPHY controller
	TRNG_RSTN,              /*64+ 25*/      //< Reset TRNG controller
	RSA_RSTN,               /*64+ 26*/      //< Reset RSA controller
	HASH_RSTN,              /*64+ 27*/      //< Reset HASH controller
	DDRPHY_RSTN,            /*64+ 28*/      //< Reset DDRPHY
	ETH_PHY_RSTN            = 64+ 30,       //< Reset Ethernet PHY module
	ETH_PHY_HI_RSTN,        /*64+ 31*/      //< Reset Ethernet PHY register

	ENUM_DUMMY4WORD(CG_RSTN)
} CG_RSTN;




/**
    Clock frequency select ID

    @note This is for pll_set_clock_freq().
*/
typedef enum {
	SIEMCLK_FREQ,           ///< SIE    MCLK freq Select ID
	SIEMCLK2_FREQ,          ///< SIE    MCLK2 freq Select ID
	SIEMCLK3_FREQ,          ///< SIE    MCLK2 freq Select ID

	SIECLK_FREQ,            ///< SIE    CLK freq Select ID
	SIE2CLK_FREQ,           ///< SIE2   CLK freq Select ID
	SIE3CLK_FREQ,           ///< SIE3   CLK freq Select ID

	IDECLK_FREQ,            ///< IDE    CLK freq Select ID

	SPCLK_FREQ,             ///< SP     CLK Select ID
	SPCLK2_FREQ,            ///< SP2    CLK Select ID

	ADOCLK_FREQ,            ///< ADO    CLK Select ID

	SDIOCLK_FREQ,           ///< SDIO   CLK Select ID
	SDIO2CLK_FREQ,          ///< SDIO2  CLK Select ID
	SDIO3CLK_FREQ,          ///< SDIO3  CLK Select ID

	SPICLK_FREQ,            ///< SPI    CLK Select ID
	SPI2CLK_FREQ,           ///< SPI2   CLK Select ID
	SPI3CLK_FREQ,           ///< SPI3   CLK Select ID

	IDEOUTIFCLK_FREQ,       ///< IDE    output I/F CLK freq Select ID

	CPUCLK_FREQ,			///< CPU1   CLK Select ID
	APBCLK_FREQ,			///< APB    CLK Select ID
	TRNGCLK_FREQ,			///< TRNG   CLK Select ID
	SIEMCLK_12SYNC_FREQ,    ///< SIE    MCLK-1/2 SYNC freq Select ID

	PLL_CLKFREQ_MAXNUM,

	DSICLK_FREQ,            ///< DSI    CLK Select ID
	IDE2CLK_FREQ,           ///< Backward compatible
	HDMIADOCLK_FREQ,        ///< Backward compatible

	ENUM_DUMMY4WORD(PLL_CLKFREQ)
} PLL_CLKFREQ;



//
//	Exporting APIs
//

extern void     pll_set_clock_rate(PLL_CLKSEL clk_sel, UINT32 uiValue);
extern UINT32   pll_get_clock_rate(PLL_CLKSEL clk_sel);

extern void     pll_enable_clock(CG_EN Num);
extern void     pll_disable_clock(CG_EN Num);
extern ER       pll_set_clock_freq(PLL_CLKFREQ ClkID, UINT32 uiFreq);
extern ER       pll_get_clock_freq(PLL_CLKFREQ ClkID, UINT32 *pFreq);
extern void     pll_enable_system_reset(CG_RSTN Num);
extern void     pll_disable_system_reset(CG_RSTN Num);

extern UINT32   pll_get_osc_freq(void);
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

//
// For Backward Compatible
//
#define PLL_CLKSEL_NAND_60             0 //< Backward compatible
#define PLL_CLKSEL_SIE_MCLKSRC_PLL10   0 //< Backward compatible
#define PLL_CLKSEL_SIE_MCLKSRC_PLL18   0 //< Backward compatible
#define PLL_CLKSEL_SIE_MCLK2SRC_PLL10  0 //< Backward compatible
#define PLL_CLKSEL_SIE_MCLK3SRC_PLL10  0 //< Backward compatible
#define PLL_CLKSEL_SIE_MCLK3SRC_PLL18  0 //< Backward compatible
#define H265_CLK                       0 //< Backward compatible
#define SIE4_CLK                       0 //< Backward compatible
#define SIE5_CLK                       0 //< Backward compatible
#define PLL_CLKSEL_SIE_CLKSRC_PLL10    0 //< Backward compatible
#define PLL_CLKSEL_SIE2_CLKSRC_PLL10   0 //< Backward compatible
#define PLL_CLKSEL_SIE3_CLKSRC_PLL10   0 //< Backward compatible
#define PLL_CLKSEL_SIE4_CLKSRC_PLL10   0 //< Backward compatible
#define PLL_CLKSEL_SIE5_CLKSRC_PLL10   0 //< Backward compatible
#define SIE4CLK_FREQ                   0 //< Backward compatible
#define SIE5CLK_FREQ                   0 //< Backward compatible
#define SIE4_PXCLK                     0 //< Backward compatible
#define H265_M_GCLK                    0 //< Backward compatible
#define PLL_CLKSEL_H265                0 //< Backward compatible
#define PLL_CLKSEL_H265_240            0 //< Backward compatible
#define PLL_CLKSEL_H265_320            0 //< Backward compatible
#define PLL_CLKSEL_H265_PLL15          0 //< Backward compatible
#define PLL_CLKSEL_AFFINE_PLL13        5 //< Backward compatible
#define PLL_CLKSEL_ISE_PLL13           0 //< Backward compatible
#define PLL_CLKSEL_GRAPHIC_PLL6        6 //< Backward compatible
#define PLL_CLKSEL_GRAPHIC_PLL13       5 //< Backward compatible
#define PLL_CLKSEL_GRAPHIC2_PLL6       6 //< Backward compatible
#define PLL_CLKSEL_GRAPHIC2_PLL13      5 //< Backward compatible
#define CNN2_M_GCLK                    0 //< Backward compatible
#define SPI4_CLK                       0 //< Backward compatible
#define SPI5_CLK                       0 //< Backward compatible
#define SPI4CLK_FREQ                   0 //< Backward compatible
#define SPI5CLK_FREQ                   0 //< Backward compatible
#define SDE_CLK                        0 //< Backward compatible
#define PLL_CLKSEL_SDE_480             0 //< Backward compatible
#define PLL_CLKSEL_SDE_PLL13           0 //< Backward compatible
#define PLL_CLKSEL_SDE_320             0 //< Backward compatible
#define PLL_CLKSEL_SDE_240             0 //< Backward compatible
#define IFE2_M_GCLK                    0 //< Backward compatible
#define IFE2_GCLK                      0 //< Backward compatible
#define IFE2_CLK                       0 //< Backward compatible
#define IFE2_RSTN                      0 //< Backward compatible
#define PLL_CLKSEL_IFE2_240            0 //< Backward compatible
#define PLL_CLKSEL_IDE_CLKSRC_PLL4     0 //< Backward compatible
#define PLL_CLKSEL_MDBC_PLL13          0 //< Backward compatible
#define PLL_CLKSEL_MDBC_320            0 //< Backward compatible
#define PLL_CLKSEL_DIS_PLL13           0 //< Backward compatible
#define PLL_CLKSEL_DIS_480             0 //< Backward compatible
#define PLL_CLKSEL_DIS_320             0 //< Backward compatible
#define PLL_CLKSEL_IVE_PLL13           5 //< Backward compatible
#define CNN2_CLK                       0 //< Backward compatible
#define PLL_CLKSEL_NUE                 0 //< Backward compatible
#define PLL_CLKSEL_NUE_240             0 //< Backward compatible
#define PLL_CLKSEL_NUE_PLL10           0 //< Backward compatible
#define PLL_CLKSEL_NUE_480             0 //< Backward compatible
#define PLL_CLKSEL_NUE_320             0 //< Backward compatible
#define PLL_CLKSEL_NUE2_PLL13          0 //< Backward compatible
#define PLL_CLKSEL_CNN_PLL10           0 //< Backward compatible
#define PLL_CLKSEL_CNN_480             0 //< Backward compatible
#define NUE_CLK                        0 //< Backward compatible
#define PLL_CLKSEL_SDIO_PLL4           5 //< Backward compatible
#define PLL_CLKSEL_SDIO2_PLL4          5 //< Backward compatible
#define PLL_CLKSEL_SDIO3_PLL4          5 //< Backward compatible
#define PLL_CLKSEL_TRNGRO_CLKSRC_PLL4  5 //< Backward compatible
#define UART4_CLK                      0 //< Backward compatible
#define UART5_CLK                      0 //< Backward compatible
#define UART6_CLK                      0 //< Backward compatible
//@}

#endif


