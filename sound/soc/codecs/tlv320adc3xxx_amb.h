/*
 * sound/soc/codecs/tlv320adc3xxx_amb.H
 *
 * Author: Dongge wu <dgwu@ambarella.com>
 *
 * History:
 *	2015/10/28 - [Dongge wu] Created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */


#ifndef _ADC3xxx_AMB_H
#define _ADC3xxx_AMB_H

#define AUDIO_NAME 			"tlv320adc3xxx"
#define ADC3xxx_VERSION		"Rev A"

#define ADC3101_CODEC_SUPPORT
//#define ADC3001_CODEC_SUPPORT

/* Macro enables or disables support for TiLoad in the driver */
#define ADC3xxx_TiLoad
//#undef ADC3xxx_TiLoad

/* 8 bit mask value */
#define ADC3xxx_8BITS_MASK           0xFF

/* Enable slave / master mode for codec */
#define ADC3xxx_MCBSP_SLAVE //codec master
//#undef ADC3xxx_MCBSP_SLAVE


/****************************************************************************/
/* 							 Page 0 Registers 						 		*/
/****************************************************************************/

/* Page select register */
#define PAGE_SELECT				0
/* Software reset register */
#define RESET					1

/* 2-3 Reserved */

/* PLL programming register B */
#define CLKGEN_MUX				4
/* PLL P and R-Val */
#define PLL_PROG_PR				5
/* PLL J-Val */
#define PLL_PROG_J				6
/* PLL D-Val MSB */
#define PLL_PROG_D_MSB			7
/* PLL D-Val LSB */
#define PLL_PROG_D_LSB			8

/* 9-17 Reserved */

/* ADC NADC */
#define ADC_NADC				18
/* ADC MADC */
#define ADC_MADC				19
/* ADC AOSR */
#define ADC_AOSR				20
/* ADC IADC */
#define ADC_IADC				21

/* 23-24 Reserved */

/* CLKOUT MUX */
#define CLKOUT_MUX				25
/* CLOCKOUT M divider value */
#define CLKOUT_M_DIV			26
/*Audio Interface Setting Register 1*/
#define INTERFACE_CTRL_1		27
/* Data Slot Offset (Ch_Offset_1) */
#define CH_OFFSET_1				28
/* ADC interface control 2 */
#define INTERFACE_CTRL_2		29
/* BCLK N Divider */
#define BCLK_N_DIV				30
/* Secondary audio interface control 1 */
#define INTERFACE_CTRL_3		31
/* Secondary audio interface control 2 */
#define INTERFACE_CTRL_4		32
/* Secondary audio interface control 3 */
#define INTERFACE_CTRL_5		33
/* I2S sync */
#define I2S_SYNC				34

/* 35 Reserved */

/* ADC flag register */
#define ADC_FLAG				36
/* Data slot offset 2 (Ch_Offset_2) */
#define CH_OFFSET_2				37
/* I2S TDM control register */
#define I2S_TDM_CTRL			38

/* 39-41 Reserved */

/* Interrupt flags (overflow) */
#define INTR_FLAG_1				42
/* Interrupt flags (overflow) */
#define INTR_FLAG_2				43

/* 44 Reserved */

/* Interrupt flags ADC */
#define INTR_FLAG_ADC1			45

/* 46 Reserved */

/* Interrupt flags ADC */
#define INTR_FLAG_ADC2			47
/* INT1 interrupt control */
#define INT1_CTRL				48
/* INT2 interrupt control */
#define INT2_CTRL				49

/* 50 Reserved */

/* DMCLK/GPIO2 control */
#define GPIO2_CTRL				51
/* DMDIN/GPIO1 control */
#define GPIO1_CTRL				52
/* DOUT Control */
#define DOUT_CTRL				53

/* 54-56 Reserved */

/* ADC sync control 1 */
#define SYNC_CTRL_1				57
/* ADC sync control 2 */
#define SYNC_CTRL_2				58
/* ADC CIC filter gain control */
#define CIC_GAIN_CTRL			59

