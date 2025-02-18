/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __ETH_UTIL_H__
#define __ETH_UTIL_H__

#include "femac.h"

int femac_check_hw_capability(struct sk_buff *skb);
u32 femac_get_pkt_info(struct sk_buff *skb);
void femac_sleep_us(u32 time_us);
void femac_set_flow_ctrl(const struct femac_priv *priv);
void femac_get_pauseparam(struct net_device *dev,
		  struct ethtool_pauseparam *pause);
int femac_set_pauseparam(struct net_device *dev,
		  struct ethtool_pauseparam *pause);
void femac_enable_rxcsum_drop(const struct femac_priv *priv,
		  bool drop);
int femac_set_features(struct net_device *dev, netdev_features_t features);

void femac_mdio_dummy(void);
void __femac_set_phy_device(struct phy_device *phy);

#endif
