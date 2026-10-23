// SPDX-License-Identifier: GPL-2.0
/****************************************************************************
 *
 *    The MIT License (MIT)
 *
 *    COPYRIGHT (C) 2019 VERISILICON ALL RIGHTS RESERVED
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a
 *    copy of this software and associated documentation files (the "Software"),
 *    to deal in the Software without restriction, including without limitation
 *    the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *    and/or sell copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *    DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 *    The GPL License (GPL)
 *
 *    COPYRIGHT (C) 2019 VERISILICON ALL RIGHTS RESERVED
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************
 *
 *    Note: This software is released under dual MIT and GPL licenses. A
 *    recipient may use this file under the terms of either the MIT license or
 *    GPL License. If you wish to use only one license not the other, you can
 *    indicate your decision by deleting one of the above license notices in your
 *    version of this file.
 *
 *****************************************************************************
 */
/*
 * ESWIN cipher serivce driver
 *
 * Copyright 2024, Beijing ESWIN Computing Technology Co., Ltd.. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Xiaojun Zou <zouxiaojun@eswincomputing.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
/* needed for __init,__exit directives */
#include <linux/init.h>
/* needed for remap_page_range
 *   SetPageReserved
 *   ClearPageReserved
 */
#include <linux/mm.h>
/* obviously, for kmalloc */
#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>

#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/moduleparam.h>
/* request_irq(), free_irq() */
#include <linux/interrupt.h>
#include <linux/sched.h>

#include <linux/semaphore.h>
#include <linux/spinlock.h>
/* needed for virt_to_phys() */
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>

#include <asm/irq.h>

#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/kthread.h>

/* device tree access */
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
/* AX630C-PORT: dma-heap import helper is an ESWIN/5.6+ out-of-tree header; the
 * AX630C 4.19 build has no dma-heap and drives the VCMD pools from a coherent
 * DMA allocation, so gate the include on SUPPORT_DMA_HEAP (undefined here). */
#ifdef SUPPORT_DMA_HEAP
#include <linux/dmabuf-heap-import-helper.h>
#endif

/* our own stuff */
#include <linux/platform_device.h>
#include "vcmdswhwregisters.h"
#include "bidirect_list.h"
#include "vc8000_driver.h"
#include "vc8000_vcmd_cfg.h"

/* AX630C-PORT: access_ok() lost its first (type) argument in Linux 5.0
 * (commit 96d4f267e40f). The eswin 6.6 source uses the 2-arg form; provide a
 * 2-arg wrapper that maps to the 3-arg form on pre-5.0 (our 4.19) kernels. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
#define AX_ACCESS_OK(addr, size) access_ok(VERIFY_WRITE, (addr), (size))
#else
#define AX_ACCESS_OK(addr, size) access_ok((addr), (size))
#endif

/* AX630C-PORT: arm64 4.19 has no pfn_to_phys(); it is the generic PFN_PHYS(). */
#include <linux/pfn.h>
#ifndef pfn_to_phys
#define pfn_to_phys(pfn) PFN_PHYS(pfn)
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG  VENC_DEV_NAME ":vc"
#include "vc_drv_log.h"
#include "../framebuf_alloc.h"   /* AX630C-PORT: from-source frame-buffer allocator (#45) */

/*these size need to be modified according to hw config.*/
#define VCMD_ENCODER_REGISTER_SIZE              (ENCODER_REGISTER_SIZE * 4)
#define VCMD_DECODER_REGISTER_SIZE              (DECODER_REGISTER_SIZE * 4)
#define VCMD_IM_REGISTER_SIZE                   (IM_REGISTER_SIZE * 4)
#define VCMD_JPEG_ENCODER_REGISTER_SIZE         (JPEG_ENCODER_REGISTER_SIZE * 4)
#define VCMD_JPEG_DECODER_REGISTER_SIZE         (JPEG_DECODER_REGISTER_SIZE * 4)

#define MAX_VCMD_NUMBER (MAX_VCMD_TYPE * MAX_SAME_MODULE_TYPE_CORE_NUMBER) //

#define HW_WORK_STATE_IDLE                0
#define HW_WORK_STATE_PEND                3

#define MAX_CMDBUF_INT_NUMBER             1
#define INT_MIN_SUM_OF_IMAGE_SIZE_DEF     (4096 * 2160 * 1 * MAX_CMDBUF_INT_NUMBER)
#define INT_MIN_SUM_OF_IMAGE_SIZE_MAX     (INT_MIN_SUM_OF_IMAGE_SIZE_DEF * 32)
#define MAX_PROCESS_CORE_NUMBER           (4 * 8)
#define PROCESS_MAX_VIDO_SIZE             (4096 * 2160 * MAX_SAME_MODULE_TYPE_CORE_NUMBER * MAX_PROCESS_CORE_NUMBER)
#define PROCESS_MAX_JPEG_SIZE             (32768ULL*32768ULL*(2+1)) //32K*32K*(2(a pic per core)+1(extra a pic for a proc))
#define PROCESS_MAX_SUM_OF_IMAGE_SIZE                         \
                    (PROCESS_MAX_VIDO_SIZE > PROCESS_MAX_JPEG_SIZE ? PROCESS_MAX_VIDO_SIZE : PROCESS_MAX_JPEG_SIZE)

/** the wait time for the vcmd task list(in milliseconds):
 * 200x frame encoding time + 1000ms
 * = size*200*1000/800000000 + 1000
*/
#define ENC_VCMD_WAIT_TIME(size)          (size/4000 + 1000)

#define MAX_SAME_MODULE_TYPE_CORE_NUMBER  4


/*pcie address need to subtract this value then can be put to register*/
static size_t base_ddr_addr;

#ifdef HANTROAXIFE_SUPPORT
#define AXIFE_SIZE (64 * 4)
volatile u8 *axife_hwregs[MAX_VCMD_NUMBER][2];
#endif

/* vcmd remained DDR: 16M */
#ifndef HLINA_TRANSL_VCMD_SIZE
#define HLINA_TRANSL_VCMD_SIZE          0x1000000
#endif

static const unsigned int mmu_enable = 0;
#define DRIVER_NAME "es_venc"

#ifndef VENC_CLASS_NAME
# define VENC_CLASS_NAME "es_venc_class"
#endif
static struct class *venc_class = NULL;

/********variables declaration related with race condition**********/

#define CMDBUF_MAX_SIZE       (512 * 4 * 4)

#define CMDBUF_POOL_TOTAL_SIZE            (2 * 1024 * 1024) //approximately=128x(320x240)=128x2k=128x8kbyte=1Mbytes
#define TOTAL_DISCRETE_CMDBUF_NUM         (CMDBUF_POOL_TOTAL_SIZE / CMDBUF_MAX_SIZE)
#define CMDBUF_VCMD_REGISTER_TOTAL_SIZE   (9 * 1024 * 1024 - CMDBUF_POOL_TOTAL_SIZE * 2)
#define VCMD_REGISTER_SIZE                (128 * 4)

struct noncache_mem {
	u32 *virtualAddress;
	dma_addr_t busAddress;
	dma_addr_t phy_address; /* memory bus address - send to userspace to do mmap, better to use dma_mmap_coherent() */
	size_t mmu_bus_address; /* buffer physical address in MMU*/
	u32 size;
	u16 cmdbuf_id;
};

struct process_manager_obj {
	struct file *filp;
	u64 total_exe_time;
	spinlock_t spinlock;
	wait_queue_head_t wait_queue;
};

struct cmdbuf_obj {
	u32 module_type; //current CMDBUF type: input vc8000e=0,IM=1,vc8000d=2,jpege=3, jpegd=4
	u32 priority; //current CMDBUFpriority: normal=0, high=1
	u64 executing_time; //current CMDBUFexecuting_time=encoded_image_size*(rdoLevel+1)*(rdoq+1);
	u32 cmdbuf_size; //current CMDBUF size
	u32 *cmdbuf_virtualAddress; //current CMDBUF start virtual address.
	size_t cmdbuf_busAddress; //current CMDBUF start physical address.
	size_t mmu_cmdbuf_busAddress; //current CMDBUF start mmu mapping address.
	u32 *status_virtualAddress; //current status CMDBUF start virtual address.
	size_t status_busAddress; //current status CMDBUF start physical address.
	size_t mmu_status_busAddress; //current status CMDBUF start mmu mapping address.
	u32 status_size; //current status CMDBUF size
	u32 executing_status; //current CMDBUF executing status.
	struct file *filp; //file pointer in the same process.
	u16 core_id; //which vcmd core is used.
	u16 cmdbuf_id; //used to manage CMDBUF in driver.It is a handle to identify cmdbuf.also is an interrupt vector.position in pool,same as status position.
	u8 cmdbuf_data_loaded; //0 means sw has not copied data into this CMDBUF; 1 means sw has copied data into this CMDBUF
	u8 cmdbuf_data_linked; //0 :not linked, 1:linked into a vcmd core list.
	u8 cmdbuf_run_done; //if 0,waiting for CMDBUF finish; if 1, op code in CMDBUF has finished one by one. HANTRO_VCMD_IOCH_WAIT_CMDBUF will check this variable.
	u8 cmdbuf_need_remove; // if 0, not need to remove CMDBUF; 1 CMDBUF can be removed if it is not the last CMDBUF;
	u8 has_end_cmdbuf; //if 1, the last opcode is end opCode.
	u8 no_normal_int_cmdbuf; //if 1, JMP will not send normal interrupt.
	struct process_manager_obj *process_manager_obj;
};

struct hantrovcmd_dev {
	struct vcmd_config vcmd_core_cfg; //config of each core,such as base addr, irq,etc
	u32 core_id; //vcmd core id for driver and sw internal use
	u32 sw_cmdbuf_rdy_num;
	spinlock_t *spinlock;
	wait_queue_head_t *wait_queue;
	wait_queue_head_t *wait_abort_queue;
	bi_list list_manager;

	volatile u8 *hwregs; /* IO mem base */
	u32 reg_mirror[ASIC_VCMD_SWREG_AMOUNT];
	u32 duration_without_int; //number of cmdbufs without interrupt.
	u32 duration_without_int_max;

	volatile u8 working_state;
	u64 total_exe_time;
	u16 status_cmdbuf_id; //used for analyse configuration in cwl.
	u32 hw_version_id; /*megvii 0x43421001, later 0x43421102*/
	u32 *vcmd_reg_mem_virtualAddress; //start virtual address of vcmd registers memory of  CMDBUF.
	size_t vcmd_reg_mem_busAddress; //start physical address of vcmd registers memory of  CMDBUF.

	unsigned int mmu_vcmd_reg_mem_busAddress; //start mmu mapping address of vcmd registers memory of CMDBUF.
	u32 vcmd_reg_mem_size; // size of vcmd registers memory of CMDBUF.

	// watchdog
	struct timer_list watchdog_timer;
	u8 watchdog_active;
	u8 watchdog_triggered;
	u32 vce_hang;
	u32 restart_cmdbuf_id;

	/* status statistics*/
	atomic64_t core_tot_cycles;
	atomic64_t core_tot_exetime;

	int software_triger_abort;
};

/*
 * Ioctl definitions
 */
#define VCMD_HW_ID                  0x4342



static struct noncache_mem vcmd_buf_mem_pool;
static struct noncache_mem vcmd_status_buf_mem_pool;
static struct noncache_mem vcmd_registers_mem_pool;

static u16 cmdbuf_used[TOTAL_DISCRETE_CMDBUF_NUM];
static u16 cmdbuf_used_pos;
static u16 cmdbuf_used_residual;

static struct hantrovcmd_dev *vcmd_manager[MAX_VCMD_TYPE][MAX_VCMD_NUMBER];
static bi_list_node *global_cmdbuf_node[TOTAL_DISCRETE_CMDBUF_NUM];

static bi_list global_process_manager;

static u16 vcmd_position[MAX_VCMD_TYPE];
static int vcmd_type_core_num[MAX_VCMD_TYPE];

#define EXECUTING_CMDBUF_ID_ADDR       26
#define VCMD_EXE_CMDBUF_COUNT           3

#define WORKING_STATE_IDLE          0
#define WORKING_STATE_WORKING       1
#define WORKING_STATE_STALL         2
#define CMDBUF_EXE_STATUS_OK        0
#define CMDBUF_EXE_STATUS_CMDERR        1
#define CMDBUF_EXE_STATUS_BUSERR        2

static struct semaphore vcmd_reserve_cmdbuf_sem[MAX_VCMD_TYPE]; //for reserve

//#define VCMD_DEBUG_INTERNAL

/***************************TYPE AND FUNCTION DECLARATION****************/

/* here's all the must remember stuff */

static int vcmd_reserve_IO(void);
static void vcmd_release_IO(void);
static void vcmd_reset_asic(struct hantrovcmd_dev *dev);
static void vcmd_reset_current_asic(struct hantrovcmd_dev *dev);
static int allocate_cmdbuf(struct noncache_mem *new_cmdbuf_addr,
			   struct noncache_mem *new_status_cmdbuf_addr);
static void vcmd_link_cmdbuf(struct hantrovcmd_dev *dev,
			     bi_list_node *last_linked_cmdbuf_node);
static void vcmd_start(struct hantrovcmd_dev *dev,
		       bi_list_node *first_linked_cmdbuf_node);
static int create_kernel_process_manager(void);

static irqreturn_t hantrovcmd_isr(int irq, void *dev_id);

/** pm runtime sync & put*/
extern int enc_pm_runtime_get(u32 core_id);
extern int enc_pm_runtime_put(u32 core_id);
/** system reset*/
extern int enc_reset_system(u32 core_id);

#ifdef VCMD_DEBUG_INTERNAL
static void printk_vcmd_register_debug(const void *hwregs, char *info);
static void PrintInstr(u32 i, u32 instr, u32 *size);
#endif

/*********************local variable declaration*****************/
//static unsigned long vcmd_sram_base;
//static unsigned int vcmd_sram_size;
/* and this is our MAJOR; use 0 for dynamic allocation (recommended)*/
static int hantrovcmd_major;
int venc_vcmd_core_num = 0;
/* dynamic allocation*/
static struct hantrovcmd_dev *hantrovcmd_data;

// kernel thread
struct task_struct *kthread;
u8 stop_kthread;
wait_queue_head_t kthread_waitq;

#ifdef IRQ_SIMULATION
struct timer_manager {
	u32 core_id; //vcmd core id for driver and sw internal use
	u32 timer_id;
	struct timer_list *timer;
};

static struct timer_list timer[10000];
struct timer_manager timer_reserve[10000];
#endif

//hw_queue can be used for reserve cmdbuf memory
static DECLARE_WAIT_QUEUE_HEAD(vcmd_cmdbuf_memory_wait);

static DEFINE_SPINLOCK(vcmd_cmdbuf_alloc_lock);
static DEFINE_SPINLOCK(vcmd_process_manager_lock);

static spinlock_t owner_lock_vcmd[MAX_VCMD_NUMBER];

static wait_queue_head_t wait_queue_vcmd[MAX_VCMD_NUMBER];

static wait_queue_head_t abort_queue_vcmd[MAX_VCMD_NUMBER];

/**
 * @brief hook function for system-driver to do further process
 *  for abort vcmd failed.
 */
static void hook_vcmd_abort_failed(struct hantrovcmd_dev *dev)
{
	/** wait_event_xxx could not be interrupt by pm suspend*/
	LOG_ERR("dev[%d]: vcmd abort failed, need to do further process\n", dev->core_id);
}

/**
 * @brief wait jobs count that need to be finished from dev list
 */
static u32 dev_wait_job_exetime(struct hantrovcmd_dev *dev)
{
	struct cmdbuf_obj *obj;
	bi_list_node *node = dev->list_manager.head;
	u32 exttime = 0;

	while (node) {
		obj = (struct cmdbuf_obj *)node->data;
		if (obj->cmdbuf_run_done == 0) {
			exttime += obj->executing_time;
			if (obj->has_end_cmdbuf == 1 || obj->no_normal_int_cmdbuf == 0) {
				return exttime;
			}
		}
		node = node->next;
	}

	return exttime;
}

/**
 * check if hang after receive JMP interrupt
 */
static int check_vce_hang(struct hantrovcmd_dev *dev, u32 cmdbuf_id)
{
	u32 *status_virt;

	if (dev->vcmd_core_cfg.sub_module_type != VCMD_TYPE_ENCODER)
		return 0;

	status_virt = vcmd_status_buf_mem_pool.virtualAddress + \
				cmdbuf_id*CMDBUF_MAX_SIZE / 4 + \
				(dev->vcmd_core_cfg.submodule_main_addr / 2 / 4 + 0);

	if (*(status_virt + 217) != *(status_virt + 218)) {
		LOG_INFO("check vce hang, cmdbuf_id=%u, swreg217=0x%08x, swreg218=0x%08x\n"
			, cmdbuf_id, status_virt[217], status_virt[218]);

		return 1;
	}

	return 0;
}

/**
 * @brief abort vcmd
 */
static int vcmd_abort(struct hantrovcmd_dev *dev)
{
	unsigned long flags;
	u32 state;
	u8 pend_state;

	state = vcmd_get_register_value((const void *)dev->hwregs,
		dev->reg_mirror, HWIF_VCMD_WORK_STATE);
	pend_state = (state == HW_WORK_STATE_PEND) ? 1 : 0;
	LOG_INFO("vcmd_abort, start vcmd aborting, core_id = %u, vcmd state = %u, vce_hang = %u, working_state = %u\n"
		, dev->core_id, state, dev->vce_hang, dev->working_state);
	spin_lock_irqsave(dev->spinlock, flags);
	if (state == HW_WORK_STATE_IDLE) {
		dev->vce_hang = 0;
		spin_unlock_irqrestore(dev->spinlock, flags);
		return 0;
	}
	if (dev->vce_hang)
		vcmd_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_ABORT_MODE, 0x1);
	else
		vcmd_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_ABORT_MODE, 0x0);

	dev->software_triger_abort = 1;
	vcmd_write_register_value((const void *)dev->hwregs,
			dev->reg_mirror,
			HWIF_VCMD_START_TRIGGER, 0);
	spin_unlock_irqrestore(dev->spinlock, flags);

#ifdef VCMD_POLLING_ISR
	int loop_cnt = 0;

	do {
		state = vcmd_get_register_value((const void *)dev->hwregs,
										dev->reg_mirror, HWIF_VCMD_WORK_STATE);
		if (state == HW_WORK_STATE_IDLE) {
			if (pend_state)
			hantrovcmd_isr(0, dev);
			//aborted
			break;
		}
		mdelay(10); // wait 10ms
	} while (++loop_cnt < 100);

	if (loop_cnt < 100) {
		mdelay(10);
		dev->software_triger_abort = 0;
		return 0;
	}
	dev->software_triger_abort = 0;
	LOG_ERR("%s, can't go to IDLE, need to re-power sub-system!\n", __func__);
	return -1;
#else
	if (wait_event_interruptible(*dev->wait_abort_queue,
		(dev->working_state == WORKING_STATE_IDLE))) {
		dev->software_triger_abort = 0;
		LOG_ERR("vcmd_abort: wait_abort_queue is signaled!!! software_triger_abort_ = 0\n");
		return -ERESTARTSYS;
	}
	LOG_INFO("vcmd_abort, vcmd abort completed, core_id = %u, state = %u, software_triger_abort_ = 0\n"
		, dev->core_id, dev->working_state);

	dev->software_triger_abort = 0;
	return 0;
#endif
}

/**
 * @brief stop vcmd when hang
*/
static int stop_vcmd(struct hantrovcmd_dev *dev)
{
	int ret = 0;

	// abort vcmd of vce immediately
	ret = vcmd_abort(dev);
	if (ret < 0)
		hook_vcmd_abort_failed(dev);

	return ret;
}

/**
 * @brief reset system after vce vcmd abort
 */
static void reset_system(struct hantrovcmd_dev *dev)
{
    LOG_INFO("reset vce system, core_id = %u, working_state = %u!\n"
		, dev->core_id, dev->working_state);
	if (dev->working_state == HW_WORK_STATE_IDLE) {
        int ret = enc_reset_system(dev->core_id);
		LOG_INFO("reset vce system completed! core_id = %u, ret = %d\n"
			, dev->core_id, ret);
	}
}

/**
 * restart vcmd for re-encode
 */
static void restart_vcmd(struct hantrovcmd_dev *dev)
{
	bi_list_node *restart_node = NULL;

	LOG_INFO("restart_vcmd, restart_cmdbuf_id = %u\n", dev->restart_cmdbuf_id);
	/* restart VCE vcmd */
	if (dev->restart_cmdbuf_id != 0XFFFF) {
		unsigned long flags = 0;

		spin_lock_irqsave(dev->spinlock, flags);
		restart_node = global_cmdbuf_node[dev->restart_cmdbuf_id];
		if (restart_node == NULL)
			restart_node = dev->list_manager.head;
		while (restart_node &&
			((struct cmdbuf_obj *)restart_node->data)->cmdbuf_run_done)
			restart_node = restart_node->next;
		if (restart_node) {
			LOG_INFO("restart_vcmd, restart_node cmdbuf_id = %u, cmdbuf_run_done = %u"
				, ((struct cmdbuf_obj *)restart_node->data)->cmdbuf_id
				, ((struct cmdbuf_obj *)restart_node->data)->cmdbuf_run_done);
		} else {
			LOG_INFO("restart_vcmd, no restart_node found, make vcmd idle\n");
		}
		LOG_INFO("restart_vcmd, sw_cmdbuf_rdy_num = %u, working_state = %u\n"
			, dev->sw_cmdbuf_rdy_num, dev->working_state);
		if (dev->sw_cmdbuf_rdy_num == 0)
			vcmd_link_cmdbuf(dev, restart_node);
		if (dev->sw_cmdbuf_rdy_num != 0)
			vcmd_start(dev, restart_node);
		dev->restart_cmdbuf_id = 0xFFFF;

		spin_unlock_irqrestore(dev->spinlock, flags);
		LOG_INFO("restart_vcmd, restart vcmd completed\n");
	}
}

/**
 * vce hang process
 */
static void vce_hang_process(struct hantrovcmd_dev *dev)
{
	int ret = 0;
    LOG_INFO("vce hang process start\n");
	/* abort VCE vcmd */
	ret = stop_vcmd(dev);
	if (ret == 0) {
		/* do system reset */
		reset_system(dev);
		/* re-encode */
		restart_vcmd(dev);
	} else {
		LOG_ERR("vce hang process failed, failed to stop vcmd\n");
	}
	LOG_INFO("vce hang process completed\n");
}

#ifdef SUPPORT_WATCHDOG
/**
 * @brief process when watchdog triggered.
 */
static void _vcmd_watchdog_process(struct hantrovcmd_dev *dev)
{
	if (dev->vce_hang)
		vce_hang_process(dev);
}
#endif

/**
 * @brief To check dev's actions which need kthread to process
 * @return int: 0: no actions; 1: have actions
 */
static int _vcmd_kthread_actions(struct hantrovcmd_dev **dev)
{
	int ret = 0, i;

	if (stop_kthread == 1) {
		return 1;
	}

	for (i = 0; i < venc_vcmd_core_num; i++) {
		*dev = &hantrovcmd_data[i];
		if ((*dev)->vce_hang
#ifdef SUPPORT_WATCHDOG
		 || (*dev)->watchdog_triggered == 1
#endif
		) {
			ret = 1;
			break;
		} else {
			ret = 0;
		}
	}

	return ret;
}

/**
 * @brief vcmd kernel thread main function
 */
static int _vcmd_kthread_fn(void *data)
{
	struct hantrovcmd_dev *dev = NULL;

	LOG_INFO("vcmd kthread enter\n");
	while (!kthread_should_stop()) {
        dev = NULL;
		if (wait_event_interruptible(kthread_waitq,
			_vcmd_kthread_actions(&dev))) {
			/** wait_event_xxx could not be interrupt by pm suspend*/
			LOG_ERR("wait kthread_waitq, signaled!!!\n");
			return -ERESTARTSYS;
		}
		if (dev == NULL)
			continue;

		if (dev->vce_hang) {
			vce_hang_process(dev);
			continue;
		}

#ifdef SUPPORT_WATCHDOG
		if (dev->watchdog_triggered == 1) {
			if (dev->vcmd_core_cfg.sub_module_type == VCMD_TYPE_ENCODER)
				dev->vce_hang = 1;
			else
				LOG_ERR("Non-VE module is also timeout, module_type = %u, core_id = %u\n"
					, dev->vcmd_core_cfg.sub_module_type, dev->core_id);
			dev->watchdog_triggered = 0;
			_vcmd_watchdog_process(dev);

			continue;
		}
#endif
	}
	LOG_INFO("vcmd kthread exit\n");

	return 0;
}

/**
 * @brief wake up vcmd kernel thread
 */
static void _vcmd_kthread_wakeup(void)
{

	if (IS_ERR(kthread))
		return;

	wake_up_interruptible_all(&kthread_waitq);
}

/**
 * @brief create kernel thread for vcmd driver
 */
static void _vcmd_kthread_create(void)
{
	if (kthread && !IS_ERR(kthread)) {
		return;
	}
	stop_kthread = 0;
	init_waitqueue_head(&kthread_waitq);
	kthread =
	kthread_run(_vcmd_kthread_fn, NULL, "vcmd_kthread");
	if (IS_ERR(kthread)) {
		LOG_ERR("create vcmd kthread failed!\n");
		return;
	}
}

/**
 * @brief stop kernel thread vcmd driver
 */
static void _vcmd_kthread_stop(void)
{
	if (kthread && !IS_ERR(kthread)) {
		stop_kthread = 1;
		kthread_stop(kthread);
		kthread = NULL;
	}
}

#ifdef SUPPORT_WATCHDOG
/**
 * @brief timer callback function of watchdog
 */
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
static void _vcmd_watchdog_cb(unsigned long arg)
#else
static void _vcmd_watchdog_cb(struct timer_list *timer)
#endif
{
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	struct timer_list * timer = (struct timer_list *)arg;
#endif
	struct hantrovcmd_dev *dev;

	dev = container_of(timer, struct hantrovcmd_dev, watchdog_timer);
	LOG_INFO("vcmd watchdog triggered, core_type = %u, core_id = %u\n"
		, dev->vcmd_core_cfg.sub_module_type, dev->core_id);
	dev->watchdog_triggered = 1;
	_vcmd_kthread_wakeup();
}

