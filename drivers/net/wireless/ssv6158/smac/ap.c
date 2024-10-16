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
#include <linux/version.h>
#include <ssv6200.h>
#include <linux/nl80211.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/sched.h>

#ifdef SSV_MAC80211
#include "ssv_mac80211.h"
#else
#include <net/mac80211.h>
#endif
#include <ssv6200.h>
#include "lib.h"
#include "dev.h"
#include "ap.h"
#include <hal.h>

int ssv6200_bcast_queue_len(struct ssv6xxx_bcast_txq *bcast_txq);

#define PBUF_BASE_ADDR	            0x80000000
#define PBUF_ADDR_SHIFT	            16

#define PBUF_MapPkttoID(_PKT)		(((u32)_PKT&0x0FFF0000)>>PBUF_ADDR_SHIFT)
#define PBUF_MapIDtoPkt(_ID)		(PBUF_BASE_ADDR|((_ID)<<PBUF_ADDR_SHIFT))


#define SSV6xxx_BEACON_MAX_ALLOCATE_CNT	   10

//=====>ADR_MTX_BCN_EN_MISC
#define MTX_BCN_PKTID_CH_LOCK_SHIFT MTX_BCN_PKTID_CH_LOCK_SFT																//bit 0

#define MTX_BCN_CFG_VLD_SHIFT MTX_BCN_CFG_VLD_SFT																			// 1
#define MTX_BCN_CFG_VLD_MASK  MTX_BCN_CFG_VLD_MSK																			//0x00000007  //just see 3 bits

#define AUTO_BCN_ONGOING_MASK  MTX_AUTO_BCN_ONGOING_MSK																		//0x00000008
#define AUTO_BCN_ONGOING_SHIFT MTX_AUTO_BCN_ONGOING_SFT																		// 3

#define MTX_BCN_TIMER_EN_SHIFT				MTX_BCN_TIMER_EN_SFT															//	0
#define MTX_TSF_TIMER_EN_SHIFT				MTX_TSF_TIMER_EN_SFT															//	5
#define MTX_HALT_MNG_UNTIL_DTIM_SHIFT		MTX_HALT_MNG_UNTIL_DTIM_SFT														//	6
#define MTX_BCN_ENABLE_MASK					(MTX_BCN_TIMER_EN_I_MSK)	//0xffffff9e

//=====>

//=====>ADR_MTX_BCN_PRD
#define MTX_BCN_PERIOD_SHIFT	MTX_BCN_PERIOD_SFT		// 0			//bit0~7
#define MTX_DTIM_NUM_SHIFT		MTX_DTIM_NUM_SFT		// 24			//bit 24~bit31
//=====>

//=====>ADR_MTX_BCN_CFG0/ADR_MTX_BCN_CFG1
#define MTX_DTIM_OFST0			MTX_DTIM_OFST0_SFT		// 16
//=====>

//Assume Beacon come from one sk_buf
//Beacon
bool ssv6xxx_beacon_set(struct ssv_softc *sc, struct sk_buff *beacon_skb, int dtim_offset)
{
	enum ssv6xxx_beacon_type avl_bcn_type = SSV6xxx_BEACON_0;
	bool ret = true;

	//Lock HW BCN module.
	//SET_MTX_BCN_PKTID_CH_LOCK(1);

	SSV_SET_BEACON_REG_LOCK(sc->sh, true);

	//if(beacon_skb->len != beacon_skb->data_len)
	//{
	//	ret = false;
	//	printk("=============>ERROR!! Beacon store in more than one sk_buff: beacon_skb->len[%d] beacon_skb->data_len[%d]....Need to implement\n",
	//		beacon_skb->len, beacon_skb->data_len);
	//	goto out;
	//}

	//1.Decide which register can be used to set

    avl_bcn_type = SSV_BEACON_GET_VALID_CFG(sc->sh);

	dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
	    "[A] ssv6xxx_beacon_set avl_bcn_type[%d]\n", avl_bcn_type);

	//2.Get Pbuf from ASIC
	do{

		if(IS_BIT_SET(sc->beacon_usage, avl_bcn_type)){
			dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
	            "[A] beacon has already been set old len[%d] new len[%d]\n",
	            sc->beacon_info[avl_bcn_type].len, beacon_skb->len);

			if (sc->beacon_info[avl_bcn_type].len >= beacon_skb->len){
				break;
			} else {
				//old beacon too small, need to free
		        if (false == SSV_FREE_PBUF(sc, sc->beacon_info[avl_bcn_type].pubf_addr)){

					dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
	                    "=============>ERROR!!Intend to allcoate beacon from ASIC fail.\n");
					ret = false;
					goto out;
				}
				CLEAR_BIT(sc->beacon_usage, avl_bcn_type);
			}
		}


		//Allocate new one
	    sc->beacon_info[avl_bcn_type].pubf_addr = SSV_ALLOC_PBUF(sc, beacon_skb->len, RX_BUF);
		sc->beacon_info[avl_bcn_type].len = beacon_skb->len;

		//if can't allocate beacon, just leave.
		if(sc->beacon_info[avl_bcn_type].pubf_addr == 0){
			ret = false;
			goto out;
		}

		//Indicate reg is stored packet buf.
		SET_BIT(sc->beacon_usage, avl_bcn_type);
		dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
	        "[A] beacon type[%d] usage[%d] allocate new beacon addr[%08x] \n",
	        avl_bcn_type, sc->beacon_usage, sc->beacon_info[avl_bcn_type].pubf_addr);
	}while(0);


	//3. Write Beacon content.
	HAL_FILL_BCN(sc, sc->beacon_info[avl_bcn_type].pubf_addr, beacon_skb);

	//4. Assign to register let tx know. Beacon is updated.
	HAL_SET_BCN_ID_DTIM(sc, avl_bcn_type, dtim_offset);

