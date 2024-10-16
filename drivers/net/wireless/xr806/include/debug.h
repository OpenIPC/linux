/*
 * xr806/debug.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Debug info APIs for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XRADIO_DEBUG_H__
#define __XRADIO_DEBUG_H__

#include "os_intf.h"

/* Message always need to be present even in release version. */
#define XRADIO_DBG_ALWY 0x01

/* Error message to report an error, it can hardly works. */
#define XRADIO_DBG_ERROR 0x02

/* Warning message to inform us of something unnormal or
 * something very important, but it still work. */
#define XRADIO_DBG_WARN 0x04

/* Important message we need to know in unstable version. */
#define XRADIO_DBG_NIY 0x08

/* Normal message just for debug in developing stage. */
#define XRADIO_DBG_MSG 0x10

/* Trace of functions, for sequence of functions called. Normally,
 * don't set this level because there are too more print. */
#define XRADIO_DBG_TRC 0x20

#define XRADIO_DBG_LEVEL 0xFF

/* for host debuglevel*/
extern u8 dbg_common;
extern u8 dbg_cfg;
extern u8 dbg_iface;
extern u8 dbg_hwio;
extern u8 dbg_plat;
extern u8 dbg_queue;
extern u8 dbg_spi;
extern u8 dbg_txrx;
extern u8 dbg_cmd;

