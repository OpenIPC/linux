/*
 * xr806/up_cmd.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * usrer protocol flow APIs for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XR_UP_CMD_H__
#define __XR_UP_CMD_H__

int xradio_up_cmd_init(void *priv);
int xradio_up_cmd_deinit(void *priv);
int xradio_up_cmd_push(u8 *buff, u32 len);

#endif
