/*
 * ak4951_amb.c  --  audio driver for AK4951
 *
 * Copyright 2014 Ambarella Ltd.
 *
 * Author: Diao Chengdong <cddiao@ambarella.com>
 *
 * History:
 *	2014/03/27 - created
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "ak4951_amb.h"

//#define PLL_32BICK_MODE
//#define PLL_64BICK_MODE

//#define AK4951_DEBUG			//used at debug mode
//#define AK4951_CONTIF_DEBUG		//used at debug mode

#ifdef AK4951_DEBUG
#define akdbgprt printk
#else
#define akdbgprt(format, arg...) do {} while (0)
#endif

#define LINEIN1_MIC_BIAS_CONNECT
#define LINEIN2_MIC_BIAS_CONNECT

/* AK4951 Codec Private Data */
struct ak4951_priv {
	unsigned int rst_pin;
	unsigned int rst_active;
	unsigned int sysclk;
	unsigned int clkid;
	struct regmap *regmap;
	struct i2c_client* i2c_clt;
	u8 reg_cache[AK4951_MAX_REGISTERS];
	int onStereo;
	int mic;
	u8 fmt;
};

struct ak4951_priv *ak4951_data;

/*
 * ak4951 register
 */
static struct reg_default  ak4951_reg_defaults[] = {
	{ 0,  0x00 },
	{ 1,  0x00 },
	{ 2,  0x06 },
	{ 3,  0x00 },
	{ 4,  0x44 },
	{ 5,  0x52 },
	{ 6,  0x09 },
	{ 7,  0x14 },
	{ 8,  0x00 },
	{ 9,  0x00 },
	{ 10, 0x60 },
	{ 11, 0x00 },
	{ 12, 0xE1 },
	{ 13, 0xE1 },
	{ 14, 0xE1 },
	{ 15, 0x00 },
	{ 16, 0x80 },
	{ 17, 0x80 },
	{ 18, 0x00 },
	{ 19, 0x18 },
	{ 20, 0x18 },
	{ 21, 0x00 },
	{ 22, 0x00 },
	{ 23, 0x00 },
	{ 24, 0x00 },
	{ 25, 0x00 },
	{ 26, 0x0C },
	{ 27, 0x01 },
	{ 28, 0x00 },
	{ 29, 0x03 },
	{ 30, 0xA9 },
	{ 31, 0x1F },
	{ 32, 0xAD },
	{ 33, 0x20 },
	{ 34, 0x7F },
	{ 35, 0x0C },
	{ 36, 0xFF },
	{ 37, 0x38 },
	{ 38, 0xA2 },
	{ 39, 0x83 },
	{ 40, 0x80 },
	{ 41, 0x2E },
	{ 42, 0x5B },
	{ 43, 0x23 },
	{ 44, 0x07 },
	{ 45, 0x28 },
	{ 46, 0xAA },
	{ 47, 0xEC },
	{ 48, 0x00 },
	{ 49, 0x00 },
	{ 50, 0x9F },
	{ 51, 0x00 },
	{ 52, 0x2B },
	{ 53, 0x3F },
	{ 54, 0xD4 },
	{ 55, 0xE0 },
	{ 56, 0x6A },
	{ 57, 0x00 },
	{ 58, 0x1B },
	{ 59, 0x3F },
	{ 60, 0xD4 },
	{ 61, 0xE0 },
	{ 62, 0x6A },
	{ 63, 0x00 },
	{ 64, 0xA2 },
	{ 65, 0x3E },
	{ 66, 0xD4 },
	{ 67, 0xE0 },
	{ 68, 0x6A },
	{ 69, 0x00 },
	{ 70, 0xA8 },
	{ 71, 0x38 },
	{ 72, 0xD4 },
	{ 73, 0xE0 },
	{ 74, 0x6A },
	{ 75, 0x00 },
	{ 76, 0x96 },
	{ 77, 0x1F },
	{ 78, 0xD4 },
	{ 79, 0xE0 },
};

