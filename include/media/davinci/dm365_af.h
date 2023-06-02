/* *
 * Copyright (C) 2009 Texas Instruments Inc
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef AF_DM365_DRIVER_H
#define AF_DM365_DRIVER_H

#include <linux/ioctl.h>
#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/mutex.h>
#include <asm/io.h>
#endif				/* End of __KERNEL_ */

#ifdef __KERNEL__
/* Device Constants */
#define AF_MAJOR_NUMBER			0
#define AF_NR_DEVS			1
#define AF_TIMEOUT			((300 * HZ) / 1000)
#endif				/* End of #ifdef __KERNEL__ */

/* Range Constants */

#define AF_PAXEL_HORIZONTAL_COUNT_MIN	1
#define AF_PAXEL_HORIZONTAL_COUNT_MAX	36

#define AF_PAXEL_VERTICAL_COUNT_MIN	1
#define AF_PAXEL_VERTICAL_COUNT_MAX	128

#define AF_PAXEL_HF_VF_COUNT_MAX	12
#define AF_PAXEL_HF_VF_COUNT_MIN	1

#define AF_WIDTH_MIN			8
#define AF_WIDTH_MAX			512

#define AF_LINE_INCR_MIN		2
#define AF_LINE_INCR_MAX		32

#define AF_COLUMN_INCR_MIN		2
#define AF_COLUMN_INCR_MAX		32

#define AF_HEIGHT_MIN			2
#define AF_HEIGHT_MAX			512

#define AF_HZSTART_MIN			2
#define AF_HZSTART_MAX			4094

#define AF_VTSTART_MIN			0
#define AF_VTSTART_MAX			4095

#define AF_MEDTH_MAX			255

#define AF_IIRSH_MAX			4094

/* Statistics data size per paxel */
#define AF_PAXEL_SIZE_HF_ONLY		48
#define AF_PAXEL_SIZE_HF_VF		64

#define AF_NUMBER_OF_HFV_COEF		11
#define AF_NUMBER_OF_VFV_COEF		5
#define AF_HFV_COEF_MASK		0xFFF
#define AF_VFV_COEF_MASK		0xFF
#define AF_HFV_THR_MAX			0xFFFF
#define AF_VFV_THR_MAX			0xFFFF

/* list of ioctls */
#pragma pack(1)
#define  AF_IOC_MAXNR			5
#define  AF_MAGIC_NO			'a'
#define  AF_S_PARAM	_IOWR(AF_MAGIC_NO, 1, struct af_configuration *)
#define  AF_G_PARAM	_IOWR(AF_MAGIC_NO, 2, struct af_configuration *)
#define  AF_ENABLE	_IO(AF_MAGIC_NO, 3)
#define  AF_DISABLE	_IO(AF_MAGIC_NO, 4)
#pragma  pack()

/* enum used for status of specific feature */
enum af_enable_flag {
	H3A_AF_DISABLE,
	H3A_AF_ENABLE
};

enum af_config_flag {
	H3A_AF_CONFIG_NOT_DONE,
	H3A_AF_CONFIG
};

struct af_reg_dump {
	unsigned int addr;
	unsigned int val;
};

/* enum used for keep track of whether hardware is used */
enum af_in_use_flag {
	AF_NOT_IN_USE,
	AF_IN_USE
};

enum af_mode {
	ACCUMULATOR_SUMMED,
	ACCUMULATOR_PEAK
};

/* Focus value selection */
enum af_focus_val_sel {
	/* 4 color Horizontal focus value only */
	AF_HFV_ONLY,
	/* 1 color Horizontal focus value & 1 color Vertical focus vlaue */
	AF_HFV_AND_VFV
};


