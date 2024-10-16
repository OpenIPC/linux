/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

#ifndef __UAPI_MISC_ARMCHINA_AIPU_H__
#define __UAPI_MISC_ARMCHINA_AIPU_H__

#include <linux/types.h>
#include <linux/ioctl.h>

/*
 * In the following member descriptions,
 * [must]  mean that the fields must be filled by user mode driver any way.
 * [alloc] mean that the buffer(s) represented by the fields must be allocated
 *         by user mode driver before calling IOCTL.
 * [kmd]   mean that the fields should be filled by kernel mode driver
 *         if the calls are successfull.
 */

/**
 * emum aipu_arch - AIPU architecture number
 * @AIPU_ARCH_ZHOUYI: AIPU architecture is Zhouyi.
 *
 * This enum is used to indicate the architecture of an AIPU core in the system.
 */
enum aipu_arch {
	AIPU_ARCH_ZHOUYI = 0,
};

/**
 * emum aipu_isa_version - AIPU ISA version number
 * @AIPU_ISA_VERSION_ZHOUYI_V1: AIPU ISA version is Zhouyi v1.
 * @AIPU_ISA_VERSION_ZHOUYI_V2: AIPU ISA version is Zhouyi v2.
 *
 * Zhouyi architecture has multiple ISA versions released.
 * This enum is used to indicate the ISA version of an AIPU core in the system.
 */
enum aipu_isa_version {
	AIPU_ISA_VERSION_ZHOUYI_V1 = 1,
	AIPU_ISA_VERSION_ZHOUYI_V2 = 2,
};

/**
 * struct aipu_core_cap - Capability of an AIPU core
 * @core_id: [kmd] Core ID
 * @arch:    [kmd] Architecture number
 * @version: [kmd] ISA version number
 * @config:  [kmd] Configuration number
 * @info:    [kmd] Debugging information
 *
 * For example, Z2-1104:
 *    arch == AIPU_ARCH_ZHOUYI (0)
 *    version == AIPU_ISA_VERSION_ZHOUYI_V2 (2)
 *    config == 1104
 */
struct aipu_core_cap {
	__u32 core_id;
	__u32 arch;
	__u32 version;
	__u32 config;
	struct aipu_debugger_info {
		__u64 reg_base;	/* External register base address (physical) */
	} info;
};

/**
 * struct aipu_cap - Common capability of the AIPU core(s)
 * @core_cnt:            [kmd] Count of AIPU core(s) in the system
 * @host_to_aipu_offset: [kmd] Address space offset between host and AIPU
 * @is_homogeneous:      [kmd] IS homogeneous AIPU system or not (1/0)
 * @core_cap:            [kmd] Capability of the single AIPU core
 *
 * AIPU driver supports the management of multiple AIPU cores in the system.
 * This struct is used to indicate the common capability of all AIPU core(s).
 * User mode driver should get this capability via AIPU_IOCTL_QUERYCAP command.
 * If the core count is 1, the per-core capability is in the core_cap member;
 * otherwise user mode driver should get all the per-core capabilities as the
 * core_cnt indicates via AIPU_IOCTL_QUERYCORECAP command.
 */
struct aipu_cap {
	__u32 core_cnt;
	__u64 host_to_aipu_offset;
	__u32 is_homogeneous;
	struct aipu_core_cap core_cap;
};

/**
 * enum aipu_mm_data_type - Data/Buffer type
 * @AIPU_MM_DATA_TYPE_NONE:   No type
 * @AIPU_MM_DATA_TYPE_TEXT:   Text (instructions)
 * @AIPU_MM_DATA_TYPE_RODATA: Read-only data (parameters)
 * @AIPU_MM_DATA_TYPE_STACK:  Stack
 * @AIPU_MM_DATA_TYPE_STATIC: Static data (weights)
 * @AIPU_MM_DATA_TYPE_REUSE:  Reuse data (feature maps)
 */
enum aipu_mm_data_type {
	AIPU_MM_DATA_TYPE_NONE,
	AIPU_MM_DATA_TYPE_TEXT,
	AIPU_MM_DATA_TYPE_RODATA,
	AIPU_MM_DATA_TYPE_STACK,
	AIPU_MM_DATA_TYPE_STATIC,
	AIPU_MM_DATA_TYPE_REUSE,
};

/**
 * struct aipu_buf_desc - Buffer description.
 * @pa:         [kmd] Buffer physical base address
 * @dev_offset: [kmd] Device offset used in mmap
 * @bytes:      [kmd] Buffer size in bytes
 */
struct aipu_buf_desc {
	__u64 pa;
	__u64 dev_offset;
	__u64 bytes;
};