static bool ak4951_volatile_register(struct device *dev,
							unsigned int reg)
{
	switch (reg) {
		case AK4951_00_POWER_MANAGEMENT1:
		case AK4951_01_POWER_MANAGEMENT2:
		case AK4951_02_SIGNAL_SELECT1:
		case AK4951_03_SIGNAL_SELECT2:
		case AK4951_04_SIGNAL_SELECT3:
		case AK4951_05_MODE_CONTROL1:
		case AK4951_06_MODE_CONTROL2:
		case AK4951_07_MODE_CONTROL3:
		case AK4951_08_DIGITL_MIC:
		case AK4951_09_TIMER_SELECT:
		case AK4951_0A_ALC_TIMER_SELECT:
		case AK4951_0B_ALC_MODE_CONTROL1:
		case AK4951_0C_ALC_MODE_CONTROL2:
		case AK4951_0D_LCH_INPUT_VOLUME_CONTROL:
		case AK4951_0E_RCH_INPUT_VOLUME_CONTROL:
		case AK4951_0F_ALC_VOLUME:
		case AK4951_10_LCH_MIC_GAIN_SETTING:
		case AK4951_11_RCH_MIC_GAIN_SETTING:
		case AK4951_12_BEEP_CONTROL:
		case AK4951_13_LCH_DIGITAL_VOLUME_CONTROL:
		case AK4951_14_RCH_DIGITAL_VOLUME_CONTROL:
		case AK4951_15_EQ_COMMON_GAIN_SELECT:
		case AK4951_16_EQ2_COMMON_GAIN_SELECT:
		case AK4951_17_EQ3_COMMON_GAIN_SELECT:
		case AK4951_18_EQ4_COMMON_GAIN_SELECT:
		case AK4951_19_EQ5_COMMON_GAIN_SELECT:
		case AK4951_1A_AUTO_HPF_CONTROL:
		case AK4951_1B_DIGITAL_FILTER_SELECT1:
		case AK4951_1C_DIGITAL_FILTER_SELECT2:
		case AK4951_1D_DIGITAL_FILTER_MODE:
		case AK4951_1E_HPF2_COEFFICIENT0:
		case AK4951_1F_HPF2_COEFFICIENT1:
		case AK4951_20_HPF2_COEFFICIENT2:
		case AK4951_21_HPF2_COEFFICIENT3:
		case AK4951_22_LPF_COEFFICIENT0:
		case AK4951_23_LPF_COEFFICIENT1:
		case AK4951_24_LPF_COEFFICIENT2:
		case AK4951_25_LPF_COEFFICIENT3:
		case AK4951_26_FIL3_COEFFICIENT0:
		case AK4951_27_FIL3_COEFFICIENT1:
		case AK4951_28_FIL3_COEFFICIENT2:
		case AK4951_29_FIL3_COEFFICIENT3:
		case AK4951_2A_EQ_COEFFICIENT0:
		case AK4951_2B_EQ_COEFFICIENT1:
		case AK4951_2C_EQ_COEFFICIENT2:
		case AK4951_2D_EQ_COEFFICIENT3:
		case AK4951_2E_EQ_COEFFICIENT4:
		case AK4951_2F_EQ_COEFFICIENT5:
		case AK4951_30_DIGITAL_FILTER_SELECT3:
		case AK4951_31_DEVICE_INFO:
		case AK4951_32_E1_COEFFICIENT0:
		case AK4951_33_E1_COEFFICIENT1:
		case AK4951_34_E1_COEFFICIENT2:
		case AK4951_35_E1_COEFFICIENT3:
		case AK4951_36_E1_COEFFICIENT4:
		case AK4951_37_E1_COEFFICIENT5:
		case AK4951_38_E2_COEFFICIENT0:
		case AK4951_39_E2_COEFFICIENT1:
		case AK4951_3A_E2_COEFFICIENT2:
		case AK4951_3B_E2_COEFFICIENT3:
		case AK4951_3C_E2_COEFFICIENT4:
		case AK4951_3D_E2_COEFFICIENT5:
		case AK4951_3E_E3_COEFFICIENT0:
		case AK4951_3F_E3_COEFFICIENT1:
		case AK4951_40_E3_COEFFICIENT2:
		case AK4951_41_E3_COEFFICIENT3:
		case AK4951_42_E3_COEFFICIENT4:
		case AK4951_43_E3_COEFFICIENT5:
		case AK4951_44_E4_COEFFICIENT0:
		case AK4951_45_E4_COEFFICIENT1:
		case AK4951_46_E4_COEFFICIENT2:
		case AK4951_47_E4_COEFFICIENT3:
		case AK4951_48_E4_COEFFICIENT4:
		case AK4951_49_E4_COEFFICIENT5:
		case AK4951_4A_E5_COEFFICIENT0:
		case AK4951_4B_E5_COEFFICIENT1:
		case AK4951_4C_E5_COEFFICIENT2:
		case AK4951_4D_E5_COEFFICIENT3:
		case AK4951_4E_E5_COEFFICIENT4:
		case AK4951_4F_E5_COEFFICIENT5:
		return true;

	default:
		return false;
	}
}

static inline int aK4951_reset(struct regmap *map)
{
	return regmap_write(map, AK4951_00_POWER_MANAGEMENT1, 0x00);
}

/*
 *  MIC Gain control:
 * from 6 to 26 dB in 6.5 dB steps
 *
 * MIC Gain control: (table 39)
 *
 * max : 0x00 : +12.0 dB
 *       ( 0.5 dB step )
 * min : 0xFE : -115.0 dB
 * mute: 0xFF
 */
static DECLARE_TLV_DB_SCALE(mgain_tlv, 600, 650, 0);

/*
 * Input Digital volume control: (Table 44)
 *
 * max : 0xF1 : +36.0 dB
 *       ( 0.375 dB step )
 * min : 0x00 : -54.375 dB
 * mute: 0x00 - 0x04
 * from -54.375 to 36 dB in 0.375 dB steps mute instead of -54.375 dB)
 */
static DECLARE_TLV_DB_SCALE(ivol_tlv, -5437, 37, 1);

/*
 * Speaker output volume control: (Table 55)
 *
 * max : 0x11: +14.9dB
 *       0x10: +11.1dB
 *		 0x01: +8.4 dB
 * min : 0x00: +6.4 dB
 * from 6.4 to 14.9 dB in 2.8 dB steps
 */
static DECLARE_TLV_DB_SCALE(spkout_tlv, 640, 280, 0);

/*
 * Output Digital volume control: (Table 49)
 * (This can be used as Bluetooth I/F output volume)
 *
 * max : 0x00: +12.0 dB
 *       ( 0.5 dB step )
 * min : 0x90: -66.0 dB
 * from -90.0 to 12 dB in 0.5 dB steps (mute instead of -90.0 dB)
 */
static DECLARE_TLV_DB_SCALE(dvol_tlv, -9000, 50, 1);

/*
 * Programmable Filter Output volume control: (Table 46)
  *
 * max : 0x00: 0.0 dB
 *       ( 6 dB step )
 * min : 0x11: -18.0 dB
 * from -18 to 0 dB in 6 dB steps
 */
//static DECLARE_TLV_DB_SCALE(pfvol_tlv, -1800, 600, 0);

/*For our Board -- cddiao*/
static const char *mic_and_lin_select[]  =
{
	"LIN",
	"MIC",
};


static const struct soc_enum ak4951_micswitch_enum[] = {
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_and_lin_select), mic_and_lin_select),
};


static int get_micstatus(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak4951_priv *ak4951 = snd_soc_codec_get_drvdata(codec);

    ucontrol->value.enumerated.item[0] = ak4951->mic;

    return 0;

}

static int set_micstatus(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak4951_priv *ak4951 = snd_soc_codec_get_drvdata(codec);

	ak4951->mic = ucontrol->value.enumerated.item[0];

	if ( ak4951->mic ) {
		snd_soc_update_bits(codec,AK4951_03_SIGNAL_SELECT2,0x0f,0x05);// LIN2 RIN2
	}else {
		snd_soc_update_bits(codec,AK4951_03_SIGNAL_SELECT2,0x0f,0x0a);// LIN3 RIN3
	}
    return 0;
}



static const char *stereo_on_select[]  =
{
	"Stereo Emphasis Filter OFF",
	"Stereo Emphasis Filter ON",
};

