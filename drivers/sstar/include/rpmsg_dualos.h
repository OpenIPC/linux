/*
 * rpmsg_dualos.h- Sigmastar
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

/*
 * @file rpmsg_dualos.h
 *
 * @brief Global header file for sstar rpmsg_dualos
 *
 * @ingroup rpmsg_dualos
 */
#ifndef _RPMSG_DUALOS_H_
#define _RPMSG_DUALOS_H_
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <linux/rpmsg.h>

#include "rpmsg_dualos_common.h"

struct dualos_rpmsg_node
{
    struct rpmsg_endpoint *ept;

    spinlock_t          queue_lock;
    struct sk_buff_head queue;
    wait_queue_head_t   wq;

    void *private;
};

struct dualos_rpmsg_node *dualos_rpmsg_node_alloc(rpmsg_device_type_t type, int dev_id, const char *name, int addr);

void dualos_rpmsg_node_release(struct dualos_rpmsg_node *dnode);

struct sk_buff *dualos_rpmsg_recv(struct dualos_rpmsg_node *dnode, unsigned char **data, unsigned int *src,
                                  signed long timeout);

int dualos_rpmsg_sendto(struct dualos_rpmsg_node *dnode, int dst, char *buffer, int size, int timeout);

// low level RPMSG APIs
int                    dualos_rpmsg_wait_remote_device(rpmsg_device_type_t type, int dev_id, signed long timeout);
struct rpmsg_endpoint *dualos_rpmsg_create_ept(rpmsg_device_type_t type, int dev_id, rpmsg_rx_cb_t cb, void *priv,
                                               struct rpmsg_channel_info chinfo);
int                    dualos_rpmsg_destroy_ept(struct rpmsg_endpoint *ept);
void                   dualos_rpmsg_remote_adaptor_online(rpmsg_device_type_t type, int dev_id);

void *RPMsgDualosCreateEptdev(rpmsg_device_type_t type, int dev_id, rpmsg_rx_cb_t cb, void *priv,
                              struct rpmsg_channel_info chinfo);
void  RPMsgDualosDestroyEptdev(void *handle);
int   RPMsgDualosEptdevSendto(void *handle, void *data, int len, u32 dst);
int   RPMsgDualosEptdevTrySendto(void *handle, void *data, int len, u32 dst);

#endif /* _RPMSG_DUALOS_H_ */
