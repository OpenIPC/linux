/*
    Emaltion External audio codec driver

    This file is the driver for Emaltion extended audio codec.

    @file       aud_ac108.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

/** \addtogroup mISYSAud */
//@{

#ifdef __KERNEL__
#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "aud_ac108_dbg.h"
#include "aud_ac108.h"
#else
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "aud_ac108_dbg.h"
#include "aud_ac108.h"
#endif

unsigned int aud_ac108_debug_level = NVT_DBG_WRN;

static BOOL			ac108_gain_balance = TRUE; //balance gain of each mic
static UINT32		digital_gain = 0xA0;
static BOOL			ac108_opened = FALSE;

static ER aud_open_ac108(CTL_AUD_ID id);                                               ///< initial sensor flow
static ER aud_close_ac108(CTL_AUD_ID id);                                              ///< un-initial sensor flow
static ER aud_start_ac108(CTL_AUD_ID id);                                              ///< enter sensor sleep mode
static ER aud_stop_ac108(CTL_AUD_ID id);                                             ///< exit sensor sleep mode
static ER aud_set_cfg_ac108(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data);   ///< set sensor information (if sensor driver not support feature, pls return E_NOSPT)
static ER aud_get_cfg_ac108(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data);   ///< get sensor information (if sensor driver not support feature, pls return E_NOSPT)


static CTL_AUD_DRV_TAB ac108_aud_drv_tab = {
	aud_open_ac108,
	aud_close_ac108,
	aud_start_ac108,
	aud_stop_ac108,
	aud_set_cfg_ac108,
	aud_get_cfg_ac108,
};

CTL_AUD_DRV_TAB *aud_get_drv_tab_ac108(void)
{
	return &ac108_aud_drv_tab;
}

