/*
* (C) Copyright 2019-2025
* Allwinner Technology Co., Ltd. <www.allwinnertech.com>
* wujiayi <wujiayi@allwinnertech.com>
*
* some simple description for this code
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
*/
#ifndef __LINUX_DSP_DEBUG_H
#define __LINUX_DSP_DEBUG_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

struct debug_msg_t {
	uint32_t sys_cnt;
	uint32_t log_head_addr;
	uint32_t log_end_addr;
	uint32_t log_head_size;
};

struct dsp_sharespace_t {
	/* dsp write space msg */
	uint32_t dsp_write_addr;
	uint32_t dsp_write_size;

	/* arm write space msg */
	uint32_t arm_write_addr;
	uint32_t arm_write_size;

	/* dsp log space msg */
	uint32_t dsp_log_addr;
	uint32_t dsp_log_size;

	uint32_t mmap_phy_addr;
	uint32_t mmap_phy_size;

	/* arm read addr about dsp log*/
	uint32_t arm_read_dsp_log_addr;
	/* save msg value */
	struct debug_msg_t value;
};

enum dsp_debug_cmd {
	CMD_REFRESH_LOG_HEAD_ADDR = 0x00,
	CMD_READ_DEBUG_MSG = 0x01,
	CMD_WRITE_DEBUG_MSG = 0x03,
};
#endif /* __LINUX_DSP_DEBUG_H */
