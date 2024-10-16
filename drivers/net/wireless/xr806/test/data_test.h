#ifndef __DATA_TEST_H__
#define __DATA_TEST_H__
#define DATA_TEST 0
#if DATA_TEST

int xradio_data_test_start(struct xradio_priv *priv);

void xradio_data_test_stop(struct xradio_priv *priv);

int xradio_data_test_rx_handle(u8 *data, u16 len);
#endif
#endif