static const struct soc_enum ak4951_stereo_enum[] = {
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(stereo_on_select), stereo_on_select),
};

static int ak4951_writeMask(struct snd_soc_codec *, u16, u16, u16);

static int get_onstereo(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak4951_priv *ak4951 = snd_soc_codec_get_drvdata(codec);

    ucontrol->value.enumerated.item[0] = ak4951->onStereo;

    return 0;

}

static int set_onstereo(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak4951_priv *ak4951 = snd_soc_codec_get_drvdata(codec);

	ak4951->onStereo = ucontrol->value.enumerated.item[0];

	if ( ak4951->onStereo ) {
		ak4951_writeMask(codec, AK4951_1C_DIGITAL_FILTER_SELECT2, 0x30, 0x30);
	}
	else {
		ak4951_writeMask(codec, AK4951_1C_DIGITAL_FILTER_SELECT2, 0x30, 0x00);
	}

    return 0;
}

#ifdef AK4951_DEBUG
static const char *test_reg_select[]   =
{
    "read AK4951 Reg 00:2F",
    "read AK4951 Reg 30:4F",
};

static const struct soc_enum ak4951_enum[] =
{
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(test_reg_select), test_reg_select),
};

static int nTestRegNo = 0;

static int get_test_reg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    /* Get the current output routing */
    ucontrol->value.enumerated.item[0] = nTestRegNo;

    return 0;

}

static int set_test_reg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    u32    currMode = ucontrol->value.enumerated.item[0];
	int    i, value;
	int	   regs, rege;

	nTestRegNo = currMode;

	switch(nTestRegNo) {
		case 1:
			regs = 0x30;
			rege = 0x4F;
			break;
		default:
			regs = 0x00;
			rege = 0x2F;
			break;
	}

	for ( i = regs ; i <= rege ; i++ ){
		value = snd_soc_read(codec, i);
		printk("***AK4951 Addr,Reg=(%x, %x)\n", i, value);
	}

	return(0);

}
#endif

static const struct snd_kcontrol_new ak4951_snd_controls[] = {
	SOC_SINGLE_TLV("Mic Gain Control",
			AK4951_02_SIGNAL_SELECT1, 0, 0x07, 0, mgain_tlv),
	SOC_SINGLE_TLV("Input Digital Volume",
			AK4951_0D_LCH_INPUT_VOLUME_CONTROL, 0, 0xF1, 0, ivol_tlv),
	SOC_SINGLE_TLV("Speaker Output Volume",
			AK4951_03_SIGNAL_SELECT2, 6, 0x03, 0, spkout_tlv),
	SOC_SINGLE_TLV("Digital Output Volume",
			AK4951_13_LCH_DIGITAL_VOLUME_CONTROL, 0, 0xFF, 1, dvol_tlv),

    SOC_SINGLE("High Path Filter 1", AK4951_1B_DIGITAL_FILTER_SELECT1, 1, 3, 0),
    SOC_SINGLE("High Path Filter 2", AK4951_1C_DIGITAL_FILTER_SELECT2, 0, 1, 0),
    SOC_SINGLE("Low Path Filter", 	 AK4951_1C_DIGITAL_FILTER_SELECT2, 1, 1, 0),
	SOC_SINGLE("5 Band Equalizer 1", AK4951_30_DIGITAL_FILTER_SELECT3, 0, 1, 0),
	SOC_SINGLE("5 Band Equalizer 2", AK4951_30_DIGITAL_FILTER_SELECT3, 1, 1, 0),
	SOC_SINGLE("5 Band Equalizer 3", AK4951_30_DIGITAL_FILTER_SELECT3, 2, 1, 0),
	SOC_SINGLE("5 Band Equalizer 4", AK4951_30_DIGITAL_FILTER_SELECT3, 3, 1, 0),
	SOC_SINGLE("5 Band Equalizer 5", AK4951_30_DIGITAL_FILTER_SELECT3, 4, 1, 0),
	SOC_SINGLE("Auto Level Control 1", AK4951_0B_ALC_MODE_CONTROL1, 5, 1, 0),
	SOC_SINGLE("Soft Mute Control", AK4951_07_MODE_CONTROL3, 5, 1, 0),
	SOC_ENUM_EXT("Stereo Emphasis Filter Control", ak4951_stereo_enum[0], get_onstereo, set_onstereo),
	SOC_ENUM_EXT("Mic and Lin Switch",ak4951_micswitch_enum[0],get_micstatus,set_micstatus),
#ifdef AK4951_DEBUG
	SOC_ENUM_EXT("Reg Read", ak4951_enum[0], get_test_reg, set_test_reg),
#endif


};

static const char *ak4951_lin_select_texts[] =
		{"LIN1", "LIN2", "LIN3"};

static const struct soc_enum ak4951_lin_mux_enum =
	SOC_ENUM_SINGLE(AK4951_03_SIGNAL_SELECT2, 2,
			ARRAY_SIZE(ak4951_lin_select_texts), ak4951_lin_select_texts);

static const struct snd_kcontrol_new ak4951_lin_mux_control =
	SOC_DAPM_ENUM("LIN Select", ak4951_lin_mux_enum);

static const char *ak4951_rin_select_texts[] =
		{"RIN1", "RIN2", "RIN3"};

static const struct soc_enum ak4951_rin_mux_enum =
	SOC_ENUM_SINGLE(AK4951_03_SIGNAL_SELECT2, 0,
			ARRAY_SIZE(ak4951_rin_select_texts), ak4951_rin_select_texts);

static const struct snd_kcontrol_new ak4951_rin_mux_control =
	SOC_DAPM_ENUM("RIN Select", ak4951_rin_mux_enum);
//////////////////////////////////////////
static const char *ak4951_lin1_select_texts[] =
		{"LIN1", "Mic Bias"};

static const struct soc_enum ak4951_lin1_mux_enum =
	SOC_ENUM_SINGLE(0, 0,
			ARRAY_SIZE(ak4951_lin1_select_texts), ak4951_lin1_select_texts);

static const struct snd_kcontrol_new ak4951_lin1_mux_control =
	SOC_DAPM_ENUM_VIRT("LIN1 Switch", ak4951_lin1_mux_enum);
