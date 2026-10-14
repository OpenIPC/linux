// SPDX-License-Identifier: GPL-2.0
// Copyright (c) LOTUS. All rights reserved.

#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/ethtool.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>

#include "ftgmac030.h"

enum { NETDEV_STATS, E1000_STATS };

static const char ftgmac030_gstrings_test[][ETH_GSTRING_LEN] = {
	"Loopback test  (offline)", "Link test   (on/offline)"
};

#define FTGMAC030_TEST_LEN ARRAY_SIZE(ftgmac030_gstrings_test)

static u32 ftgmac030_get_msglevel(struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	return ctrl->msg_enable;
}

static void ftgmac030_set_msglevel(struct net_device *netdev, u32 data)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	ctrl->msg_enable = data;
}

static int ftgmac030_get_regs_len(struct net_device __always_unused *netdev)
{
#define FTGMAC030_REGS_LEN 32
	return FTGMAC030_REGS_LEN * sizeof(u32);
}

extern void ftgmac030_dump(struct ftgmac030_ctrl *ctrl);
static void ftgmac030_get_regs(struct net_device *netdev,
			       struct ethtool_regs *regs, void *p)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	int i;
	u32 *regs_buff = p;

	pm_runtime_get_sync(netdev->dev.parent);
	memset(p, 0, FTGMAC030_REGS_LEN * sizeof(u32));

	for (i = 0; i < FTGMAC030_REGS_LEN; i++)
		regs_buff[i] = FTGMAC030_READ_REG_ARRAY(ctrl, 0, i);

	ftgmac030_dump(ctrl);

	pm_runtime_put_sync(netdev->dev.parent);
}

static void ftgmac030_get_drvinfo(struct net_device *netdev,
				  struct ethtool_drvinfo *drvinfo)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	strlcpy(drvinfo->driver, ftgmac030_driver_name, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, ftgmac030_driver_version,
		sizeof(drvinfo->version));

	strlcpy(drvinfo->bus_info, dev_name(&ctrl->pdev->dev),
		sizeof(drvinfo->bus_info));
	drvinfo->regdump_len = ftgmac030_get_regs_len(netdev);
	drvinfo->eedump_len = 0;
}

static void ftgmac030_get_ringparam(struct net_device *netdev,
				    struct ethtool_ringparam *ring)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	ring->rx_max_pending = FTGMAC030_MAX_RXD;
	ring->tx_max_pending = FTGMAC030_MAX_TXD;
	ring->rx_pending = ctrl->rx_ring_count;
	ring->tx_pending = ctrl->tx_ring_count;
}

