/*
 * mdrv_swtoe_rxq.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include "mstar_chip.h"
#include "drv_dualos.h"
#include "mdrv_swtoe.h"
#include "mdrv_swtoe_intl.h"

//--------------------------------------------------------------------------------------------------
//  Constant definition
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Data structure definition
//--------------------------------------------------------------------------------------------------
typedef struct
{
    // int                 rx_pkt_size;
    // int                 rxq_depth;
    // virtual addr
    sstoe_rx_desc_t* rxq;
    // miu address
    // dma_addr_t          rxq_phy;

    //
    // struct sk_buff**    skb_list;

    // char                used;
    // u16                 read;
    char** buf_list;
    // int                 buf_size;
    int frag_size;
} sstoe_rxq_info;

//--------------------------------------------------------------------------------------------------
//  Data definition
//--------------------------------------------------------------------------------------------------
static sstoe_rxq_info rxq_info[1];

//--------------------------------------------------------------------------------------------------
//  Forward declaration
//--------------------------------------------------------------------------------------------------
// static int _drv_sstoe_rxq_skb_alloc(struct sk_buff **skb, int size);
// static void _drv_sstoe_rxq_skb_free(struct sk_buff *skb);

#if 0
static inline int mtk_max_frag_size(int mtu)
{
	/* make sure buf_size will be at least MTK_MAX_RX_LENGTH */
	if (mtu + MTK_RX_ETH_HLEN < MTK_MAX_RX_LENGTH)
		mtu = MTK_MAX_RX_LENGTH - MTK_RX_ETH_HLEN;

	return SKB_DATA_ALIGN(MTK_RX_HLEN + mtu) +
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
}

static inline int mtk_max_buf_size(int frag_size)
{
	int buf_size = frag_size - NET_SKB_PAD - NET_IP_ALIGN -
		       SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	WARN_ON(buf_size < MTK_MAX_RX_LENGTH);

	return buf_size;
}
#endif

#if 0
		/* receive data */
		skb = build_skb(data, ring->frag_size);
		if (unlikely(!skb)) {
			SWTOE_FREE(new_data);
			netdev->stats.rx_dropped++;
			goto release_desc;
		}
		skb_reserve(skb, NET_SKB_PAD + NET_IP_ALIGN);

#endif
#if 0
		dma_addr = dma_map_single(eth->dev,
					  new_data + NET_SKB_PAD,
					  ring->buf_size,
					  DMA_FROM_DEVICE);
#endif
//--------------------------------------------------------------------------------------------------
//  Implementation
//--------------------------------------------------------------------------------------------------
int drv_swtoe_cnx_rxq_create(int serv_id)
{
    sstoe_rxq_info*     pRXQ     = NULL;
    sstoe_ipc_rxq_info* pRXQ_IPC = NULL;
    int                 i;

    if (1 <= serv_id)
    {
        printk("[%s][%d] illegal ervice Id. (max, requested) = (%d, %d)\n", __FUNCTION__, __LINE__, 1, serv_id);
        return -1;
    }
    pRXQ_IPC = &g_pIpcInfo->rxq_info[serv_id];
    if (1 == pRXQ_IPC->used)
    {
        printk("[%s][%d] RX queue of service %d has been created\n", __FUNCTION__, __LINE__, serv_id);
        return 0;
    }
    pRXQ            = &rxq_info[serv_id];
    pRXQ->frag_size = MAX_RXQ_PKT_SIZE;
    pRXQ_IPC->depth = MAX_RXQ_DEPTH; // @FIXME : dynamic
    // pRXQ_IPC->pkt_size = pRXQ->frag_size - NET_SKB_PAD - NET_IP_ALIGN - SKB_DATA_ALIGN(sizeof(struct
    // skb_shared_info));
    pRXQ_IPC->pkt_size = pRXQ->frag_size;
    // printk("[%s][%d] try to allocate rx desc = %d, %d, %d\n", __FUNCTION__, __LINE__, pRXQ_IPC->depth,
    // sizeof(sstoe_rx_desc_t), pRXQ_IPC->depth*sizeof(sstoe_rx_desc_t));
    pRXQ->rxq = (sstoe_rx_desc_t*)dma_alloc_coherent(NULL, pRXQ_IPC->depth * sizeof(sstoe_rx_desc_t),
                                                     &pRXQ_IPC->desc_addr, GFP_KERNEL);
    if (NULL == pRXQ->rxq)
    {
        printk("[%s][%d] allocate rx descriptor memory fail\n", __FUNCTION__, __LINE__);
        goto create_fail;
    }
    memset(pRXQ->rxq, 0, pRXQ_IPC->depth * sizeof(sstoe_rx_desc_t));
    if (NULL == (pRXQ->buf_list = kzalloc(pRXQ_IPC->depth * sizeof(char*), GFP_KERNEL)))
    {
        printk("[%s][%d] allocate rx buffer list fail\n", __FUNCTION__, __LINE__);
        goto create_fail;
    }

    for (i = 0; i < pRXQ_IPC->depth; i++)
    {
        if (NULL == (pRXQ->buf_list[i] = SWTOE_MALLOC(pRXQ->frag_size)))
        {
            goto create_fail;
        }
        // pRXQ->rxq[i].buf_addr = VIRT2BUS(pRXQ->buf_list[i] + NET_IP_ALIGN);
        pRXQ->rxq[i].buf_addr = VIRT2BUS(pRXQ->buf_list[i]);
        pRXQ->rxq[i].buf_size = 0;
        pRXQ->rxq[i].hw_own   = 1;
    }
    pRXQ_IPC->used = 1;
    pRXQ_IPC->read = pRXQ_IPC->write = 0;

    return 0;
create_fail:
    for (i = 0; i < pRXQ_IPC->depth; i++)
    {
        if (pRXQ->buf_list[i])
        {
            SWTOE_FREE(pRXQ->buf_list[i]);
            pRXQ->buf_list[i] = NULL;
        }
    }
    if (pRXQ->buf_list)
    {
        kfree(pRXQ->buf_list);
        pRXQ->buf_list = NULL;
    }
    if (pRXQ->rxq)
    {
        dma_free_coherent(NULL, pRXQ_IPC->depth * sizeof(sstoe_rx_desc_t), pRXQ->rxq, pRXQ_IPC->desc_addr);
        pRXQ->rxq           = NULL;
        pRXQ_IPC->desc_addr = 0;
    }
    return -ENOMEM;
}

