/*
 * ak7755.c  --  audio driver for AK7755
 *
 * Copyright (C) 2014 Asahi Kasei Microdevices Corporation
 *  Author                Date        Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      14/04/22	    1.0
 *                      15/06/15	    1.1
 *                      16/01/13	    2.01  kernel 3.10.94
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>
#include <plat/spi.h>


#include <linux/of_gpio.h> // '16/01/13

#include <linux/mutex.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>

#include <sound/ak7755_pdata.h>    // '15/06/15
#include "ak7755.h"
#include "ak7755_dsp_code.h"
#include <linux/types.h>

//#define AK7755_PD_SUSPEND     // '15/06/15

//#define AK7755_DEBUG			//used at debug mode
#define AK7755_CONTIF_DEBUG	//used at debug mode

#ifdef AK7755_DEBUG
#define akdbgprt printk
#else
#define akdbgprt(format, arg...) do {} while (0)
#endif
static int fast_boot = 0;
module_param(fast_boot, uint, 0664);
static int aec = 1;
module_param(aec, uint, 0664);
static int RAM_FLAG = 1;

/* AK7755 Codec Private Data */
struct ak7755_priv {
	enum snd_soc_control_type control_type;
	struct snd_soc_codec *codec;
	struct spi_device *spi;
	struct i2c_client *i2c;
	int fs;
	int rclk;			//Master Clock
	int lign;			//LIGN3-0(LineIn Volume)
	int selmix;			//SELMIX2-0
	int status;
	int MIRNo;
	int bickfs;         // '15/06/15  0:64fs, 1:48fs, 2:32fs, 3:256fs
	int pdn_gpio;       // '15/06/15
	unsigned int pdn_active;
	int power_gpio;
	unsigned int power_active;
	int amp_gpio;
	unsigned int amp_active;
	unsigned int fmt;
	u8 reg_cache[AK7755_MAX_REGNUM];

	u16  DSPPramMode;
	u16  DSPCramMode;
	u16  DSPOfregMode;
	u16  DSPAcramMode;
	int  EQLevel[5];
	int  HPFfc[2];
	int  LimRel;
	int  LimVol;
};

struct _ak7755_pd_handler {
	int ref_count;
	struct mutex lock;
	struct ak7755_priv *data;
} ak7755_pd_handler = {
	.ref_count = -1,
	.data = NULL,
};


static int ak7755_reads(u8 *, size_t, u8 *, size_t);
static int ak7755_writes(const u8 *tx, size_t wlen);
static unsigned int ak7755_i2c_read(u8 *reg,int reglen,u8 *data,int datalen);


static struct ak7755_priv *ak7755_data;
/* ak7755 register cache & default register settings */
static u8 ak7755_reg[AK7755_MAX_REGISTERS];

static const u8 ak7755_reg2[] = {
	0x00,	/*	0xC0	AK7755_C0_CLOCK_SETTING1			*/
	0x00,	/*	0xC1	AK7755_C1_CLOCK_SETTING2			*/
	0x00,	/*	0xC2	AK7755_C2_SERIAL_DATA_FORMAT		*/
	0x00,	/*	0xC3	AK7755_C3_DELAY_RAM_DSP_IO			*/
	0x00,	/*	0xC4	AK7755_C4_DATARAM_CRAM_SETTING		*/
	0x00,	/*	0xC5	AK7755_C5_ACCELARETOR_SETTING		*/
	0x00,	/*	0xC6	AK7755_C6_DAC_DEM_SETTING			*/
	0x00,	/*	0xC7	AK7755_C7_DSP_OUT_SETTING			*/
	0x00,	/*	0xC8	AK7755_C8_DAC_IN_SETTING			*/
	0x00,	/*	0xC9	AK7755_C9_ANALOG_IO_SETTING			*/
	0x00,	/*	0xCA	AK7755_CA_CLK_SDOUT_SETTING			*/
	0x00,	/*	0xCB	AK7755_CB_TEST_SETTING				*/
	0x00,	/*	0xCC	AK7755_CC_VOLUME_TRANSITION			*/
	0x80,	/*	0xCD	AK7755_CD_STO_DLS_SETTING			*/
	0x00,	/*	0xCE	AK7755_CE_POWER_MANAGEMENT			*/
	0x00,	/*	0xCF	AK7755_CF_RESET_POWER_SETTING		*/
	0x00,	/*	0xD0	AK7755_D0_FUNCTION_SETTING			*/
	0x00,	/*	0xD1	AK7755_D1_DSPMCLK_SETTING			*/
	0x00,	/*	0xD2	AK7755_D2_MIC_GAIN_SETTING			*/
	0x00,	/*	0xD3	AK7755_D3_LIN_LO3_VOLUME_SETTING	*/
	0x00,	/*	0xD4	AK7755_D4_LO1_LO2_VOLUME_SETTING	*/
	0x30,	/*	0xD5	AK7755_D5_ADC_DVOLUME_SETTING1		*/
	0x30,	/*	0xD6	AK7755_D6_ADC_DVOLUME_SETTING2		*/
	0x30,	/*	0xD7	AK7755_D7_ADC2_DVOLUME_SETTING1		*/
	0x18,	/*	0xD8	AK7755_D8_DAC_DVOLUME_SETTING1		*/
	0x18,	/*	0xD9	AK7755_D9_DAC_DVOLUME_SETTING2		*/
	0x00,	/*	0xDA	AK7755_DA_MUTE_ADRC_ZEROCROSS_SET	*/
	0x00,	/*	0xDB	AK7755_DB_ADRC_MIC_GAIN_READ		*/
	0x00,	/*	0xDC	AK7755_DC_TEST_SETTING				*/
	0x30,	/*	0xDD	AK7755_DD_ADC2_DVOLUME_SETTING2		*/
	0x00,	/*	0xDE	AK7755_DE_DMIC_IF_SETTING			*/
};

static struct {
	int readable;   /* Mask of readable bits */
	int writable;   /* Mask of writable bits */
} ak7755_access_masks[AK7755_MAX_REGISTERS];

static const struct {
	int readable;   /* Mask of readable bits */
	int writable;   /* Mask of writable bits */
} ak7755_access_masks2[] = {
    { 0xFF, 0x7F },	//0xC0
    { 0xFF, 0xFF },	//0xC1
    { 0xFF, 0xFF },	//0xC2
    { 0xFF, 0xFF },	//0xC3
    { 0xFF, 0xFB },	//0xC4
    { 0xFF, 0xFF },	//0xC5
    { 0xFF, 0xF7 },	//0xC6
    { 0xFF, 0xF7 },	//0xC7
    { 0xFF, 0xFF },	//0xC8
    { 0xFF, 0xFF },	//0xC9
    { 0xFF, 0xE7 },	//0xCA
    { 0xFF, 0x00 },	//0xCB
    { 0xFF, 0xB7 },	//0xCC
    { 0xFF, 0x81 },	//0xCD
    { 0xFF, 0xFF },	//0xCE
    { 0xFF, 0x3F },	//0xCF
    { 0xFF, 0xF9 },	//0xD0
    { 0xFF, 0xFF },	//0xD1
    { 0xFF, 0xFF },	//0xD2
    { 0xFF, 0xFF },	//0xD3
    { 0xFF, 0xFF },	//0xD4
    { 0xFF, 0xFF },	//0xD5
    { 0xFF, 0xFF },	//0xD6
    { 0xFF, 0xFF },	//0xD7
    { 0xFF, 0xFF },	//0xD8
    { 0xFF, 0xFF },	//0xD9
    { 0xFF, 0xFF },	//0xDA
    { 0xFF, 0x00 },	//0xDB
    { 0xFF, 0x00 },	//0xDC
    { 0xFF, 0xFF },	//0xDD
    { 0xFF, 0xFC },	//0xDE
};

/* MIC Input Volume control:
 * from 0 to 36 dB (quantity of each step is various) */
static DECLARE_TLV_DB_MINMAX(mgnl_tlv, 0, 3600);
static DECLARE_TLV_DB_MINMAX(mgnr_tlv, 0, 3600);

/* Line-out Volume control:
 * from -30 to 0 dB in 2 dB steps (mute instead of -30 dB) */
static DECLARE_TLV_DB_SCALE(lovol1_tlv, -3000, 200, 0);
static DECLARE_TLV_DB_SCALE(lovol2_tlv, -3000, 200, 0);
static DECLARE_TLV_DB_SCALE(lovol3_tlv, -3000, 200, 0);

static const char *line_in_texts[]  =
{
	"-21dB","-18dB","-15dB","-12dB","-9dB","-6dB","-3dB",
	"0dB","+3dB","+6dB","+9dB","+12dB","+15dB","+18dB","+21dB",
};

static const struct soc_enum ak7755_linein_enum[] = {
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(line_in_texts), line_in_texts),
};

/* ADC, ADC2 Digital Volume control:
 * from -103.5 to 24 dB in 0.5 dB steps (mute instead of -103.5 dB) */
static DECLARE_TLV_DB_SCALE(voladl_tlv, -10350, 50, 0);
static DECLARE_TLV_DB_SCALE(voladr_tlv, -10350, 50, 0);
static DECLARE_TLV_DB_SCALE(volad2l_tlv, -10350, 50, 0);
static DECLARE_TLV_DB_SCALE(volad2r_tlv, -10350, 50, 0);

/* DAC Digital Volume control:
 * from -115.5 to 12 dB in 0.5 dB steps (mute instead of -115.5 dB) */
static DECLARE_TLV_DB_SCALE(voldal_tlv, -11550, 50, 0);
static DECLARE_TLV_DB_SCALE(voldar_tlv, -11550, 50, 0);


static const char *ak7755_bank_select_texts[] =
		{"0:8192", "1024:7168","2048:6144","3072:5120","4096:4096",
			"5120:3072","6144:2048","7168:1024","8192:0"};
static const char *ak7755_drms_select_texts[] =
		{"512:1536", "1024:1024", "1536:512"};
static const char *ak7755_dram_select_texts[] =
		{"Ring:Ring", "Ring:Linear", "Linear:Ring", "Linear:Linear"};
static const char *ak7755_pomode_select_texts[] = {"DBUS Immediate", "OFREG"};
static const char *ak7755_wavp_select_texts[] =
		{"33 word", "65 word", "129 word", "257 word"};
static const char *ak7755_filmode1_select_texts[] = {"Adaptive Filter", "FIR Filter"};
static const char *ak7755_filmode2_select_texts[] = {"Adaptive Filter", "FIR Filter"};
static const char *ak7755_submode1_select_texts[] = {"Fullband", "Subband"};
static const char *ak7755_submode2_select_texts[] = {"Fullband", "Subband"};
static const char *ak7755_memdiv_select_texts[] =
		{"2048:-", "1792:256", "1536:512", "1024:1024"};
static const char *ak7755_dem_select_texts[] = {"Off", "48kHz", "44.1kHz", "32kHz"};
static const char *ak7755_clkoe_select_texts[] = {"CLKO=L", "CLKO Out Enable"};
static const char *ak7755_clks_select_texts[] =
		{"12.288MHz", "6.144MHz", "3.072MHz", "8.192MHz",
			"4.096MHz", "2.048MHz", "256fs", "XTI or BICK"};

static const struct soc_enum ak7755_set_enum[] = {
	SOC_ENUM_SINGLE(AK7755_C3_DELAY_RAM_DSP_IO, 0,
			ARRAY_SIZE(ak7755_bank_select_texts), ak7755_bank_select_texts),
	SOC_ENUM_SINGLE(AK7755_C4_DATARAM_CRAM_SETTING, 6,
			ARRAY_SIZE(ak7755_drms_select_texts), ak7755_drms_select_texts),
	SOC_ENUM_SINGLE(AK7755_C4_DATARAM_CRAM_SETTING, 4,
			ARRAY_SIZE(ak7755_dram_select_texts), ak7755_dram_select_texts),
	SOC_ENUM_SINGLE(AK7755_C4_DATARAM_CRAM_SETTING, 3,
			ARRAY_SIZE(ak7755_pomode_select_texts), ak7755_pomode_select_texts),
	SOC_ENUM_SINGLE(AK7755_C4_DATARAM_CRAM_SETTING, 0,
			ARRAY_SIZE(ak7755_wavp_select_texts), ak7755_wavp_select_texts),
	SOC_ENUM_SINGLE(AK7755_C5_ACCELARETOR_SETTING, 5,
			ARRAY_SIZE(ak7755_filmode1_select_texts), ak7755_filmode1_select_texts),
	SOC_ENUM_SINGLE(AK7755_C5_ACCELARETOR_SETTING, 4,
			ARRAY_SIZE(ak7755_filmode2_select_texts), ak7755_filmode2_select_texts),
	SOC_ENUM_SINGLE(AK7755_C5_ACCELARETOR_SETTING, 3,
			ARRAY_SIZE(ak7755_submode1_select_texts), ak7755_submode1_select_texts),
	SOC_ENUM_SINGLE(AK7755_C5_ACCELARETOR_SETTING, 2,
			ARRAY_SIZE(ak7755_submode2_select_texts), ak7755_submode2_select_texts),
	SOC_ENUM_SINGLE(AK7755_C5_ACCELARETOR_SETTING, 0,
			ARRAY_SIZE(ak7755_memdiv_select_texts), ak7755_memdiv_select_texts),
	SOC_ENUM_SINGLE(AK7755_C6_DAC_DEM_SETTING, 6,
			ARRAY_SIZE(ak7755_dem_select_texts), ak7755_dem_select_texts),
	SOC_ENUM_SINGLE(AK7755_CA_CLK_SDOUT_SETTING, 7,
			ARRAY_SIZE(ak7755_clkoe_select_texts), ak7755_clkoe_select_texts),
	SOC_ENUM_SINGLE(AK7755_C1_CLOCK_SETTING2, 1,
			ARRAY_SIZE(ak7755_clks_select_texts), ak7755_clks_select_texts),
};