/**
 * @brief init vcmd watchdog
 */
static void _vcmd_watchdog_start(struct hantrovcmd_dev *dev)
{
	struct timer_list *timer = &dev->watchdog_timer;

#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	init_timer(timer);
	timer->function = _vcmd_watchdog_cb;
	timer->data = (unsigned long)timer;
#else
	timer_setup(timer, _vcmd_watchdog_cb, 0);
#endif
	dev->watchdog_active = 1;
}

/**
 * stop vcmd watchdog
 */
static void _vcmd_watchdog_stop(struct hantrovcmd_dev *dev){
	if (dev->watchdog_active) {
		del_timer(&dev->watchdog_timer);
		dev->watchdog_active = 0;
	}
}

/**
 * stop vcmd watchdog and wait watchdog callback
 */
static void _vcmd_watchdog_stop_and_sync(struct hantrovcmd_dev *dev){
	if (dev->watchdog_active) {
		del_timer_sync(&dev->watchdog_timer);
		dev->watchdog_active = 0;
	}
}

/**
 * feed and start vcmd watchdog
 */
static void _vcmd_watchdog_feed(struct hantrovcmd_dev *dev){
	u32 exetime = dev_wait_job_exetime(dev);

	/** 1. stop watchdog for the idle devices;
	 * 2. disable watchdog for the non-VE devcies;
	*/
	if (exetime == 0 || dev->vcmd_core_cfg.sub_module_type != VCMD_TYPE_ENCODER) {
		if (dev->watchdog_active) {
			_vcmd_watchdog_stop(dev);
		}
	} else {
		unsigned long expires = ENC_VCMD_WAIT_TIME(exetime);

		if (dev->watchdog_active == 0) {
			_vcmd_watchdog_start(dev);
		}
		mod_timer(&dev->watchdog_timer,
			(jiffies + msecs_to_jiffies(expires)));
	}
}
#endif //SUPPORT_WATCHDOG

#ifdef VCMD_DEBUG_INTERNAL
static void PrintInstr(u32 i, u32 instr, u32 *size)
{
	if ((instr & 0xF8000000) == OPCODE_WREG) {
		int length = ((instr >> 16) & 0x3FF);

		LOG_INFO("current cmdbuf data %d = 0x%08x => [%s %s %d 0x%x]\n",
			i, instr, "WREG", ((instr >> 26) & 0x1) ? "FIX" : "",
			length, (instr & 0xFFFF));
		*size = ((length + 2) >> 1) << 1;
	} else if ((instr & 0xF8000000) == OPCODE_END) {
		LOG_INFO("current cmdbuf data %d = 0x%08x => [%s]\n", i, instr, "END");
		*size = 2;
	} else if ((instr & 0xF8000000) == OPCODE_NOP) {
		LOG_INFO("current cmdbuf data %d = 0x%08x => [%s]\n", i, instr, "NOP");
		*size = 2;
	} else if ((instr & 0xF8000000) == OPCODE_RREG) {
		int length = ((instr >> 16) & 0x3FF);

		LOG_INFO("current cmdbuf data %d = 0x%08x => [%s %s %d 0x%x]\n",
			i, instr, "RREG", ((instr >> 26) & 0x1) ? "FIX" : "",
			length, (instr & 0xFFFF));
		*size = 4;
	} else if ((instr & 0xF8000000) == OPCODE_JMP) {
		LOG_INFO("current cmdbuf data %d = 0x%08x => [%s %s %s]\n", i,
			instr, "JMP", ((instr >> 26) & 0x1) ? "RDY" : "",
			((instr >> 25) & 0x1) ? "IE" : "");
		*size = 4;
	} else if ((instr & 0xF8000000) == OPCODE_STALL) {
		LOG_INFO("current cmdbuf data %d = 0x%08x => [%s %s 0x%x]\n", i,
			instr, "STALL", ((instr >> 26) & 0x1) ? "IM" : "",
			(instr & 0xFFFF));
		*size = 2;
	} else if ((instr & 0xF8000000) == OPCODE_CLRINT) {
		LOG_INFO("current cmdbuf data %d = 0x%08x => [%s %d 0x%x]\n", i,
			instr, "CLRINT", (instr >> 25) & 0x3, (instr & 0xFFFF));
		*size = 2;
	} else
		*size = 1;
}
#endif

/******************************************************************************
 *cmdbuf object management
 ******************************************************************************
 */
static struct cmdbuf_obj *create_cmdbuf_obj(void)
{
	struct cmdbuf_obj *cmdbuf_obj = NULL;

	cmdbuf_obj = kmalloc(sizeof(struct cmdbuf_obj), GFP_ATOMIC);
	if (!cmdbuf_obj) {
		LOG_DBG("%s\n", "kmalloc for cmdbuf_obj fail!");
		return cmdbuf_obj;
	}
	memset(cmdbuf_obj, 0, sizeof(struct cmdbuf_obj));
	return cmdbuf_obj;
}

static void free_cmdbuf_obj(struct cmdbuf_obj *cmdbuf_obj)
{
	if (!cmdbuf_obj) {
		LOG_DBG("%s\n", "remove_cmdbuf_obj NULL");
		return;
	}
	//free current cmdbuf_obj
	kfree(cmdbuf_obj);
}

static void free_cmdbuf_mem(u16 cmdbuf_id)
{
	unsigned long flags;

	spin_lock_irqsave(&vcmd_cmdbuf_alloc_lock, flags);
	cmdbuf_used[cmdbuf_id] = 0;
	cmdbuf_used_residual += 1;
	spin_unlock_irqrestore(&vcmd_cmdbuf_alloc_lock, flags);
	wake_up_interruptible_all(&vcmd_cmdbuf_memory_wait);
}

static int create_cmdbuf_node(bi_list_node **new_node)
{
	bi_list_node *current_node = NULL;
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	struct noncache_mem new_cmdbuf_addr;
	struct noncache_mem new_status_cmdbuf_addr;

	if (!new_node) {
		LOG_ERR("nullptr of addr of new_node\n");
		return -1;
	}
	*new_node = NULL;
	if (wait_event_interruptible(vcmd_cmdbuf_memory_wait,
				     allocate_cmdbuf(&new_cmdbuf_addr,
						     &new_status_cmdbuf_addr))) {
		LOG_INFO("%s:%d, vcmd_cmdbuf_memory_wait aborted\n", __func__, __LINE__);
		return -ERESTARTSYS;
	}
	cmdbuf_obj = create_cmdbuf_obj();
	if (!cmdbuf_obj) {
		LOG_DBG("%s\n", "create_cmdbuf_obj fail!");
		free_cmdbuf_mem(new_cmdbuf_addr.cmdbuf_id);
		return -1;
	}
	cmdbuf_obj->cmdbuf_busAddress = new_cmdbuf_addr.busAddress;
	cmdbuf_obj->mmu_cmdbuf_busAddress = new_cmdbuf_addr.mmu_bus_address;
	cmdbuf_obj->cmdbuf_virtualAddress = new_cmdbuf_addr.virtualAddress;
	cmdbuf_obj->cmdbuf_size = new_cmdbuf_addr.size;
	cmdbuf_obj->cmdbuf_id = new_cmdbuf_addr.cmdbuf_id;
	cmdbuf_obj->status_busAddress = new_status_cmdbuf_addr.busAddress;
	cmdbuf_obj->mmu_status_busAddress =
		new_status_cmdbuf_addr.mmu_bus_address;
	cmdbuf_obj->status_virtualAddress =
		new_status_cmdbuf_addr.virtualAddress;
	cmdbuf_obj->status_size = new_status_cmdbuf_addr.size;
	// /* add by xiaogang.li for debug purpose */
	// LOG_INFO("create_cmdbuf_node: cmdbuf_busAddress=0x%lx, mmu_cmdbuf_busAddress=0x%lx\n",
	// 	cmdbuf_obj->cmdbuf_busAddress, cmdbuf_obj->mmu_cmdbuf_busAddress);
	// LOG_INFO("create_cmdbuf_node: status_busAddress=0x%lx, mmu_status_busAddress=0x%lx\n",
	// 	cmdbuf_obj->status_busAddress, cmdbuf_obj->mmu_status_busAddress);
	current_node = bi_list_create_node();
	if (!current_node) {
		LOG_DBG("%s\n", "bi_list_create_node fail!");
		free_cmdbuf_mem(new_cmdbuf_addr.cmdbuf_id);
		free_cmdbuf_obj(cmdbuf_obj);
		return -1;
	}
	current_node->data = (void *)cmdbuf_obj;
	current_node->next = NULL;
	current_node->previous = NULL;
	*new_node = current_node;
	return 0;
}

static void free_cmdbuf_node(bi_list_node *cmdbuf_node)
{
	struct cmdbuf_obj *cmdbuf_obj = NULL;

	if (!cmdbuf_node) {
		LOG_DBG("%s\n", "remove_cmdbuf_node NULL");
		return;
	}
	cmdbuf_obj = (struct cmdbuf_obj *)cmdbuf_node->data;
	//free cmdbuf mem in pool
	free_cmdbuf_mem(cmdbuf_obj->cmdbuf_id);

	//free struct cmdbuf_obj
	free_cmdbuf_obj(cmdbuf_obj);
	//free current cmdbuf_node entity.
	bi_list_free_node(cmdbuf_node);
}

//just remove, not free the node.
static bi_list_node *remove_cmdbuf_node_from_list(bi_list *list,
						  bi_list_node *cmdbuf_node)
{
	if (!cmdbuf_node) {
		LOG_DBG("%s\n", "remove_cmdbuf_node_from_list  NULL");
		return NULL;
	}
	if (cmdbuf_node->next) {
		bi_list_remove_node(list, cmdbuf_node);
		return cmdbuf_node;
	} else {
		//the last one, should not be removed.
		return NULL;
	}
}

//calculate executing_time of each vcmd
static u64 calculate_executing_time_after_node(bi_list_node *exe_cmdbuf_node)
{
	u64 time_run_all = 0;
	struct cmdbuf_obj *cmdbuf_obj_temp = NULL;

	while (1) {
		if (!exe_cmdbuf_node)
			break;
		cmdbuf_obj_temp = (struct cmdbuf_obj *)exe_cmdbuf_node->data;
		time_run_all += cmdbuf_obj_temp->executing_time;
		exe_cmdbuf_node = exe_cmdbuf_node->next;
	}
	return time_run_all;
}
static u64 calculate_executing_time_after_node_high_priority(bi_list_node *exe_cmdbuf_node)
{
	u64 time_run_all = 0;
	struct cmdbuf_obj *cmdbuf_obj_temp = NULL;

	if (!exe_cmdbuf_node)
		return time_run_all;
	cmdbuf_obj_temp = (struct cmdbuf_obj *)exe_cmdbuf_node->data;
	time_run_all += cmdbuf_obj_temp->executing_time;
	exe_cmdbuf_node = exe_cmdbuf_node->next;
	while (1) {
		if (!exe_cmdbuf_node)
			break;
		cmdbuf_obj_temp = (struct cmdbuf_obj *)exe_cmdbuf_node->data;
		if (cmdbuf_obj_temp->priority == CMDBUF_PRIORITY_NORMAL)
			break;
		time_run_all += cmdbuf_obj_temp->executing_time;
		exe_cmdbuf_node = exe_cmdbuf_node->next;
	}
	return time_run_all;
}

/******************************************************************************
 *cmdbuf pool management
 ******************************************************************************
 */
static int allocate_cmdbuf(struct noncache_mem *new_cmdbuf_addr,
			   struct noncache_mem *new_status_cmdbuf_addr)
{
	unsigned long flags;

	spin_lock_irqsave(&vcmd_cmdbuf_alloc_lock, flags);
	if (cmdbuf_used_residual == 0) {
		spin_unlock_irqrestore(&vcmd_cmdbuf_alloc_lock, flags);
		//no empty cmdbuf
		return 0;
	}
	//there is one cmdbuf at least
	while (1) {
		if (cmdbuf_used[cmdbuf_used_pos] == 0 && !global_cmdbuf_node[cmdbuf_used_pos]) {
			cmdbuf_used[cmdbuf_used_pos] = 1;
			cmdbuf_used_residual -= 1;
			new_cmdbuf_addr->virtualAddress =
				vcmd_buf_mem_pool.virtualAddress + cmdbuf_used_pos * CMDBUF_MAX_SIZE / 4;
			new_cmdbuf_addr->busAddress =
				vcmd_buf_mem_pool.busAddress + cmdbuf_used_pos * CMDBUF_MAX_SIZE;
			new_cmdbuf_addr->mmu_bus_address =
				vcmd_buf_mem_pool.mmu_bus_address + cmdbuf_used_pos * CMDBUF_MAX_SIZE;
			new_cmdbuf_addr->size = CMDBUF_MAX_SIZE;
			new_cmdbuf_addr->cmdbuf_id = cmdbuf_used_pos;
			new_status_cmdbuf_addr->virtualAddress =
				vcmd_status_buf_mem_pool.virtualAddress + cmdbuf_used_pos * CMDBUF_MAX_SIZE / 4;
			new_status_cmdbuf_addr->busAddress =
				vcmd_status_buf_mem_pool.busAddress + cmdbuf_used_pos * CMDBUF_MAX_SIZE;
			new_status_cmdbuf_addr->mmu_bus_address =
				vcmd_status_buf_mem_pool.mmu_bus_address + cmdbuf_used_pos * CMDBUF_MAX_SIZE;
			new_status_cmdbuf_addr->size = CMDBUF_MAX_SIZE;
			new_status_cmdbuf_addr->cmdbuf_id = cmdbuf_used_pos;
			cmdbuf_used_pos++;
			if (cmdbuf_used_pos >= TOTAL_DISCRETE_CMDBUF_NUM)
				cmdbuf_used_pos = 0;
			spin_unlock_irqrestore(&vcmd_cmdbuf_alloc_lock, flags);
			return 1;
		} else {
			cmdbuf_used_pos++;
			if (cmdbuf_used_pos >= TOTAL_DISCRETE_CMDBUF_NUM)
				cmdbuf_used_pos = 0;
		}
	}
	return 0;
}

static bi_list_node *get_cmdbuf_node_in_list_by_addr(size_t cmdbuf_addr,
						     bi_list *list)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct cmdbuf_obj *cmdbuf_obj = NULL;

	new_cmdbuf_node = list->head;
	while (1) {
		if (!new_cmdbuf_node)
			return NULL;
		cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		if (((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr) <= cmdbuf_addr) &&
		    (((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr + cmdbuf_obj->cmdbuf_size) > cmdbuf_addr))) {
			return new_cmdbuf_node;
		}
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
	return NULL;
}

static int wait_abort_rdy(struct hantrovcmd_dev *dev)
{
	return dev->working_state == WORKING_STATE_IDLE;
}

static int select_vcmd(bi_list_node *new_cmdbuf_node, u16 numa_id)
{
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	bi_list_node *curr_cmdbuf_node = NULL;
	bi_list *list = NULL;
	struct hantrovcmd_dev *dev = NULL;
	struct hantrovcmd_dev *smallest_dev = NULL;
	u64 executing_time = 0xffffffffffffffff;
	int counter = 0;
	unsigned long flags = 0;
	u32 hw_rdy_cmdbuf_num = 0;
	size_t exe_cmdbuf_addr = 0;
	struct cmdbuf_obj *cmdbuf_obj_temp = NULL;
	u32 cmdbuf_id = 0;

	cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;

	/** bind the task to the specified core_id if necessary*/
	if (numa_id < vcmd_type_core_num[cmdbuf_obj->module_type]) {
		dev = vcmd_manager[cmdbuf_obj->module_type][numa_id];
		list = &dev->list_manager;
		spin_lock_irqsave(dev->spinlock, flags);
		bi_list_insert_node_tail(list, new_cmdbuf_node);
		spin_unlock_irqrestore(dev->spinlock, flags);

		cmdbuf_obj->core_id = dev->core_id;
		return 0;
	}

	//there is an empty vcmd to be used
	while (1) {
		dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
		list = &dev->list_manager;
		spin_lock_irqsave(dev->spinlock, flags);
		if (!list->tail) {
			bi_list_insert_node_tail(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			cmdbuf_obj->core_id = dev->core_id;
			return 0;
		} else {
			spin_unlock_irqrestore(dev->spinlock, flags);
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			counter++;
		}
		if (counter >= vcmd_type_core_num[cmdbuf_obj->module_type])
			break;
	}
	//there is a vcmd which tail node -> cmdbuf_run_done == 1. It means this vcmd has nothing to do, so we select it.
	counter = 0;
	while (1) {
		dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
		list = &dev->list_manager;
		spin_lock_irqsave(dev->spinlock, flags);
		curr_cmdbuf_node = list->tail;
		if (!curr_cmdbuf_node) {
			bi_list_insert_node_tail(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			cmdbuf_obj->core_id = dev->core_id;
			return 0;
		}
		cmdbuf_obj_temp = (struct cmdbuf_obj *)curr_cmdbuf_node->data;
		if (cmdbuf_obj_temp->cmdbuf_run_done == 1) {
			bi_list_insert_node_tail(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			cmdbuf_obj->core_id = dev->core_id;
			return 0;
		} else {
			spin_unlock_irqrestore(dev->spinlock, flags);
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			counter++;
		}
		if (counter >= vcmd_type_core_num[cmdbuf_obj->module_type])
			break;
	}

	//another case, tail = executing node, and vcmd=pend state (finish but not generate interrupt)
	counter = 0;
	while (1) {
		dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
		list = &dev->list_manager;
		//read executing cmdbuf address
		if (dev->hw_version_id <= HW_ID_1_0_C)
			hw_rdy_cmdbuf_num = vcmd_get_register_value(
				(const void *)dev->hwregs, dev->reg_mirror,
				HWIF_VCMD_EXE_CMDBUF_COUNT);
		else {
			hw_rdy_cmdbuf_num = *(dev->vcmd_reg_mem_virtualAddress +
					      VCMD_EXE_CMDBUF_COUNT);
			if (hw_rdy_cmdbuf_num != dev->sw_cmdbuf_rdy_num)
				hw_rdy_cmdbuf_num += 1;
		}
		spin_lock_irqsave(dev->spinlock, flags);
		curr_cmdbuf_node = list->tail;
		if (!curr_cmdbuf_node) {
			bi_list_insert_node_tail(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			cmdbuf_obj->core_id = dev->core_id;
			return 0;
		}

		if (dev->sw_cmdbuf_rdy_num == hw_rdy_cmdbuf_num) {
			bi_list_insert_node_tail(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			cmdbuf_obj->core_id = dev->core_id;
			return 0;
		} else {
			spin_unlock_irqrestore(dev->spinlock, flags);
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			counter++;
		}
		if (counter >= vcmd_type_core_num[cmdbuf_obj->module_type])
			break;
	}

	//there is no idle vcmd,if low priority,calculate exe time, select the least one.
	// or if high priority, calculate the exe time, select the least one and abort it.
	if (cmdbuf_obj->priority == CMDBUF_PRIORITY_NORMAL) {
		counter = 0;
		//calculate total execute time of all devices
		while (1) {
			dev = vcmd_manager[cmdbuf_obj->module_type]
				[vcmd_position[cmdbuf_obj->module_type]];
			//read executing cmdbuf address
			if (dev->hw_version_id <= HW_ID_1_0_C) {
				exe_cmdbuf_addr = VCMDGetAddrRegisterValue((const void *)dev->hwregs,
									   dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR);
				list = &dev->list_manager;
				spin_lock_irqsave(dev->spinlock, flags);
				//get the executing cmdbuf node.
				curr_cmdbuf_node =
					get_cmdbuf_node_in_list_by_addr(exe_cmdbuf_addr, list);

				//calculate total execute time of this device
				dev->total_exe_time =
					calculate_executing_time_after_node(curr_cmdbuf_node);
				spin_unlock_irqrestore(dev->spinlock, flags);
			} else {
				/*cmdbuf_id = vcmd_get_register_value((const void *)dev->hwregs,dev->reg_mirror,
				 *                           HWIF_VCMD_CMDBUF_EXECUTING_ID);
				 */
				cmdbuf_id = *(dev->vcmd_reg_mem_virtualAddress + EXECUTING_CMDBUF_ID_ADDR + 1);
				spin_lock_irqsave(dev->spinlock, flags);
				if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM ||
				    cmdbuf_id == 0) {
					LOG_INFO("cmdbuf_id greater than the ceiling !!\n");
					spin_unlock_irqrestore(dev->spinlock, flags);
					return -1;
				}
				//get the executing cmdbuf node.
				curr_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
				if (!curr_cmdbuf_node) {
					list = &dev->list_manager;
					curr_cmdbuf_node = list->head;
					while (1) {
						if (!curr_cmdbuf_node)
							break;
						cmdbuf_obj_temp =
							(struct cmdbuf_obj *)curr_cmdbuf_node->data;
						if (cmdbuf_obj_temp->cmdbuf_data_linked &&
						    cmdbuf_obj_temp->cmdbuf_run_done == 0)
							break;
						curr_cmdbuf_node = curr_cmdbuf_node->next;
					}
				}

				//calculate total execute time of this device
				dev->total_exe_time =
					calculate_executing_time_after_node(curr_cmdbuf_node);
				spin_unlock_irqrestore(dev->spinlock, flags);
			}
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			counter++;
			if (counter >= vcmd_type_core_num[cmdbuf_obj->module_type])
				break;
		}
		//find the device with the least total_exe_time.
		counter = 0;
		executing_time = 0xffffffffffffffff;
		while (1) {
			dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
			if (dev->total_exe_time <= executing_time) {
				executing_time = dev->total_exe_time;
				smallest_dev = dev;
			}
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			counter++;
			if (counter >= vcmd_type_core_num[cmdbuf_obj->module_type])
				break;
		}
		//insert list
		list = &smallest_dev->list_manager;
		spin_lock_irqsave(smallest_dev->spinlock, flags);
		bi_list_insert_node_tail(list, new_cmdbuf_node);
		spin_unlock_irqrestore(smallest_dev->spinlock, flags);
		cmdbuf_obj->core_id = smallest_dev->core_id;
		return 0;
	} else {
		//CMDBUF_PRIORITY_HIGH
		counter = 0;
		//calculate total execute time of all devices
		while (1) {
			dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
			if (dev->hw_version_id <= HW_ID_1_0_C) {
				//read executing cmdbuf address
				exe_cmdbuf_addr = VCMDGetAddrRegisterValue(
					(const void *)dev->hwregs,
					dev->reg_mirror,
					HWIF_VCMD_EXECUTING_CMD_ADDR);
				list = &dev->list_manager;
				spin_lock_irqsave(dev->spinlock, flags);
				//get the executing cmdbuf node.
				curr_cmdbuf_node =
					get_cmdbuf_node_in_list_by_addr(exe_cmdbuf_addr, list);

				//calculate total execute time of this device
				dev->total_exe_time =
					calculate_executing_time_after_node_high_priority(curr_cmdbuf_node);
				spin_unlock_irqrestore(dev->spinlock, flags);
			} else {
				/*cmdbuf_id = vcmd_get_register_value((const void *)dev->hwregs,dev->reg_mirror,
				 *                      HWIF_VCMD_CMDBUF_EXECUTING_ID);
				 */
				cmdbuf_id = *(dev->vcmd_reg_mem_virtualAddress + EXECUTING_CMDBUF_ID_ADDR);
				spin_lock_irqsave(dev->spinlock, flags);
				if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM || cmdbuf_id == 0) {
					LOG_INFO("cmdbuf_id greater than the ceiling !!\n");
					spin_unlock_irqrestore(dev->spinlock, flags);
					return -1;
				}
				//get the executing cmdbuf node.
				curr_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
				if (!curr_cmdbuf_node) {
					list = &dev->list_manager;
					curr_cmdbuf_node = list->head;
					while (1) {
						if (!curr_cmdbuf_node)
							break;
						cmdbuf_obj_temp =
							(struct cmdbuf_obj *)curr_cmdbuf_node->data;
						if (cmdbuf_obj_temp->cmdbuf_data_linked &&
						    cmdbuf_obj_temp->cmdbuf_run_done == 0)
							break;
						curr_cmdbuf_node = curr_cmdbuf_node->next;
					}
				}

				//calculate total execute time of this device
				dev->total_exe_time =
					calculate_executing_time_after_node(curr_cmdbuf_node);
				spin_unlock_irqrestore(dev->spinlock, flags);
			}
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			counter++;
			if (counter >= vcmd_type_core_num[cmdbuf_obj->module_type])
				break;
		}
		//find the smallest device.
		counter = 0;
		executing_time = 0xffffffffffffffff;
		while (1) {
			dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
			if (dev->total_exe_time <= executing_time) {
				executing_time = dev->total_exe_time;
				smallest_dev = dev;
			}
			vcmd_position[cmdbuf_obj->module_type]++;
			if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_type_core_num[cmdbuf_obj->module_type])
				vcmd_position[cmdbuf_obj->module_type] = 0;
			counter++;
			if (counter >= vcmd_type_core_num[cmdbuf_obj->module_type])
				break;
		}
		//abort the vcmd and wait
		vcmd_write_register_value((const void *)smallest_dev->hwregs,
					  smallest_dev->reg_mirror,
					  HWIF_VCMD_START_TRIGGER, 0);
		dev->software_triger_abort = 1;
		if (wait_event_interruptible(*smallest_dev->wait_abort_queue, wait_abort_rdy(smallest_dev))) {
			dev->software_triger_abort = 0;
			return -ERESTARTSYS;
		}
		dev->software_triger_abort = 0;
		//need to select inserting position again because hw maybe have run to the next node.
		//CMDBUF_PRIORITY_HIGH
		spin_lock_irqsave(smallest_dev->spinlock, flags);
		curr_cmdbuf_node = smallest_dev->list_manager.head;
		while (1) {
			//if list is empty or tail,insert to tail
			if (!curr_cmdbuf_node)
				break;
			cmdbuf_obj_temp = (struct cmdbuf_obj *)curr_cmdbuf_node->data;
			//if find the first node which priority is normal, insert node prior to  the node
			if (cmdbuf_obj_temp->priority ==
				    CMDBUF_PRIORITY_NORMAL && cmdbuf_obj_temp->cmdbuf_run_done == 0)
				break;
			curr_cmdbuf_node = curr_cmdbuf_node->next;
		}
		bi_list_insert_node_before(list, curr_cmdbuf_node, new_cmdbuf_node);
		cmdbuf_obj->core_id = smallest_dev->core_id;
		spin_unlock_irqrestore(smallest_dev->spinlock, flags);

		return 0;
	}
	return 0;
}

static int wait_process_resource_rdy(struct process_manager_obj *process_manager_obj)
{
	return process_manager_obj->total_exe_time <= PROCESS_MAX_SUM_OF_IMAGE_SIZE;
}

static long reserve_cmdbuf(struct file *filp,
			   struct exchange_parameter *input_para)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	bi_list_node *process_manager_node = NULL;
	struct process_manager_obj *process_manager_obj = NULL;
	unsigned long flags;
	int ret = 0;

	input_para->cmdbuf_id = 0;
	if (input_para->cmdbuf_size > CMDBUF_MAX_SIZE)
		return -1;
	spin_lock_irqsave(&vcmd_process_manager_lock, flags);
	process_manager_node = global_process_manager.head;
	while (1) {
		if (!process_manager_node) {
			//should not happen
			LOG_ERR("hantrovcmd: ERROR process_manager_node !!\n");
			spin_unlock_irqrestore(&vcmd_process_manager_lock, flags);
			return -1;
		}
		process_manager_obj = (struct process_manager_obj *)process_manager_node->data;
		if (filp == process_manager_obj->filp)
			break;
		process_manager_node = process_manager_node->next;
	}
	spin_unlock_irqrestore(&vcmd_process_manager_lock, flags);
	spin_lock_irqsave(&process_manager_obj->spinlock, flags);
	process_manager_obj->total_exe_time += input_para->executing_time;
	spin_unlock_irqrestore(&process_manager_obj->spinlock, flags);
	if (wait_event_interruptible(
		    process_manager_obj->wait_queue,
		    wait_process_resource_rdy(process_manager_obj))) {
		LOG_INFO("%s:%d, pmo wait_queue aborted\n", __func__, __LINE__);
		return -ERESTARTSYS;
	}

	ret = create_cmdbuf_node(&new_cmdbuf_node);
	if (ret != 0) {
		return ret;
	}
	if (!new_cmdbuf_node)
		return -1;

	cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
	cmdbuf_obj->module_type = input_para->module_type;
	cmdbuf_obj->priority = input_para->priority;
	cmdbuf_obj->executing_time = input_para->executing_time;
	cmdbuf_obj->cmdbuf_size = CMDBUF_MAX_SIZE;
	input_para->cmdbuf_size = CMDBUF_MAX_SIZE;
	cmdbuf_obj->filp = filp;
	cmdbuf_obj->process_manager_obj = process_manager_obj;

	input_para->cmdbuf_id = cmdbuf_obj->cmdbuf_id;
	global_cmdbuf_node[input_para->cmdbuf_id] = new_cmdbuf_node;

	return 0;
}

static long release_cmdbuf(struct file *filp, u16 cmdbuf_id)
{
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	bi_list_node *last_cmdbuf_node = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list *list = NULL;
	u32 module_type;

	unsigned long flags;
	struct hantrovcmd_dev *dev = NULL;
	/*get cmdbuf object according to cmdbuf_id*/
	new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
	if (!new_cmdbuf_node) {
		//should not happen
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u !!\n", cmdbuf_id);
		return -1;
	}
	cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
	if (cmdbuf_obj->filp != filp) {
		//should not happen
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u !!\n", cmdbuf_id);
		return -1;
	}
	module_type = cmdbuf_obj->module_type;
	if (VCMD_TYPE_ENCODER != module_type && VCMD_TYPE_JPEG_ENCODER != module_type) {
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u, module_type = %u\n", cmdbuf_id, module_type);
		return -1;
	}
	//TODO
	if (down_interruptible(&vcmd_reserve_cmdbuf_sem[module_type])) {
		LOG_INFO("%s:%d, reserve_cmdbuf_sem aborted\n", __func__, __LINE__);
		return -ERESTARTSYS;
	}
	new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
	if (!new_cmdbuf_node) {
		up(&vcmd_reserve_cmdbuf_sem[module_type]);
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u !!\n", cmdbuf_id);
		return -1;
	}
	cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
	if (cmdbuf_obj->filp != filp) {
		up(&vcmd_reserve_cmdbuf_sem[module_type]);
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u !!\n", cmdbuf_id);
		return -1;
	}
	dev = &hantrovcmd_data[cmdbuf_obj->core_id];

	//spin_lock_irqsave(dev->spinlock, flags);
	list = &dev->list_manager;
	cmdbuf_obj->cmdbuf_need_remove = 1;
	last_cmdbuf_node = new_cmdbuf_node->previous;
	while (1) {
		//remove current node
		cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		if (cmdbuf_obj->cmdbuf_need_remove == 1 && cmdbuf_obj->cmdbuf_run_done) {
			new_cmdbuf_node = remove_cmdbuf_node_from_list(
				list, new_cmdbuf_node);
			if (new_cmdbuf_node) {
				//free node
				global_cmdbuf_node[cmdbuf_obj->cmdbuf_id] = NULL;
				if (cmdbuf_obj->process_manager_obj) {
					spin_lock_irqsave(&cmdbuf_obj->process_manager_obj->spinlock, flags);
					cmdbuf_obj->process_manager_obj->total_exe_time -= cmdbuf_obj->executing_time;
					spin_unlock_irqrestore(&cmdbuf_obj->process_manager_obj->spinlock,
							       flags);
					wake_up_interruptible_all(&cmdbuf_obj->process_manager_obj->wait_queue);
				}
				free_cmdbuf_node(new_cmdbuf_node);
			}
		}
		if (!last_cmdbuf_node)
			break;
		new_cmdbuf_node = last_cmdbuf_node;
		last_cmdbuf_node = new_cmdbuf_node->previous;
	}
	//spin_unlock_irqrestore(dev->spinlock, flags);
	up(&vcmd_reserve_cmdbuf_sem[module_type]);
	if (filp) {
		struct filp_priv *fp_priv = (struct filp_priv *)filp->private_data;

		/** release for the pm*/
		if (atomic_dec_if_positive(&(fp_priv->core_tasks[dev->core_id])) >= 0)
			enc_pm_runtime_put(dev->core_id);
	}

	return 0;
}

static long release_cmdbuf_node(bi_list *list, bi_list_node *cmdbuf_node)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	/*get cmdbuf object according to cmdbuf_id*/
	new_cmdbuf_node = cmdbuf_node;
	if (!new_cmdbuf_node)
		return -1;
	//remove node from list
	new_cmdbuf_node = remove_cmdbuf_node_from_list(list, new_cmdbuf_node);
	if (new_cmdbuf_node) {
		//free node
		cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		global_cmdbuf_node[cmdbuf_obj->cmdbuf_id] = NULL;
		free_cmdbuf_node(new_cmdbuf_node);
		return 0;
	}
	return 1;
}

