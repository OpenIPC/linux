/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _AP_H_
#define _AP_H_

//#define BCAST_DEBUG

void ssv6xxx_beacon_change(struct ssv_softc *sc, struct ieee80211_hw *hw, struct ieee80211_vif *vif, bool aid0_bit_set);
void ssv6xxx_beacon_release(struct ssv_softc *sc);

void ssv6200_set_tim_work(struct work_struct *work);
void ssv6200_bcast_start_work(struct work_struct *work);
void ssv6200_bcast_stop_work(struct work_struct *work);
void ssv6200_bcast_tx_work(struct work_struct *work);

//void ssv6200_bcast_timer(unsigned long arg);
int ssv6200_bcast_queue_len(struct ssv6xxx_bcast_txq *bcast_txq);
struct sk_buff* ssv6200_bcast_dequeue(struct ssv6xxx_bcast_txq *bcast_txq, u8 *remain_len);

/*return how many frames in queue*/
int ssv6200_bcast_enqueue(struct ssv_softc *sc, struct ssv6xxx_bcast_txq *bcast_txq, struct sk_buff *skb);

void ssv6200_bcast_start(struct ssv_softc *sc);
void ssv6200_bcast_stop(struct ssv_softc *sc);

void ssv6200_release_bcast_frame_res(struct ssv_softc *sc, struct ieee80211_vif *vif);


#define SSV_SET_BEACON_REG_LOCK( _sh, _bool)    HAL_SET_BEACON_REG_LOCK( _sh, _bool)
#define SSV_BEACON_GET_VALID_CFG( _sh)          HAL_BEACON_GET_VALID_CFG( _sh)
#define SSV_ALLOC_PBUF( _sc, _len, _TX_BUF)     HAL_ALLOC_PBUF( _sc, _len, _TX_BUF)
#define SSV_BEACON_ENABLE( _sc, _bool)          HAL_BEACON_ENABLE( _sc, _bool)
#define SSV_GET_BCN_ONGOING( _sh)               HAL_GET_BCN_ONGOING( _sh)
#define SSV_SET_BCN_IFNO( _sh, _beacon_interval, _beacon_dtim_cnt)              \
            HAL_SET_BCN_IFNO( _sh, _beacon_interval, _beacon_dtim_cnt)
#define SSV_FILL_BEACON_TX_DESC(_sc, _skb)		HAL_FILL_BEACON_TX_DESC(_sc, _skb)

#endif /* _LIB_H_ */