int drv_swtoe_cnx_rxq_get(int* cnx_id, char** buf, int* buf_size, char** data, int* data_size)
{
    sstoe_rxq_info*     pRXQ     = NULL;
    sstoe_ipc_rxq_info* pRXQ_IPC = NULL;
    sstoe_rx_desc_t*    pDesc    = NULL;
    int                 read;
    int                 serv_id = 0;

    pRXQ_IPC = &g_pIpcInfo->rxq_info[serv_id];
    if (NULL == pRXQ_IPC)
    {
        panic("what heppen for NULL pRXQ_IPC, %d", serv_id);
        return -1;
    }
    if (0 == pRXQ_IPC->used)
    {
        // printk("[%s][%d] RX queue of service %d has not been created\n", __FUNCTION__, __LINE__, serv_id);
        return -1;
    }
    if (pRXQ_IPC->read == pRXQ_IPC->write)
    {
        return -2;
    }
    pRXQ  = &rxq_info[serv_id];
    read  = pRXQ_IPC->read;
    pDesc = &pRXQ->rxq[read];
    if (NULL == pDesc)
    {
        panic("what heppen for NULL pRXQ desc %d", read);
        return -1;
    }
    if (pDesc->hw_own)
    {
        printk("[%s][%d] descriptor belongs to RTOS\n", __FUNCTION__, __LINE__);
        return -1;
    }
    *cnx_id    = pDesc->cnx_id;
    *buf       = pRXQ->buf_list[read];
    *buf_size  = pRXQ->frag_size;
    *data      = BUS2VIRT(pDesc->buf_addr);
    *data_size = pDesc->buf_size;

    CNX_SANITY(*cnx_id);

    if (NULL == (pRXQ->buf_list[read] = SWTOE_MALLOC(pRXQ->frag_size)))
    {
        panic("allocaate TOE rx buffer for %d bytes fail]\n", pRXQ->frag_size);
    }
    pDesc->cnx_id   = 0;
    pDesc->buf_addr = VIRT2BUS(pRXQ->buf_list[read]);
    pDesc->buf_size = 0;
    pDesc->hw_own   = 1;

    read++;
    if (read >= pRXQ_IPC->depth)
    {
        read -= pRXQ_IPC->depth;
    }
    pRXQ_IPC->read = read;
    // return (pRXQ_IPC->read == pRXQ_IPC->write) ? 0 : 1;
    return 0;
}

typedef struct
{
    struct list_head  list;
    drv_swtoe_rx_data msg_rx;
} msg_queue_rx;