/**
 * struct aipu_buf_request - Buffer allocation request structure.
 * @bytes:         [must] Buffer size to allocate (in bytes)
 * @align_in_page: [must] Buffer address alignment (in page, i.e. 4KB)
 * @data_type:     [must] Type of data in this buffer/Type of this buffer
 * @desc:          [kmd]  Descriptor of the successfully allocated buffer
 */
struct aipu_buf_request {
	__u64 bytes;
	__u32 align_in_page;
	__u32 data_type;
	struct aipu_buf_desc desc;
};

/**
 * enum aipu_job_execution_flag - Flags for AIPU's executions
 * @AIPU_JOB_EXEC_FLAG_NONE:       No flag
 * @AIPU_JOB_EXEC_FLAG_SRAM_MUTEX: The job uses SoC SRAM exclusively.
 */
enum aipu_job_execution_flag {
	AIPU_JOB_EXEC_FLAG_NONE = 0x0,
	AIPU_JOB_EXEC_FLAG_SRAM_MUTEX = 0x1,
};

/**
 * struct aipu_job_desc - Description of a job to be scheduled.
 * @is_defer_run:      Reserve an AIPU core for this job and defer the running of it
 * @version_compatible:Is this job compatible on AIPUs with different ISA version
 * @core_id:           ID of the core requested to reserve for the deferred job
 * @do_trigger:        Trigger the previously scheduled deferred job to run
 * @aipu_arch:         [must] Target device architecture
 * @aipu_version:      [must] Target device ISA version
 * @aipu_config:       [must] Target device configuration
 * @start_pc_addr:     [must] Address of the start PC
 * @intr_handler_addr: [must] Address of the AIPU interrupt handler
 * @data_0_addr:       [must] Address of the 0th data buffer
 * @data_1_addr:       [must] Address of the 1th data buffer
 * @job_id:            [must] ID of this job
 * @enable_prof:       Enable performance profiling counters in SoC (if any)
 * @enable_asid:       Enable ASID feature
 * @enable_poll_opt:   Enable optimizations for job status polling
 * @exec_flag:         Combinations of execution flags
 *
 * For fields is_defer_run/do_trigger/enable_prof/enable_asid/enable_poll_opt,
 * set them to be 1/0 to enable/disable the corresponding operations.
 */
struct aipu_job_desc {
	__u32 is_defer_run;
	__u32 version_compatible;
	__u32 core_id;
	__u32 do_trigger;
	__u32 aipu_arch;
	__u32 aipu_version;
	__u32 aipu_config;
	__u64 start_pc_addr;
	__u64 intr_handler_addr;
	__u64 data_0_addr;
	__u64 data_1_addr;
	__u32 job_id;
	__u32 enable_prof;
	__u32 enable_asid;
	__u32 enable_poll_opt;
	__u32 exec_flag;
};

/**
 * struct aipu_job_status_desc - Jod execution status.
 * @job_id:    [kmd] Job ID
 * @thread_id: [kmd] ID of the thread scheduled this job
 * @state:     [kmd] Execution state: done or exception
 * @pdata:     [kmd] External profiling results
 */
struct aipu_job_status_desc {
	__u32 job_id;
	__u32 thread_id;
#define AIPU_JOB_STATE_DONE      0x1
#define AIPU_JOB_STATE_EXCEPTION 0x2
	__u32 state;
	struct aipu_ext_profiling_data {
		__s64 execution_time_ns; /* [kmd] Execution time */
		__u32 rdata_tot_msb;     /* [kmd] Total read transactions (MSB) */
		__u32 rdata_tot_lsb;     /* [kmd] Total read transactions (LSB) */
		__u32 wdata_tot_msb;     /* [kmd] Total write transactions (MSB) */
		__u32 wdata_tot_lsb;     /* [kmd] Total write transactions (LSB) */
		__u32 tot_cycle_msb;     /* [kmd] Total cycle counts (MSB) */
		__u32 tot_cycle_lsb;     /* [kmd] Total cycle counts (LSB) */
	} pdata;
};

/**
 * struct aipu_job_status_query - Query status of (a) job(s) scheduled before.
 * @max_cnt:        [must] Maximum number of job status to query
 * @of_this_thread: [must] Get status of jobs scheduled by this thread/all threads share fd (1/0)
 * @status:         [alloc] Pointer to an array (length is max_cnt) to store the status
 * @poll_cnt:       [kmd] Count of the successfully polled job(s)
 */
struct aipu_job_status_query {
	__u32 max_cnt;
	__u32 of_this_thread;
	struct aipu_job_status_desc *status;
	__u32 poll_cnt;
};

struct aipu_job_status_query_32 {
	__u32 max_cnt;
	__u32 of_this_thread;
	compat_caddr_t status;
	__u32 poll_cnt;
};

