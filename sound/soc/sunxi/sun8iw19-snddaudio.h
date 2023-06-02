/*
 * sound\soc\sunxi\sun8iw19-snddaudio.h
 * (C) Copyright 2014-2019
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

#ifndef	__SUN8IW19_SNDDAUDIO_H_
#define	__SUN8IW19_SNDDAUDIO_H_

#include "sun8iw19-daudio.h"

struct platform_data {
	unsigned int audio_format;
	unsigned int signal_inversion;
	unsigned int daudio_master;
};

struct sunxi_snddaudio_priv {
	struct snd_soc_card *card;
	struct platform_data pdata;
#ifdef DAUDIO_FORMAT_DYNC_DEBUG
	struct dentry *daudio_debug_root;
	struct dentry *format_file;
	struct dentry *signal_invert_file;
	struct dentry *clock_master_file;
	char debug_fmt_cmd[64];
	char debug_signal_invert_cmd[64];
	char debug_clk_mode_cmd[64];
#endif
};

#endif
