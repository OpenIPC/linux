/* SPDX-License-Identifier: GPL-2.0
 ****************************************************************************
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

#ifndef _VC8000_VCMD_DRIVER_H_
#define _VC8000_VCMD_DRIVER_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
// #include "hantrommu.h"

#ifdef SUPPORT_DMA_HEAP
#ifdef __KERNEL__
#include <linux/dmabuf-heap-import-helper.h>
#endif
#endif

//#define HANTROAXIFE_SUPPORT
#ifdef HANTROAXIFE_SUPPORT
#include "vc8000_axife.h"
#endif

#define ENC_DEV_IDLEWAIT_TIME       (msecs_to_jiffies(10000))

#define ENC_CORE_NUM                (4)

#undef ptr_t
#define ptr_t PTR_T_KERNEL
typedef size_t ptr_t;

#ifdef SUPPORT_DMA_HEAP
struct dmabuf_cfg {
	int dmabuf_fd;
	unsigned long iova;
};
#endif

#ifdef __KERNEL__
struct filp_priv {
#ifdef SUPPORT_DMA_HEAP
	struct heap_root root;
	struct heap_root root_d1;
#endif
	void *dev;
	atomic_t core_tasks[ENC_CORE_NUM];  /** for task count of 4 cores*/
};
#endif

#define ENC_HW_ID1                  0x48320100
#define ENC_HW_ID2                  0x80006000
#define CORE_INFO_MODE_OFFSET       31
#define CORE_INFO_AMOUNT_OFFSET     28

/* Use 'k' as magic number */
#define HANTRO_IOC_MAGIC 'k'

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

#define HANTRO_IOCG_HWOFFSET                           _IOR(HANTRO_IOC_MAGIC,  3, unsigned long *)
#define HANTRO_IOCG_HWIOSIZE                           _IOR(HANTRO_IOC_MAGIC,  4, unsigned int *)
#define HANTRO_IOC_CLI                                 _IO(HANTRO_IOC_MAGIC,  5)
#define HANTRO_IOC_STI                                 _IO(HANTRO_IOC_MAGIC,  6)
#define HANTRO_IOCX_VIRT2BUS                           _IOWR(HANTRO_IOC_MAGIC,  7, unsigned long *)
#define HANTRO_IOCH_ARDRESET                           _IO(HANTRO_IOC_MAGIC, 8)   /* debugging tool */
#define HANTRO_IOCG_SRAMOFFSET                         _IOR(HANTRO_IOC_MAGIC,  9, unsigned long *)
#define HANTRO_IOCG_SRAMEIOSIZE                        _IOR(HANTRO_IOC_MAGIC,  10, unsigned int *)
#define HANTRO_IOCH_ENC_RESERVE                        _IOR(HANTRO_IOC_MAGIC, 11, unsigned int *)
#define HANTRO_IOCH_ENC_RELEASE                        _IOR(HANTRO_IOC_MAGIC, 12, unsigned int *)
#define HANTRO_IOCG_CORE_NUM                           _IOR(HANTRO_IOC_MAGIC, 13, unsigned int *)
#define HANTRO_IOCG_CORE_INFO                          _IOR(HANTRO_IOC_MAGIC, 14, SUBSYS_CORE_INFO *)
#define HANTRO_IOCG_CORE_WAIT                          _IOR(HANTRO_IOC_MAGIC, 15, unsigned int *)
#define HANTRO_IOCG_ANYCORE_WAIT                       _IOR(HANTRO_IOC_MAGIC, 16, CORE_WAIT_OUT *)
#define HANTRO_IOCG_ANYCORE_WAIT_POLLING               _IOR(HANTRO_IOC_MAGIC, 17, CORE_WAIT_OUT *)
#define HANTRO_IOCG_DUMP_ALL_REGISTERS                 _IO(HANTRO_IOC_MAGIC, 18)
#define HANTRO_IOCG_DUMP_KEY_REGISTERS                 _IO(HANTRO_IOC_MAGIC, 19)
#define HANTRO_IOCG_DEBUG_TRIGGER_REGISTER             _IO(HANTRO_IOC_MAGIC, 20)

