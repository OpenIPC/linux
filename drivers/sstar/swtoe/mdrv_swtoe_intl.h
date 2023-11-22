/*
 * mdrv_swtoe_intl.h- Sigmastar
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

#ifndef _MDRV_SWTOE_INTL_H__
#define _MDRV_SWTOE_INTL_H__

#include "mdrv_swtoe_ipc.h"

//--------------------------------------------------------------------------------------------------
//  Constant definition
//--------------------------------------------------------------------------------------------------
#define MAX_RXQ_DEPTH    256
#define MAX_RXQ_PKT_SIZE 0x4000

#define MAX_TXQ_DEPTH     128
#define MAX_TXQ_PKT_SIZE  0x4000
#define MAX_TXQ_DATA_SIZE 0x3FF0

#define DBG_CNX_SANITY 1
#define DBG_CNX        0
#define DBG_TX_MEM     0
#define DBG_TX_PERF    0

//--------------------------------------------------------------------------------------------------
//  Some helper functions
//--------------------------------------------------------------------------------------------------
#if (BITS_PER_LONG == 32)
#define LONG_DIV_SHFT (0x05)
#define LONG_MOD_MASK (0x1f)
#elif (BITS_PER_LONG == 64)
#define LONG_DIV_SHFT (0x06)
#define LONG_MOD_MASK (0x3f)
#endif

#define LONG_DIV(a)  ((a) >> LONG_DIV_SHFT)
#define LONG_MOD(a)  ((a)&LONG_MOD_MASK)
#define LONG_MULT(a) ((a) << LONG_DIV_SHFT)

#define CLR_BITS(a, pos) clear_bit((pos), &(a))
#define SET_BITS(a, pos) set_bit((pos), &(a))
#define HAS_BITS(a, pos) test_bit((pos), &(a))

#define CLR_BITS_POS(a, pos) clear_bit(LONG_MOD((pos)), &(a)[LONG_DIV((pos))])
#define SET_BITS_POS(a, pos) set_bit(LONG_MOD((pos)), &(a)[LONG_DIV((pos))])
#define HAS_BITS_POS(a, pos) test_bit(LONG_MOD((pos)), &(a)[LONG_DIV((pos))])

#define CLR_FLAGS(a, flags) ((a) & (~(flags)))
#define SET_FLAGS(a, flags) ((a) | (flags))
#define HAS_FLAGS(a, flags) ((a) & (flags))

#define PA2BUS(a) CLR_FLAGS(a, MSTAR_MIU0_BUS_BASE)
#define BUS2PA(a) SET_FLAGS(a, MSTAR_MIU0_BUS_BASE)

#define BUS2VIRT(a) phys_to_virt(BUS2PA((a)))
#define VIRT2BUS(a) PA2BUS(virt_to_phys((a)))

//--------------------------------------------------------------------------------------------------
//  Data structure definition
//--------------------------------------------------------------------------------------------------
typedef struct
{
    /// TX data part
    sstoe_tx_desc_t* tx_desc;
    u16              read_lx;
    void**           RTF; /// Resource To Free
    struct list_head wdata_queue;
#define MAX_CNX_WDATA_QUEUE_SIZE 8
    int wdata_queue_size;

    /// RX data part
    struct list_head rdata_queue; /// _msg_queue_rx

    /// callback function
    drv_swtoe_cb_func_t cb_func; /// swtoe_cb_func
    void*               cb_data; /// swtoe_cb_data
    u32                 cb_mask; /// swtoe_cb_mask

    /// binding
    char bind_ready;

    /// listen data
    struct list_head lstn_queue;

    /// mutex
    spinlock_t lock;

} sstoe_cnx_info;

//--------------------------------------------------------------------------------------------------
//  global variables
//--------------------------------------------------------------------------------------------------
extern sstoe_ipc_ctrl_t* g_pIpcInfo;
extern sstoe_cnx_info*   g_pCnxInfo;

#if DBG_TX_MEM
extern int g_cnt_alloc;
extern int g_cnt_free;
extern int g_cnt_ipc_free;
#endif

//--------------------------------------------------------------------------------------------------
//  malloc helper function
//--------------------------------------------------------------------------------------------------
#define MEM_ALLOC_FRAG 0

#if MEM_ALLOC_FRAG
#define SWTOE_MALLOC(size) netdev_alloc_frag((size))
#define SWTOE_FREE(p)      skb_free_frag((p))
#else
// #define SWTOE_MALLOC(size)          kmalloc((size), GFP_ATOMIC)
#define SWTOE_MALLOC(size) kmalloc((size), GFP_KERNEL)
#define SWTOE_FREE(p)      kfree((p))
#endif

#if LONG_MOD(MAX_CNX_TXQ_NUM)
#pragma GCC error "MAX_CNX_TXQ_NUM should be mutiple of long"
#endif

#if DBG_CNX_SANITY
#define CNX_SANITY(cnx_id)                                                                     \
    {                                                                                          \
        if (MAX_CNX_TXQ_NUM <= (cnx_id))                                                       \
        {                                                                                      \
            printk("[%s][%d] invalid cnx_id %d\n", __FUNCTION__, __LINE__, (cnx_id));          \
        }                                                                                      \
        if (0 == g_pIpcInfo->txq_info[(cnx_id)].used)                                          \
        {                                                                                      \
            printk("[%s][%d] cnx_id = %d is not created\n", __FUNCTION__, __LINE__, (cnx_id)); \
        }                                                                                      \
    }
#else
#define CNX_SANITY(cnx_id) \
    {                      \
    }
#endif

#if DBG_CNX
#define CNX_DUMP(cnx_id)                                                                                              \
    {                                                                                                                 \
        sstoe_ipc_txq_info* pTXQ_IPC = NULL;                                                                          \
        if (MAX_CNX_TXQ_NUM <= (cnx_id))                                                                              \
        {                                                                                                             \
            printk("[%s][%d] invalid cnx_id %d\n", __FUNCTION__, __LINE__, (cnx_id));                                 \
        }                                                                                                             \
        pTXQ_IPC = &g_pIpcInfo->txq_info[(cnx_id)];                                                                   \
        printk("[%s][%d] (cnx, used, prot, pkt_size, depth, write, read) = (%d, %d, %d, %d, %d, %d, %d)\n",           \
               __FUNCTION__, __LINE__, (cnx_id), pTXQ_IPC->used, pTXQ_IPC->prot, pTXQ_IPC->pkt_size, pTXQ_IPC->depth, \
               pTXQ_IPC->write, pTXQ_IPC->read);                                                                      \
    }
#else
#define CNX_DUMP(cnx_id) \
    {                    \
    }
#endif

#if DBG_TX_MEM
#define TX_MEM_ALLOC_INC() \
    {                      \
        g_cnt_alloc++;     \
    }
#define TX_MEM_FREE_INC() \
    {                     \
        g_cnt_free++;     \
    }
#define TX_MEM_IPC_FREE_INC() \
    {                         \
        g_cnt_ipc_free++;     \
    }
#define TX_MEM_INFO_DUMP()                                                                                      \
    {                                                                                                           \
        printk("[%s][%d] TX mem (alloc, free, free_ipc) = (%d, %d, %d)\n", __FUNCTION__, __LINE__, g_cnt_alloc, \
               g_cnt_free, g_cnt_ipc_free);                                                                     \
    }
#else
#define TX_MEM_ALLOC_INC() \
    {                      \
    }
#define TX_MEM_FREE_INC() \
    {                     \
    }
#define TX_MEM_IPC_FREE_INC() \
    {                         \
    }
#define TX_MEM_INFO_DUMP() \
    {                      \
    }
#endif

#if DBG_TX_PERF
#define CNX_PERF_TXQ_UNDERRUN(cnx_id)                                                         \
    {                                                                                         \
        if (g_pIpcInfo->txq_info[(cnx_id)].read == g_pIpcInfo->txq_info[(cnx_id)].write)      \
        {                                                                                     \
            printk("[%s][%d] TXQ underrun. cnx_id = %d\n", __FUNCTION__, __LINE__, (cnx_id)); \
        }                                                                                     \
    }
#else
#define CNX_PERF_TXQ_UNDERRUN(cnx_id) \
    {                                 \
    }
#endif

// #define CNX_LOCK(pCNX, flags)           spin_lock_irqsave(&(pCNX)->lock, (flags))
// #define CNX_UNLOCK(pCNX, flags)         spin_unlock_irqrestore(&(pCNX)->lock, (flags))

//--------------------------------------------------------------------------------------------------
//  function declaration
//--------------------------------------------------------------------------------------------------
int drv_swtoe_glue_init(void);
int drv_swtoe_glue_invoke(u32 arg1, u32 arg2, u32 arg3);

/// @FIXME : temporarily put it here
int drv_swtoe_tx_consume(int cnx_id);

/// rx
int drv_swtoe_rx_data_put(void);

/// listen
int drv_swtoe_lstn_resp(int cnx_id);

// txq/rxq can be put in private header file
int drv_swtoe_cnx_txq_create(int prot, int* cnx_id);

// int drv_swtoe_cnx_txq_put(int cnx_id, struct sk_buff* skb);
int drv_swtoe_cnx_txq_free(int cnx_id);
// int drv_swtoe_cnx_txq_avail(int cnx_id, int n);
// int drv_swtoe_cnx_txq_put(int cnx_id, dma_addr_t addr, int size);
// int drv_swtoe_cnx_txq_size(int cnx_id);
// int drv_swtoe_cnx_txq_free(int cnx_id);
// int drv_swtoe_cnx_txq_used(int cnx_id);
// int drv_swtoe_cnx_txq_full(int cnx_id);
// int drv_swtoe_cnx_txq_empty(int cnx_id);

// int drv_swtoe_cnx_rxq_create(int serv_id);
// int drv_swtoe_cnx_rxq_close(int cnx_id);
int drv_swtoe_cnx_rxq_create(int serv_id);
int drv_swtoe_cnx_rxq_get(int* cnx_id, char** buf, int* buf_size, char** data, int* data_size);
// int drv_swtoe_cnx_rxq_put(int cnx_id, dma_addr_t addr, int size);
int drv_swtoe_cnx_rxq_size(int cnx_id);
int drv_swtoe_cnx_rxq_free(int cnx_id);
int drv_swtoe_cnx_rxq_used(int cnx_id);

#endif