static long release_cmdbuf_node_cleanup(bi_list *list)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct cmdbuf_obj *cmdbuf_obj = NULL;

	while (1) {
		new_cmdbuf_node = list->head;
		if (!new_cmdbuf_node)
			return 0;
		//remove node from list
		bi_list_remove_node(list, new_cmdbuf_node);
		//free node
		cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		global_cmdbuf_node[cmdbuf_obj->cmdbuf_id] = NULL;
		free_cmdbuf_node(new_cmdbuf_node);
	}
	return 0;
}

static bi_list_node *find_last_linked_cmdbuf(bi_list_node *current_node)
{
	bi_list_node *new_cmdbuf_node = current_node;
	bi_list_node *last_cmdbuf_node;
	struct cmdbuf_obj *cmdbuf_obj = NULL;

	if (!current_node)
		return NULL;
	last_cmdbuf_node = new_cmdbuf_node;
	new_cmdbuf_node = new_cmdbuf_node->previous;
	while (1) {
		if (!new_cmdbuf_node)
			return last_cmdbuf_node;
		cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		if (cmdbuf_obj->cmdbuf_data_linked)
			return new_cmdbuf_node;
		last_cmdbuf_node = new_cmdbuf_node;
		new_cmdbuf_node = new_cmdbuf_node->previous;
	}
	return NULL;
}

static long link_and_run_cmdbuf(struct file *filp,
				struct exchange_parameter *input_para)
{
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *last_cmdbuf_node;
	u32 *jmp_addr = NULL;
	u32 opCode;
	u32 tempOpcode;
	u32 record_last_cmdbuf_rdy_num;
	struct hantrovcmd_dev *dev = NULL;
	unsigned long flags;
	int return_value;
	u16 cmdbuf_id = input_para->cmdbuf_id;

	new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
	if (!new_cmdbuf_node) {
		//should not happen
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u !!\n", cmdbuf_id);
		return -1;
	}
	cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
	if (cmdbuf_obj->filp != filp) {
		//should not happen
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u !!\n", cmdbuf_id);
		return -1;
	}
	cmdbuf_obj->cmdbuf_data_loaded = 1;
	cmdbuf_obj->cmdbuf_size = input_para->cmdbuf_size;
#ifdef VCMD_DEBUG_INTERNAL
	{
		u32 i, inst = 0, size = 0;

		LOG_INFO("vcmd link, current cmdbuf content\n");
		for (i = 0; i < cmdbuf_obj->cmdbuf_size / 4; i++) {
			if (i == inst) {
				PrintInstr(i, *(cmdbuf_obj->cmdbuf_virtualAddress + i), &size);
				inst += size;
			} else {
				LOG_INFO("current cmdbuf data %d = 0x%x\n", i, *(cmdbuf_obj->cmdbuf_virtualAddress + i));
			}
		}
	}
#endif
	//test nop and end opcode, then assign value.
	cmdbuf_obj->has_end_cmdbuf = 0; //0: has jmp opcode,1 has end code
	cmdbuf_obj->no_normal_int_cmdbuf = 0; //0: interrupt when JMP,1 not interrupt when JMP
	jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress + (cmdbuf_obj->cmdbuf_size / 4);
	opCode = tempOpcode = *(jmp_addr - 4);
	opCode >>= 27;
	opCode <<= 27;
	//we can't identify END opcode or JMP opcode, so we don't support END opcode in control sw and driver.
	if (opCode == OPCODE_JMP) {
		//jmp
		opCode = tempOpcode;
		opCode &= 0x02000000;
		if (opCode == JMP_IE_1)
			cmdbuf_obj->no_normal_int_cmdbuf = 0;
		else
			cmdbuf_obj->no_normal_int_cmdbuf = 1;
	} else {
		//not support other opcode
		LOG_ERR("vcmd: invalid opCode=%d\n", opCode);
		return -1;
	}

	if (down_interruptible(&vcmd_reserve_cmdbuf_sem[cmdbuf_obj->module_type])) {
		LOG_INFO("vcmd: failed to enter waiting status\n");
		return -ERESTARTSYS;
	}

	return_value = select_vcmd(new_cmdbuf_node, input_para->numa_id);
	if (return_value) {
		/** release vcmd_reserve_cmdbuf_sem if return, avoid dead lock.*/
		up(&vcmd_reserve_cmdbuf_sem[cmdbuf_obj->module_type]);
		LOG_ERR("vcmd: error return from select_vcmd\n");
		return return_value;
	}

	dev = &hantrovcmd_data[cmdbuf_obj->core_id];
	input_para->core_id = cmdbuf_obj->core_id;
	LOG_DBG("Venc Allocate cmd buffer [%d] to core [%d], filp = %p\n", cmdbuf_id, input_para->core_id, filp);
	if (filp) {
		struct filp_priv *fp_priv = (struct filp_priv *)filp->private_data;

		return_value = enc_pm_runtime_get(dev->core_id);
		/** check if the device be resumed ok*/
		if (return_value < 0) {
			up(&vcmd_reserve_cmdbuf_sem[cmdbuf_obj->module_type]);
			LOG_ERR("pm_runtime_get_sync failed, return %d\n", return_value);
			return return_value;
		}
		atomic_inc(&(fp_priv->core_tasks[dev->core_id]));
	}
	LOG_DBG("Venc link_and_run cmd buffer [%d] in core [%d]\n", cmdbuf_id, input_para->core_id);
	//set ddr address for vcmd registers copy.
	if (dev->hw_version_id > HW_ID_1_0_C) {
		//read vcmd executing register into ddr memory.
		//now core id is got and output ddr address of vcmd register can be filled in.
		//each core has its own fixed output ddr address of vcmd registers.
		jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress;
		if (mmu_enable) {
			*(jmp_addr + 2) = 0;
			*(jmp_addr + 1) =
				(u32)((dev->mmu_vcmd_reg_mem_busAddress + (EXECUTING_CMDBUF_ID_ADDR + 1) * 4));
		} else {
			if (sizeof(size_t) == 8) {
				*(jmp_addr + 2) = (u32)(
					(u64)(dev->vcmd_reg_mem_busAddress + (EXECUTING_CMDBUF_ID_ADDR + 1) * 4) >> 32);
			} else {
				*(jmp_addr + 2) = 0;
			}
			*(jmp_addr + 1) =
				(u32)((dev->vcmd_reg_mem_busAddress + (EXECUTING_CMDBUF_ID_ADDR + 1) * 4));
		}

		jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress + (cmdbuf_obj->cmdbuf_size / 4);
		//read vcmd all registers into ddr memory.
		//now core id is got and output ddr address of vcmd registers can be filled in.
		//each core has its own fixed output ddr address of vcmd registers.
		if (mmu_enable) {
			if (sizeof(size_t) == 8)
				*(jmp_addr - 6) = 0;
			*(jmp_addr - 7) = (u32)(dev->mmu_vcmd_reg_mem_busAddress);
		} else {
			if (sizeof(size_t) == 8) {
				*(jmp_addr - 6) = (u32)((u64)dev->vcmd_reg_mem_busAddress >> 32);
			} else {
				*(jmp_addr - 6) = 0;
			}
			*(jmp_addr - 7) = (u32)(dev->vcmd_reg_mem_busAddress);
		}
	}
	//start to link and/or run
	spin_lock_irqsave(dev->spinlock, flags);
	last_cmdbuf_node = find_last_linked_cmdbuf(new_cmdbuf_node);
	record_last_cmdbuf_rdy_num = dev->sw_cmdbuf_rdy_num;
	LOG_DBG("dev->sw_cmdbuf_rdy_num = %d before vcmd_link_cmdbuf, last cmdbuf_id = %u\n"
		, dev->sw_cmdbuf_rdy_num, ((struct cmdbuf_obj *)last_cmdbuf_node->data)->cmdbuf_id);
	vcmd_link_cmdbuf(dev, last_cmdbuf_node);
	LOG_DBG("dev->sw_cmdbuf_rdy_num = %d after vcmd_link_cmdbuf\n", dev->sw_cmdbuf_rdy_num);
	if (dev->working_state == WORKING_STATE_IDLE) {
		//run
		while (last_cmdbuf_node &&
		       ((struct cmdbuf_obj *)last_cmdbuf_node->data)->cmdbuf_run_done)
			last_cmdbuf_node = last_cmdbuf_node->next;

		if (last_cmdbuf_node && last_cmdbuf_node->data) {
			LOG_DBG("vcmd start for cmdbuf id %d, cmdbuf_run_done = %d\n",
			       ((struct cmdbuf_obj *)last_cmdbuf_node->data)->cmdbuf_id,
			       ((struct cmdbuf_obj *)last_cmdbuf_node->data)->cmdbuf_run_done);
		}
		LOG_INFO("link_and_run_cmdbuf, restart vcmd, core_id = %u, cmdbuf_id = %u, restart_cmdbuf_id = %u\n"
			, dev->core_id, cmdbuf_id, dev->restart_cmdbuf_id);
		if (dev->restart_cmdbuf_id == 0XFFFF)
			vcmd_start(dev, last_cmdbuf_node);
	} else {
		//just update cmdbuf ready number
		LOG_DBG("link_and_run_cmdbuf, dev state = %d, cmdbuf rdy num updated %d -> %d\n",
		       dev->working_state, record_last_cmdbuf_rdy_num,
		       dev->sw_cmdbuf_rdy_num);
		if (record_last_cmdbuf_rdy_num != dev->sw_cmdbuf_rdy_num)
			vcmd_write_register_value((const void *)dev->hwregs,
						  dev->reg_mirror,
						  HWIF_VCMD_RDY_CMDBUF_COUNT,
						  dev->sw_cmdbuf_rdy_num);
#ifdef SUPPORT_WATCHDOG
		_vcmd_watchdog_feed(dev);
#endif
	}
	spin_unlock_irqrestore(dev->spinlock, flags);

	up(&vcmd_reserve_cmdbuf_sem[cmdbuf_obj->module_type]);

	return 0;
}

/******************************************************************************/
static int check_cmdbuf_irq(struct hantrovcmd_dev *dev,
			    struct cmdbuf_obj *cmdbuf_obj, u32 *irq_status_ret)
{
	int rdy = 0;
	unsigned long flags;