#define HANTRO_IOCH_GET_CMDBUF_PARAMETER               _IOWR(HANTRO_IOC_MAGIC, 25, struct cmdbuf_mem_parameter *)
#define HANTRO_IOCH_GET_CMDBUF_POOL_SIZE               _IOWR(HANTRO_IOC_MAGIC, 26, unsigned long)
#define HANTRO_IOCH_SET_CMDBUF_POOL_BASE               _IOWR(HANTRO_IOC_MAGIC, 27, unsigned long)
#define HANTRO_IOCH_GET_VCMD_PARAMETER                 _IOWR(HANTRO_IOC_MAGIC, 28, struct config_parameter *)
#define HANTRO_IOCH_RESERVE_CMDBUF                     _IOWR(HANTRO_IOC_MAGIC, 29, struct exchange_parameter *)
#define HANTRO_IOCH_LINK_RUN_CMDBUF                    _IOR(HANTRO_IOC_MAGIC, 30, u16 *)
#define HANTRO_IOCH_WAIT_CMDBUF                        _IOR(HANTRO_IOC_MAGIC, 31, u16 *)
#define HANTRO_IOCH_RELEASE_CMDBUF                     _IOR(HANTRO_IOC_MAGIC, 32, u16 *)
#define HANTRO_IOCH_POLLING_CMDBUF                     _IOR(HANTRO_IOC_MAGIC, 33, u16 *)
#define HANTRO_IOCH_DMA_HEAP_GET_IOVA                  _IOR(HANTRO_IOC_MAGIC, 34, struct dmabuf_cfg *)
#define HANTRO_IOCH_DMA_HEAP_PUT_IOVA                  _IOR(HANTRO_IOC_MAGIC, 35, unsigned int *)

#define HANTRO_IOCH_GET_VCMD_ENABLE                    _IOWR(HANTRO_IOC_MAGIC, 50, unsigned long)
#define HANTRO_IOCH_GET_MMU_ENABLE                     _IOWR(HANTRO_IOC_MAGIC, 51, unsigned long)

#define GET_ENCODER_IDX(type_info)                    (CORE_VC8000E)
#define CORETYPE(core)                                (1 << (core))
#define HANTRO_IOC_MAXNR                              60

/*priority support*/

#define MAX_CMDBUF_PRIORITY_TYPE   2 //0:normal priority,1:high priority

#define CMDBUF_PRIORITY_NORMAL     0
#define CMDBUF_PRIORITY_HIGH       1

#define OPCODE_WREG               (0x01 << 27)
#define OPCODE_END                (0x02 << 27)
#define OPCODE_NOP                (0x03 << 27)
#define OPCODE_RREG               (0x16 << 27)
#define OPCODE_INT                (0x18 << 27)
#define OPCODE_JMP                (0x19 << 27)
#define OPCODE_STALL              (0x09 << 27)
#define OPCODE_CLRINT             (0x1a << 27)
#define OPCODE_JMP_RDY0           (0x19U << 27)
#define OPCODE_JMP_RDY1           ((0x19 << 27) | (1 << 26))
#define JMP_IE_1                  BIT(25)
#define JMP_RDY_1                 BIT(26)

#define CLRINT_OPTYPE_READ_WRITE_1_CLEAR         0
#define CLRINT_OPTYPE_READ_WRITE_0_CLEAR         1
#define CLRINT_OPTYPE_READ_CLEAR                 2

#define VC8000E_FRAME_RDY_INT_MASK                        0x0001
#define VC8000E_CUTREE_RDY_INT_MASK                       0x0002
#define VC8000E_DEC400_INT_MASK                           0x0004
#define VC8000E_L2CACHE_INT_MASK                          0x0008
#define VC8000E_MMU_INT_MASK                              0x0010
#define CUTREE_MMU_INT_MASK                               0x0020

