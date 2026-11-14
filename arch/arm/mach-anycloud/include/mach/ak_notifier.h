/*
 * linux/arch/arm/mach-anycloud/include/mach/ak_notifier.h
 *
 * Copyright (C) 2021 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
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
 */

#ifndef __AK_NOTIFIER_H__
#define __AK_NOTIFIER_H__

#define AK_NOTIFIER_EVENT_GROUP_AUDIO               (0)

/*
* AK_NOTIFIER_EVENT_GROUP_AUDIO
*/
#define AK_NOTIFIER_AUDIO_ALIVE                     (0)
#define AK_NOTIFIER_AUDIO_REMOVE                    (1)
#define AK_NOTIFIER_AUDIO_SUB_ALIVE                 (2)
#define AK_NOTIFIER_AUDIO_SUB_REMOVE                (3)
#define AK_NOTIFIER_AUDIO_SUB_ADD                   (4)

#define AK_NOTIFIER_EVENT_MK(group, event)          ((group << 16) | event)
#define AK_NOTIFIER_EVENT_GROUP(val)                ((val >> 16) & 0xFFFF)
#define AK_NOTIFIER_EVENT_VAL(val)                  ((val) & 0xFFFF)

#endif //__AK_NOTIFIER_H__
