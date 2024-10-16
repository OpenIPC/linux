/*
 * xr806/up_cmd.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * usrer protocol flow implementation for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include "os_intf.h"
#include "xradio.h"
#include "up_cmd.h"
#include "txrx.h"
#include "debug.h"

struct up_cmd_priv {
	struct sock *nlsk;
	void *dev_priv;
	u32 user_pid;
};

extern struct net init_net;

static struct up_cmd_priv cmd_priv = {
	.nlsk = NULL,
	.user_pid = 0,
};

#define XR_WLAN_NETLINK_ID 28

static void xradio_netlink_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;

	if (skb->len >= nlmsg_total_size(0)) {
		nlh = nlmsg_hdr(skb);

		cmd_priv.user_pid = nlh->nlmsg_pid;
		// TODO: User give pid for tx.

		if (nlh) {
			xradio_tx_cmd_process(cmd_priv.dev_priv, NLMSG_DATA(nlh),
					      nlh->nlmsg_len - NLMSG_HDRLEN);
		}
	}
}

struct netlink_kernel_cfg xradio_up_cmd = {
	.input = xradio_netlink_recv_msg,
};

int xradio_up_cmd_init(void *priv)
{
	memset(&cmd_priv, 0, sizeof(struct up_cmd_priv));
	cmd_priv.nlsk =
		(struct sock *)netlink_kernel_create(&init_net, XR_WLAN_NETLINK_ID, &xradio_up_cmd);

	if (!cmd_priv.nlsk)
		return -ENOMEM;

	cmd_priv.dev_priv = priv;

	return 0;
}

int xradio_up_cmd_deinit(void *priv)
{
	if (cmd_priv.nlsk) {
		netlink_kernel_release(cmd_priv.nlsk);
		cmd_priv.nlsk = NULL;
		cmd_priv.user_pid = 0;
	}

	return 0;
}

int xradio_up_cmd_push(u8 *buff, u32 len)
{
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *skb = NULL;

	// data_hex_dump(__func__,20,buff,len);

	if (!cmd_priv.nlsk || !cmd_priv.user_pid)
		return -EINVAL;

	skb = nlmsg_new(len, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;

	nlh = nlmsg_put(skb, 0, 0, XR_WLAN_NETLINK_ID, len, 0);

	memcpy(NLMSG_DATA(nlh), buff, len);

	if (netlink_unicast(cmd_priv.nlsk, skb, cmd_priv.user_pid, MSG_DONTWAIT) < 0)
		cfg_printk(XRADIO_DBG_ERROR, "Error while sending back to user\n");

	return 0;
}