/* Red, Green, and blue pixel location in the AF windows */
enum rgbpos {
	/* GR and GB as Bayer pattern */
	GR_GB_BAYER,
	/* RG and GB as Bayer pattern */
	RG_GB_BAYER,
	/* GR and BG as Bayer pattern */
	GR_BG_BAYER,
	/* RG and BG as Bayer pattern */
	RG_BG_BAYER,
	/* GG and RB as custom pattern */
	GG_RB_CUSTOM,
	/* RB and GG as custom pattern */
	RB_GG_CUSTOM
};

/* Contains the information regarding the Horizontal Median Filter */
struct af_hmf {
	/* Status of Horizontal Median Filter */
	enum af_enable_flag enable;
	/* Threshhold Value for Horizontal Median Filter */
	unsigned int threshold;
};

/* Contains the information regarding the IIR Filters */
struct af_iir {
	/* IIR Start Register Value */
	unsigned int hz_start_pos;
	/* IIR Filter Coefficient for Set 0 */
	int coeff_set0[AF_NUMBER_OF_HFV_COEF];
	/* IIR Filter Coefficient for Set 1 */
	int coeff_set1[AF_NUMBER_OF_HFV_COEF];
};

/* Contains the information regarding the VFV FIR filters */
struct af_fir {
	/* FIR 1 coefficents */
	int coeff_1[AF_NUMBER_OF_VFV_COEF];
	/* FIR 2 coefficents */
	int coeff_2[AF_NUMBER_OF_VFV_COEF];
	/* Horizontal FV threshold for FIR 1 */
	unsigned int hfv_thr1;
	/* Horizontal FV threshold for FIR 2 */
	unsigned int hfv_thr2;
	/* Vertical FV threshold for FIR 1 */
	unsigned int vfv_thr1;
	/* Vertical FV threshold for FIR 2 */
	unsigned int vfv_thr2;
};
/* Contains the information regarding the Paxels Structure in AF Engine */
struct af_paxel {
	/* Width of the Paxel */
	unsigned int width;
	/* Height of the Paxel */
	unsigned int height;
	/* Horizontal Start Position */
	unsigned int hz_start;
	/* Vertical Start Position */
	unsigned int vt_start;
	/* Horizontal Count */
	unsigned int hz_cnt;
	/* Vertical Count */
	unsigned int vt_cnt;
	/* Line Increment */
	unsigned int line_incr;
	/* Column Increment. Only for VFV */
	unsigned int column_incr;
};


/* Contains the parameters required for hardware set up of AF Engine */
struct af_configuration {
	/* ALAW status */
	enum af_enable_flag alaw_enable;
	/* Focus value selection */
	enum af_focus_val_sel fv_sel;
	/* HMF configurations */
	struct af_hmf hmf_config;
	/* RGB Positions. Only applicable with AF_HFV_ONLY selection */
	enum rgbpos rgb_pos;
	/* IIR filter configurations */
	struct af_iir iir_config;
	/* FIR filter configuration */
	struct af_fir fir_config;
	/* Paxel parameters */
	struct af_paxel paxel_config;
	/* Accumulator mode */
	enum af_mode mode;
};

#ifdef __KERNEL__
/* Structure for device of AF Engine */
struct af_device {
	/* Driver usage counter */
	enum af_in_use_flag in_use;
	/* Device configuration structure */
	struct af_configuration *config;
	/* Contains the latest statistics */
	void *buff_old;
	/* Buffer in which HW will fill the statistics or HW is already
	 * filling statistics
	 */
	void *buff_curr;
	/* Buffer which will be passed to */
	void *buff_app;
	/* user space on read call Size of image buffer */
	unsigned int buff_size;
	/* Flag indicates */
	int buffer_filled;
	/* statistics are available Paxel size in bytes */
	int size_paxel;
	/* Wait queue for driver */
	wait_queue_head_t af_wait_queue;
	/* mutex for driver */
	struct mutex read_blocked;
	/* Flag indicates Engine is configured */
	enum af_config_flag af_config;
};

#endif				/* __KERNEL__ */
#endif				/* AF_DM365_DRIVER_H */