/* 60 Reserved */

/* ADC processing block selection  */
#define PRB_SELECT				61
/* Programmable instruction mode control bits */
#define INST_MODE_CTRL			62

/* 63-79 Reserved */

/* Digital microphone polarity control */
#define MIC_POLARITY_CTRL		80
/* ADC Digital */
#define ADC_DIGITAL				81
/* ADC Fine Gain Adjust */
#define	ADC_FGA					82
/* Left ADC Channel Volume Control */
#define LADC_VOL				83
/* Right ADC Channel Volume Control */
#define RADC_VOL				84
/* ADC phase compensation */
#define ADC_PHASE_COMP			85
/* Left Channel AGC Control Register 1 */
#define LEFT_CHN_AGC_1			86
/* Left Channel AGC Control Register 2 */
#define LEFT_CHN_AGC_2			87
/* Left Channel AGC Control Register 3 */
#define LEFT_CHN_AGC_3			88
/* Left Channel AGC Control Register 4 */
#define LEFT_CHN_AGC_4			89
/* Left Channel AGC Control Register 5 */
#define LEFT_CHN_AGC_5			90
/* Left Channel AGC Control Register 6 */
#define LEFT_CHN_AGC_6			91
/* Left Channel AGC Control Register 7 */
#define LEFT_CHN_AGC_7			92
/* Left AGC gain */
#define LEFT_AGC_GAIN			93
/* Right Channel AGC Control Register 1 */
#define RIGHT_CHN_AGC_1			94
/* Right Channel AGC Control Register 2 */
#define RIGHT_CHN_AGC_2 		95
/* Right Channel AGC Control Register 3 */
#define RIGHT_CHN_AGC_3			96
/* Right Channel AGC Control Register 4 */
#define RIGHT_CHN_AGC_4			97
/* Right Channel AGC Control Register 5 */
#define RIGHT_CHN_AGC_5			98
/* Right Channel AGC Control Register 6 */
#define RIGHT_CHN_AGC_6			99
/* Right Channel AGC Control Register 7 */
#define RIGHT_CHN_AGC_7			100
/* Right AGC gain */
#define RIGHT_AGC_GAIN			101

/* 102-127 Reserved */

/****************************************************************************/
/* 							 Page 1 Registers 						 		*/
/****************************************************************************/
#define PAGE_1					128

/* 1-25 Reserved */

/* Dither control */
#define DITHER_CTRL			(PAGE_1 + 26)

/* 27-50 Reserved */

/* MICBIAS Configuration Register */
#define MICBIAS_CTRL		(PAGE_1 + 51)
/* Left ADC input selection for Left PGA */
#define LEFT_PGA_SEL_1		(PAGE_1 + 52)

/* 53 Reserved */

/* Right ADC input selection for Left PGA */
#define LEFT_PGA_SEL_2		(PAGE_1 + 54)
/* Right ADC input selection for right PGA */
#define RIGHT_PGA_SEL_1		(PAGE_1 + 55)

/* 56 Reserved */

/* Right ADC input selection for right PGA */
#define RIGHT_PGA_SEL_2		(PAGE_1 + 57)

/* 58 Reserved */

/* Left analog PGA settings */
#define LEFT_APGA_CTRL		(PAGE_1 + 59)
/* Right analog PGA settings */
#define RIGHT_APGA_CTRL		(PAGE_1 + 60)
/* ADC Low current Modes */
#define LOW_CURRENT_MODES	(PAGE_1 + 61)
/* ADC analog PGA flags */
#define ANALOG_PGA_FLAGS	(PAGE_1 + 62)

/* 63-127 Reserved */

/****************************************************************************/
/*						Macros and definitions							   	*/
/****************************************************************************/

/* ADC3xxx register space */
#define ADC3xxx_CACHEREGNUM		192
#define ADC3xxx_PAGE_SIZE		128

#define ADC3xxx_RATES   SNDRV_PCM_RATE_8000_96000
#define ADC3xxx_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
             SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)

