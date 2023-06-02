/*
 * arch/arm/plat-ambarella/generic/audio.c
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
#include <linux/export.h>
#include <linux/notifier.h>
#include <plat/audio.h>

/* ==========================================================================*/
static struct srcu_notifier_head audio_notifier_list;
static struct notifier_block audio_notify;
static struct ambarella_i2s_interface audio_i2s_intf;

struct ambarella_i2s_interface get_audio_i2s_interface(void)
{
	return audio_i2s_intf;
}
EXPORT_SYMBOL(get_audio_i2s_interface);

static int audio_notify_transition(struct notifier_block *nb,
		unsigned long val, void *data)
{
	switch(val) {
	case AUDIO_NOTIFY_INIT:
		audio_i2s_intf.state = AUDIO_NOTIFY_INIT;
		memcpy(&audio_i2s_intf, data,
			sizeof(struct ambarella_i2s_interface));
		break;

	case AUDIO_NOTIFY_SETHWPARAMS:
		audio_i2s_intf.state = AUDIO_NOTIFY_SETHWPARAMS;
		memcpy(&audio_i2s_intf, data,
			sizeof(struct ambarella_i2s_interface));
		break;

	case AUDIO_NOTIFY_REMOVE:
		memset(&audio_i2s_intf, 0,
			sizeof(struct ambarella_i2s_interface));
		audio_i2s_intf.state = AUDIO_NOTIFY_REMOVE;
		break;
	default:
		audio_i2s_intf.state = AUDIO_NOTIFY_UNKNOWN;
		break;
	}

	return 0;
}

void ambarella_audio_notify_transition (
	struct ambarella_i2s_interface *data, unsigned int type)
{
	srcu_notifier_call_chain(&audio_notifier_list, type, data);
}
EXPORT_SYMBOL(ambarella_audio_notify_transition);

int ambarella_audio_register_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_register( &audio_notifier_list, nb);
}
EXPORT_SYMBOL(ambarella_audio_register_notifier);


int ambarella_audio_unregister_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_unregister(&audio_notifier_list, nb);
}
EXPORT_SYMBOL(ambarella_audio_unregister_notifier);


int __init ambarella_init_audio(void)
{
	int retval = 0;

	srcu_init_notifier_head(&audio_notifier_list);

	memset(&audio_i2s_intf, 0, sizeof(struct ambarella_i2s_interface));
	audio_i2s_intf.state = AUDIO_NOTIFY_UNKNOWN;

	audio_notify.notifier_call = audio_notify_transition;
	retval = ambarella_audio_register_notifier(&audio_notify);

	return retval;
}

