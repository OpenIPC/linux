/*
 * sstar_rpmsg.h- Sigmastar
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

#ifndef _SSTAR_RPMSG_H_
#define _SSTAR_RPMSG_H_

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * These constants need to be sync with rpmsg_device_type_t
 * in linux/rpmsg.h
 */
#define RPMSG_MODE_RC_PORT 0
#define RPMSG_MODE_EP_PORT 1
#define RPMSG_MODE_SHM     2
#define RPMSG_MODE_CA7     3
#define RPMSG_MODE_UNKNOWN 255

#define EPT_TYPE_SIGMASTAR   0x0
#define EPT_TYPE_CUSTOMER    0x1
#define EPT_TYPE(x)          ((x & 0x1) << 30)
#define EPT_ADDR_MACRO(t, c) (EPT_TYPE(t) | (c & 0x3fffffff))

#define RPMSG_QUERY_STAGE 0x1
#define RPMSG_STAGE_RTOS  0x1
#define RPMSG_STAGE_LINUX 0x2
#define RPMSG_STAGE_APP   0x3

/**
 * wait for App on slave to call SS_RPMSG_BOOTUP_DONE_IOCTL
 */
#define RPMSG_BOOTUP_FLAG_WAIT_APP 0x1

/**
 * struct ss_rpmsg_endpoint_info - endpoint info representation
 *
 * @name: name of service
 * @src: local address
 * @dst: destination address
 * @id: RPMsg Endpoint device node id (/dev/rpmsgX, id is the value of X).
 * @mode: mode of RPMsg bus that the target device connected on
 * @target_id: target id
 */
struct ss_rpmsg_endpoint_info
{
    char  name[32];
    __u32 src;
    __u32 dst;
    __u32 id;

    __u32 mode;
    __u16 target_id;
};

/**
 * struct ss_rpmsg_devices_info - connected devices info
 *
 * @mode: mode of RPMsg bus that the target device connected on
 * @count: the max number of target_id that buffer could store, it will become
 *         current number of connected devices after ioctl return.
 * @buffer: start address of buffer
 */
struct ss_rpmsg_devices_info
{
    __u32 mode;
    __u32 count;
    __u64 buffer;
};

/**
 * struct ss_rpmsg_query - query info of specified device
 *
 * @mode: mode of RPMsg bus that the target device connected on
 * @target_id: target id
 * @param: paramter of query
 * @value: return value
 */
struct ss_rpmsg_query_info
{
    __u32 mode;
    __u16 target_id;
    __u32 param;
    __u64 value;
};

/**
 * struct ss_rpmsg_load_image - load boot image into slave device
 *
 * @mode: mode of RPMsg bus that the target device connected on
 * @target_id: target id
 * @name: image name
 * @reserved: reserved element
 * @addr_phys: physical address of image content buffer
 * @load_addr_phys: load address on slave device
 * @size: size of image
 */
struct ss_rpmsg_image_info
{
    __u32 mode;
    __u16 target_id;
    char  name[32];
    __u64 reserved;
    __u64 addr_phys;
    __u64 load_addr_phys;
    __u64 size;
};

/**
 * struct ss_rpmsg_bootup - bootup slave device
 *
 * @mode: mode of RPMsg bus that the target device connected on
 * @target_id: target id
 * @name: image name
 * @flags: bit array of bootup flags as defined by RPMSG_BOOTUP_FLAG_*
 * @entry: bootup entry address on slave device
 * @bootargs: bootup arguments (\0 terminated string)
 */
struct ss_rpmsg_bootup_info
{
    __u32 mode;
    __u16 target_id;
    char  name[32];
    __u32 flags;
    __u64 entry;
    __u64 bootargs_addr_phys;
    __u64 bootargs_load_addr_phys;
    __u64 bootargs_size;
};

/**
 * struct ss_rpmsg_bootup_done - notify RC side that slave device is ready
 *
 * @mode: mode of RPMsg bus that the target device connected on
 * @target_id: target id
 */
struct ss_rpmsg_bootup_done
{
    __u32 mode;
    __u16 target_id;
};

#define SS_RPMSG_CREATE_EPT_IOCTL   _IOWR(0xb6, 0x1, struct ss_rpmsg_endpoint_info)
#define SS_RPMSG_DESTROY_EPT_IOCTL  _IO(0xb6, 0x2)
#define SS_RPMSG_DEVICES_INFO_IOCTL _IOWR(0xb6, 0x3, struct ss_rpmsg_devices_info)
#define SS_RPMSG_QUERY_IOCTL        _IOWR(0xb6, 0x4, struct ss_rpmsg_query_info)
#define SS_RPMSG_LOAD_IMAGE_IOCTL   _IOW(0xb6, 0x5, struct ss_rpmsg_image_info)
#define SS_RPMSG_BOOTUP_IOCTL       _IOW(0xb6, 0x6, struct ss_rpmsg_bootup_info)
#define SS_RPMSG_BOOTUP_DONE_IOCTL  _IOW(0xb6, 0x7, struct ss_rpmsg_bootup_done)

#endif /* _SSTAR_RPMSG_H_ */