	spin_lock_irqsave(dev->spinlock, flags);
	if (cmdbuf_obj->cmdbuf_run_done) {
		rdy = 1;
		*irq_status_ret =
			cmdbuf_obj->executing_status; //need to decide how to assign this variable
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	return rdy;
}

#ifdef IRQ_SIMULATION
static void get_random_bytes(void *buf, int nbytes);

static void hantrovcmd_trigger_irq(struct timer_list *timer)
{
	u32 timer_id = 0;
	u32 core_id = 0;
	u32 i;

	for (i = 0; i < 10000; i++) {
		if (timer_reserve[i].timer == timer) {
			timer_id = timer_reserve[i].timer_id;
			core_id = timer_reserve[i].core_id;
			break;
		}
	}
	LOG_DBG("trigger core 0 irq\n");
	hantrovcmd_isr(core_id, (void *)&hantrovcmd_data[core_id]);
	del_timer(timer);
	timer_reserve[timer_id].timer = NULL;
}
#endif

static long wait_cmdbuf_ready(struct file *filp, u16 cmdbuf_id,
				      u32 *irq_status_ret)
{
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	struct hantrovcmd_dev *dev = NULL;

	new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
	if (!new_cmdbuf_node) {
		//should not happen
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u !!\n", cmdbuf_id);
		return -1;
	}
	cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
	if (cmdbuf_obj->filp != filp) {
		//should not happen
		LOG_ERR("vcmd: ERROR cmdbuf_id = %u !!\n", cmdbuf_id);
		return -1;
	}
	dev = &hantrovcmd_data[cmdbuf_obj->core_id];
#ifdef IRQ_SIMULATION
	{
		u32 i, random_num;
		//get_random_bytes(&random_num, sizeof(u32));
		random_num = (u32)((u64)100 * cmdbuf_obj->executing_time / (4096 * 2160) + 50);
		LOG_DBG("random_num=%d\n", random_num);

		for (i = 0; i < 10000; i++) {
			if (!timer_reserve[i].timer) {
				timer_reserve[i].timer_id = i;
				timer_reserve[i].core_id = cmdbuf_obj->core_id;
				timer_reserve[i].timer = &timer[i];
				break;
			}
		}

		if (i == 10000) {
			LOG_ERR("vcmd: Exceed maximum timer number \n");
			return -1;
		}

		timer_setup(timer_reserve[i].timer, hantrovcmd_trigger_irq, 0);
		timer_reserve[i].timer->expires = jiffies + random_num * HZ / 10; //the expires time is 1s
		add_timer(timer_reserve[i].timer);
	}
#endif

	if (wait_event_interruptible(*dev->wait_queue, check_cmdbuf_irq(dev, cmdbuf_obj, irq_status_ret))) {
		LOG_ERR("vcmd_wait_queue_0 interrupted, cmdbuf_id = %u\n", cmdbuf_obj->cmdbuf_id);
		//abort the vcmd
		//vcmd_write_register_value((const void *)dev->hwregs,dev->reg_mirror,HWIF_VCMD_START_TRIGGER,0);
		return -ERESTARTSYS;
	}

	/** statistics the total cycles*/
	u32 *status_base_virt_addr =
			vcmd_status_buf_mem_pool.virtualAddress + cmdbuf_id * CMDBUF_MAX_SIZE / 4 +
			(dev->vcmd_core_cfg.submodule_main_addr / 2 / 4 + 0);
	u32 cycles = status_base_virt_addr[82];

	atomic64_add(cycles, &dev->core_tot_cycles);
	atomic64_add(cmdbuf_obj->executing_time, &dev->core_tot_exetime);
	// LOG_DBG("cmdbuf_id = %u, status_base_virt_addr=0x%llx, cycles = %u, tot_cycles=%llu, hwid = 0x%x\n"
	// 	, cmdbuf_id, (unsigned long long)status_base_virt_addr
	// 	, cycles, atomic64_read(&dev->core_tot_cycles), status_base_virt_addr[0]);

	return 0;
}

static long hantrovcmd_ioctl(struct file *filp, unsigned int cmd,
			     unsigned long arg)
{
	int err = 0;
	long retval = 0;

	LOG_DBG("ioctl cmd 0x%08x\n", cmd);
	/*
     * extract the type and number bitfields, and don't encode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
	if (_IOC_TYPE(cmd) != HANTRO_IOC_MAGIC)
		return -ENOTTY;
	if ((_IOC_TYPE(cmd) == HANTRO_IOC_MAGIC && _IOC_NR(cmd) > HANTRO_IOC_MAXNR))
		return -ENOTTY;

	/*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !AX_ACCESS_OK((void *)arg, _IOC_SIZE(cmd)); /* AX630C-PORT */
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !AX_ACCESS_OK((void *)arg, _IOC_SIZE(cmd)); /* AX630C-PORT */
	if (err)
		return -EFAULT;

	switch (cmd) {
	case HANTRO_IOCH_GET_VCMD_ENABLE: {
		__put_user(1, (unsigned long __user *)arg);
		break;
	}

	case HANTRO_IOCH_GET_MMU_ENABLE: {
		__put_user(mmu_enable, (unsigned int __user  *)arg);
		break;
	}

	case HANTRO_IOCH_GET_CMDBUF_PARAMETER: {
		struct cmdbuf_mem_parameter local_cmdbuf_mem_data;

		LOG_DBG(" VCMD GET_CMDBUF_PARAMETER\n");
		local_cmdbuf_mem_data.cmd_unit_size = CMDBUF_MAX_SIZE;
		local_cmdbuf_mem_data.status_unit_size = CMDBUF_MAX_SIZE;
		local_cmdbuf_mem_data.cmd_total_size = CMDBUF_POOL_TOTAL_SIZE;
		local_cmdbuf_mem_data.status_total_size = CMDBUF_POOL_TOTAL_SIZE;
#ifdef SUPPORT_DMA_HEAP
		local_cmdbuf_mem_data.status_phy_addr = vcmd_status_buf_mem_pool.phy_address;
		local_cmdbuf_mem_data.cmd_phy_addr = vcmd_buf_mem_pool.phy_address;
#else
		local_cmdbuf_mem_data.status_phy_addr = vcmd_status_buf_mem_pool.busAddress;
		local_cmdbuf_mem_data.cmd_phy_addr = vcmd_buf_mem_pool.busAddress;
#endif
		if (mmu_enable) {
			local_cmdbuf_mem_data.status_hw_addr = vcmd_status_buf_mem_pool.mmu_bus_address;
			local_cmdbuf_mem_data.cmd_hw_addr = vcmd_buf_mem_pool.mmu_bus_address;
		} else {
#ifdef SUPPORT_DMA_HEAP
			local_cmdbuf_mem_data.status_hw_addr = vcmd_status_buf_mem_pool.busAddress;
			local_cmdbuf_mem_data.cmd_hw_addr = vcmd_buf_mem_pool.busAddress;
#else
			local_cmdbuf_mem_data.status_hw_addr = vcmd_status_buf_mem_pool.busAddress - base_ddr_addr;
			local_cmdbuf_mem_data.cmd_hw_addr = vcmd_buf_mem_pool.busAddress - base_ddr_addr;
#endif
		}
		local_cmdbuf_mem_data.base_ddr_addr = base_ddr_addr;

		// /* add by xiaogang.li for debug purpose */
		// LOG_INFO("status_phy_addr=0x%lx\n", local_cmdbuf_mem_data.status_phy_addr);
		// LOG_INFO("cmd_phy_addr=0x%lx\n", local_cmdbuf_mem_data.cmd_phy_addr);
		// LOG_INFO("status_hw_addr=0x%lx\n", local_cmdbuf_mem_data.status_hw_addr);
		// LOG_INFO("cmd_hw_addr=0x%lx\n", local_cmdbuf_mem_data.cmd_hw_addr);

		retval = copy_to_user((struct cmdbuf_mem_parameter __user *)arg, &local_cmdbuf_mem_data,
			     sizeof(struct cmdbuf_mem_parameter));
		if (retval) {
			LOG_DBG("copy_to_user failed, returned %li\n", retval);
			return -EFAULT;
		}

		break;
	}
	case HANTRO_IOCH_GET_VCMD_PARAMETER: {
		struct config_parameter input_para;

		LOG_DBG(" VCMD get vcmd config parameter\n");
		retval = copy_from_user(&input_para, (struct config_parameter __user *)arg,
			       sizeof(struct config_parameter));
		if (retval) {
			LOG_DBG("copy_from_user failed, returned %li\n", retval);
			return -EFAULT;
		}

		if (vcmd_type_core_num[input_para.module_type]) {
			input_para.submodule_main_addr =
				vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_main_addr;
			input_para.submodule_dec400_addr =
				vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_dec400_addr;
			input_para.submodule_L2Cache_addr =
				vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_L2Cache_addr;
			input_para.submodule_MMU_addr[0] =
				vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_MMU_addr[0];
			input_para.submodule_MMU_addr[1] =
				vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_MMU_addr[1];
			input_para.submodule_axife_addr[0] =
				vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_axife_addr[0];
			input_para.submodule_axife_addr[1] =
				vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_axife_addr[1];
			input_para.config_status_cmdbuf_id =
				vcmd_manager[input_para.module_type][0]->status_cmdbuf_id;
			input_para.vcmd_hw_version_id =
				vcmd_manager[input_para.module_type][0]->hw_version_id;
			input_para.vcmd_core_num = vcmd_type_core_num[input_para.module_type];

			// /* add by xiaogang.li for debug purpose */
			// LOG_INFO("main_addr=0x%lx\n", input_para.submodule_main_addr);
			// LOG_INFO("dec400_addr=0x%lx\n", input_para.submodule_dec400_addr);
			// LOG_INFO("L2Cache_addr=0x%lx\n", input_para.submodule_L2Cache_addr);
			// LOG_INFO("MMU_addr0=0x%lx\n", input_para.submodule_MMU_addr[0]);
			// LOG_INFO("MMU_addr1=0x%lx\n", input_para.submodule_MMU_addr[1]);
			// LOG_INFO("axife_addr0=0x%lx\n", input_para.submodule_axife_addr[0]);
			// LOG_INFO("axife_addr1=0x%lx\n", input_para.submodule_axife_addr[1]);
			// LOG_INFO("config_status_cmdbuf_id=%d\n", input_para.config_status_cmdbuf_id);
			// LOG_INFO("vcmd_hw_version_id=0x%x\n", input_para.vcmd_hw_version_id);
			// LOG_INFO("vcmd_core_num=%d\n", input_para.vcmd_core_num);
		} else {
			input_para.submodule_main_addr = 0xffff;
			input_para.submodule_dec400_addr = 0xffff;
			input_para.submodule_L2Cache_addr = 0xffff;
			input_para.submodule_MMU_addr[0] = 0xffff;
			input_para.submodule_MMU_addr[1] = 0xffff;
			input_para.submodule_axife_addr[0] = 0xffff;
			input_para.submodule_axife_addr[1] = 0xffff;
			input_para.config_status_cmdbuf_id = 0;
			input_para.vcmd_core_num = 0;
			input_para.vcmd_hw_version_id = HW_ID_1_0_C;
		}
		retval = copy_to_user((struct config_parameter __user *)arg, &input_para,
			     sizeof(struct config_parameter));
		if (retval) {
			LOG_DBG("copy_to_user failed, returned %li\n", retval);
			return -EFAULT;
		}

		break;
	}
	case HANTRO_IOCH_RESERVE_CMDBUF: {
		struct exchange_parameter input_para;

		retval = copy_from_user(&input_para, (struct exchange_parameter __user *)arg,
			       sizeof(struct exchange_parameter));
		if (retval) {
			LOG_DBG("copy_to_user failed, returned %li\n", retval);
			return -EFAULT;
		}

		retval = reserve_cmdbuf(filp, &input_para);
		if (retval == 0) {
			retval = copy_to_user((struct exchange_parameter __user *)arg,
					&input_para,
					sizeof(struct exchange_parameter));
			if (retval) {
				LOG_DBG("copy_to_user failed, returned %li\n", retval);
				return -EFAULT;
			}
		}
		LOG_DBG(" VCMD Reserve CMDBUF %d\n", input_para.cmdbuf_id);
		return retval;
	}

	case HANTRO_IOCH_LINK_RUN_CMDBUF: {
		struct exchange_parameter input_para;

		retval = copy_from_user(&input_para, (struct exchange_parameter __user *)arg,
			       sizeof(struct exchange_parameter));
		if (retval) {
			LOG_DBG("copy_to_user failed, returned %li\n", retval);
			return -EFAULT;
		}


		LOG_DBG("VCMD link and run cmdbuf\n");
		retval = link_and_run_cmdbuf(filp, &input_para);
		if (retval == 0) {
			retval = copy_to_user((struct exchange_parameter __user *)arg, &input_para,
					sizeof(struct exchange_parameter));
			if (retval) {
				LOG_DBG("copy_to_user failed, returned %li\n", retval);
				return -EFAULT;
			}
		}

		return retval;
		// break;
	}

	case HANTRO_IOCH_WAIT_CMDBUF: {
		u16 cmdbuf_id;
		u32 irq_status_ret = 0;

		__get_user(cmdbuf_id, (u16 __user *)arg);
		/*high 16 bits are core id, low 16 bits are cmdbuf_id*/

		LOG_DBG("VCMD wait for CMDBUF finishing.\n");

		//TODO
		retval = wait_cmdbuf_ready(filp, cmdbuf_id, &irq_status_ret);
		LOG_TRACE("wait cmdbuf_id for encoder [pid=%d][%u], status:%x\n", current->pid, cmdbuf_id, irq_status_ret);
		cmdbuf_id = (u16)irq_status_ret;
		if (retval == 0) {
			__put_user(cmdbuf_id, (u16 __user *)arg);
			return retval; //return core_id
		} else {
			return retval;
		}

		// break;
	}
	case HANTRO_IOCH_RELEASE_CMDBUF: {
		u16 cmdbuf_id;

		__get_user(cmdbuf_id, (u16 __user *)arg);
		/*16 bits are cmdbuf_id*/

		LOG_DBG("VCMD release CMDBUF\n");

		retval = release_cmdbuf(filp, cmdbuf_id);
		return retval;
	}
	case HANTRO_IOCH_POLLING_CMDBUF: {
		u16 core_id;

		__get_user(core_id, (u16 __user *)arg);
		/*16 bits are cmdbuf_id*/
		if (core_id >= venc_vcmd_core_num)
			return -1;
		hantrovcmd_isr(core_id, &hantrovcmd_data[core_id]);
		return 0;
		// break;
	}
#ifdef SUPPORT_DMA_HEAP
	case HANTRO_IOCH_DMA_HEAP_GET_IOVA: {
		struct dmabuf_cfg dbcfg;
		size_t buf_size = 0;
		struct heap_mem *hmem, *hmem_d1;
		struct filp_priv *fp_priv = (struct filp_priv *)filp->private_data;

		if (copy_from_user(&dbcfg, (void __user *)arg, sizeof(struct dmabuf_cfg)) != 0)
			return -EFAULT;

		/* map the pha to dma addr(iova)*/
		hmem = common_dmabuf_heap_import_from_user(&fp_priv->root, dbcfg.dmabuf_fd);
		if(IS_ERR(hmem)) {
			LOG_ERR("dmabuf-heap import from userspace failed\n");
			return -ENOMEM;
		}
		dbcfg.iova = (unsigned long)sg_dma_address(hmem->sgt->sgl);
		/* get the size of the dmabuf allocated by dmabuf_heap */
		buf_size = common_dmabuf_heap_get_size(hmem);
		LOG_DBG("fd = %d, dmabuf info: CPU VA:0x%lx, PA:0x%lx, DMA addr(iova):0x%lx, size=0x%lx\n"
				, dbcfg.dmabuf_fd
				, (unsigned long)hmem->vaddr
				, (unsigned long)sg_phys(hmem->sgt->sgl)
				, (unsigned long)sg_dma_address(hmem->sgt->sgl)
				, (unsigned long)buf_size);

		if (venc_pdev_d1) {
			hmem_d1 = common_dmabuf_heap_rsv_iova_map(&fp_priv->root_d1, dbcfg.dmabuf_fd, dbcfg.iova, buf_size);
			if ((IS_ERR(hmem_d1))) {
				LOG_ERR("dmabuf-heap rsv iova map failed for d1\n");
				common_dmabuf_heap_release(hmem);
				return err;
			}
		}

		retval = copy_to_user((u32 __user *)arg, &dbcfg,
				sizeof(struct dmabuf_cfg));
		if (retval) {
			if (venc_pdev_d1)
				common_dmabuf_heap_rsv_iova_unmap(hmem_d1);

			common_dmabuf_heap_release(hmem);
			LOG_DBG("copy_to_user failed, returned %li\n", retval);
			return -EFAULT;
		}

		return 0;
	}
	case HANTRO_IOCH_DMA_HEAP_PUT_IOVA: {
		struct heap_mem *hmem, *hmem_d1;
		unsigned int dmabuf_fd;
		struct filp_priv *fp_priv = (struct filp_priv *)filp->private_data;

		if (copy_from_user(&dmabuf_fd, (void __user *)arg, sizeof(int)) != 0)
			return -EFAULT;

		/* find the heap_mem */
		hmem = common_dmabuf_lookup_heapobj_by_fd(&fp_priv->root, dmabuf_fd);
		if(IS_ERR(hmem)) {
			LOG_ERR("cannot find dmabuf-heap for dmabuf_fd %d\n", dmabuf_fd);
			return -ENOMEM;
		}
		LOG_DBG("release dmabuf_fd = %d, iova 0x%llx\n", dmabuf_fd, sg_dma_address(hmem->sgt->sgl));

		if (venc_pdev_d1) {
			hmem_d1 = common_dmabuf_lookup_heapobj_by_fd(&fp_priv->root_d1, dmabuf_fd);
			if (IS_ERR(hmem_d1)) {
				LOG_ERR("cannot find dmabuf-heap for dmabuf_fd %d on d1\n", dmabuf_fd);
				return -EFAULT;
			}
			common_dmabuf_heap_rsv_iova_unmap(hmem_d1);
		}

		common_dmabuf_heap_release(hmem);
		return 0;
	}
#endif
	/* AX630C-PORT: from-source frame-buffer allocator over the CMM carveout
	 * (#45) -- replaces the fixed-address /dev/mem placement. */
	case HANTRO_IOCH_ALLOC_FRAMEBUF: {
		struct framebuf_parameter fb;

		if (copy_from_user(&fb, (void __user *)arg, sizeof(fb)))
			return -EFAULT;
		retval = vcmd_fb_alloc(filp, fb.size, &fb.bus_addr);
		if (retval)
			return retval;
		if (copy_to_user((void __user *)arg, &fb, sizeof(fb))) {
			vcmd_fb_free(filp, fb.bus_addr);
			return -EFAULT;
		}
		break;
	}
	case HANTRO_IOCH_FREE_FRAMEBUF: {
		struct framebuf_parameter fb;

		if (copy_from_user(&fb, (void __user *)arg, sizeof(fb)))
			return -EFAULT;
		return vcmd_fb_free(filp, fb.bus_addr);
	}
	/* AX630C-PORT: dma-buf import -> bus address, for zero-copy V4L2 capture
	 * frames as encoder input (#60 M3). */
	case HANTRO_IOCH_IMPORT_DMABUF: {
		struct dmabuf_import_parameter im;

		if (copy_from_user(&im, (void __user *)arg, sizeof(im)))
			return -EFAULT;
		retval = vcmd_dmabuf_import(filp, im.fd, &im.bus_addr, &im.size);
		if (retval)
			return retval;
		if (copy_to_user((void __user *)arg, &im, sizeof(im))) {
			vcmd_dmabuf_release(filp, im.bus_addr);
			return -EFAULT;
		}
		break;
	}
	case HANTRO_IOCH_RELEASE_DMABUF: {
		struct dmabuf_import_parameter im;

		if (copy_from_user(&im, (void __user *)arg, sizeof(im)))
			return -EFAULT;
		return vcmd_dmabuf_release(filp, im.bus_addr);
	}
	default: {
		LOG_DBG("inivalid IOCTL\n");
		return -1;
	}
	}
	return 0;
}

static int hantrovcmd_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long size = 0;
	dma_addr_t phy_addr = (vma->vm_pgoff << PAGE_SHIFT);
	dma_addr_t bus_addr = 0;
	u32* cpu_vaddr = NULL;
	int ret = 0;

	LOG_DBG("hantrovcmd_mmap, phy_addr=%llx\n", (long long unsigned int)phy_addr);

	/** find the dma_addr that will be mmapped.
	 * AX630C-PORT: match on EITHER busAddress or phy_address. The pools live
	 * in a dma_declare_coherent_memory() carveout (glue), where
	 * phy_address = pfn_to_phys(vmalloc_to_pfn(ioremap'd virt)) does NOT equal
	 * busAddress (the dma_handle). GET_CMDBUF_PARAMETER hands userspace
	 * busAddress (drv:1989-1990), so a mmap keyed only on phy_address rejects
	 * the offset userspace actually has. Accepting busAddress makes the
	 * driver self-consistent; dma_mmap_coherent(cpu_vaddr,bus_addr) then maps
	 * the declared region correctly regardless of the passed offset. */
	if (phy_addr == vcmd_status_buf_mem_pool.busAddress ||
	    phy_addr == vcmd_status_buf_mem_pool.phy_address) {
		cpu_vaddr = vcmd_status_buf_mem_pool.virtualAddress;
		bus_addr = vcmd_status_buf_mem_pool.busAddress;
		size = vcmd_status_buf_mem_pool.size;
	} else if(phy_addr == vcmd_buf_mem_pool.busAddress ||
		  phy_addr == vcmd_buf_mem_pool.phy_address) {
		cpu_vaddr = vcmd_buf_mem_pool.virtualAddress;
		bus_addr = vcmd_buf_mem_pool.busAddress;
		size = vcmd_buf_mem_pool.size;
	} else if (vcmd_fb_lookup((unsigned long)phy_addr, &size) == 0) {
		/* AX630C-PORT: a frame-buffer allocation (#45). Same writecombine
		 * remap_pfn_range path as the pools below. */
		bus_addr = phy_addr;
	} else {
		LOG_ERR("hantrovcmd_mmap, Unknown phy_addr(%llx), mmap failed\n", (long long unsigned int)phy_addr);
		return -ENXIO;
	}

	LOG_DBG("hantrovcmd_mmap, bus_addr=0x%llx, phy_addr=0x%llx, cpu_vaddr=0x%llx, size=%lx\n"
		, (long long unsigned int)bus_addr, (long long unsigned int)phy_addr, (unsigned long long)cpu_vaddr, size);

	/* AX630C-PORT: map the pool directly by its (fixed, reserved-carveout)
	 * bus/phys address. dma_mmap_coherent's declared-region path is unreliable
	 * here, and the pool lives outside kernel-managed DRAM (mem=... carveout),
	 * so a plain remap_pfn_range with a non-cached prot -- matching the pool's
	 * DMA-coherent kernel mapping -- is both correct and robust. This is what
	 * the vendor stack achieved via /dev/mem, minus the STRICT_DEVMEM/no-struct-
	 * page hazards of mapping a reserved region through /dev/mem. */
	{
		unsigned long user_bytes = vma->vm_end - vma->vm_start;
		unsigned long pfn = (unsigned long)(bus_addr >> PAGE_SHIFT);

		if (user_bytes > size) {
			LOG_ERR("hantrovcmd_mmap, request %lu > pool %lu\n", user_bytes, size);
			return -EINVAL;
		}
		vma->vm_pgoff = 0;
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		ret = remap_pfn_range(vma, vma->vm_start, pfn, user_bytes,
				      vma->vm_page_prot);
		if (ret) {
			LOG_ERR("hantrovcmd_mmap, remap_pfn_range failed, ret:%d\n", ret);
			return ret;
		}
	}

	return 0;
}

/******************************************************************************
 *process manager object management
 ******************************************************************************
 */
static struct process_manager_obj *create_process_manager_obj(void)
{
	struct process_manager_obj *process_manager_obj = NULL;

	process_manager_obj = kmalloc(sizeof(struct process_manager_obj), GFP_ATOMIC);
	if (!process_manager_obj) {
		LOG_DBG("%s\n", "kmalloc for process_manager_obj fail!");
		return process_manager_obj;
	}
	memset(process_manager_obj, 0, sizeof(struct process_manager_obj));
	return process_manager_obj;
}

static void free_process_manager_obj( struct process_manager_obj *process_manager_obj)
{
	if (!process_manager_obj) {
		LOG_DBG("%s\n", "free_process_manager_obj NULL");
		return;
	}
	//free current cmdbuf_obj
	kfree(process_manager_obj);
}

static bi_list_node *create_process_manager_node(void)
{
	bi_list_node *current_node = NULL;
	struct process_manager_obj *process_manager_obj = NULL;

	process_manager_obj = create_process_manager_obj();
	if (!process_manager_obj) {
		LOG_DBG("%s\n", "create_process_manager_obj fail!");
		return NULL;
	}
	process_manager_obj->total_exe_time = 0;
	spin_lock_init(&process_manager_obj->spinlock);
	init_waitqueue_head(&process_manager_obj->wait_queue);
	current_node = bi_list_create_node();
	if (!current_node) {
		LOG_DBG("%s\n", "bi_list_create_node fail!");
		free_process_manager_obj(process_manager_obj);
		return NULL;
	}
	current_node->data = (void *)process_manager_obj;
	return current_node;
}

static void free_process_manager_node(bi_list_node *process_node)
{
	struct process_manager_obj *process_manager_obj = NULL;

	if (!process_node) {
		LOG_DBG("%s\n", "free_process_manager_node NULL");
		return;
	}
	process_manager_obj = (struct process_manager_obj *)process_node->data;
	//free struct process_manager_obj
	free_process_manager_obj(process_manager_obj);
	//free current process_manager_obj entity.
	bi_list_free_node(process_node);
}

static long release_process_node_cleanup(bi_list *list)
{
	bi_list_node *new_process_node = NULL;

	while (1) {
		new_process_node = list->head;
		if (!new_process_node)
			break;
		//remove node from list
		bi_list_remove_node(list, new_process_node);
		//remove node from list
		free_process_manager_node(new_process_node);
	}
	return 0;
}

static int create_kernel_process_manager(void)
{
	bi_list_node *process_manager_node;
	struct process_manager_obj *process_manager_obj = NULL;

	process_manager_node = create_process_manager_node();
	if (!process_manager_node) {
		LOG_ERR("create process manager node failed\n");
		return -1;
	}
	process_manager_obj =
		(struct process_manager_obj *)process_manager_node->data;
	process_manager_obj->filp = NULL;
	bi_list_insert_node_tail(&global_process_manager, process_manager_node);

	return 0;
}

/* Update the last JMP cmd in cmdbuf_ojb in order to jump to next_cmdbuf_obj. */
static void cmdbuf_update_jmp_cmd(int hw_version_id,
				  struct cmdbuf_obj *cmdbuf_obj,
				  struct cmdbuf_obj *next_cmdbuf_obj,
				  int jmp_IE_1)
{
	u32 *jmp_addr;
	u32 operation_code;

	if (!cmdbuf_obj)
		return;

	if (cmdbuf_obj->has_end_cmdbuf == 0) {
		//need to link, current cmdbuf link to next cmdbuf
		jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress +
			   (cmdbuf_obj->cmdbuf_size / 4);
		if (!next_cmdbuf_obj) {
			operation_code = *(jmp_addr - 4);
			operation_code >>= 16;
			operation_code <<= 16;
			*(jmp_addr - 4) = (u32)(operation_code & ~JMP_RDY_1);
		} else {
			if (hw_version_id > HW_ID_1_0_C) {
				//set next cmdbuf id
				*(jmp_addr - 1) = next_cmdbuf_obj->cmdbuf_id;
			}
			if (mmu_enable) {
				if (sizeof(size_t) == 8) {
					*(jmp_addr - 2) = (u32)(
						(u64)(next_cmdbuf_obj->mmu_cmdbuf_busAddress) >> 32);
				} else {
					*(jmp_addr - 2) = 0;
				}
				*(jmp_addr - 3) = (u32)(next_cmdbuf_obj->mmu_cmdbuf_busAddress);
			} else {
				if (sizeof(size_t) == 8) {
					*(jmp_addr - 2) = (u32)(
						(u64)(next_cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr) >> 32);
				} else {
					*(jmp_addr - 2) = 0;
				}
				*(jmp_addr - 3) = (u32)(
					next_cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr);
			}
			operation_code = *(jmp_addr - 4);
			operation_code >>= 16;
			operation_code <<= 16;
			*(jmp_addr - 4) =
				(u32)(operation_code | JMP_RDY_1 | jmp_IE_1 |
				      ((next_cmdbuf_obj->cmdbuf_size + 7) / 8));
		}

#ifdef VCMD_DEBUG_INTERNAL
		{
			u32 i;

			LOG_INFO("vcmd link, last cmdbuf content\n");
			for (i = cmdbuf_obj->cmdbuf_size / 4 - 8;
			     i < cmdbuf_obj->cmdbuf_size / 4; i++) {
				LOG_INFO("current linked cmdbuf data %d =0x%x\n",
					i, *(cmdbuf_obj->cmdbuf_virtualAddress + i));
			}
		}
#endif
	}
}

/* delink given cmd buffer (cmdbuf_node) and remove it from list.
 * Also modify the last JMP of buf P to point to cmdbuf N.
 * Used when a process is terminated but there are pending cmd bufs in vmcd list.
 * E.g.,
 * before:
 *
 *    L->L->...->P->X->N->        ...        ->L
 *    ^             ^                          ^
 *    head          cmdbuf_node                 tail
 *
 *  end:
 *
 *   L->L->...->P->N->        ...        ->L
 *    ^                                     ^
 *    head                                  tail
 *
 *  Return: pointer to N or NULL if N doesn't exist.
 */
static void vcmd_delink_rm_cmdbuf(struct hantrovcmd_dev *dev,
				  bi_list_node *cmdbuf_node)
{
	bi_list *list = &dev->list_manager;
	struct cmdbuf_obj *cmdbuf_obj = (struct cmdbuf_obj *)cmdbuf_node->data;
	bi_list_node *prev = cmdbuf_node->previous;
	bi_list_node *next = cmdbuf_node->next;

	LOG_DBG("Delink and remove cmdbuf [%d] from vcmd list.\n",
	       cmdbuf_obj->cmdbuf_id);
	if (prev) {
		LOG_DBG("prev cmdbuf [%d].\n",
		       ((struct cmdbuf_obj *)prev->data)->cmdbuf_id);
	} else {
		LOG_DBG("NO prev cmdbuf.\n");
	}
	if (next) {
		LOG_DBG("next cmdbuf [%d].\n",
		       ((struct cmdbuf_obj *)next->data)->cmdbuf_id);
	} else {
		LOG_DBG("NO next cmdbuf.\n");
	}

	bi_list_remove_node(list, cmdbuf_node);
	global_cmdbuf_node[cmdbuf_obj->cmdbuf_id] = NULL;
	free_cmdbuf_node(cmdbuf_node);

	cmdbuf_update_jmp_cmd(dev->hw_version_id, prev ? prev->data : NULL,
			      next ? next->data : NULL,
			      dev->duration_without_int >
				      dev->duration_without_int_max);
}

static int hantrovcmd_open(struct inode *inode, struct file *filp)
{
	int result = 0;
	struct hantrovcmd_dev *dev = hantrovcmd_data;
	bi_list_node *process_manager_node;
	unsigned long flags;
	struct process_manager_obj *process_manager_obj = NULL;
	struct filp_priv *fp_priv;

	fp_priv = kzalloc(sizeof(struct filp_priv), GFP_KERNEL);
	if (!fp_priv) {
		pr_err("%s: alloc failed\n", __func__);
		return -ENOMEM;
	}
	/*
	 * AX630C: keep register access and async DMA powered until final close,
	 * including the lifetime of VMAs that retain this file.
	 */
	result = enc_pm_runtime_get(0);
	if (result < 0) {
		kfree(fp_priv);
		return result;
	}
#ifdef SUPPORT_DMA_HEAP
		common_dmabuf_heap_import_init(&fp_priv->root, &venc_pdev->dev);
	if (venc_pdev_d1) {
		common_dmabuf_heap_import_init(&fp_priv->root_d1, &venc_pdev_d1->dev);
	}
#endif
	fp_priv->dev = (void*)dev;
	for (u32 core_id = 0; core_id < ENC_CORE_NUM; core_id ++) {
		atomic_set(&(fp_priv->core_tasks[core_id]), 0);
	}
	filp->private_data = (void *)fp_priv;
	process_manager_node = create_process_manager_node();
	if (!process_manager_node) {
		kfree(fp_priv);
		enc_pm_runtime_put(0);
		return -1;
	}
	process_manager_obj =
		(struct process_manager_obj *)process_manager_node->data;
	process_manager_obj->filp = filp;
	spin_lock_irqsave(&vcmd_process_manager_lock, flags);
	bi_list_insert_node_tail(&global_process_manager, process_manager_node);
	spin_unlock_irqrestore(&vcmd_process_manager_lock, flags);
	LOG_DBG("dev opened\n");
	return result;
}