////////////////////////////////////////////

static const char *ak4951_micbias_select_texts[] =
		{"LIN1", "LIN2"};

static const struct soc_enum ak4951_micbias_mux_enum =
	SOC_ENUM_SINGLE(AK4951_02_SIGNAL_SELECT1, 4,
			ARRAY_SIZE(ak4951_micbias_select_texts), ak4951_micbias_select_texts);

static const struct snd_kcontrol_new ak4951_micbias_mux_control =
	SOC_DAPM_ENUM("MIC bias Select", ak4951_micbias_mux_enum);

static const char *ak4951_spklo_select_texts[] =
		{"Speaker", "Line"};

static const struct soc_enum ak4951_spklo_mux_enum =
	SOC_ENUM_SINGLE(AK4951_01_POWER_MANAGEMENT2, 0,
			ARRAY_SIZE(ak4951_spklo_select_texts), ak4951_spklo_select_texts);

static const struct snd_kcontrol_new ak4951_spklo_mux_control =
	SOC_DAPM_ENUM("SPKLO Select", ak4951_spklo_mux_enum);

static const char *ak4951_adcpf_select_texts[] =
		{"SDTI", "ADC"};

static const struct soc_enum ak4951_adcpf_mux_enum =
	SOC_ENUM_SINGLE(AK4951_1D_DIGITAL_FILTER_MODE, 1,
			ARRAY_SIZE(ak4951_adcpf_select_texts), ak4951_adcpf_select_texts);

static const struct snd_kcontrol_new ak4951_adcpf_mux_control =
	SOC_DAPM_ENUM("ADCPF Select", ak4951_adcpf_mux_enum);


static const char *ak4951_pfsdo_select_texts[] =
		{"ADC", "PFIL"};

static const struct soc_enum ak4951_pfsdo_mux_enum =
	SOC_ENUM_SINGLE(AK4951_1D_DIGITAL_FILTER_MODE, 0,
			ARRAY_SIZE(ak4951_pfsdo_select_texts), ak4951_pfsdo_select_texts);

static const struct snd_kcontrol_new ak4951_pfsdo_mux_control =
	SOC_DAPM_ENUM("PFSDO Select", ak4951_pfsdo_mux_enum);

static const char *ak4951_pfdac_select_texts[] =
		{"SDTI", "PFIL", "SPMIX"};

static const struct soc_enum ak4951_pfdac_mux_enum =
	SOC_ENUM_SINGLE(AK4951_1D_DIGITAL_FILTER_MODE, 2,
			ARRAY_SIZE(ak4951_pfdac_select_texts), ak4951_pfdac_select_texts);

static const struct snd_kcontrol_new ak4951_pfdac_mux_control =
	SOC_DAPM_ENUM("PFDAC Select", ak4951_pfdac_mux_enum);


static const char *ak4951_mic_select_texts[] =
		{"AMIC", "DMIC"};

static const struct soc_enum ak4951_mic_mux_enum =
	SOC_ENUM_SINGLE(AK4951_08_DIGITL_MIC, 0,
			ARRAY_SIZE(ak4951_mic_select_texts), ak4951_mic_select_texts);

static const struct snd_kcontrol_new ak4951_mic_mux_control =
	SOC_DAPM_ENUM("MIC Select", ak4951_mic_mux_enum);


static const struct snd_kcontrol_new ak4951_dacsl_mixer_controls[] = {
	SOC_DAPM_SINGLE("DACSL", AK4951_02_SIGNAL_SELECT1, 5, 1, 0),
};