// '15/06/15
static const char *ak7755_bick_select_texts[] =
		{"64fs", "48fs", "32fs", "TDM" };

static const struct soc_enum ak7755_bick_enum[] = {
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ak7755_bick_select_texts), ak7755_bick_select_texts)
};

static int get_bickfs(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);

    ucontrol->value.enumerated.item[0] = ak7755->bickfs;

    return 0;
}

static int set_bickfs(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);

	ak7755->bickfs = ucontrol->value.enumerated.item[0];
    return 0;
}
// '15/06/15

static const char *selmix_set_texts[]  =
{
	"SDOUTAD", "ADL_AD2L/ADR", "ADL/ADR_AD2R", "SDOUTAD2",
	"DSPL/AD2R", "AD2L/DSPR", "DSPL/ADR", "ADL/DSPR",
};

static const struct soc_enum ak7755_selmix_enum[] = {
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(selmix_set_texts), selmix_set_texts),
};

// Added for AK7755
static const struct soc_enum ak7755_firmware_enum[] =
{
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ak7755_firmware_pram), ak7755_firmware_pram),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ak7755_firmware_cram), ak7755_firmware_cram),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ak7755_firmware_ofreg), ak7755_firmware_ofreg),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ak7755_firmware_acram), ak7755_firmware_acram),
};

static inline u32 ak7755_read_reg_cache(struct snd_soc_codec *, u16);
static int ak7755_firmware_write_ram(u16 mode, u16 cmd);
static int ak7755_set_status(enum ak7755_status status);
static int ak7755_write_cram(int addr, int len, unsigned char *cram_data);

static int get_linein(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);

    ucontrol->value.enumerated.item[0] = ak7755->lign;

    return 0;
}

static int set_linein(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);

	ak7755->lign = ucontrol->value.enumerated.item[0];

	if ( ak7755->lign <= 7) {
		snd_soc_update_bits(codec, AK7755_D3_LIN_LO3_VOLUME_SETTING, 0xF0, ((7 - ak7755->lign) << 4) );
	}
	else {
		snd_soc_update_bits(codec, AK7755_D3_LIN_LO3_VOLUME_SETTING, 0xF0, ((ak7755->lign + 1) << 4) );
	}

    return 0;
}

static int get_selmix(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);

    ucontrol->value.enumerated.item[0] = ak7755->selmix;

    return 0;
}

static int set_selmix(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);

	ak7755->selmix = ucontrol->value.enumerated.item[0];

	if ( ak7755->selmix < 4) {	//SELMIX2=0, SELMIX1-0=selmix
		snd_soc_update_bits(codec, AK7755_C9_ANALOG_IO_SETTING, 0x01, 0x00);
		snd_soc_update_bits(codec, AK7755_C8_DAC_IN_SETTING, 0x03, ak7755->selmix);
	}
	else {						//SELMIX2=1, SELMIX1-0=(selmix & 0x03)
		snd_soc_update_bits(codec, AK7755_C9_ANALOG_IO_SETTING, 0x01, 0x01);
		snd_soc_update_bits(codec, AK7755_C8_DAC_IN_SETTING, 0x03, (ak7755->selmix & 0x03));
	}

    return 0;
}

static int get_DSP_write_pram(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    /* Get the current output routing */
    ucontrol->value.enumerated.item[0] = ak7755_data->DSPPramMode;

    return 0;

}

static int set_DSP_write_pram(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    u32    currMode = ucontrol->value.enumerated.item[0];
	int    ret;
	akdbgprt("set_DSP_write_pram start ...\n");

/*
	if ( currMode == ak7755_data->DSPPramMode ) {
		akdbgprt("\t%s Same PRAM mode =%d\n",__FUNCTION__, currMode);
		return(0);
	}
*/
	if (RAM_FLAG ==1 ){
		akdbgprt("\t%s PRAM mode =%d\n",__FUNCTION__, currMode);

		ret = ak7755_firmware_write_ram(RAMTYPE_PRAM, currMode);
		if ( ret != 0 ) return(-1);

		ak7755_data->DSPPramMode = currMode;
	}
	akdbgprt("set_DSP_write_pram done ...\n");
	return(0);
}

static int get_DSP_write_cram(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    /* Get the current output routing */
    ucontrol->value.enumerated.item[0] = ak7755_data->DSPCramMode;

    return 0;

}

static int set_DSP_write_cram(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    u32    currMode = ucontrol->value.enumerated.item[0];
	int    ret;
	akdbgprt("set_DSP_write_cram start ...\n");

/*
	if ( currMode == ak7755_data->DSPCramMode ) {
		akdbgprt("\t%s Same CRAM mode =%d\n",__FUNCTION__, currMode);
		return(0);
	}
*/
	if (RAM_FLAG ==1){
		RAM_FLAG = 0 ;
		akdbgprt("\t%s CRAM mode =%d\n",__FUNCTION__, currMode);

		ret = ak7755_firmware_write_ram(RAMTYPE_CRAM, currMode);
		if ( ret != 0 ) return(-1);

		ak7755_data->DSPCramMode =currMode;
	}
	akdbgprt("set_DSP_write_cram done ...\n");
	return(0);
}

static int get_DSP_write_ofreg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    /* Get the current output routing */
    ucontrol->value.enumerated.item[0] = ak7755_data->DSPOfregMode;

    return 0;

}

static int set_DSP_write_ofreg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    u32    currMode = ucontrol->value.enumerated.item[0];
	int    ret;

/*
	if ( currMode == ak7755_data->DSPOfregMode ) {
		akdbgprt("\t%s Same CRAM mode =%d\n",__FUNCTION__, currMode);
		return(0);
	}
	akdbgprt("\t%s OFREG mode =%d\n",__FUNCTION__, currMode);
*/
	ret = ak7755_firmware_write_ram(RAMTYPE_OFREG, currMode);
	if ( ret != 0 ) return(-1);

	ak7755_data->DSPOfregMode =currMode;

	return(0);
}

static int get_DSP_write_acram(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    /* Get the current output routing */
    ucontrol->value.enumerated.item[0] = ak7755_data->DSPAcramMode;

    return 0;

}

static int set_DSP_write_acram(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    u32    currMode = ucontrol->value.enumerated.item[0];
	int    ret;

/*
	if ( currMode == ak7755_data->DSPAcramMode ) {
		akdbgprt("\t%s Same CRAM mode =%d\n",__FUNCTION__, currMode);
		return(0);
	}
*/
	akdbgprt("\t%s ACRAM mode =%d\n",__FUNCTION__, currMode);

	ret = ak7755_firmware_write_ram(RAMTYPE_ACRAM, currMode);
	if ( ret != 0 ) return(-1);

	ak7755_data->DSPAcramMode =currMode;

	return(0);
}

#ifdef AK7755_DEBUG

static int test_read_ram(int mode)
{
	u8	tx[3], rx[512];
	int i, n, plen, clen;

	akdbgprt("*****[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	snd_soc_update_bits(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING, 0x1, 0x1);  // DLRDY bit = 1
	mdelay(1);
	if ( mode == 1 ) {
		plen = 10;
		for ( n = 0 ; n < (5 * plen) ; n++ ) rx[n] = 0;
		tx[0] = 0x38;
		tx[1] = 0x0;
		tx[2] = 0x0;
		ak7755_reads(tx, 3, rx, 5 * plen);
		printk("*****%s PRAM LEN = %d *******\n", __func__, plen);
		n = 0;
		for ( i = 0 ; i < plen ; i ++ ) {
			printk("PAddr=%x %x %x %x %x %x\n", i,(int)rx[n], (int)rx[n+1], (int)rx[n+2], (int)rx[n+3], (int)rx[n+4]);
			n += 5;
		}
	}
	else if ( mode < 5 ) {
		clen = 80;
		for ( n = 0 ; n < (3 * clen) ; n++ ) rx[n] = 0;
		if ( mode == 2 ) tx[0] = 0x34;
		else if ( mode == 3 ) {
			tx[0] = 0x32;
			clen = 16;
		}
		else  tx[0] = 0x3B;
		tx[1] = 0x0;
		tx[2] = 0x0;
		ak7755_reads(tx, 3, rx, 3 * clen);
		printk("*****%s RAM CMD=%d,  LEN = %d*******\n", __func__, (int)tx[0], clen);
		n = 0;
		for ( i = 0 ; i < clen ; i ++ ) {
			printk("CAddr=%x %x %x %x\n", i,(int)rx[n], (int)rx[n+1], (int)rx[n+2]);
			n += 3;
		}
	}
	mdelay(1);
	snd_soc_update_bits(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING, 0x1, 0x0);  // DLRDY bit = 0

	return(0);

}

static const char *test_reg_select[]   =
{
    "read AK7755 Reg 00:24",
	"read PRAM",
	"read CRAM",
	"read OFREG",
	"read ACRAM",
};

static const struct soc_enum ak7755_enum[] =
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

	if ( currMode == 0 ) {
		regs = 0xC0;
		rege = 0xDE;

		for ( i = regs ; i <= rege ; i++ ){
			value = snd_soc_read(codec, i);
			printk("***AK7755 Addr,Reg=(%x, %x)\n", i, value);
		}
	}
	else if ( currMode < 5 ) {
		test_read_ram(currMode);
	}

	return 0;

}
#endif