static int ftgmac030_set_ringparam(struct net_device *netdev,
				   struct ethtool_ringparam *ring)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct ftgmac030_ring *temp_tx = NULL, *temp_rx = NULL;
	int err = 0, size = sizeof(struct ftgmac030_ring);
	bool set_tx = false, set_rx = false;
	u16 new_rx_count, new_tx_count;

	if ((ring->rx_mini_pending) || (ring->rx_jumbo_pending))
		return -EINVAL;

	new_rx_count = clamp_t(u32, ring->rx_pending, FTGMAC030_MIN_RXD,
			       FTGMAC030_MAX_RXD);
	new_rx_count = ALIGN(new_rx_count, REQ_RX_DESCRIPTOR_MULTIPLE);

	new_tx_count = clamp_t(u32, ring->tx_pending, FTGMAC030_MIN_TXD,
			       FTGMAC030_MAX_TXD);
	new_tx_count = ALIGN(new_tx_count, REQ_TX_DESCRIPTOR_MULTIPLE);

	if ((new_tx_count == ctrl->tx_ring_count) &&
	    (new_rx_count == ctrl->rx_ring_count))
		return 0;

	while (test_and_set_bit(__FTGMAC030_RESETTING, &ctrl->state))
		usleep_range(1000, 2000);

	if (!netif_running(ctrl->netdev)) {
		// Set counts now and allocate resources during open()
		ctrl->tx_ring->count = new_tx_count;
		ctrl->rx_ring->count = new_rx_count;
		ctrl->tx_ring_count = new_tx_count;
		ctrl->rx_ring_count = new_rx_count;
		goto clear_reset;
	}

	set_tx = (new_tx_count != ctrl->tx_ring_count);
	set_rx = (new_rx_count != ctrl->rx_ring_count);

	// Allocate temporary storage for ring updates
	if (set_tx) {
		temp_tx = vmalloc(size);
		if (!temp_tx) {
			err = -ENOMEM;
			goto free_temp;
		}
	}
	if (set_rx) {
		temp_rx = vmalloc(size);
		if (!temp_rx) {
			err = -ENOMEM;
			goto free_temp;
		}
	}

	pm_runtime_get_sync(netdev->dev.parent);

	ftgmac030_down(ctrl, true);

	if (set_tx) {
		memcpy(temp_tx, ctrl->tx_ring, size);
		temp_tx->count = new_tx_count;
		err = ftgmac030_setup_tx_resources(temp_tx);
		if (err)
			goto err_setup;
	}
	if (set_rx) {
		memcpy(temp_rx, ctrl->rx_ring, size);
		temp_rx->count = new_rx_count;
		err = ftgmac030_setup_rx_resources(temp_rx);
		if (err)
			goto err_setup_rx;
	}

	// ...then free the old resources and copy back any new ring data
	if (set_tx) {
		ftgmac030_free_tx_resources(ctrl->tx_ring);
		memcpy(ctrl->tx_ring, temp_tx, size);
		ctrl->tx_ring_count = new_tx_count;
	}
	if (set_rx) {
		ftgmac030_free_rx_resources(ctrl->rx_ring);
		memcpy(ctrl->rx_ring, temp_rx, size);
		ctrl->rx_ring_count = new_rx_count;
	}

err_setup_rx:
	if (err && set_tx)
		ftgmac030_free_tx_resources(temp_tx);
err_setup:
	ftgmac030_up(ctrl);
	pm_runtime_put_sync(netdev->dev.parent);
free_temp:
	vfree(temp_tx);
	vfree(temp_rx);
clear_reset:
	clear_bit(__FTGMAC030_RESETTING, &ctrl->state);
	return err;
}


static void ftgmac030_free_desc_rings(struct ftgmac030_ctrl *ctrl)
{
	struct ftgmac030_ring *tx_ring = &ctrl->test_tx_ring;
	struct ftgmac030_ring *rx_ring = &ctrl->test_rx_ring;
	struct platform_device *pdev = ctrl->pdev;
	struct ftgmac030_buffer *buffer_info;
	int i;

	if (tx_ring->desc && tx_ring->buffer_info) {
		for (i = 0; i < tx_ring->count; i++) {
			buffer_info = &tx_ring->buffer_info[i];

			if (buffer_info->dma)
				dma_unmap_single(&pdev->dev,
						 buffer_info->dma,
						 buffer_info->length,
						 DMA_TO_DEVICE);
			if (buffer_info->skb)
				dev_kfree_skb(buffer_info->skb);
		}
	}

	if (rx_ring->desc && rx_ring->buffer_info) {
		for (i = 0; i < rx_ring->count; i++) {
			buffer_info = &rx_ring->buffer_info[i];

			if (buffer_info->dma)
				dma_unmap_single(&pdev->dev,
						 buffer_info->dma,
						 2048, DMA_FROM_DEVICE);
			if (buffer_info->skb)
				dev_kfree_skb(buffer_info->skb);
		}
	}

	if (tx_ring->desc) {
		dma_free_coherent(&pdev->dev, tx_ring->size, tx_ring->desc,
				  tx_ring->dma);
		tx_ring->desc = NULL;
	}
	if (rx_ring->desc) {
		dma_free_coherent(&pdev->dev, rx_ring->size, rx_ring->desc,
				  rx_ring->dma);
		rx_ring->desc = NULL;
	}

	kfree(tx_ring->buffer_info);
	tx_ring->buffer_info = NULL;
	kfree(rx_ring->buffer_info);
	rx_ring->buffer_info = NULL;
}

