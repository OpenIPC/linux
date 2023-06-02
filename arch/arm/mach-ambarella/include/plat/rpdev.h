/**
 * system/src/rpclnt/rpdev.h
 *
 * History:
 *    2012/08/15 - [Tzu-Jung Lee] created file
 *
 * Copyright 2008-2015 Ambarella Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __RPDEV_H__
#define __RPDEV_H__

#define RPMSG_VRING_ALIGN		(4096)
#define RPMSG_RESERVED_ADDRESSES	(1024)
#define RPMSG_ADDRESS_ANY		(0xffffffff)
#define RPMSG_NS_ADDR			(53)
#define RPMSG_NAME_LEN			(32)

#if defined (__KERNEL__)
#include <plat/remoteproc_cfg.h>
#include <linux/sched.h>
#include <linux/rpmsg.h>
#else
#include "rpmsg.h"
#endif

struct rpdev;
typedef void (*rpdev_cb)(struct rpdev* rpdev, void *data, int len,
			 void *priv, u32 src);
struct rpdev {

#if defined(__KERNEL__)
	struct completion	comp;
#else
	ID			flgid;
#endif

	struct rpclnt		*rpclnt;
	char			name[RPMSG_NAME_LEN];
	u32     		src;
	u32     		dst;
	u32			flags;

	rpdev_cb		cb;
	void			*priv;
};

extern int rpdev_register(struct rpdev *rpdev, const char *bus_name);

extern struct rpdev *rpdev_alloc(const char *name, int flags,
                                 rpdev_cb cb, void *priv);

extern int rpdev_send_offchannel(struct rpdev *rpdev, u32 src, u32 dst,
                                 void *data, int len);

extern int rpdev_send(struct rpdev *rpdev, void *data, int len);
extern int rpdev_trysend(struct rpdev *rpdev, void *data, int len);

#endif /* __RPDEV_H__ */
