/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/*
 * @file aipu_priv.h
 * Header of the aipu private struct
 */

#ifndef __AIPU_PRIV_H__
#define __AIPU_PRIV_H__

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include "aipu_irq.h"
#include "aipu_io.h"
#include "aipu_core.h"
#include "aipu_job_manager.h"
#include "aipu_mm.h"

/*
 * struct aipu_priv - AIPU private struct contains all core info and shared resources
 *
 * @version:     AIPU hardware version
 * @core_cnt:    AIPU core count in system
 * @cores:       core pointer array
 * @core0_dev:   device struct pointer of core 0
 * @aipu_fops:   file operation struct
 * @misc:        misc driver struct
 * @job_manager: job manager struct
 * @mm:          memory manager
 * @interrupts:  interrupt registers IO region
 * @is_init:     init flag
 */
struct aipu_priv {
	int version;
	int core_cnt;
	struct aipu_core **cores;
	struct device *core0_dev;
	const struct file_operations *aipu_fops;
	struct miscdevice            misc;
	struct aipu_job_manager      job_manager;
	struct aipu_memory_manager   mm;
	struct io_region interrupts;
	int is_init;
};

/*
 * @brief initialize an input AIPU private data struct
 *
 * @param aipu:  pointer to AIPU private data struct
 * @param p_dev: platform device struct pointer
 * @param fops: file_operations struct pointer
 *
 * @return 0 if successful; others if failed;
 */
int init_aipu_priv(struct aipu_priv *aipu, struct platform_device *p_dev,
	const struct file_operations *fops);
/*
 * @brief add new detected core into aipu_priv struct in probe phase
 *
 * @param aipu:    pointer to AIPU private data struct
 * @param core:    aipu core pointer allocated in calling function
 * @param version: aipu core hardware version number
 * @param id:      aipu core ID
 * @param p_dev:   platform device struct pointer
 *
 * @return 0 if successful; others if failed;
 */
int aipu_priv_add_core(struct aipu_priv *aipu, struct aipu_core *core,
	int version, int id, struct platform_device *p_dev);
/*
 * @brief get AIPU hardware version number wrapper
 *
 * @param aipu: pointer to AIPU private data struct
 *
 * @return version
 */
int aipu_priv_get_version(struct aipu_priv *aipu);
/*
 * @brief get AIPU core count
 *
 * @param aipu: pointer to AIPU private data struct
 *
 * @return core count
 */
int aipu_priv_get_core_cnt(struct aipu_priv *aipu);
/*
 * @brief deinit an AIPU private data struct
 *
 * @param aipu: pointer to AIPU private data struct
 *
 * @return 0 if successful; others if failed;
 */
int deinit_aipu_priv(struct aipu_priv *aipu);
/*
 * @brief query AIPU capability wrapper (per core capability)
 *
 * @param aipu: pointer to AIPU private data struct
 * @param cap:  pointer to the capability struct
 *
 * @return 0 if successful; others if failed;
 */
int aipu_priv_query_core_capability(struct aipu_priv *aipu, struct aipu_core_cap *cap);
/*
 * @brief query AIPU capability wrapper (multicore common capability)
 *
 * @param aipu: pointer to AIPU private data struct
 * @param cap:  pointer to the capability struct
 *
 * @return 0 if successful; others if failed;
 */
int aipu_priv_query_capability(struct aipu_priv *aipu, struct aipu_cap *cap);
/*
 * @brief AIPU external register read/write wrapper
 *
 * @param aipu: pointer to AIPU private data struct
 * @param io_req:  pointer to the io_req struct
 *
 * @return 0 if successful; others if failed;
 */
int aipu_priv_io_rw(struct aipu_priv *aipu, struct aipu_io_req *io_req);
/*
 * @brief check if aipu status is ready for usage
 *
 * @param aipu: pointer to AIPU private data struct
 *
 * @return 0 if successful; others if failed;
 */
int aipu_priv_check_status(struct aipu_priv *aipu);

#endif /* __AIPU_PRIV_H__ */
