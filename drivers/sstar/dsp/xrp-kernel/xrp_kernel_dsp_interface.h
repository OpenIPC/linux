/*
 * xrp_kernel_dsp_interface.h- Sigmastar
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

#ifndef _XRP_KERNEL_DSP_INTERFACE_H_
#define _XRP_KERNEL_DSP_INTERFACE_H_

#ifndef XRP_DSP_COMM_BASE_MAGIC
#define XRP_DSP_COMM_BASE_MAGIC 0x20161006
#endif

enum
{
    XRP_DSP_SYNC_IDLE         = 0,
    XRP_DSP_SYNC_HOST_TO_DSP  = 0x1,
    XRP_DSP_SYNC_DSP_TO_HOST  = 0x3,
    XRP_DSP_SYNC_START        = 0x101,
    XRP_DSP_SYNC_DSP_READY_V1 = 0x203,
    XRP_DSP_SYNC_DSP_READY_V2 = 0x303,
};

enum
{
    XRP_DSP_SYNC_TYPE_ACCEPT = 0x80000000,
    XRP_DSP_SYNC_TYPE_MASK   = 0x00ffffff,

    XRP_DSP_SYNC_TYPE_LAST         = 0,
    XRP_DSP_SYNC_TYPE_HW_SPEC_DATA = 1,
    XRP_DSP_SYNC_TYPE_HW_QUEUES    = 2,
};

struct xrp_dsp_tlv
{
    __u32 type;
    __u32 length;
    __u32 value[0];
};

struct xrp_dsp_sync_v1
{
    __u32 sync;
    __u32 hw_sync_data[0];
};

struct xrp_dsp_sync_v2
{
    __u32              sync;
    __u32              reserved[3];
    struct xrp_dsp_tlv hw_sync_data[0];
};

enum
{
    XRP_DSP_BUFFER_FLAG_READ  = 0x1,
    XRP_DSP_BUFFER_FLAG_WRITE = 0x2,
};

struct xrp_dsp_buffer
{
    /*
     * When submitted to DSP: types of access allowed
     * When returned to host: actual access performed
     */
    __u32 flags;
    __u32 size;
    __u32 addr;
};

enum
{
    XRP_DSP_CMD_FLAG_REQUEST_VALID          = 0x00000001,
    XRP_DSP_CMD_FLAG_RESPONSE_VALID         = 0x00000002,
    XRP_DSP_CMD_FLAG_REQUEST_NSID           = 0x00000004,
    XRP_DSP_CMD_FLAG_RESPONSE_DELIVERY_FAIL = 0x00000008,
};

#define XRP_DSP_CMD_INLINE_DATA_SIZE    16
#define XRP_DSP_CMD_INLINE_BUFFER_COUNT 1
#define XRP_DSP_CMD_NAMESPACE_ID_SIZE   16
#define XRP_DSP_CMD_STRIDE              128

struct xrp_dsp_cmd
{
    __u32 flags;
    __u32 in_data_size;
    __u32 out_data_size;
    __u32 buffer_size;
    union
    {
        __u32 in_data_addr;
        __u8  in_data[XRP_DSP_CMD_INLINE_DATA_SIZE];
    };
    union
    {
        __u32 out_data_addr;
        __u8  out_data[XRP_DSP_CMD_INLINE_DATA_SIZE];
    };
    union
    {
        __u32                 buffer_addr;
        struct xrp_dsp_buffer buffer_data[XRP_DSP_CMD_INLINE_BUFFER_COUNT];
        __u8                  buffer_alignment[XRP_DSP_CMD_INLINE_DATA_SIZE];
    };
    __u8 nsid[XRP_DSP_CMD_NAMESPACE_ID_SIZE];
};

#endif /*_XRP_KERNEL_DSP_INTERFACE_H_*/
