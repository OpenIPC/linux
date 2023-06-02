/* Keytable for Novatek NEC Extended Remote Controller
 *
 * keymap imported from ir-keymaps.c
 *
 * Copyright (c) 2020 by Shawn Chou <shawn_chou@novatek.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/rc-map.h>
#include <linux/module.h>

static struct rc_map_table nvt_nec_ext[] = {
	{ 0x866b1e, KEY_POWER},
	{ 0x866b1c, MSC_SCAN},
	{ 0x866b18, KEY_MUTE},
	{ 0x866b01, KEY_1},
	{ 0x866b0b, KEY_2},
	{ 0x866b1b, KEY_3},
	{ 0x866b05, KEY_4},
	{ 0x866b09, KEY_5},
	{ 0x866b15, KEY_6},
	{ 0x866b06, KEY_7},
	{ 0x866b0a, KEY_8},
	{ 0x866b12, KEY_9},
	{ 0x866b02, KEY_0},
	{ 0x866b1f, KEY_CHANNELUP},
	{ 0x866b17, KEY_CHANNELDOWN},
	{ 0x866b16, KEY_VOLUMEUP},
	{ 0x866b14, KEY_VOLUMEDOWN},
};

static struct rc_map_list nvt_nec_ext_map = {
	.map = {
		.scan     = nvt_nec_ext,
		.size     = ARRAY_SIZE(nvt_nec_ext),
		.rc_proto = RC_PROTO_UNKNOWN,	/* Legacy IR type */
		.name     = RC_MAP_NVT_NEC_EXT,
	}
};

static int __init init_rc_map_nvt_nec_ext(void)
{
	return rc_map_register(&nvt_nec_ext_map);
}

static void __exit exit_rc_map_nvt_nec_ext(void)
{
	rc_map_unregister(&nvt_nec_ext_map);
}

module_init(init_rc_map_nvt_nec_ext)
module_exit(exit_rc_map_nvt_nec_ext)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shawn Chou <shawn_chou@novatek.com.tw>");

