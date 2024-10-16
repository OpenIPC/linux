#include "hwio_test.h"

#if HWIO_TEST
#include <linux/skbuff.h>
#include "os_intf.h"
#include "checksum.h"
#include "xradio.h"
#include "hwio.h"
#define SD_LEN 1024
#define SD_THREAD_SLEEP 10
#if 0
#define SD_TX_COUNT 2

#define TEST_ECHO "hello world"
#define TEST_REPLY "Ok, hello world"
#define TEST_END "hello, goodbye"

static xr_thread_handle_t txrx_thread;
static xr_atomic_t txrx_term;
static xr_atomic_t tx_end;

static struct sk_buff *xradio_hwio_get_txbuff(u8 *data, uint16_t len)
{
	u8 *tx_buff = NULL;
	struct sk_buff *skb = NULL;
	size_t total_len = 0;
	struct xradio_hdr *hdr = NULL;
	static u8 seq_number;

	total_len = len + sizeof(struct xradio_hdr);

	skb = xradio_alloc_skb(total_len);

	tx_buff = skb_put(skb, total_len);

	if (!skb) {
		xradio_printf("xradio alloc skb failed.\n");
		return NULL;
	}

	hdr = (struct xradio_hdr *)tx_buff;
	memset(hdr, 0, sizeof(struct xradio_hdr));
	hdr->len = xradio_k_cpu_to_le16(len);
	hdr->offset = xradio_k_cpu_to_le16(sizeof(struct xradio_hdr));

	hdr->message = xradio_k_cpu_to_le16(seq_number << 8 | XR_REQ_DATA);

	if (!data)
		memset(tx_buff + hdr->offset, 0x68, len);
	else
		memcpy(tx_buff + hdr->offset, data, len);

	hdr->checksum = xradio_k_cpu_to_le16(
		xradio_crc_16(tx_buff + CHECKSUM_ADDR_OFFSET, len + CHECKSUM_LEN_OFFSET));

	seq_number++;

	return skb;
}

static void xradio_hwio_rx_process(struct sk_buff *skb, u8 *rx, u16 rxlen)
{
	struct xradio_hdr *hdr = NULL;
	u16 len = 0;
	int i;
	u8 type_id = 0;
	u16 offset = 0;
	u8 seq;

	u16 checksum = 0, c_checksum = 0;

	if (!skb)
		return;

	hdr = (struct xradio_hdr *)skb->data;

	len = le16_to_cpu(hdr->len);
	offset = le16_to_cpu(hdr->offset);
	checksum = le16_to_cpu(hdr->checksum);

	c_checksum =
		xradio_crc_16((u8 *)hdr + CHECKSUM_ADDR_OFFSET, hdr->len + CHECKSUM_LEN_OFFSET);

	if (checksum != c_checksum) {
		xradio_printf("checksum failed,[%d,%d]\n", checksum, c_checksum);

		dev_kfree_skb_any(skb);
		xradio_printf("RX:\n");
		for (i = 0; i < 8; i++)
			xradio_printf("%x ", skb->data[i]);
		xradio_printf("\n");
		return;
	} else {
		xradio_printf("Recv data success:%d\n", len);
	}

	type_id = le16_to_cpu(hdr->message) & TYPE_ID_MASK;
	seq = (le16_to_cpu(hdr->message) & SEQ_NUM_MASK) >> 8;
	skb_pull(skb, offset);
	if (rx && type_id == XR_DATA)
		memcpy(rx, skb->data, (len < rxlen ? len : rxlen));

	if (memcmp(skb->data, TEST_END, strlen(TEST_END)) == 0) {
		if (xradio_k_atomic_read(&tx_end))
			xradio_k_atomic_set(&tx_end, 0);
	}

	xradio_free_skb(skb);
}
#endif
/*The SPI of the device cannot send or receive at the same time*/
static int xradio_hwio_txrx_thread(void *arg)
{
	struct sk_buff *tx_skb = NULL;
	u8 *buf = NULL;
	int rx;

	// int tx_count = SD_TX_COUNT + 1;
	// int tx_enable = 1;

	while (1) {
		if (xradio_k_atomic_read(&txrx_term)) {
			xradio_printf("hwio txrx thread quit.\n");
			break;
		}
#if 0
		rx = xradio_hwio_rx_pending();
		if (rx) {
			rx_skb = xradio_hwio_read();
			if (rx_skb)
				xradio_hwio_rx_process(rx_skb, NULL, 0);
		}

		if (tx_enable && (tx_count > SD_TX_COUNT || xradio_k_atomic_read(&tx_end))) {
			tx_count = 0;

			if (xradio_k_atomic_read(&tx_end)) {
				tx_skb = xradio_hwio_get_txbuff(TEST_END, strlen(TEST_END));
				xradio_printf("Send tx end pkt to dev\n");
				tx_enable = 0;
			} else {
				tx_skb = xradio_hwio_get_txbuff(NULL, SD_LEN);
			}

			if (tx_skb) {
				xradio_printf("tx data len:%d\n", SD_LEN);
				if (!xradio_hwio_write(tx_skb)) {
					xradio_free_skb(tx_skb);
					tx_skb = NULL;
				}
			}
		}

		tx_count++;
#endif
		tx_skb = xradio_alloc_skb(SD_LEN);

		buf = skb_put(tx_skb, SD_LEN);

		memset(buf, 0xaa, SD_LEN);

		while (xradio_tx_net_process(xr_priv, tx_skb) != 0)
			msleep(SD_THREAD_SLEEP);

		msleep(SD_THREAD_SLEEP);
	}

	xradio_k_thread_exit(&txrx_thread);
	xradio_k_atomic_set(&txrx_term, 0);
	return 0;
}

