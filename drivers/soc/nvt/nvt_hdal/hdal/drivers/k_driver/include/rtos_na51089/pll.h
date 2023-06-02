/**
    PLL Configuration module header

    PLL Configuration module header file.

    @file       pll.h
    @ingroup    mIDrvSys_CG
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _PLL_H
#define _PLL_H

#include <kwrap/nvt_type.h>

/**
    @addtogroup mIHALSysCG
*/
//@{

/**
    Clock Enable ID

    This is for pll_enable_clock() and pll_disable_clock().
*/
typedef enum {
	DMA_CLK       = 1,  ///< DMA(SDRAM) clock
	SIE_MCLK,           ///< SIE MCLK
	SIE_MCLK2,          ///< SIE MCLK2
	SIE_CLK,            ///< SIE clock
	SIE2_CLK,           ///< SIE2 Clock
	TGE_CLK,            ///< TGE clock
	IPE_CLK,            ///< IPE clock
	DIS_CLK,            ///< DIS clock
	IME_CLK,            ///< IME clock
	SIE_MCLK3,          ///< SIE MCLK3
	ISE_CLK,            ///< ISE clock
	SP_CLK,             ///< special clock
	IFE_CLK,            ///< IFE clock
	DCE_CLK,            ///< DCE clock
	IDE1_CLK      = 16, ///< IDE clock
	SIE3_CLK,           ///< SIE3 clock
	CRYPTO_CLK    = 23, ///< Crypto clock
	VENC_CLK      = 24, ///< VENC clock
	AFFINE_CLK    = 25, ///< AFFINE clock
	JPG_CLK       = 26, ///< JPEG clock
	GRAPH_CLK,          ///< Graphic clock
	GRAPH2_CLK,         ///< Graphic2 clock
	DAI_CLK,            ///< DAI clock
	EAC_A_ADC_CLK,      ///< EAC analog AD clock
	EAC_A_DAC_CLK,      ///< EAC analog DA clock

	NAND_CLK      = 32, ///< NAND clock
	SDIO_CLK      = 34, ///< SDIO clock
	SDIO2_CLK,          ///< SDIO2 clock
	I2C_CLK       = 36, ///< I2C clock
	I2C2_CLK      = 37, ///< I2C2 clock
	SPI_CLK,            ///< SPI clock
	SPI2_CLK,           ///< SPI2 clock
	SPI3_CLK,           ///< SPI3 clock
	SIF_CLK,            ///< SIF clock
	UART_CLK,           ///< UART clock
	UART2_CLK,          ///< UART2 clock
	REMOTE_CLK,         ///< Remote clock
	ADC_CLK,            ///< ADC clock
	SDIO3_CLK,          ///< SDIO3 clock
	WDT_CLK       = 49, ///< WDT clock
	TMR_CLK,            ///< Timer clock
	EAC_D_CLKEN   = 52, ///< EAC digital clock
	UVCP_CLKEN    = 53, ///< UVCP clock
	UART3_CLK     = 54, ///< UART3 clock
	EFUSE_CLK     = 60, ///< EFUSE clock
	ETH_CLK,            ///< ETH clock
	SP2_CLK,            ///< SP2 clock
	I2C3_CLK      = 63, ///< I2C3 clock

	MIPI_LVDS_CLK = 65, ///< MIPI/LVDS clock
	MIPI_LVDS2_CLK,     ///< MIPI/LVDS2 clock
	MIPI_DSI_CLK,       ///< MIPI DSI clock
	SIE_PXCLK,          ///< SIE PX clock
	SIE2_PXCLK,         ///< SIE2 PX clock
	SIE3_PXCLK,         ///< SIE3 PX clock
	PWM_CCNT_CLK  = 72, ///< PWM CCNT clock
	PWM_CCNT0_CLK = 72, ///< PWM CCNT0 clock
	PWM_CCNT1_CLK,      ///< PWM CCNT1 clock
	PWM_CCNT2_CLK,      ///< PWM CCNT2 clock
	TSE_CLK       = 77, ///< TSE  clock
	DRTC_CLK      = 86, ///< DRTC clock
	ETHPHY_CLK    = 88, ///< ETH PHY clock
	TRNG_CLK,           ///< TRNG clock
	RSA_CLK,            ///< RSA clock
	HASH_CLK,           ///< HASH clock
	TRNG_RO_CLK,        ///< TRNG clock
	MIPI_LVDS_PHYD4_CLK2 = 93,///< MIPI/LVDS PHY HS-clock/4
	MIPI_LVDS_PHYD4_CLK  = 94,///< MIPI/LVDS PHY HS-clock/4
	MIPI_LVDS2_PHYD4_CLK = 95,///< MIPI/LVDS2 PHY HS-clock/4

	PWM0_CLK      = 96, ///< PWM0 clock
	PWM1_CLK,           ///< PWM1 clock
	PWM2_CLK,           ///< PWM2 clock
	PWM3_CLK,           ///< PWM3 clock
	PWM4_CLK,           ///< PWM4 clock
	PWM5_CLK,           ///< PWM5 clock
	PWM6_CLK,           ///< PWM6 clock
	PWM7_CLK,           ///< PWM7 clock
	PWM8_CLK,           ///< PWM8 clock
	PWM9_CLK,           ///< PWM9 clock
	PWM10_CLK,          ///< PWM10 clock
	PWM11_CLK,          ///< PWM11 clock

	SDP_CLK       = 112, ///< SDP clock
	NUE2_CLK      = 117, ///< NUE2 clock
	MDBC_CLK,            ///< MDBC clock
	CNN_CLK,             ///< CNN clock
	IVE_CLK       = 121, ///< IVE clock
	IP_ACLK_CLK   = 127, ///< IP_ACLK clock

	ENUM_DUMMY4WORD(CG_EN)
} CG_EN;