static int ftgmac030_setup_desc_rings(struct ftgmac030_ctrl *ctrl)
{
	struct ftgmac030_ring *tx_ring = &ctrl->test_tx_ring;
	struct ftgmac030_ring *rx_ring = &ctrl->test_rx_ring;
	struct ftgmac030_txdes *tx_desc;
	struct ftgmac030_rxdes *rx_desc = NULL;
	struct platform_device *pdev = ctrl->pdev;
	int i;
	int ret_val;

	/* Setup Tx descriptor ring and Tx buffers */

	if (!tx_ring->count)
		tx_ring->count = FTGMAC030_DEFAULT_TXD;

	tx_ring->buffer_info = kcalloc(tx_ring->count,
				       sizeof(struct ftgmac030_buffer), GFP_KERNEL);
	if (!tx_ring->buffer_info) {
		ret_val = 1;
		goto err_nomem;
	}

	tx_ring->size = tx_ring->count * sizeof(struct ftgmac030_txdes);
	tx_ring->size = ALIGN(tx_ring->size, 4096);
	tx_ring->desc = dma_alloc_coherent(&pdev->dev, tx_ring->size,
					   &tx_ring->dma, GFP_KERNEL);
	if (!tx_ring->desc) {
		ret_val = 2;
		goto err_nomem;
	}
	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;

	iow32(FTGMAC030_REG_NPTXR_BADR, (tx_ring->dma & DMA_BIT_MASK(32)));

	for (i = 0; i < tx_ring->count; i++) {
		struct sk_buff *skb;
		unsigned int skb_size = 1024;

		skb = alloc_skb(skb_size, GFP_KERNEL);
		if (!skb) {
			ret_val = 3;
			goto err_nomem;
		}
		skb_put(skb, skb_size);
		tx_ring->buffer_info[i].skb = skb;
		tx_ring->buffer_info[i].length = skb->len;
		tx_ring->buffer_info[i].dma =
		    dma_map_single(&pdev->dev, skb->data, skb->len,
				   DMA_TO_DEVICE);
		if (dma_mapping_error(&pdev->dev,
				      tx_ring->buffer_info[i].dma)) {
			ret_val = 4;
			goto err_nomem;
		}
	}
	tx_desc = FTGMAC030_TX_DESC(*tx_ring, (tx_ring->count - 1));
	tx_desc->txdes0 = FTGMAC030_TXDES0_EDOTR;

	/* Setup Rx descriptor ring and Rx buffers */

	if (!rx_ring->count)
		rx_ring->count = FTGMAC030_DEFAULT_RXD;

	rx_ring->buffer_info = kcalloc(rx_ring->count,
				       sizeof(struct ftgmac030_buffer), GFP_KERNEL);
	if (!rx_ring->buffer_info) {
		ret_val = 5;
		goto err_nomem;
	}

	rx_ring->size = rx_ring->count * sizeof(struct ftgmac030_rxdes);
	rx_ring->desc = dma_alloc_coherent(&pdev->dev, rx_ring->size,
					   &rx_ring->dma, GFP_KERNEL);
	if (!rx_ring->desc) {
		ret_val = 6;
		goto err_nomem;
	}
	rx_ring->next_to_use = 0;
	rx_ring->next_to_clean = 0;

	iow32(FTGMAC030_REG_RXR_BADR, (rx_ring->dma & DMA_BIT_MASK(32)));

	for (i = 0; i < rx_ring->count; i++) {
		struct ftgmac030_rxdes *rx_desc;
		struct sk_buff *skb;

		skb = alloc_skb(2048 + NET_IP_ALIGN, GFP_KERNEL);
		if (!skb) {
			ret_val = 7;
			goto err_nomem;
		}
		skb_reserve(skb, NET_IP_ALIGN);
		rx_ring->buffer_info[i].skb = skb;
		rx_ring->buffer_info[i].dma =
		    dma_map_single(&pdev->dev, skb->data, 2048,
				   DMA_FROM_DEVICE);
		if (dma_mapping_error(&pdev->dev,
				      rx_ring->buffer_info[i].dma)) {
			ret_val = 8;
			goto err_nomem;
		}
		rx_desc = FTGMAC030_RX_DESC(*rx_ring, i);
		rx_desc->rxdes0 = 0;
		rx_desc->rxdes1 = 0;
		rx_desc->rxdes3 = cpu_to_le32(rx_ring->buffer_info[i].dma);
		memset(skb->data, 0x00, skb->len);
	}
	rx_desc->rxdes0 = FTGMAC030_RXDES0_EDORR;

	iow32(FTGMAC030_REG_APTC, FTGMAC030_APTC_RXPOLL_CNT(1));

	return 0;

err_nomem:
	ftgmac030_free_desc_rings(ctrl);
	return ret_val;
}

