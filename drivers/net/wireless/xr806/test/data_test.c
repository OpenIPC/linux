#include <linux/skbuff.h>
#include "os_intf.h"
#include "xradio.h"
#include "data_test.h"
#include "txrx.h"

#if DATA_TEST
#define TX_LEN 1024
#define SD_THREAD_SLEEP 10

#define TEST_START "data test start!"
#define TEST_END "data test end!"

static xr_thread_handle_t tx_thread;

static xr_atomic_t txrx_term;

static int xradio_data_test_tx_thread(void *arg)
{
	struct xradio_priv *priv = (struct xradio_priv *)arg;
	struct sk_buff *tx_skb = NULL;
	u8 *tx_buff = NULL;

	// TEST START IND
	tx_skb = xradio_alloc_skb(strlen(TEST_START));

	tx_buff = skb_put(tx_skb, strlen(TEST_START));

	memcpy(tx_buff, TEST_START, strlen(TEST_START));

	while (xradio_tx_net_process(priv, tx_skb) != 0)
		msleep(SD_THREAD_SLEEP);

	while (1) {
		if (xradio_k_atomic_read(&txrx_term)) {
			xradio_printf("hwio txrx thread quit.\n");
			break;
		}

		tx_skb = xradio_alloc_skb(TX_LEN);

		tx_buff = skb_put(tx_skb, TX_LEN);

		memset(tx_buff, 0xaa, TX_LEN);

		printk(KERN_INFO "Send data len:%d\n", TX_LEN);

		if (xradio_tx_net_process(priv, tx_skb) != 0)
			break;

		msleep(SD_THREAD_SLEEP);
	}

	// TEST END IND
	tx_skb = xradio_alloc_skb(strlen(TEST_END));

	tx_buff = skb_put(tx_skb, strlen(TEST_END));

	memcpy(tx_buff, TEST_START, strlen(TEST_END));

	xradio_tx_net_process(priv, tx_skb);

	xradio_k_thread_exit(&tx_thread);
	xradio_k_atomic_set(&txrx_term, 0);
	return 0;
}

int xradio_data_test_rx_handle(u8 *data, u16 len)
{
	printk(KERN_INFO "Recv data len:%d\n", len);
	return 0;
}

int xradio_data_test_start(struct xradio_priv *priv)
{
	if (xradio_k_thread_create(&tx_thread, "xr_hw_tx", xradio_data_test_tx_thread, (void *)priv,
				   0, 4096)) {
		xradio_printf("create hw test thread failed.\n");
		return -1;
	}
	return 0;
}

void xradio_data_test_stop(struct xradio_priv *priv)
{
	xradio_k_atomic_add(1, &txrx_term);
	xradio_k_thread_delete(&tx_thread);

	for (;;) {
		if (!xradio_k_atomic_read(&txrx_term))
			break;
		xradio_k_msleep(50);
	}

	xradio_printf("Test end.\n");
}
#endif
