 /* ak7755ioctrl.h
 *
 * Copyright (C) 2014 Asahi Kasei Microdevices Corporation.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *      14/04/09	     1.0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DSP_AK7755_H__
#define __DSP_AK7755_H__

/* IO CONTROL definition of AK7755 */
#define AK7755_IOCTL_MAGIC     's'

#define AK7755_MAGIC	0xD0
#define MAX_WREG		32
#define MAX_WCRAM		48

enum ak7755_ram_type {
	RAMTYPE_PRAM = 0,
	RAMTYPE_CRAM,
	RAMTYPE_OFREG,
	RAMTYPE_ACRAM,
};


enum ak7755_status {
	POWERDOWN = 0,
	SUSPEND,
	STANDBY,
	DOWNLOAD,
	RUN,
};

typedef struct _REG_CMD {
	unsigned char addr;
	unsigned char data;
} REG_CMD;

struct ak7755_wreg_handle {
	REG_CMD *regcmd;
	int len;
};

struct ak7755_wcram_handle{
	int    addr;
	unsigned char *cram;
	int    len;
};

struct ak7755_loadram_handle {
	int ramtype;
	int mode;
};

#define AK7755_IOCTL_SETSTATUS	_IOW(AK7755_MAGIC, 0x10, int)
#define AK7755_IOCTL_GETSTATUS	_IOR(AK7755_MAGIC, 0x11, int)
#define AK7755_IOCTL_SETMIR		_IOW(AK7755_MAGIC, 0x12, int)
#define AK7755_IOCTL_GETMIR		_IOR(AK7755_MAGIC, 0x13, unsigned long)
#define AK7755_IOCTL_WRITEREG	_IOW(AK7755_MAGIC, 0x14, struct ak7755_wreg_handle)
#define AK7755_IOCTL_WRITECRAM	_IOW(AK7755_MAGIC, 0x15, struct ak7755_wcram_handle)
#define AK7755_IOCTL_LOADRAM	_IOW(AK7755_MAGIC, 0x16, struct ak7755_loadram_handle)

#endif