static void ftgmac030_create_lbtest_frame(struct sk_buff *skb,
					  unsigned int frame_size)
{
	unsigned int set_size = 1;

	memset(skb->data, 0xFF, frame_size);
	frame_size &= ~1;
	memset(&skb->data[frame_size / 2], 0xAA, frame_size / 2 - 1);
	memset(&skb->data[frame_size / 2 + 10], 0xBE, set_size);
	memset(&skb->data[frame_size / 2 + 12], 0xAF, set_size);
}

static int ftgmac030_check_lbtest_frame(struct sk_buff *skb,
					unsigned int frame_size)
{
	frame_size &= ~1;
	if (*(skb->data + 3) == 0xFF)
		if ((*(skb->data + frame_size / 2 + 10) == 0xBE) &&
		    (*(skb->data + frame_size / 2 + 12) == 0xAF))
			return 0;
	return 13;
}

static int ftgmac030_run_loopback_test(struct ftgmac030_ctrl *ctrl)
{
	struct ftgmac030_ring *tx_ring = &ctrl->test_tx_ring;
	struct ftgmac030_ring *rx_ring = &ctrl->test_rx_ring;
	struct platform_device *pdev = ctrl->pdev;
	struct ftgmac030_txdes *tx_desc;
	struct ftgmac030_buffer *buffer_info;
	int i, j, k, l;
	int lc;
	int good_cnt;
	int ret_val = 0;
	unsigned long time;

	if (rx_ring->count <= tx_ring->count)
		lc = ((tx_ring->count / 64) * 2) + 1;
	else
		lc = ((rx_ring->count / 64) * 2) + 1;

	k = 0;
	l = 0;
	// loop count loop
	for (j = 0; j <= lc; j++) {
		// Send the packets
		for (i = 0; i < 64; i++) {
			tx_desc = FTGMAC030_TX_DESC(*tx_ring, k);
			buffer_info = &tx_ring->buffer_info[k];

			ftgmac030_create_lbtest_frame(buffer_info->skb, 1024);
			dma_sync_single_for_device(&pdev->dev,
						   buffer_info->dma,
						   buffer_info->length,
						   DMA_TO_DEVICE);

			tx_desc->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_TXDMA_OWN
						       | FTGMAC030_TXDES0_TXBUF_SIZE(
							buffer_info->length));
			if (i == 0)
				tx_desc->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_FTS);

			tx_desc->txdes3 = cpu_to_le32(buffer_info->dma);
			k++;
			if (k == tx_ring->count)
				k = 0;
		}
		tx_desc->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_LTS);

		iow32(FTGMAC030_REG_NPTXPD, k);

		msleep(200);
		time = jiffies;	// Set the start time for the receive
		good_cnt = 0;
		// Receive the sent packets
		do {
			buffer_info = &rx_ring->buffer_info[l];

			dma_sync_single_for_cpu(&pdev->dev,
						buffer_info->dma, 2048,
						DMA_FROM_DEVICE);

			ret_val = ftgmac030_check_lbtest_frame(buffer_info->skb,
							       1024);
			if (!ret_val)
				good_cnt++;
			l++;
			if (l == rx_ring->count)
				l = 0;
		} while ((good_cnt < 64) && !time_after(jiffies, time + 20));
		if (good_cnt != 64) {
			ret_val = 13;	// ret_val is the same as mis-compare
			break;
		}
		if (time_after(jiffies, time + 20)) {
			ret_val = 14;	// error code for time out error
			break;
		}
	}
	return ret_val;
}