static ER aud_open_ac108(CTL_AUD_ID id)
{
	UINT32 temp, vec;
	UINT32 m1, m2, n, k1, k2;

	if (ac108_opened) {
		return E_OK;
	}

	//reset
	//aud_i2c_write(CHIP_RST, CHIP_RST_VAL);
	//Delay_DelayMs(3);


	#if 1
	// SYSCLK_SRC_PLL
	aud_i2c_update(SYSCLK_CTRL, 0x1 << SYSCLK_SRC, SYSCLK_SRC_PLL << SYSCLK_SRC);
	#else
	// SYSCLK_SRC_MCLK
	aud_i2c_update(SYSCLK_CTRL, 0x1 << SYSCLK_SRC, SYSCLK_SRC_MCLK << SYSCLK_SRC);
	#endif


	/*
		ac108_set_fmt
	*/

	#if 1
	// slave mode
	// 0x30:chip is slave mode, BCLK & LRCK input,enable SDO1_EN and
	//  SDO2_EN, Transmitter Block Enable, Globe Enable
	aud_i2c_update(I2S_CTRL, 0x03 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN,
						  0x00 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN);
	#else
	// master mode
	aud_i2c_update(I2S_CTRL, 0x03 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN,
					  0x00 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN);
	/* multi_chips: only one chip set as Master, and the others also need to set as Slave */
	aud_i2c_update(I2S_CTRL, 0x3 << LRCK_IOEN, 0x3 << LRCK_IOEN);
	#endif


	/* ac108_configure_power */
	//0x06:Enable Analog LDO
	aud_i2c_update(PWR_CTRL6, 0x01 << LDO33ANA_ENABLE, 0x01 << LDO33ANA_ENABLE);
	// 0x07:
	// Control VREF output and micbias voltage ?
	// REF faststart disable, enable Enable VREF (needed for Analog
	// LDO and MICBIAS)
	aud_i2c_update(PWR_CTRL7, 0x1f << VREF_SEL | 0x01 << VREF_FASTSTART_ENABLE | 0x01 << VREF_ENABLE,
					   0x13 << VREF_SEL | 0x00 << VREF_FASTSTART_ENABLE | 0x01 << VREF_ENABLE);
	// 0x09:
	// Disable fast-start circuit on VREFP
	// VREFP_RESCTRL=00=1 MOhm
	// IGEN_TRIM=100=+25%
	// Enable VREFP (needed by all audio input channels)
	aud_i2c_update(PWR_CTRL9, 0x01 << VREFP_FASTSTART_ENABLE | 0x03 << VREFP_RESCTRL | 0x07 << IGEN_TRIM | 0x01 << VREFP_ENABLE,
					   0x00 << VREFP_FASTSTART_ENABLE | 0x00 << VREFP_RESCTRL | 0x04 << IGEN_TRIM | 0x01 << VREFP_ENABLE);


	//0x31: 0: normal mode, negative edge drive and positive edge sample
	//1: invert mode, positive edge drive and negative edge sample
	aud_i2c_update(I2S_BCLK_CTRL,  0x01 << BCLK_POLARITY, BCLK_NORMAL_DRIVE_N_SAMPLE_P << BCLK_POLARITY);
	// 0x32: same as 0x31
	aud_i2c_update(I2S_LRCK_CTRL1, 0x01 << LRCK_POLARITY, LRCK_LEFT_HIGH_RIGHT_LOW << LRCK_POLARITY);

	// 0x34:Encoding Mode Selection,Mode
	// Selection,data is offset by 1 BCLKs to LRCK
	// normal mode for the last half cycle of BCLK in the slot ?
	// turn to hi-z state (TDM) when not transferring slot ?
	aud_i2c_update(I2S_FMT_CTRL1,	0x01 << ENCD_SEL | 0x03 << MODE_SEL | 0x01 << TX2_OFFSET |
						0x01 << TX1_OFFSET | 0x01 << TX_SLOT_HIZ | 0x01 << TX_STATE,
								0 << ENCD_SEL	|
								LEFT_JUSTIFIED_FORMAT << MODE_SEL	|
								1 << TX2_OFFSET	|
								1 << TX1_OFFSET	|
								0x00 << TX_SLOT_HIZ	|
								0x01 << TX_STATE);

	// 0x60:
	// MSB / LSB First Select: This driver only support MSB First Select .
	// OUT2_MUTE,OUT1_MUTE shoule be set in widget.
	// LRCK = 1 BCLK width
	// Linear PCM
	//  TODO:pcm mode, bit[0:1] and bit[2] is special
	aud_i2c_update(I2S_FMT_CTRL3,	0x01 << TX_MLS | 0x03 << SEXT  | 0x01 << LRCK_WIDTH | 0x03 << TX_PDM,
						0x00 << TX_MLS | 0x03 << SEXT  | 0x00 << LRCK_WIDTH | 0x00 << TX_PDM);
	aud_i2c_write(HPF_EN, 0x00);

	m1 = 4;
	m2 = 0;
	n  = 128;
	k1 = 24;
	k2 = 0;

	/*
		ac108_hw_params
	*/

	// TDM mode or normal mode
	aud_i2c_write(I2S_LRCK_CTRL2, 128 - 1);
	aud_i2c_update(I2S_LRCK_CTRL1, 0x03 << 0, 0x00);

	/**
	 * 0x35:
	 * TX Encoding mode will add  4bits to mark channel number
	 * TODO: need a chat to explain this
	 */
	aud_i2c_update(I2S_FMT_CTRL2, 0x07 << SAMPLE_RESOLUTION | 0x07 << SLOT_WIDTH_SEL,
						7 << SAMPLE_RESOLUTION
							| 7 << SLOT_WIDTH_SEL);

	/**
	 * 0x60:
	 * ADC Sample Rate synchronised with I2S1 clock zone
	 */
	aud_i2c_update(ADC_SPRC, 0x0f << ADC_FS_I2S1, 8 << ADC_FS_I2S1);
	aud_i2c_write(HPF_EN, 0x0F);

	/*ac108_config_pll(ac10x, ac108_sample_rate[rate].real_val, 0); */
	/* 0x11,0x12,0x13,0x14: Config PLL DIV param M1/M2/N/K1/K2 */
	aud_i2c_update(PLL_CTRL5, 0x1f << PLL_POSTDIV1 | 0x01 << PLL_POSTDIV2,
					   k1 << PLL_POSTDIV1 | k2 << PLL_POSTDIV2);
	aud_i2c_update(PLL_CTRL4, 0xff << PLL_LOOPDIV_LSB, (unsigned char)n << PLL_LOOPDIV_LSB);
	aud_i2c_update(PLL_CTRL3, 0x03 << PLL_LOOPDIV_MSB, (n >> 8) << PLL_LOOPDIV_MSB);
	aud_i2c_update(PLL_CTRL2, 0x1f << PLL_PREDIV1 | 0x01 << PLL_PREDIV2,
					    m1 << PLL_PREDIV1 | m2 << PLL_PREDIV2);

	/*0x18: PLL clk lock enable*/
	aud_i2c_update(PLL_LOCK_CTRL, 0x1 << PLL_LOCK_EN, 0x1 << PLL_LOCK_EN);

	/*0x10: PLL Common voltage Enable, PLL Enable,PLL loop divider factor detection enable*/
	aud_i2c_update(PLL_CTRL1, 0x01 << PLL_EN | 0x01 << PLL_COM_EN | 0x01 << PLL_NDET,
					   0x01 << PLL_EN | 0x01 << PLL_COM_EN | 0x01 << PLL_NDET);

	/**
	 * 0x20: enable pll, pll source from mclk/bclk, sysclk source from pll, enable sysclk
	 */
	aud_i2c_update(SYSCLK_CTRL, 0x01 << PLLCLK_EN | 0x03  << PLLCLK_SRC | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN,
					     0x01 << PLLCLK_EN |0<< PLLCLK_SRC | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN);

	aud_i2c_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 2 << BCLKDIV);

	/*
		ac108_multi_chips_slots
	*/
	vec = 0xFUL;
	//aud_i2c_write(I2S_TX1_CTRL1, 8 - 1);
	aud_i2c_write(I2S_TX1_CTRL1, 4 - 1);
	aud_i2c_write(I2S_TX1_CTRL2, (vec >> 0) & 0xFF);
	aud_i2c_write(I2S_TX1_CTRL3, (vec >> 8) & 0xFF);