#define VC8000D_FRAME_RDY_INT_MASK                        0x0100
#define VC8000D_DEC400_INT_MASK                           0x0400
#define VC8000D_L2CACHE_INT_MASK                          0x0800
#define VC8000D_MMU_INT_MASK                              0x1000

#define VC8000D_DEC400_INT_MASK_1_1_1                      0x0200
#define VC8000D_L2CACHE_INT_MASK_1_1_1                     0x0400
#define VC8000D_MMU_INT_MASK_1_1_1                         0x0800

#define HW_ID_1_0_C                0x43421001
#define HW_ID_1_1_1                0x43421101
#define HW_ID_1_1_2                0x43421102
#define HW_ID_1_1_3                0x43421103
#define HW_ID_1_2_1                0x43421201

#ifndef ASIC_SWREG_AMOUNT
#define ASIC_SWREG_AMOUNT                  543 //from encswhwregister.h
#endif

/*these size need to be modified according to hw config.*/
#define ENCODER_REGISTER_SIZE              ASIC_SWREG_AMOUNT
#define DECODER_REGISTER_SIZE              512
#define IM_REGISTER_SIZE                   ASIC_SWREG_AMOUNT
#define JPEG_ENCODER_REGISTER_SIZE         ASIC_SWREG_AMOUNT
#define JPEG_DECODER_REGISTER_SIZE         512

#define DEC400_REGISTER_SIZE               1600
#define MMU_REGISTER_SIZE                  500
#define L2CACHE_REGISTER_SIZE              500
#define AXIFE_REGISTER_SIZE                500

#define ASIC_STATUS_SEGMENT_READY       0x1000
#define ASIC_STATUS_FUSE_ERROR          0x200
#define ASIC_STATUS_SLICE_READY         0x100
#define ASIC_STATUS_LINE_BUFFER_DONE    0x080  /* low latency */
#define ASIC_STATUS_HW_TIMEOUT          0x040
#define ASIC_STATUS_BUFF_FULL           0x020
#define ASIC_STATUS_HW_RESET            0x010
#define ASIC_STATUS_ERROR               0x008
#define ASIC_STATUS_FRAME_READY         0x004
#define ASIC_IRQ_LINE                   0x001
#define ASIC_STATUS_ALL       (ASIC_STATUS_SEGMENT_READY |\
			       ASIC_STATUS_FUSE_ERROR |\
			       ASIC_STATUS_SLICE_READY |\
			       ASIC_STATUS_LINE_BUFFER_DONE |\
			       ASIC_STATUS_HW_TIMEOUT |\
			       ASIC_STATUS_BUFF_FULL |\
			       ASIC_STATUS_HW_RESET |\
			       ASIC_STATUS_ERROR |\
			       ASIC_STATUS_FRAME_READY)

enum {
  CORE_VC8000E = 0,
  CORE_VC8000EJ = 1,
  CORE_CUTREE = 2,
  CORE_DEC400 = 3,
  CORE_MMU = 4,
  CORE_L2CACHE = 5,
  CORE_AXIFE = 6,
  CORE_APBFT = 7,
  CORE_MMU_1 = 8,
  CORE_AXIFE_1 = 9,
  CORE_MAX
};

//#define CORE_MAX  (CORE_MMU)

/*module_type support*/

enum vcmd_module_type {
	VCMD_TYPE_ENCODER = 0,
	VCMD_TYPE_CUTREE,
	VCMD_TYPE_DECODER,
	VCMD_TYPE_JPEG_ENCODER,
	VCMD_TYPE_JPEG_DECODER,
	MAX_VCMD_TYPE
};