int xradio_hwio_test_start(void)
{
#if 0
	struct sk_buff *tx_skb = NULL, *rx_skb = NULL;
	u8 reply[20];
	int rx_count = 0;
	int i;

	xradio_k_atomic_set(&txrx_term, 0);

	xradio_k_atomic_set(&tx_end, 0);

	if (xradio_hwio_init(NULL)) {
		xradio_printf("hwio init failed.\n");
		return -1;
	}

	xradio_printf("Phase 1 Handshake Test......\n");

	tx_skb = xradio_hwio_get_txbuff(TEST_ECHO, strlen(TEST_ECHO));
	if (!tx_skb)
		return -1;

	for (i = 0; i < tx_skb->len; i++)
		xradio_printf("0x%x ", tx_skb->data[i]);
	xradio_printf("\n");

	if (xradio_hwio_write(tx_skb)) {
		xradio_printf("HWIO send data failed\n");
		goto failed;
	}

rx_again:
	rx_count++;
	while (!xradio_hwio_rx_pending()) {
		msleep(50);
		if (rx_count < 5000 / 50) {
			xradio_printf("HWIO wait rx  pending timeout.\n");
			goto failed;
		}
	}

	rx_skb = xradio_hwio_read();
	if (rx_skb) {
		xradio_hwio_rx_process(rx_skb, reply, 20);
		if (memcmp(reply, TEST_REPLY, strlen(TEST_REPLY)) != 0) {
			xradio_printf("Recv reply error\n");
			for (i = 0; i < 20; i++)
				xradio_printf("0x%x ", reply[i]);
			xradio_printf("\n");
			goto failed;
		} else {
			for (i = 0; i < 20; i++)
				xradio_printf("0x%x ", reply[i]);
			xradio_printf("\n");
		}
	} else {
		msleep(20);
		if (rx_count > 5000 / 10) {
			xradio_printf("HWIO wait reply timeout.\n");
			goto failed;
		} else {
			goto rx_again;
		}
	}

	xradio_printf("Handshake success.\n");

	xradio_printf("Phase 2, stress tx and rx test......\n");
	if (xradio_k_thread_create(&txrx_thread, "xr_hw_txrx", xradio_hwio_txrx_thread, NULL, 0,
				   4096)) {
		xradio_printf("create hw test thread failed.\n");
		goto failed;
	}

	return 0;

failed:
	xradio_free_skb(tx_skb);
	xradio_hwio_deinit(NULL);
	return -1;
#else

	if (xradio_k_thread_create(&txrx_thread, "xr_hw_txrx", xradio_hwio_txrx_thread, NULL, 0,
				   4096)) {
		xradio_printf("create hw test thread failed.\n");
		return -1;
	}

	return 0;
#endif
}

int xradio_hwio_test_stop(void)
{
#if 0
	xradio_k_atomic_add(1, &tx_end);

	for (;;) {
		if (!xradio_k_atomic_read(&tx_end))
			break;
		xradio_k_msleep(50);
	}
#endif
	xradio_k_atomic_add(1, &txrx_term);
	xradio_k_thread_delete(&txrx_thread);

	for (;;) {
		if (!xradio_k_atomic_read(&txrx_term))
			break;
		xradio_k_msleep(50);
	}
	xradio_printf("Test end.\n");
#if 0
	xradio_hwio_deinit(NULL);
#endif
	return 0;
}
#endif
