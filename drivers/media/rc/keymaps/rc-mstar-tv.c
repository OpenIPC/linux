/* mstar-tv.h - Keytable for mstar_tv Remote Controller
 *
 * keymap imported from ir-keymaps.c
 *
 * Copyright (c) 2010 by Mauro Carvalho Chehab <mchehab@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <media/rc-core.h>

/*
 * Jimmy Hsu <jimmy.hsu@mstarsemi.com>
 * this is the remote control that comes with the mstar smart tv
 * which based on STAOS standard.
 */
static struct rc_map_table mstar_tv[] = {
    { 0x0046, KEY_POWER },
    { 0x0050, KEY_0 },
    { 0x0049, KEY_1 },
    { 0x0055, KEY_2 },
    { 0x0059, KEY_3 },
    { 0x004D, KEY_4 },
    { 0x0051, KEY_5 },
    { 0x005D, KEY_6 },
    { 0x0048, KEY_7 },
    { 0x0054, KEY_8 },
    { 0x0058, KEY_9 },
    { 0x0047, KEY_RED },
    { 0x004B, KEY_GREEN },
    { 0x0057, KEY_YELLOW },
    { 0x005B, KEY_BLUE },
    { 0x0052, KEY_UP },
    { 0x0013, KEY_DOWN },
    { 0x0006, KEY_LEFT },
    { 0x001A, KEY_RIGHT },
    { 0x000F, KEY_ENTER },
    { 0x001F, KEY_CHANNELUP },
    { 0x0019, KEY_CHANNELDOWN },
    { 0x0016, KEY_VOLUMEUP },
    { 0x0015, KEY_VOLUMEDOWN },
    { 0x0003, KEY_PAGEUP },
    { 0x0005, KEY_PAGEDOWN },
    { 0x0017, KEY_HOME},
    { 0x0007, KEY_MENU },
    { 0x001B, KEY_BACK },
    { 0x005A, KEY_MUTE },
    { 0x000D, KEY_RECORD },     // DVR
    { 0x0042, KEY_HELP },       // GUIDE
    { 0x0014, KEY_INFO },
    { 0x0040, KEY_KP0 },        // WINDOW
    { 0x0004, KEY_KP1 },        // TV_INPUT
    { 0x000E, KEY_REWIND },
    { 0x0012, KEY_FORWARD },
    { 0x0002, KEY_PREVIOUSSONG },
    { 0x001E, KEY_NEXTSONG },
    { 0x0001, KEY_PLAY },
    { 0x001D, KEY_PAUSE },
    { 0x0011, KEY_STOP },
    { 0x0044, KEY_AUDIO },      // (C)SOUND_MODE
    { 0x0056, KEY_CAMERA },     // (C)PICTURE_MODE
    { 0x004C, KEY_ZOOM },       // (C)ASPECT_RATIO
    { 0x005C, KEY_CHANNEL },    // (C)CHANNEL_RETURN
    { 0x0045, KEY_SLEEP },      // (C)SLEEP
    { 0x004A, KEY_EPG },        // (C)EPG
    { 0x0010, KEY_LIST },       // (C)LIST
    { 0x0053, KEY_SUBTITLE },   // (C)SUBTITLE
    { 0x0041, KEY_FN_F1 },      // (C)MTS
    { 0x004E, KEY_FN_F2 },      // (C)FREEZE
    { 0x000A, KEY_FN_F3 },      // (C)TTX
    { 0x0009, KEY_FN_F4 },      // (C)CC
    { 0x001C, KEY_FN_F5 },      // (C)TV_SETTING
    { 0x0008, KEY_FN_F6 },      // (C)SCREENSHOT
    { 0x000B, KEY_F1 },         // MSTAR_BALANCE
    { 0x0018, KEY_F2 },         // MSTAR_INDEX
    { 0x0000, KEY_F3 },         // MSTAR_HOLD
    { 0x000C, KEY_F4 },         // MSTAR_UPDATE
    { 0x004F, KEY_F5 },         // MSTAR_REVEAL
    { 0x005E, KEY_F6 },         // MSTAR_SUBCODE
    { 0x0043, KEY_F7 },         // MSTAR_SIZE
    { 0x005F, KEY_F8 },         // MSTAR_CLOCK
    { 0x00FE, KEY_POWER2 },     // FAKE_POWER
    { 0x00FF, KEY_OK },         // KEY_OK

    // 2nd IR controller.
};

static struct rc_map_list mstar_tv_map = {
	.map = {
		.scan    = mstar_tv,
		.size    = ARRAY_SIZE(mstar_tv),
		.rc_type = RC_TYPE_UNKNOWN,	/* Legacy IR type */
		.name    = RC_MAP_MSTAR_TV,
	}
};

static int __init init_rc_map_mstar_tv(void)
{
	return rc_map_register(&mstar_tv_map);
}

static void __exit exit_rc_map_mstar_tv(void)
{
	rc_map_unregister(&mstar_tv_map);
}

module_init(init_rc_map_mstar_tv);
module_exit(exit_rc_map_mstar_tv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