struct cmdbuf_mem_parameter {
	u32 *cmd_virt_addr; //cmdbuf pool base virtual address
	ptr_t cmd_phy_addr; //cmdbuf pool base physical address, it's for cpu
	ptr_t cmd_hw_addr; //cmdbuf pool base hardware address, it's for hardware ip
	u32 cmd_total_size; //cmdbuf pool total size in bytes.
	u16 cmd_unit_size; //one cmdbuf size in bytes. all cmdbuf have same size.
	u32 *status_virt_addr;
	ptr_t status_phy_addr; //status cmdbuf pool base physical address, it's for cpu
	ptr_t status_hw_addr; //status cmdbuf pool base hardware address, it's for hardware ip
	u32 status_total_size; //status cmdbuf pool total size in bytes.
	u16 status_unit_size; //one status cmdbuf size in bytes. all status cmdbuf have same size.
	ptr_t base_ddr_addr; //for pcie interface, hw can only access phy_cmdbuf_addr-pcie_base_ddr_addr.
		//for other interface, this value should be 0?
};

struct config_parameter {
	u16 module_type; //input vc8000e=0,cutree=1,vc8000d=2，jpege=3, jpegd=4
	u16 vcmd_core_num; //output, how many vcmd cores are there with corresponding module_type.
	u16 submodule_main_addr; //output,if submodule addr == 0xffff, this submodule does not exist.
	u16 submodule_dec400_addr; //output ,if submodule addr == 0xffff, this submodule does not exist.
	u16 submodule_L2Cache_addr; //output,if submodule addr == 0xffff, this submodule does not exist.

	u16 submodule_MMU_addr[2]; //output,if submodule addr == 0xffff, this submodule does not exist.
	u16 submodule_axife_addr[2]; //output,if submodule addr == 0xffff, this submodule does not exist.
	u16 config_status_cmdbuf_id; // output , this status comdbuf save the all register values read in driver init.//used for analyse configuration in cwl.
	u32 vcmd_hw_version_id;
};

/*need to consider how many memory should be allocated for status.*/
struct exchange_parameter {
	u32 executing_time; //input ;executing_time=encoded_image_size*(rdoLevel+1)*(rdoq+1);
	u16 module_type; //input input vc8000e=0,IM=1,vc8000d=2，jpege=3, jpegd=4
	u16 cmdbuf_size; //input, reserve is not used; link and run is input.
	u16 priority; //input,normal=0, high/live=1
	u16 cmdbuf_id; //output ,it is unique in driver.
	u16 core_id; //just used for polling.
	u16 numa_id; //bind the task to the specified numa_id
};

typedef struct CoreWaitOut {
	u32 job_id[4];
	u32 irq_status[4];
	u32 irq_num;
} CORE_WAIT_OUT;

typedef struct {
	u32 subsys_idx;
	u32 core_type;
	unsigned long offset;
	u32 reg_size;
	int irq;
} CORE_CONFIG;

typedef struct {
	unsigned long base_addr;
	u32 iosize;
	u32 resource_shared; //indicate the core share resources with other cores or not.If 1, means cores can not work at the same time.
	// u32 type_main_core;
} SUBSYS_CONFIG;

typedef struct {
	// u32 type_vce;
	u32 type_info; //indicate which IP is contained in this subsystem and each uses one bit of this variable
	unsigned long offset[CORE_MAX];
	unsigned long regSize[CORE_MAX];
	int irq[CORE_MAX];
} SUBSYS_CORE_INFO;

/*for all vcmds, the core info should be listed here for subsequent use*/
typedef struct vcmd_config {
	unsigned long vcmd_base_addr;
	u32 vcmd_iosize;
	int vcmd_irq;
	u32 sub_module_type; /*input vc8000e=0,IM=1,vc8000d=2，jpege=3, jpegd=4*/
	u16 submodule_main_addr; // in byte
	u16 submodule_dec400_addr; //if submodule addr == 0xffff, this submodule does not exist.// in byte
	u16 submodule_L2Cache_addr; // in byte
	u16 submodule_MMU_addr[2]; // in byte
	u16 submodule_axife_addr[2]; // in byte
	u32 freq; // indicate the frequency of the core.
} VCMD_CONFIG;