static const struct snd_kcontrol_new ak7755_snd_controls[] = {
	SOC_SINGLE_TLV("MIC Input Volume L",
			AK7755_D2_MIC_GAIN_SETTING, 0, 0x0F, 0, mgnl_tlv),
	SOC_SINGLE_TLV("MIC Input Volume R",
			AK7755_D2_MIC_GAIN_SETTING, 4, 0x0F, 0, mgnr_tlv),
	SOC_SINGLE_TLV("Line Out Volume 1",
			AK7755_D4_LO1_LO2_VOLUME_SETTING, 0, 0x0F, 0, lovol1_tlv),
	SOC_SINGLE_TLV("Line Out Volume 2",
			AK7755_D4_LO1_LO2_VOLUME_SETTING, 4, 0x0F, 0, lovol2_tlv),
	SOC_SINGLE_TLV("Line Out Volume 3",
			AK7755_D3_LIN_LO3_VOLUME_SETTING, 0, 0x0F, 0, lovol3_tlv),
	SOC_ENUM_EXT("Line Input Volume", ak7755_linein_enum[0], get_linein, set_linein),
	SOC_SINGLE_TLV("ADC Digital Volume L",
			AK7755_D5_ADC_DVOLUME_SETTING1, 0, 0xFF, 1, voladl_tlv),
	SOC_SINGLE_TLV("ADC Digital Volume R",
			AK7755_D6_ADC_DVOLUME_SETTING2, 0, 0xFF, 1, voladr_tlv),
	SOC_SINGLE_TLV("ADC2 Digital Volume L",
			AK7755_D7_ADC2_DVOLUME_SETTING1, 0, 0xFF, 1, volad2l_tlv),
	SOC_SINGLE_TLV("ADC2 Digital Volume R",
			AK7755_DD_ADC2_DVOLUME_SETTING2, 0, 0xFF, 1, volad2r_tlv),
	SOC_SINGLE_TLV("DAC Digital Volume L",
			AK7755_D8_DAC_DVOLUME_SETTING1, 0, 0xFF, 1, voldal_tlv),
	SOC_SINGLE_TLV("DAC Digital Volume R",
			AK7755_D9_DAC_DVOLUME_SETTING2, 0, 0xFF, 1, voldar_tlv),

	SOC_SINGLE("ADC Mute", AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 7, 1, 0),
	SOC_SINGLE("ADC2 Mute", AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 6, 1, 0),
	SOC_SINGLE("DAC Mute", AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 5, 1, 0),
	SOC_SINGLE("Analog DRC Lch", AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 2, 1, 0),
	SOC_SINGLE("Analog DRC Rch", AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 3, 1, 0),
	SOC_SINGLE("MICGAIN Lch Zero-cross", AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 0, 1, 0),
	SOC_SINGLE("MICGAIN Rch Zero-cross", AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 1, 1, 0),

	SOC_ENUM("DAC De-emphasis", ak7755_set_enum[10]),

	SOC_SINGLE("JX0 Enable", AK7755_C2_SERIAL_DATA_FORMAT, 0, 1, 0),
	SOC_SINGLE("JX1 Enable", AK7755_C2_SERIAL_DATA_FORMAT, 1, 1, 0),
	SOC_SINGLE("JX2 Enable", AK7755_C1_CLOCK_SETTING2, 7, 1, 0),
	SOC_SINGLE("JX3 Enable", AK7755_C5_ACCELARETOR_SETTING, 6, 1, 0),

	SOC_ENUM("DLRAM Mode(Bank1:Bank0)", ak7755_set_enum[0]),
	SOC_ENUM("DRAM Size(Bank1:Bank0)", ak7755_set_enum[1]),
	SOC_ENUM("DRAM Addressing Mode(Bank1:Bank0)", ak7755_set_enum[2]),
	SOC_ENUM("POMODE DLRAM Pointer 0", ak7755_set_enum[3]),
	SOC_ENUM("CRAM Memory Assignment", ak7755_set_enum[4]),
	SOC_ENUM("FIRMODE1 Accelerator Ch1", ak7755_set_enum[5]),
	SOC_ENUM("FIRMODE2 Accelerator Ch2", ak7755_set_enum[6]),
	SOC_ENUM("SUBMODE1 Accelerator Ch1", ak7755_set_enum[7]),
	SOC_ENUM("SUBMODE2 Accelerator Ch2", ak7755_set_enum[8]),
	SOC_ENUM("Accelerator Memory(ch1:ch2)", ak7755_set_enum[9]),
	SOC_ENUM("CLKO pin", ak7755_set_enum[11]),
	SOC_ENUM("CLKO Output Clock", ak7755_set_enum[12]),

	SOC_ENUM_EXT("BICK fs", ak7755_bick_enum[0], get_bickfs, set_bickfs),     // '15/06/15

// Added for AK7755
	SOC_ENUM_EXT("DSP Firmware PRAM", ak7755_firmware_enum[0], get_DSP_write_pram, set_DSP_write_pram),
	SOC_ENUM_EXT("DSP Firmware CRAM", ak7755_firmware_enum[1], get_DSP_write_cram, set_DSP_write_cram),
	SOC_ENUM_EXT("DSP Firmware OFREG", ak7755_firmware_enum[2], get_DSP_write_ofreg, set_DSP_write_ofreg),
	SOC_ENUM_EXT("DSP Firmware ACRAM", ak7755_firmware_enum[3], get_DSP_write_acram, set_DSP_write_acram),
#if 0
#ifdef AK7755_CRAM_BASIC_COTROL
	SOC_ENUM_EXT("CRAM EQ1 Level", ak7755_cram_write_enum[0], get_cram_write_eq1, set_cram_write_eq1),
	SOC_ENUM_EXT("CRAM EQ2 Level", ak7755_cram_write_enum[0], get_cram_write_eq2, set_cram_write_eq2),
	SOC_ENUM_EXT("CRAM EQ3 Level", ak7755_cram_write_enum[0], get_cram_write_eq3, set_cram_write_eq3),
	SOC_ENUM_EXT("CRAM EQ4 Level", ak7755_cram_write_enum[0], get_cram_write_eq4, set_cram_write_eq4),
	SOC_ENUM_EXT("CRAM EQ5 Level", ak7755_cram_write_enum[0], get_cram_write_eq5, set_cram_write_eq5),
	SOC_ENUM_EXT("CRAM HPF1 fc", ak7755_cram_write_enum[1], get_cram_write_hpf1fc, set_cram_write_hpf1fc),
	SOC_ENUM_EXT("CRAM HPF2 fc", ak7755_cram_write_enum[1], get_cram_write_hpf2fc, set_cram_write_hpf2fc),
	SOC_ENUM_EXT("CRAM Limiter Release Time", ak7755_cram_write_enum[2], get_cram_write_limrel, set_cram_write_limrel),
	SOC_ENUM_EXT("CRAM Limiter Volume", ak7755_cram_write_enum[3], get_cram_write_limvol, set_cram_write_limvol),
#endif
#endif
#ifdef AK7755_DEBUG
	SOC_ENUM_EXT("Reg Read", ak7755_enum[0], get_test_reg, set_test_reg),
#endif

	SOC_ENUM_EXT("SELMIX2-0", ak7755_selmix_enum[0], get_selmix, set_selmix),	//SELMIX2-0 bit setting

};


/* Clock Event for DAC, ADC */
static int ak7755_clkset_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:	/* after widget power up */
		snd_soc_update_bits(codec, AK7755_CF_RESET_POWER_SETTING, 0x08,0x08);	//CRESETN(CODEC ResetN)=1
		break;
	}

	return 0;
}

/* Clock Event */
static int ak7755_clock_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) //CONFIG_LINF
{
	struct snd_soc_codec *codec = w->codec;

	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	switch (event) {
		case SND_SOC_DAPM_POST_PMU:	/* after widget power up */
			akdbgprt("\t[AK7755] %s wait=10msec\n",__FUNCTION__);
			mdelay(10);
			break;
		case SND_SOC_DAPM_PRE_PMD:	/* before widget power down */
			snd_soc_update_bits(codec, AK7755_CF_RESET_POWER_SETTING, 0x08,0x00);	//CODEC Reset
			break;
	}

	return 0;
}

/* SDOUT 1 switch */
static const struct snd_kcontrol_new ak7755_out1e_control =
	SOC_DAPM_SINGLE("Switch", AK7755_CA_CLK_SDOUT_SETTING, 0, 1, 0);

/* SDOUT 2 switch */
static const struct snd_kcontrol_new ak7755_out2e_control =
	SOC_DAPM_SINGLE("Switch", AK7755_CA_CLK_SDOUT_SETTING, 1, 1, 0);

/* SDOUT 3 switch */
static const struct snd_kcontrol_new ak7755_out3e_control =
	SOC_DAPM_SINGLE("Switch", AK7755_CA_CLK_SDOUT_SETTING, 2, 1, 0);

/* LineOut1 Switch */
static const char *ak7755_pmlo1_select_texts[] =
		{"Off", "On"};

static const struct soc_enum ak7755_pmlo1_mux_enum =
	SOC_ENUM_SINGLE(AK7755_CE_POWER_MANAGEMENT, 2,
			ARRAY_SIZE(ak7755_pmlo1_select_texts), ak7755_pmlo1_select_texts);
//	SOC_ENUM_SINGLE(0, 0,
//			ARRAY_SIZE(ak7755_pmlo1_select_texts), ak7755_pmlo1_select_texts);

static const struct snd_kcontrol_new ak7755_pmlo1_mux_control =
	SOC_DAPM_ENUM("OUT1 SW", ak7755_pmlo1_mux_enum);
	//SOC_DAPM_ENUM_VIRT("OUT1 SW", ak7755_pmlo1_mux_enum);

/* LineOut2 Switch */
static const char *ak7755_pmlo2_select_texts[] =
		{"Off", "On"};

static const struct soc_enum ak7755_pmlo2_mux_enum =
	SOC_ENUM_SINGLE(AK7755_CE_POWER_MANAGEMENT, 3,
			ARRAY_SIZE(ak7755_pmlo2_select_texts), ak7755_pmlo2_select_texts);
//	SOC_ENUM_SINGLE(0, 0,
//			ARRAY_SIZE(ak7755_pmlo2_select_texts), ak7755_pmlo2_select_texts);

static const struct snd_kcontrol_new ak7755_pmlo2_mux_control =
	SOC_DAPM_ENUM("OUT2 SW", ak7755_pmlo2_mux_enum);
//	SOC_DAPM_ENUM_VIRT("OUT2 SW", ak7755_pmlo2_mux_enum);

/* LineOut3 Mixer */
static const struct snd_kcontrol_new ak7755_lo3sw_mixer_controls[] = {
	SOC_DAPM_SINGLE("LOSW1", AK7755_C9_ANALOG_IO_SETTING, 1, 1, 0),
	SOC_DAPM_SINGLE("LOSW2", AK7755_C9_ANALOG_IO_SETTING, 2, 1, 0),
#ifndef DIGITAL_MIC		//LOSW3 used only Analog MIC
	SOC_DAPM_SINGLE("LOSW3", AK7755_C9_ANALOG_IO_SETTING, 3, 1, 0),
#endif
};

/* DSPIn Virt SWITCH */
static const char *ak7755_dspinad_texts[] =
		{"Off", "On"};

static const struct soc_enum ak7755_dspinad_enum =
	SOC_ENUM_SINGLE(0, 0,
			ARRAY_SIZE(ak7755_dspinad_texts), ak7755_dspinad_texts);

static const struct soc_enum ak7755_dspinad2_enum =
	SOC_ENUM_SINGLE(0, 0,
			ARRAY_SIZE(ak7755_dspinad_texts), ak7755_dspinad_texts);

static const struct snd_kcontrol_new ak7755_dspinad_control =
	SOC_DAPM_ENUM_VIRT("DSPINAD Switch", ak7755_dspinad_enum);

static const struct snd_kcontrol_new ak7755_dspinad2_control =
	SOC_DAPM_ENUM_VIRT("DSPINAD2 Switch", ak7755_dspinad2_enum);

/* LIN MUX */
static const char *ak7755_lin_select_texts[] =
		{"IN1", "IN2", "INPN1"};

static const struct soc_enum ak7755_lin_mux_enum =
	SOC_ENUM_SINGLE(AK7755_C9_ANALOG_IO_SETTING, 4,
			ARRAY_SIZE(ak7755_lin_select_texts), ak7755_lin_select_texts);

static const struct snd_kcontrol_new ak7755_lin_mux_control =
	SOC_DAPM_ENUM("LIN Select", ak7755_lin_mux_enum);

/* RIN MUX */
static const char *ak7755_rin_select_texts[] =
		{"IN3", "IN4", "INPN2"};

static const struct soc_enum ak7755_rin_mux_enum =
	SOC_ENUM_SINGLE(AK7755_C9_ANALOG_IO_SETTING, 6,
			ARRAY_SIZE(ak7755_rin_select_texts), ak7755_rin_select_texts);

static const struct snd_kcontrol_new ak7755_rin_mux_control =
	SOC_DAPM_ENUM("RIN Select", ak7755_rin_mux_enum);

/* DAC MUX */
static const char *ak7755_seldai_select_texts[] =
		{"DSP", "MIXOUT", "SDIN2", "SDIN1"};

static const struct soc_enum ak7755_seldai_mux_enum =
	SOC_ENUM_SINGLE(AK7755_C8_DAC_IN_SETTING, 6,
			ARRAY_SIZE(ak7755_seldai_select_texts), ak7755_seldai_select_texts);

static const struct snd_kcontrol_new ak7755_seldai_mux_control =
	SOC_DAPM_ENUM("SELDAI Select", ak7755_seldai_mux_enum);

/* SDOUT1 MUX */
static const char *ak7755_seldo1_select_texts[] =
		{"DSP", "DSP GP0", "SDIN1", "SDOUTAD", "EEST", "SDOUTAD2"};

static const struct soc_enum ak7755_seldo1_mux_enum =
	SOC_ENUM_SINGLE(AK7755_CC_VOLUME_TRANSITION, 0,
			ARRAY_SIZE(ak7755_seldo1_select_texts), ak7755_seldo1_select_texts);

static const struct snd_kcontrol_new ak7755_seldo1_mux_control =
	SOC_DAPM_ENUM("SELDO1 Select", ak7755_seldo1_mux_enum);

/* SDOUT2 MUX */
static const char *ak7755_seldo2_select_texts[] =
		{"DSP", "DSP GP1", "SDIN2", "SDOUTAD2"};

static const struct soc_enum ak7755_seldo2_mux_enum =
	SOC_ENUM_SINGLE(AK7755_C8_DAC_IN_SETTING, 2,
			ARRAY_SIZE(ak7755_seldo2_select_texts), ak7755_seldo2_select_texts);

static const struct snd_kcontrol_new ak7755_seldo2_mux_control =
	SOC_DAPM_ENUM("SELDO2 Select", ak7755_seldo2_mux_enum);

/* SDOUT3 MUX */
static const char *ak7755_seldo3_select_texts[] =
		{"DSP DOUT3", "MIXOUT", "DSP DOUT4", "SDOUTAD2"};

static const struct soc_enum ak7755_seldo3_mux_enum =
	SOC_ENUM_SINGLE(AK7755_C8_DAC_IN_SETTING, 4,
			ARRAY_SIZE(ak7755_seldo3_select_texts), ak7755_seldo3_select_texts);

static const struct snd_kcontrol_new ak7755_seldo3_mux_control =
	SOC_DAPM_ENUM("SELDO3 Select", ak7755_seldo3_mux_enum);

/* SELMIX Virt SWITCH */
static const char *ak7755_selmix_select_texts[] =
		{"Off", "On"};

static const struct soc_enum ak7755_sdoutad_selmix_enum =
	SOC_ENUM_SINGLE(0, 0,
			ARRAY_SIZE(ak7755_selmix_select_texts), ak7755_selmix_select_texts);

static const struct soc_enum ak7755_sdoutad2_selmix_enum =
	SOC_ENUM_SINGLE(0, 0,
			ARRAY_SIZE(ak7755_selmix_select_texts), ak7755_selmix_select_texts);

static const struct soc_enum ak7755_dsp_selmix_enum =
	SOC_ENUM_SINGLE(0, 0,
			ARRAY_SIZE(ak7755_selmix_select_texts), ak7755_selmix_select_texts);

static const struct snd_kcontrol_new ak7755_sdoutad_selmix_control =
	SOC_DAPM_ENUM_VIRT("SELMIX Switch", ak7755_sdoutad_selmix_enum);

static const struct snd_kcontrol_new ak7755_sdoutad2_selmix_control =
	SOC_DAPM_ENUM_VIRT("SELMIX Switch", ak7755_sdoutad2_selmix_enum);

