/*
 *    Filename: cedarv_ve.h
 *     Version: 0.01alpha
 * Description: Video engine driver API, Don't modify it in user space.
 *     License: GPLv2
 *
 *		Author  : xyliu <xyliu@allwinnertech.com>
 *		Date    : 2016/04/13
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
/* Notice: It's video engine driver API, Don't modify it in user space. */
#ifndef _CEDAR_VE_H_
#define _CEDAR_VE_H_

/* the struct must be same with user-header-file: libcedarc/veAw.h*/
struct ve_channel_proc_info {
	unsigned char *base_info_data;
	unsigned int   base_info_size;
	unsigned char *advance_info_data;
	unsigned int   advance_info_size;
	unsigned int   channel_id;
};

enum IOCTL_CMD {
	IOCTL_UNKOWN = 0x100,
	IOCTL_GET_ENV_INFO,
	IOCTL_WAIT_VE_DE,
	IOCTL_WAIT_VE_EN,
	IOCTL_RESET_VE,
	IOCTL_ENABLE_VE,
	IOCTL_DISABLE_VE,
	IOCTL_SET_VE_FREQ,

	IOCTL_CONFIG_AVS2 = 0x200,
	IOCTL_GETVALUE_AVS2,
	IOCTL_PAUSE_AVS2,
	IOCTL_START_AVS2,
	IOCTL_RESET_AVS2,
	IOCTL_ADJUST_AVS2,
	IOCTL_ENGINE_REQ,
	IOCTL_ENGINE_REL,
	IOCTL_ENGINE_CHECK_DELAY,
	IOCTL_GET_IC_VER,
	IOCTL_ADJUST_AVS2_ABS,
	IOCTL_FLUSH_CACHE,
	IOCTL_SET_REFCOUNT,
	IOCTL_FLUSH_CACHE_ALL,
	IOCTL_TEST_VERSION,

	IOCTL_GET_LOCK = 0x310,
	IOCTL_RELEASE_LOCK,

	IOCTL_SET_VOL = 0x400,

	IOCTL_WAIT_JPEG_DEC = 0x500,
	/*for get the ve ref_count for ipc to delete the semphore*/
	IOCTL_GET_REFCOUNT,

	/*for iommu*/
	IOCTL_GET_IOMMU_ADDR,
	IOCTL_FREE_IOMMU_ADDR,

	/*for debug*/
	IOCTL_SET_PROC_INFO,
	IOCTL_STOP_PROC_INFO,
	IOCTL_COPY_PROC_INFO,

	IOCTL_SET_DRAM_HIGH_CHANNAL = 0x600,

	/* debug for decoder and encoder*/
	IOCTL_PROC_INFO_COPY = 0x610,
	IOCTL_PROC_INFO_STOP,
	IOCTL_POWER_SETUP = 0x700,
	IOCTL_POWER_SHUTDOWN,

	IOCTL_GET_CSI_ONLINE_INFO = 0x710,
	IOCTL_CLEAR_EN_INT_FLAG   = 0x711,
};

enum VE_INTERRUPT_RESULT_TYPE {
	VE_INT_RESULT_TYPE_TIMEOUT   = 0,
	VE_INT_RESULT_TYPE_NORMAL    = 1,
	VE_INT_RESULT_TYPE_CSI_RESET = 2,
};

#define VE_LOCK_VDEC 0x01
#define VE_LOCK_VENC 0x02
#define VE_LOCK_JDEC 0x04
#define VE_LOCK_00_REG 0x08
#define VE_LOCK_04_REG 0x10
#define VE_LOCK_ERR 0x80

typedef struct CsiOnlineRelatedInfo {
	unsigned int csi_frame_start_cnt;
	unsigned int csi_frame_done_cnt;
	unsigned int csi_cur_frame_addr;
	unsigned int csi_pre_frame_addr;
	unsigned int csi_line_start_cnt;
	unsigned int csi_line_done_cnt;
} CsiOnlineRelatedInfo;

struct cedarv_env_infomation {
	unsigned int phymem_start;
	int phymem_total_size;
	unsigned long address_macc;
};

#endif