#if 1
	vec = (0x2 << 0 | 0x3 << 2 | 0x0 << 4 | 0x1 << 6);
#else
	//DBG_DUMP("AC108 Set to 2CH Mode\r\n");
	//2CH TEST
	vec = (0x2 << 0 | 0x0 << 2 | 0x3 << 4 | 0x1 << 6);
#endif
	aud_i2c_write(I2S_TX1_CHMP_CTRL1, (vec >>  0) & 0xFF);
	aud_i2c_write(I2S_TX1_CHMP_CTRL2, (vec >>  8) & 0xFF);
	aud_i2c_write(I2S_TX1_CHMP_CTRL3, (vec >> 16) & 0xFF);
	aud_i2c_write(I2S_TX1_CHMP_CTRL4, (vec >> 24) & 0xFF);

	/* Digital gain Default value */
	aud_i2c_write(ADC1_DVOL_CTRL, 0xA7);//CH3
	aud_i2c_write(ADC2_DVOL_CTRL, 0xA7);//CH2
	aud_i2c_write(ADC3_DVOL_CTRL, 0xA7);//CH1
	aud_i2c_write(ADC4_DVOL_CTRL, 0x9C);//CH0
	//aud_i2c_write(ADC1_DVOL_CTRL, 0x98);//CH3
	//aud_i2c_write(ADC2_DVOL_CTRL, 0x98);//CH2
	//aud_i2c_write(ADC3_DVOL_CTRL, 0x98);//CH1
	//aud_i2c_write(ADC4_DVOL_CTRL, 0x98);//CH0


	/* PGA gain Default value */
	aud_i2c_write(ANA_PGA1_CTRL, 0x00<<1);
	aud_i2c_write(ANA_PGA2_CTRL, 0x00<<1);
	aud_i2c_write(ANA_PGA3_CTRL, 0x00<<1);
	aud_i2c_write(ANA_PGA4_CTRL, 0x00<<1);

	// MIC-BIAS ON
	aud_i2c_update(ANA_ADC1_CTRL1, 0x07 << ADC1_MICBIAS_EN,  0x07 << ADC1_MICBIAS_EN);
	aud_i2c_update(ANA_ADC2_CTRL1, 0x07 << ADC2_MICBIAS_EN,  0x07 << ADC2_MICBIAS_EN);
	aud_i2c_update(ANA_ADC3_CTRL1, 0x07 << ADC3_MICBIAS_EN,  0x07 << ADC3_MICBIAS_EN);
	aud_i2c_update(ANA_ADC4_CTRL1, 0x07 << ADC4_MICBIAS_EN,  0x07 << ADC4_MICBIAS_EN);

	aud_i2c_write(ADC_DIG_EN,		0x1F);
	aud_i2c_write(ANA_ADC4_CTRL7,	0x0F);
	aud_i2c_write(ANA_ADC4_CTRL6,	0x20);

	/*
		ac108_trigger
	*/
	temp = aud_i2c_read(I2S_CTRL);
	if ((temp & (0x02 << LRCK_IOEN)) && (temp & (0x01 << LRCK_IOEN)) == 0) {
		/* disable global clock */
		aud_i2c_update(I2S_CTRL, 0x1 << TXEN | 0x1 << GEN, 0x1 << TXEN | 0x0 << GEN);
	}
	/*0x21: Module clock enable<I2S, ADC digital, MIC offset Calibration, ADC analog>*/
	aud_i2c_write(MOD_CLK_EN, 1 << _I2S | 1 << ADC_DIGITAL | 1 << MIC_OFFSET_CALIBRATION | 1 << ADC_ANALOG);
	/*0x22: Module reset de-asserted<I2S, ADC digital, MIC offset Calibration, ADC analog>*/
	aud_i2c_write(MOD_RST_CTRL, 1 << _I2S | 1 << ADC_DIGITAL | 1 << MIC_OFFSET_CALIBRATION | 1 << ADC_ANALOG);

	// AC108 as Master!!
	aud_i2c_write(I2S_CTRL, aud_i2c_read(I2S_CTRL)|0xC0);

	ac108_opened = TRUE;

	return E_OK;
}