static int ak4951_spklo_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) //CONFIG_LINF
{
	struct snd_soc_codec *codec = w->codec;
	u32 reg, nLOSEL;

	akdbgprt("\t[AK4951] %s(%d)\n",__FUNCTION__,__LINE__);

	reg = snd_soc_read(codec, AK4951_01_POWER_MANAGEMENT2);
	nLOSEL = (0x1 & reg);

	switch (event) {
		case SND_SOC_DAPM_PRE_PMU:	/* before widget power up */
			break;
		case SND_SOC_DAPM_POST_PMU:	/* after widget power up */
			if ( nLOSEL ) {
				akdbgprt("\t[AK4951] %s wait=300msec\n",__FUNCTION__);
				mdelay(300);
			}
			else {
				akdbgprt("\t[AK4951] %s wait=1msec\n",__FUNCTION__);
				mdelay(1);
			}
			snd_soc_update_bits(codec, AK4951_02_SIGNAL_SELECT1, 0x80,0x80);
			break;
		case SND_SOC_DAPM_PRE_PMD:	/* before widget power down */
			snd_soc_update_bits(codec, AK4951_02_SIGNAL_SELECT1, 0x80,0x00);
			mdelay(1);
			break;
		case SND_SOC_DAPM_POST_PMD:	/* after widget power down */
			if ( nLOSEL ) {
				akdbgprt("\t[AK4951] %s wait=300msec\n",__FUNCTION__);
				mdelay(300);
			}
			break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget ak4951_dapm_widgets[] = {

// ADC, DAC
	SND_SOC_DAPM_ADC("ADC Left", "NULL", AK4951_00_POWER_MANAGEMENT1, 0, 0),
	SND_SOC_DAPM_ADC("ADC Right", "NULL", AK4951_00_POWER_MANAGEMENT1, 1, 0),
	SND_SOC_DAPM_DAC("DAC", "NULL", AK4951_00_POWER_MANAGEMENT1, 2, 0),

#ifdef PLL_32BICK_MODE
	SND_SOC_DAPM_SUPPLY("PMPLL", AK4951_01_POWER_MANAGEMENT2, 2, 0, NULL, 0),
#else
#ifdef PLL_64BICK_MODE
	SND_SOC_DAPM_SUPPLY("PMPLL", AK4951_01_POWER_MANAGEMENT2, 2, 0, NULL, 0),
#endif
#endif

	SND_SOC_DAPM_ADC("PFIL", "NULL", AK4951_00_POWER_MANAGEMENT1, 7, 0),

	SND_SOC_DAPM_ADC("DMICL", "NULL", AK4951_08_DIGITL_MIC, 4, 0),
	SND_SOC_DAPM_ADC("DMICR", "NULL", AK4951_08_DIGITL_MIC, 5, 0),

	SND_SOC_DAPM_AIF_OUT("SDTO", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("SDTI", "Playback", 0, SND_SOC_NOPM, 0, 0),

// Analog Output
	SND_SOC_DAPM_OUTPUT("HPL"),
	SND_SOC_DAPM_OUTPUT("HPR"),
	SND_SOC_DAPM_OUTPUT("SPKLO"),

	SND_SOC_DAPM_PGA("HPL Amp", AK4951_01_POWER_MANAGEMENT2, 4, 0, NULL, 0),
	SND_SOC_DAPM_PGA("HPR Amp", AK4951_01_POWER_MANAGEMENT2, 5, 0, NULL, 0),

	SND_SOC_DAPM_PGA("SPK Amp", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Line Amp", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MIXER_E("SPKLO Mixer", AK4951_01_POWER_MANAGEMENT2, 1, 0,
			&ak4951_dacsl_mixer_controls[0], ARRAY_SIZE(ak4951_dacsl_mixer_controls),
			ak4951_spklo_event, (SND_SOC_DAPM_POST_PMU |SND_SOC_DAPM_PRE_PMD
                            |SND_SOC_DAPM_PRE_PMU |SND_SOC_DAPM_POST_PMD)),

// Analog Input
	SND_SOC_DAPM_INPUT("LIN1"),
	SND_SOC_DAPM_INPUT("RIN1"),
	SND_SOC_DAPM_INPUT("LIN2"),
	SND_SOC_DAPM_INPUT("RIN2"),
	SND_SOC_DAPM_INPUT("LIN3"),
	SND_SOC_DAPM_INPUT("RIN3"),

	SND_SOC_DAPM_MUX("LIN MUX", SND_SOC_NOPM, 0, 0,	&ak4951_lin_mux_control),
	SND_SOC_DAPM_MUX("RIN MUX", SND_SOC_NOPM, 0, 0,	&ak4951_rin_mux_control),

// MIC Bias
	SND_SOC_DAPM_MICBIAS("Mic Bias", AK4951_02_SIGNAL_SELECT1, 3, 0),
	SND_SOC_DAPM_VIRT_MUX("LIN1 MUX", SND_SOC_NOPM, 0, 0, &ak4951_lin1_mux_control),
	SND_SOC_DAPM_MUX("Mic Bias MUX", SND_SOC_NOPM, 0, 0, &ak4951_micbias_mux_control),
	SND_SOC_DAPM_MUX("SPKLO MUX", SND_SOC_NOPM, 0, 0, &ak4951_spklo_mux_control),


// PFIL
	SND_SOC_DAPM_MUX("PFIL MUX", SND_SOC_NOPM, 0, 0, &ak4951_adcpf_mux_control),
	SND_SOC_DAPM_MUX("PFSDO MUX", SND_SOC_NOPM, 0, 0, &ak4951_pfsdo_mux_control),
	SND_SOC_DAPM_MUX("PFDAC MUX", SND_SOC_NOPM, 0, 0, &ak4951_pfdac_mux_control),

// Digital Mic
	SND_SOC_DAPM_INPUT("DMICLIN"),
	SND_SOC_DAPM_INPUT("DMICRIN"),

	SND_SOC_DAPM_VIRT_MUX("MIC MUX", SND_SOC_NOPM, 0, 0, &ak4951_mic_mux_control),

};


static const struct snd_soc_dapm_route ak4951_intercon[] = {

#ifdef PLL_32BICK_MODE
	{"ADC Left", "NULL", "PMPLL"},
	{"ADC Right", "NULL", "PMPLL"},
	{"DAC", "NULL", "PMPLL"},
#else
#ifdef PLL_64BICK_MODE
	{"ADC Left", "NULL", "PMPLL"},
	{"ADC Right", "NULL", "PMPLL"},
	{"DAC", "NULL", "PMPLL"},
#endif
#endif

	{"Mic Bias MUX", "LIN1", "LIN1"},
	{"Mic Bias MUX", "LIN2", "LIN2"},

	{"Mic Bias", "NULL", "Mic Bias MUX"},

	{"LIN1 MUX", "LIN1", "LIN1"},
	{"LIN1 MUX", "Mic Bias", "Mic Bias"},

	{"LIN MUX", "LIN1", "LIN1 MUX"},
	{"LIN MUX", "LIN2", "Mic Bias"},
	{"LIN MUX", "LIN3", "LIN3"},
	{"RIN MUX", "RIN1", "RIN1"},
	{"RIN MUX", "RIN2", "RIN2"},
	{"RIN MUX", "RIN3", "RIN3"},
	{"ADC Left", "NULL", "LIN MUX"},
	{"ADC Right", "NULL", "RIN MUX"},

	{"DMICL", "NULL", "DMICLIN"},
	{"DMICR", "NULL", "DMICRIN"},

	{"MIC MUX", "AMIC", "ADC Left"},
	{"MIC MUX", "AMIC", "ADC Right"},
	{"MIC MUX", "DMIC", "DMICL"},
	{"MIC MUX", "DMIC", "DMICR"},

	{"PFIL MUX", "SDTI", "SDTI"},
	{"PFIL MUX", "ADC", "MIC MUX"},
	{"PFIL", "NULL", "PFIL MUX"},

	{"PFSDO MUX", "ADC", "MIC MUX"},
	{"PFSDO MUX", "PFIL", "PFIL"},

	{"SDTO", "NULL", "PFSDO MUX"},

	{"PFDAC MUX", "SDTI", "SDTI"},
	{"PFDAC MUX", "PFIL", "PFIL"},

//	{"DAC MUX", "PFDAC", "PFDAC MUX"},
	{"DAC", "NULL", "PFDAC MUX"},

//	{"DAC", "NULL", "DAC MUX"},

	{"HPL Amp", "NULL", "DAC"},
	{"HPR Amp", "NULL", "DAC"},
	{"HPL", "NULL", "HPL Amp"},
	{"HPR", "NULL", "HPR Amp"},

	{"SPKLO Mixer", "DACSL", "DAC"},
	{"SPK Amp", "NULL", "SPKLO Mixer"},
	{"Line Amp", "NULL", "SPKLO Mixer"},
	{"SPKLO MUX", "Speaker", "SPK Amp"},
	{"SPKLO MUX", "Line", "Line Amp"},
	{"SPKLO", "NULL", "SPKLO MUX"},

};

static int ak4951_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	int rate = params_rate(params);
	struct ak4951_priv *ak4951 = snd_soc_codec_get_drvdata(codec);
	int oversample = 0;
	u8 	fs = 0;
	akdbgprt("\t[AK4951] %s(%d)\n",__FUNCTION__,__LINE__);
	oversample = ak4951->sysclk / rate;
	switch (oversample) {
	case 256:
		fs &= (~AK4951_FS_CM1);
		fs &= (~AK4951_FS_CM0);
		break;
	case 384:
		fs &= (~AK4951_FS_CM1);
		fs |= AK4951_FS_CM0;
		break;
	case 512:
		fs |= AK4951_FS_CM1;
		fs &= (~AK4951_FS_CM0);
		break;
	case 1024:
		fs |= AK4951_FS_CM1;
		fs |= AK4951_FS_CM0;
		break;
	default:
		break;
	}

	if(ak4951->clkid == AK4951_BCLK_IN) {
		switch (rate) {
			case 8000:
				fs |= AK4951_BICK_FS_8KHZ;
				break;
			case 11025:
				fs |= AK4951_BICK_FS_11_025KHZ;
				break;
			case 12000:
				fs |= AK4951_BICK_FS_12KHZ;
				break;
			case 16000:
				fs |= AK4951_BICK_FS_16KHZ;
				break;
			case 22050:
				fs |= AK4951_BICK_FS_22_05KHZ;
				break;
			case 24000:
				fs |= AK4951_BICK_FS_24KHZ;
				break;
			case 32000:
				fs |= AK4951_BICK_FS_32KHZ;
				break;
			case 44100:
				fs |= AK4951_BICK_FS_44_1KHZ;
				break;
			case 48000:
				fs |= AK4951_BICK_FS_48KHZ;
				break;

			default:
				return -EINVAL;
		}

	} else {
		switch (rate) {
			case 8000:
				fs |= AK4951_MCKI_FS_8KHZ;
				break;
			case 11025:
				fs |= AK4951_MCKI_FS_11_025KHZ;
				break;
			case 12000:
				fs |= AK4951_MCKI_FS_12KHZ;
				break;
			case 16000:
				fs |= AK4951_MCKI_FS_16KHZ;
				break;
			case 22050:
				fs |= AK4951_MCKI_FS_22_05KHZ;
				break;
			case 24000:
				fs |= AK4951_MCKI_FS_24KHZ;
				break;
			case 32000:
				fs |= AK4951_MCKI_FS_32KHZ;
				break;
			case 44100:
				fs |= AK4951_MCKI_FS_44_1KHZ;
				break;
			case 48000:
				fs |= AK4951_MCKI_FS_48KHZ;
				break;

			default:
				return -EINVAL;
		}
	}

	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			ak4951->fmt = 2 << 4;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			ak4951->fmt = 3 << 4;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			ak4951->fmt = 3 << 4;
			break;
		default:
			dev_err(codec->dev, "Can not support the format");
			return -EINVAL;
	}
	snd_soc_write(codec, AK4951_06_MODE_CONTROL2, fs);

	return 0;
}

static int ak4951_set_pll(u8 *pll, int clk_id,int freq)
{
	if (clk_id == AK4951_MCLK_IN_BCLK_OUT){
		switch (freq) {
		case 11289600:
			*pll |= (4 << 4);
			break;
		case 12288000:
			*pll |= (5 << 4);
			break;
		case 12000000:
			*pll |= (6 << 4);
			break;
		case 24000000:
			*pll |= (7 << 4);
			break;
		case 13500000:
			*pll |= (0xC << 4);
			break;
		case 27000000:
			*pll |= (0xD << 4);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int ak4951_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct ak4951_priv *ak4951 = snd_soc_codec_get_drvdata(codec);
	u8 pllpwr = 0, pll = 0;

	akdbgprt("\t[AK4951] %s(%d),CLK id is %d\n",__FUNCTION__,__LINE__, clk_id);

	pll= snd_soc_read(codec, AK4951_05_MODE_CONTROL1);
	akdbgprt("\t[AK4951] pll valuse is 0x%x\n",pll);
	pll &=(~0xF0);
	pllpwr = snd_soc_read(codec,AK4951_01_POWER_MANAGEMENT2);
	akdbgprt("\t[AK4951] pllpwr valuse is 0x%x\n",pllpwr);
	pllpwr &=(~0x0c);

	if (clk_id == AK4951_MCLK_IN) {
		pll |= AK4951_PLL_12_288MHZ;
		pllpwr &= (~AK4951_PMPLL);
		pllpwr &= (~AK4951_M_S);
	}else if (clk_id == AK4951_BCLK_IN) {
		pllpwr |= AK4951_PMPLL;
		pllpwr &= (~AK4951_M_S);
		pll |= ak4951->fmt;
	}else if (clk_id == AK4951_MCLK_IN_BCLK_OUT) {
		pllpwr |= AK4951_PMPLL;
		pllpwr |= AK4951_M_S;
		ak4951_set_pll(&pll, clk_id, freq);
	}
	snd_soc_write(codec, AK4951_05_MODE_CONTROL1, pll);
	snd_soc_write(codec, AK4951_01_POWER_MANAGEMENT2, pllpwr);
	msleep(5); //AKM suggested

	ak4951->sysclk = freq;
	ak4951->clkid = clk_id;
	return 0;
}

static int ak4951_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{

	struct snd_soc_codec *codec = dai->codec;
	u8 mode;
	u8 format;

	akdbgprt("\t[AK4951] %s(%d)\n",__FUNCTION__,__LINE__);

	/* set master/slave audio interface */
	mode = snd_soc_read(codec, AK4951_01_POWER_MANAGEMENT2);
	format = snd_soc_read(codec, AK4951_05_MODE_CONTROL1);
	format &= ~AK4951_DIF;

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
        case SND_SOC_DAIFMT_CBS_CFS:
			akdbgprt("\t[AK4951] %s(Slave)\n",__FUNCTION__);
            mode &= ~(AK4951_M_S);
            //format &= ~(AK4951_BCKO);
            break;
        case SND_SOC_DAIFMT_CBM_CFM:
			akdbgprt("\t[AK4951] %s(Master)\n",__FUNCTION__);
            mode |= AK4951_M_S;
            //format |= (AK4951_BCKO);
            break;
        case SND_SOC_DAIFMT_CBS_CFM:
        case SND_SOC_DAIFMT_CBM_CFS:
        default:
            dev_err(codec->dev, "Clock mode unsupported");
           return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:
			format |= AK4951_DIF_I2S_MODE;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			format |= AK4951_DIF_24MSB_MODE;
			break;
		default:
			return -EINVAL;
	}

	/* set mode and format */

	snd_soc_write(codec, AK4951_01_POWER_MANAGEMENT2, mode);
	snd_soc_write(codec, AK4951_05_MODE_CONTROL1, format);

	return 0;
}

/*
 * Write with Mask to  AK4951 register space
 */
static int ak4951_writeMask(
struct snd_soc_codec *codec,
u16 reg,
u16 mask,
u16 value)
{
    u16 olddata;
    u16 newdata;

	if ( (mask == 0) || (mask == 0xFF) ) {
		newdata = value;
	}
	else {
		olddata = snd_soc_read(codec, reg);
	    newdata = (olddata & ~(mask)) | value;
	}

	snd_soc_write(codec, (unsigned int)reg, (unsigned int)newdata);

	akdbgprt("\t[ak4951_writeMask] %s(%d): (addr,data)=(%x, %x)\n",__FUNCTION__,__LINE__, reg, newdata);

    return(0);
}

// * for AK4951
static int ak4951_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *codec_dai)
{
	int 	ret = 0;
 //   struct snd_soc_codec *codec = codec_dai->codec;

	akdbgprt("\t[AK4951] %s(%d)\n",__FUNCTION__,__LINE__);

	return ret;
}


static int ak4951_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	u8 reg;

	akdbgprt("\t[AK4951] %s(%d)\n",__FUNCTION__,__LINE__);

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		reg = snd_soc_read(codec, AK4951_00_POWER_MANAGEMENT1);	// * for AK4951
		snd_soc_write(codec, AK4951_00_POWER_MANAGEMENT1,			// * for AK4951
				reg | AK4951_PMVCM | AK4951_PMPFIL);
		msleep(250);	//AKM suggested
		break;
	case SND_SOC_BIAS_OFF:
		snd_soc_write(codec, AK4951_00_POWER_MANAGEMENT1, 0x00);	// * for AK4951
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

#define AK4951_RATES		(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
				SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
				SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
				SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |\
				SNDRV_PCM_RATE_96000)