static const struct snd_kcontrol_new ak7755_dsp_selmix_control =
	SOC_DAPM_ENUM_VIRT("SELMIX Switch", ak7755_dsp_selmix_enum);



/* ak7755 dapm widgets */
static const struct snd_soc_dapm_widget ak7755_dapm_widgets[] = {

// ADC, DAC
#ifdef DIGITAL_MIC
	SND_SOC_DAPM_INPUT("DMICIN1"),
	SND_SOC_DAPM_INPUT("DMICIN2"),
	SND_SOC_DAPM_ADC_E("DMIC1 Left",  "NULL", AK7755_CE_POWER_MANAGEMENT, 6, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
	SND_SOC_DAPM_ADC_E("DMIC1 Right", "NULL", AK7755_CE_POWER_MANAGEMENT, 7, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
	SND_SOC_DAPM_ADC_E("DMIC2 Left",  "NULL", AK7755_CE_POWER_MANAGEMENT, 5, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
	SND_SOC_DAPM_ADC_E("DMIC2 Right", "NULL", AK7755_CF_RESET_POWER_SETTING, 1, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
	SND_SOC_DAPM_SUPPLY("DMIC1 CLK", AK7755_DE_DMIC_IF_SETTING, 5, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DMIC2 CLK", AK7755_DE_DMIC_IF_SETTING, 2, 0, NULL, 0),
#else
	SND_SOC_DAPM_ADC_E("ADC Left", "NULL", AK7755_CE_POWER_MANAGEMENT, 6, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
	SND_SOC_DAPM_ADC_E("ADC Right", "NULL", AK7755_CE_POWER_MANAGEMENT, 7, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
	SND_SOC_DAPM_ADC_E("ADC2 Left", "NULL", AK7755_CE_POWER_MANAGEMENT, 5, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
#endif
	SND_SOC_DAPM_DAC_E("DAC Left", "NULL", AK7755_CE_POWER_MANAGEMENT, 0, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
	SND_SOC_DAPM_DAC_E("DAC Right", "NULL", AK7755_CE_POWER_MANAGEMENT, 1, 0,
		ak7755_clkset_event, SND_SOC_DAPM_POST_PMU ),
	SND_SOC_DAPM_SUPPLY("CLOCK", AK7755_C1_CLOCK_SETTING2, 0, 0,
		ak7755_clock_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

// Analog Output
	SND_SOC_DAPM_OUTPUT("Line Out1"),
	SND_SOC_DAPM_OUTPUT("Line Out2"),
	SND_SOC_DAPM_OUTPUT("Line Out3"),

	SND_SOC_DAPM_MIXER("LineOut Amp3 Mixer", AK7755_CE_POWER_MANAGEMENT, 4, 0,
			&ak7755_lo3sw_mixer_controls[0], ARRAY_SIZE(ak7755_lo3sw_mixer_controls)),
//	SND_SOC_DAPM_MUX("LineOut Amp2", AK7755_CE_POWER_MANAGEMENT, 3, 0, &ak7755_pmlo2_mux_control),
//	SND_SOC_DAPM_MUX("LineOut Amp1", AK7755_CE_POWER_MANAGEMENT, 2, 0, &ak7755_pmlo1_mux_control),

	SND_SOC_DAPM_MUX("LineOut Amp1", SND_SOC_NOPM, 0, 0,	&ak7755_pmlo1_mux_control),
	SND_SOC_DAPM_MUX("LineOut Amp2", SND_SOC_NOPM, 0, 0,	&ak7755_pmlo2_mux_control),

// Analog Input
	SND_SOC_DAPM_INPUT("LIN"),
	SND_SOC_DAPM_INPUT("IN1"),
	SND_SOC_DAPM_INPUT("IN2"),
	SND_SOC_DAPM_INPUT("IN3"),
	SND_SOC_DAPM_INPUT("IN4"),
	SND_SOC_DAPM_INPUT("INPN1"),
	SND_SOC_DAPM_INPUT("INPN2"),

	SND_SOC_DAPM_PGA("LineIn Amp", AK7755_CF_RESET_POWER_SETTING, 5, 0, NULL, 0),
	SND_SOC_DAPM_MUX("RIN MUX", SND_SOC_NOPM, 0, 0,	&ak7755_rin_mux_control),
	SND_SOC_DAPM_MUX("LIN MUX", SND_SOC_NOPM, 0, 0,	&ak7755_lin_mux_control),
	SND_SOC_DAPM_VIRT_MUX("DSPIN SDOUTAD2", SND_SOC_NOPM, 0, 0, &ak7755_dspinad2_control),
	SND_SOC_DAPM_VIRT_MUX("DSPIN SDOUTAD", SND_SOC_NOPM, 0, 0, &ak7755_dspinad_control),

// Digital Input/Output
	SND_SOC_DAPM_AIF_IN("SDIN1", "Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("SDIN2", "Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("SDOUT1", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("SDOUT2", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("SDOUT3", "Capture", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_SWITCH("SDOUT3 Enable", SND_SOC_NOPM, 0, 0,
		&ak7755_out3e_control),
	SND_SOC_DAPM_SWITCH("SDOUT2 Enable", SND_SOC_NOPM, 0, 0,
		&ak7755_out2e_control),
	SND_SOC_DAPM_SWITCH("SDOUT1 Enable", SND_SOC_NOPM, 0, 0,
		&ak7755_out1e_control),

	SND_SOC_DAPM_PGA("SDOUTAD", SND_SOC_NOPM, 0, 0, NULL, 0),		//ADC's Output
	SND_SOC_DAPM_PGA("SDOUTAD2", SND_SOC_NOPM, 0, 0, NULL, 0),		//ADC2's Output
	SND_SOC_DAPM_PGA("DSP", AK7755_CF_RESET_POWER_SETTING, 2, 0, NULL, 0),

	SND_SOC_DAPM_AIF_IN("DSP GP0", NULL, 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("DSP GP1", NULL, 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("EEST", NULL, 0, SND_SOC_NOPM, 0, 0),

// Multiplexer (selects 1 analog signal from many inputs) & Mixer
	SND_SOC_DAPM_MUX("SDOUT3 MUX", SND_SOC_NOPM, 0, 0, &ak7755_seldo3_mux_control),
	SND_SOC_DAPM_MUX("SDOUT2 MUX", SND_SOC_NOPM, 0, 0, &ak7755_seldo2_mux_control),
	SND_SOC_DAPM_MUX("SDOUT1 MUX", SND_SOC_NOPM, 0, 0, &ak7755_seldo1_mux_control),
	SND_SOC_DAPM_MUX("DAC MUX", SND_SOC_NOPM, 0, 0, &ak7755_seldai_mux_control),
	SND_SOC_DAPM_VIRT_MUX("SELMIX DSP", SND_SOC_NOPM, 0, 0, &ak7755_dsp_selmix_control),
	SND_SOC_DAPM_VIRT_MUX("SELMIX AD2", SND_SOC_NOPM, 0, 0, &ak7755_sdoutad2_selmix_control),
	SND_SOC_DAPM_VIRT_MUX("SELMIX AD", SND_SOC_NOPM, 0, 0, &ak7755_sdoutad_selmix_control),
	SND_SOC_DAPM_PGA("SELMIX Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),

};

static const struct snd_soc_dapm_route ak7755_intercon[] =
{

#ifdef DIGITAL_MIC
	{"DMIC1 Left", "NULL", "CLOCK"},
	{"DMIC1 Right", "NULL", "CLOCK"},
	{"DMIC2 Left", "NULL", "CLOCK"},
	{"DMIC2 Right", "NULL", "CLOCK"},

	{"DMICIN1", "NULL", "DMIC1 CLK"},
	{"DMICIN2", "NULL", "DMIC2 CLK"},
	{"DMIC1 Left", "NULL", "DMICIN1"},
	{"DMIC1 Right", "NULL", "DMICIN1"},
	{"DMIC2 Left", "NULL", "DMICIN2"},
	{"DMIC2 Right", "NULL", "DMICIN2"},
	{"SDOUTAD", "NULL", "DMIC1 Left"},
	{"SDOUTAD", "NULL", "DMIC1 Right"},
	{"SDOUTAD2", "NULL", "DMIC2 Left"},
	{"SDOUTAD2", "NULL", "DMIC2 Right"},
#else
	{"ADC Left", "NULL", "CLOCK"},
	{"ADC Right", "NULL", "CLOCK"},
	{"ADC2 Left", "NULL", "CLOCK"},

	{"LineIn Amp", "NULL", "LIN"},
	{"ADC2 Left", "NULL", "LineIn Amp"},
	{"SDOUTAD2", "NULL", "ADC2 Left"},

	{"LIN MUX", "IN1", "IN1"},
	{"LIN MUX", "IN2", "IN2"},
	{"LIN MUX", "INPN1", "INPN1"},
	{"RIN MUX", "IN3", "IN3"},
	{"RIN MUX", "IN4", "IN4"},
	{"RIN MUX", "INPN2", "INPN2"},
	{"ADC Left", "NULL", "LIN MUX"},
	{"ADC Right", "NULL", "RIN MUX"},
	{"SDOUTAD", "NULL", "ADC Left"},
	{"SDOUTAD", "NULL", "ADC Right"},
#endif

	{"DAC Left", "NULL", "CLOCK"},
	{"DAC Right", "NULL", "CLOCK"},
	{"DSP", "NULL", "CLOCK"},

	{"DSP", "NULL", "SDIN1"},
	{"DSP", "NULL", "SDIN2"},

	{"DSPIN SDOUTAD", "On", "SDOUTAD"},
	{"DSPIN SDOUTAD2", "On", "SDOUTAD2"},
	{"DSP", "NULL", "DSPIN SDOUTAD"},
	{"DSP", "NULL", "DSPIN SDOUTAD2"},

	{"SELMIX AD", "On", "SDOUTAD"},
	{"SELMIX AD2", "On", "SDOUTAD2"},
	{"SELMIX DSP", "On", "DSP"},
	{"SELMIX Mixer", "NULL", "SELMIX AD"},
	{"SELMIX Mixer", "NULL", "SELMIX AD2"},
	{"SELMIX Mixer", "NULL", "SELMIX DSP"},
	{"DAC MUX", "MIXOUT", "SELMIX Mixer"},
	{"DAC MUX", "DSP", "DSP"},
	{"DAC MUX", "SDIN2", "SDIN2"},
	{"DAC MUX", "SDIN1", "SDIN1"},

	{"DAC Left", "NULL", "DAC MUX"},
	{"DAC Right", "NULL", "DAC MUX"},

	{"LineOut Amp1", "On", "DAC Left"},
	{"LineOut Amp2", "On", "DAC Right"},
	{"Line Out1", "NULL", "LineOut Amp1"},
	{"Line Out2", "NULL", "LineOut Amp2"},
	{"LineOut Amp3 Mixer", "LOSW1", "DAC Left"},
	{"LineOut Amp3 Mixer", "LOSW2", "DAC Right"},
#ifndef DIGITAL_MIC		//LOSW3 used only Analog MIC
	{"LineOut Amp3 Mixer", "LOSW3", "LineIn Amp"},
#endif
	{"Line Out3", "NULL", "LineOut Amp3 Mixer"},

	{"SDOUT1 MUX", "DSP", "DSP"},
	{"SDOUT1 MUX", "DSP GP0", "DSP GP0"},
	{"SDOUT1 MUX", "SDIN1", "SDIN1"},
	{"SDOUT1 MUX", "SDOUTAD", "SDOUTAD"},
	{"SDOUT1 MUX", "EEST", "EEST"},
	{"SDOUT1 MUX", "SDOUTAD2", "SDOUTAD2"},

	{"SDOUT2 MUX", "DSP", "DSP"},
	{"SDOUT2 MUX", "DSP GP1", "DSP GP1"},
	{"SDOUT2 MUX", "SDIN2", "SDIN2"},
	{"SDOUT2 MUX", "SDOUTAD2", "SDOUTAD2"},

	{"SDOUT3 MUX", "DSP DOUT3", "DSP"},
	{"SDOUT3 MUX", "MIXOUT", "DAC MUX"},
	{"SDOUT3 MUX", "DSP DOUT4", "DSP"},
	{"SDOUT3 MUX", "SDOUTAD2", "SDOUTAD2"},

	{"SDOUT1 Enable", "Switch", "SDOUT1 MUX"},
	{"SDOUT2 Enable", "Switch", "SDOUT2 MUX"},
	{"SDOUT3 Enable", "Switch", "SDOUT3 MUX"},

	{"SDOUT1", "NULL", "SDOUT1 Enable"},
	{"SDOUT2", "NULL", "SDOUT2 Enable"},
	{"SDOUT3", "NULL", "SDOUT3 Enable"},

};

static int ak7755_hw_params_set(struct snd_soc_codec *codec, int nfs)
{
	u8 	fs;

	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	fs = snd_soc_read(codec, AK7755_C0_CLOCK_SETTING1);
	fs &= ~AK7755_FS;

	switch (nfs) {
	case 8000:
		fs |= AK7755_FS_8KHZ;
		break;
	case 11025:
		fs |= AK7755_FS_12KHZ;
		break;
	case 16000:
		fs |= AK7755_FS_16KHZ;
		break;
	case 22050:
		fs |= AK7755_FS_24KHZ;
		break;
	case 32000:
		fs |= AK7755_FS_32KHZ;
		break;
	case 44100:
	case 48000:
		fs |= AK7755_FS_48KHZ;
		break;
	case 88200:
	case 96000:
		fs |= AK7755_FS_96KHZ;
		break;
	default:
		return -EINVAL;
	}
	snd_soc_write(codec, AK7755_C0_CLOCK_SETTING1, fs);
	ak7755_set_status(RUN);
	if (aec == 1){
		snd_soc_update_bits(codec, AK7755_CE_POWER_MANAGEMENT, 0x80, 0x0);		//If aec is enable, only CH-L enable by DSP
	}

	return 0;
}

static int ak7755_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);
	u8 format;

	format = snd_soc_read(codec, AK7755_C2_SERIAL_DATA_FORMAT);
	format &= ~AK7755_LRIF;
	if (aec ==0){
		switch (params_format(params)) {
			case SNDRV_PCM_FORMAT_S16_LE:
				ak7755->bickfs = (AK7755_AIF_BICK32 >> 4);
				break;
			case SNDRV_PCM_FORMAT_S24_LE:
				ak7755->bickfs = (AK7755_AIF_BICK48 >> 4);
				break;
			case SNDRV_PCM_FORMAT_S32_LE:
				ak7755->bickfs = (AK7755_AIF_BICK64 >> 4);
				break;
			default:
				dev_err(codec->dev, "Can not support the format");
				return -EINVAL;
		}
	}else{
		ak7755->bickfs = (AK7755_AIF_BICK64 >> 4);
	}
	ak7755->fs = params_rate(params);
	ak7755_hw_params_set(codec, ak7755->fs);

	snd_soc_update_bits(codec, AK7755_C1_CLOCK_SETTING2, 0x30, (ak7755->bickfs << 4));
	if ( ak7755->bickfs < 3 ) {
		format &= 0x7F;
	} else {
		format |= 0x80;
		format &= 0xF3;
		format |=  AK7755_TDM_INPUT_SOURCE;
	}

	switch (ak7755->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		format |= AK7755_LRIF_I2S_MODE;
		if ( ak7755->bickfs == 2 ) {   // 32fs
			snd_soc_update_bits(codec, AK7755_C3_DELAY_RAM_DSP_IO, 0xF0, 0xf0); 	// DIF2, DOF2
			snd_soc_update_bits(codec, AK7755_C6_DAC_DEM_SETTING, 0x37, 0x33);		//DIF, DIFDA
			snd_soc_update_bits(codec, AK7755_C7_DSP_OUT_SETTING, 0xFF, 0xf3); 		//DOF
		}else {
			snd_soc_update_bits(codec, AK7755_C3_DELAY_RAM_DSP_IO, 0xF0, 0x0); 	// DIF2, DOF2
			snd_soc_update_bits(codec, AK7755_C6_DAC_DEM_SETTING, 0x37, 0x0);	//DIF, DIFDA
			snd_soc_update_bits(codec, AK7755_C7_DSP_OUT_SETTING, 0xFF, 0x0); 	//DOF
		}
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		format |= AK7755_LRIF_MSB_MODE;
		if ( ak7755->bickfs == 2 ) {   // 32fs
			snd_soc_update_bits(codec, AK7755_C3_DELAY_RAM_DSP_IO, 0xF0, 0xF0); 	// DIF2, DOF2
			snd_soc_update_bits(codec, AK7755_C6_DAC_DEM_SETTING, 0x37, 0x33);	//DIF, DIFDA
			snd_soc_update_bits(codec, AK7755_C7_DSP_OUT_SETTING, 0xFF, 0xF3); 	//DOF
		}
		else {
			snd_soc_update_bits(codec, AK7755_C3_DELAY_RAM_DSP_IO, 0xF0, 0x0); 	// DIF2, DOF2
			snd_soc_update_bits(codec, AK7755_C6_DAC_DEM_SETTING, 0x37, 0x0);	//DIF, DIFDA
			snd_soc_update_bits(codec, AK7755_C7_DSP_OUT_SETTING, 0xFF, 0x0); 	//DOF
		}
		break;
/*
	case SND_SOC_DAIFMT_PCM_SHORT:
		format &= 0xBF;							//BCKP Clear
		format |= (AK7755_BCKP_BIT << 6);		//BCKP set
		format |= AK7755_LRIF_PCM_SHORT_MODE;
		break;
	case SND_SOC_DAIFMT_PCM_LONG:
		format &= 0xBF;							//BCKP lear
		format |= (AK7755_BCKP_BIT << 6);		//BCKP set
		format |= AK7755_LRIF_PCM_LONG_MODE;
		break;
*/
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, AK7755_C2_SERIAL_DATA_FORMAT, format);
	return 0;
}

static int ak7755_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	ak7755_data->rclk = freq;

	return 0;
}

static int ak7755_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{

	struct snd_soc_codec *codec = dai->codec;
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);  // '15/06/15
	u8 mode, mode2;

	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	/* set master/slave audio interface */
	mode = snd_soc_read(codec, AK7755_C0_CLOCK_SETTING1);//CKM2-0(M/S)
	mode &= ~AK7755_M_S;
	mode2 = snd_soc_read(codec, AK7755_CA_CLK_SDOUT_SETTING);//BICKOE,LRCKOE

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
        case SND_SOC_DAIFMT_CBS_CFS:
	#ifdef CLOCK_MODE_BICK
	    	mode |= AK7755_M_S_3;//CKM mode = 3(Slave, BICK)
	#else
	    	mode |= AK7755_M_S_2;//CKM mode = 2(Slave, XTI=12.288MHz)
	#endif
          	mode2 &= ~AK7755_BICK_LRCK;//BICK = LRCK = 0
            break;
        case SND_SOC_DAIFMT_CBM_CFM:
	#ifdef CLOCK_MODE_18_432
	    	mode |= AK7755_M_S_1;//CKM mode = 1(Master, XTI=18.432MHz)
	#else
	    	mode |= AK7755_M_S_0;//CKM mode = 0(Master, XTI=12.288MHz)
	#endif
		mode2 |= AK7755_BICK_LRCK;//BICK = LRCK = 1
            break;
        case SND_SOC_DAIFMT_CBS_CFM:
        case SND_SOC_DAIFMT_CBM_CFS:
        default:
            dev_err(codec->dev, "Clock mode unsupported");
           return -EINVAL;
	}

	ak7755->fmt = fmt;
	/* set mode */
	snd_soc_write(codec, AK7755_C0_CLOCK_SETTING1, mode);
	snd_soc_write(codec, AK7755_CA_CLK_SDOUT_SETTING, mode2);
	return 0;
}

static int ak7755_volatile(struct snd_soc_codec *codec, unsigned int reg)
{
	int	ret;

	switch (reg) {
//		case :
//			ret = 1;
		default:
			ret = 0;
			break;
	}
	return ret;
}

static int ak7755_readable(struct snd_soc_codec *codec, unsigned int reg)
{
	return ak7755_access_masks[reg].readable != 0;

}

static int ak7755_reads(u8 *tx, size_t wlen, u8 *rx, size_t rlen)
{
	int ret;

	//akdbgprt("*****[AK7755] %s tx[0]=%x, %d, %d\n",__FUNCTION__, tx[0],wlen,rlen);
	if (ak7755_data->control_type == SND_SOC_SPI) {
		ret = spi_write_then_read(ak7755_data->spi, tx, wlen, rx, rlen);
	}
	else {
		ret = ak7755_i2c_read( tx, wlen, rx, rlen);
	}

	return ret;

}

static int ak7755_writes(const u8 *tx, size_t wlen)
{
	int rc;

	akdbgprt("![AK7755W] %s tx[0]=%x tx[1]=%x, len=%d\n",__FUNCTION__, (int)tx[0], (int)tx[1], wlen);

	if (ak7755_data->control_type == SND_SOC_SPI)
		rc = spi_write_then_read(ak7755_data->spi, tx, wlen, NULL, 0);
	else
		rc = i2c_master_send(ak7755_data->i2c, tx, wlen);

	if (rc < 0) {
		akdbgprt("\t[AK7755] %s error rc = %d\n",__FUNCTION__, rc);
	}

	return rc;
}

#ifdef AK7755_CONTIF_DEBUG
static inline void ak7755_write_reg_cache(struct snd_soc_codec *codec,u16 reg,u16 value)
{
    u8 *cache = codec->reg_cache;
    //BUG_ON(reg_index > ARRAY_SIZE(ak7755_reg));

    if (reg < ARRAY_SIZE(ak7755_reg))
    	cache[reg] = (u8)value;
}

/*
* Read ak7755 register cache
 */
static inline u32 ak7755_read_reg_cache(struct snd_soc_codec *codec, u16 reg)
{
    u8 *cache = codec->reg_cache;
    BUG_ON(reg > ARRAY_SIZE(ak7755_reg));
    return (u32)cache[reg];
}


static unsigned int ak7755_i2c_read(
u8 *reg,
int reglen,
u8 *data,
int datalen)
{
	struct i2c_msg xfer[2];
	int ret;
	struct i2c_client *client = ak7755_data->i2c;

	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = reglen;
	xfer[0].buf = reg;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = datalen;
	xfer[1].buf = data;

	ret = i2c_transfer(client->adapter, xfer, 2);

//	akdbgprt("*****[AK7755] %s (%x,%x)\n",__FUNCTION__, (int)reg[0], (int)data[0]);

	if (ret == 2)
		return 0;
	else if (ret < 0)
		return -ret;
	else
		return -EIO;
}

unsigned int ak7755_reg_read(struct snd_soc_codec *codec, unsigned int reg)
{
	unsigned char tx[1], rx[1];
	int	wlen, rlen;
	int ret;
	unsigned int rdata;

	wlen = 1;
	rlen = 1;
	tx[0] = (unsigned char)(0x7F & reg);

	//akdbgprt("*****[AK7755] %s reg = %0x, tx[0]=%0x, %d, %d\n",__FUNCTION__, reg,  tx[0],wlen,rlen);

	if (ak7755_data->control_type == SND_SOC_SPI) {
		ret = spi_write_then_read(ak7755_data->spi, tx, wlen, rx, rlen);
	}
	else {
		ret = ak7755_i2c_read(tx, wlen, rx, rlen);
	}

	if (ret < 0) {
		akdbgprt("\t[AK7755] %s error ret = %d\n",__FUNCTION__, ret);
		rdata = -EIO;
		return rdata;
	}
	else {
		rdata = (unsigned int)rx[0];
		if (ak7755_read_reg_cache(codec, reg) != rdata)
			ak7755_write_reg_cache(codec, reg, rdata);
	}
	akdbgprt("\t[AK7755] %s addr, read data =(%x, %x)\n",__FUNCTION__, reg, (int)rdata);

	return rdata;
}


int ak7755_reg_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	unsigned char tx[3];
	int	wlen;
	int ret;

	wlen = 2;
	tx[0] = reg;
	tx[1] = value;

	ret = ak7755_writes(tx, wlen);

	if (ret >=0) {
		ak7755_write_reg_cache(codec, reg, value);
	}

	return ret;
}


/*
 * Write with Mask to  AK7755 register space
 */
// ak7755_writeMask() => snd_soc_update_bits()  // '16/01/13

#endif


static int crc_read(void)
{
	int rc;
	u8	tx[1], rx[2];

	tx[0] = CRC_COMMAND_READ_RESULT;

	rc =  ak7755_reads(tx, 1, rx, 2);

	return (rc < 0) ? rc : ((rx[0] << 8) + rx[1]);
}

static int ak7755_set_status(enum ak7755_status status)
{

	switch (status) {
	case RUN:
		snd_soc_update_bits(ak7755_data->codec, AK7755_C1_CLOCK_SETTING2, 0x1, 0x1);  // CKRESETN bit = 1
		mdelay(10);
		snd_soc_update_bits(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING, 0xd, 0xc);  // CRESETN bit = DSPRESETN = 1;
		mdelay(10);
		break;
	case DOWNLOAD:
		snd_soc_update_bits(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING, 0x1, 0x1);  // DLRDY bit = 1
		mdelay(10);
		break;
	case STANDBY:
		snd_soc_update_bits(ak7755_data->codec, AK7755_C1_CLOCK_SETTING2, 0x1, 0x1);  // CKRESETN bit = 1
		mdelay(10);
		snd_soc_update_bits(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING, 0xc, 0x0);  // CRESETN bit = DSPRESETN = 0;
		mdelay(10);
		break;
	case SUSPEND:
	case POWERDOWN:
		snd_soc_update_bits(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING, 0x3f, 0x0);
		snd_soc_update_bits(ak7755_data->codec, AK7755_CE_POWER_MANAGEMENT, 0xFF, 0x0);
		snd_soc_update_bits(ak7755_data->codec, AK7755_C1_CLOCK_SETTING2, 0x0, 0x0);
		mdelay(10);
		break;
	default:
		return -EINVAL;
	}

	ak7755_data->status = status;

	return 0;
}

static int ak7755_ram_download(const u8 *tx_ram, u64 num, u16 crc)
{
	int rc;
	u16	read_crc;
	u8 tx[2];

	akdbgprt("\t[AK7755] %s num=%ld\n",__FUNCTION__, (long int)num);
	if (fast_boot == 0)
		ak7755_set_status(DOWNLOAD);

	rc = ak7755_writes(tx_ram, num);
	if (rc < 0) {
		printk("%s: RAM Write Error! RAM size = %lld \n", __func__, num);
		if ( fast_boot == 1 ) {
			tx[0] = AK7755_CF_RESET_POWER_SETTING;
			tx[1] = 0x0c;
			ak7755_writes(tx, 2);
		} else {
			snd_soc_update_bits(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING, 0x0d,0x0c);  // system rest release
		}
		return rc;
	}

	if ( ( crc != 0 ) && (rc >= 0) )  {
		read_crc = crc_read();
		akdbgprt("\t[AK7755] %s CRC Cal=%x Read=%x\n",__FUNCTION__, (int)crc,(int)read_crc);

		if ( read_crc == crc ) rc = 0;
		else rc = 1;
	}
	if ( fast_boot == 0 )
		ak7755_set_status(STANDBY);

	return rc;

}

static int calc_CRC(int length, u8 *data )
{

#define CRC16_CCITT (0x1021)

	unsigned short crc = 0x0000;
	int i, j;

	for ( i = 0; i < length; i++ ) {
		crc ^= *data++ << 8;
		for ( j = 0; j < 8; j++) {
			if ( crc & 0x8000) {
				crc <<= 1;
				crc ^= CRC16_CCITT;
			}
			else {
				crc <<= 1;
			}
		}
	}

	akdbgprt("[AK7755] %s CRC=%x\n",__FUNCTION__, crc);

	return crc;
}

static int ak7755_write_ram(
int	 nPCRam,  // 0 : PRAM, 1 : CRAM, 2:  OFREG. 3: ACRAM
u8 	*upRam,
int	 nWSize)
{
	int n, ret;
	int	wCRC;

	switch(nPCRam) {
		case RAMTYPE_PRAM:
			if (  nWSize > TOTAL_NUM_OF_PRAM_MAX ) {
				printk("%s: PRAM Write size is over! \n", __func__);
				return(-1);
			}
			break;
		case RAMTYPE_CRAM:
			if (  nWSize > TOTAL_NUM_OF_CRAM_MAX ) {
				printk("%s: CRAM Write size is over! \n", __func__);
				return(-1);
			}
			break;
		case RAMTYPE_OFREG:
			if (  nWSize > TOTAL_NUM_OF_OFREG_MAX ) {
				printk("%s: OFREG Write size is over! \n", __func__);
				return(-1);
			}
			break;
		case RAMTYPE_ACRAM:
			if (  nWSize > TOTAL_NUM_OF_ACRAM_MAX ) {
				printk("%s: ACRAM Write size is over! \n", __func__);
				return(-1);
			}
			break;
		default:
			break;
	}

	wCRC = calc_CRC(nWSize, upRam);

	n = MAX_LOOP_TIMES;
	do {
		ret = ak7755_ram_download(upRam, nWSize, wCRC);
		if ( ret >= 0 ) break;
		printk("%s: RAM Write Error! RAM No = %d \n", __func__, nPCRam);
		n--;
	} while ( n > 0 );

	if ( ret < 0 ) {
		printk("%s: RAM Write Error! RAM No = %d \n", __func__, nPCRam);
		return(-1);
	}

	return(0);

}

static int ak7755_firmware_write_ram(u16 mode, u16 cmd)
{
	int ret = 0;
	int nNumMode, nMaxLen;
	int nRamSize;
	u8  *ram_basic;
	const struct firmware *fw;
	u8  *fwdn;
	char szFileName[32];

	akdbgprt("[AK7755] %s mode=%d, cmd=%d\n",__FUNCTION__, mode, cmd);

	switch(mode) {
		case RAMTYPE_PRAM:
			nNumMode = sizeof(ak7755_firmware_pram) / sizeof(ak7755_firmware_pram[0]);
			break;
		case RAMTYPE_CRAM:
			nNumMode = sizeof(ak7755_firmware_cram) / sizeof(ak7755_firmware_cram[0]);
			break;
		case RAMTYPE_OFREG:
			nNumMode = sizeof(ak7755_firmware_ofreg) / sizeof(ak7755_firmware_ofreg[0]);
			break;
		case RAMTYPE_ACRAM:
			nNumMode = sizeof(ak7755_firmware_acram) / sizeof(ak7755_firmware_acram[0]);
			break;
		default:
			akdbgprt("[AK7755] %s mode Error=%d\n",__FUNCTION__, mode);
			return( -EINVAL);
	}

	if ( cmd == 0 ) return(0);

	if ( cmd >= nNumMode ) {
		pr_err("%s: invalid command %d\n", __func__, cmd);
		return( -EINVAL);
	}

	if ( cmd == 1 ) {
		switch(mode) {
			case RAMTYPE_PRAM:
				if (aec == 0){
					ram_basic = ak7755_pram_basic;
					nRamSize = sizeof(ak7755_pram_basic);
				}else{
					ram_basic = ak7755_pram_aec;
					nRamSize = sizeof(ak7755_pram_aec);
				}
				break;
			case RAMTYPE_CRAM:
				if (aec == 0){
					ram_basic = ak7755_cram_basic;
					nRamSize = sizeof(ak7755_cram_basic);
				}else{
					ram_basic = ak7755_cram_aec;
					nRamSize = sizeof(ak7755_cram_aec);
				}
				break;
			case RAMTYPE_OFREG:
				ram_basic = ak7755_ofreg_basic;
				nRamSize = sizeof(ak7755_ofreg_basic);
				break;
			case RAMTYPE_ACRAM:
				ram_basic = ak7755_acram_basic;
				nRamSize = sizeof(ak7755_acram_basic);
				break;
			default:
				return( -EINVAL);
		}
		ret = ak7755_write_ram((int)mode, ram_basic, nRamSize);
	} else {
		switch(mode) {
			case RAMTYPE_PRAM:
				sprintf(szFileName, "ak7755_pram_%s.bin", ak7755_firmware_pram[cmd]);
				nMaxLen = TOTAL_NUM_OF_PRAM_MAX;
				break;
			case RAMTYPE_CRAM:
				sprintf(szFileName, "ak7755_cram_%s.bin", ak7755_firmware_cram[cmd]);
				nMaxLen = TOTAL_NUM_OF_CRAM_MAX;
				break;
			case RAMTYPE_OFREG:
				sprintf(szFileName, "ak7755_ofreg_%s.bin", ak7755_firmware_cram[cmd]);
				nMaxLen = TOTAL_NUM_OF_OFREG_MAX;
				break;
			case RAMTYPE_ACRAM:
				sprintf(szFileName, "ak7755_acram_%s.bin", ak7755_firmware_cram[cmd]);
				nMaxLen = TOTAL_NUM_OF_ACRAM_MAX;
				break;
			default:
				return( -EINVAL);
		}

		if (ak7755_data->control_type == SND_SOC_SPI) {
			ret = request_firmware(&fw, szFileName, &(ak7755_data->spi->dev));
		}
		else {
			ret = request_firmware(&fw, szFileName, &(ak7755_data->i2c->dev));
		}
		if (ret) {
			akdbgprt("[AK7755] %s could not load firmware=%d\n", szFileName, ret);
			return -EINVAL;
		}

		//printk("[AK7755] %s name=%s size=%d\n",__FUNCTION__, szFileName, fw->size);
		if ( fw->size > nMaxLen ) {
			akdbgprt("[AK7755] %s RAM Size Error : %d\n",__FUNCTION__, fw->size);
			return -ENOMEM;
		}

		fwdn = kmalloc((unsigned long)fw->size, GFP_KERNEL);
		if (fwdn == NULL) {
			printk(KERN_ERR "failed to buffer vmalloc: %d\n", fw->size);
			return -ENOMEM;
		}

		memcpy((void *)fwdn, fw->data, fw->size);

		ret = ak7755_write_ram((int)mode, (u8 *)fwdn, (fw->size));
		//printk("ak7755 download ram is ok, ret = %d\n", ret);

		kfree(fwdn);
	}


	return ret;
}

static int ak7755_write_cram(
int addr,
int len,
unsigned char *cram_data)
{
	int i, n, ret;
	int nDSPRun;
	unsigned char tx[51];

	akdbgprt("[AK7755] %s addr=%d, len=%d\n",__FUNCTION__, addr, len);

	if ( len > 48 ) {
		akdbgprt("[AK7755] %s Length over!\n",__FUNCTION__);
		return(-1);
	}

	nDSPRun = snd_soc_read(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING);

	if ( nDSPRun & 0x4 ) {
		tx[0] = 0x80 + (unsigned char)((len / 3) - 1);
		tx[1] = (unsigned char)(0xFF & (addr >> 8));
		tx[2] = (unsigned char)(0xFF & addr);
	}
	else {
		ak7755_set_status(DOWNLOAD);
		tx[0] = 0xB4;
		tx[1] = (unsigned char)(0xFF & (addr >> 8));
		tx[2] = (unsigned char)(0xFF & addr);
	}

	n = 3;
	for ( i = 0 ; i < len; i ++ ) {
		tx[n++] = cram_data[i];
	}

	ret = ak7755_writes(tx, n);

	if ( nDSPRun & 0x4 ) {
		tx[0] = 0xA4;
		tx[1] = 0;
		tx[2] = 0;
		ret = ak7755_writes(tx, 3);
	}
	else {
		snd_soc_write(ak7755_data->codec, AK7755_CF_RESET_POWER_SETTING, nDSPRun);
	}


	return ret;

}

static unsigned long ak7755_readMIR(
int nMIRNo)
{
	unsigned char tx[1];
	unsigned char rx[4];
	unsigned long dwMIRData;

	if ( ( nMIRNo < 1 )  ||  ( nMIRNo > 4 ) ) return(-1);

	tx[0] = (unsigned char)(0x74 + (2 * nMIRNo));

	ak7755_reads(tx, 1, rx, 4);

	dwMIRData = ((0xFF & (unsigned long)rx[0]) << 20) + ((0xFF & (unsigned long)rx[1]) << 12) +
                       ((0xFF & (unsigned long)rx[2]) << 4) + ((0xFF & (unsigned long)rx[3]) >> 4);


	return(dwMIRData);

}

static int ak7755_reg_cmd(REG_CMD *reg_param, int len)
{
	int i;
	int rc;
	int  addr, value;

	akdbgprt("*****[AK7755] %s len = %d\n",__FUNCTION__, len);

	rc = 0;
	for (i = 0; i < len; i++) {
		addr = (int)(reg_param[i].addr);
		value = (int)(reg_param[i].data);

		if ( addr != 0xFF ) {
			rc = snd_soc_write(ak7755_data->codec, addr, value);
			if (rc < 0) {
				break;
			}
		}
		else {
			mdelay(value);
		}
	}

	if (rc != 0 ) rc = 0;

	return(rc);

}

#ifdef AK7755_IO_CONTROL
static long ak7755_ioctl(struct file *file, unsigned int cmd, unsigned long args)
{
	struct ak7755_priv *ak7755 = (struct ak7755_priv*)file->private_data;
	struct ak7755_wreg_handle ak7755_wreg;
	struct ak7755_wcram_handle  ak7755_wcram;
	struct ak7755_loadram_handle  ak7755_ram;
	void __user *data = (void __user *)args;
	int *val = (int *)args;
	int i;
	unsigned long dwMIRData;
	int ret = 0;
	REG_CMD      regcmd[MAX_WREG];
	unsigned char cram_data[MAX_WCRAM];

	akdbgprt("*****[AK7755] %s cmd, val=%x, %x\n",__FUNCTION__, cmd, val[0]);

	switch(cmd) {
	case AK7755_IOCTL_WRITECRAM:
		if (copy_from_user(&ak7755_wcram, data, sizeof(struct ak7755_wcram_handle)))
			return -EFAULT;
		if ( (  ak7755_wcram.len % 3 ) != 0 ) {
			printk(KERN_ERR "[AK7755] %s CRAM len error\n",__FUNCTION__);
			return -EFAULT;
		}
		if ( ( ak7755_wcram.len < 3 ) || ( ak7755_wcram.len > MAX_WCRAM ) ) {
			printk(KERN_ERR "[AK7755] %s CRAM len error2\n",__FUNCTION__);
			return -EFAULT;
		}
		for ( i = 0 ; i < ak7755_wcram.len ; i ++ ) {
			cram_data[i] = ak7755_wcram.cram[i];
		}
		ret = ak7755_write_cram(ak7755_wcram.addr, ak7755_wcram.len, cram_data);
		break;
	case AK7755_IOCTL_WRITEREG:
		if (copy_from_user(&ak7755_wreg, data, sizeof(struct ak7755_wreg_handle)))
			return -EFAULT;
		if ( ( ak7755_wreg.len < 1 ) || ( ak7755_wreg.len > MAX_WREG ) ) {
			printk(KERN_ERR "MAXREG ERROR %d\n", ak7755_wreg.len );
			return -EFAULT;
		}
		for ( i = 0 ; i < ak7755_wreg.len; i ++ ) {
			regcmd[i].addr = ak7755_wreg.regcmd[i].addr;
			regcmd[i].data = ak7755_wreg.regcmd[i].data;
		}
		ak7755_reg_cmd(regcmd, ak7755_wreg.len);
		break;
	case AK7755_IOCTL_LOADRAM:
		if (copy_from_user(&ak7755_ram, data, sizeof(struct ak7755_loadram_handle)))
			return -EFAULT;
		ak7755_firmware_write_ram(ak7755_ram.ramtype, ak7755_ram.mode);
		break;
	case AK7755_IOCTL_SETSTATUS:
		ret = ak7755_set_status(val[0]);
		if (ret < 0) {
			printk(KERN_ERR "ak7755: set_status error: \n");
			return ret;
		}
		break;
	case AK7755_IOCTL_GETSTATUS:
		ret = copy_to_user(data, (const void*)&ak7755->status,
							(unsigned long)sizeof(unsigned int));
		if (ret < 0) {
			printk(KERN_ERR "ak7755: get status error\n");
			return -EFAULT;
		}
		break;
	case AK7755_IOCTL_SETMIR:
		ak7755->MIRNo = val[0];
		if (ret < 0) {
			printk(KERN_ERR "ak7755: set MIR error\n");
			return -EFAULT;
		}
		break;
	case AK7755_IOCTL_GETMIR:
		dwMIRData = ak7755_readMIR(ak7755->MIRNo);
		ret = copy_to_user(data, (const void*)&dwMIRData,
							(unsigned long)sizeof(unsigned int));
		if (ret < 0) {
			printk(KERN_ERR "ak7755: get status error\n");
			return -EFAULT;
		}
		break;

	default:
		printk(KERN_ERR "Unknown command required: %d\n", cmd);
		return -EINVAL;
	}

	return ret;
}

static int init_ak7755_pd(struct ak7755_priv *data)
{
	struct _ak7755_pd_handler *ak7755 = &ak7755_pd_handler;

	if (data == NULL)
		return -EFAULT;

	mutex_init(&ak7755->lock);

	mutex_lock(&ak7755->lock);
	ak7755->data = data;
	mutex_unlock(&ak7755->lock);

	printk("data:%p, ak7755->data:%p\n", data, ak7755->data);

	return 0;
}

static struct ak7755_priv* get_ak7755_pd(void)
{
	struct _ak7755_pd_handler *ak7755 = &ak7755_pd_handler;

	if (ak7755->data == NULL)
		return NULL;

	mutex_lock(&ak7755->lock);
	ak7755->ref_count++;
	mutex_unlock(&ak7755->lock);

	return ak7755->data;
}

static int rel_ak7755_pd(struct ak7755_priv *data)
{
	struct _ak7755_pd_handler *ak7755 = &ak7755_pd_handler;

	if (ak7755->data == NULL)
		return -EFAULT;

	mutex_lock(&ak7755->lock);
	ak7755->ref_count--;
	mutex_unlock(&ak7755->lock);

	data = NULL;

	return 0;
}

/* AK7755 Misc driver interfaces */
static int ak7755_open(struct inode *inode, struct file *file)
{
	struct ak7755_priv *ak7755;

	ak7755 = get_ak7755_pd();
	file->private_data = ak7755;

	return 0;
}

static int ak7755_close(struct inode *inode, struct file *file)
{
	struct ak7755_priv *ak7755 = (struct ak7755_priv*)file->private_data;

	rel_ak7755_pd(ak7755);

	return 0;
}

static const struct file_operations ak7755_fops = {
	.owner = THIS_MODULE,
	.open = ak7755_open,
	.release = ak7755_close,
	.unlocked_ioctl = ak7755_ioctl,
};

static struct miscdevice ak7755_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ak7755-dsp",
	.fops = &ak7755_fops,
};

#endif

/*********************************/

// * for AK7755
static int ak7755_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);
	int 	ret = 0;

	if(ak7755->amp_gpio > 0) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				gpio_direction_output(ak7755->amp_gpio, ak7755->amp_active);
			}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
				gpio_direction_output(ak7755->amp_gpio, !ak7755->amp_active);
			}
			break;
		default:
			break;
		}

	}

	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	return ret;
}

static int ak7755_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	akdbgprt("\t[AK7755] %s(%d) level : %d\n",__FUNCTION__,__LINE__, level);

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		ak7755_set_status(RUN);
		break;
	case SND_SOC_BIAS_STANDBY:
		ak7755_set_status(STANDBY);
		break;
	case SND_SOC_BIAS_OFF:
		ak7755_set_status(POWERDOWN);
		break;
	}
	codec->dapm.bias_level = level;

	return 0;
}

static int ak7755_set_dai_mute2(struct snd_soc_codec *codec, int mute)
{
	int ret = 0;
	//int ndt;

	//akdbgprt("\t[AK7755] %s mute[%s]\n",__FUNCTION__, mute ? "ON":"OFF");

#if 0
	if (mute) {	//SMUTE: 1 , MUTE
		ret = snd_soc_update_bits(codec, AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 0xE0, 0xE0);
	}
	else {
		ak7755_set_status(RUN);
		// SMUTE: 0 ,NORMAL operation
		ret = snd_soc_update_bits(codec, AK7755_DA_MUTE_ADRC_ZEROCROSS_SET, 0xE0, 0x00);
	}

	ndt = 1021000 / ak7755_data->fs;
	mdelay(ndt);
#endif
	return ret;
}

static int ak7755_set_dai_mute(struct snd_soc_dai *dai, int mute)
{
    struct snd_soc_codec *codec = dai->codec;

	ak7755_set_dai_mute2(codec, mute);

	return 0;
}

#define AK7755_RATES		(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
				SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
				SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
				SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |\
			    SNDRV_PCM_RATE_96000)

#define AK7755_FORMATS		(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops ak7755_dai_ops = {
	.hw_params	= ak7755_hw_params,
	.set_sysclk	= ak7755_set_dai_sysclk,
	.set_fmt	= ak7755_set_dai_fmt,
	.trigger = ak7755_trigger,
	.digital_mute = ak7755_set_dai_mute,
};