/**
    APB Clock Gating Select ID

    This is for pll_set_pclk_auto_gating() / pll_clear_pclk_auto_gating() / pll_get_pclk_auto_gating().
*/
typedef enum {
	SIE_GCLK,               /*0*/   ///< Gating SIE APB clock
	CNN_GCLK,               /*1*/   ///< Gating CNN APB clock
	IPE_GCLK,               /*2*/   ///< Gating IPE APB clock
	IME_GCLK,               /*3*/   ///< Gating IME APB clock
	DIS_GCLK,               /*4*/   ///< Gating DIS APB clock
	DCE_GCLK                = 6,    ///< Gating DCE APB clock
	IFE_GCLK,               /*7*/   ///< Gating IFE APB clock
	GRA_GCLK,               /*8*/   ///< Gating Graphic APB clock
	GRA2_GCLK,              /*9*/   ///< Gating Graphic2 APB clock
	IDE_GCLK,               /*10*/  ///< Gating IDE APB clock
	NUE2_GCLK               = 12,   ///< Gating NUE2 APB clock
	MDBC_GCLK,              /*13*/  ///< Gating MDBC APB clock
	JPG_GCLK                = 15,   ///< Gating JPEG APB clock
	VENC_GCLK               = 16,   ///< Gating VENC APB clock
	DAI_GCLK,               /*17*/  ///< Gating DAI APB clock
	EAC_GCLK,               /*18*/  ///< Gating EAC APB clock
	NAND_GCLK,              /*19*/  ///< Gating xD/Nand APB clock
	SDIO_GCLK,              /*20*/  ///< Gating SDIO APB clock
	SDIO2_GCLK,             /*21*/  ///< Gating SDIO2 APB clock
	I2C_GCLK,               /*22*/  ///< Gating I2C APB clock
	I2C2_GCLK,              /*23*/  ///< Gating I2C2 APB clock
	SPI_GCLK,               /*24*/  ///< Gating SPI APB clock
	SPI2_GCLK,              /*25*/  ///< Gating SPI2 APB clock
	SPI3_GCLK,              /*26*/  ///< Gating SPI3 APB clock
	SIF_GCLK,               /*27*/  ///< Gating SIF APB clock
	UART_GCLK,              /*28*/  ///< Gating UART APB clock
	UART2_GCLK,             /*29*/  ///< Gating UART2 APB clock
	RM_GCLK,                /*30*/  ///< Gating Remote APB clock
	ADC_GCLK,               /*31*/  ///< Gating ADC APB clock

	TMR_GCLK,               /*32*/  ///< Gating TMR APB clock
	WDT_GCLK,               /*33*/  ///< Gating Watchdog APB clock
	SDE_GCLK,               /*34*/  ///< Gating SDE APB clock
	MIPI_LVDS_GCLK          = 35,   ///< Gating MIPI/LVDS APB clock
	MIPI_LVDS2_GCLK,        /*36*/  ///< Gating MIPI/LVDS2 APB clock
	MIPI_DSI_GCLK           = 38,   ///< Gating MIPI DSI APB clock
	ISE_GCLK                = 40,   ///< Gating ISE APB clock
	SIE2_GCLK,              /*41*/  ///< Gating SIE2 APB clock
	SIE3_GCLK,              /*42*/  ///< Gating SIE3 APB clock
	PWM_GCLK,               /*43*/  ///< Gating PWM APB clock
	SDIO3_GCLK              = 47,   ///< Gating SDIO3 APB clock
	UART3_GCLK,             /*48*/  ///< Gating UART3 APB clock
	TGE_GCLK                = 52,   ///< Gating TGE APB clock

	I2C3_GCLK               = 56,   ///< Gating I2C3 APB clock
	TSE_GCLK,               /*57*/  ///< Gating TSE APB clock
	AFFINE_GCLK,            /*58*/  ///< Gating AFFINE APB clock
	IVE_GCLK                = 60,   ///< Gating IVE APB clock
	SENPHY_GCLK             = 61,   ///< Gating SENPHY APB clock
	UVCP_GCLK               = 62,   ///< Gating UVCP APB clock

	GPIO_GCLK               = 64,   ///< Gating GPIO APB clock
	INTC_GCLK,              /*65*/  ///< Gating INTC APB clock
	DMA_GCLK                = 67,   ///< Gating DMAC APB clock

	PCLKGAT_MAXNUM,
	ENUM_DUMMY4WORD(GATECLK)
} GATECLK;


