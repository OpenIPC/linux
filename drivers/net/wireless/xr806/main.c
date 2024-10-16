/*
 * xr806/main.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Main code init for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "os_intf.h"

#if PLATFORM_LINUX
#include <linux/init.h>
#include <linux/module.h>
#endif

#include "xradio.h"
#include "os_net.h"
#include "hwio.h"
#include "txrx.h"
#include "up_cmd.h"
#include "debug.h"
#include "low_cmd.h"
#include "xr_version.h"
#include "hwio_test.h"
#include "data_test.h"

MODULE_AUTHOR("XRadioTech");
MODULE_DESCRIPTION("XRadioTech WLAN driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("xr806");

#if (HWIO_TEST == 0)
static struct xradio_priv *priv;
#endif
static int xradio_core_init(void)
{
	int ret = 0;
#if HWIO_TEST
	return xradio_hwio_test_start();
#else
	int i = 0;

	xradio_dbg(XRADIO_DBG_ALWY, "xradio wlan version:%s\n", XRADIO_VERSION);

	xradio_dbg(XRADIO_DBG_ALWY, "add wlan interface dev\n");

	priv = (struct xradio_priv *)xradio_k_zmalloc(sizeof(struct xradio_priv));
	if (!priv) {
		xradio_dbg(XRADIO_DBG_ERROR, "xradio priv malloc failed.\n");
		return -1;
	}

	if (xradio_add_net_dev(priv, XR_STA_IF, 0, "wlan0") != 0)
		goto error;

	xradio_dbg(XRADIO_DBG_ALWY, "xradio queue init\n");
	for (i = 0; i < XR_MAX; i++) {
		if (xradio_queue_init(&priv->tx_queue[i], i, XRWL_MAX_QUEUE_SZ, 2 * HZ,
				      xradio_free_skb)) {
			for (; i > 0; i--)
				xradio_queue_deinit(&priv->tx_queue[i - 1]);
			goto error1;
		}
	}

	xradio_dbg(XRADIO_DBG_ALWY, "hardware io init\n");
	ret = xradio_hwio_init(priv);
	if (ret)
		goto error2;

	xradio_dbg(XRADIO_DBG_ALWY, "register tx and rx task\n");
	ret = xradio_register_trans(priv);
	if (ret)
		goto error3;

	xradio_dbg(XRADIO_DBG_ALWY, "up cmd init\n");
	ret = xradio_up_cmd_init(priv);
	if (ret)
		goto error4;

	xradio_dbg(XRADIO_DBG_ALWY, "low cmd init\n");
	ret = xradio_low_cmd_init(priv);
	if (ret)
		goto error5;

	ret = xraido_low_cmd_dev_hand_way();
	if (ret)
		goto error6;

	xradio_dbg(XRADIO_DBG_ALWY, "xradio core init Suceess\n");
#if DATA_TEST
	xradio_data_test_start(priv);
#endif
	return 0;
error6:
	xradio_low_cmd_deinit(priv);
error5:
	xradio_up_cmd_deinit(priv);
error4:
	xradio_hwio_deinit(priv);
error3:
	xradio_unregister_trans(priv);
error2:
	for (i = 0; i < XR_MAX; i++)
		xradio_queue_deinit(&priv->tx_queue[i]);
error1:
	xradio_remove_net_dev(priv);
error:
	xradio_k_free(priv);
	priv = NULL;
	xradio_dbg(XRADIO_DBG_ERROR, "xradio core init Failed\n");
#endif
	return ret;
}

static void xradio_core_deinit(void)
{
#if HWIO_TEST
	xradio_hwio_test_stop();
#else
	int i;
#if DATA_TEST
	xradio_data_test_stop(priv);
#endif
	if (!priv)
		return;

	xradio_dbg(XRADIO_DBG_ALWY, "low cmd deinit\n");
	xradio_low_cmd_deinit(priv);

	xradio_dbg(XRADIO_DBG_ALWY, "up cmd deinit\n");
	xradio_up_cmd_deinit(priv);

	xradio_dbg(XRADIO_DBG_ALWY, "unregister tx rx transmit\n");
	xradio_unregister_trans(priv);

	xradio_dbg(XRADIO_DBG_ALWY, "xradio queue deinit\n");
	for (i = 0; i < XR_MAX; i++)
		xradio_queue_deinit(&priv->tx_queue[i]);

	xradio_dbg(XRADIO_DBG_ALWY, "hardware io deinit\n");
	xradio_hwio_deinit(priv);

	xradio_dbg(XRADIO_DBG_ALWY, "xradio wlan interface dev remove\n");

	xradio_remove_net_dev(priv);

	xradio_k_free(priv);

	priv = NULL;
#endif
}

#if PLATFORM_LINUX
static int __init xradio_init(void)
{
	return xradio_core_init();
}

static void __exit xradio_exit(void)
{
	xradio_core_deinit();
}

module_init(xradio_init);
module_exit(xradio_exit);
#endif
