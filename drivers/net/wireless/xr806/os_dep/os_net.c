#include "os_intf.h"
#include "xradio.h"
#include "txrx.h"

#if PLATFORM_LINUX
#include "iface.h"
#endif

static int xradio_net_data_output(void *priv, struct sk_buff *skb)
{
#if PLATFORM_LINUX
	return xradio_tx_net_process((struct xradio_priv *)priv, skb);
#endif
}

int xradio_add_net_dev(struct xradio_priv *priv, u8 if_type, u8 if_id, char *name)
{
#if PLATFORM_LINUX
	priv->net_dev_priv = xradio_iface_add_net_dev((void *)priv, if_type, if_id, name,
						      xradio_net_data_output);
	if (priv->net_dev_priv == NULL)
		return -1;
	else
		priv->link_state = XR_LINK_UP;
	return 0;
#endif
}

void xradio_remove_net_dev(struct xradio_priv *priv)
{
#if PLATFORM_LINUX
	xradio_iface_remove_net_dev(priv->net_dev_priv);
	priv->link_state = XR_LINK_DOWN;
#endif
}

void xradio_net_tx_pause(struct xradio_priv *priv)
{
#if PLATFORM_LINUX
	xradio_iface_net_tx_pause(priv->net_dev_priv);
#endif
}

void xradio_net_tx_resume(struct xradio_priv *priv)
{
#if PLATFORM_LINUX
	xradio_iface_net_tx_resume(priv->net_dev_priv);
#endif
}

int xradio_net_data_input(struct xradio_priv *priv, struct sk_buff *skb)
{
#if PLATFORM_LINUX
	return xradio_iface_net_input(priv->net_dev_priv, skb);
#endif
}

int xradio_net_set_mac(struct xradio_priv *priv, u8 *mac)
{
#if PLATFORM_LINUX
	return xradio_iface_net_set_mac(priv->net_dev_priv, mac);
#endif
}
