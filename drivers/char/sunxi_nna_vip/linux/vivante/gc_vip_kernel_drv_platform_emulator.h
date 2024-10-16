/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2022 Vivante Corporation
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
*    Copyright (C) 2017 - 2022 Vivante Corporation
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
*****************************************************************************/

#ifndef _GC_VIP_PLATFORM_CONFIG_H
#define _GC_VIP_PLATFORM_CONFIG_H

#include <gc_vip_common.h>

/* the core number of VIP.
   It's for multiple VIP hardware. Please set vpmdCORE_COUNT to 1 if is single core VIP
*/
#define  vpmdCORE_COUNT                  1

/* the logic devices number and core number for each logical device.
 * we need configure this after create nbg files.
 * The core count of logical device should be matched with core count in NBG file.
 * for example: LOGIC_DEVICES_WITH_CORE[8] = {2,2,4,0,0,0,0,0}.
 * indicate that the current driver supports 3 logical device.
 * device[0] and device[1] have 2 cores, device[2] have 4 cores.
 * the others devices not available, each device have different core index.
 * NOTE:
 * LOGIC_DEVICES_WITH_CORE[8] = {2,1,0,4,0,0,0,0}. indicate that driver supports 2 logical device.
 * device[0] is 2 cores, device[1] is 1 core.  the 4 is invalid config.
*/
static const vip_uint32_t LOGIC_DEVICES_WITH_CORE[vpmdCORE_COUNT] = {1};

/* default size 2M for system memory heap, access by CPU only */
/* NBinfo tool can get system memory size */
#define SYSTEM_MEMORY_HEAP_SIZE         (2 << 20)

/* NBinfo tool can get video memory size */
#define VIDEO_MEMORY_HEAP_SIZE          (208 << 20)

/* defalut base pyhsical address of video memory heap */
/* Verisilicon config the base address on insmod command line */
#define  VIDEO_MEMORY_HEAP_BASE_ADDRESS   0x39bfc5000000

/* AHB register base physical address for each core.
   NOTE: VSI replace these values are in the insmod command line parameter */
static const  vip_uint32_t AHB_REGISTER_BASE_ADDRESS[vpmdCORE_COUNT] = {0xCE000000};

/* The size of AHB register for each core. */
static const  vip_uint32_t AHB_REGISTER_SIZE[vpmdCORE_COUNT] = {0x00020000};

/* irq line number for each core. */
static const  vip_uint32_t IRQ_LINE_NUMBER[vpmdCORE_COUNT] = {301};

/* the size of VIP SRAM */
#define VIP_SRAM_SIZE               0x800000

/*
The base address of VIP SRAM.
VIP SRAM is a SRAM inside VIP core. It needs to be mapped to a 32 bits address space.
The VIP_SRAM_BASE_ADDRESS is a base address for mapping VIP SRAM, so the VIP SRAM has been mapped to
[VIP_SRAM_BASE_ADDRESS ~~ VIP_SRAM_BASE_ADDRESS + VIP_SRAM_SIZE] range.
After this mapping, VIP core access VIP SRAM through
VIP_SRAM_BASE_ADDRESS ~~ VIP_SRAM_BASE_ADDRESS+VIP_SRAM_SIZE address range.
Actually, The value of VIP_SRAM_BASE_ADDRESS may not be on DRAM.

How to config the VIP_SRAM_BASE_ADDRESS?
1. It can only be a 32bits value.
2. can't be config to zero.
3. It can't be configured to be close to 4Gbytes value.
   (VIP_SRAM_BASE_ADDRESS+vip_sram_size) can't be close to 4Gbytes.
4. It should be align to 64bytes.
5. can't be overlap with video memory range.

example:

The size of DRAM is 1G bytes. So DRAM memory address range is [0 ~~ 0x40000000].
In this case, you can set the value of vip_sram_address to 0x50000000. 0x50000000 is outside of DRAM.
0x50000000 is not a real physical address on SRAM.

If you need to specify address for VIP-SRAM, this value can be outside the range of video memory.
The address shoule be aligned to 64byte.

don't need configure VIP_SRAM_BASE_ADDRESS if set vpmdENABLE_MMU to 1.
Because driver will reserved memory space for VIP-SRAM in MMU page table.
We recomment that remap VIP-SRAM base address to 0x400000 when MMU is disabled.
*/
#define VIP_SRAM_BASE_ADDRESS       0x400000

/* the size of AXI SRAM, please set the value to zero if chip without AXI-SRAM.
   otherwise, follow chip real AXI-SRAM size setting.
*/
#define AXI_SRAM_SIZE               0x800000


/* the physical base address of AXI SRAM. The value of AXI_SRAM_BASE_ADDRESS is should be
   real address of AXI-SRAM on chip if the chip have AXI-SRAM.
*/
#define AXI_SRAM_BASE_ADDRESS       0x00

/* PCIE bar.
   NOTE: VSI replace these values are in the insmod command line parameter */
static const  vip_uint32_t PCI_BARS[vpmdCORE_COUNT] = {1};


#define VIV_PCI_VENDOR_ID           0x1d9b
#define VIV_PCI_DEVICE_ID           0xface