/**
 * struct aipu_io_req - AIPU core IO operations request.
 * @core_id: [must] Core ID
 * @offset:  [must] Register offset
 * @rw:      [must] Read or write operation
 * @value:   [must]/[kmd] Value to be written/value readback
 */
struct aipu_io_req {
	__u32 core_id;
	__u32 offset;
	enum aipu_rw_attr {
		AIPU_IO_READ,
		AIPU_IO_WRITE
	} rw;
	__u32 value;
};

/*
 * AIPU IOCTL List
 */
#define AIPU_IOCTL_MAGIC 'A'
/**
 * DOC: AIPU_IOCTL_QUERY_CAP
 *
 * @Description
 *
 * ioctl to query the common capability of AIPUs
 *
 * User mode driver should call this before calling AIPU_IOCTL_QUERYCORECAP.
 */
#define AIPU_IOCTL_QUERY_CAP _IOR(AIPU_IOCTL_MAGIC,  0, struct aipu_cap)
/**
 * DOC: AIPU_IOCTL_QUERY_CORE_CAP
 *
 * @Description
 *
 * ioctl to query the capability of an AIPU core
 *
 * User mode driver only need to call this when the core count returned by AIPU_IOCTL_QUERYCAP > 1.
 */
#define AIPU_IOCTL_QUERY_CORE_CAP _IOR(AIPU_IOCTL_MAGIC,  1, struct aipu_core_cap)
/**
 * DOC: AIPU_IOCTL_REQ_BUF
 *
 * @Description
 *
 * ioctl to request to allocate a coherent buffer
 *
 * This fails if kernel driver cannot find a free buffer meets the size/alignment request.
 */
#define AIPU_IOCTL_REQ_BUF _IOWR(AIPU_IOCTL_MAGIC, 2, struct aipu_buf_request)
/**
 * DOC: AIPU_IOCTL_FREE_BUF
 *
 * @Description
 *
 * ioctl to request to free a coherent buffer allocated by AIPU_IOCTL_REQBUF
 *
 */
#define AIPU_IOCTL_FREE_BUF _IOW(AIPU_IOCTL_MAGIC,  3, struct aipu_buf_desc)
/**
 * DOC: AIPU_IOCTL_DISABLE_SRAM
 *
 * @Description
 *
 * ioctl to disable the management of SoC SRAM in kernel driver
 *
 * This fails if the there is no SRAM in the system or the SRAM has already been allocated.
 */
#define AIPU_IOCTL_DISABLE_SRAM _IO(AIPU_IOCTL_MAGIC,  4)
/**
 * DOC: AIPU_IOCTL_ENABLE_SRAM
 *
 * @Description
 *
 * ioctl to enable the management of SoC SRAM in kernel driver disabled by AIPU_IOCTL_DISABLE_SRAM
 */
#define AIPU_IOCTL_ENABLE_SRAM _IO(AIPU_IOCTL_MAGIC,  5)
/**
 * DOC: AIPU_IOCTL_SCHEDULE_JOB
 *
 * @Description
 *
 * ioctl to schedule a user job to kernel mode driver for execution
 *
 * This is a non-blocking operation therefore user mode driver should check the job status
 * via AIPU_IOCTL_QUERY_STATUS.
 */
#define AIPU_IOCTL_SCHEDULE_JOB _IOW(AIPU_IOCTL_MAGIC,  6, struct aipu_job_desc)
/**
 * DOC: AIPU_IOCTL_QUERY_STATUS
 *
 * @Description
 *
 * ioctl to query the execution status of one or multiple scheduled job(s)
 */
#define AIPU_IOCTL_QUERY_STATUS _IOWR(AIPU_IOCTL_MAGIC, 7, struct aipu_job_status_query)
#define AIPU_IOCTL_QUERY_STATUS_32 _IOWR(AIPU_IOCTL_MAGIC, 7, struct aipu_job_status_query_32)
/**
 * DOC: AIPU_IOCTL_KILL_TIMEOUT_JOB
 *
 * @Description
 *
 * ioctl to kill a timeout job and clean it from kernel mode driver.
 */
#define AIPU_IOCTL_KILL_TIMEOUT_JOB _IOW(AIPU_IOCTL_MAGIC,  8, __u32)
/**
 * DOC: AIPU_IOCTL_REQ_IO
 *
 * @Description
 *
 * ioctl to read/write an external register of an AIPU core.
 */
#define AIPU_IOCTL_REQ_IO _IOWR(AIPU_IOCTL_MAGIC, 9, struct aipu_io_req)

#endif /* __UAPI_MISC_ARMCHINA_AIPU_H__ */