typedef struct {
	SUBSYS_CONFIG cfg;
	SUBSYS_CORE_INFO core_info;
} SUBSYS_DATA;

extern struct platform_device *venc_pdev;
extern struct platform_device *venc_pdev_d1;

#define DUMP_HARDWARE_FEATURES
#ifdef DUMP_HARDWARE_FEATURES
typedef union {
	struct swreg4_s {
		u32 reserved0:19;
		u32 sw_enc_max_trb_size:2;
		u32 sw_enc_min_trb_size:2;
		u32 sw_enc_max_cb_size:2;
		u32 sw_enc_min_cb_size:2;
		u32 reserved1:5;
	} swreg4;
	struct swreg80_s {
		u32 sw_enc_HWMaxVideoWidth:13;
		u32 sw_enc_HWBusWidth:2;
		u32 sw_enc_HWJPEGSupport:1;
		u32 sw_enc_HWTU32Support:1;
		u32 sw_enc_HWRFCSupport:1;
		u32 sw_enc_HWProgRdoSupport:1;
		u32 sw_enc_HWLineBufSupport:1;
		u32 sw_enc_HWCavlcSupport:1;
		u32 sw_enc_HWBus:3;
		u32 sw_enc_HWMain10Support:1;
		u32 sw_enc_HWDenoiseSupport:1;
		u32 sw_enc_HWVp9Support:1;
		u32 sw_enc_HWHevcSupport:1;
		u32 sw_enc_HWRgbSupport:1;
		u32 sw_enc_HWBFrameSupport:1;
		u32 sw_enc_HWScalingSupport:1;
		u32 sw_enc_HWH264Support:1;
	} swreg80;
	struct swreg214_s {
		u32 sw_enc_HWMaxVideoWithJPEG:13;
		u32 sw_enc_HWMaxVideoWithH264:13;
		u32 sw_enc_HWRoiMapVersion:3;
		u32 sw_enc_HWIntraTU32Support:1;
		u32 sw_enc_HWAbsQpSupport:1;
		u32 sw_enc_HWLJPEGSupport:1;
	} swreg214;
	struct swreg226_s {
		u32 sw_enc_reserved:9;
		u32 sw_enc_HWROI8Support:1;
		u32 sw_enc_HWGmvSupport:1;
		u32 sw_enc_HWJpeg422Support:1;
		u32 sw_enc_HWCtbRcVersion:3;
		u32 sw_enc_me_vert_searchrange_h264:6;
		u32 sw_enc_me_vert_searchrange_hevc:6;
		u32 sw_enc_HWCuInforVersion:3;
		u32 sw_enc_HWP010RefSupport:1;
		u32 sw_enc_HWSSIMSupport:1;
	} swreg226;
	struct swreg227_s {
		u32 sw_enc_ssim_y_numerator_lsb;
	} swreg227;
	struct swreg228_s {
		u32 sw_enc_ssim_y_numerator_msb;
	} swreg228;
	struct swreg229_s {
		u32 sw_enc_ssim_u_numerator_lsb;
	} swreg229;
	struct swreg230_s {
		u32 sw_enc_ssim_u_numerator_msb;
	} swreg230;
	struct swreg231_s {
		u32 sw_enc_ssim_v_numerator_lsb;
	} swreg231;
	struct swreg232_s {
		u32 sw_enc_ssim_v_numerator_msb;
	} swreg232;
	struct swreg233_s {
		u32 sw_enc_ssim_y_denominator;
	} swreg233;
	struct swreg234_s {
		u32 sw_enc_ssim_uv_denominator;
	} swreg234;
	u32 regval;
} VCE_REGISTER;
#endif

#define VENC_NORMDRV_NAME "es_venc_drv"
#define VENC_DRV_NAME  "es_venc_vcmd_drv"

#endif /* !_VC8000_VCMD_DRIVER_H_ */