static int ftgmac030_loopback_test(struct ftgmac030_ctrl *ctrl, u64 *data)
{
	u32 tmp, maccr;

	tmp = ior32(FTGMAC030_REG_MACCR);
	maccr = (tmp | FTGMAC030_MACCR_TXDMA_EN | FTGMAC030_MACCR_RXDMA_EN
		 | FTGMAC030_MACCR_TXMAC_EN | FTGMAC030_MACCR_RXMAC_EN);

	// External loopback
	maccr &= ~FTGMAC030_MACCR_LOOP_EN;

	*data = ftgmac030_setup_desc_rings(ctrl);
	if (*data)
		goto out;

	iow32(FTGMAC030_REG_MACCR, maccr);

	*data = ftgmac030_run_loopback_test(ctrl);

	iow32(FTGMAC030_REG_MACCR, tmp);

	ftgmac030_free_desc_rings(ctrl);
out:
	return *data;
}

static int ftgmac030_link_test(struct ftgmac030_ctrl *ctrl, u64 *data)
{
	struct phy_device *phydev = ctrl->phy_dev;
	int err;

	*data = 0;

	err = phy_start_aneg(phydev);
	if (err < 0) {
		*data = 1;
		return *data;
	}

	// On some Phy/switch combinations, link establishment
	// can take a few seconds more than expected.
	msleep_interruptible(5000);

	err = phy_read_status(ctrl->phy_dev);
	if (err || !ctrl->phy_dev->link)
		*data = 1;

	return *data;
}

