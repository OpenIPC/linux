/*
 * uvc_configfs.h- Sigmastar
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
 * uvc_configfs.h
 *
 * Configfs support for the uvc function.
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrzej Pietrasiewicz <andrzejtp2010@gmail.com>
 */

#ifndef UVC_CONFIGFS_H
#define UVC_CONFIGFS_H

struct f_uvc_opts;

int uvcg_attach_configfs(struct f_uvc_opts *opts);

#endif /* UVC_CONFIGFS_H */
