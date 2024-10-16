/*
 * sound\soc\sunxi\hifi-dsp\sun50iw11-sndcodec.h
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

#ifndef	__SUN50IW11_SNDCODEC_H_
#define	__SUN50IW11_SNDCODEC_H_

#include "sun50iw11-codec.h"

enum sunxi_sndcodec_div_id {
	CODEC_DIV_PLAYBACK = 0,
	CODEC_DIV_CAPTURE = 1,
};

struct sunxi_sndcodec_priv {
	struct snd_soc_card *card;
};

#endif