out:
    SSV_SET_BEACON_REG_LOCK(sc->sh, false);

    if(sc->beacon_usage && (sc->enable_beacon&BEACON_WAITING_ENABLED)){
        printk("[A] enable beacon for BEACON_WAITING_ENABLED flags\n");
        SSV_BEACON_ENABLE(sc, true);
    }

	return ret;
}


//need to stop beacon firstly.
void ssv6xxx_beacon_release(struct ssv_softc *sc)
{
	//int cnt=10;

	printk("[A] ssv6xxx_beacon_release Enter\n");

    cancel_work_sync(&sc->set_tim_work);
    SSV_BEACON_ENABLE(sc, false);
#if 0
	do {
        if (SSV_GET_BCN_ONGOING(sc->sh)){
            SSV_BEACON_ENABLE(sc, false);
		} else {
			break;
	    }

		cnt--;
		if(cnt<=0)
			break;

	} while(1);


	if(IS_BIT_SET(sc->beacon_usage, SSV6xxx_BEACON_0)){
	    SSV_FREE_PBUF(sc, sc->beacon_info[SSV6xxx_BEACON_0].pubf_addr);
		CLEAR_BIT(sc->beacon_usage, SSV6xxx_BEACON_0);
	}
	//else
	//	printk("=============>ERROR!! release ");


	if(IS_BIT_SET(sc->beacon_usage, SSV6xxx_BEACON_1)) {
        SSV_FREE_PBUF(sc, sc->beacon_info[SSV6xxx_BEACON_1].pubf_addr);
		CLEAR_BIT(sc->beacon_usage, SSV6xxx_BEACON_1);
	}

	sc->enable_beacon = 0;

    if(sc->beacon_buf){
        dev_kfree_skb_any(sc->beacon_buf);
        sc->beacon_buf = NULL;
    }

	dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON, "[A] ssv6xxx_beacon_release leave\n");
#endif
}




