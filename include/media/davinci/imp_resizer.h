/*
 * Copyright (C) 2008-2009 Texas Instruments Inc
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
 * Resizer device is used for upscale or downscale the YCbCr image to
 * desired resolution.
 * The channel operates in 2 modes :-
 *   1. continuous mode. In this mode, the resizer modules are chained in
 *      the data path from ISIF/CCDC and does resizing on the fly.
 *      No IO allowed in this mode.
 *   2. Single shot mode. In this mode, modules are configured to resize
 *      the image stored in SDRAM.  Multiple IO instances are allowed in
 *      this mode and each IO instance is allocated a logical channel.
 *
 * device name : davinci_resizer
 ************************************************************************/

#ifndef _IMP_RESIZER_H
#define _IMP_RESIZER_H

#include <linux/ioctl.h>
#include <media/davinci/imp_common.h>

/* ioctls definitions for resizer operations */
#define RSZ_IOC_BASE		'R'
#define RSZ_REQBUF		_IOWR(RSZ_IOC_BASE, 1, struct imp_reqbufs)
#define RSZ_QUERYBUF		_IOWR(RSZ_IOC_BASE, 2, struct imp_buffer)
#define RSZ_RESIZE		_IOWR(RSZ_IOC_BASE, 3, struct imp_convert)
#define RSZ_S_CONFIG		_IOWR(RSZ_IOC_BASE, 4,\
					struct rsz_channel_config)
#define RSZ_G_CONFIG		_IOWR(RSZ_IOC_BASE, 5,\
					struct rsz_channel_config)
#define RSZ_S_OPER_MODE		_IOW(RSZ_IOC_BASE, 6, unsigned long)
#define RSZ_G_OPER_MODE		_IOR(RSZ_IOC_BASE, 7, unsigned long)
#define RSZ_G_PRIORITY		_IOR(RSZ_IOC_BASE, 8, unsigned long)
#define RSZ_S_PRIORITY		_IOW(RSZ_IOC_BASE, 9, unsigned long)
#define RSZ_RECONFIG		_IOWR(RSZ_IOC_BASE, 10, struct rsz_reconfig)
/* only for debug purpose */
#define RSZ_DUMP_HW_CONFIG	_IOW(RSZ_IOC_BASE, 11, unsigned long)
#define RSZ_IOC_MAXNR		11

#ifdef __KERNEL__

struct rsz_device {
	/* mutex lock for this device */
	struct mutex lock;
	/* number of users */
	int users;
};
#endif
#endif