static ER aud_close_ac108(CTL_AUD_ID id)
{
	if (!ac108_opened) {
		return E_OK;
	}

	ac108_opened = FALSE;
	//aud_i2c_remove_driver(0);

	return E_OK;
}
static ER aud_start_ac108(CTL_AUD_ID id)
{
	return E_OK;
}
static ER aud_stop_ac108(CTL_AUD_ID id)
{
	return E_OK;
}

static ER aud_set_cfg_ac108(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_AUDDRV_CFGID_SET_SAMPLE_RATE:
	{
        UINT32 sample_rate = *(UINT32*)data;
		if(sample_rate == 48000) {
			aud_i2c_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 2 << BCLKDIV);
		} else if (sample_rate == 16000) {
			aud_i2c_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 4 << BCLKDIV);
		} else if (sample_rate == 8000) {
			aud_i2c_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 6 << BCLKDIV);
		} else if (sample_rate == 24000) {
			aud_i2c_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 3 << BCLKDIV);
		}
		break;
	}
	case CTL_AUDDRV_CFGID_SET_VOLUME:
	{
		if (id == CTL_AUD_ID_CAP) {
			UINT32 value;
			UINT32 gain = *(UINT32*)data;

			DBG_DUMP("gain = %d\r\n", gain);

			if (gain == 0) {
				value = 0;

				aud_i2c_write(ADC1_DVOL_CTRL, 0x00);
				aud_i2c_write(ADC2_DVOL_CTRL, 0x00);
				aud_i2c_write(ADC3_DVOL_CTRL, 0x00);
				aud_i2c_write(ADC4_DVOL_CTRL, 0x00);

				aud_i2c_write(ANA_PGA1_CTRL, value<<1);
				aud_i2c_write(ANA_PGA2_CTRL, value<<1);
				aud_i2c_write(ANA_PGA3_CTRL, value<<1);
				aud_i2c_write(ANA_PGA4_CTRL, value<<1);

			} else {
				gain = gain/12;

				value = ((gain - 1)<<2)+3;

				DBG_DUMP("gain = %d, value = %d\r\n", gain, value);

				aud_i2c_write(ADC1_DVOL_CTRL, digital_gain);
				aud_i2c_write(ADC2_DVOL_CTRL, digital_gain);
				aud_i2c_write(ADC3_DVOL_CTRL, digital_gain);
				aud_i2c_write(ADC4_DVOL_CTRL, digital_gain);

				/* PGA gain Max 0x1F */
				aud_i2c_write(ANA_PGA1_CTRL, value<<1); // Our Ch3

				aud_i2c_write(ANA_PGA3_CTRL, value<<1); // Our CH1

				if (ac108_gain_balance) {
					//making volume balance
					if (value > 6)
						value -= 6;
					else
						value = 0;
				}
				aud_i2c_write(ANA_PGA4_CTRL, value<<1); // Our CH0
				aud_i2c_write(ANA_PGA2_CTRL, value<<1); // Our CH2
			}
		}
		break;
	}
	default:
		break;
	}

	return E_OK;
}

static ER aud_get_cfg_ac108(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data)
{
	return E_OK;
}

int aud_init_ac108(void)
{
	INT32 ret;
	CTL_AUD_DRV_TAB *drv_tab = NULL;

	drv_tab = aud_get_drv_tab_ac108();

	ret = ctl_aud_reg_auddrv("nvt_aud_ac108", drv_tab);
	if (ret != E_OK) {
		DBG_ERR("register audio driver fail \r\n");
	}

	ret = aud_i2c_init_driver();

	if (ret != E_OK) {
		DBG_ERR("init. i2c driver fail\r\n");
		return -1;
	}

	return ret;
}

int aud_uninit_ac108(void)
{
	INT32 ret = 0;

	aud_i2c_remove_driver(0);

	return ret;
}


//@}