/**
    Module Clock Gating Select ID

    This is for pll_set_clk_auto_gating() / pll_clear_clk_auto_gating() / pll_get_clk_auto_gating().
*/
typedef enum {
	M_GCLK_BASE             = PCLKGAT_MAXNUM,

	CNN_M_GCLK,             /*1*/                   ///< Gating CNN Module clock
	IPE_M_GCLK,             /*2*/                   ///< Gating IPE Module clock
	IME_M_GCLK,             /*3*/                   ///< Gating IME Module clock
	DIS_M_GCLK,             /*4*/                   ///< Gating DIS Module clock
	DCE_M_GCLK              = 6+M_GCLK_BASE,        ///< Gating DCE Module clock
	IFE_M_GCLK,             /*7*/                   ///< Gating IFE Module clock
	GRA_M_GCLK,             /*8*/                   ///< Gating Graphic Module clock
	GRA2_M_GCLK,            /*9*/                   ///< Gating Graphic2 Module clock
	NUE2_M_GCLK             = 12+M_GCLK_BASE,       ///< Gating NUE2 Module clock
	MDBC_M_GCLK,            /*13*/                  ///< Gating MDBC Module clock
	JPG_M_GCLK              = 15+M_GCLK_BASE,       ///< Gating JPEG Module clock
	VENC_M_GCLK             = 16+M_GCLK_BASE,       ///< Gating VENC Module clock
	NAND_M_GCLK             = 19+M_GCLK_BASE,       ///< Gating xD/Nand Module clock
	SDIO_M_GCLK,            /*20*/                  ///< Gating SDIO Module clock
	SDIO2_M_GCLK,           /*21*/                  ///< Gating SDIO2 Module clock

	SPI_M_GCLK              = 24+M_GCLK_BASE,       ///< Gating SPI Module clock
	SPI2_M_GCLK,            /*25*/                  ///< Gating SPI2 Module clock
	SPI3_M_GCLK,            /*26*/                  ///< Gating SPI3 Module clock
	SIF_M_GCLK,             /*27*/                  ///< Gating SIF Module clock

	ISE_M_GCLK              = 40+M_GCLK_BASE,       ///< Gating ISE Module clock
	PWM_M_GCLK              = 43+M_GCLK_BASE,       ///< Gating PWM Module clock
	SDIO3_M_GCLK            = 47+M_GCLK_BASE,       ///< Gating SDIO3 Module clock
	TSE_M_GCLK              = 57+M_GCLK_BASE,       ///< Gating TSE Module clock
	AFFINE_M_GCLK           = 58+M_GCLK_BASE,       ///< Gating AFFINE Module clock
	ETH_M_GCLK              = 59+M_GCLK_BASE,       ///< Gating ETH Module clock
	IVE_M_GCLK              = 60+M_GCLK_BASE,       ///< Gating IVE Module clock

	MCLKGAT_MAXNUM,
	ENUM_DUMMY4WORD(M_GATECLK)
} M_GATECLK;



/*
    @name Default Gating Clock Select definition

    This is for pll_config_clk_auto_gating() & pll_config_pclk_auto_gating().
*/
//@{
#define PLL_CLKSEL_DEFAULT_CLKGATE1     0x00000000
#define PLL_CLKSEL_DEFAULT_CLKGATE2     0x00000000
#define PLL_CLKSEL_DEFAULT_PCLKGATE1    0x00000000
#define PLL_CLKSEL_DEFAULT_PCLKGATE2    0x00000000
#define PLL_CLKSEL_DEFAULT_PCLKGATE3    0x00000000
//@}