static int hantrovcmd_release(struct inode *inode, struct file *filp)
{
	struct filp_priv *fp_priv = (struct filp_priv *)filp->private_data;
	struct hantrovcmd_dev *dev = (struct hantrovcmd_dev *)fp_priv->dev;
	u32 core_id = 0;
	u32 release_cmdbuf_num = 0;
	bi_list_node *new_cmdbuf_node = NULL;
	struct cmdbuf_obj *cmdbuf_obj_temp = NULL;
	bi_list_node *process_manager_node;
	struct process_manager_obj *process_manager_obj = NULL;
	int vcmd_aborted = 0; // vcmd is aborted in this function
	struct cmdbuf_obj *restart_cmdbuf = NULL;
	bi_list_node *next_cmdbuf = NULL;

	unsigned long flags;
	long retVal = 0;

	LOG_DBG("dev closed for process %p\n", (void *)filp);

	if (dev->hw_version_id >= HW_ID_1_2_1) {
		for (core_id = 0; core_id < venc_vcmd_core_num; core_id++) {
			if (down_interruptible(&vcmd_reserve_cmdbuf_sem[dev[core_id].vcmd_core_cfg.sub_module_type]))
					goto error;
			spin_lock_irqsave(dev[core_id].spinlock, flags);
			new_cmdbuf_node = dev[core_id].list_manager.head;
			while (1) {
				if (!new_cmdbuf_node)
					break;
				next_cmdbuf = new_cmdbuf_node->next;
				cmdbuf_obj_temp = (struct cmdbuf_obj *)new_cmdbuf_node->data;
				LOG_DBG("Process %p is releasing: checking cmdbuf %d of process %p.\n",
				       filp, cmdbuf_obj_temp->cmdbuf_id, cmdbuf_obj_temp->filp);
				if (dev[core_id].hwregs &&
				    cmdbuf_obj_temp->filp == filp) {
					if (cmdbuf_obj_temp->cmdbuf_run_done) {
						cmdbuf_obj_temp->cmdbuf_need_remove = 1;
						retVal = release_cmdbuf_node(
							&dev[core_id].list_manager, new_cmdbuf_node);
						if (retVal == 1)
							cmdbuf_obj_temp->process_manager_obj = NULL;
					} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 0) {
						cmdbuf_obj_temp->cmdbuf_data_linked = 1;
						cmdbuf_obj_temp->cmdbuf_run_done = 1;
						cmdbuf_obj_temp->cmdbuf_need_remove = 1;
						retVal = release_cmdbuf_node(
							&dev[core_id].list_manager, new_cmdbuf_node);
						if (retVal == 1)
							cmdbuf_obj_temp->process_manager_obj = NULL;
					} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 1 &&
						   dev[core_id].working_state == WORKING_STATE_IDLE) {
						vcmd_delink_rm_cmdbuf(&dev[core_id], new_cmdbuf_node);
						if (restart_cmdbuf == cmdbuf_obj_temp)
							restart_cmdbuf = next_cmdbuf ? next_cmdbuf->data : NULL;;
					} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 1 &&
						   dev[core_id].working_state == WORKING_STATE_WORKING) {
						bi_list_node *last_cmdbuf_node = NULL;
						bi_list_node *done_cmdbuf_node = NULL;
						int abort_cmdbuf_id;
						int loop_count = 0;

						//abort the vcmd and wait
						LOG_DBG("Abort due to linked cmdbuf %d of current process.\n",
						       cmdbuf_obj_temp->cmdbuf_id);
#ifdef VCMD_DEBUG_INTERNAL
						printk_vcmd_register_debug(
							(const void *)dev[core_id].hwregs, "Before trigger to 0");
#endif
						// disable abort interrupt
						vcmd_write_register_value((const void *)dev[core_id].hwregs,
									  dev[core_id].reg_mirror, HWIF_VCMD_START_TRIGGER, 0);
						vcmd_aborted = 1;
						dev->software_triger_abort = 1;
#ifdef VCMD_DEBUG_INTERNAL
						printk_vcmd_register_debug(
							(const void *)dev[core_id].hwregs,
							"After trigger to 0");
#endif
						// wait vcmd core aborted and vcmd enters IDLE mode.
						while (vcmd_get_register_value((const void *)dev[core_id].hwregs,
									       dev[core_id].reg_mirror, HWIF_VCMD_WORK_STATE)) {
							loop_count++;
							if (!(loop_count % 10)) {
								u32 irq_status = vcmd_read_reg(
									(const void *)dev[core_id].hwregs,
									VCMD_REGISTER_INT_STATUS_OFFSET);
								LOG_ERR("vcmd: expected idle state, but irq status = 0x%0x\n",
								       irq_status);
								LOG_ERR("vcmd: vcmd current status is %d\n",
								       vcmd_get_register_value((const void *)dev[core_id].hwregs,
											       dev[core_id].reg_mirror, HWIF_VCMD_WORK_STATE));
							}
							mdelay(10); // wait 10ms
							if (loop_count > 100) { // too long
								LOG_ERR("vcmd: too long before vcmd core to IDLE state\n");
								spin_unlock_irqrestore(
									dev[core_id].spinlock, flags);
								up(&vcmd_reserve_cmdbuf_sem[dev[core_id].vcmd_core_cfg.sub_module_type]);
								dev->software_triger_abort = 0;
								goto error;
							}
						}
						dev[core_id].working_state = WORKING_STATE_IDLE;
						// clear interrupt & restore abort_e
						if (vcmd_get_register_value((const void *)dev[core_id].hwregs,
									    dev[core_id].reg_mirror, HWIF_VCMD_IRQ_ABORT)) {
							LOG_DBG("Abort interrupt triggered, now clear it to avoid abort int...\n");
							vcmd_write_reg((const void *)dev[core_id].hwregs,
								       VCMD_REGISTER_INT_STATUS_OFFSET, 0x1 << 4);
							LOG_DBG("Now irq status = 0x%0x.\n",
							       vcmd_read_reg((const void *)dev[core_id].hwregs,
									     VCMD_REGISTER_INT_STATUS_OFFSET));
						}

						abort_cmdbuf_id = vcmd_get_register_value((const void *)dev[core_id].hwregs,
											  dev[core_id].reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID);
						LOG_DBG("Abort when executing cmd buf %d.\n", abort_cmdbuf_id);
						dev[core_id].sw_cmdbuf_rdy_num = 0;
						dev[core_id].duration_without_int = 0;
						vcmd_write_register_value((const void *)dev[core_id].hwregs,
									  dev[core_id].reg_mirror, HWIF_VCMD_EXE_CMDBUF_COUNT, 0);
						vcmd_write_register_value((const void *)dev[core_id].hwregs,
									  dev[core_id].reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT, 0);

						/* Mark cmdbuf_run_done to 1 for all the cmd buf executed. */
						done_cmdbuf_node =
							dev[core_id].list_manager.head;
						while (done_cmdbuf_node) {
							if (!((struct cmdbuf_obj *)done_cmdbuf_node->data)->cmdbuf_run_done) {
								((struct cmdbuf_obj *)done_cmdbuf_node->data)->cmdbuf_run_done = 1;
								((struct cmdbuf_obj *)done_cmdbuf_node->data)->cmdbuf_data_linked = 0;
								LOG_DBG("Set cmdbuf [%d] cmdbuf_run_done to 1.\n",
								       ((struct cmdbuf_obj *)done_cmdbuf_node->data)->cmdbuf_id);
							}
							if (((struct cmdbuf_obj *)done_cmdbuf_node->data)->cmdbuf_id == abort_cmdbuf_id)
								break;
							done_cmdbuf_node = done_cmdbuf_node->next;
						}
						if (cmdbuf_obj_temp->cmdbuf_run_done) {
							/* current cmdbuf is in fact has been executed, but due to interrupt is not triggered,
							 * the status is not updated.
							 * Just delink and remove it from the list. */
							if (done_cmdbuf_node &&
							    done_cmdbuf_node->data) {
								LOG_DBG("done_cmdbuf_node is cmdbuf [%d].\n",
								       ((struct cmdbuf_obj *)done_cmdbuf_node->data)->cmdbuf_id);
							}
							done_cmdbuf_node = done_cmdbuf_node->next;
							if (done_cmdbuf_node)
								restart_cmdbuf =
									(struct cmdbuf_obj *)done_cmdbuf_node->data;
							if (restart_cmdbuf) {
								LOG_DBG("Set restart cmdbuf [%d] via if.\n", restart_cmdbuf->cmdbuf_id);
							}
						} else {
							last_cmdbuf_node = new_cmdbuf_node;
							/* cmd buf num from aborted cmd buf to current cmdbuf_obj_temp */
							if (cmdbuf_obj_temp->cmdbuf_id != abort_cmdbuf_id) {
								last_cmdbuf_node = new_cmdbuf_node->previous;

								while (last_cmdbuf_node &&
								       ((struct cmdbuf_obj *)last_cmdbuf_node->data)->cmdbuf_id != abort_cmdbuf_id) {
									restart_cmdbuf = (struct cmdbuf_obj *)last_cmdbuf_node->data;
									last_cmdbuf_node = last_cmdbuf_node->previous;
									dev[core_id].sw_cmdbuf_rdy_num++;
									dev[core_id].duration_without_int += restart_cmdbuf->executing_time;
									LOG_DBG("Keep valid cmdbuf [%d] in the list.\n", restart_cmdbuf->cmdbuf_id);
								}
							}
							if (restart_cmdbuf) {
								LOG_DBG("Set restart cmdbuf [%d] via else.\n", restart_cmdbuf->cmdbuf_id);
							}
						}

						// remove first linked cmdbuf from list
						vcmd_delink_rm_cmdbuf(&dev[core_id], new_cmdbuf_node);
					}
					dev->software_triger_abort = 0;
					release_cmdbuf_num++;
					LOG_DBG("release reserved cmdbuf\n");
				} else if (vcmd_aborted && !cmdbuf_obj_temp->cmdbuf_run_done) {
					/* VCMD is aborted, need to re-calculate the duration_without_int */
					if (!restart_cmdbuf)
						restart_cmdbuf = cmdbuf_obj_temp; /* first cmdbuf to be restarted */
					dev[core_id].duration_without_int += cmdbuf_obj_temp->executing_time;
					dev[core_id].sw_cmdbuf_rdy_num++;
				}

				new_cmdbuf_node = next_cmdbuf;
			}

			if (restart_cmdbuf && restart_cmdbuf->core_id == core_id) {
				u32 irq_status1, irq_status2;

				LOG_DBG("Restart from cmdbuf [%d] after aborting.\n",
				       restart_cmdbuf->cmdbuf_id);

				irq_status1 = vcmd_read_reg((const void *)dev[core_id].hwregs,
							    VCMD_REGISTER_INT_STATUS_OFFSET);
				vcmd_write_reg((const void *)dev[core_id].hwregs,
					       VCMD_REGISTER_INT_STATUS_OFFSET, irq_status1);
				irq_status2 = vcmd_read_reg((const void *)dev[core_id].hwregs,
							    VCMD_REGISTER_INT_STATUS_OFFSET);
				LOG_DBG("Clear irq status from 0x%0x -> 0x%0x\n",
				       irq_status1, irq_status2);
				if (mmu_enable) {
					vcmd_write_register_value((const void *)dev[core_id].hwregs,
								  dev[core_id].reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR,
						(u32)(restart_cmdbuf->mmu_cmdbuf_busAddress));
					if (sizeof(size_t) == 8) {
						vcmd_write_register_value((const void *)dev[core_id].hwregs,
									  dev[core_id].reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
							(u32)((u64)(restart_cmdbuf->mmu_cmdbuf_busAddress) >> 32));
					} else {
						vcmd_write_register_value((const void *)dev[core_id].hwregs,
									  dev[core_id].reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR_MSB, 0);
					}
				} else {
					vcmd_write_register_value((const void *)dev[core_id].hwregs,
								  dev[core_id].reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR,
						(u32)(restart_cmdbuf->cmdbuf_busAddress - base_ddr_addr));
					if (sizeof(size_t) == 8) {
						vcmd_write_register_value((const void *)dev[core_id].hwregs,
									  dev[core_id].reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
							(u32)((u64)(restart_cmdbuf->cmdbuf_busAddress - base_ddr_addr) >> 32));
					} else {
						vcmd_write_register_value((const void *)dev[core_id].hwregs,
									  dev[core_id].reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR_MSB, 0);
					}
				}
				vcmd_write_register_value(
					(const void *)dev[core_id].hwregs,
					dev[core_id].reg_mirror,
					HWIF_VCMD_EXE_CMDBUF_COUNT, 0);
				vcmd_write_register_value((const void *)dev[core_id].hwregs,
							  dev[core_id].reg_mirror, HWIF_VCMD_EXE_CMDBUF_LENGTH,
					(u32)((restart_cmdbuf->cmdbuf_size + 7) / 8));
				vcmd_write_register_value((const void *)dev[core_id].hwregs,
							  dev[core_id].reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID,
					restart_cmdbuf->cmdbuf_id);
				vcmd_write_register_value((const void *)dev[core_id].hwregs,
							  dev[core_id].reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT,
							  dev[core_id].sw_cmdbuf_rdy_num);
#ifdef VCMD_DEBUG_INTERNAL
				printk_vcmd_register_debug(
					(const void *)dev[core_id].hwregs, "before restart");
#endif
				vcmd_write_register_value((const void *)dev[core_id].hwregs, dev[core_id].reg_mirror,
							  HWIF_VCMD_START_TRIGGER, 1);

				LOG_DBG("Restart from cmdbuf [%d] dev register sw_cmdbuf_rdy_num is %d\n",
				       restart_cmdbuf->cmdbuf_id,
				       vcmd_get_register_value((const void *)dev[core_id].hwregs,
							       dev[core_id].reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT));

				LOG_DBG("Restart from cmdbuf [%d] after aborting: start trigger = %d.\n",
				       restart_cmdbuf->cmdbuf_id,
				       vcmd_get_register_value((const void *)dev[core_id].hwregs,
							       dev[core_id].reg_mirror, HWIF_VCMD_START_TRIGGER));
				LOG_DBG("dev state from %d -> WORKING.\n", dev[core_id].working_state);
				dev[core_id].working_state = WORKING_STATE_WORKING;
#ifdef SUPPORT_WATCHDOG
				_vcmd_watchdog_feed(&dev[core_id]);
#endif
#ifdef VCMD_DEBUG_INTERNAL
				printk_vcmd_register_debug((const void *)dev[core_id].hwregs, "after restart");
#endif
			} else {
// #ifdef SUPPORT_WATCHDOG
// 				_vcmd_watchdog_stop(&dev[core_id]);
// #endif
				LOG_DBG("No more command buffer to be restarted, core_id = %u!\n", core_id);
			}
			spin_unlock_irqrestore(dev[core_id].spinlock, flags);
			// VCMD aborted but not restarted, nedd to wake up
			if (vcmd_aborted && !restart_cmdbuf)
				wake_up_interruptible_all(dev[core_id].wait_queue);
			up(&vcmd_reserve_cmdbuf_sem[dev[core_id].vcmd_core_cfg.sub_module_type]);
		}
	} else {
		LOG_ERR("vcmd hwid=0x%08x, unsupported\n", dev->hw_version_id);
		// for (core_id = 0; core_id < venc_vcmd_core_num; core_id++) {
		// 	if (down_interruptible(&vcmd_reserve_cmdbuf_sem[dev[core_id].vcmd_core_cfg.sub_module_type]))
		// 			goto error;
		// 	spin_lock_irqsave(dev[core_id].spinlock, flags);
		// 	new_cmdbuf_node = dev[core_id].list_manager.head;
		// 	while (1) {
		// 		if (!new_cmdbuf_node)
		// 			break;
		// 		next_cmdbuf = new_cmdbuf_node->next;
		// 		cmdbuf_obj_temp = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		// 		if (dev[core_id].hwregs && cmdbuf_obj_temp->filp == filp) {
		// 			if (cmdbuf_obj_temp->cmdbuf_run_done) {
		// 				cmdbuf_obj_temp->cmdbuf_need_remove = 1;
		// 				retVal = release_cmdbuf_node(&dev[core_id].list_manager,
		// 							     new_cmdbuf_node);
		// 				if (retVal == 1)
		// 					cmdbuf_obj_temp->process_manager_obj = NULL;
		// 			} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 0) {
		// 				cmdbuf_obj_temp->cmdbuf_data_linked = 1;
		// 				cmdbuf_obj_temp->cmdbuf_run_done = 1;
		// 				cmdbuf_obj_temp->cmdbuf_need_remove = 1;
		// 				retVal = release_cmdbuf_node(
		// 					&dev[core_id].list_manager, new_cmdbuf_node);
		// 				if (retVal == 1)
		// 					cmdbuf_obj_temp->process_manager_obj = NULL;
		// 			} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 1 &&
		// 				   dev[core_id].working_state == WORKING_STATE_IDLE) {
		// 				cmdbuf_obj_temp->cmdbuf_run_done = 1;
		// 				cmdbuf_obj_temp->cmdbuf_need_remove = 1;
		// 				retVal = release_cmdbuf_node(
		// 					&dev[core_id].list_manager, new_cmdbuf_node);
		// 				if (retVal == 1)
		// 					cmdbuf_obj_temp->process_manager_obj = NULL;
		// 			} else if (cmdbuf_obj_temp->cmdbuf_data_linked ==
		// 					   1 && dev[core_id].working_state == WORKING_STATE_WORKING) {
		// 				bi_list_node *last_cmdbuf_node;
		// 				u32 record_last_cmdbuf_rdy_num;
		// 				//abort the vcmd and wait
		// 				//vcmd_write_register_value((const void *)dev[core_id].hwregs,dev[core_id].reg_mirror,HWIF_VCMD_START_TRIGGER,0);
		// 				dev->software_triger_abort = 1;
		// 				if (wait_event_interruptible(*dev[core_id].wait_abort_queue, wait_abort_rdy(&dev[core_id]))) {
		// 					spin_unlock_irqrestore(dev[core_id].spinlock, flags);
		// 					up(&vcmd_reserve_cmdbuf_sem[dev[core_id].vcmd_core_cfg.sub_module_type]);
		// 					dev->software_triger_abort = 0;
		// 					goto error;
		// 				}
		// 				dev->software_triger_abort = 0;
		// 				cmdbuf_obj_temp->cmdbuf_run_done = 1;
		// 				cmdbuf_obj_temp->cmdbuf_need_remove = 1;
		// 				retVal = release_cmdbuf_node(
		// 					&dev[core_id].list_manager, new_cmdbuf_node);
		// 				if (retVal == 1)
		// 					cmdbuf_obj_temp->process_manager_obj = NULL;
		// 				//link
		// 				last_cmdbuf_node =
		// 					find_last_linked_cmdbuf(dev[core_id].list_manager.tail);
		// 				record_last_cmdbuf_rdy_num = dev[core_id].sw_cmdbuf_rdy_num;
		// 				vcmd_link_cmdbuf(&dev[core_id], last_cmdbuf_node);
		// 				//re-run
		// 				if (dev[core_id].sw_cmdbuf_rdy_num)
		// 					vcmd_start(&dev[core_id], last_cmdbuf_node);
		// 			}
		// 			release_cmdbuf_num++;
		// 			LOG_DBG("release reserved cmdbuf\n");
		// 		}
		// 		new_cmdbuf_node = next_cmdbuf;
		// 	}
		// 	spin_unlock_irqrestore(dev[core_id].spinlock, flags);
		// 	up(&vcmd_reserve_cmdbuf_sem[dev[core_id].vcmd_core_cfg.sub_module_type]);
		// }
	}

	if (release_cmdbuf_num)
		wake_up_interruptible_all(&vcmd_cmdbuf_memory_wait);
	spin_lock_irqsave(&vcmd_process_manager_lock, flags);
	process_manager_node = global_process_manager.head;
	while (1) {
		if (!process_manager_node)
			break;
		process_manager_obj = (struct process_manager_obj *)
					      process_manager_node->data;
		if (process_manager_obj->filp == filp)
			break;
		process_manager_node = process_manager_node->next;
	}
	//remove node from list
	bi_list_remove_node(&global_process_manager, process_manager_node);
	spin_unlock_irqrestore(&vcmd_process_manager_lock, flags);
	free_process_manager_node(process_manager_node);

	for (u32 core_id = 0; core_id < ENC_CORE_NUM; core_id ++) {
		/** clear the tasks for pm*/
		int n = atomic_xchg(&(fp_priv->core_tasks[core_id]), 0);

		while (n-- > 0)
			enc_pm_runtime_put(core_id);
	}
#ifdef SUPPORT_DMA_HEAP
	if (venc_pdev_d1) {
		common_dmabuf_heap_rsv_iova_uninit(&fp_priv->root_d1);
	}

	common_dmabuf_heap_import_uninit(&fp_priv->root);
#endif
	vcmd_fb_release_filp(filp);  /* AX630C-PORT: drop this file's frame buffers (#45) */
	kfree(fp_priv);
	/* Abort/delink, buffer release and any pending ISR precede autosuspend. */
	enc_pm_runtime_put(0);
	return 0;

error:
	/*
	 * A failed abort may leave DMA live. Retain power rather than gating a
	 * running command engine; recovery requires resetting the device.
	 */
	LOG_ERR("VCMD close failed; retaining runtime power for unfinished DMA\n");
#ifdef SUPPORT_DMA_HEAP
	if (venc_pdev_d1) {
		common_dmabuf_heap_rsv_iova_uninit(&fp_priv->root_d1);
	}

	common_dmabuf_heap_import_uninit(&fp_priv->root);
#endif
	vcmd_fb_release_filp(filp);  /* AX630C-PORT: drop this file's frame buffers (#45) */
	kfree(fp_priv);

	return -ERESTARTSYS;
}

/* VFS methods */
static const struct file_operations hantrovcmd_fops = {
	.owner = THIS_MODULE,
	.open = hantrovcmd_open,
	.release = hantrovcmd_release,
	.unlocked_ioctl = hantrovcmd_ioctl,
	.mmap = hantrovcmd_mmap,
	.fasync = NULL,
};

static u32 vcmd_release_AXIFE_IO(void)
{
#ifdef HANTROAXIFE_SUPPORT
	int i = 0, j = 0;

	for (i = 0; i < venc_vcmd_core_num; i++) {
		for (j = 0; j < 2; j++) {
			if (axife_hwregs[i][j]) {
				iounmap(axife_hwregs[i][j]);
				release_mem_region(
					vc8000e_vcmd_core_array[i].vcmd_base_addr +
						vc8000e_vcmd_core_array[i].submodule_axife_addr[j], AXIFE_SIZE);
				axife_hwregs[i][j] = NULL;
			}
		}
	}
#endif
	return 0;
}

static u32 ConfigAXIFE(u32 mode)
{
#ifdef HANTROAXIFE_SUPPORT
	u32 i = 0;
	u8 sub_module_type = 0;

	for (i = 0; i < venc_vcmd_core_num; i++) {
		sub_module_type = vc8000e_vcmd_core_array[i].sub_module_type;
		LOG_DBG("%s: sub_module_type is %d\n", __func__,
			sub_module_type);
		if (vc8000e_vcmd_core_array[i].submodule_axife_addr[0] != 0xffff) {
			if (!request_mem_region(
				    vc8000e_vcmd_core_array[i].vcmd_base_addr +
					    vc8000e_vcmd_core_array[i].submodule_axife_addr[0], AXIFE_SIZE, VENC_DRV_NAME)) {
				LOG_INFO("failed to request_mem_region for axife_hwregs[%d][0]\n", i);
				return -EBUSY;
			}

			axife_hwregs[i][0] = (volatile u8 *)ioremap(
				vc8000e_vcmd_core_array[i].vcmd_base_addr +
					vc8000e_vcmd_core_array[i].submodule_axife_addr[0], AXIFE_SIZE);
			if (axife_hwregs[i][0]) {
				LOG_DBG("%s: axife_hwregs[%d][0]=0x%px\n", __func__, i, axife_hwregs[i][0]);
				vc8000_AXIFEEnable(axife_hwregs[i][0], mode);
			}
		}
		if (vc8000e_vcmd_core_array[i].submodule_axife_addr[1] != 0xffff) {
			if (!request_mem_region(
				    vc8000e_vcmd_core_array[i].vcmd_base_addr +
					    vc8000e_vcmd_core_array[i].submodule_axife_addr[1], AXIFE_SIZE, VENC_DRV_NAME)) {
				LOG_INFO("failed to request_mem_region for axife_hwregs[%d][0]\n", i);
				return -EBUSY;
			}

			axife_hwregs[i][1] = (volatile u8 *)ioremap(
				vc8000e_vcmd_core_array[i].vcmd_base_addr +
					vc8000e_vcmd_core_array[i].submodule_axife_addr[1], AXIFE_SIZE);
			if (axife_hwregs[i][1]) {
				LOG_DBG("%s: axife_hwregs[%d][1]=0x%px\n", __func__,
				       i, axife_hwregs[i][1]);
				vc8000_AXIFEEnable(axife_hwregs[i][1], mode);
			}
		}
	}
#endif
	return 0;
}