#define AK4951_FORMATS		(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops ak4951_dai_ops = {
	.hw_params	= ak4951_hw_params,
	.set_sysclk	= ak4951_set_dai_sysclk,
	.set_fmt	= ak4951_set_dai_fmt,
	.trigger = ak4951_trigger,
};

struct snd_soc_dai_driver ak4951_dai[] = {
	{
		.name = "ak4951-hifi",
		.playback = {
		       .stream_name = "Playback",
		       .channels_min = 1,
		       .channels_max = 2,
		       .rates = AK4951_RATES,
		       .formats = AK4951_FORMATS,
		},
		.capture = {
		       .stream_name = "Capture",
		       .channels_min = 1,
		       .channels_max = 2,
		       .rates = AK4951_RATES,
		       .formats = AK4951_FORMATS,
		},
		.ops = &ak4951_dai_ops,
	},
};

static int ak4951_probe(struct snd_soc_codec *codec)
{
	struct ak4951_priv *ak4951 = ak4951_data;
	int ret = 0;

	akdbgprt("\t[AK4951] %s(%d)\n",__FUNCTION__,__LINE__);
	codec->control_data = ak4951->regmap;
	snd_soc_codec_set_drvdata(codec, ak4951);
	ret = snd_soc_codec_set_cache_io(codec, 8, 8, SND_SOC_REGMAP);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	akdbgprt("\t[AK4951] %s(%d) ak4951=%x\n",__FUNCTION__,__LINE__, (int)ak4951);

	ret = devm_gpio_request(codec->dev, ak4951->rst_pin, "ak4951 reset");
	if (ret < 0){
		dev_err(codec->dev, "Failed to request rst_pin: %d\n", ret);
		return ret;
	}

	/* Reset AK4951 codec */
	gpio_direction_output(ak4951->rst_pin, ak4951->rst_active);
	msleep(1);
	gpio_direction_output(ak4951->rst_pin, !ak4951->rst_active);

	/*The 0x00 register no Ack for the dummy command:write 0x00 to 0x00*/
	ak4951->i2c_clt->flags |= I2C_M_IGNORE_NAK;
	i2c_smbus_write_byte_data(ak4951->i2c_clt, (u8)(AK4951_00_POWER_MANAGEMENT1 & 0xFF), 0x00);
	ak4951->i2c_clt->flags &= ~I2C_M_IGNORE_NAK;

	ak4951_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	akdbgprt("\t[AK4951 bias] %s(%d)\n",__FUNCTION__,__LINE__);

	ak4951->onStereo = 0;
	ak4951->mic = 1;
	/*Enable Line out */
	snd_soc_update_bits(codec,AK4951_01_POWER_MANAGEMENT2,0x02,0x02);
	snd_soc_update_bits(codec,AK4951_02_SIGNAL_SELECT1, 0xa0,0xa0);//0x10100110

	/*Enable LIN2*/
	snd_soc_update_bits(codec,AK4951_02_SIGNAL_SELECT1,0x18,0x08);//MPWR1
	snd_soc_update_bits(codec,AK4951_03_SIGNAL_SELECT2,0x0f,0x05);// LIN2 RIN2
	snd_soc_update_bits(codec,AK4951_08_DIGITL_MIC,0x01,0x00);//AMIC
	snd_soc_update_bits(codec,AK4951_1D_DIGITAL_FILTER_MODE,0x02,0x02);//ADC output
	snd_soc_update_bits(codec,AK4951_1D_DIGITAL_FILTER_MODE,0x01,0x01);//ALC output
	snd_soc_update_bits(codec,AK4951_02_SIGNAL_SELECT1,0x47,0x00);//Mic Gain 0x10100110
	snd_soc_update_bits(codec,AK4951_0D_LCH_INPUT_VOLUME_CONTROL,0xff,0xb0);//Lch gain
	snd_soc_update_bits(codec,AK4951_0E_RCH_INPUT_VOLUME_CONTROL,0xff,0xb0);//Lch gain
	snd_soc_write(codec, AK4951_0B_ALC_MODE_CONTROL1, 0x20);	//enable ALC
	snd_soc_write(codec, AK4951_1B_DIGITAL_FILTER_SELECT1, 0x07); //enable HPF1
	snd_soc_write(codec, AK4951_0C_ALC_MODE_CONTROL2, 0xF1);
	/*Enable LIN3*/
	//snd_soc_update_bits(codec,AK4951_03_SIGNAL_SELECT2,0x0f,0x0a);// LIN3 RIN3
    return ret;

}