/**
    PLL ID
*/
typedef enum {
	PLL_ID_1        = 1,        ///< PLL1 (internal 480 MHz)
	PLL_ID_3        = 3,        ///< PLL3 (for DMA)
	PLL_ID_4        = 4,        ///< PLL4 (N/A:dummy)
	PLL_ID_5        = 5,        ///< PLL5 (for sensor1)
	PLL_ID_6        = 6,        ///< PLL6 (for IDE/eth)
	PLL_ID_7        = 7,        ///< PLL7 (for audio)
	PLL_ID_8        = 8,        ///< PLL8 (for CPU)
	PLL_ID_9        = 9,        ///< PLL9 (for IDE/eth backup)
	PLL_ID_10       = 10,       ///< PLL10 (N/A:dummy)
	PLL_ID_11       = 11,       ///< PLL11 (for DSI)
	PLL_ID_12       = 12,       ///< PLL12 (for sensor2)

	PLL_ID_13       = 13,       ///< PLL13 (N/A:dummy)
	PLL_ID_14       = 14,       ///< PLL14 (N/A:dummy)
	PLL_ID_15       = 15,       ///< PLL15 (N/A:dummy)
	PLL_ID_16       = 16,       ///< PLL16 (N/A:dummy)
	PLL_ID_17       = 17,       ///< PLL17 (N/A:dummy)
	PLL_ID_18       = 18,       ///< PLL18 (N/A:dummy)

	PLL_ID_FIXED320 = 24,       ///< Fixed 320MHz PLL
	PLL_ID_MAX,
	ENUM_DUMMY4WORD(PLL_ID)
} PLL_ID;


#define PLL_ID_2    PLL_ID_MAX		///< Backward compatible


extern ER       pll_set_pll(PLL_ID id, UINT32 ui_setting);
extern BOOL     pll_get_pll_enable(PLL_ID id);
extern ER       pll_set_pll_enable(PLL_ID id, BOOL b_enable);
extern ER		pll_set_pll_freq(PLL_ID id, UINT32 ui_frequency);
extern UINT32   pll_get_pll_freq(PLL_ID id);
extern ER       pll_set_pll_spread_spectrum(PLL_ID id, UINT32 lower_frequency, UINT32 upper_frequency);
extern ER       pll_get_pll_spread_spectrum(PLL_ID id, UINT32 *pui_lower_freq, UINT32 *pui_upper_freq);

extern void     pll_set_pwm_clock_rate(UINT32 pwm_number, UINT32 ui_divider);
extern BOOL     pll_is_clock_enabled(CG_EN num);

extern void     pll_set_clk_auto_gating(M_GATECLK clock_select);
extern void     pll_clear_clk_auto_gating(M_GATECLK clock_select);
extern BOOL     pll_get_clk_auto_gating(M_GATECLK clock_select);

extern void     pll_set_pclk_auto_gating(GATECLK clock_select);
extern void     pll_clear_pclk_auto_gating(GATECLK clock_select);
extern BOOL     pll_get_pclk_auto_gating(GATECLK clock_select);

extern void     pll_set_trng_ro_sel(UINT32 trng_ro_select, UINT32 ui_divider);
extern void     pll_get_trng_ro_sel(UINT32 *pui_trng_ro_sel, UINT32 *pui_divider);
extern ER       pll_init(void);
extern UINT32   pll_get_apb_freq(void);



#define pll_setPLL					pll_set_pll
#define pll_getPLLEn				pll_get_pll_enable
#define pll_setPLLEn				pll_set_pll_enable
#define pll_getPLLFreq				pll_get_pll_freq
#define pll_setPLLSpreadSpectrum	pll_set_pll_spread_spectrum
#define pll_getPLLSpreadSpectrum	pll_get_pll_spread_spectrum
#define pll_isClockEnabled			pll_is_clock_enabled
#define pll_setClkAutoGating		pll_set_clk_auto_gating
#define pll_clearClkAutoGating		pll_clear_clk_auto_gating
#define pll_getClkAutoGating		pll_get_clk_auto_gating
#define pll_setPclkAutoGating		pll_set_pclk_auto_gating
#define pll_clearPclkAutoGating		pll_clear_pclk_auto_gating
#define pll_getPclkAutoGating		pll_get_pclk_auto_gating


//@}

#endif