static void vcmd_link_cmdbuf(struct hantrovcmd_dev *dev,
			     bi_list_node *last_linked_cmdbuf_node)
{
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *next_cmdbuf_node = NULL;
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	struct cmdbuf_obj *next_cmdbuf_obj = NULL;
	u32 *jmp_addr = NULL;
	u32 operation_code;

	new_cmdbuf_node = last_linked_cmdbuf_node;
	//for the first cmdbuf.
	if (new_cmdbuf_node) {
		cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		LOG_DBG("%s %d ", __func__, cmdbuf_obj->cmdbuf_id);
		if (cmdbuf_obj->cmdbuf_data_linked == 0) {
			dev->sw_cmdbuf_rdy_num++;
			cmdbuf_obj->cmdbuf_data_linked = 1;
			dev->duration_without_int = 0;
			if (cmdbuf_obj->has_end_cmdbuf == 0) {
				if (cmdbuf_obj->no_normal_int_cmdbuf == 1) {
					dev->duration_without_int = cmdbuf_obj->executing_time;
					//maybe nop is modified, so write back.
					if (dev->duration_without_int >= dev->duration_without_int_max) {
						jmp_addr =
							cmdbuf_obj->cmdbuf_virtualAddress + (cmdbuf_obj->cmdbuf_size / 4);
						operation_code = *(jmp_addr - 4);
						operation_code = JMP_IE_1 | operation_code;
						*(jmp_addr - 4) = operation_code;
						dev->duration_without_int = 0;
					}
				}
			}
		}
	}
	while (1) {
		if (!new_cmdbuf_node)
			break;
		if (!new_cmdbuf_node->next)
			break;
		next_cmdbuf_node = new_cmdbuf_node->next;
		cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		next_cmdbuf_obj = (struct cmdbuf_obj *)next_cmdbuf_node->data;
		if (cmdbuf_obj->has_end_cmdbuf == 0 &&
		    !next_cmdbuf_obj->cmdbuf_run_done) {
			//need to link, current cmdbuf link to next cmdbuf
			LOG_DBG("Link cmdbuf %d to cmdbuf %d",
			       cmdbuf_obj->cmdbuf_id, next_cmdbuf_obj->cmdbuf_id);
			jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress + (cmdbuf_obj->cmdbuf_size / 4);
			if (dev->hw_version_id > HW_ID_1_0_C) {
				//set next cmdbuf id
				*(jmp_addr - 1) = next_cmdbuf_obj->cmdbuf_id;
			}
			if (mmu_enable) {
				if (sizeof(size_t) == 8) {
					*(jmp_addr - 2) = (u32)(
						(u64)(next_cmdbuf_obj->mmu_cmdbuf_busAddress) >> 32);
				} else {
					*(jmp_addr - 2) = 0;
				}
				*(jmp_addr - 3) = (u32)(
					next_cmdbuf_obj->mmu_cmdbuf_busAddress);
				LOG_DBG(KERN_INFO
				       "%s: next_cmdbuf_obj->mmu_cmdbuf_busAddress=0x%zx\n",
				       __func__, next_cmdbuf_obj->mmu_cmdbuf_busAddress);
			} else {
				if (sizeof(size_t) == 8) {
					*(jmp_addr - 2) = (u32)(
						(u64)(next_cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr) >> 32);
				} else {
					*(jmp_addr - 2) = 0;
				}
				*(jmp_addr - 3) = (u32)(
					next_cmdbuf_obj->cmdbuf_busAddress -
					base_ddr_addr);
			}
			operation_code = *(jmp_addr - 4);
			operation_code >>= 16;
			operation_code <<= 16;
			*(jmp_addr - 4) =
				(u32)(operation_code | JMP_RDY_1 | ((next_cmdbuf_obj->cmdbuf_size + 7) / 8));
			next_cmdbuf_obj->cmdbuf_data_linked = 1;
			dev->sw_cmdbuf_rdy_num++;
			//modify nop code of next cmdbuf
			if (next_cmdbuf_obj->has_end_cmdbuf == 0) {
				if (next_cmdbuf_obj->no_normal_int_cmdbuf == 1) {
					dev->duration_without_int += next_cmdbuf_obj->executing_time;

					//maybe we see the modified nop before abort, so need to write back.
					if (dev->duration_without_int >=
					    dev->duration_without_int_max) {
						jmp_addr =
							next_cmdbuf_obj->cmdbuf_virtualAddress + (next_cmdbuf_obj->cmdbuf_size / 4);
						operation_code = *(jmp_addr - 4);
						operation_code = JMP_IE_1 |
								 operation_code;
						*(jmp_addr - 4) = operation_code;
						dev->duration_without_int = 0;
					}
				}
			} else {
				dev->duration_without_int = 0;
			}
#ifdef VCMD_DEBUG_INTERNAL
			{
				u32 i;

				LOG_INFO("vcmd link, last cmdbuf content\n");
				for (i = cmdbuf_obj->cmdbuf_size / 4 - 8;
				     i < cmdbuf_obj->cmdbuf_size / 4; i++) {
					LOG_INFO("current linked cmdbuf data %d =0x%x\n",
						i, *(cmdbuf_obj->cmdbuf_virtualAddress + i));
				}
			}
#endif
		}
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
}

/* delink all the cmd buffers from the cmdbuf in front of last_linked_cmdbuf_node
 *  to head of the list. All the cmd bufs marked as X will be delinked.
 *  E.g.,
 *  X->X->...->X->L->L->        ...        ->L
 *  ^             ^                          ^
 *  head          last_linked_cmdbuf_node    tail
 */

static void vcmd_delink_cmdbuf(struct hantrovcmd_dev *dev,
			       bi_list_node *last_linked_cmdbuf_node)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct cmdbuf_obj *cmdbuf_obj = NULL;

	new_cmdbuf_node = last_linked_cmdbuf_node;
	while (1) {
		if (!new_cmdbuf_node)
			break;
		cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
		if (cmdbuf_obj->cmdbuf_data_linked)
			cmdbuf_obj->cmdbuf_data_linked = 0;
		else
			break;
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
	dev->sw_cmdbuf_rdy_num = 0;
}

static void ConfigAIIXFE_MMU_BYVCMD(struct hantrovcmd_dev **device)
{
#ifdef HANTROVCMD_ENABLE_IP_SUPPORT
	u32 i = 0;
	u32 mirror_index, register_index, register_value;
	u32 write_command = 0;
	struct hantrovcmd_dev *dev = *device;

	if (!device)
		return;

	mirror_index = VCMD_REGISTER_INDEX_SW_INIT_CMD0;
	write_command = OPCODE_WREG | (1 << 26) | (1 << 16);
#ifdef HANTROAXIFE_SUPPORT
	//enable AXIFE by VCMD
	for (i = 0; i < 2; i++) {
		if (dev->vcmd_core_cfg.submodule_axife_addr[i] != 0xffff) {
			register_index = AXI_REG10_SW_FRONTEND_EN;
			register_value = 0x02;
			dev->reg_mirror[mirror_index++] =
				write_command | (dev->vcmd_core_cfg.submodule_axife_addr[i] + register_index);
			dev->reg_mirror[mirror_index++] = register_value;

			register_index = AXI_REG11_SW_WORK_MODE;
			register_value = 0x00;
			dev->reg_mirror[mirror_index++] =
				write_command | (dev->vcmd_core_cfg.submodule_axife_addr[i] + register_index);
			dev->reg_mirror[mirror_index++] = register_value;
		}
	}
#endif
	//END command
	dev->reg_mirror[mirror_index++] = OPCODE_END;
	dev->reg_mirror[mirror_index] = 0x00;

	for (i = 0; i < mirror_index - VCMD_REGISTER_INDEX_SW_INIT_CMD0; i++) {
		register_index = (i + VCMD_REGISTER_INDEX_SW_INIT_CMD0) * 4;
		vcmd_write_reg((const void *)dev->hwregs, register_index,
			       dev->reg_mirror[i + VCMD_REGISTER_INDEX_SW_INIT_CMD0]);
	}
#endif
}

static void vcmd_start(struct hantrovcmd_dev *dev,
		       bi_list_node *first_linked_cmdbuf_node)
{
	struct cmdbuf_obj *cmdbuf_obj = NULL;

	if (dev->working_state == WORKING_STATE_IDLE) {
		if ((first_linked_cmdbuf_node) && dev->sw_cmdbuf_rdy_num) {
			cmdbuf_obj = (struct cmdbuf_obj *)
					     first_linked_cmdbuf_node->data;
#ifdef VCMD_DEBUG_INTERNAL
			printk_vcmd_register_debug((const void *)dev->hwregs,
						   "vcmd_start enters");
#endif
			//0x40
#ifdef HANTROVCMD_ENABLE_IP_SUPPORT
			if (dev->hw_version_id >= HW_ID_1_2_1)
				vcmd_set_register_mirror_value(
					dev->reg_mirror, HWIF_VCMD_INIT_MODE,
					1); //when start vcmd, first vcmd is init mode
#endif
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_AXI_CLK_GATE_DISABLE, 0);
			//this bit should be set 1 only when need to reset dec400
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_MASTER_OUT_CLK_GATE_DISABLE, 1);
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_CORE_CLK_GATE_DISABLE, 0);
			vcmd_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_ABORT_MODE, 0);
			vcmd_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_RESET_CORE, 0);
			vcmd_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_RESET_ALL, 0);
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_START_TRIGGER, 0);
			//0x48
			if (dev->hw_version_id <= HW_ID_1_0_C)
				vcmd_set_register_mirror_value(
					dev->reg_mirror, HWIF_VCMD_IRQ_INTCMD_EN, 0xffff);
			else {
				vcmd_set_register_mirror_value(
					dev->reg_mirror, HWIF_VCMD_IRQ_JMPP_EN, 1);
				vcmd_set_register_mirror_value(
					dev->reg_mirror, HWIF_VCMD_IRQ_JMPD_EN, 1);
			}

			if (dev->hw_version_id < HW_ID_1_1_1)
				vcmd_set_register_mirror_value(
					dev->reg_mirror, HWIF_VCMD_IRQ_RESET_EN, 1);

			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_IRQ_ABORT_EN, 1);
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_IRQ_CMDERR_EN, 1);
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_IRQ_TIMEOUT_EN, 1);
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_IRQ_BUSERR_EN, 1);
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_IRQ_ENDCMD_EN, 1);
			//0x4c
			vcmd_set_register_mirror_value(dev->reg_mirror,
						       HWIF_VCMD_TIMEOUT_EN, 1);
			vcmd_set_register_mirror_value(dev->reg_mirror,
						       HWIF_VCMD_TIMEOUT_CYCLES, 0x1dcd6500);
			if (mmu_enable) {
				vcmd_set_register_mirror_value(
					dev->reg_mirror,
					HWIF_VCMD_EXECUTING_CMD_ADDR,
					(u32)(cmdbuf_obj->mmu_cmdbuf_busAddress));
				LOG_INFO("cmdbuf_obj->mmu_cmdbuf_busAddress=0x%08x\n",
					(u32)cmdbuf_obj->mmu_cmdbuf_busAddress);
				if (sizeof(size_t) == 8) {
					vcmd_set_register_mirror_value(
						dev->reg_mirror,
						HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
						(u32)((u64)(cmdbuf_obj->mmu_cmdbuf_busAddress) >> 32));
				} else {
					vcmd_set_register_mirror_value(dev->reg_mirror,
								       HWIF_VCMD_EXECUTING_CMD_ADDR_MSB, 0);
				}
			} else {
				vcmd_set_register_mirror_value(
					dev->reg_mirror,
					HWIF_VCMD_EXECUTING_CMD_ADDR,
					(u32)(cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr));
				if (sizeof(size_t) == 8) {
					vcmd_set_register_mirror_value(
						dev->reg_mirror,
						HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
						(u32)((u64)(cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr) >> 32));
				} else {
					vcmd_set_register_mirror_value(dev->reg_mirror,
								       HWIF_VCMD_EXECUTING_CMD_ADDR_MSB, 0);
				}
			}
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_EXE_CMDBUF_LENGTH,
				(u32)((cmdbuf_obj->cmdbuf_size + 7) / 8));
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT,
				dev->sw_cmdbuf_rdy_num);
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_MAX_BURST_LEN, 0x10);
			if (dev->hw_version_id > HW_ID_1_0_C) {
				vcmd_write_register_value((const void *)dev->hwregs,
							  dev->reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID,
					(u32)cmdbuf_obj->cmdbuf_id);
			}

			vcmd_write_reg((const void *)dev->hwregs, 0x44,
				       vcmd_read_reg((const void *)dev->hwregs, 0x44));
			vcmd_write_reg((const void *)dev->hwregs, 0x40,
				       dev->reg_mirror[0x40 / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x48,
				       dev->reg_mirror[0x48 / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x4c,
				       dev->reg_mirror[0x4c / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x50,
				       dev->reg_mirror[0x50 / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x54,
				       dev->reg_mirror[0x54 / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x58,
				       dev->reg_mirror[0x58 / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x5c,
				       dev->reg_mirror[0x5c / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x60,
				       dev->reg_mirror[0x60 / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x64, 0xffffffff); //not interrupt cpu

			//init HWIF_VCMD_EXE_CMDBUF_COUNT
			if (!vcmd_get_register_value((const void *)dev->hwregs,
					dev->reg_mirror,
					HWIF_VCMD_START_TRIGGER))
				vcmd_write_register_value((const void *)dev->hwregs,
					dev->reg_mirror,
					HWIF_VCMD_EXE_CMDBUF_COUNT, 0);
			dev->working_state = WORKING_STATE_WORKING;

			if (dev->hw_version_id >= HW_ID_1_2_1)
				ConfigAIIXFE_MMU_BYVCMD(&dev);

			//start
			vcmd_set_register_mirror_value(dev->reg_mirror,
						       HWIF_VCMD_MASTER_OUT_CLK_GATE_DISABLE, 0); //this bit should be set 1 only when need to reset dec400
			vcmd_set_register_mirror_value(
				dev->reg_mirror, HWIF_VCMD_START_TRIGGER, 1);
			LOG_DBG("To write vcmd register 16:0x%x\n",
			       dev->reg_mirror[0x40 / 4]);
			vcmd_write_reg((const void *)dev->hwregs, 0x40,
				       dev->reg_mirror[0x40 / 4]);
			LOG_INFO("vcmd start completed, core_id = %u, cmdbuf_id = %u\n", dev->core_id, cmdbuf_obj->cmdbuf_id);
#ifdef SUPPORT_WATCHDOG
			_vcmd_watchdog_feed(dev);
#endif
#ifdef VCMD_DEBUG_INTERNAL
      printk_vcmd_register_debug(dev->hwregs, "vcmd_start exits ");
#endif
    }
#ifdef SUPPORT_WATCHDOG
	else {
		LOG_INFO("no linked cmdbuf, first_linked_cmdbuf_node = %u, sw_cmdbuf_rdy_num = %u, core_id = %u\n"
			, first_linked_cmdbuf_node!=NULL, dev->sw_cmdbuf_rdy_num, dev->core_id);
		_vcmd_watchdog_stop(dev);
	}
#endif
  }

}

static void create_read_all_registers_cmdbuf(struct exchange_parameter *input_para)
{
	u32 register_range[] = {
				VCMD_ENCODER_REGISTER_SIZE,
				 VCMD_IM_REGISTER_SIZE,
				 VCMD_DECODER_REGISTER_SIZE,
				 VCMD_JPEG_ENCODER_REGISTER_SIZE,
				 VCMD_JPEG_DECODER_REGISTER_SIZE };
	u32 counter_cmdbuf_size = 0;
	u32 *set_base_addr = vcmd_buf_mem_pool.virtualAddress +
			     input_para->cmdbuf_id * CMDBUF_MAX_SIZE / 4;
	//u32 *status_base_virt_addr=vcmd_status_buf_mem_pool.virtualAddress + input_para->cmdbuf_id*CMDBUF_MAX_SIZE/4+(vcmd_manager[input_para->module_type][0]->vcmd_core_cfg.submodule_main_addr/2/4+0);
	ptr_t status_base_phy_addr =
		vcmd_status_buf_mem_pool.busAddress +
		input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
		(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_main_addr / 2 + 0);
	u32 map_status_base_phy_addr =
		vcmd_status_buf_mem_pool.mmu_bus_address + input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
		(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_main_addr / 2 + 0);
	u32 offset_inc = 0;
	u32 offset_inc_dec400 = 0;

	if (vcmd_manager[input_para->module_type][input_para->core_id]->hw_version_id >
	    HW_ID_1_0_C) {
		LOG_DBG("vcmd_driver:create cmdbuf data when hw_version_id = 0x%x\n",
			vcmd_manager[input_para->module_type][input_para->core_id]->hw_version_id);

		//read vcmd executing cmdbuf id registers to ddr for balancing core load.
		*(set_base_addr + 0) = (OPCODE_RREG) | (1 << 16) | (EXECUTING_CMDBUF_ID_ADDR * 4);
		counter_cmdbuf_size += 4;
		*(set_base_addr + 1) = (u32)0; //will be changed in link stage
		counter_cmdbuf_size += 4;
		*(set_base_addr + 2) = (u32)0; //will be changed in link stage
		counter_cmdbuf_size += 4;
		//alignment
		*(set_base_addr + 3) = 0;
		counter_cmdbuf_size += 4;

		//read main IP all registers
		*(set_base_addr + 4) =
			(OPCODE_RREG) | ((register_range[input_para->module_type] / 4) << 16) |
			(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_main_addr + 0);
		counter_cmdbuf_size += 4;
		if (mmu_enable) {
			*(set_base_addr + 5) = map_status_base_phy_addr;
		} else {
			*(set_base_addr + 5) = (u32)(status_base_phy_addr - base_ddr_addr);
		}
		counter_cmdbuf_size += 4;
		if (mmu_enable) {
			*(set_base_addr + 6) = 0;
		} else {
			if (sizeof(size_t) == 8)
				*(set_base_addr + 6) = (u32)((u64)(status_base_phy_addr - base_ddr_addr) >> 32);
			else
				*(set_base_addr + 6) = 0;
		}
		counter_cmdbuf_size += 4;
		//alignment
		*(set_base_addr + 7) = 0;
		counter_cmdbuf_size += 4;

		if (vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_L2Cache_addr != 0xffff) {
			//read L2 cache register
			offset_inc = 4;
			status_base_phy_addr =
				vcmd_status_buf_mem_pool.busAddress +
				input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
				(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_L2Cache_addr / 2 + 0);
			map_status_base_phy_addr =
				vcmd_status_buf_mem_pool.mmu_bus_address + input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
				(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_L2Cache_addr / 2 + 0);
			//read L2cache IP first register
			*(set_base_addr +
			  8) = (OPCODE_RREG) | (1 << 16) |
			       (vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_L2Cache_addr + 0);
			counter_cmdbuf_size += 4;
			if (mmu_enable)
				*(set_base_addr + 9) = map_status_base_phy_addr;
			else
				*(set_base_addr + 9) = (u32)(status_base_phy_addr - base_ddr_addr);
			counter_cmdbuf_size += 4;
			if (mmu_enable) {
				*(set_base_addr + 10) = 0;
			} else {
				if (sizeof(size_t) == 8) {
					*(set_base_addr + 10) = (u32)(
						(u64)(status_base_phy_addr - base_ddr_addr) >> 32);
				} else {
					*(set_base_addr + 10) = 0;
				}
			}
			counter_cmdbuf_size += 4;
			//alignment
			*(set_base_addr + 11) = 0;
			counter_cmdbuf_size += 4;
		}
		if (vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_dec400_addr != 0xffff) {
			//read dec400 register
			offset_inc_dec400 = 4;
			status_base_phy_addr =
				vcmd_status_buf_mem_pool.busAddress +
				input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
				(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_dec400_addr / 2 + 0);
			map_status_base_phy_addr =
				vcmd_status_buf_mem_pool.mmu_bus_address + input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
				(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_dec400_addr / 2 + 0);
			//read DEC400 IP first register
			*(set_base_addr + 8 + offset_inc) =
				(OPCODE_RREG) | (0x2b << 16) |
				(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_dec400_addr + 0);
			counter_cmdbuf_size += 4;
			if (mmu_enable)
				*(set_base_addr + 9 + offset_inc) = map_status_base_phy_addr;
			else
				*(set_base_addr + 9 + offset_inc) = (u32)(status_base_phy_addr - base_ddr_addr);
			counter_cmdbuf_size += 4;
			if (mmu_enable) {
				*(set_base_addr + 10 + offset_inc) = 0;
			} else {
				if (sizeof(size_t) == 8) {
					*(set_base_addr + 10 + offset_inc) =
						(u32)((u64)(status_base_phy_addr - base_ddr_addr) >> 32);
				} else {
					*(set_base_addr + 10 + offset_inc) = 0;
				}
			}
			counter_cmdbuf_size += 4;
			//alignment
			*(set_base_addr + 11 + offset_inc) = 0;
			counter_cmdbuf_size += 4;
		}

#if 0
	//INT code, interrupt immediately
	*(set_base_addr + 4) = (OPCODE_INT) | 0 | input_para->cmdbuf_id;
	counter_cmdbuf_size += 4;
	//alignment
	*(set_base_addr + 5) = 0;
	counter_cmdbuf_size += 4;
#endif
		//read vcmd registers to ddr
		*(set_base_addr + 8 + offset_inc + offset_inc_dec400) =
			(OPCODE_RREG) | (27 << 16) | (0);
		counter_cmdbuf_size += 4;
		*(set_base_addr + 9 + offset_inc + offset_inc_dec400) =
			(u32)0; //will be changed in link stage
		counter_cmdbuf_size += 4;
		*(set_base_addr + 10 + offset_inc + offset_inc_dec400) =
			(u32)0; //will be changed in link stage
		counter_cmdbuf_size += 4;
		//alignment
		*(set_base_addr + 11 + offset_inc + offset_inc_dec400) = 0;
		counter_cmdbuf_size += 4;
		//JMP RDY = 0
		*(set_base_addr + 12 + offset_inc + offset_inc_dec400) =
			(OPCODE_JMP_RDY0) | 0 | JMP_IE_1 | 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 13 + offset_inc + offset_inc_dec400) = 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 14 + offset_inc + offset_inc_dec400) = 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 15 + offset_inc + offset_inc_dec400) =
			input_para->cmdbuf_id;
		//don't add the last alignment DWORD in order to  identify END command or JMP command.
		//counter_cmdbuf_size += 4;
		input_para->cmdbuf_size =
			(16 + offset_inc + offset_inc_dec400) * 4;
	} else {
		LOG_DBG("vcmd_driver:create cmdbuf data when hw_version_id = 0x%x\n",
			vcmd_manager[input_para->module_type][input_para->core_id]->hw_version_id);
		//read all registers
		*(set_base_addr + 0) =
			(OPCODE_RREG) |
			((register_range[input_para->module_type] / 4) << 16) |
			(vcmd_manager[input_para->module_type][input_para->core_id]->vcmd_core_cfg.submodule_main_addr + 0);
		counter_cmdbuf_size += 4;
		if (mmu_enable)
			*(set_base_addr + 1) = map_status_base_phy_addr;
		else
			*(set_base_addr + 1) = (u32)(status_base_phy_addr - base_ddr_addr);
		counter_cmdbuf_size += 4;
		if (mmu_enable) {
			*(set_base_addr + 2) = 0;
		} else {
			if (sizeof(size_t) == 8) {
				*(set_base_addr + 2) =
					(u32)((u64)(status_base_phy_addr - base_ddr_addr) >> 32);
			} else {
				*(set_base_addr + 2) = 0;
			}
		}
		counter_cmdbuf_size += 4;
		//alignment
		*(set_base_addr + 3) = 0;
		counter_cmdbuf_size += 4;
#if 0
	//INT code, interrupt immediately
	*(set_base_addr + 4) = (OPCODE_INT) | 0 | input_para->cmdbuf_id;
	counter_cmdbuf_size += 4;
	//alignment
	*(set_base_addr + 5) = 0;
	counter_cmdbuf_size += 4;
#endif
		//JMP RDY = 0
		*(set_base_addr + 4) = (OPCODE_JMP_RDY0) | 0 | JMP_IE_1 | 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 5) = 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 6) = 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 7) = input_para->cmdbuf_id;
		//don't add the last alignment DWORD in order to  identify END command or JMP command.
		//counter_cmdbuf_size += 4;
		input_para->cmdbuf_size = 8 * 4;
	}
}

static void read_main_module_all_registers(u32 main_module_type)
{
	int ret, i;
	struct exchange_parameter input_para;
	u32 irq_status_ret = 0;
	u32 *status_base_virt_addr;

	for (i = 0; i < vcmd_type_core_num[main_module_type]; i++) {
		input_para.executing_time = 0;
		input_para.priority = CMDBUF_PRIORITY_NORMAL;
		input_para.module_type = main_module_type;
		input_para.cmdbuf_size = 0;
		input_para.core_id = i;
		ret = reserve_cmdbuf(NULL, &input_para);
		vcmd_manager[main_module_type][i]->status_cmdbuf_id =
			input_para.cmdbuf_id;
		create_read_all_registers_cmdbuf(&input_para);
		link_and_run_cmdbuf(NULL, &input_para);
		// msleep(1000);
		// hantrovcmd_isr(input_para.core_id,
		// 	       &hantrovcmd_data[input_para.core_id]);
		wait_cmdbuf_ready(NULL, input_para.cmdbuf_id, &irq_status_ret);
		status_base_virt_addr =
			vcmd_status_buf_mem_pool.virtualAddress + input_para.cmdbuf_id * CMDBUF_MAX_SIZE / 4 +
			(vcmd_manager[input_para.module_type][i]->vcmd_core_cfg.submodule_main_addr / 2 / 4 + 0);
		LOG_DBG("vcmd_driver: main module register 0:0x%x\n", *status_base_virt_addr);
		LOG_DBG("vcmd_driver: main module register 80:0x%x\n", *(status_base_virt_addr + 80));
		LOG_DBG("vcmd_driver: main module register 214:0x%x\n", *(status_base_virt_addr + 214));
		LOG_DBG("vcmd_driver: main module register 226:0x%x\n", *(status_base_virt_addr + 226));
		LOG_DBG("vcmd_driver: main module register 287:0x%x\n", *(status_base_virt_addr + 287));
		LOG_DBG("vcmd_driver: status_cmdbuf_id:%u\n", input_para.cmdbuf_id);
#ifdef DUMP_HARDWARE_FEATURES
		{
			VCE_REGISTER reg;
			reg.regval = status_base_virt_addr[4];
			LOG_DBG("min_cb=%d, max_cb=%d\n", reg.swreg4.sw_enc_min_cb_size, reg.swreg4.sw_enc_max_cb_size);
			LOG_DBG("min_trb=%d, max_trb=%d\n", reg.swreg4.sw_enc_min_trb_size, reg.swreg4.sw_enc_max_trb_size);

			reg.regval = status_base_virt_addr[80];
			LOG_DBG("H264Support=%d\n", reg.swreg80.sw_enc_HWH264Support);
			LOG_DBG("ScalingSupport=%d\n", reg.swreg80.sw_enc_HWScalingSupport);
			LOG_DBG("BFrameSupport=%d\n", reg.swreg80.sw_enc_HWBFrameSupport);
			LOG_DBG("RgbSupport=%d\n", reg.swreg80.sw_enc_HWRgbSupport);
			LOG_DBG("HevcSupport=%d\n", reg.swreg80.sw_enc_HWHevcSupport);
			LOG_DBG("Vp9Support=%d\n", reg.swreg80.sw_enc_HWVp9Support);
			LOG_DBG("DenoiseSupport=%d\n", reg.swreg80.sw_enc_HWDenoiseSupport);
			LOG_DBG("Main10Support=%d\n", reg.swreg80.sw_enc_HWMain10Support);
			LOG_DBG("Bus=%d\n", reg.swreg80.sw_enc_HWBus);
			LOG_DBG("CavlcSupport=%d\n", reg.swreg80.sw_enc_HWCavlcSupport);
			LOG_DBG("LineBufSupport=%d\n", reg.swreg80.sw_enc_HWLineBufSupport);
			LOG_DBG("ProgRdoSupport=%d\n", reg.swreg80.sw_enc_HWProgRdoSupport);
			LOG_DBG("RFCSupport=%d\n", reg.swreg80.sw_enc_HWRFCSupport);
			LOG_DBG("TU32Support=%d\n", reg.swreg80.sw_enc_HWTU32Support);
			LOG_DBG("JPEGSupport=%d\n", reg.swreg80.sw_enc_HWJPEGSupport);
			LOG_DBG("BusWidth=%d\n", reg.swreg80.sw_enc_HWBusWidth);
			LOG_DBG("MaxVideoWidth=%d\n", reg.swreg80.sw_enc_HWMaxVideoWidth);

			reg.regval = status_base_virt_addr[214];
			LOG_DBG("LJPEGSupport=%d\n", reg.swreg214.sw_enc_HWLJPEGSupport);
			LOG_DBG("AbsQpSupport=%d\n", reg.swreg214.sw_enc_HWAbsQpSupport);
			LOG_DBG("IntraTU32Support=%d\n", reg.swreg214.sw_enc_HWIntraTU32Support);
			LOG_DBG("RoiMapVersion=%d\n", reg.swreg214.sw_enc_HWRoiMapVersion);
			LOG_DBG("MaxVideoWithH264=%d\n", reg.swreg214.sw_enc_HWMaxVideoWithH264);
			LOG_DBG("MaxVideoWithJPEG=%d\n", reg.swreg214.sw_enc_HWMaxVideoWithJPEG);

			reg.regval = status_base_virt_addr[226];
			LOG_DBG("ROI8Support=%d\n", reg.swreg226.sw_enc_HWROI8Support);
			LOG_DBG("GmvSupport=%d\n", reg.swreg226.sw_enc_HWGmvSupport);
			LOG_DBG("Jpeg422Support=%d\n", reg.swreg226.sw_enc_HWJpeg422Support);
			LOG_DBG("CtbRcVersion=%d\n", reg.swreg226.sw_enc_HWCtbRcVersion);
			LOG_DBG("me_vert_searchrange_h264=%d\n", reg.swreg226.sw_enc_me_vert_searchrange_h264);
			LOG_DBG("me_vert_searchrange_hevc=%d\n", reg.swreg226.sw_enc_me_vert_searchrange_hevc);
			LOG_DBG("CuInforVersion=%d\n", reg.swreg226.sw_enc_HWCuInforVersion);
			LOG_DBG("P010RefSupport=%d\n", reg.swreg226.sw_enc_HWP010RefSupport);
			LOG_DBG("SSIMSupport=%d\n", reg.swreg226.sw_enc_HWSSIMSupport);
		}
#endif
	}
	//don't release cmdbuf because it can be used repeatedly
	//release_cmdbuf(input_para.cmdbuf_id);
}

