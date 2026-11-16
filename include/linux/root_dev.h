/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ROOT_DEV_H_
#define _ROOT_DEV_H_

#include <linux/major.h>
#include <linux/types.h>
#include <linux/kdev_t.h>

enum {
	Root_NFS = MKDEV(UNNAMED_MAJOR, 255),
	Root_CIFS = MKDEV(UNNAMED_MAJOR, 254),
	Root_Generic = MKDEV(UNNAMED_MAJOR, 253),
	Root_RAM0 = MKDEV(RAMDISK_MAJOR, 0),
};

extern dev_t ROOT_DEV;

/*
 * Set when the flash itself names the root filesystem (the "rootfs" of the
 * OpenIPC split NOR layout, drivers/mtd/split_layout.c): a root= naming an MTD
 * device -- whatever layout the bootloader believes in -- then gives way to it.
 */
extern bool root_dev_from_flash;

#endif
