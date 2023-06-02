/*
* ES8374.h  --  ES8374 ALSA SoC Audio Codec
*
*
*
* Authors:
*
* Based on ES8374.h by David Yang
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef _ES8374_H
#define _ES8374_H

/*       THE REGISTER DEFINITION FORMAT            */
/*
*	ES8374_REGISTER NAME_REG_REGISTER ADDRESS
*/
#define ES8374_RESET_REG00			0x00  /* this register is used to reset digital,csm,clock manager etc.*/

/*
* Clock Scheme Register definition
*/
#define ES8374_CLK_MANAGEMENT_REG01		0x01 /* The register is used to turn on/off the clock for ADC, DAC, MCLK, BCLK and CLASS D */
#define ES8374_CLK_MANAGEMENT_REG02		0x02 /* ADC, PLL clock */


#define ES8374_ADC_OSR_REG03			0x03 /* The ADC OSR setting */
#define ES8374_DAC_FLT_CNT_REG04		0x04 /* The DAC filter counter waiting cycles control */
#define ES8374_CLK_DIV_REG05			0x05 /* ADC&DAC internal mclk Divider */
#define ES8374_LRCK_DIV_REG06			0x06 /* The  LRCK divider */
#define ES8374_LRCK_DIV_REG07			0x07
#define ES8374_CLASS_D_DIV_REG08		0x08 /* Class D clock divider */
/*
* PLL control register definition
*/
#define ES8374_PLL_CONTROL1_REG09		0x09 /* Register 0x09 for PLL reset, power up/down, dither and divider settting */
#define ES8374_PLL_CONTROL2_REG0A		0x0A /* Register 0x0A for PLL low power, gain, supply voltage and  vco setting */
#define ES8374_PLL_N_REG0B			0x0B /* Register 0x0B for N and pll calibrate */
#define ES8374_PLL_K_REG0C			0x0C /* Register 0x0C - 0x0E for PLLK */
#define ES8374_PLL_K_REG0D			0x0D
#define ES8374_PLL_K_REG0E			0x0E
/*
*	The serial digital audio port
*/
#define ES8374_MS_BCKDIV_REG0F			0x0F /* The setting for Master/Slave mode and BCLK divider */
#define ES8374_ADC_FMT_REG10			0x10 /* ADC Format	*/
#define ES8374_DAC_FMT_REG11			0x11 /* DAC Format	*/

/*
*	The system Control
*/
#define ES8374_CHP_INI_REG12			0x12 /* The time control for chip initialization	*/
#define ES8374_POWERUP_REG13			0x13 /* The time control for power up	*/
/*
*	The analog control
*/
#define ES8374_ANA_REF_REG14			0x14 /* The analog reference */
#define ES8374_ANA_PWR_CTL_REG15		0x15 /* The analog power up/down */
#define ES8374_ANA_LOW_PWR_REG16		0x16 /* Low power setting*/
#define ES8374_ANA_REF_LP_REG17			0x17 /* Reference and low power setting */
#define ES8374_ANA_BIAS_REG18		  	0x18 /* Bias selecting */
/*
*	MONO OUT
*/
#define ES8374_MONO_MIX_REG1A			0x1A /* MonoMixer */
#define ES8374_MONO_GAIN_REG1B			0x1B /* Mono Out Gain */
/*
* SPEAKER OUT
*/
#define ES8374_SPK_MIX_REG1C			0x1C /* Speaker Mixer */
#define ES8374_SPK_MIX_GAIN_REG1D		0x1D /* Speaker Mixer Gain */
#define ES8374_SPK_OUT_GAIN_REG1E		0x1E /* Speaker Output Gain */
#define ES8374_SPK_CTL_REG1F		 	 0x1F /* Speaker control */
#define ES8374_SPK_CTL_REG20		  	0x20 /* Speaker control */
/*
* Analog In
*/
#define ES8374_AIN_PWR_SRC_REG21		0x21 /* Power control and source selecting for AIN*/
#define ES8374_AIN_PGA_REG22		  	0x22 /* AIN PGA Gain Control */
/*
*	ADC Control
*/
#define ES8374_ADC_CONTROL_REG24		0x24 /* adc and DMIC Setting */
#define ES8374_ADC_VOLUME_REG25			0x25 /* adc volume */
/*
* ADC ALC
*/
#define ES8374_ALC_EN_MAX_GAIN_REG26		0x26 /* adc alc enable and max gain setting */
#define ES8374_ALC_MIN_GAIN_REG27		0x27 /* ADC Alc min gain */
#define ES8374_ALC_LVL_HLD_REG28		0x28 /* alc level and hold time control */
#define ES8374_ALC_DCY_ATK_REG29		0x29 /* ALC DCY and ATK time control */
#define ES8374_ALC_WIN_SIZE_REG2A		0x2A /* ALC WIN Size */
#define ES8374_ALC_NGTH_REG2B		 	0x2B /* alc noise gate setting */
/*
*	ADC HPF setting
*/
#define ES8374_ADC_HPF_REG2C		    	0x2C /* The high pass filter coefficient*/
/*
*	Equalizer SRC setting
*/
#define ES8374_EQ_SRC_REG2D			0x2D /* adc src, dac src and EQ SRC setting */

/*
* ADC 1st shelving filter
*/
#define ES8374_ADC_SHV_A_REG2E		 	0x2E /* adc shelving A*/
#define ES8374_ADC_SHV_A_REG2F		  	0x2F
#define ES8374_ADC_SHV_A_REG30		 	0x30
#define ES8374_ADC_SHV_A_REG31			0x31

#define ES8374_ADC_SHV_B_REG32		 	0x32 /* ADC Shelving B*/
#define ES8374_ADC_SHV_B_REG33		  	0x33
#define ES8374_ADC_SHV_B_REG34		  	0x34
#define ES8374_ADC_SHV_B_REG35			0x35

/*
* DAC control
*/
#define ES8374_DAC_CONTROL_REG36		0x36 /* dac control1 */
#define ES8374_DAC_CONTROL_REG37		0x37 /* dac control2 */

#define ES8374_DAC_VOLUME_REG38		    	0x38 /* dac volume */
#define ES8374_DAC_OFFSET_REG39			0x39 /* DAC offset */

/*
* DAC 2nd shleving filter
*/
/* register 0x3A to 0x44 is for dac shelving filter */
/*
* Equalizer coeffient
*/
/* register 0x45 to 0x6c is for 2nd equalizer filter, this filter can be shared by DAC and ADC */

#define ES8374_GPIO_INSERT_REG6D	    	0x6D /* GPIO & HP INSERT CONTROL */
#define ES8374_FLAG_REG6E	  		0x6E /* Flag register */

/* The End Of Register definition */

#define ES8374_PLL			1
#define ES8374_PLL_SRC_FRM_MCLK		1

#define ES8374_CLKID_MCLK		0
#define ES8374_CLKID_PLLO		1

#define ES8374_MAX_REGISTER		128

#endif