static int ak4951_remove(struct snd_soc_codec *codec)
{

	akdbgprt("\t[AK4951] %s(%d)\n",__FUNCTION__,__LINE__);

	ak4951_set_bias_level(codec, SND_SOC_BIAS_OFF);


	return 0;
}

static int ak4951_suspend(struct snd_soc_codec *codec)
{
	struct ak4951_priv *ak4951 = snd_soc_codec_get_drvdata(codec);
	int i;

	for(i = 0; i < 7; i++) {
		ak4951->reg_cache[i] = snd_soc_read(codec, i);
	}

	ak4951_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int ak4951_resume(struct snd_soc_codec *codec)
{
	struct ak4951_priv *ak4951 = snd_soc_codec_get_drvdata(codec);
	int i;

	for(i = 0; i < 7; i++) {
		snd_soc_write(codec, i, ak4951->reg_cache[i]);
	}

	ak4951_set_bias_level(codec, codec->dapm.bias_level);

	return 0;
}


struct snd_soc_codec_driver soc_codec_dev_ak4951 = {
	.probe = ak4951_probe,
	.remove = ak4951_remove,
	.suspend =	ak4951_suspend,
	.resume =	ak4951_resume,

	.set_bias_level = ak4951_set_bias_level,

	.controls = ak4951_snd_controls,
	.num_controls = ARRAY_SIZE(ak4951_snd_controls),
	.dapm_widgets = ak4951_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ak4951_dapm_widgets),
	.dapm_routes = ak4951_intercon,
	.num_dapm_routes = ARRAY_SIZE(ak4951_intercon),
};
EXPORT_SYMBOL_GPL(soc_codec_dev_ak4951);