/* bits defined for easy usage */
#define D7                    (0x01 << 7)
#define D6                    (0x01 << 6)
#define D5                    (0x01 << 5)
#define D4                    (0x01 << 4)
#define D3                    (0x01 << 3)
#define D2                    (0x01 << 2)
#define D1                    (0x01 << 1)
#define D0                    (0x01 << 0)

/****************************************************************************/
/*						ADC3xxx Register bits							   	*/
/****************************************************************************/
/* PLL Enable bits */
#define ENABLE_PLL              D7
#define ENABLE_NADC             D7
#define ENABLE_MADC             D7
#define ENABLE_BCLK             D7

/* Power bits */
#define LADC_PWR_ON     		D7
#define RADC_PWR_ON     		D6

#define SOFT_RESET              D0
#define BCLK_MASTER          	D3
#define WCLK_MASTER         	D2

/* Interface register masks */
#define FMT_MASK				(D7|D6|D3|D2)
#define WLENGTH_MASK			(D5|D4)

/* PLL P/R bit offsets */
#define PLLP_SHIFT      	4
#define PLLR_SHIFT      	0
#define PLL_PR_MASK			0x7F
#define PLLJ_MASK			0x3F
#define PLLD_MSB_MASK		0x3F
#define PLLD_LSB_MASK		0xFF
#define NADC_MASK			0x7F
#define MADC_MASK			0x7F
#define AOSR_MASK			0xFF
#define IADC_MASK			0xFF
#define BDIV_MASK			0x7F

/* PLL_CLKIN bits */
#define PLL_CLKIN_SHIFT			2
#define PLL_CLKIN_MCLK			0x0
#define PLL_CLKIN_BCLK			0x1
#define PLL_CLKIN_ZERO			0x3

/* CODEC_CLKIN bits */
#define CODEC_CLKIN_SHIFT		0
#define CODEC_CLKIN_MCLK		0x0
#define CODEC_CLKIN_BCLK		0x1
#define CODEC_CLKIN_PLL_CLK		0x3

#define USE_PLL					(PLL_CLKIN_MCLK << PLL_CLKIN_SHIFT) |	\
					            (CODEC_CLKIN_PLL_CLK << CODEC_CLKIN_SHIFT)

/*  Analog PGA control bits */
#define LPGA_MUTE				D7
#define RPGA_MUTE				D7

#define LPGA_GAIN_MASK			0x7F
#define RPGA_GAIN_MASK			0x7F

/* ADC current modes */
#define ADC_LOW_CURR_MODE		D0

/* Left ADC Input selection bits */
#define LCH_SEL1_SHIFT			0
#define LCH_SEL2_SHIFT			2
#define LCH_SEL3_SHIFT			4
#define LCH_SEL4_SHIFT			6

#define LCH_SEL1X_SHIFT			0
#define LCH_SEL2X_SHIFT			2
#define LCH_SEL3X_SHIFT			4
#define LCH_COMMON_MODE			D6
#define BYPASS_LPGA				D7

/* Right ADC Input selection bits */
#define RCH_SEL1_SHIFT			0
#define RCH_SEL2_SHIFT			2
#define RCH_SEL3_SHIFT			4
#define RCH_SEL4_SHIFT			6

#define RCH_SEL1X_SHIFT			0
#define RCH_SEL2X_SHIFT			2
#define RCH_SEL3X_SHIFT			4
#define RCH_COMMON_MODE			D6
#define BYPASS_RPGA				D7

/* MICBIAS control bits */
#define MICBIAS1_SHIFT			5
#define MICBIAS2_SHIFT			3

#define ADC_MAX_VOLUME			64
#define ADC_POS_VOL			24

/****************** RATES TABLE FOR ADC3xxx ************************/
struct adc3xxx_rate_divs {
	u32 mclk;
	u32 rate;
	u8 pll_p;
	u8 pll_r;
	u8 pll_j;
	u16 pll_d;
	u8 nadc;
	u8 madc;
	u8 aosr;
	u8 bdiv_n;
	u8 iadc;
};

#endif /* _ADC3xxx_AMB_H */