struct snd_soc_dai_driver ak7755_dai[] = {
	{
		.name = "ak7755-AIF1",
		.playback = {
		       .stream_name = "Playback",
		       .channels_min = 1,
		       .channels_max = 2,
		       .rates = AK7755_RATES,
		       .formats = AK7755_FORMATS,
		},
		.capture = {
		       .stream_name = "Capture",
		       .channels_min = 1,
		       .channels_max = 4,
		       .rates = AK7755_RATES,
		       .formats = AK7755_FORMATS,
		},
		.ops = &ak7755_dai_ops,
	},
};

static int ak7755_init_reg(struct snd_soc_codec *codec)
{
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);   // '15/06/15

	akdbgprt("\t[AK7755 bias] %s(%d)\n",__FUNCTION__,__LINE__);


	if ( ak7755->power_gpio > 0 ) {
		gpio_direction_output(ak7755->power_gpio, ak7755->power_active);
	}

	if ( ak7755->pdn_gpio > 0 ) {
		gpio_direction_output(ak7755->pdn_gpio, ak7755->pdn_active);
		msleep(10);
		gpio_direction_output(ak7755->pdn_gpio, !ak7755->pdn_active);
	}


	ak7755_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	snd_soc_update_bits(codec, AK7755_D4_LO1_LO2_VOLUME_SETTING, 0xFF, 0xFF);			//LOVOL1,2=F(0dB)
	snd_soc_update_bits(codec, AK7755_D3_LIN_LO3_VOLUME_SETTING, 0x0F, 0x0F);			//LOVOL3=F(0dB)
	snd_soc_update_bits(codec, AK7755_D2_MIC_GAIN_SETTING, 0xFF, 0xCC);
	snd_soc_update_bits(codec, AK7755_D0_FUNCTION_SETTING, 0x40, 0x40);				//CRCE=1(CRC Enable)
	snd_soc_update_bits(codec, AK7755_C2_SERIAL_DATA_FORMAT, 0x40, AK7755_BCKP_BIT);	//BCKP bit
	snd_soc_update_bits(codec, AK7755_D0_FUNCTION_SETTING, 0x10, AK7755_SOCFG_BIT);	//SOFG bit