#define PCI_BAR_0                   0x0
#define PCI_BAR_2                   0x2
#define PCI_BAR_4                   0x4

#define IATU_TYPE_INBOUND                   (0x80000000UL)
#define IATU_TYPE_OUTBOUND                  (0x00000000UL)
#define IATU_BOUND_INDEX(x)                 (x)
#define IATU_REGION_ENABLE                  (0x80000000UL)

/* ATU_CAP registers */
#define IATU_BASE                               0x8100
#define IATU_REGION_CTRL_1_OFF_INBOUND(i)       (IATU_BASE + 0x200 * (i) + 0x00)
#define IATU_REGION_CTRL_2_OFF_INBOUND(i)       (IATU_BASE + 0x200 * (i) + 0x04)
#define IATU_LWR_BASE_ADDR_OFF_INBOUND(i)       (IATU_BASE + 0x200 * (i) + 0x08)
#define IATU_UPPER_BASE_ADDR_OFF_INBOUND(i)     (IATU_BASE + 0x200 * (i) + 0x0c)
#define IATU_LIMIT_ADDR_OFF_INBOUND(i)          (IATU_BASE + 0x200 * (i) + 0x10)
#define IATU_LWR_TARGET_ADDR_OFF_INBOUND(i)     (IATU_BASE + 0x200 * (i) + 0x14)
#define IATU_UPPER_TARGET_ADDR_OFF_INBOUND(i)   (IATU_BASE + 0x200 * (i) + 0x18)

#define BAR_PCIE_MAP_SIZE           0x00080000
#define IPS_H_ADDR                  0x00000001

/* PCIe AXi master view two silics DDR address */
#define AXI_DDR_BASE                0x00000000ULL
#define AXI_DDR0_L_OFFSET           0x04000000
#define AXI_DDR0_H_OFFSET           0x00000000
#define AXI_DDR1_L_OFFSET           0x84000000
#define AXI_DDR1_H_OFFSET           0x00000000
/* PCIe AXI master view pcie, addr and other ips controller offset */
#define AXI_CFG_BASE                0x100000000ULL
#define PCIE_CCM_DDR_NOC_CON_OFFSET 0x00000000
#define DDR0_CON_OFFSET             0x02000000
#define DDR1_CON_OFFSET             0x04000000
#define THS_CON_OFFSET              0x06000000

#define S0_DDR_SIZE                 0x80000000
#define S1_DDR_SIZE                 0x80000000
#define S0_DDR_TCACHE_SIZE          0x04000000
#define S1_DDR_TCACHE_SIZE          0x04000000
#define S0_DDR_RESERVE_SIZE         S0_DDR_TCACHE_SIZE
#define S0_DDR_BAR_SIZE             0x10000000

/* pcie, ccm and ddr_noc size */
#define PCIE_CCM_DDR_NOC_SIZE       0x01100000
/* ddr_a and ddr_b size */
#define DDR0_PHY_CON_SIZE           0x02000000
/* ddr_a and ddr_b size */
#define DDR1_PHY_CON_SIZE           0x02000000
/* ths and sys con size */
#define THS_CON_SIZE                0x00e00000
#define THS0_CON_AND_RESERVE_SIZE   0x00400000
#define CCM_OFFSET                  0x00400000
#define CCM_MAP_SIZE                0x00030000

/* global int flag */
#define INT_FLAG_EDMA               (1 << 0)
#define INT_FLAG_HW_MONITOR         (1 << 2)
#define INT_FLAG_THS0_VCD_A         (1 << 7)
#define INT_FLAG_THS0_VCD_B         (1 << 9)
#define INT_FLAG_THS1_VCD_A         (1 << 8)
#define INT_FLAG_THS1_VCD_B         (1 << 10)
#define INT_FLAG_THS0_VCE           (1 << 11)
#define INT_FLAG_THS1_VCE           (1 << 12)
#define INT_FLAG_THS0_BIGSEA        (1 << 15)
#define INT_FLAG_THS1_BIGSEA        (1 << 16)
#define GLUE_LGIC_INTE              (\
        INT_FLAG_HW_MONITOR | INT_FLAG_THS0_VCD_A | INT_FLAG_THS0_VCD_B |\
        INT_FLAG_THS1_VCD_A | INT_FLAG_THS1_VCD_B | INT_FLAG_THS0_VCE |\
        INT_FLAG_THS1_VCE | INT_FLAG_THS0_BIGSEA | INT_FLAG_THS1_BIGSEA)

#define MAIL_BOX_INTERRUPT_CONTROLLER   0x00020300
/* pcie glue logic registers */
#define INT_POL_REF_CFG             0x000200ac

#define DWORD_LO(a) ((unsigned long long)(a) & 0xffffffff)
#define DWORD_HI(a) ((unsigned long long)(a) >> 32)

#define FB_MEM_RESERVE_SIZE     (0)
#define EDMA_LINK_TABLE_RESERVE_SIZE (8*1024*1024)

typedef void __iomem *      iomem_t;

#define write_mreg32(_base, _reg, _val) writel((_val), ( volatile iomem_t)(_base + _reg))
#define read_mreg32(_base, _reg) readl(( volatile iomem_t)(_base + _reg))



#endif