static struct regmap_config ak4951_regmap = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = AK4951_MAX_REGISTERS,
	.reg_defaults = ak4951_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(ak4951_reg_defaults),
	.volatile_reg = ak4951_volatile_register,
	.cache_type = REGCACHE_RBTREE,
};

static int ak4951_i2c_probe(struct i2c_client *i2c,
                            const struct i2c_device_id *id)
{
	struct device_node *np = i2c->dev.of_node;
	struct ak4951_priv *ak4951;
	enum of_gpio_flags flags;
	int rst_pin;
	int ret = 0;

	akdbgprt("\t[AK4951] %s(%d)\n",__FUNCTION__,__LINE__);

	ak4951 = devm_kzalloc(&i2c->dev, sizeof(struct ak4951_priv), GFP_KERNEL);
	if (ak4951 == NULL)
		return -ENOMEM;

	rst_pin = of_get_gpio_flags(np, 0, &flags);
	if (rst_pin < 0 || !gpio_is_valid(rst_pin))
		return -ENXIO;

	ak4951->i2c_clt = i2c;
	ak4951->rst_pin = rst_pin;
	ak4951->rst_active = !!(flags & OF_GPIO_ACTIVE_LOW);
	i2c_set_clientdata(i2c, ak4951);
	ak4951->regmap = devm_regmap_init_i2c(i2c, &ak4951_regmap);
	if (IS_ERR(ak4951->regmap)) {
		ret = PTR_ERR(ak4951->regmap);
		dev_err(&i2c->dev, "regmap_init() failed: %d\n", ret);
		return ret;
	}

	ak4951_data = ak4951;

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_ak4951, &ak4951_dai[0], ARRAY_SIZE(ak4951_dai));
	if (ret < 0){
		kfree(ak4951);
		akdbgprt("\t[AK4951 Error!] %s(%d)\n",__FUNCTION__,__LINE__);
	}
	return ret;
}

static int ak4951_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static struct of_device_id ak4951_of_match[] = {
	{ .compatible = "ambarella,ak4951",},
	{},
};
MODULE_DEVICE_TABLE(of, ak4951_of_match);

static const struct i2c_device_id ak4951_i2c_id[] = {
	{ "ak4951", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ak4951_i2c_id);

static struct i2c_driver ak4951_i2c_driver = {
	.driver = {
		.name = "ak4951-codec",
		.owner = THIS_MODULE,
		.of_match_table = ak4951_of_match,
	},
	.probe		=	ak4951_i2c_probe,
	.remove		=	ak4951_i2c_remove,
	.id_table	=	ak4951_i2c_id,
};

static int __init ak4951_modinit(void)
{
	int ret;
	akdbgprt("\t[AK4951] %s(%d)\n", __FUNCTION__,__LINE__);

	ret = i2c_add_driver(&ak4951_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register ak4951 I2C driver: %d\n",ret);
	return ret;
}

module_init(ak4951_modinit);

static void __exit ak4951_exit(void)
{
	i2c_del_driver(&ak4951_i2c_driver);
}
module_exit(ak4951_exit);

MODULE_DESCRIPTION("Soc AK4951 driver");
MODULE_AUTHOR("Diao Chengdong<cddiao@ambarella.com>");
MODULE_LICENSE("GPL");