int drv_swtoe_rx_data_get(int cnx_id, drv_swtoe_rx_data* from, drv_swtoe_rx_data** pp_rx_data)
{
    msg_queue_rx*   p_msg_rx;
    msg_queue_rx*   p_prev = NULL;
    sstoe_cnx_info* pCNX   = &g_pCnxInfo[cnx_id];

    CNX_SANITY(cnx_id);
    *pp_rx_data = NULL;
    if (list_empty(&pCNX->rdata_queue))
    {
        return -1;
    }
    if (!from)
    {
        p_msg_rx    = list_first_entry(&pCNX->rdata_queue, msg_queue_rx, list);
        *pp_rx_data = &p_msg_rx->msg_rx;
        return 0;
    }

    p_prev = container_of(from, msg_queue_rx, msg_rx);
    if (list_is_last(&p_prev->list, &pCNX->rdata_queue))
    {
        return -1;
    }

    p_msg_rx    = list_next_entry(p_prev, list);
    *pp_rx_data = &p_msg_rx->msg_rx;
    return 0;
}

int drv_swtoe_rx_data_free(int cnx_id, drv_swtoe_rx_data* p_rx_data, int free)
{
    msg_queue_rx* p_msg_rx = container_of(p_rx_data, msg_queue_rx, msg_rx);

    CNX_SANITY(cnx_id);
    if (!p_msg_rx)
    {
        printk("[%s][%d] container of fail. 0x%08x\n", __FUNCTION__, __LINE__, (int)p_rx_data);
        return -1;
    }
    list_del(&p_msg_rx->list);

    if ((free) && (p_msg_rx->msg_rx.buf))
    {
        SWTOE_FREE(p_msg_rx->msg_rx.buf);
    }
    kfree(p_msg_rx);
    return 0;
}

int drv_swtoe_rx_data_avail(int cnx_id)
{
    msg_queue_rx*   p_msg_rx;
    sstoe_cnx_info* pCNX = &g_pCnxInfo[cnx_id];

    CNX_SANITY(cnx_id);
    if (list_empty(&pCNX->rdata_queue))
    {
        return -1;
    }
    p_msg_rx = list_first_entry(&pCNX->rdata_queue, msg_queue_rx, list);
    return (p_msg_rx->msg_rx.data_size == p_msg_rx->msg_rx.offset) ? -1 : 0;
    // return (p_msg_rx->msg_rx.data_size) ? 0 : -1;
}

static unsigned long cnx_signal[LONG_DIV(MAX_CNX_TXQ_NUM)] = {0};

int drv_swtoe_rx_data_put(void)
{
    int             ret;
    char *          buf, *data;
    int             buf_size, data_size;
    msg_queue_rx*   p_queue_data_rx = NULL;
    int             i;
    unsigned long*  p;
    long            base, pos;
    int             cnx_id;
    sstoe_cnx_info* pCNX = NULL;

    while (1)
    {
        ret = drv_swtoe_cnx_rxq_get(&cnx_id, &buf, &buf_size, &data, &data_size);
        if (0 > ret)
            break;
        CNX_SANITY(cnx_id);
        if (NULL == (p_queue_data_rx = kmalloc(sizeof(msg_queue_rx), GFP_ATOMIC)))
        {
            printk("[%s][%d] alloc fail. size = %d\n", __FUNCTION__, __LINE__, sizeof(msg_queue_rx));
            continue;
        }
        pCNX                              = &g_pCnxInfo[cnx_id];
        p_queue_data_rx->msg_rx.buf       = buf;
        p_queue_data_rx->msg_rx.buf_size  = buf_size;
        p_queue_data_rx->msg_rx.data      = data;
        p_queue_data_rx->msg_rx.data_size = data_size;
        p_queue_data_rx->msg_rx.offset    = 0;
        list_add_tail(&p_queue_data_rx->list, &pCNX->rdata_queue);

        SET_BITS_POS(cnx_signal, cnx_id);
        if (0 == ret)
        {
            break;
        }
    }
    for (i = 0; i < LONG_DIV(MAX_CNX_TXQ_NUM); i++)
    {
        p    = &cnx_signal[i];
        base = LONG_MULT(i);
        pos  = -1;
        do
        {
            pos = find_next_bit(p, BITS_PER_LONG, pos + 1);
            if (BITS_PER_LONG == pos)
            {
                *p = 0;
                break;
            }
            cnx_id = base + pos;
            pCNX   = &g_pCnxInfo[cnx_id];
            /// if ((HAS_BITS(pCNX->cb_mask, DRV_SWTOE_CB_RCOM)) && (pCNX->cb_func))
            if (1)
            {
                (pCNX->cb_func)(cnx_id, DRV_SWTOE_GLUE_RCOM, NULL, pCNX->cb_data);
            }
        } while (pos < BITS_PER_LONG);
    }
    return 0;
}
