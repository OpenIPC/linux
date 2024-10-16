/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file zhouyi.h
 * Header of the zhouyi AIPU hardware control and interrupt handle operations
 */

#ifndef __AIPU_ZHOUYI_H__
#define __AIPU_ZHOUYI_H__

#include <linux/platform_device.h>
#include <linux/device.h>
#include "armchina_aipu.h"
#include "aipu_io.h"
#include "config.h"

/**
 * Zhouyi AIPU Common Interrupts
 */
#define ZHOUYI_IRQ_NONE   0x0
#define ZHOUYI_IRQ_QEMPTY 0x1
#define ZHOUYI_IRQ_DONE   0x2
#define ZHOUYI_IRQ_EXCEP  0x4

#define ZHOUYI_IRQ  (ZHOUYI_IRQ_QEMPTY | ZHOUYI_IRQ_DONE | ZHOUYI_IRQ_EXCEP)

#define ZHOUYI_AIPU_IDLE_STATUS   0x70000

/**
 * Zhouyi AIPU Common Host Control Register Map
 */
#define ZHOUYI_CTRL_REG_OFFSET                0x0
#define ZHOUYI_STAT_REG_OFFSET                0x4
#define ZHOUYI_START_PC_REG_OFFSET            0x8
#define ZHOUYI_INTR_PC_REG_OFFSET             0xC
#define ZHOUYI_IPI_CTRL_REG_OFFSET            0x10
#define ZHOUYI_DATA_ADDR_0_REG_OFFSET         0x14
#define ZHOUYI_DATA_ADDR_1_REG_OFFSET         0x18
#define ZHOUYI_CLK_CTRL_REG_OFFSET            0x3C
#define ZHOUYI_ISA_VERSION_REG_OFFSET         0x40
#define ZHOUYI_TPC_FEATURE_REG_OFFSET         0x44
#define ZHOUYI_SPU_FEATURE_REG_OFFSET         0x48
#define ZHOUYI_HWA_FEATURE_REG_OFFSET         0x4C
#define ZHOUYI_REVISION_ID_REG_OFFSET         0x50
#define ZHOUYI_MEM_FEATURE_REG_OFFSET         0x54
#define ZHOUYI_INST_RAM_FEATURE_REG_OFFSET    0x58
#define ZHOUYI_LOCAL_SRAM_FEATURE_REG_OFFSET  0x5C
#define ZHOUYI_GLOBAL_SRAM_FEATURE_REG_OFFSET 0x60
#define ZHOUYI_INST_CACHE_FEATURE_REG_OFFSET  0x64
#define ZHOUYI_DATA_CACHE_FEATURE_REG_OFFSET  0x68

/**
 * @brief read AIPU core status register
 *
 * @param io: core IO region pointer
 *
 * @return value if successful; 0 if failed;
 */
int zhouyi_read_status_reg(struct io_region *io);
/**
 * @brief clear AIPU core interrupt: queue empty
 *
 * @param io: core IO region pointer
 *
 * @return void
 */
void zhouyi_clear_qempty_interrupt(struct io_region *io);
/**
 * @brief clear AIPU core interrupt: job done
 *
 * @param io: core IO region pointer
 *
 * @return void
 */
void zhouyi_clear_done_interrupt(struct io_region *io);
/**
 * @brief clear AIPU core interrupt: job exception
 *
 * @param io: core IO region pointer
 *
 * @return void
 */
void zhouyi_clear_excep_interrupt(struct io_region *io);
/**
 * @brief request to do IO read/write operation
 *
 * @param io: core IO region pointer
 * @param io_req: struct comtains I/O requests info.
 *
 * @return void
 */
void zhouyi_io_rw(struct io_region *io, struct aipu_io_req *io_req);
/**
 * @brief detect and return AIPU core version
 *
 * @param p_dev:   platform device pointer
 * @param version: pointer to a space allocated by user to store the ISA version
 * @param config:  pointer to a space allocated by user to store the configuration
 *
 * @return version number if succssful; 0 if failed;
 */
int zhouyi_detect_aipu_version(struct platform_device *p_dev, int *version, int *config);
#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
/**
 * @brief print value of a register into a buffer
 *
 * @param io: core IO region pointer
 * @param buf: buffer allocated by caller and filled in this function
 * @param name: register name
 * @param offset: register offset
 *
 * @return written bytes
 */
int zhouyi_print_reg_info(struct io_region *io, char *buf, const char *name, int offset);
/**
 * @brief print zhouyi common register values into a buffer for sysfs usage
 *
 * @param io: core IO region pointer
 * @param buf: buffer allocated by caller and filled in this function
 *
 * @return written bytes
 */
int zhouyi_sysfs_show(struct io_region *io, char *buf);
#endif
/**
 * @brief get ISA version number read from AIPU register to AIPU HW version number
 *
 * @param io: core IO region pointer
 *
 * @return AIPU HW version number or 0 if failed
 */
int zhouyi_get_hw_version_number(struct io_region *io);
/**
 * @brief get AIPU hardware configuration version number
 *
 * @param io: core IO region pointer
 *
 * @return AIPU HW config number or 0 if failed
 */
int zhouyi_get_hw_config_number(struct io_region *io);

#endif /* __AIPU_ZHOUYI_H__ */
