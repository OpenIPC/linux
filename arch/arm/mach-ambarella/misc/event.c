/*
 * arch/arm/plat-ambarella/misc/event.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/export.h>

#include <mach/hardware.h>
#include <plat/event.h>

/* ==========================================================================*/
static BLOCKING_NOTIFIER_HEAD(blocking_event_list);
static RAW_NOTIFIER_HEAD(raw_event_list);

/* ==========================================================================*/
int ambarella_register_event_notifier(void *nb)
{
	return blocking_notifier_chain_register(&blocking_event_list, nb);
}
EXPORT_SYMBOL(ambarella_register_event_notifier);

int ambarella_unregister_event_notifier(void *nb)
{
	return blocking_notifier_chain_unregister(&blocking_event_list, nb);
}
EXPORT_SYMBOL(ambarella_unregister_event_notifier);

int ambarella_set_event(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&blocking_event_list, val, v);
}
EXPORT_SYMBOL(ambarella_set_event);

int ambarella_register_raw_event_notifier(void *nb)
{
	return raw_notifier_chain_register(&raw_event_list, nb);
}
EXPORT_SYMBOL(ambarella_register_raw_event_notifier);

int ambarella_unregister_raw_event_notifier(void *nb)
{
	return raw_notifier_chain_unregister(&raw_event_list, nb);
}
EXPORT_SYMBOL(ambarella_unregister_raw_event_notifier);

int ambarella_set_raw_event(unsigned long val, void *v)
{
	return raw_notifier_call_chain(&raw_event_list, val, v);
}
EXPORT_SYMBOL(ambarella_set_raw_event);