void ssv6xxx_beacon_change(struct ssv_softc *sc, struct ieee80211_hw *hw, struct ieee80211_vif *vif, bool aid0_bit_set)
{
	//struct ssv_hw *sh = sc->sh;
	struct sk_buff *skb;
    struct sk_buff *old_skb = NULL;
	u16 tim_offset, tim_length;


//	printk("[A] ssv6xxx_beacon_change\n");

    if(sc == NULL || hw == NULL || vif == NULL ){
        printk("[Error]........ssv6xxx_beacon_change input error\n");
		return;
    }

    do{
	skb = ieee80211_beacon_get_tim(hw, vif,
			&tim_offset, &tim_length);

        if(skb == NULL){
            printk("[Error]........skb is NULL\n");
            break;
        }

	if (tim_offset && tim_length >= 6) {
	/* Ignore DTIM count from mac80211:
		 * firmware handles DTIM internally.
		 */
		skb->data[tim_offset + 2] = 0;
            // for sw beacon, set dtim period = 1
            if (!(sc->sh->cfg.hw_caps & SSV6200_HW_CAP_BEACON))
		    skb->data[tim_offset + 3] = 1;

		/* Set/reset aid0 bit */
		if (aid0_bit_set)
			skb->data[tim_offset + 4] |= 1;
		else
			skb->data[tim_offset + 4] &= ~1;
	}

	dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
	        "[A] beacon len [%d] tim_offset[%d]\n", skb->len, tim_offset);

	SSV_FILL_BEACON_TX_DESC(sc, skb);

	dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
	        "[A] beacon len [%d] tim_offset[%d]\n", skb->len, tim_offset);

        if(sc->beacon_buf)
        {
            //struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc *)sc->beacon_buf->data;
            /*
                    * Compare if content is the same ( not include fcs)
                    */
            if(memcmp(sc->beacon_buf->data, skb->data, (skb->len-FCS_LEN)) == 0){
                /* no need set new beacon to register*/
                old_skb = skb;
                break;
            }
            else{
                 old_skb = sc->beacon_buf;
                 sc->beacon_buf = skb;
            }
        }
        else{
            sc->beacon_buf = skb;
        }

    //for debug
    //	{
    //		int i;
    //		u8 *ptr = &skb->data[tim_offset];
    //		printk("=================DTIM===================\n");
    //		for(i=0;i<tim_length;i++)
    //			printk("%08x ", *(ptr+i));
    //		printk("\n=======================================\n");
    //	}
    //


        //ASIC need to know the position of DTIM count. therefore add 2 bytes more.
	tim_offset+=2;

	if(ssv6xxx_beacon_set(sc, skb, tim_offset))
	{

		u8 dtim_cnt = vif->bss_conf.dtim_period-1;
		if(sc->beacon_dtim_cnt != dtim_cnt)
		{
			sc->beacon_dtim_cnt = dtim_cnt;

                dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
	                "[A] beacon_dtim_cnt [%d]\n", sc->beacon_dtim_cnt);

                SSV_SET_BCN_IFNO(sc->sh, sc->beacon_interval,
                    sc->beacon_dtim_cnt);
		}
	}
    }while(0);

    if(old_skb)
	    dev_kfree_skb_any(old_skb);

}

void ssv6200_set_tim_work(struct work_struct *work)
{
    struct ssv_softc *sc =
            container_of(work, struct ssv_softc, set_tim_work);
#ifdef BROADCAST_DEBUG
	printk("%s() enter\n", __FUNCTION__);
#endif
	ssv6xxx_beacon_change(sc, sc->hw, sc->ap_vif, sc->aid0_bit_set);
#ifdef BROADCAST_DEBUG
    printk("%s() leave\n", __FUNCTION__);
#endif
}


int ssv6200_bcast_queue_len(struct ssv6xxx_bcast_txq *bcast_txq)
{
	u32 len;
    unsigned long flags;

    spin_lock_irqsave(&bcast_txq->txq_lock, flags);
    len = bcast_txq->cur_qsize;
    spin_unlock_irqrestore(&bcast_txq->txq_lock, flags);

    return len;
}



struct sk_buff* ssv6200_bcast_dequeue(struct ssv6xxx_bcast_txq *bcast_txq, u8 *remain_len)
{
    struct sk_buff *skb = NULL;
    unsigned long flags;

    spin_lock_irqsave(&bcast_txq->txq_lock, flags);
    if(bcast_txq->cur_qsize){
        bcast_txq->cur_qsize --;
        if(remain_len)
            *remain_len = bcast_txq->cur_qsize;
        skb = __skb_dequeue(&bcast_txq->qhead);
    }
    spin_unlock_irqrestore(&bcast_txq->txq_lock, flags);

    return skb;
}


int ssv6200_bcast_enqueue(struct ssv_softc *sc, struct ssv6xxx_bcast_txq *bcast_txq,
                                                        struct sk_buff *skb)
{
    unsigned long flags;

    spin_lock_irqsave(&bcast_txq->txq_lock, flags);

	/* Release oldest frame. */
    if (bcast_txq->cur_qsize >=
                    SSV6200_MAX_BCAST_QUEUE_LEN){
        struct sk_buff *old_skb;

		old_skb = __skb_dequeue(&bcast_txq->qhead);
        bcast_txq->cur_qsize --;
        //dev_kfree_skb_any(old_skb);
        ssv6xxx_txbuf_free_skb(old_skb, (void*)sc);

        printk("[B] ssv6200_bcast_enqueue - remove oldest queue\n");
    }


    __skb_queue_tail(&bcast_txq->qhead, skb);
    bcast_txq->cur_qsize ++;

    spin_unlock_irqrestore(&bcast_txq->txq_lock, flags);

    return bcast_txq->cur_qsize;
}