static int ftgmac030_get_sset_count(struct net_device __always_unused *netdev,
				    int sset)
{
	switch (sset) {
	case ETH_SS_TEST:
		return FTGMAC030_TEST_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static void ftgmac030_diag_test(struct net_device *netdev,
				struct ethtool_test *eth_test, u64 *data)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	int forced_speed, forced_duplex, autoneg;
	bool if_running = netif_running(netdev);
	__ETHTOOL_DECLARE_LINK_MODE_MASK(autoneg_advertised);

	pm_runtime_get_sync(netdev->dev.parent);

	set_bit(__FTGMAC030_TESTING, &ctrl->state);

	if (!if_running)
		ftgmac030_reset(ctrl);

	if (eth_test->flags == ETH_TEST_FL_OFFLINE) {
		linkmode_copy(autoneg_advertised, ctrl->phy_dev->advertising);
		forced_speed = ctrl->phy_dev->speed;
		forced_duplex = ctrl->phy_dev->duplex;
		autoneg = ctrl->phy_dev->autoneg;

		e_info(drv, "Offline testing starts\n");

		if (if_running)
			dev_close(netdev);

		ftgmac030_reset(ctrl);
		if (ftgmac030_loopback_test(ctrl, &data[3]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		ftgmac030_reset(ctrl);

		if (ftgmac030_link_test(ctrl, &data[4]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		// Restore speed, duplex, autoneg settings
		linkmode_copy(ctrl->phy_dev->advertising, autoneg_advertised);
		ctrl->phy_dev->speed = forced_speed;
		ctrl->phy_dev->duplex = forced_duplex;
		ctrl->phy_dev->autoneg = autoneg;
		ftgmac030_reset(ctrl);

		clear_bit(__FTGMAC030_TESTING, &ctrl->state);
		if (if_running)
			ftgmac030_open(netdev);
	} else {
		e_info(drv, "Online testing starts\n");

		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;

		if (ftgmac030_link_test(ctrl, &data[4]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		clear_bit(__FTGMAC030_TESTING, &ctrl->state);
	}

	if (!if_running)
		ftgmac030_reset(ctrl);

	msleep_interruptible(4 * 1000);

	pm_runtime_put_sync(netdev->dev.parent);
}

static int ftgmac030_get_coalesce(struct net_device *netdev,
				  struct ethtool_coalesce *ec)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	if (ctrl->itr_setting <= 4)
		ec->rx_coalesce_usecs = ctrl->itr_setting;
	else
		ec->rx_coalesce_usecs = 1000000 / ctrl->itr_setting;

	return 0;
}

static int ftgmac030_set_coalesce(struct net_device *netdev,
				  struct ethtool_coalesce *ec)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	if ((ec->rx_coalesce_usecs > FTGMAC030_MAX_ITR_USECS) ||
	    ((ec->rx_coalesce_usecs > 4) &&
	     (ec->rx_coalesce_usecs < FTGMAC030_MIN_ITR_USECS)) ||
	     (ec->rx_coalesce_usecs == 2))
		return -EINVAL;

	if (ec->rx_coalesce_usecs == 4) {
		ctrl->itr_setting = 4;
		ctrl->itr = ctrl->itr_setting;
	} else if (ec->rx_coalesce_usecs <= 3) {
		ctrl->itr = 20000;
		ctrl->itr_setting = ec->rx_coalesce_usecs;
	} else {
		ctrl->itr = (1000000 / ec->rx_coalesce_usecs);
		ctrl->itr_setting = ctrl->itr & ~3;
	}

	pm_runtime_get_sync(netdev->dev.parent);

	if (ctrl->itr_setting != 0)
		ftgmac030_write_itr(ctrl, ctrl->itr);
	else
		ftgmac030_write_itr(ctrl, 0);

	pm_runtime_put_sync(netdev->dev.parent);

	return 0;
}

static int ftgmac030_nway_reset(struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	if (!netif_running(netdev))
		return -EAGAIN;

	if (!ctrl->phy_dev->autoneg)
		return -EINVAL;

	pm_runtime_get_sync(netdev->dev.parent);
	ftgmac030_reinit_locked(ctrl);
	pm_runtime_put_sync(netdev->dev.parent);

	return 0;
}

static void ftgmac030_get_strings(struct net_device __always_unused *netdev,
				  u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, ftgmac030_gstrings_test, sizeof(ftgmac030_gstrings_test));
		break;
	}
}

static int ftgmac030_get_eee(struct net_device *netdev,
			     struct ethtool_eee *edata)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	int val;
	u16 tx_wake_cnt, tx_enter_cnt;

	if (!(ctrl->flags & FLAG_HAS_EEE))
		return -EOPNOTSUPP;

	pm_runtime_get_sync(netdev->dev.parent);

	val = phy_ethtool_get_eee(ctrl->phy_dev, edata);
	if (val) {
		e_err(drv, "PHY get EEE configuration value failed\n");
		pm_runtime_put_sync(netdev->dev.parent);
		return val;
	}
	edata->eee_enabled = ctrl->eee_enabled;
	edata->tx_lpi_enabled = ctrl->tx_lpi_enabled;

	// Puts tx_enter_cnt value to bit [16:27] of eeecmd.tx_lpi_timer
	// for ethtool.c
	tx_enter_cnt = (ctrl->tx_lpi_timer >> FTGMAC030_EEE_TX_ENTER_SHIFT) &
		       FTGMAC030_EEE_TX_ENTER_MASK;
	tx_wake_cnt = ctrl->tx_lpi_timer & FTGMAC030_EEE_TX_WAKEUP;
	edata->tx_lpi_timer = ((tx_enter_cnt << 16) | tx_wake_cnt);

	e_info(drv, "%s: eee=%s tx-lpi=%s tx-timer=0x%x\n", __func__,
		    edata->eee_enabled ? "on" : "off",
		    edata->tx_lpi_enabled ? "on" : "off", edata->tx_lpi_timer);

	pm_runtime_put_sync(netdev->dev.parent);
	return 0;
}

static int ftgmac030_set_eee(struct net_device *netdev,
			     struct ethtool_eee *edata)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	u16 tx_wake_cnt, tx_enter_cnt;

	e_info(drv, "%s: eee=%s adv=0x%x, tx-lpi=%s, tx-timer=0x%x\n", __func__,
		     edata->eee_enabled ? "on" : "off", edata->advertised,
		     edata->tx_lpi_enabled ? "on" : "off", edata->tx_lpi_timer);

	if (edata->advertised & ~(ADVERTISED_100baseT_Full
				  | ADVERTISED_1000baseT_Full)) {
		e_err(drv, "EEE advertisement supports only 100TX and/or "
			   "1000T full-duplex\n");
		return -EINVAL;
	}

	pm_runtime_get_sync(netdev->dev.parent);

	phy_ethtool_set_eee(ctrl->phy_dev, edata);

	ctrl->eee_enabled = edata->eee_enabled;
	ctrl->tx_lpi_enabled = edata->tx_lpi_enabled;
	// ethtool.c puts tx_enter_cnt value at bit [16:27] of
	// eeecmd.tx_lpi_timer
	tx_enter_cnt = (edata->tx_lpi_timer >> 16) & FTGMAC030_EEE_TX_ENTER_MASK;
	tx_wake_cnt = edata->tx_lpi_timer & FTGMAC030_EEE_TX_WAKEUP;
	ctrl->tx_lpi_timer = ((tx_enter_cnt << FTGMAC030_EEE_TX_ENTER_SHIFT) |
			     tx_wake_cnt);

	if (netif_running(netdev))
		ftgmac030_reinit_locked(ctrl);
	else
		ftgmac030_reset(ctrl);

	pm_runtime_put_sync(netdev->dev.parent);

	return 0;
}

static int ftgmac030_get_ts_info(struct net_device *netdev,
				 struct ethtool_ts_info *info)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	ethtool_op_get_ts_info(netdev, info);

	if (!(ctrl->flags & FLAG_HAS_HW_TIMESTAMP))
		return 0;

	info->so_timestamping |= (SOF_TIMESTAMPING_TX_HARDWARE |
				  SOF_TIMESTAMPING_RX_HARDWARE |
				  SOF_TIMESTAMPING_RAW_HARDWARE);

	info->tx_types = (1 << HWTSTAMP_TX_OFF) | (1 << HWTSTAMP_TX_ON);

	info->rx_filters = ((1 << HWTSTAMP_FILTER_NONE) |
			    (1 << HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
			    (1 << HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_L2_SYNC) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_EVENT) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_SYNC) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_DELAY_REQ) |
			    (1 << HWTSTAMP_FILTER_ALL));

	if (ctrl->ptp_clock)
		info->phc_index = ptp_clock_index(ctrl->ptp_clock);

	return 0;
}