int vcmd_mem_init(void)
{
	dma_addr_t dma_handle = 0;
	dma_addr_t dma_handle_d1 = 0;


	vcmd_buf_mem_pool.size = CMDBUF_POOL_TOTAL_SIZE;

	/* command buffer */
	vcmd_buf_mem_pool.virtualAddress = (u32 *)dma_alloc_attrs(&venc_pdev->dev,
			vcmd_buf_mem_pool.size, &dma_handle,
			GFP_KERNEL, DMA_ATTR_FORCE_CONTIGUOUS);

	/* AX630C-PORT: fail-safe. On a kernel without CONFIG_CMA a 2MB
	 * DMA_ATTR_FORCE_CONTIGUOUS coherent alloc can return NULL; the original
	 * code never checks and then dereferences NULL -> kernel panic. Bail. */
	if (!vcmd_buf_mem_pool.virtualAddress) {
		LOG_ERR("AX630C-PORT: vcmd_buf_mem_pool dma_alloc_attrs(%zu) FAILED (no CMA?)\n",
				(size_t)vcmd_buf_mem_pool.size);
		return -ENOMEM;
	}
	vcmd_buf_mem_pool.busAddress = (unsigned long long)dma_handle;
	vcmd_buf_mem_pool.phy_address = dma_handle;	/* AX630C-PORT (#63): carveout virt is a section-mapped memremap -- vmalloc_to_pfn() WARNs (pmd_bad) and returns garbage; bus == phys on AX630C, no IOMMU */

	if (venc_pdev_d1) {
		dma_handle_d1 = dma_map_page(&venc_pdev_d1->dev,
				vmalloc_to_page(vcmd_buf_mem_pool.virtualAddress), 0, vcmd_buf_mem_pool.size, DMA_BIDIRECTIONAL);
		if (dma_handle != dma_handle_d1) {
			LOG_ERR("venc_vcmd: dma address of vcmd buf not the same between d0 and d1\n");
			return -1;
		}
	}

	LOG_INFO("Init: vcmd_buf_mem_pool.busAddress=0x%llx.\n",
			(unsigned long long)vcmd_buf_mem_pool.busAddress);
	LOG_INFO("Init: vcmd_buf_mem_pool.phy_address=0x%llx.\n",
			(unsigned long long)vcmd_buf_mem_pool.phy_address);
	LOG_INFO("Init: vcmd_buf_mem_pool.virtualAddress=0x%llx.\n",
			(unsigned long long)vcmd_buf_mem_pool.virtualAddress);

	/* status buffer */
	vcmd_status_buf_mem_pool.size = CMDBUF_POOL_TOTAL_SIZE;
	vcmd_status_buf_mem_pool.virtualAddress = (u32 *)dma_alloc_attrs(&venc_pdev->dev,
			vcmd_status_buf_mem_pool.size, &dma_handle,
			GFP_KERNEL, DMA_ATTR_FORCE_CONTIGUOUS);

	if (!vcmd_status_buf_mem_pool.virtualAddress) {	/* AX630C-PORT: fail-safe */
		LOG_ERR("AX630C-PORT: vcmd_status_buf_mem_pool dma_alloc_attrs FAILED\n");
		return -ENOMEM;
	}
	vcmd_status_buf_mem_pool.busAddress = (unsigned long long)dma_handle;
	vcmd_status_buf_mem_pool.phy_address = dma_handle;	/* AX630C-PORT (#63): carveout virt is a section-mapped memremap -- vmalloc_to_pfn() WARNs (pmd_bad) and returns garbage; bus == phys on AX630C, no IOMMU */

	if (venc_pdev_d1) {
		dma_handle_d1 = dma_map_page(&venc_pdev_d1->dev,
				vmalloc_to_page(vcmd_status_buf_mem_pool.virtualAddress), 0, vcmd_status_buf_mem_pool.size, DMA_BIDIRECTIONAL);
		if (dma_handle != dma_handle_d1) {
			LOG_ERR("venc_vcmd: dma address of status buf not the same between d0 and d1\n");
			return -1;
		}
	}

	LOG_INFO("Init: vcmd_status_buf_mem_pool.busAddress=0x%llx.\n",
			(unsigned long long)vcmd_status_buf_mem_pool.busAddress);
	LOG_INFO("Init: vcmd_status_buf_mem_pool.phy_address=0x%llx.\n",
			(unsigned long long)vcmd_status_buf_mem_pool.phy_address);
	LOG_INFO("Init: vcmd_status_buf_mem_pool.virtualAddress=0x%llx.\n",
			(unsigned long long)vcmd_status_buf_mem_pool.virtualAddress);

	/* register buffer */
	vcmd_registers_mem_pool.size = CMDBUF_POOL_TOTAL_SIZE;
	vcmd_registers_mem_pool.virtualAddress = (u32 *)dma_alloc_attrs(&venc_pdev->dev,
			vcmd_registers_mem_pool.size, &dma_handle,
			GFP_KERNEL, DMA_ATTR_FORCE_CONTIGUOUS);

	if (!vcmd_registers_mem_pool.virtualAddress) {	/* AX630C-PORT: fail-safe */
		LOG_ERR("AX630C-PORT: vcmd_registers_mem_pool dma_alloc_attrs FAILED\n");
		return -ENOMEM;
	}
	vcmd_registers_mem_pool.busAddress = (unsigned long long)dma_handle;
	vcmd_registers_mem_pool.phy_address = dma_handle;	/* AX630C-PORT (#63): carveout virt is a section-mapped memremap -- vmalloc_to_pfn() WARNs (pmd_bad) and returns garbage; bus == phys on AX630C, no IOMMU */

	if (venc_pdev_d1) {
		dma_handle_d1 = dma_map_page(&venc_pdev_d1->dev,
				vmalloc_to_page(vcmd_registers_mem_pool.virtualAddress), 0, vcmd_registers_mem_pool.size, DMA_BIDIRECTIONAL);
		if (dma_handle != dma_handle_d1) {
			LOG_ERR("vdec_vcmd: dma address of registers buf not the same between d0 and d1\n");
			return -1;
		}
	}

	LOG_INFO("Init: vcmd_registers_mem_pool.busAddress=0x%llx.\n",
			(unsigned long long)vcmd_registers_mem_pool.busAddress);
	LOG_INFO("Init: vcmd_registers_mem_pool.phy_address=0x%llx.\n",
			(unsigned long long)vcmd_registers_mem_pool.phy_address);
	LOG_INFO("Init: vcmd_registers_mem_pool.virtualAddress=0x%llx.\n",
			(unsigned long long)vcmd_registers_mem_pool.virtualAddress);

	return 0;
}

void vcmd_mem_cleanup(void)
{
	if (vcmd_buf_mem_pool.virtualAddress)
		dma_free_attrs(
				&venc_pdev->dev, vcmd_buf_mem_pool.size,
				vcmd_buf_mem_pool.virtualAddress,
				(dma_addr_t)vcmd_buf_mem_pool.busAddress,
				0);
	if (vcmd_status_buf_mem_pool.virtualAddress)
		dma_free_attrs(
				&venc_pdev->dev, vcmd_status_buf_mem_pool.size,
				vcmd_status_buf_mem_pool.virtualAddress,
				(dma_addr_t)vcmd_status_buf_mem_pool.busAddress,
				0);
	if (vcmd_registers_mem_pool.virtualAddress)
		dma_free_attrs(
				&venc_pdev->dev, vcmd_registers_mem_pool.size,
				vcmd_registers_mem_pool.virtualAddress,
				(dma_addr_t)vcmd_registers_mem_pool.busAddress,
				0);

	if (venc_pdev_d1) {
		dma_unmap_page(&venc_pdev_d1->dev,
				(dma_addr_t)vcmd_buf_mem_pool.busAddress, vcmd_buf_mem_pool.size, DMA_BIDIRECTIONAL);
		dma_unmap_page(&venc_pdev_d1->dev,
				(dma_addr_t)vcmd_status_buf_mem_pool.busAddress, vcmd_status_buf_mem_pool.size, DMA_BIDIRECTIONAL);
		dma_unmap_page(&venc_pdev_d1->dev,
				(dma_addr_t)vcmd_registers_mem_pool.busAddress, vcmd_registers_mem_pool.size, DMA_BIDIRECTIONAL);
	}
}

int hantroenc_vcmd_init(void)
{
	int i, k;
	int result;

	// pr_info("[%s]build version: %s\n", VENC_DEV_NAME, ES_VENC_GIT_VER);
	for (i = 0; i < venc_vcmd_core_num; i++) {
		LOG_INFO("module init - vcore[%d] addr =0x%llx, freq =%u\n", i,
			(unsigned long long)vc8000e_vcmd_core_array[i].vcmd_base_addr,
			vc8000e_vcmd_core_array[i].freq);
	}
	hantrovcmd_data = vmalloc(sizeof(struct hantrovcmd_dev) * venc_vcmd_core_num);
	if (!hantrovcmd_data)
		goto err1;
	memset(hantrovcmd_data, 0, sizeof(struct hantrovcmd_dev) * venc_vcmd_core_num);
	for (k = 0; k < MAX_VCMD_TYPE; k++) {
		vcmd_type_core_num[k] = 0;
		vcmd_position[k] = 0;
		for (i = 0; i < MAX_VCMD_NUMBER; i++)
			vcmd_manager[k][i] = NULL;
	}

	init_bi_list(&global_process_manager);
	result = ConfigAXIFE(1); //1: normal, 2: bypass
	if (result < 0) {
		vcmd_release_AXIFE_IO();
		goto err1;
	}

	for (i = 0; i < venc_vcmd_core_num; i++) {
		hantrovcmd_data[i].vcmd_core_cfg = vc8000e_vcmd_core_array[i];
		hantrovcmd_data[i].hwregs = NULL;
		hantrovcmd_data[i].core_id = i;
		hantrovcmd_data[i].duration_without_int_max = INT_MIN_SUM_OF_IMAGE_SIZE_DEF;
		hantrovcmd_data[i].working_state = WORKING_STATE_IDLE;
		hantrovcmd_data[i].sw_cmdbuf_rdy_num = 0;
		hantrovcmd_data[i].spinlock = &owner_lock_vcmd[i];
		spin_lock_init(&owner_lock_vcmd[i]);
		hantrovcmd_data[i].wait_queue = &wait_queue_vcmd[i];
		init_waitqueue_head(&wait_queue_vcmd[i]);
		hantrovcmd_data[i].wait_abort_queue = &abort_queue_vcmd[i];
		init_waitqueue_head(&abort_queue_vcmd[i]);
		init_bi_list(&hantrovcmd_data[i].list_manager);
		hantrovcmd_data[i].duration_without_int = 0;
		vcmd_manager[vc8000e_vcmd_core_array[i].sub_module_type]
			    [vcmd_type_core_num[vc8000e_vcmd_core_array[i].sub_module_type]] = &hantrovcmd_data[i];
		vcmd_type_core_num[vc8000e_vcmd_core_array[i].sub_module_type]++;
		hantrovcmd_data[i].vcmd_reg_mem_busAddress =
			vcmd_registers_mem_pool.busAddress + i * VCMD_REGISTER_SIZE - base_ddr_addr;
		//next todo: split out
		hantrovcmd_data[i].mmu_vcmd_reg_mem_busAddress =
			vcmd_registers_mem_pool.mmu_bus_address + i * VCMD_REGISTER_SIZE;
		hantrovcmd_data[i].vcmd_reg_mem_virtualAddress =
			vcmd_registers_mem_pool.virtualAddress + i * VCMD_REGISTER_SIZE / 4;
		hantrovcmd_data[i].vcmd_reg_mem_size = VCMD_REGISTER_SIZE;
		memset(hantrovcmd_data[i].vcmd_reg_mem_virtualAddress, 0, VCMD_REGISTER_SIZE);
#ifdef SUPPORT_WATCHDOG
		hantrovcmd_data[i].watchdog_active = 0;
		hantrovcmd_data[i].watchdog_triggered = 0;
#endif
		hantrovcmd_data[i].vce_hang = 0;
		hantrovcmd_data[i].restart_cmdbuf_id = 0XFFFF;
		atomic64_set(&hantrovcmd_data[i].core_tot_cycles, 0);
		atomic64_set(&hantrovcmd_data[i].core_tot_exetime, 0);
		hantrovcmd_data[i].software_triger_abort = 0;
	}

	result = register_chrdev(hantrovcmd_major, DRIVER_NAME, &hantrovcmd_fops);
	if (result < 0) {
		LOG_ERR("vcmd_driver: unable to get major <%d>\n",
			hantrovcmd_major);
		goto err1;
	} else if (result != 0) {
		/* this is for dynamic major */
		hantrovcmd_major = result;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
    venc_class = class_create(VENC_CLASS_NAME);
#else
    venc_class = class_create(THIS_MODULE, VENC_CLASS_NAME);
#endif
    if (IS_ERR(venc_class)) {
        venc_class = NULL;
        LOG_ERR("%s(%d): Failed to create the class.\n", __func__, __LINE__);
        goto err;
    }else{
        device_create(venc_class, NULL, MKDEV(hantrovcmd_major, 0), NULL, VENC_DEV_NAME);
        LOG_NOTICE("create device node-%s done\n", VENC_DEV_NAME);
    }
	result = vcmd_reserve_IO();
	if (result < 0)
		goto err;
	for (i = 0; i < venc_vcmd_core_num; i++) {
		vcmd_reset_asic(&hantrovcmd_data[i]);
	}

	/* get the IRQ line */
	for (i = 0; i < venc_vcmd_core_num; i++) {
		if (!hantrovcmd_data[i].hwregs)
			continue;
		if (hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq != -1) {
			result = request_irq(
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq,
				hantrovcmd_isr,
				IRQF_SHARED,
				VENC_DRV_NAME,
				(void *)&hantrovcmd_data[i]);
			if (result == -EINVAL) {
				LOG_ERR("vcmd_driver: Bad vcmd_irq number or handler. core_id=%d\n", i);
				vcmd_release_IO();
				goto err;
			} else if (result == -EBUSY) {
				LOG_ERR("vcmd_driver: IRQ <%d> busy, change your config. core_id=%d\n",
				       hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq, i);
				vcmd_release_IO();
				goto err;
			}
		} else {
			LOG_INFO("vcmd_driver: IRQ not in use!\n");
		}
	}
	//cmdbuf pool allocation
	//init_vcmd_non_cachable_memory_allocate();
	//for cmdbuf management
	cmdbuf_used_pos = 0;
	for (k = 0; k < TOTAL_DISCRETE_CMDBUF_NUM; k++) {
		cmdbuf_used[k] = 0;
		global_cmdbuf_node[k] = NULL;
	}
	//cmdbuf_used[0] not be used, because int vector must non-zero
	cmdbuf_used_residual = TOTAL_DISCRETE_CMDBUF_NUM;
	cmdbuf_used_pos = 1;
	cmdbuf_used[0] = 1;
	cmdbuf_used_residual -= 1;

	LOG_INFO("module inserted. Major <%d>\n",
		hantrovcmd_major);

	if (create_kernel_process_manager()) {
		LOG_ERR("create kernel process manager failed\n");
		vcmd_release_IO();
		goto err;
	}
	for (i = 0; i < MAX_VCMD_TYPE; i++) {
		if (vcmd_type_core_num[i] == 0)
			continue;
		sema_init(&vcmd_reserve_cmdbuf_sem[i], 1);
	}
#ifdef IRQ_SIMULATION
	for (i = 0; i < 10000; i++)
		timer_reserve[i].timer = NULL;
#endif
	/*read all registers for each type of module for analyzing configuration in cwl*/
	for (i = 0; i < MAX_VCMD_TYPE; i++) {
		if (vcmd_type_core_num[i] == 0)
			continue;
		LOG_DBG("vcmd_init: vcmd_core_type is %d\n", i);
		read_main_module_all_registers(i);
	}

	_vcmd_kthread_create();

	return 0;
err:
	unregister_chrdev(hantrovcmd_major, DRIVER_NAME);
	if (venc_class) {
        LOG_NOTICE("destroy device node - %s\n", DRIVER_NAME);
        device_destroy(venc_class, MKDEV(hantrovcmd_major, 0));
        class_destroy(venc_class);
        venc_class = NULL;
    }
err1:
	if (hantrovcmd_data)
		vfree(hantrovcmd_data);
	LOG_INFO("vcmd_driver: module not inserted\n");
	return result;
}

static void hantroenc_vcmd_exit(bool powered)
{
	int i = 0;
	u32 result;

	_vcmd_kthread_stop();

	for (i = 0; i < venc_vcmd_core_num; i++) {
		if (!hantrovcmd_data[i].hwregs)
			continue;
		if (powered) {
			//disable interrupt at first
			vcmd_write_reg((const void *)hantrovcmd_data[i].hwregs,
				       VCMD_REGISTER_INT_CTL_OFFSET, 0x0000);
			//disable HW
			vcmd_write_reg((const void *)hantrovcmd_data[i].hwregs,
				       VCMD_REGISTER_CONTROL_OFFSET, 0x0000);
			//read status register
			result = vcmd_read_reg((const void *)hantrovcmd_data[i].hwregs,
					       VCMD_REGISTER_INT_STATUS_OFFSET);
			//clean status register
			vcmd_write_reg((const void *)hantrovcmd_data[i].hwregs,
				       VCMD_REGISTER_INT_STATUS_OFFSET, result);
		}

		/* free the vcmd IRQ */
		if (hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq != -1) {
			free_irq(hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq,
				 (void *)&hantrovcmd_data[i]);
		}
		release_cmdbuf_node_cleanup(&hantrovcmd_data[i].list_manager);
	}

	release_process_node_cleanup(&global_process_manager);

	vcmd_release_IO();
	vfree(hantrovcmd_data);
	unregister_chrdev(hantrovcmd_major, DRIVER_NAME);
	if (venc_class) {
		LOG_NOTICE("destroy device node - %s\n", DRIVER_NAME);
		device_destroy(venc_class, MKDEV(hantrovcmd_major, 0));
		class_destroy(venc_class);
		venc_class = NULL;
    }

	LOG_INFO("module removed\n");
}

static int vcmd_reserve_IO(void)
{
	u32 hwid;
	int i;
	u32 found_hw = 0;

	LOG_DBG("%s: venc_vcmd_core_num is %d\n", __func__,
		venc_vcmd_core_num);
	for (i = 0; i < venc_vcmd_core_num; i++) {
		hantrovcmd_data[i].hwregs = NULL;

		/* add by xiaogang.li for debug purpose */
		LOG_DBG("vcmd_base_addr[%d] = 0x%08lx, iosize = %d\n",
			i,
			hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr,
			hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize);

		if (!request_mem_region(
			    hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr,
			    hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize,
			    VENC_DRV_NAME)) {
			LOG_INFO("vcmd: failed to reserve HW regs for vcmd %d\n", i);
			LOG_INFO("vcmd: vcmd_base_addr = 0x%08lx, iosize = %d\n",
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr,
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize);
			continue;
		}

		hantrovcmd_data[i].hwregs = (volatile u8 *)ioremap(
			hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr,
			hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize);
		if (!hantrovcmd_data[i].hwregs) {
			LOG_INFO("vcmd: failed to ioremap HW regs\n");
			release_mem_region(
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr,
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize);
			continue;
		}

		/*read hwid and check validness and store it*/
		hwid = (u32)ioread32((void __iomem *)hantrovcmd_data[i].hwregs);
		LOG_DBG("%s: vcmd_data[%d].hwregs=0x%px\n", __func__,
			i, hantrovcmd_data[i].hwregs);
		LOG_DBG("hwid=0x%08x\n", hwid);
		hantrovcmd_data[i].hw_version_id = hwid;

		/* check for vcmd HW ID */
		if (((hwid >> 16) & 0xFFFF) != VCMD_HW_ID || hwid < HW_ID_1_2_1) {
			LOG_INFO("vcmd: HW not found at 0x%llx, hwid=0x%08x\n",
				(unsigned long long)hantrovcmd_data[i]
					.vcmd_core_cfg.vcmd_base_addr, hwid);
			iounmap((void __iomem *)hantrovcmd_data[i].hwregs);
			release_mem_region(
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr,
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize);
			hantrovcmd_data[i].hwregs = NULL;
			continue;
		}

		found_hw = 1;

		LOG_DBG("vcmd: HW at base <0x%llx> with ID <0x%08x>\n",
			(unsigned long long)hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr, hwid);
	}

	if (found_hw == 0) {
		LOG_ERR("vcmd: NO ANY HW found!!\n");
		return -1;
	}

	return 0;
}

static void vcmd_release_IO(void)
{
	u32 i;

	vcmd_release_AXIFE_IO();
	for (i = 0; i < venc_vcmd_core_num; i++) {
		if (hantrovcmd_data[i].hwregs) {
			iounmap((void __iomem *)hantrovcmd_data[i].hwregs);
			release_mem_region(
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr,
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize);
			hantrovcmd_data[i].hwregs = NULL;
		}
	}
}

static irqreturn_t hantrovcmd_isr(int irq, void *dev_id)
{
	unsigned int handled = 0;
	struct hantrovcmd_dev *dev = (struct hantrovcmd_dev *)dev_id;
	u32 irq_status = 0;
	unsigned long flags;
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *base_cmdbuf_node = NULL;
	bi_list_node* curr_cmdbuf_node=NULL;
	struct cmdbuf_obj *cmdbuf_obj = NULL;
	size_t exe_cmdbuf_busAddress;
	u32 cmdbuf_processed_num = 0;
	u32 cmdbuf_id = 0;

	/*If core is not reserved by any user, but irq is received, just clean it*/
	spin_lock_irqsave(dev->spinlock, flags);
	if (!dev->list_manager.head) {
		LOG_DBG("vcmd_isr:received IRQ but core has nothing to do.\n");
		irq_status = vcmd_read_reg((const void *)dev->hwregs,
					   VCMD_REGISTER_INT_STATUS_OFFSET);
		vcmd_write_reg((const void *)dev->hwregs,
			       VCMD_REGISTER_INT_STATUS_OFFSET, irq_status);
		spin_unlock_irqrestore(dev->spinlock, flags);
		return IRQ_HANDLED;
	}

	LOG_DBG("vcmd_isr: received IRQ!\n");
	irq_status = vcmd_read_reg((const void *)dev->hwregs,
				   VCMD_REGISTER_INT_STATUS_OFFSET);
#ifdef VCMD_DEBUG_INTERNAL
	{
		u32 i, fordebug;

		for (i = 0; i < ASIC_VCMD_SWREG_AMOUNT; i++) {
			fordebug = vcmd_read_reg((const void *)dev->hwregs, i * 4);
			LOG_INFO("vcmd register %d:0x%x\n", i, fordebug);
		}
	}
#endif

	if (!irq_status) {
		LOG_INFO("vcmd_isr error,irq_status :0x%x",irq_status);
		spin_unlock_irqrestore(dev->spinlock, flags);
		return IRQ_HANDLED;
	}

	LOG_DBG("irq_status of %d is: 0x%x\n", dev->core_id, irq_status);
	vcmd_write_reg((const void *)dev->hwregs,
		       VCMD_REGISTER_INT_STATUS_OFFSET, irq_status);
	dev->reg_mirror[VCMD_REGISTER_INT_STATUS_OFFSET / 4] = irq_status;

	if (dev->hw_version_id > HW_ID_1_0_C && (irq_status & 0x3f)) {
		//if error,read from register directly.
		LOG_INFO("irq_status of %d is: 0x%x, error occurred\n", dev->core_id, irq_status);
		cmdbuf_id =
			vcmd_get_register_value((const void *)dev->hwregs,
						dev->reg_mirror,
						HWIF_VCMD_CMDBUF_EXECUTING_ID);
		if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM) {
			LOG_ERR("vcmd_isr error cmdbuf_id greater than the ceiling !!\n");
			spin_unlock_irqrestore(dev->spinlock, flags);
			return IRQ_HANDLED;
		}
	} else if (dev->hw_version_id > HW_ID_1_0_C) {
		//read cmdbuf id from ddr
#ifdef VCMD_DEBUG_INTERNAL
		{
			u32 i, fordebug;

			LOG_INFO("ddr vcmd register phy_addr=0x%x\n",
				dev->vcmd_reg_mem_busAddress);
			LOG_INFO("ddr vcmd register virt_addr=0x%x\n",
				dev->vcmd_reg_mem_virtualAddress);
			for (i = 0; i < ASIC_VCMD_SWREG_AMOUNT; i++) {
				fordebug =
					*(dev->vcmd_reg_mem_virtualAddress + i);
				LOG_INFO("ddr vcmd register %d:0x%x\n", i,
					fordebug);
			}
		}
#endif

		cmdbuf_id = *(dev->vcmd_reg_mem_virtualAddress +
			      EXECUTING_CMDBUF_ID_ADDR);
		LOG_DBG("Venc vcmd_isr: cmdbuf_id %d from virtual!!\n", cmdbuf_id);
		if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM) {
			LOG_ERR("vcmd_isr error cmdbuf_id greater than the ceiling !!\n");
			spin_unlock_irqrestore(dev->spinlock, flags);
			return IRQ_HANDLED;
		}
	}

	if (dev->hw_version_id < HW_ID_1_1_1) {
		if (vcmd_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_RESET)) {
			LOG_INFO("VCMD_IRQ_RESET, working state from %u to idle, cmdbuf_id = %u\n"
				, dev->working_state, cmdbuf_id);
			//reset error,all cmdbuf that is not  done will be run again.
			new_cmdbuf_node = dev->list_manager.head;
			dev->working_state = WORKING_STATE_IDLE;
			//find the first run_done=0
			while (1) {
				if (!new_cmdbuf_node)
					break;
				cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
				if (cmdbuf_obj->cmdbuf_run_done == 0)
					break;
				new_cmdbuf_node = new_cmdbuf_node->next;
			}
			base_cmdbuf_node = new_cmdbuf_node;
			vcmd_delink_cmdbuf(dev, base_cmdbuf_node);
			vcmd_link_cmdbuf(dev, base_cmdbuf_node);
			if (dev->sw_cmdbuf_rdy_num != 0) {
				//restart new command
				vcmd_start(dev, base_cmdbuf_node);
			}
			handled++;
			spin_unlock_irqrestore(dev->spinlock, flags);
			return IRQ_HANDLED;
		}
	}

	if (vcmd_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_ABORT)) {
		u8 restart_curr_node = 0;

		LOG_INFO("VCMD_IRQ_ABORT, working state from %u to idle, cmdbuf_id = %u, vce_hang = %u, core_id = %u\n"
			, dev->working_state, cmdbuf_id, dev->vce_hang, dev->core_id);
#ifdef SUPPORT_WATCHDOG
		_vcmd_watchdog_stop(dev);
#endif
		//abort error,don't need to reset
		new_cmdbuf_node = dev->list_manager.head;
		dev->working_state = WORKING_STATE_IDLE;
		if (dev->hw_version_id > HW_ID_1_0_C) {
			new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
			if (!new_cmdbuf_node) {
				LOG_ERR("vcmd_isr abort error cmdbuf_id = %u !!\n", cmdbuf_id);
				spin_unlock_irqrestore(dev->spinlock, flags);
				return IRQ_HANDLED;
			}
		} else {
			exe_cmdbuf_busAddress = VCMDGetAddrRegisterValue(
				(const void *)dev->hwregs, dev->reg_mirror,
				HWIF_VCMD_EXECUTING_CMD_ADDR);
			//find the cmdbuf that tigers ABORT
			while (1) {
				if (!new_cmdbuf_node) {
					spin_unlock_irqrestore(dev->spinlock, flags);
					return IRQ_HANDLED;
				}
				cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
				if ((((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr) <=
				      exe_cmdbuf_busAddress) &&
				     (((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr + cmdbuf_obj->cmdbuf_size) >
				       exe_cmdbuf_busAddress))) &&
				    cmdbuf_obj->cmdbuf_run_done == 0)
					break;
				new_cmdbuf_node = new_cmdbuf_node->next;
			}
		}
		curr_cmdbuf_node = new_cmdbuf_node;
		// this cmdbuf and cmdbufs prior to itself, run_done = 1
		while (1) {
			if (!new_cmdbuf_node)
				break;
			cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
			if (cmdbuf_obj->cmdbuf_run_done == 0) {
				cmdbuf_obj->cmdbuf_run_done = 1;
				cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
				cmdbuf_processed_num++;
				if (dev->vce_hang && new_cmdbuf_node == curr_cmdbuf_node) {
					restart_curr_node = 1;
					cmdbuf_obj->cmdbuf_run_done = 0;
					LOG_INFO("VCMD_IRQ_ABORT, cmdbuf_id=%u, vce hang, cmdbuf_run_done = 0\n"
						, cmdbuf_obj->cmdbuf_id);
				}
			} else
				break;
			new_cmdbuf_node = new_cmdbuf_node->previous;
		}
		base_cmdbuf_node = curr_cmdbuf_node->next;
		if (dev->vce_hang) {
			/**
			 * if abort previous cmdbuf of hang, restart from next cmdbuf
			 * if abort cmdbuf of hang, restart current cmdbuf
			 */
			dev->vce_hang = 0;
			// LOG_INFO("%s:%d, vcmd aborted, vce_hang reset = %u\n", __FUNCTION__, __LINE__, dev->vce_hang);
			if (restart_curr_node == 1) {
				restart_curr_node = 0;
				base_cmdbuf_node=curr_cmdbuf_node;
			}

			if (base_cmdbuf_node && base_cmdbuf_node->data)
				dev->restart_cmdbuf_id =
					((struct cmdbuf_obj*)base_cmdbuf_node->data)->cmdbuf_id;
			else
				dev->restart_cmdbuf_id =
					((struct cmdbuf_obj*)curr_cmdbuf_node->data)->cmdbuf_id;
			LOG_INFO("VCMD_IRQ_ABORT, restart_cmdbuf_id = %u\n", dev->restart_cmdbuf_id);
		}
		vcmd_delink_cmdbuf(dev, base_cmdbuf_node);
		if (dev->software_triger_abort == 0 && dev->restart_cmdbuf_id == 0xFFFF) {
			//for QCFE
			vcmd_link_cmdbuf(dev, base_cmdbuf_node);
			if (dev->sw_cmdbuf_rdy_num != 0) {
				//restart new command
				vcmd_start(dev, base_cmdbuf_node);
			}
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		if (cmdbuf_processed_num)
			wake_up_interruptible_all(dev->wait_queue);
		//to let high priority cmdbuf be inserted
		wake_up_interruptible_all(dev->wait_abort_queue);
		handled++;
		return IRQ_HANDLED;
	}
	if (vcmd_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_BUSERR)) {
		LOG_INFO("VCMD_IRQ_BUSERR, working state from %u to idle, cmdbuf_id = %u\n"
			, dev->working_state, cmdbuf_id);
		//bus error, don't need to reset where to record status?
		new_cmdbuf_node = dev->list_manager.head;
		dev->working_state = WORKING_STATE_IDLE;
		if (dev->hw_version_id > HW_ID_1_0_C) {
			new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
			if (!new_cmdbuf_node) {
				LOG_ERR("VCMD_IRQ_BUSERR, cmdbuf_id = %u !!\n", cmdbuf_id);
				spin_unlock_irqrestore(dev->spinlock, flags);
				return IRQ_HANDLED;
			}
		} else {
			exe_cmdbuf_busAddress = VCMDGetAddrRegisterValue(
				(const void *)dev->hwregs, dev->reg_mirror,
				HWIF_VCMD_EXECUTING_CMD_ADDR);
			//find the buserr cmdbuf
			while (1) {
				if (!new_cmdbuf_node) {
					spin_unlock_irqrestore(dev->spinlock,
							       flags);
					return IRQ_HANDLED;
				}
				cmdbuf_obj = (struct cmdbuf_obj *)
						     new_cmdbuf_node->data;
				if ((((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr) <=
				      exe_cmdbuf_busAddress) &&
				     (((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr + cmdbuf_obj->cmdbuf_size) >
				       exe_cmdbuf_busAddress))) &&
				    (cmdbuf_obj->cmdbuf_run_done == 0))
					break;
				new_cmdbuf_node = new_cmdbuf_node->next;
			}
		}
		base_cmdbuf_node = new_cmdbuf_node;
		// this cmdbuf and cmdbufs prior to itself, run_done = 1
		while (1) {
			if (!new_cmdbuf_node)
				break;
			cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
			if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
				cmdbuf_obj->cmdbuf_run_done = 1;
				cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
				cmdbuf_processed_num++;
			} else
				break;
			new_cmdbuf_node = new_cmdbuf_node->previous;
		}
		new_cmdbuf_node = base_cmdbuf_node;
		if (new_cmdbuf_node) {
			cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
			cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_BUSERR;
		}
		base_cmdbuf_node = base_cmdbuf_node->next;
		vcmd_delink_cmdbuf(dev, base_cmdbuf_node);
		vcmd_link_cmdbuf(dev, base_cmdbuf_node);
		if (dev->sw_cmdbuf_rdy_num != 0) {
			//restart new command
			vcmd_start(dev, base_cmdbuf_node);
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		if (cmdbuf_processed_num)
			wake_up_interruptible_all(dev->wait_queue);
		handled++;
		return IRQ_HANDLED;
	}
	if (vcmd_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_TIMEOUT)) {
		//time out,need to reset
		new_cmdbuf_node = dev->list_manager.head;
		LOG_INFO("VCMD_IRQ_TIMEOUT, working state from %u to idle, cmdbuf_id = %u\n"
			, dev->working_state, cmdbuf_id);
		dev->working_state = WORKING_STATE_IDLE;
		if (dev->hw_version_id > HW_ID_1_0_C) {
			new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
			if (!new_cmdbuf_node) {
				LOG_ERR("vcmd_isr timeout error cmdbuf_id = %u !!\n", cmdbuf_id);
				spin_unlock_irqrestore(dev->spinlock, flags);
				return IRQ_HANDLED;
			}
		} else {
			exe_cmdbuf_busAddress = VCMDGetAddrRegisterValue(
				(const void *)dev->hwregs, dev->reg_mirror,
				HWIF_VCMD_EXECUTING_CMD_ADDR);
			//find the timeout cmdbuf
			while (1) {
				if (!new_cmdbuf_node) {
					spin_unlock_irqrestore(dev->spinlock,
							       flags);
					return IRQ_HANDLED;
				}
				cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
				if ((((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr) <=
				      exe_cmdbuf_busAddress) &&
				     (((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr + cmdbuf_obj->cmdbuf_size) >
				       exe_cmdbuf_busAddress))) &&
				    (cmdbuf_obj->cmdbuf_run_done == 0))
					break;
				new_cmdbuf_node = new_cmdbuf_node->next;
			}
		}
		base_cmdbuf_node = new_cmdbuf_node;
		new_cmdbuf_node = new_cmdbuf_node->previous;
		// this cmdbuf and cmdbufs prior to itself, run_done = 1
		while (1) {
			if (!new_cmdbuf_node)
				break;
			cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
			if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
				cmdbuf_obj->cmdbuf_run_done = 1;
				cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
				cmdbuf_processed_num++;
			} else
				break;
			new_cmdbuf_node = new_cmdbuf_node->previous;
		}
		vcmd_delink_cmdbuf(dev, base_cmdbuf_node);
		vcmd_link_cmdbuf(dev, base_cmdbuf_node);
		if (dev->sw_cmdbuf_rdy_num != 0) {
			//reset
			vcmd_reset_current_asic(dev);
			//restart new command
			vcmd_start(dev, base_cmdbuf_node);
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		if (cmdbuf_processed_num)
			wake_up_interruptible_all(dev->wait_queue);
		handled++;
		return IRQ_HANDLED;
	}
	if (vcmd_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_CMDERR)) {
		//command error,don't need to reset
		new_cmdbuf_node = dev->list_manager.head;
		LOG_INFO("VCMD_IRQ_CMDERR, working state from %u to idle, cmdbuf_id = %u\n"
			, dev->working_state, cmdbuf_id);
		dev->working_state = WORKING_STATE_IDLE;
		if (dev->hw_version_id > HW_ID_1_0_C) {
			new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
			if (!new_cmdbuf_node) {
				LOG_ERR("vcmd_isr cmderr error cmdbuf_id = %u !!\n", cmdbuf_id);
				spin_unlock_irqrestore(dev->spinlock, flags);
				return IRQ_HANDLED;
			}
		} else {
			exe_cmdbuf_busAddress = VCMDGetAddrRegisterValue(
				(const void *)dev->hwregs, dev->reg_mirror,
				HWIF_VCMD_EXECUTING_CMD_ADDR);
			//find the cmderror cmdbuf
			while (1) {
				if (!new_cmdbuf_node) {
					spin_unlock_irqrestore(dev->spinlock,
							       flags);
					return IRQ_HANDLED;
				}
				cmdbuf_obj = (struct cmdbuf_obj *)
						     new_cmdbuf_node->data;
				if ((((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr) <=
				      exe_cmdbuf_busAddress) &&
				     (((cmdbuf_obj->cmdbuf_busAddress - base_ddr_addr + cmdbuf_obj->cmdbuf_size) >
				       exe_cmdbuf_busAddress))) &&
				    (cmdbuf_obj->cmdbuf_run_done == 0))
					break;
				new_cmdbuf_node = new_cmdbuf_node->next;
			}
		}
		base_cmdbuf_node = new_cmdbuf_node;
		// this cmdbuf and cmdbufs prior to itself, run_done = 1
		while (1) {
			if (!new_cmdbuf_node)
				break;
			cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
			if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
				cmdbuf_obj->cmdbuf_run_done = 1;
				cmdbuf_obj->executing_status =
					CMDBUF_EXE_STATUS_OK;
				cmdbuf_processed_num++;
			} else
				break;
			new_cmdbuf_node = new_cmdbuf_node->previous;
		}
		new_cmdbuf_node = base_cmdbuf_node;
		if (new_cmdbuf_node) {
			cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
			cmdbuf_obj->executing_status =
				CMDBUF_EXE_STATUS_CMDERR; //cmderr
		}
		base_cmdbuf_node = base_cmdbuf_node->next;
		vcmd_delink_cmdbuf(dev, base_cmdbuf_node);
		vcmd_link_cmdbuf(dev, base_cmdbuf_node);
		if (dev->sw_cmdbuf_rdy_num != 0) {
			//restart new command
			vcmd_start(dev, base_cmdbuf_node);
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		if (cmdbuf_processed_num)
			wake_up_interruptible_all(dev->wait_queue);
		handled++;
		return IRQ_HANDLED;
	}

	if (vcmd_get_register_mirror_value(dev->reg_mirror,
					   HWIF_VCMD_IRQ_ENDCMD)) {
		//end command interrupt
		new_cmdbuf_node = dev->list_manager.head;
		LOG_INFO("VCMD_IRQ_ENDCMD, working state from %u to idle, cmdbuf_id = %u\n"
			, dev->working_state, cmdbuf_id);
		dev->working_state = WORKING_STATE_IDLE;
		if (dev->hw_version_id > HW_ID_1_0_C) {
			new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
			if (!new_cmdbuf_node) {
				LOG_ERR("vcmd_isr endcmd error cmdbuf_id = %u !!\n", cmdbuf_id);
				spin_unlock_irqrestore(dev->spinlock, flags);
				return IRQ_HANDLED;
			}
		} else {
			//find the end cmdbuf
			while (1) {
				if (!new_cmdbuf_node) {
					spin_unlock_irqrestore(dev->spinlock,
							       flags);
					return IRQ_HANDLED;
				}
				cmdbuf_obj = (struct cmdbuf_obj *)
						     new_cmdbuf_node->data;
				if ((cmdbuf_obj->has_end_cmdbuf == 1) &&
				    (cmdbuf_obj->cmdbuf_run_done == 0))
					break;
				new_cmdbuf_node = new_cmdbuf_node->next;
			}
		}
		base_cmdbuf_node = new_cmdbuf_node;
		// this cmdbuf and cmdbufs prior to itself, run_done = 1
		while (1) {
			if (!new_cmdbuf_node)
				break;
			cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
			if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
				cmdbuf_obj->cmdbuf_run_done = 1;
				cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
				cmdbuf_processed_num++;
			} else
				break;
			new_cmdbuf_node = new_cmdbuf_node->previous;
		}
		base_cmdbuf_node = base_cmdbuf_node->next;
		vcmd_delink_cmdbuf(dev, base_cmdbuf_node);
		vcmd_link_cmdbuf(dev, base_cmdbuf_node);
		if (dev->sw_cmdbuf_rdy_num != 0) {
			//restart new command
			vcmd_start(dev, base_cmdbuf_node);
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		if (cmdbuf_processed_num)
			wake_up_interruptible_all(dev->wait_queue);
		handled++;
		return IRQ_HANDLED;
	}
	if (dev->hw_version_id <= HW_ID_1_0_C)
		cmdbuf_id = vcmd_get_register_mirror_value(
			dev->reg_mirror, HWIF_VCMD_IRQ_INTCMD);
	if (cmdbuf_id) {
		if (dev->hw_version_id <= HW_ID_1_0_C) {
			if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM) {
				LOG_ERR("vcmd_isr error cmdbuf_id greater than the ceiling !!\n");
				spin_unlock_irqrestore(dev->spinlock, flags);
				return IRQ_HANDLED;
			}
		}
		new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
		if (!new_cmdbuf_node) {
			LOG_INFO("task has been done, but vdec interrupt occurred, cmdbuf_id line=%d, cmdbuf_id=%u!!\n", __LINE__, cmdbuf_id);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return IRQ_HANDLED;
		}
		// interrupt cmdbuf and cmdbufs prior to itself, run_done = 1
		while (1) {
			if (!new_cmdbuf_node)
				break;
			cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
			if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
				cmdbuf_obj->cmdbuf_run_done = 1;
				cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
				cmdbuf_processed_num++;
			} else
				break;
			new_cmdbuf_node = new_cmdbuf_node->previous;
		}
