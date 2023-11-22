/*
 * uvc_video.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	uvc_video.h  --  USB Video Class Gadget driver
 *
 * Copyright (C) 2009-2010
 *		Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *		Author: Andrzej Pietrasiewicz <andrzejtp2010@gmail.com>
 */

#ifndef __UVC_VIDEO_H__
#define __UVC_VIDEO_H__

struct uvc_video;

int uvcg_video_enable(struct uvc_video *video, int enable);

int uvcg_video_init(struct uvc_video *video, struct uvc_device *uvc);

#endif /* __UVC_VIDEO_H__ */