void ssv6200_bcast_flush(struct ssv_softc *sc, struct ssv6xxx_bcast_txq *bcast_txq)
{

    struct sk_buff *skb;
    unsigned long flags;

#ifdef  BCAST_DEBUG
    printk("ssv6200_bcast_flush\n");
#endif
    spin_lock_irqsave(&bcast_txq->txq_lock, flags);
    while(bcast_txq->cur_qsize > 0) {
        skb = __skb_dequeue(&bcast_txq->qhead);
        bcast_txq->cur_qsize --;
        //dev_kfree_skb_any(skb);
        ssv6xxx_txbuf_free_skb(skb, (void*)sc);
    }
    spin_unlock_irqrestore(&bcast_txq->txq_lock, flags);
}



static int queue_block_cnt = 0;

/* If buffer any mcast frame, send it. */
//void ssv6200_bcast_timer(unsigned long arg)
void ssv6200_bcast_tx_work(struct work_struct *work)
{
    struct ssv_softc *sc =
            container_of(work, struct ssv_softc, bcast_tx_work.work);
//	struct ssv_softc *sc = (struct ssv_softc *)arg;


#if 1
    struct sk_buff* skb;
    int i;
    u8 remain_size;
#endif
    unsigned long flags;
    bool needtimer = true;
    long tmo = sc->bcast_interval;							//Trigger after DTIM

    spin_lock_irqsave(&sc->ps_state_lock, flags);

    do{
#ifdef  BCAST_DEBUG
        printk("[B] bcast_timer: hw_mng_used[%d] HCI_TXQ_EMPTY[%d] bcast_queue_len[%d].....................\n",
               sc->hw_mng_used, HCI_TXQ_EMPTY(sc->sh, 8), ssv6200_bcast_queue_len(&sc->bcast_txq));
#endif
        //HCI_PAUSE(sc->sh, (TXQ_MGMT));

        /* if there is any frame in low layer, stop sending at this time */
        if(sc->hw_mng_used != 0 ||
            false == HCI_TXQ_EMPTY(sc->sh, 8)){
#ifdef  BCAST_DEBUG
            printk("HW queue still have frames insdide. skip this one hw_mng_used[%d] bEmptyTXQ4[%d]\n",
                sc->hw_mng_used, HCI_TXQ_EMPTY(sc->sh, 8));
#endif
            queue_block_cnt++;
            /* does hw queue have problem??? flush bcast queue to prevent tx_work drop in an inifate loop*/
            if(queue_block_cnt>5){
                queue_block_cnt = 0;
                ssv6200_bcast_flush(sc, &sc->bcast_txq);
                needtimer = false;
            }

            break;
        }

        queue_block_cnt = 0;

        for(i=0;i<SSV6200_ID_MANAGER_QUEUE;i++){

            skb = ssv6200_bcast_dequeue(&sc->bcast_txq, &remain_size);
            if(!skb){
                needtimer = false;
                break;
            }

            if( (0 != remain_size) &&
                (SSV6200_ID_MANAGER_QUEUE-1) != i ){
                /* tell station there are more broadcast frame sending... */
                struct ieee80211_hdr *hdr;
                //struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc *)skb->data;
                //hdr = (struct ieee80211_hdr *) ((u8*)tx_desc+tx_desc->hdr_offset);
                //tx_desc->hdr_offset = TXPB_OFFSET for always
                hdr = (struct ieee80211_hdr *) ((u8*)skb->data+TXPB_OFFSET);
                hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_MOREDATA);
            }

#ifdef  BCAST_DEBUG
            printk("[B] bcast_timer:tx remain_size[%d] i[%d]\n", remain_size, i);
#endif
			spin_unlock_irqrestore(&sc->ps_state_lock, flags);

            if(HCI_SEND(sc->sh, skb, 4, false)<0){
                printk("bcast_timer send fail!!!!!!! \n");
                //dev_kfree_skb_any(skb);
                ssv6xxx_txbuf_free_skb(skb, (void*)sc);
                BUG_ON(1);
            }
			spin_lock_irqsave(&sc->ps_state_lock, flags);

        }
    }while(0);

    if(needtimer){
#ifdef  BCAST_DEBUG
        printk("[B] bcast_timer:need more timer to tx bcast frame time[%d]\n", sc->bcast_interval);
#endif
        queue_delayed_work(sc->config_wq,
				   &sc->bcast_tx_work,
				   tmo);

        //mod_timer(&sc->bcast_timeout, jiffies + tmo);
    }
    else{
#ifdef  BCAST_DEBUG
       printk("[B] bcast_timer: ssv6200_bcast_stop\n");
#endif
       ssv6200_bcast_stop(sc);
    }

    spin_unlock_irqrestore(&sc->ps_state_lock, flags);

