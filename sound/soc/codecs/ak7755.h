/*
 * ak7755.h  --  audio driver for ak7755
 *
 * Copyright (C) 2014 Asahi Kasei Microdevices Corporation
 *  Author                Date        Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      14/04/22	    1.0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef _AK7755_H
#define _AK7755_H

//#define AK7755_I2C_IF
#define AK7755_IO_CONTROL


/* AK7755_C1_CLOCK_SETTING2 (0xC1)  Fields */
#define AK7755_AIF_BICK32		(2 << 4)
#define AK7755_AIF_BICK48		(1 << 4)
#define AK7755_AIF_BICK64		(0 << 4)
#define AK7755_AIF_TDM			(3 << 4)	//TDM256 bit is set "1" at initialization.

/* TDMMODE Input Source */
#define AK7755_TDM_DSP				(0 << 2)
#define AK7755_TDM_DSP_AD1			(1 << 2)
#define AK7755_TDM_DSP_AD1_AD2		(2 << 2)

/* User Setting */
//#define DIGITAL_MIC
//#define CLOCK_MODE_BICK
//#define CLOCK_MODE_18_432
#define AK7755_AUDIO_IF_MODE		AK7755_AIF_BICK32	//32fs, 48fs, 64fs, 256fs(TDM)
#define AK7755_TDM_INPUT_SOURCE		AK7755_TDM_DSP		//Effective only in TDM mode
#define AK7755_BCKP_BIT				(0 << 6)	//BICK Edge Setting
#define AK7755_SOCFG_BIT  			(0 << 4)	//SO pin Hi-z Setting
#define AK7755_DMCLKP1_BIT			(0 << 6)	//DigitalMIC 1 Channnel Setting
#define AK7755_DMCLKP2_BIT			(0 << 3)	//DigitalMIC 1 Channnel Setting
/* User Setting */

#define AK7755_C0_CLOCK_SETTING1			0xC0
#define AK7755_C1_CLOCK_SETTING2			0xC1
#define AK7755_C2_SERIAL_DATA_FORMAT		0xC2
#define AK7755_C3_DELAY_RAM_DSP_IO			0xC3
#define AK7755_C4_DATARAM_CRAM_SETTING		0xC4
#define AK7755_C5_ACCELARETOR_SETTING		0xC5
#define AK7755_C6_DAC_DEM_SETTING			0xC6
#define AK7755_C7_DSP_OUT_SETTING			0xC7
#define AK7755_C8_DAC_IN_SETTING			0xC8
#define AK7755_C9_ANALOG_IO_SETTING			0xC9
#define AK7755_CA_CLK_SDOUT_SETTING			0xCA
#define AK7755_CB_TEST_SETTING				0xCB
#define AK7755_CC_VOLUME_TRANSITION			0xCC
#define AK7755_CD_STO_DLS_SETTING			0xCD
#define AK7755_CE_POWER_MANAGEMENT			0xCE
#define AK7755_CF_RESET_POWER_SETTING		0xCF
#define AK7755_D0_FUNCTION_SETTING			0xD0
#define AK7755_D1_DSPMCLK_SETTING			0xD1
#define AK7755_D2_MIC_GAIN_SETTING			0xD2
#define AK7755_D3_LIN_LO3_VOLUME_SETTING	0xD3
#define AK7755_D4_LO1_LO2_VOLUME_SETTING	0xD4
#define AK7755_D5_ADC_DVOLUME_SETTING1		0xD5
#define AK7755_D6_ADC_DVOLUME_SETTING2		0xD6
#define AK7755_D7_ADC2_DVOLUME_SETTING1		0xD7
#define AK7755_D8_DAC_DVOLUME_SETTING1		0xD8
#define AK7755_D9_DAC_DVOLUME_SETTING2		0xD9
#define AK7755_DA_MUTE_ADRC_ZEROCROSS_SET	0xDA
#define AK7755_DB_ADRC_MIC_GAIN_READ		0xDB
#define AK7755_DC_TEST_SETTING				0xDC
#define AK7755_DD_ADC2_DVOLUME_SETTING2		0xDD
#define AK7755_DE_DMIC_IF_SETTING			0xDE

#define AK7755_MAX_REGISTERS	(AK7755_DE_DMIC_IF_SETTING + 1)
#define AK7755_MAX_REGNUM		(31)

/* Bitfield Definitions */

/* AK7755_C0_CLOCK_SETTING1 (0xC0) Fields */
#define AK7755_FS				0x07
#define AK7755_FS_8KHZ			(0x00 << 0)
#define AK7755_FS_12KHZ			(0x01 << 0)
#define AK7755_FS_16KHZ			(0x02 << 0)
#define AK7755_FS_24KHZ			(0x03 << 0)
#define AK7755_FS_32KHZ			(0x04 << 0)
#define AK7755_FS_48KHZ			(0x05 << 0)
#define AK7755_FS_96KHZ			(0x06 << 0)

#define AK7755_M_S				0x30		//CKM1-0 (CKM2 bit is not use)
#define AK7755_M_S_0			(0 << 4)	//Master, XTI=12.288MHz
#define AK7755_M_S_1			(1 << 4)	//Master, XTI=18.432MHz
#define AK7755_M_S_2			(2 << 4)	//Slave, XTI=12.288MHz
#define AK7755_M_S_3			(3 << 4)	//Slave, BICK

/* AK7755_C2_SERIAL_DATA_FORMAT (0xC2) Fields */
/* LRCK I/F Format */
#define AK7755_LRIF					0x30
#define AK7755_LRIF_MSB_MODE		(0 << 4)
#define AK7755_LRIF_I2S_MODE		(1 << 4)
#define AK7755_LRIF_PCM_SHORT_MODE	(2 << 4)
#define AK7755_LRIF_PCM_LONG_MODE	(3 << 4)
/* Input Format is set as "MSB(24- bit)" by following register.
   CONT03(DIF2,DOF2), CONT06(DIFDA, DIF1), CONT07(DOF4,DOF3,DOF1) */

/* AK7755_CA_CLK_SDOUT_SETTING (0xCA) Fields */
#define AK7755_BICK_LRCK			(3 << 5)	//BICKOE, LRCKOE



#define MAX_LOOP_TIMES		3

#define CRC_COMMAND_READ_RESULT			0x72
#define	TOTAL_NUM_OF_PRAM_MAX		20483
#define	TOTAL_NUM_OF_CRAM_MAX		6147
#define	TOTAL_NUM_OF_OFREG_MAX		99
#define	TOTAL_NUM_OF_ACRAM_MAX		6147

static const char *ak7755_firmware_pram[] =
{
    "Off",
    "basic",
    "data2",
    "data3",
    "data4",
    "data5",
    "data6",
    "data7",
    "data8",
    "data9",
};

static const char *ak7755_firmware_cram[] =
{
    "Off",
    "basic",
    "data2",
    "data3",
    "data4",
    "data5",
    "data6",
    "data7",
    "data8",
    "data9",
};

static const char *ak7755_firmware_ofreg[] =
{
    "Off",
    "basic",
    "data2",
    "data3",
    "data4",
};

static const char *ak7755_firmware_acram[] =
{
    "Off",
    "basic",
    "data2",
    "data3",
    "data4",
};

#include "ak7755ctl.h"

#endif

