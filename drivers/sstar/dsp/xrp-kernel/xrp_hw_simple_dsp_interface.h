/*
 * xrp_hw_simple_dsp_interface.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _XRP_KERNEL_SIMPLE_HW_DSP_INTERFACE_
#define _XRP_KERNEL_SIMPLE_HW_DSP_INTERFACE_

enum
{
    XRP_DSP_SYNC_IRQ_MODE_NONE  = 0x0,
    XRP_DSP_SYNC_IRQ_MODE_LEVEL = 0x1,
    XRP_DSP_SYNC_IRQ_MODE_EDGE  = 0x2,
};

struct xrp_hw_simple_sync_data
{
    __u32 device_mmio_base;
    __u32 host_irq_mode;
    __u32 host_irq_offset;
    __u32 host_irq_bit;
    __u32 device_irq_mode;
    __u32 device_irq_offset;
    __u32 device_irq_bit;
    __u32 device_irq;
};

#endif /*_XRP_KERNEL_SIMPLE_HW_DSP_INTERFACE_*/
