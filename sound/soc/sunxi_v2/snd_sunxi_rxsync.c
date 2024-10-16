/*
 * sound\soc\sunxi\snd_sunxi_rxsync.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * caiyongheng <caiyongheng@allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_rxsync.h"

#define HLOG		"RXSYNC"

#define RX_SYNC_DEV_MAX	8

typedef void (*enable_func)(void *data, bool enable);

typedef struct {
	void *data;
	/*enable_func route_enable;*/
	enable_func rx_enable;
} rx_sync_dev_t;

static struct {
	int total_count[RX_SYNC_DOMAIN_CNT];
	int enabled_count[RX_SYNC_DOMAIN_CNT];
	rx_sync_dev_t dev[RX_SYNC_DOMAIN_CNT][RX_SYNC_DEV_MAX];
	rx_sync_dev_t *last_enabled_dev[RX_SYNC_DOMAIN_CNT];
} rx_sync_data;

DEFINE_SPINLOCK(rx_sync_lock);

/* Return the rx_sync id. The id is unique in its own domain. */
int sunxi_rx_sync_probe(rx_sync_domain_t domain)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&rx_sync_lock, flags);
	rx_sync_data.total_count[domain]++;
	SND_LOG_DEBUG(HLOG, "domain=%d, total_count=%d\n",
		      domain, rx_sync_data.total_count[domain]);
	if (rx_sync_data.total_count[domain] > RX_SYNC_DEV_MAX) {
		SND_LOG_ERR(HLOG, "domain=%d, too many rx_sync devices "
			    "(current=%d, max=%d)\n", domain,
			    rx_sync_data.total_count[domain], RX_SYNC_DEV_MAX);
		ret = -EINVAL;
		goto unlock;
	}
	/* use total_count to define rx_sync id */
	ret = rx_sync_data.total_count[domain] - 1;
unlock:
	spin_unlock_irqrestore(&rx_sync_lock, flags);
	return ret;
}
EXPORT_SYMBOL(sunxi_rx_sync_probe);

void sunxi_rx_sync_remove(rx_sync_domain_t domain)
{
	unsigned long flags;

	spin_lock_irqsave(&rx_sync_lock, flags);
	rx_sync_data.total_count[domain]--;
	SND_LOG_DEBUG(HLOG, "domain=%d, total_count=%d\n",
		      domain, rx_sync_data.total_count[domain]);
	if (rx_sync_data.total_count[domain] < 0)
		rx_sync_data.total_count[domain] = 0;
	spin_unlock_irqrestore(&rx_sync_lock, flags);
}
EXPORT_SYMBOL(sunxi_rx_sync_remove);

void sunxi_rx_sync_startup(void *data, rx_sync_domain_t domain, int id,
			   void (*rx_enable)(void *data, bool enable))
{
	unsigned long flags;

	if (id < 0 || id >= RX_SYNC_DEV_MAX) {
		SND_LOG_ERR(HLOG, "unsupport rx sync id\n");
		return;
	}

	spin_lock_irqsave(&rx_sync_lock, flags);
	rx_sync_data.dev[domain][id].data = data;
	rx_sync_data.dev[domain][id].rx_enable = rx_enable;

	SND_LOG_DEBUG(HLOG, "domain=%d, id=%d\n", domain, id);
	spin_unlock_irqrestore(&rx_sync_lock, flags);
}
EXPORT_SYMBOL(sunxi_rx_sync_startup);

void sunxi_rx_sync_shutdown(rx_sync_domain_t domain, int id)
{
	unsigned long flags;
	int i;
	int enabled_count = 0;

	if (id < 0 || id >= RX_SYNC_DEV_MAX) {
		SND_LOG_ERR(HLOG, "unsupport rx sync id\n");
		return;
	}

	spin_lock_irqsave(&rx_sync_lock, flags);
	SND_LOG_DEBUG(HLOG, "domian=%d, id=%d, enabled_count=%d\n",
		      domain, id, rx_sync_data.enabled_count[domain]);

	/* ensure that only if all devices in all domains have been disabled,
	 * the rx_sync_data could be reset.
	 */
	for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++)
		enabled_count += rx_sync_data.enabled_count[i];
	if (enabled_count > 0)
		goto unlock;
	SND_LOG_DEBUG(HLOG, "domian=%d, id=%d, sum of enabled_count is %d, "
		      "reset rx_sync_data\n", domain, id, enabled_count);
	memset(rx_sync_data.dev, 0, sizeof(rx_sync_dev_t) * RX_SYNC_DOMAIN_CNT * RX_SYNC_DEV_MAX);
	for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++)
		rx_sync_data.last_enabled_dev[i] = NULL;