static const struct ethtool_ops ftgmac030_ethtool_ops = {
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS |
				     ETHTOOL_COALESCE_MAX_FRAMES,
	.get_link_ksettings	= phy_ethtool_get_link_ksettings,
	.set_link_ksettings	= phy_ethtool_set_link_ksettings,
	.get_drvinfo		= ftgmac030_get_drvinfo,
	.get_regs_len		= ftgmac030_get_regs_len,
	.get_regs		= ftgmac030_get_regs,
	.get_msglevel		= ftgmac030_get_msglevel,
	.set_msglevel		= ftgmac030_set_msglevel,
	.nway_reset		= ftgmac030_nway_reset,
	.get_link		= ethtool_op_get_link,
	.get_ringparam		= ftgmac030_get_ringparam,
	.set_ringparam		= ftgmac030_set_ringparam,
	.self_test		= ftgmac030_diag_test,
	.get_strings		= ftgmac030_get_strings,
	.get_sset_count		= ftgmac030_get_sset_count,
	.get_coalesce		= ftgmac030_get_coalesce,
	.set_coalesce		= ftgmac030_set_coalesce,
	.get_ts_info		= ftgmac030_get_ts_info,
	.get_eee		= ftgmac030_get_eee,
	.set_eee		= ftgmac030_set_eee,
};

void ftgmac030_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &ftgmac030_ethtool_ops;
}
