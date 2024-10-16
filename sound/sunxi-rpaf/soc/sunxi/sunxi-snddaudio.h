/*
 * sound\sunxi-rpaf\soc\sunxi\sunxi-snddaudio.h
 *
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__SUNXI_SNDDAUDIO_H_
#define	__SUNXI_SNDDAUDIO_H_

#include "sunxi-daudio.h"

struct platform_data {
	unsigned int audio_format;
	unsigned int signal_inversion;
	unsigned int daudio_master;
};

struct sunxi_snddaudio_priv {
	struct snd_soc_card *card;
	struct platform_data pdata;
};

#endif
