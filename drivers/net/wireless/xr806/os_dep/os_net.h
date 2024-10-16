#ifndef __OS_NET_H__
#define __OS_NET_H__

int xradio_add_net_dev(struct xradio_priv *priv, u8 if_type, u8 if_id, char *name);

void xradio_remove_net_dev(struct xradio_priv *priv);

void xradio_net_tx_pause(struct xradio_priv *priv);

void xradio_net_tx_resume(struct xradio_priv *priv);

int xradio_net_data_input(struct xradio_priv *priv, struct sk_buff *skb);

int xradio_net_set_mac(struct xradio_priv *priv, u8 *mac);
#endif