#ifdef DIGITAL_MIC		//Analog MIC Use
	snd_soc_update_bits(codec, AK7755_DE_DMIC_IF_SETTING, 0x90, 0x90);					//DMIC1=DMIC2=1(Digital MIC Use)
	snd_soc_update_bits(codec, AK7755_DE_DMIC_IF_SETTING, 0x40, AK7755_DMCLKP1_BIT);	//DMCLKP1 bit
	snd_soc_update_bits(codec, AK7755_DE_DMIC_IF_SETTING, 0x08, AK7755_DMCLKP2_BIT);	//DMCLKP2 bit
	snd_soc_update_bits(codec, AK7755_C0_CLOCK_SETTING1, 0x08, 0x00);
#else
	snd_soc_update_bits(codec, AK7755_C0_CLOCK_SETTING1, 0x08, 0x08);		//AINE=1(Analog MIC Use)
#endif
	snd_soc_write(codec, 0xCA, 0x01);
	snd_soc_write(codec, 0xDA, 0x10);
	snd_soc_write(codec, 0xCD, 0xC0);
	snd_soc_write(codec, 0xE6, 0x01);
	snd_soc_write(codec, 0xEA, 0x80);

	if(aec == 1) {
		/*Select DAC MUX as DSP DOUT4*/
		snd_soc_update_bits(codec, AK7755_C8_DAC_IN_SETTING, 0xC0, 0x00);
		/*Lineout1 power on*/
		snd_soc_update_bits(codec, AK7755_CE_POWER_MANAGEMENT, 0xC7, 0xC7);

		/*Select SDOUT1 MUX as DOUT1 Of DSP*/
		snd_soc_update_bits(codec, AK7755_CC_VOLUME_TRANSITION, 0x07, 0x00);
	} else {
		/*Select DAC MUX as SDIN1*/
		snd_soc_update_bits(codec, AK7755_C8_DAC_IN_SETTING, 0xC0, 0xC0);
		/*Lineout1 power on*/
		snd_soc_update_bits(codec, AK7755_CE_POWER_MANAGEMENT, 0xC7, 0xC7);

		/*Select SDOUT1 MUX as SDOUTAD*/
		snd_soc_update_bits(codec, AK7755_CC_VOLUME_TRANSITION, 0x07, 0x03);
	}

	return 0;
}