#ifdef  BCAST_DEBUG
    printk("[B] bcast_timer: leave.....................\n");
#endif
}






/**
  *1. Update DTIM
  *2. Send Broadcast frame after DTIM
  */
void ssv6200_bcast_start_work(struct work_struct *work)
{
	struct ssv_softc *sc =
		container_of(work, struct ssv_softc, bcast_start_work);

#ifdef  BCAST_DEBUG
    printk("[B] ssv6200_bcast_start_work==\n");
#endif

	/* Every DTIM launch timer to send b-frames*/
    sc->bcast_interval = (sc->beacon_dtim_cnt+1) *
			(sc->beacon_interval + 20) * HZ / 1000;							//Trigger after DTIM;

	if (!sc->aid0_bit_set) {
		sc->aid0_bit_set = true;

        /* 1. Update DTIM  */
		ssv6xxx_beacon_change(sc, sc->hw,
                        sc->ap_vif, sc->aid0_bit_set);

        /* 2. Send Broadcast frame after DTIM  */
        //mod_timer(&sc->bcast_timeout, jiffies + sc->bcast_interval);
        queue_delayed_work(sc->config_wq,
				   &sc->bcast_tx_work,
				   sc->bcast_interval);

#ifdef  BCAST_DEBUG
        printk("[B] bcast_start_work: Modify timer to DTIM[%d]ms==\n",
               (sc->beacon_dtim_cnt+1)*(sc->beacon_interval + 20));
#endif
	}


}

/**
  *1. Update DTIM.
  *2. Remove timer to send beacon.
  */
void ssv6200_bcast_stop_work(struct work_struct *work)
{
	struct ssv_softc *sc =
		container_of(work, struct ssv_softc, bcast_stop_work.work);
    long tmo = HZ / 100;//10ms

#ifdef  BCAST_DEBUG
    printk("[B] ssv6200_bcast_stop_work\n");
#endif

    /* expired every 10ms*/
    //sc->bcast_interval = HZ / 10;

	if (sc->aid0_bit_set) {
        if(0== ssv6200_bcast_queue_len(&sc->bcast_txq)){

            /* 1. Remove timer to send beacon. */
//		del_timer_sync(&sc->bcast_timeout);
            cancel_delayed_work_sync(&sc->bcast_tx_work);

		sc->aid0_bit_set = false;

            /* 2. Update DTIM. */
		ssv6xxx_beacon_change(sc, sc->hw,
                            sc->ap_vif, sc->aid0_bit_set);
#ifdef  BCAST_DEBUG
            printk("remove group bit in DTIM\n");
#endif
        }
        else{
#ifdef  BCAST_DEBUG
            printk("bcast_stop_work: bcast queue still have data. just modify timer to 10ms\n");
#endif
            //mod_timer(&sc->bcast_timeout, jiffies + sc->bcast_interval);
            queue_delayed_work(sc->config_wq,
				   &sc->bcast_tx_work,
				   tmo);
        }
	}
}





void ssv6200_bcast_stop(struct ssv_softc *sc)
{
    //cancel_work_sync(&sc->bcast_start_work);
    queue_delayed_work(sc->config_wq,
						   &sc->bcast_stop_work, sc->beacon_interval*HZ/1024);
}



void ssv6200_bcast_start(struct ssv_softc *sc)
{
    //cancel_delayed_work_sync(&sc->bcast_stop_work);
    queue_work(sc->config_wq, &sc->bcast_start_work);
}


void ssv6200_release_bcast_frame_res(struct ssv_softc *sc, struct ieee80211_vif *vif)
{
    unsigned long flags;
    struct ssv_vif_priv_data *priv_vif = (struct ssv_vif_priv_data *)vif->drv_priv;

    //?? no need do this ??
    spin_lock_irqsave(&sc->ps_state_lock, flags);
    /*clear asleep mask to deny new frame insert*/
    priv_vif->sta_asleep_mask = 0;
    spin_unlock_irqrestore(&sc->ps_state_lock, flags);


    cancel_work_sync(&sc->bcast_start_work);
    cancel_delayed_work_sync(&sc->bcast_stop_work);
    ssv6200_bcast_flush(sc, &sc->bcast_txq);
    cancel_delayed_work_sync(&sc->bcast_tx_work);
}


//Beacon related end
//----------------------------------------------------------------------------