#ifdef SUPPORT_WATCHDOG
		_vcmd_watchdog_feed(dev);
#endif
		if (dev->vce_hang == 0) {
			dev->vce_hang = check_vce_hang(dev, cmdbuf_id);
			if (dev->vce_hang) {
				_vcmd_kthread_wakeup();
			}
		}
		handled++;
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	if (cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	if (!handled) {
		LOG_DBG("IRQ received, but not eswin's!\n");
	}
	return IRQ_HANDLED;
}

static void vcmd_reset_asic(struct hantrovcmd_dev *dev)
{
	int i;
	u32 result;

	if (dev->hwregs) {
		//disable interrupt at first
		vcmd_write_reg((const void *)dev->hwregs,
					VCMD_REGISTER_INT_CTL_OFFSET, 0x0000);
		//reset all
		vcmd_write_reg((const void *)dev->hwregs,
					VCMD_REGISTER_CONTROL_OFFSET, 0x0002);
		//read status register
		result = vcmd_read_reg((const void *)dev->hwregs,
						VCMD_REGISTER_INT_STATUS_OFFSET);
		//clean status register
		vcmd_write_reg((const void *)dev->hwregs,
					VCMD_REGISTER_INT_STATUS_OFFSET, result);
		//set register sw_exe_cmdbuf_count be 0
		vcmd_write_reg((const void *)dev->hwregs,
					VCMD_REGISTER_EXE_CMDBUF_COUNT, 0x0000);
		for (i = VCMD_REGISTER_CONTROL_OFFSET;
				i < dev->vcmd_core_cfg.vcmd_iosize; i += 4) {
			//set all register 0
			vcmd_write_reg((const void *)dev->hwregs, i,
						0x0000);
		}
		//enable all interrupt
		vcmd_write_reg((const void *)dev->hwregs,
					VCMD_REGISTER_INT_CTL_OFFSET,
					0xffffffff);
	}
}

static void vcmd_reset_current_asic(struct hantrovcmd_dev *dev)
{
	u32 result;

	if (dev->hwregs) {
		//disable interrupt at first
		vcmd_write_reg((const void *)dev->hwregs,
			       VCMD_REGISTER_INT_CTL_OFFSET, 0x0000);
		//reset core
		vcmd_write_reg((const void *)dev->hwregs,
			       VCMD_REGISTER_CONTROL_OFFSET, 0x0004);
		//read status register
		result = vcmd_read_reg((const void *)dev->hwregs,
				       VCMD_REGISTER_INT_STATUS_OFFSET);
		//clean status register
		vcmd_write_reg((const void *)dev->hwregs,
			       VCMD_REGISTER_INT_STATUS_OFFSET, result);
	}
}

extern int memalloc_init(void);
extern void memalloc_exit(void);

#ifdef VCMD_DEBUG_INTERNAL
static void printk_vcmd_register_debug(const void *hwregs, char *info)
{
	u32 i, fordebug;

	for (i = 0; i < ASIC_VCMD_SWREG_AMOUNT; i++) {
		fordebug = vcmd_read_reg((const void *)hwregs, i * 4);
		LOG_INFO("%s vcmd register %d:0x%x\n", info, i, fordebug);
	}
}
#endif

int vc8000e_vcmd_init(void)
{
	int ret = vcmd_mem_init();	/* AX630C-PORT: honor the return (was ignored -> panic on NULL) */

	if (ret) {
		LOG_ERR("AX630C-PORT: vcmd_mem_init failed (%d); aborting init\n", ret);
		return ret;
	}
	return hantroenc_vcmd_init();
}

int vc8000e_vcmd_cleanup(bool powered)
{
	hantroenc_vcmd_exit(powered);
	vcmd_mem_cleanup();

	return 0;
}

void vc8000e_vcmd_abort(u32 core_id)
{
	struct hantrovcmd_dev *dev = NULL;

	if (core_id >= venc_vcmd_core_num) {
		LOG_ERR("vc8000e_vcmd_abort, invalid core_id = %u, venc_vcmd_core_num = %u\n"
			, core_id, venc_vcmd_core_num);
		return;
	}
	dev = &hantrovcmd_data[core_id];
	LOG_INFO("vc8000e_vcmd_abort, core_id = %u\n", dev->core_id);
#ifdef SUPPORT_WATCHDOG
	/** stop watchdog timer firstly*/
	_vcmd_watchdog_stop_and_sync(dev);
#endif
	/** and the wait kthread exit*/
	_vcmd_kthread_stop();

	/** set vce_hang be 1, avoid the current running task is already in the hang state.*/
	dev->vce_hang = 1;
	vcmd_abort(dev);
}

int vc8000e_vcmd_reset(u32 core_id)
{
	unsigned long flags;
	struct hantrovcmd_dev *dev = NULL;

	LOG_DBG("vc8000e_vcmd_reset\n");

	if (core_id >= venc_vcmd_core_num) {
		LOG_ERR("vc8000e_vcmd_reset, invalid core_id = %u, venc_vcmd_core_num = %u\n"
			, core_id, venc_vcmd_core_num);
		return -1;
	}
	dev = &hantrovcmd_data[core_id];

	spin_lock_irqsave(dev->spinlock, flags);

	/** reset asic*/
	vcmd_reset_asic(dev);

	/** re-initialize the dev state.*/
    u32 working_state = vcmd_get_register_value((const void *)dev->hwregs, dev->reg_mirror, HWIF_VCMD_WORK_STATE);
    u32 rdy_cmdbuf_count = vcmd_get_register_value((const void *)dev->hwregs, dev->reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT);
    u32 exe_cmdbuf_count = vcmd_get_register_value((const void *)dev->hwregs, dev->reg_mirror, HWIF_VCMD_EXE_CMDBUF_COUNT);

	LOG_DBG("vc8000e_vcmd_reset, core_id = %u, working_state %u -> %u, sw_cmdbuf_rdy_num 0x%x -> 0x%x\n"
        , core_id, dev->working_state, working_state, dev->sw_cmdbuf_rdy_num, rdy_cmdbuf_count);
    dev->working_state = working_state;
    dev->sw_cmdbuf_rdy_num = rdy_cmdbuf_count;
    /** reset the sw_exe_cmdbuf_count, because the vcmd was aborted*/
    if (exe_cmdbuf_count > dev->sw_cmdbuf_rdy_num) {
        LOG_WARN("vc8000e_vcmd_reset, unexpected vcmd cmdbuf count, exe_cmdbuf_count > rdy_cmdbuf_count, 0x%x > 0x%x\n"
            , exe_cmdbuf_count, dev->sw_cmdbuf_rdy_num);
    }
	spin_unlock_irqrestore(dev->spinlock, flags);

	return 0;
}

void vc8000e_vcmd_restart(u32 core_id) {
    struct hantrovcmd_dev *dev = NULL;

    if (core_id >= venc_vcmd_core_num) {
        LOG_ERR("vc8000e_vcmd_restart, invalid core_id = %u, venc_vcmd_core_num = %u\n"
            , core_id, venc_vcmd_core_num);
        return;
    }
    dev = &hantrovcmd_data[core_id];

    LOG_INFO("vc8000e_vcmd_restart for core_id %u\n", dev->core_id);
	/** create the kthread firstly*/
	_vcmd_kthread_create();
	/** donot need to create watchdog timer, it would be started if startup vcmd task*/
	restart_vcmd(dev);
}

/** get status statistcs*/
void hantroenc_dev_stat(u32 core_id, u32 *module_type, u64 *tot_cycles, u64 *tot_exetime, u64 *freq)
{
	if (core_id >= venc_vcmd_core_num) {
		LOG_ERR("hantroenc_dev_stat, unknown core_id = %u\n", core_id);
		return;
	}
	if (module_type) {
		*module_type = hantrovcmd_data[core_id].vcmd_core_cfg.sub_module_type;
	}
	if (tot_cycles) {
		*tot_cycles = atomic64_read(&hantrovcmd_data[core_id].core_tot_cycles);
	}
	if (tot_exetime) {
		*tot_exetime = atomic64_read(&hantrovcmd_data[core_id].core_tot_exetime);
	}
	if (freq) {
		*freq = hantrovcmd_data[core_id].vcmd_core_cfg.freq;
	}
}

/** get vcmd interrupt time */
void hantroenc_get_vcmd_int(u32 core_id, s32 *vcmd_int)
{
	if (!vcmd_int) {
		return;
	}
	if (core_id >= venc_vcmd_core_num) {
		LOG_ERR("hantroenc_get_vcmd_int, unknown core_id = %u\n", core_id);
		*vcmd_int = -1;
		return;
	}
	unsigned long flags;
	struct hantrovcmd_dev *dev = &hantrovcmd_data[core_id];

	spin_lock_irqsave(dev->spinlock, flags);
	*vcmd_int = (s32)dev->duration_without_int_max;
	spin_unlock_irqrestore(dev->spinlock, flags);
}

/** set vcmd interrupt time */
void hantroenc_set_vcmd_int(u32 core_id, u32 vcmd_int)
{
	if (core_id >= venc_vcmd_core_num) {
		LOG_ERR("hantroenc_set_vcmd_int, unknown core_id = %u\n", core_id);
		return;
	}
	if (vcmd_int > INT_MIN_SUM_OF_IMAGE_SIZE_MAX) {
		LOG_ERR("hantroenc_set_vcmd_int, outof range[0,%u]\n", vcmd_int);
		return;
	}

	unsigned long flags;
	struct hantrovcmd_dev *dev = &hantrovcmd_data[core_id];

	spin_lock_irqsave(dev->spinlock, flags);
	dev->duration_without_int_max = vcmd_int;
	spin_unlock_irqrestore(dev->spinlock, flags);
	LOG_INFO("hantroenc_set_vcmd_int, set vcmd_int of %u to %u\n", core_id, vcmd_int);
}