static int ak7755_parse_dt(struct ak7755_priv *ak7755)
{
	struct device *dev;
	struct device_node *np;
	enum of_gpio_flags flags;
	int ret = 0;

	if (ak7755->control_type == SND_SOC_SPI) {
		dev = &(ak7755->spi->dev);
	}
	else {
		dev = &(ak7755->i2c->dev);
	}

	np = dev->of_node;

	if (!np)
		return -1;

	ak7755->pdn_gpio = of_get_named_gpio_flags(np, "ak7755,pdn-gpio", 0, &flags);
	ak7755->pdn_active = !!(flags & OF_GPIO_ACTIVE_LOW);
	if(ak7755->pdn_gpio < 0 || !gpio_is_valid(ak7755->pdn_gpio)) {
		dev_err(dev, "ak7755 pdn pin(%u) is invalid\n", ret);
		ak7755->pdn_gpio = -1;
		return ret;
	}
	ret = devm_gpio_request(dev, ak7755->pdn_gpio, "ak7755 pdn_gpio");
	if ( ret < 0 )
		dev_err(dev, "Failed to request pdn_pin: %d\n", ret);


	ak7755->power_gpio = of_get_named_gpio_flags(np, "ak7755,pwr-gpio", 0, &flags);
	ak7755->power_active = !!(flags & OF_GPIO_ACTIVE_LOW);
	if (ak7755->power_gpio < 0 || !gpio_is_valid(ak7755->power_gpio)) {
		ak7755->power_gpio = -1;
		dev_err(dev, "ak7755 power pin(%u) is invalid\n", ret);
		return ret;
	}
	ret = devm_gpio_request(dev, ak7755->power_gpio, "ak7755 power_gpio");
	if ( ret < 0 )
		dev_err(dev, "Failed to request power_pin: %d\n", ret);


	ak7755->amp_gpio = of_get_named_gpio_flags(np, "ak7755,amp-gpio", 0, &flags);
	ak7755->amp_active = !!(flags & OF_GPIO_ACTIVE_LOW);
	if (ak7755->amp_gpio < 0 || !gpio_is_valid(ak7755->amp_gpio)) {
		ak7755->amp_gpio = -1;
		dev_err(dev, "ak7755 amplifier pin(%u) is invalid\n", ret);
		return ret;
	}
	ret = devm_gpio_request(dev, ak7755->amp_gpio, "ak7755 amp_gpio");
	if ( ret < 0 )
		dev_err(dev, "Failed to request amp_pin: %d\n", ret);

	return 0;
}

static int ak7755_set_reg_table(void)
{
	int i;
	int n;

	n = 0;
	do {
		ak7755_reg[n] = 0;
		ak7755_access_masks[n].readable = 0;
		ak7755_access_masks[n].writable = 0;
		n ++;
	} while ( n < AK7755_C0_CLOCK_SETTING1 );

	i = 0;
	do {
		ak7755_reg[n] = ak7755_reg2[i] ;
		ak7755_access_masks[n].readable = ak7755_access_masks2[i].readable;
		ak7755_access_masks[n].writable = ak7755_access_masks2[i].writable;
		i++;
		n++;
	} while ( n < AK7755_MAX_REGISTERS);

	return(0);

}

static int ak7755_probe(struct snd_soc_codec *codec)
{
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;
	int i;

	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);
	ret = snd_soc_codec_set_cache_io(codec, 8, 8, ak7755->control_type);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

