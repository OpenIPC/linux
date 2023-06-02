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
#ifndef DM365_AEW_DRIVER_H
#define DM365_AEW_DRIVER_H

#include <linux/ioctl.h>
#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/mutex.h>
#include <asm/io.h>
#endif				/* end of #ifdef __KERNEL__ */

/* Driver Range Constants */
#define AEW_WINDOW_VERTICAL_COUNT_MIN		1
#define AEW_WINDOW_VERTICAL_COUNT_MAX		128
#define AEW_WINDOW_HORIZONTAL_COUNT_MIN		2
#define AEW_WINDOW_HORIZONTAL_COUNT_MAX		36

#define AEW_WIDTH_MIN				8
#define AEW_WIDTH_MAX				256

#define AEW_AVELMT_MAX				1023

#define AEW_HZ_LINEINCR_MIN			2
#define AEW_HZ_LINEINCR_MAX			32

#define AEW_VT_LINEINCR_MIN			2
#define AEW_VT_LINEINCR_MAX			32

#define AEW_HEIGHT_MIN				2
#define AEW_HEIGHT_MAX				256

#define AEW_HZSTART_MIN				0
#define AEW_HZSTART_MAX				4095

#define AEW_VTSTART_MIN				0
#define AEW_VTSTART_MAX				4095

#define AEW_BLKWINHEIGHT_MIN			2
#define AEW_BLKWINHEIGHT_MAX			256

#define AEW_BLKWINVTSTART_MIN			0
#define AEW_BLKWINVTSTART_MAX			4095

#define AEW_SUMSHIFT_MAX			15

/* Statistics data size per window */
#define AEW_WINDOW_SIZE				32
#define AEW_WINDOW_SIZE_SUM_ONLY		16

#ifdef __KERNEL__

/* Device Constants */
#define AEW_NR_DEVS				1
#define AEW_DEVICE_NAME				"dm365_aew"
#define AEW_MAJOR_NUMBER			0
#define AEW_IOC_MAXNR				4
#define AEW_TIMEOUT				((300 * HZ) / 1000)
#endif


/* List of ioctls */
#pragma pack(1)
#define AEW_MAGIC_NO	'e'
#define AEW_S_PARAM	_IOWR(AEW_MAGIC_NO, 1, struct aew_configuration *)
#define AEW_G_PARAM	_IOWR(AEW_MAGIC_NO, 2, struct aew_configuration *)
#define AEW_ENABLE	_IO(AEW_MAGIC_NO, 3)
#define AEW_DISABLE	_IO(AEW_MAGIC_NO, 4)
#pragma  pack()

/* Enum for device usage */
enum aew_in_use_flag {
	/* Device is not in use */
	AEW_NOT_IN_USE,
	/* Device in use */
	AEW_IN_USE
};

/* Enum for Enable/Disable specific feature */
enum aew_enable_flag {
	H3A_AEW_DISABLE,
	H3A_AEW_ENABLE
};

enum aew_config_flag {
	H3A_AEW_CONFIG_NOT_DONE,
	H3A_AEW_CONFIG
};


/* Contains the information regarding Window Structure in AEW Engine */
struct aew_window {
	/* Width of the window */
	unsigned int width;
	/* Height of the window */
	unsigned int height;
	/* Horizontal Start of the window */
	unsigned int hz_start;
	/* Vertical Start of the window */
	unsigned int vt_start;
	/* Horizontal Count */
	unsigned int hz_cnt;
	/* Vertical Count */
	unsigned int vt_cnt;
	/* Horizontal Line Increment */
	unsigned int hz_line_incr;
	/* Vertical Line Increment */
	unsigned int vt_line_incr;
};

/* Contains the information regarding the AEW Black Window Structure */
struct aew_black_window {
	/* Height of the Black Window */
	unsigned int height;
	/* Vertical Start of the black Window */
	unsigned int vt_start;
};

/* Contains the information regarding the Horizontal Median Filter */
struct aew_hmf {
	/* Status of Horizontal Median Filter */
	enum aew_enable_flag enable;
	/* Threshhold Value for Horizontal Median Filter. Make sure
	 * to keep this same as AF threshold since we have a common
	 * threshold for both
	 */
	unsigned int threshold;
};

/* AE/AWB output format */
enum aew_output_format {
	AEW_OUT_SUM_OF_SQUARES,
	AEW_OUT_MIN_MAX,
	AEW_OUT_SUM_ONLY
};

/* Contains configuration required for setup of AEW engine */
struct aew_configuration {
	/* A-law status */
	enum aew_enable_flag alaw_enable;
	/* AE/AWB output format */
	enum aew_output_format out_format;
	/* AW/AWB right shift value for sum of pixels */
	char sum_shift;
	/* Saturation Limit */
	int saturation_limit;
	/* HMF configurations */
	struct aew_hmf hmf_config;
	/* Window for AEW Engine */
	struct aew_window window_config;
	/* Black Window */
	struct aew_black_window blackwindow_config;
};
#ifdef __KERNEL__
/* Contains information about device structure of AEW*/
struct aew_device {
	/* Driver usage flag */
	enum aew_in_use_flag in_use;
	/* Device configuration */
	struct aew_configuration *config;
	/* Contains latest statistics */
	void *buff_old;
	/* Buffer in which HW will fill the statistics or HW is already
	 * filling
	 */
	void *buff_curr;
	/* statistics Buffer which will be passed */
	void *buff_app;
	/* to user on read call. Flag indicates statistics are available */
	int buffer_filled;
	/* Window size in bytes */
	unsigned int size_window;
	/* Wait queue for the driver */
	wait_queue_head_t aew_wait_queue;
	/* Mutex for driver */
	struct mutex read_blocked;
	/* Flag indicates Engine is configured */
	enum aew_config_flag aew_config;
};

int aew_hardware_setup(void);
int aew_validate_parameters(void);
#endif				/* End of #ifdef __KERNEL__ */
#endif				/*End of DM365_AEW_H */
