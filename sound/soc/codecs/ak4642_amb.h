/*
 * ak4642.h  --  AK4642 Soc Audio driver
 *
 * Copyright 2009 Ambarella Ltd.
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _AK4642_H
#define _AK4642_H

/* AK4642 register space */

#define AK4642_PM1		0x00
#define AK4642_PM2		0x01
#define AK4642_SIG1		0x02
#define AK4642_SIG2		0x03
#define AK4642_MODE1		0x04
#define AK4642_MODE2		0x05
#define AK4642_TIMER		0x06
#define AK4642_ALC1		0x07
#define AK4642_ALC2		0x08
#define AK4642_LIVOL		0x09
#define AK4642_LDVOL		0x0a
#define AK4642_ALC3		0x0b
#define AK4642_RIVOL		0x0c
#define AK4642_RDVOL		0x0d
#define AK4642_MODE3		0x0e
#define AK4642_MODE4		0x0f
#define AK4642_PM3		0x10
#define AK4642_FSEL		0x11
#define AK4642_F3EF0		0x12
#define AK4642_F3EF1		0x13
#define AK4642_F3EF2		0x14
#define AK4642_F3EF3		0x15
#define AK4642_EQEF0		0x16
#define AK4642_EQEF1		0x17
#define AK4642_EQEF2		0x18
#define AK4642_EQEF3		0x19
#define AK4642_EQEF4		0x1a
#define AK4642_EQEF5		0x1b
#define AK4642_F1EF0		0x1c
#define AK4642_E1EF1		0x1d
#define AK4642_E1EF2		0x1e
#define AK4642_E1EF3		0x1f

#define AK4642_CACHEREGNUM 	0x20

#define AK4642_SYSCLK	0

#define AK4642_LINE_IN_ON	0
#define AK4642_BOTH_MIC_ON	1
#define AK4642_INPUT_UNKNOWN	2

#define AK4642_MCLK_IN	0
#define AK4642_BCLK_IN	1
#define AK4642_MCLK_IN_BCLK_OUT	2



#endif