#ifdef AK7755_CONTIF_DEBUG
	codec->read = ak7755_reg_read;
	codec->write = ak7755_reg_write;

#endif

	akdbgprt("\t[AK7755] %s(%d) ak7755=%x\n",__FUNCTION__,__LINE__, (int)ak7755);
	ak7755->codec = codec;
	ret = ak7755_parse_dt(ak7755);
	if ( ret != 0 ) {
		dev_err(codec->dev, "Failed to parse devict tree: %d\n", ret);
		return ret;
	}

	ak7755_set_reg_table();
	ak7755_init_reg(codec);
	akdbgprt("\t[AK7755 Effect] %s(%d)\n",__FUNCTION__,__LINE__);

	ak7755->fs = 48000;
	ak7755->rclk = 0;
	ak7755->lign = 7;	//lign:7 = LIGN:0(0dB)
	ak7755->selmix = 0;	//SDOUTAD(L,R)
	ak7755->MIRNo = 1;
	ak7755->status = POWERDOWN;


	ak7755->DSPPramMode = 0;
	ak7755->DSPCramMode = 0;
	ak7755->DSPOfregMode = 0;
	ak7755->DSPAcramMode = 0;

	for ( i = 0 ; i < 5 ; i ++ ) ak7755->EQLevel[i] = 24;
	ak7755->HPFfc[0] = 0;
	ak7755->HPFfc[1] = 0;
 	ak7755->LimRel = 2;
	ak7755->LimVol = 0;

	ak7755->bickfs = (AK7755_AUDIO_IF_MODE >> 4);

	akdbgprt("\t[AK7755 init_ak7755_pd] %s(%d)\n",__FUNCTION__,__LINE__);

#ifdef AK7755_IO_CONTROL
	init_ak7755_pd(ak7755);
#endif
	return ret;
}

static int ak7755_remove(struct snd_soc_codec *codec)
{
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);

	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	ak7755_set_bias_level(codec, SND_SOC_BIAS_OFF);

	if ( ak7755->pdn_gpio > 0 ) {
		gpio_direction_output(ak7755->pdn_gpio, ak7755->pdn_active);
		gpio_free(ak7755->pdn_gpio);
	}

	return 0;
}

static int ak7755_suspend(struct snd_soc_codec *codec)
{
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);
	int i = 0;

	for(i = 0; i < AK7755_MAX_REGNUM; i++) {
		ak7755_data->reg_cache[i] = snd_soc_read(codec, i + 0xc0);
	}
	ak7755_data->reg_cache[1] &= 0xfe;		//Make sure  CKRESETN bit is reset

	ak7755_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	if ( ak7755->amp_active > 0 ) {
		gpio_direction_output(ak7755->amp_gpio, !ak7755->amp_active);
	}
	if ( ak7755->pdn_gpio > 0 ) {
		gpio_direction_output(ak7755->pdn_gpio, ak7755->pdn_active);
	}
	if ( ak7755->power_gpio > 0 ) {
		gpio_direction_output(ak7755->power_gpio, !ak7755->power_active);
	}

	return 0;
}

static int ak7755_resume(struct snd_soc_codec *codec)
{
	struct ak7755_priv *ak7755 = snd_soc_codec_get_drvdata(codec);
	int i = 0;

	if ( ak7755->pdn_gpio > 0 ) {
		gpio_direction_output(ak7755->pdn_gpio, ak7755->pdn_active);
	}

	if ( ak7755->power_gpio > 0 ) {
		gpio_direction_output(ak7755->power_gpio, ak7755->power_active);
		mdelay(10);
	}

	if ( ak7755->pdn_gpio > 0 ) {
		gpio_direction_output(ak7755->pdn_gpio, !ak7755->pdn_active);
		mdelay(2);
	}

	snd_soc_write(codec,AK7755_C0_CLOCK_SETTING1,0x08);
	snd_soc_write(codec,AK7755_C1_CLOCK_SETTING2,0x00);
	for(i = 0; i < AK7755_MAX_REGNUM; i++) {
		snd_soc_write(codec, i+0xc0, ak7755_data->reg_cache[i]);
	}

	snd_soc_update_bits(codec, AK7755_C1_CLOCK_SETTING2, 0x01, 0x01);
	snd_soc_write(codec,AK7755_CF_RESET_POWER_SETTING,0x01);
	snd_soc_write(codec,AK7755_DA_MUTE_ADRC_ZEROCROSS_SET,0x10);
	mdelay(12);

	ak7755_firmware_write_ram(RAMTYPE_PRAM, ak7755_data->DSPPramMode);
	ak7755_firmware_write_ram(RAMTYPE_CRAM, ak7755_data->DSPCramMode);

	//snd_soc_cache_sync(codec);
	snd_soc_write(codec, 0xE6, 0x01);
	snd_soc_write(codec, 0xEA, 0x80);

	snd_soc_write(codec,AK7755_DA_MUTE_ADRC_ZEROCROSS_SET,0x00);

	ak7755_set_bias_level(codec, SND_SOC_BIAS_ON);

	return 0;
}


struct snd_soc_codec_driver soc_codec_dev_ak7755 = {
	.probe = ak7755_probe,
	.remove = ak7755_remove,
	.suspend =	ak7755_suspend,
	.resume =	ak7755_resume,

	//.idle_bias_off = false,
	.set_bias_level = ak7755_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(ak7755_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = ak7755_reg,
	.readable_register = ak7755_readable,
	.volatile_register = ak7755_volatile,

	.controls = ak7755_snd_controls,
	.num_controls = ARRAY_SIZE(ak7755_snd_controls),
	.dapm_widgets = ak7755_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ak7755_dapm_widgets),
	.dapm_routes = ak7755_intercon,
	.num_dapm_routes = ARRAY_SIZE(ak7755_intercon),
};
EXPORT_SYMBOL_GPL(soc_codec_dev_ak7755);

#ifdef CONFIG_OF
static struct of_device_id ak7755_if_dt_ids[] = {
	{ .compatible = "akm,ak7755"},
	{ }
};
MODULE_DEVICE_TABLE(of, ak7755_if_dt_ids);
#endif

#ifdef AK7755_I2C_IF
static int ak7755_i2c_probe(struct i2c_client *i2c,
                            const struct i2c_device_id *id)
{
	struct ak7755_priv *ak7755;
	int ret=0;

	akdbgprt("\t[AK7755] %s(%d)\n",__FUNCTION__,__LINE__);

	ak7755 = kzalloc(sizeof(struct ak7755_priv), GFP_KERNEL);
	if (ak7755 == NULL) return -ENOMEM;

	i2c_set_clientdata(i2c, ak7755);

	ak7755->control_type = SND_SOC_I2C;
	ak7755->i2c = i2c;

	ak7755_data = ak7755;

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_ak7755, &ak7755_dai[0], ARRAY_SIZE(ak7755_dai));
	if (ret < 0){
		kfree(ak7755);
		akdbgprt("\t[AK7755 Error!] %s(%d)\n",__FUNCTION__,__LINE__);
	}
	return ret;
}

static int ak7755_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id ak7755_i2c_id[] = {
	{ "ak7755", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ak7755_i2c_id);

static struct i2c_driver ak7755_i2c_driver = {
	.driver = {
		.name = "ak7755",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ak7755_if_dt_ids),
#endif
	},
	.probe = ak7755_i2c_probe,
	.remove = ak7755_i2c_remove,
	.id_table = ak7755_i2c_id,
};

#else

static int ak7755_spi_probe(struct spi_device *spi)
{
	struct ak7755_priv *ak7755;
	int	ret;

	akdbgprt("\t[AK7755] %s spi=%x\n",__FUNCTION__, (int)spi);

	ak7755 = kzalloc(sizeof(struct ak7755_priv), GFP_KERNEL);
	if (!ak7755)
		return -ENOMEM;

	ak7755->control_type = SND_SOC_SPI;
	spi->bits_per_word = 8; // bits
	spi->max_speed_hz = 7000000;
	spi->mode = SPI_MODE_3;

	akdbgprt("\t[AK7755] %s spi=%x, spi->chip_select = %d\n, cs_gpio = %d",__FUNCTION__, \
		(int)spi, spi->chip_select, spi->cs_gpio);

	ak7755->spi = spi;
	ak7755_data = ak7755;

	if ( 1 == fast_boot ) {
		u8 reg[2],rx[1];
		akdbgprt("Start loading AEC Firmware....\n");
		reg[0] = AK7755_C1_CLOCK_SETTING2 & 0x7F;
		ak7755_reads(reg,1,rx,1);
		if (1 != (rx[0] & 0x1)) {
			printk( "Fast audio setup error for ak7755: 0x%02X\n", rx[0]);
			return -1;
		}
		reg[0] = AK7755_CF_RESET_POWER_SETTING;
		reg[1] = 0x01;
		ak7755_writes(reg,2);

		msleep(1);
		ret = ak7755_firmware_write_ram(RAMTYPE_PRAM, 1);
		if ( ret < 0 ) {
			printk( "Failed to RAMTYPE_PRAM %d\n", ret);
			return ret;
		}
		ret = ak7755_firmware_write_ram(RAMTYPE_CRAM,1);
		if ( ret < 0 ) {
			printk( "Failed to RAMTYPE_CRAM: %d\n", ret);
			return ret;
		}
		reg[0] = AK7755_CF_RESET_POWER_SETTING;
		reg[1] = 0x0c;
		ak7755_writes(reg,2);
		akdbgprt("Loading AEC Firmware done\n");
		reg[0] = AK7755_CC_VOLUME_TRANSITION;	   //change to DSP output
		reg[1] = 0x00;
		ak7755_writes(reg,2);
		reg[0] = AK7755_D2_MIC_GAIN_SETTING;	   //change Mic Gain to 6dB
		reg[1] = 0x03;
		ak7755_writes(reg,2);
		reg[0] = AK7755_C8_DAC_IN_SETTING;		 //change DAC out to DSP
		reg[1] = 0x00;

#ifdef AK7755_DEBUG
		u32 i;
		for(i = 0x40; i <= 0x5e; i++) {
			reg[0] = i ;
			ak7755_reads(reg,1,rx,1);
			printk("0x%x: 0x%02x\n", i|0xc0, rx[0]);
  }
#endif
		return 0;
	} else {
		spi_set_drvdata(spi, ak7755);
	}

	ret = snd_soc_register_codec(&spi->dev,
			&soc_codec_dev_ak7755,  &ak7755_dai[0], ARRAY_SIZE(ak7755_dai));
	if (ret < 0)
		kfree(ak7755);

	ak7755_data = ak7755;

	return 0;
}

static int ak7755_spi_remove(struct spi_device *spi)
{
	if ( ak7755_data->amp_active > 0 ) {
		gpio_direction_output(ak7755_data->amp_gpio, !ak7755_data->amp_active);
		devm_gpio_free(&spi->dev,ak7755_data->amp_active);
	}
	if ( ak7755_data->pdn_gpio > 0 ) {
		gpio_direction_output(ak7755_data->pdn_gpio, ak7755_data->pdn_active);
		devm_gpio_free(&spi->dev,ak7755_data->pdn_gpio);
	}
	if ( ak7755_data->power_gpio > 0 ) {
		gpio_direction_output(ak7755_data->power_gpio, !ak7755_data->power_active);
		devm_gpio_free(&spi->dev,ak7755_data->power_gpio);
		akdbgprt("\t[AK7755] %s(%d) Release power_gpio ->>> %d\n",__FUNCTION__,__LINE__,ak7755_data->power_gpio);
	}
	snd_soc_unregister_codec(&spi->dev);
	kfree(spi_get_drvdata(spi));
	return 0;
}

static struct spi_driver ak7755_spi_driver = {
	.driver = {
		.name = "ak7755",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ak7755_if_dt_ids),
#endif
	},
	.probe = ak7755_spi_probe,
	.remove = ak7755_spi_remove,
};
#endif

static int __init ak7755_modinit(void)
{
	int ret = 0;

	akdbgprt("\t[AK7755] %s(%d)\n", __FUNCTION__,__LINE__);

#ifdef AK7755_I2C_IF
	ret = i2c_add_driver(&ak7755_i2c_driver);
	if ( ret != 0 ) {
		printk(KERN_ERR "Failed to register AK7755 I2C driver: %d\n", ret);

	}
#else
	ret = spi_register_driver(&ak7755_spi_driver);
	if ( ret != 0 ) {
		printk(KERN_ERR "Failed to register AK7755 SPI driver: %d\n",  ret);

	}
#endif

#ifdef AK7755_IO_CONTROL
	ret = misc_register(&ak7755_misc);
	if (ret < 0) {
		printk(KERN_ERR "Failed to register AK7755 MISC driver: %d\n",  ret);
	}
#endif

	return ret;
}

module_init(ak7755_modinit);

static void __exit ak7755_exit(void)
{
#ifdef AK7755_I2C_IF
	i2c_del_driver(&ak7755_i2c_driver);
#else
	spi_unregister_driver(&ak7755_spi_driver);
#endif

#ifdef AK7755_IO_CONTROL
	misc_deregister(&ak7755_misc);
#endif

}
module_exit(ak7755_exit);

MODULE_AUTHOR("Junichi Wakasugi <wakasugi.jb@om.asahi-kasei.co.jp>");
MODULE_DESCRIPTION("ASoC ak7755 codec driver");
MODULE_LICENSE("GPL");