#define xradio_dbg(level, fmt, arg...)                                           \
	do {                                                                         \
		if ((level) & dbg_common & XRADIO_DBG_ERROR)                             \
			xradio_printf("[%s,%d][XRADIO_ERR] " fmt, __func__, __LINE__, ##arg);\
		else if ((level) & dbg_common & XRADIO_DBG_WARN)                         \
			xradio_printf("[%s,%d][XRADIO_WRN] " fmt, __func__, __LINE__, ##arg);\
		else if ((level) & dbg_common)                                           \
			xradio_printf("[%s,%d][XRADIO] " fmt, __func__, __LINE__, ##arg);    \
	} while (0)

#define cfg_printk(level, fmt, arg...)                                           \
	do {                                                                         \
		if ((level) & dbg_cfg & XRADIO_DBG_ERROR)                                \
			xradio_printf("[%s,%d][CFG_ERR] " fmt, __func__, __LINE__, ##arg);   \
		else if ((level) & dbg_cfg & XRADIO_DBG_WARN)                            \
			xradio_printf("[%s,%d][CFG_WRN] " fmt, __func__, __LINE__, ##arg);   \
		else if ((level) & dbg_cfg)                                              \
			xradio_printf("[%s,%d][CFG] " fmt, __func__, __LINE__, ##arg);       \
	} while (0)

#define hwio_printk(level, fmt, arg...)                                          \
	do {                                                                         \
		if ((level) & dbg_hwio & XRADIO_DBG_ERROR)                               \
			xradio_printf("[%s,%d][HWIO_ERR] " fmt, __func__, __LINE__, ##arg);  \
		else if ((level) & dbg_hwio & XRADIO_DBG_WARN)                           \
			xradio_printf("[%s,%d][HWIO_WRN] " fmt, __func__, __LINE__, ##arg);  \
		else if ((level) & dbg_hwio)                                             \
			xradio_printf("[%s,%d][HWIO] " fmt, __func__, __LINE__, ##arg);      \
	} while (0)

#define iface_printk(level, fmt, arg...)                                         \
	do {                                                                         \
		if ((level) & dbg_iface & XRADIO_DBG_ERROR)                              \
			xradio_printf("[%s,%d][IFACE_ERR] " fmt, __func__, __LINE__, ##arg); \
		else if ((level) & dbg_iface & XRADIO_DBG_WARN)                          \
			xradio_printf("[%s,%d][IFACE_WRN] " fmt, __func__, __LINE__, ##arg); \
		else if ((level) & dbg_iface)                                            \
			xradio_printf("[%s,%d][IFACE] " fmt, __func__, __LINE__, ##arg);     \
	} while (0)

#define plat_printk(level, fmt, arg...)                                          \
	do {                                                                         \
		if ((level) & dbg_plat & XRADIO_DBG_ERROR)                               \
			xradio_printf("[%s,%d][PLAT_ERR] " fmt, __func__, __LINE__, ##arg);  \
		else if ((level) & dbg_plat & XRADIO_DBG_WARN)                           \
			xradio_printf("[%s,%d][PLAT_WRN] " fmt, __func__, __LINE__, ##arg);  \
		else if ((level) & dbg_plat)                                             \
			xradio_printf("[%s,%d][PLAT] " fmt, __func__, __LINE__, ##arg);      \
	} while (0)

#define queue_printk(level, fmt, arg...)                                         \
	do {                                                                         \
		if ((level) & dbg_queue & XRADIO_DBG_ERROR)                              \
			xradio_printf("[%s,%d][QUEUE_ERR] " fmt, __func__, __LINE__, ##arg); \
		else if ((level) & dbg_queue & XRADIO_DBG_WARN)                          \
			xradio_printf("[%s,%d][QUEUE_WRN] " fmt, __func__, __LINE__, ##arg); \
		else if ((level) & dbg_queue)                                            \
			xradio_printf("[%s,%d][QUEUE] " fmt, __func__, __LINE__, ##arg);     \
	} while (0)

#define spi_printk(level, fmt, arg...)                                           \
	do {                                                                         \
		if ((level) & dbg_spi & XRADIO_DBG_ERROR)                                \
			xradio_printf("[%s,%d][SPI_ERR] " fmt, __func__, __LINE__, ##arg);   \
		else if ((level) & dbg_spi & XRADIO_DBG_WARN)                            \
			xradio_printf("[%s,%d][SPI_WRN] " fmt, __func__, __LINE__, ##arg);   \
		else if ((level) & dbg_spi)                                              \
			xradio_printf("[%s,%d][SPI] " fmt, __func__, __LINE__, ##arg);       \
	} while (0)

#define txrx_printk(level, fmt, arg...)                                          \
	do {                                                                         \
		if ((level) & dbg_txrx & XRADIO_DBG_ERROR)                               \
			xradio_printf("[%s,%d][TXRX_ERR] " fmt, __func__, __LINE__, ##arg);  \
		else if ((level) & dbg_txrx & XRADIO_DBG_WARN)                           \
			xradio_printf("[%s,%d][TXRX_WRN] " fmt, __func__, __LINE__, ##arg);  \
		else if ((level) & dbg_txrx)                                             \
			xradio_printf("[%s,%d][TXRX] " fmt, __func__, __LINE__, ##arg);      \
	} while (0)

#define cmd_printk(level, fmt, arg...)                                           \
	do {                                                                         \
		if ((level) & dbg_cmd & XRADIO_DBG_ERROR)                                \
			xradio_printf("[%s,%d][TXRX_ERR] " fmt, __func__, __LINE__, ##arg);  \
		else if ((level) & dbg_cmd & XRADIO_DBG_WARN)                            \
			xradio_printf("[%s,%d][TXRX_WRN] " fmt, __func__, __LINE__, ##arg);  \
		else if ((level) & dbg_txrx)                                             \
			xradio_printf("[%s,%d][TXRX] " fmt, __func__, __LINE__, ##arg);      \
	} while (0)

#include <linux/kernel.h>

static inline void data_hex_dump(char *pref, int width, unsigned char *buf, int len)
{
	int i, n;

	for (i = 0, n = 1; i < len; i++, n++) {
		if (n == 1)
			printk(KERN_CONT KERN_DEBUG "%s ", pref);
		printk(KERN_CONT KERN_DEBUG "%X ", buf[i]);
		if (n == width) {
			printk(KERN_CONT KERN_DEBUG "\n");
			n = 0;
		}
	}
	if (i && n != 1)
		printk(KERN_CONT KERN_DEBUG "\n");
}

void xradio_parse_frame(const char *func, u8 *eth_data, u8 tx, u32 flags);
#endif
