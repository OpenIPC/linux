/*
 * mdrv_swtoe_txq.c- Sigmastar
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
#include "ms_platform.h"
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
    struct list_head list;
    void*            p;
    u32              size : 16;
    u32              data_cat : 2;
    u32              reserved1 : 14;
    void*            p2;
} prequeue_data;

//--------------------------------------------------------------------------------------------------
//  Helper functions
//--------------------------------------------------------------------------------------------------
#define SANITY_DESC_LX_PRIVATE(p)                                                           \
    {                                                                                       \
        if (HAS_FLAGS((int)(p), 0x1))                                                       \
        {                                                                                   \
            printk("[%s][%d] address is odd = 0x%08x\n", __FUNCTION__, __LINE__, (int)(p)); \
        }                                                                                   \
    }

//--------------------------------------------------------------------------------------------------
//  Data definition
//--------------------------------------------------------------------------------------------------
#if DBG_TX_MEM
int g_cnt_alloc    = 0;
int g_cnt_free     = 0;
int g_cnt_ipc_free = 0;
#endif

//--------------------------------------------------------------------------------------------------
//  Forward declaration
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Implementation
//--------------------------------------------------------------------------------------------------
int drv_swtoe_cnx_txq_create(int prot, int* cnx_id)
{
    sstoe_cnx_info*     pCNX     = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    int                 ipc_ret;
    int                 i;

    switch (prot)
    {
        case DRV_SWTOE_PROT_BYPASS:
        case DRV_SWTOE_PROT_TCP:
        case DRV_SWTOE_PROT_UDP:
            break;
        default:
            printk("[%s][%d] unknown txq protocol type %d\n", __FUNCTION__, __LINE__, prot);
            return -1;
    }
    for (i = 0; i < MAX_CNX_TXQ_NUM; i++)
    {
        if (0 == g_pIpcInfo->txq_info[i].used)
        {
            pTXQ_IPC = &g_pIpcInfo->txq_info[i];
            pCNX     = &g_pCnxInfo[i];
            break;
        }
    }
    if (NULL == pTXQ_IPC)
    {
        printk("[%s][%d] unable to find a free connection\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pCNX->tx_desc = (sstoe_tx_desc_t*)dma_alloc_coherent(NULL, MAX_TXQ_DEPTH * sizeof(sstoe_tx_desc_t),
                                                         &pTXQ_IPC->desc_addr, GFP_ATOMIC);
    if (NULL == pCNX->tx_desc)
    {
        printk("[%s][%d] allocate tx descriptor memory fail\n", __FUNCTION__, __LINE__);
        goto create_fail;
    }
    if (!(pCNX->RTF = kzalloc(MAX_TXQ_DEPTH * sizeof(void*), GFP_KERNEL)))
    {
        printk("[%s][%d] allocate RTF buffer fail\n", __FUNCTION__, __LINE__);
        goto create_fail;
    }
    memset(pCNX->tx_desc, 0, MAX_TXQ_DEPTH * sizeof(sstoe_tx_desc_t));
    pTXQ_IPC->depth    = MAX_TXQ_DEPTH;    // @FIXME : dynamic
    pTXQ_IPC->pkt_size = MAX_TXQ_PKT_SIZE; // @FIXME : dynamic
    pTXQ_IPC->used     = 1;
    pTXQ_IPC->prot     = prot;
    pTXQ_IPC->write    = 0;
    pTXQ_IPC->read = pCNX->read_lx = 0;
    pTXQ_IPC->desc_addr            = PA2BUS(pTXQ_IPC->desc_addr);
    Chip_Flush_MIU_Pipe();
    INIT_LIST_HEAD(&pCNX->wdata_queue);
    pCNX->wdata_queue_size = 0;
    INIT_LIST_HEAD(&pCNX->rdata_queue);
    INIT_LIST_HEAD(&pCNX->lstn_queue);

    spin_lock_init(&pCNX->lock);
    pCNX->cb_func    = NULL;
    pCNX->cb_data    = NULL;
    pCNX->cb_mask    = 0;
    pCNX->bind_ready = 0;

    if ((ipc_ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_CTRL_TXQ_ALLOC, i, sizeof(sstoe_ipc_txq_info) & 0xFFFF)))
    {
        printk("[%s][%d] TX packet IPC fail %d\n", __FUNCTION__, __LINE__, ipc_ret);
        return -1;
    }
    *cnx_id = i;
    return 0;

create_fail:
    if (pCNX->RTF)
    {
        kfree(pCNX->RTF);
        pCNX->RTF = NULL;
    }
    if (pTXQ_IPC->desc_addr)
    {
        dma_free_coherent(NULL, pTXQ_IPC->depth * sizeof(sstoe_tx_desc_t), pCNX->tx_desc, BUS2PA(pTXQ_IPC->desc_addr));
        pCNX->tx_desc = NULL;
        memset(pTXQ_IPC, 0, sizeof(*pTXQ_IPC));
    }
    return -ENOMEM;
}

int drv_swtoe_cnx_txq_free(int cnx_id)
{
    sstoe_cnx_info* pCNX = NULL;
    // sstoe_tx_desc_t* pDesc = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;

    CNX_SANITY(cnx_id);
    pCNX     = &g_pCnxInfo[cnx_id];
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];

    if (pTXQ_IPC->desc_addr)
    {
        dma_free_coherent(NULL, pTXQ_IPC->depth * sizeof(sstoe_tx_desc_t), pCNX->tx_desc, BUS2PA(pTXQ_IPC->desc_addr));
        pCNX->tx_desc = NULL;
        memset(pTXQ_IPC, 0, sizeof(*pTXQ_IPC));
    }
    return 0;
}

static int _drv_swtoe_tx_avail(int cnx_id, int n)
{
    sstoe_cnx_info*     pCNX     = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    sstoe_tx_desc_t*    pDesc    = NULL;
    u16                 write;
    u16                 read;
    int                 space;

    CNX_SANITY(cnx_id);
    if (0 >= n)
        return 0;

    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];
    pCNX     = &g_pCnxInfo[cnx_id];
    if (pTXQ_IPC->depth < n)
        return -1;
    write = pTXQ_IPC->write;
    pDesc = &(pCNX->tx_desc[write]);
    if (pDesc->hw_own)
        return -1;
    read  = pCNX->read_lx;
    space = (write >= read) ? pTXQ_IPC->depth - write + read : (read - write);
    space--;
    return (space >= n) ? 0 : -1;
}

int drv_swtoe_tx_avail(int cnx_id, int n)
{
    return _drv_swtoe_tx_avail(cnx_id, n);
}

int drv_swtoe_tx_consume(int cnx_id)
{
    sstoe_cnx_info*     pCNX     = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    sstoe_tx_desc_t*    pDesc    = NULL;
    u16                 read;
    u16                 read_lx;
    u16                 depth;
    void*               p;
    int                 cnt = 0;

    pCNX     = &g_pCnxInfo[cnx_id];
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];
    read     = pTXQ_IPC->read;
    read_lx  = pCNX->read_lx;

    if (read_lx == pTXQ_IPC->read)
    {
        return 0;
    }

    depth = pTXQ_IPC->depth;

    while (read_lx != read)
    {
        pDesc = &(pCNX->tx_desc[read_lx]);
        p     = pCNX->RTF[read_lx];
        TX_MEM_FREE_INC();
        if (NULL == p)
        {
            /// iov copy
            p = BUS2VIRT(pDesc->buf_addr);
            // printk("[%s][%d] free memory =0x%08x\n", __FUNCTION__, __LINE__, (int)p);
            SWTOE_FREE(p);
        }
        else if (HAS_FLAGS((int)p, 0x1))
        {
            /// pages
            p = (void*)CLR_FLAGS((int)p, 0x1);
            // printk("[%s][%d] free page =0x%08x\n", __FUNCTION__, __LINE__, (int)p);
            put_page((struct page*)p);
        }
        else
        {
            // printk("[%s][%d] free skb =0x%08x\n", __FUNCTION__, __LINE__, (int)p);
            /// skb
            dev_kfree_skb_any((struct sk_buff*)p);
        }
        memset(pDesc, 0, sizeof(*pDesc));
        read_lx++;
        cnt++;
        if (read_lx >= depth)
        {
            read_lx = 0;
        }
    }
    pCNX->read_lx = read_lx;
    Chip_Flush_MIU_Pipe();
    return cnt;
}

static int _drv_swtoe_txq_preput(int cnx_id, void* p, int size, int data_cat, void* p2)
{
    prequeue_data*  preq_data;
    sstoe_cnx_info* pCNX = NULL;

    CNX_SANITY(cnx_id);
    if (NULL == (preq_data = kmalloc(sizeof(prequeue_data), GFP_KERNEL)))
    {
        printk("[%s][%d] kmalloc %d byte fail.\n", __FUNCTION__, __LINE__, sizeof(prequeue_data));
        return -1;
    }
    pCNX                = &g_pCnxInfo[cnx_id];
    preq_data->p        = p;
    preq_data->size     = size & 0xFFFF;
    preq_data->data_cat = data_cat & 0x3;
    preq_data->p2       = p2;
    list_add_tail(&preq_data->list, &pCNX->wdata_queue);
    pCNX->wdata_queue_size++;
    return 0;
}

static int _drv_swtoe_tx_pump_skb(int cnx_id, struct sk_buff* skb)
{
    sstoe_cnx_info*     pCNX     = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    sstoe_tx_desc_t*    pDesc    = NULL;
    int                 nr_frags = skb_shinfo(skb)->nr_frags;
    int                 i;
    int                 len = 0;
    u16                 write;
    u16                 depth;
    void**              pRTF = NULL;

    if (_drv_swtoe_tx_avail(cnx_id, nr_frags + 1))
    {
        return 0;
    }

    pCNX     = &g_pCnxInfo[cnx_id];
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];
    write    = pTXQ_IPC->write;
    depth    = pTXQ_IPC->depth;
    pDesc    = &(pCNX->tx_desc[write]);
    pRTF     = &pCNX->RTF[write];

    Chip_Flush_Cache_Range((size_t)skb->data, skb_headlen(skb));
    pDesc->buf_size = skb_headlen(skb);
    pDesc->buf_addr = VIRT2BUS(skb->data);
    pDesc->eof      = 0;
    *pRTF           = 0;
    pDesc->hw_own   = 1;
    write++;
    if (write >= depth)
    {
        write = 0;
    }
    len += pDesc->buf_size;

    for (i = 0; i < nr_frags; i++)
    {
        struct skb_frag_struct* frag = &skb_shinfo(skb)->frags[i];
        pDesc                        = &(pCNX->tx_desc[write]);
        pRTF                         = &pCNX->RTF[write];

        if (pDesc->hw_own)
        {
            panic("[%s][%d] should not happen. no descriptor space for skb\n", __FUNCTION__, __LINE__);
            return -1;
        }
        Chip_Flush_Cache_Range((size_t)skb_frag_address(frag), skb_frag_size(frag));
        pDesc->buf_size = skb_frag_size(frag);
        pDesc->buf_addr = VIRT2BUS(skb_frag_address(frag));
        pDesc->eof      = 0;
        *pRTF           = 0;
        pDesc->hw_own   = 1;
        write++;
        if (write >= depth)
        {
            write = 0;
        }
        len += pDesc->buf_size;
    }
    pDesc->eof = 1;
    SANITY_DESC_LX_PRIVATE(skb);
    *pRTF = (void*)skb;

    pTXQ_IPC->write = write;
    Chip_Flush_MIU_Pipe();
    if (len > pTXQ_IPC->pkt_size)
    {
        panic("[%s][%d] TX packet size exceed max TX packet size (%d, %d)\n", __FUNCTION__, __LINE__, len,
              pTXQ_IPC->pkt_size);
        return -1;
    }
    return nr_frags + 1;
}

static int _drv_swtoe_tx_pump_iov_page(int cnx_id, char* p, int size, void* p2)
{
    sstoe_cnx_info*     pCNX     = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    sstoe_tx_desc_t*    pDesc    = NULL;
    u16                 write;
    u16                 depth;
    void**              pRTF;

    if (_drv_swtoe_tx_avail(cnx_id, 1))
    {
        return 0;
    }
    pCNX     = &g_pCnxInfo[cnx_id];
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];
    if (size > pTXQ_IPC->pkt_size)
    {
        panic("[%s][%d] TX packet size exceed max TX packet size (%d, %d)\n", __FUNCTION__, __LINE__, size,
              pTXQ_IPC->pkt_size);
        return 0;
    }
    Chip_Flush_Cache_Range((size_t)p, size);

    write = pTXQ_IPC->write;
    depth = pTXQ_IPC->depth;

    pDesc           = &(pCNX->tx_desc[write]);
    pRTF            = &pCNX->RTF[write];
    pDesc->buf_size = size;
    pDesc->buf_addr = VIRT2BUS(p);
    pDesc->eof      = 1;
    if (pDesc->buf_size != size)
    {
        panic("[%s][%d] (desc size, data size) = (0x%08x, 0x%8x)\n", __FUNCTION__, __LINE__, pDesc->buf_size, size);
    }

    SANITY_DESC_LX_PRIVATE(p2);
    *pRTF         = (void*)((p2) ? (long)(p2) | 0x1 : 0);
    pDesc->hw_own = 1;
    Chip_Flush_MIU_Pipe();

    write++;
    if (write >= depth)
    {
        write = 0;
    }
    pTXQ_IPC->write = write;
    return 1;
}

int drv_swtoe_tx_pump(int cnx_id)
{
    sstoe_cnx_info*     pCNX     = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    int                 ipc_ret;
    int                 prot;
    struct list_head *  p_list, *p_list_tmp;
    prequeue_data*      preq_data;
    int                 nDesc, nPump;

    CNX_SANITY(cnx_id);
    pCNX = &g_pCnxInfo[cnx_id];

    if (list_empty(&pCNX->wdata_queue))
    {
        return 0;
    }
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];
    prot     = pTXQ_IPC->prot;

    nPump = 0;
    list_for_each_safe(p_list, p_list_tmp, &pCNX->wdata_queue)
    {
        preq_data = list_entry(p_list, prequeue_data, list);
        // nDesc = (DRV_SWTOE_PROT_BYPASS == prot) ? _drv_swtoe_tx_pump_skb(cnx_id, (struct sk_buff*)preq_data->p) :
        // _drv_swtoe_tx_pump_iov(cnx_id, preq_data->p, preq_data->size);
        switch (preq_data->data_cat)
        {
            case SWTOE_TX_SEND_SKB:
                nDesc = _drv_swtoe_tx_pump_skb(cnx_id, (struct sk_buff*)preq_data->p);
                break;
            case SWTOE_TX_SEND_IOV:
            case SWTOE_TX_SEND_PAGE:
                nDesc = _drv_swtoe_tx_pump_iov_page(cnx_id, preq_data->p, preq_data->size, preq_data->p2);
                break;
            default:
                printk("[%s][%d] unknown data catagory %d\n", __FUNCTION__, __LINE__, preq_data->data_cat);
                break;
        }
        if (!nDesc)
        {
            break;
        }
        nPump += nDesc;
        list_del(&preq_data->list);
        kfree(preq_data);
        pCNX->wdata_queue_size--;
    }
    if (nPump)
    {
        if ((ipc_ret = drv_swtoe_glue_invoke(SWTOE_IPC_ARG1_TXQ_PUT, cnx_id, 0)))
        {
            printk("[%s][%d] IPC SWTOE_IPC_ARG1_TXQ_PUT fail. (cnx_id, ret)  = (%d, %d)\n", __FUNCTION__, __LINE__,
                   cnx_id, ipc_ret);
        }
        return -1;
    }
    return 0;
}

int drv_swtoe_tx_send(int cnx_id, void* p, int size, int offset, int data_cat)
{
    int                 copy     = 0;
    struct iov_iter*    iov      = NULL;
    sstoe_cnx_info*     pCNX     = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    void*               p1       = NULL;

    CNX_SANITY(cnx_id);
    pCNX     = &g_pCnxInfo[cnx_id];
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];

    drv_swtoe_tx_pump(cnx_id);

    if (MAX_CNX_WDATA_QUEUE_SIZE <= pCNX->wdata_queue_size)
    {
        return 0;
    }
    if (SWTOE_TX_SEND_SKB == data_cat)
    {
        if (_drv_swtoe_txq_preput(cnx_id, p, 0, SWTOE_TX_SEND_SKB, 0))
        {
            printk("[%s][%d] _drv_swtoe_txq_preput fail\n", __FUNCTION__, __LINE__);
            goto jmp_err;
        }
        CNX_PERF_TXQ_UNDERRUN(cnx_id);
        return 1;
    }

    TX_MEM_INFO_DUMP();
    CNX_DUMP(cnx_id);

    if (SWTOE_TX_SEND_IOV == data_cat)
    {
        iov  = (struct iov_iter*)p;
        copy = min_t(size_t, iov_iter_count(iov), MAX_TXQ_DATA_SIZE);
        TX_MEM_ALLOC_INC();
        if (NULL == (p1 = SWTOE_MALLOC(copy)))
        {
            printk("[%s][%d] allocate %d bytes fail\n", __FUNCTION__, __LINE__, copy);
            return 0;
        }
        if (copy != copy_from_iter(p1, copy, iov))
        {
            printk("[%s][%d] copy_from_iter. what happen\n", __FUNCTION__, __LINE__);
            goto jmp_err;
        }
        if (_drv_swtoe_txq_preput(cnx_id, p1, copy, SWTOE_TX_SEND_IOV, 0))
        {
            printk("[%s][%d] _drv_swtoe_txq_preput fail\n", __FUNCTION__, __LINE__);
            goto jmp_err;
        }
        CNX_PERF_TXQ_UNDERRUN(cnx_id);
        return copy;
    }
    if (SWTOE_TX_SEND_PAGE == data_cat)
    {
        struct page* pge = (struct page*)p;

        copy = min_t(size_t, size, MAX_TXQ_DATA_SIZE);
        p1   = page_address(pge) + offset;
        if (_drv_swtoe_txq_preput(cnx_id, p1, copy, SWTOE_TX_SEND_PAGE, (void*)pge))
        {
            printk("[%s][%d] _drv_swtoe_txq_preput fail\n", __FUNCTION__, __LINE__);
            goto jmp_err;
        }
        get_page(pge);
        CNX_PERF_TXQ_UNDERRUN(cnx_id);
        return copy;
    }
    return copy;
jmp_err:
    if (p1)
    {
        SWTOE_FREE(p1);
        // kfree(p1);
    }
    return 0;
}
