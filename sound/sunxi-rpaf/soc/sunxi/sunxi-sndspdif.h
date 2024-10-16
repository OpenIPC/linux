/*
 * sound\sunxi-rpaf\soc\sunxi\sunxi-sndspdif.h
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

#ifndef	__SUNXI_SNDSPDIF_H_
#define	__SUNXI_SNDSPDIF_H_

struct sunxi_sndspdif_priv {
	struct snd_soc_card *card;
	struct platform_device *codec_device;
};

#endif
