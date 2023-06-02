/*
 * arch/arm/plat-ambarella/include/plat/ambevent.h
 *
 * History:
 *    2010/07/23 - [Zhenwu Xue] Create
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
 *
 */

#ifndef __AMB_EVENT_H
#define __AMB_EVENT_H

/* ==========================================================================*/
#ifndef __ASSEMBLER__
#include <linux/kobject.h>

/* Copy from build/include/ambas_event.h */
enum amb_event_type {
	/* No Event */
	AMB_EV_NONE				= 0x00000000,

	/* VIN Event */
	AMB_EV_VIN_DECODER_SOURCE_PLUG		= 0x00010000,
	AMB_EV_VIN_DECODER_SOURCE_REMOVE,

	/* VOUT Event */
	AMB_EV_VOUT_CVBS_PLUG			= 0x00020000,
	AMB_EV_VOUT_CVBS_REMOVE,
	AMB_EV_VOUT_YPBPR_PLUG,
	AMB_EV_VOUT_YPBPR_REMOVE,
	AMB_EV_VOUT_HDMI_PLUG,
	AMB_EV_VOUT_HDMI_REMOVE,

	/* SENSOR Event*/
	AMB_EV_ACCELEROMETER_REPORT		= 0x00030000,
	AMB_EV_MAGNETIC_FIELD_REPORT,
	AMB_EV_LIGHT_REPORT,
	AMB_EV_PROXIMITY_REPORT,
	AMB_EV_GYROSCOPE_REPORT,
	AMB_EV_TEMPERATURE_REPORT,

	/* FB2 Event */
	AMB_EV_FB2_PAN_DISPLAY			= 0x00040000,

	/* FDET_EVENT */
	AMB_EV_FDET_FACE_DETECTED		= 0x00050000,
};

struct amb_event {
	u32			sno;		//sequential number
	u64			time_code;
	enum amb_event_type	type;
	u8			data[32];
};

struct amb_event_pool {
	struct mutex			op_mutex;
	struct amb_event		events[256];
	unsigned int			ev_sno;
	unsigned char			ev_index;
};

extern int amb_event_pool_init(struct amb_event_pool *pool);
extern int amb_event_pool_affuse(struct amb_event_pool *pool,
	struct amb_event event);
extern int amb_event_pool_query_index(struct amb_event_pool *pool);
extern int amb_event_pool_query_event(struct amb_event_pool *pool,
	struct amb_event *event, unsigned char index);
extern int amb_event_report_uevent(struct kobject *kobj,
	enum kobject_action action, char *envp_ext[]);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

