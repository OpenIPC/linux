/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_RISCV_UTILS_H__
#define __AX_RISCV_UTILS_H__

#include <linux/types.h>

#define RISCV_DTS_NODE_LOG_MEM          "/reserved-memory/riscv_log_memreserved"
#define RISCV_DTS_NODE_RISCV            "/reserved-memory/riscv_memreserved"
#define RISCV_DTS_NODE_RAMDISK_HEADER   "/reserved-memory/ramdisk_header_memreserved"

#define RISCV_SW_INT_GROUP(n)      n
#define RISCV_SW_INT_CHANNEL(n)    n
#define INTERRUPT_VECTOR_SW_INT_GROUP_3 34

int ax_riscv_utils_sw_int_trigger(u32 group, u32 channel);
int ax_riscv_utils_interrupt_umask(int vector);
int ax_riscv_utils_get_dts_reg(const char *dts_path, u64 *addr, u64 *size);
u64 ax_riscv_utils_get_microseconds(void);

#endif //__AX_RISCV_UTILS_H__