unlock:
	spin_unlock_irqrestore(&rx_sync_lock, flags);
}
EXPORT_SYMBOL(sunxi_rx_sync_shutdown);

void sunxi_rx_sync_control(rx_sync_domain_t domain, int id, bool enable)
{
	unsigned long flags;
	int i;
	int total_count = 0;
	int enabled_count = 0;

	SND_LOG_DEBUG(HLOG, "domain=%d, id=%d, enable=%d, enabled_count=%d\n",
		      domain, id, enable, rx_sync_data.enabled_count[domain]);
	if (id < 0 || id >= RX_SYNC_DEV_MAX) {
		SND_LOG_ERR(HLOG, "unsupport rx sync id\n");
		return;
	}

	spin_lock_irqsave(&rx_sync_lock, flags);
	if (enable) {
		rx_sync_data.enabled_count[domain]++;
		/* store the pointer of last enabled device. */
		if (rx_sync_data.enabled_count[domain] == \
		    rx_sync_data.total_count[domain]) {
			rx_sync_data.last_enabled_dev[domain] = \
				&rx_sync_data.dev[domain][id];
			SND_LOG_DEBUG(HLOG, "domain=%d, id=%d, "
				      "last_enabled_dev=0x%p\n", domain, id,
				      rx_sync_data.last_enabled_dev[domain]);
		} else if (rx_sync_data.enabled_count[domain] > \
			   rx_sync_data.total_count[domain]) {
			SND_LOG_ERR(HLOG, "domain=%d, enabled_count(%d) "
				    "is more than total_count(%d). "
				    "%s was called incorrectly?\n",
				    domain, rx_sync_data.enabled_count[domain],
				    rx_sync_data.total_count[domain], __func__);
			goto unlock;
		}

		/* Check all devices in all domains. Only if the last device in
		 * all domains runs here, we will run rx_enable(1) in only
		 * the last device in each domain.
		 */
		for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++) {
			total_count += rx_sync_data.total_count[i];
			enabled_count += rx_sync_data.enabled_count[i];
		}
		if (enabled_count < total_count) {
			goto unlock;
		} else if (enabled_count > total_count) {
			SND_LOG_ERR(HLOG, "sum of enabled_count(%d) is "
				    "more than total_count(%d). "
				    "%s was called incorrectly?\n",
				    enabled_count, total_count, __func__);
			goto unlock;
		}
		for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++) {
			rx_sync_dev_t *dev = rx_sync_data.last_enabled_dev[i];
			if (!dev)
				continue;
			SND_LOG_DEBUG(HLOG, "domain=%d, rx enable\n", i);
			dev->rx_enable(dev->data, 1);
		}
	} else {
		rx_sync_data.enabled_count[domain]--;

		if (rx_sync_data.enabled_count[domain] > 0) {
			goto unlock;
		} else if (rx_sync_data.enabled_count[domain] < 0) {
			SND_LOG_ERR(HLOG, "domain=%d, enabled_count(%d) is "
				    "less than 0. %s was called incorrectly?\n",
				    domain, rx_sync_data.enabled_count[domain],
				    __func__);
			goto unlock;
		}

		/* Check all devices in all domains. Only if the last device in
		 * all domains runs here, we will run rx_enable(0) in only
		 * the last device in each domain.
		 */
		for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++) {
			enabled_count += rx_sync_data.enabled_count[i];
		}
		if (enabled_count > 0) {
			goto unlock;
		} else if (enabled_count < 0) {
			SND_LOG_ERR(HLOG, "sum of enabled_count(%d) is "
				    "less than 0.%s was called incorrectly?\n",
				    enabled_count, __func__);
			goto unlock;
		}
		for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++) {
			rx_sync_dev_t *dev = rx_sync_data.last_enabled_dev[i];
			if (!dev)
				continue;
			SND_LOG_DEBUG(HLOG, "domain=%d, rx disable\n", i);
			dev->rx_enable(dev->data, 0);
		}
	}
unlock:
	spin_unlock_irqrestore(&rx_sync_lock, flags);
	return;
}
EXPORT_SYMBOL(sunxi_rx_sync_control);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi rx sync compontens");
