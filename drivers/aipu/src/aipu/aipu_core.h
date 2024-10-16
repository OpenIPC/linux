/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_core.h
 * Header of the aipu_core struct
 */

#ifndef __AIPU_CORE_H__
#define __AIPU_CORE_H__

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <asm/atomic.h>
#include "armchina_aipu.h"
#include "aipu_irq.h"
#include "aipu_io.h"
#include "zhouyi.h"
#include "config.h"

#define MAX_IO_REGION_CNT 2

struct aipu_core;

/**
 * struct aipu_core_operations - a struct contains AIPU hardware operation methods
 *
 * @get_version:        get hardware version number
 * @get_config:         get hardware configuration number
 * @enable_interrupt:   enable all AIPU interrupts
 * @disable_interrupt:  disable all AIPU interrupts
 * @trigger:            trigger a deferred-job to run on a reserved core
 * @reserve:            reserve AIPU core for a job/deferred-job
 * @is_idle:            is AIPU hardware idle or not
 * @read_status_reg:    read status register value
 * @print_hw_id_info:   print AIPU version ID registers information
 * @io_rw:              direct IO read/write operations
 * @upper_half:         interrupt upper half handler
 * @bottom_half:        interrupt bottom half handler
 * @start_bw_profiling: start bandwidth profiling
 * @stop_bw_profiling:  stop bandwidth profiling
 * @read_profiling_reg: read profiling register values
 * @has_clk_ctrl:       has clock gating control feature or not
 * @enable_clk_gating:  enable clock gating
 * @disable_clk_gating: disable clock gating
 * @is_clk_gated:       is in clock gating state or not
 * @has_power_ctrl:     has power on/off control feature or not
 * @power_on:           power on this core
 * @power_off:          power off this core
 * @is_power_on:        is in power-on state or not
 * @sysfs_show:         show core external register values
 */
struct aipu_core_operations {
	int  (*get_version)(struct aipu_core *core);
	int  (*get_config)(struct aipu_core *core);
	void (*enable_interrupt)(struct aipu_core *core);
	void (*disable_interrupt)(struct aipu_core *core);
	void (*trigger)(struct aipu_core *core);
	int  (*reserve)(struct aipu_core *core, struct aipu_job_desc *udesc, int do_trigger);
	bool (*is_idle)(struct aipu_core *core);
	int  (*read_status_reg)(struct aipu_core *core);
	void (*print_hw_id_info)(struct aipu_core *core);
	int  (*io_rw)(struct aipu_core *core, struct aipu_io_req *io_req);
	int  (*upper_half)(void *data);
	void (*bottom_half)(void *data);
	void (*start_bw_profiling)(struct aipu_core *core);
	void (*stop_bw_profiling)(struct aipu_core *core);
	void (*read_profiling_reg)(struct aipu_core *core, struct aipu_ext_profiling_data *pdata);
	bool (*has_clk_ctrl)(struct aipu_core *core);
	void (*enable_clk_gating)(struct aipu_core *core);
	void (*disable_clk_gating)(struct aipu_core *core);
	bool (*is_clk_gated)(struct aipu_core *core);
	bool (*has_power_ctrl)(struct aipu_core *core);
	void (*power_on)(struct aipu_core *core);
	void (*power_off)(struct aipu_core *core);
	bool (*is_power_on)(struct aipu_core *core);
#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
	int  (*sysfs_show)(struct aipu_core *core, char *buf);
#endif
};

/**
 * struct aipu_core - a general struct describe a hardware AIPU core
 *
 * @param id:        AIPU core ID
 * @arch:            AIPU architecture number
 * @version:         AIPU hardware version number
 * @config:          AIPU hardware configuration number
 * @core_name:       AIPU core name string
 * @max_sched_num:   maximum number of jobs can be scheduled in pipeline
 * @dev:             device struct pointer
 * @reg_cnt:         IO region count, should be >= 1
 * @reg:             IO region array of this AIPU core
 * @interrupts:      interrupt registers IO region reference
 * @ops:             operations of this core
 * @irq_obj:         interrupt object of this core
 * @job_manager:     AIPU job manager
 * @reg_attr:        external register attribute
 * @clk_attr:        clock gating attribute
 * @pw_attr:         power control attribute
 * @disable_attr:    disable core attribute
 * @disable:         core disable flag (for debug usage)
 * @is_init:         init flag
 */
struct aipu_core {
	int id;
	int arch;
	int version;
	int config;
	char core_name[10];
	int max_sched_num;
	struct device *dev;
	int reg_cnt;
	struct io_region reg[MAX_IO_REGION_CNT];
	struct io_region *interrupts;
	struct aipu_core_operations *ops;
	struct aipu_irq_object *irq_obj;
	void *job_manager;
	struct device_attribute *reg_attr;
	struct device_attribute *clk_attr;
	struct device_attribute *pw_attr;
	struct device_attribute *disable_attr;
	atomic_t disable;
	int is_init;
};

/**
 * @brief init an AIPU core struct in driver probe phase
 *
 * @param core:        AIPU hardware core created in a calling function
 * @param version:     AIPU core hardware version
 * @param id:          AIPU core ID
 * @param job_manager: AIPU job manager
 * @param p_dev:       platform device struct pointer
 *
 * @return 0 if successful; others if failed;
 */
int init_aipu_core(struct aipu_core *core, int version, int id, void *job_manager,
	struct platform_device *p_dev);
/**
 * @brief deinit a created aipu_core struct
 *
 * @param core: AIPU hardware core created in a calling function
 */
void deinit_aipu_core(struct aipu_core *core);

#endif /* __AIPU_CORE_H__ */
