/* Keytable for Novatek NEC Remote Controller
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

static struct rc_map_table nvt_nec[] = {
	{ 0x400, KEY_POWER},
	{ 0x408, MSC_SCAN},
	{ 0x401, KEY_1},
	{ 0x405, KEY_2},
	{ 0x409, KEY_3},
	{ 0x402, KEY_4},
	{ 0x406, KEY_5},
	{ 0x40a, KEY_6},
	{ 0x403, KEY_7},
	{ 0x407, KEY_8},
	{ 0x40b, KEY_9},
	{ 0x404, KEY_0},
	{ 0x40d, KEY_UP},
	{ 0x410, KEY_LEFT},
	{ 0x411, KEY_KPENTER},
	{ 0x412, KEY_RIGHT},
	{ 0x415, KEY_DOWN},
};

static struct rc_map_list nvt_nec_map = {
	.map = {
		.scan     = nvt_nec,
		.size     = ARRAY_SIZE(nvt_nec),
		.rc_proto = RC_PROTO_UNKNOWN,	/* Legacy IR type */
		.name     = RC_MAP_NVT_NEC,
	}
};

static int __init init_rc_map_nvt_nec(void)
{
	return rc_map_register(&nvt_nec_map);
}

static void __exit exit_rc_map_nvt_nec(void)
{
	rc_map_unregister(&nvt_nec_map);
}

module_init(init_rc_map_nvt_nec)
module_exit(exit_rc_map_nvt_nec)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shawn Chou <shawn_chou@novatek.com.tw>");

