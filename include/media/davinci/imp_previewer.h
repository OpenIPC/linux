/*
 * Copyright (C) 2008 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/************************************************************************
 * Description
 * -----------
 * Previewer device is used for accessing the image tuning capabilities
 * of the hardware. The hardware channel has many modules, each with a set
 * of parameters. The channel operates in 2 modes :-
 *   1. continuous mode. In this mode, the modules are chained in the data
 *      path from ISIF/CCDC and does image tuning on the fly. No IO
 *      allowed in this mode.
 *   2. Single shot mode. In this mode, modules are configured to tune the
 *      image stored in SDRAM. Only one IO instance allowed.
 *
 * device name : davinci_previewer
 ************************************************************************/

#ifndef _IMP_PREVIEWER_H
#define _IMP_PREVIEWER_H

#include <linux/ioctl.h>
#include <media/davinci/imp_common.h>

/* Operation modes of previewer and resizer */
#define PREV_MODE_SINGLE_SHOT	IMP_MODE_SINGLE_SHOT
#define PREV_MODE_CONTINUOUS	IMP_MODE_CONTINUOUS
#define PREV_MODE_INVALID	IMP_MODE_INVALID

#define PREV_BUF_IN		0	/* input buffer */
#define PREV_BUF_OUT1		1	/* output buffer */
#define PREV_BUF_OUT2		2	/* output buffer */

/* ioctls definition for previewer operations */
#define PREV_IOC_BASE		'P'
#define PREV_REQBUF		_IOWR(PREV_IOC_BASE, 1, struct imp_reqbufs)
#define PREV_QUERYBUF		_IOWR(PREV_IOC_BASE, 2, struct imp_buffer)
#define PREV_S_PARAM		_IOWR(PREV_IOC_BASE, 3,\
					struct prev_module_param)
#define PREV_G_PARAM		_IOWR(PREV_IOC_BASE, 4,\
					struct prev_module_param)
#define PREV_PREVIEW		_IOWR(PREV_IOC_BASE, 5, struct imp_convert)
#define PREV_ENUM_CAP		_IOWR(PREV_IOC_BASE, 6, struct prev_cap)
#define PREV_S_CONFIG		_IOWR(PREV_IOC_BASE, 7,\
					struct prev_channel_config)
#define PREV_G_CONFIG		_IOWR(PREV_IOC_BASE, 8,\
					struct prev_channel_config)
#define PREV_S_OPER_MODE	_IOW(PREV_IOC_BASE, 9, unsigned long)
#define PREV_G_OPER_MODE	_IOR(PREV_IOC_BASE, 10, unsigned long)
#define PREV_S_CONTROL		_IOWR(PREV_IOC_BASE, 11, struct prev_control)
#define PREV_G_CONTROL		_IOWR(PREV_IOC_BASE, 12, struct prev_control)
#define PREV_S_DARK_FRAME	_IOW(PREV_IOC_BASE, 13, struct prev_dark_frame)
/* only for debug purpose */
#define PREV_DUMP_HW_CONFIG	_IOW(PREV_IOC_BASE, 14, unsigned long)
#define PREV_IOC_MAXNR		14

#ifdef __KERNEL__

struct prev_device {
	/* mutex lock for this device */
	struct mutex lock;
	/* number of users of this channel */
	int users;
	/* Channel structure. Either shared or independent */
	struct imp_logical_channel *chan;
};

struct prev_fh {
	/* This is primary uses configured the previewer channel */
	char primary_user;
	/* channel associated with this file handle */
	struct imp_logical_channel *chan;
};

#endif
#endif
