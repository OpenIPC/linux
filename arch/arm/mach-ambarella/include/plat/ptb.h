/*
 * arch/arm/plat-ambarella/include/plat/ptb.h
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

#ifndef __PLAT_AMBARELLA_PTB_H
#define __PLAT_AMBARELLA_PTB_H

/* ==========================================================================*/
#define BOOT_DEV_NAND		0x1
#define BOOT_DEV_NOR		0x2
#define BOOT_DEV_SM		0x3
#define BOOT_DEV_ONENAND	0x4
#define BOOT_DEV_SNOR		0x5

#define PART_DEV_AUTO		(0x00)
#define PART_DEV_NAND		(0x01)
#define PART_DEV_EMMC		(0x02)
#define PART_DEV_SNOR		(0x04)

#define FW_MODEL_NAME_SIZE	128

#define PART_MAX_WITH_RSV	32

#define PART_MAX		20
#define CMDLINE_PART_MAX	8

#define HAS_IMG_PARTS		15
#define HAS_NO_IMG_PARTS	1
#define TOTAL_FW_PARTS		(HAS_IMG_PARTS + HAS_NO_IMG_PARTS)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

typedef struct flpart_s
{
	u32	crc32;		/**< CRC32 checksum of image */
	u32	ver_num;	/**< Version number */
	u32	ver_date;	/**< Version date */
	u32	img_len;	/**< Lengh of image in the partition */
	u32	mem_addr;	/**< Starting address to copy to RAM */
	u32	flag;		/**< Special properties of this partition */
	u32	magic;		/**< Magic number */
} __attribute__((packed)) flpart_t;

#define FLDEV_CMD_LINE_SIZE	1024

typedef struct netdev_s
{
	/* This section contains networking related settings */
	u8	mac[6];		/**< MAC address*/
	u32	ip;		/**< Boot loader's LAN IP */
	u32	mask;		/**< Boot loader's LAN mask */
	u32	gw;		/**< Boot loader's LAN gateway */
} __attribute__((packed)) netdev_t;

typedef struct fldev_s
{
	char	sn[32];		/**< Serial number */
	u8	usbdl_mode;	/**< USB download mode */
	u8	auto_boot;	/**< Automatic boot */
	u8	rsv[2];
	u32	splash_id;
	char	cmdline[FLDEV_CMD_LINE_SIZE];

	/* This section contains networking related settings */
	netdev_t eth[2];
	netdev_t wifi[2];
	netdev_t usb_eth[2];

	/* This section contains update by network  related settings */
	u32	auto_dl;	/**< Automatic download? */
	u32	tftpd;		/**< Boot loader's TFTP server */
	u32	pri_addr;	/**< RTOS download address */
	char	pri_file[32];	/**< RTOS file name */
	u32	pri_comp;	/**< RTOS compressed? */
	u32	dtb_addr;	/**< DTB download address */
	char	dtb_file[32];	/**< DTB file name */
	u32	rmd_addr;	/**< Ramdisk download address */
	char	rmd_file[32];	/**< Ramdisk file name */
	u32	rmd_comp;	/**< Ramdisk compressed? */
	u32	dsp_addr;	/**< DSP download address */
	char	dsp_file[32];	/**< DSP file name */
	u32	dsp_comp;	/**< DSP compressed? */

	u32	magic;		/**< Magic number */
} __attribute__((packed)) fldev_t;

/*The header of PTB*/
#define PTB_HEADER_SIZE		64
#define PTB_HEADER_PAD_SIZE	(PTB_HEADER_SIZE - sizeof(u32) * 4)
typedef struct ptb_header_s
{
	u32	magic;
	u32	crc32;
	u32	version;
	u32	dev;
	u8	rsv[PTB_HEADER_PAD_SIZE];
} __attribute__((packed)) ptb_header_t;

#define PTB_SIZE		4096
#define PTB_PAD_SIZE		\
	(PTB_SIZE - PART_MAX_WITH_RSV * sizeof(flpart_t) - sizeof(fldev_t))

typedef struct flpart_table_s
{
	flpart_t	part[PART_MAX_WITH_RSV];/** Partitions */
	/* ------------------------------------------ */
	fldev_t		dev;			/**< Device properties */
	u8		rsv[PTB_PAD_SIZE];	/**< Padding to 2048 bytes */
} __attribute__((packed)) flpart_table_t;

/**
 * The meta data table is a region in flash after partition table.
 * The data need by dual boot are stored.
 */
/* For first version of flpart_meta_t */
#define PTB_META_MAGIC		0x33219fbd
/* For second version of flpart_meta_t */
#define PTB_META_MAGIC2		0x4432a0ce
#define PTB_META_MAGIC3		0x42405891
#define PART_NAME_LEN		8
#define PTB_META_ACTURAL_LEN	(sizeof(u32) + (sizeof(u32) * 3 + PART_NAME_LEN) * PART_MAX + \
				FW_MODEL_NAME_SIZE)
#define PTB_META_SIZE		2048
#define PTB_META_PAD_SIZE	(PTB_META_SIZE - PTB_META_ACTURAL_LEN)
typedef struct flpart_meta_s
{
	u32		magic;
	struct {
		u32	sblk;
		u32	nblk;
		u32	dev;
		char	name[PART_NAME_LEN];
	} part_info[PART_MAX];
	u8		model_name[FW_MODEL_NAME_SIZE];
	u8 		rsv[PTB_META_PAD_SIZE];
	/* This meta crc32 doesn't include itself. */
	/* It's only calc data before this field.  */
	u32 	crc32;
} __attribute__((packed)) flpart_meta_t;

#endif /* __ASSEMBLER__ */

/* ==========================================================================*/

#endif

