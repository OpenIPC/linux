/*
 * mdrv_swtoe_cb.c- Sigmastar
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
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    u32 blk : 1;
    u32 status : 1;
    u32 reserved : 30;
} __attribute__((packed)) sstoe_prot_cnx_resp_t;

typedef struct
{
    u32 prot_cmd;
    u32 cnx_id : 11;
    u32 reserved : 21;
    union
    {
        sstoe_prot_cnx_resp_t cnx_resp;
    } u;
} __attribute__((packed)) sstoe_prot_info_t;

typedef struct
{
    struct list_head  list;
    sstoe_prot_info_t prot_info;
} _ipcq_data_t;

//--------------------------------------------------------------------------------------------------
//  Forward declaration
//--------------------------------------------------------------------------------------------------
static int _drv_swtoe_glue_ctrl_alloc(void);

// static void swtoe_work_func(unsigned long data);
static void swtoe_work_func(struct work_struct* work);

//--------------------------------------------------------------------------------------------------
//  Data definition
//--------------------------------------------------------------------------------------------------
static struct work_struct swtoe_work;
static DEFINE_SPINLOCK(mutex_L2R);
static DEFINE_SPINLOCK(mutex_R2L);

// RXQ
#define R2L_SIGNAL_RCOM 0
#define R2L_SIGNAL_TCOM 1
static unsigned long r2l_signal                                 = 0; /// @FIXME: refine this one
static unsigned long _cnx_signal_r2l[LONG_DIV(MAX_CNX_TXQ_NUM)] = {0};

#define L2R_SIGNAL_XMIT 0
static unsigned long l2r_signal                                 = 0; /// @FIXME: refine this one
static unsigned long _cnx_signal_l2r[LONG_DIV(MAX_CNX_TXQ_NUM)] = {0};
static unsigned long _cnx_signal_wq[LONG_DIV(MAX_CNX_TXQ_NUM)]  = {0};

// IPC control
sstoe_ipc_ctrl_t* g_pIpcInfo     = NULL;
static dma_addr_t _ipc_ctrl_phys = 0;

// connection information
static sstoe_cnx_info cnx_info[MAX_CNX_TXQ_NUM] = {0};
sstoe_cnx_info*       g_pCnxInfo                = cnx_info;

// ipc command queue from RTK to Linux
static struct list_head _ipcq_r2l;

/// @FIXME : end : collect these data structure into one

//--------------------------------------------------------------------------------------------------
//  Implementation
//--------------------------------------------------------------------------------------------------
int drv_swtoe_glue_init(void)
{
    int ret = 0;

    if ((ret = _drv_swtoe_glue_ctrl_alloc()))
    {
        return ret;
    }
    INIT_WORK(&swtoe_work, swtoe_work_func);
    INIT_LIST_HEAD(&_ipcq_r2l);
    return 0;
}

int drv_swtoe_glue_req(int cnx_id, drv_swtoe_cb_func_t cb, void* cb_data)
{
    sstoe_cnx_info* pCNX = &g_pCnxInfo[cnx_id];

    CNX_SANITY(cnx_id);
    pCNX->cb_func = cb;
    pCNX->cb_data = cb_data;

    return 0;
}

int drv_swtoe_glue_en(int cnx_id, int mask, int bEn)
{
    sstoe_cnx_info* pCNX = &g_pCnxInfo[cnx_id];

    CNX_SANITY(cnx_id);
    /// @FIXME : to be refined
    pCNX->cb_mask = (bEn) ? SET_FLAGS(pCNX->cb_mask, mask) : CLR_FLAGS(pCNX->cb_mask, mask);
    return 0;
}

int drv_swtoe_glue_invoke(u32 arg1, u32 arg2, u32 arg3)
{
    unsigned long flag;

    spin_lock_irqsave(&mutex_L2R, flag);
    switch (arg1)
    {
        case SWTOE_IPC_ARG1_TXQ_PUT:
            // txq_signal[arg2 >> 5] |= (1 << (arg2 & 0x1f));
            SET_BITS_POS(_cnx_signal_l2r, arg2);
            SET_BITS(l2r_signal, L2R_SIGNAL_XMIT);
            break;
    }
    spin_unlock_irqrestore(&mutex_L2R, flag);
    schedule_work(&swtoe_work);
    return 0;
}

static _ipcq_data_t* _drv_swtoe_cb_hdl_cnx_resp(u32 arg2, u32 arg3)
{
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    int                 cnx_id   = (int)arg2 & 0xFFFF;
    _ipcq_data_t*       pqdata   = NULL;

    CNX_SANITY(cnx_id);
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];
    if (NULL == (pqdata = kmalloc(sizeof(_ipcq_data_t), GFP_ATOMIC)))
    {
        printk("[%s][%d] kmalloc %d bytes fail\n", __FUNCTION__, __LINE__, sizeof(_ipcq_data_t));
        return NULL;
    }
    if (pTXQ_IPC->cnx_info.status)
    {
        pqdata->prot_info.prot_cmd = DRV_SWTOE_GLUE_CNX_FAIL;
        pqdata->prot_info.cnx_id   = cnx_id;
    }
    else
    {
        pqdata->prot_info.prot_cmd          = DRV_SWTOE_GLUE_CNX_OK;
        pqdata->prot_info.cnx_id            = cnx_id;
        pqdata->prot_info.u.cnx_resp.saddr  = htonl(pTXQ_IPC->cnx_info.saddr);
        pqdata->prot_info.u.cnx_resp.daddr  = htonl(pTXQ_IPC->cnx_info.daddr);
        pqdata->prot_info.u.cnx_resp.sport  = htons(pTXQ_IPC->cnx_info.sport);
        pqdata->prot_info.u.cnx_resp.dport  = htons(pTXQ_IPC->cnx_info.dport);
        pqdata->prot_info.u.cnx_resp.blk    = pTXQ_IPC->cnx_info.blk;
        pqdata->prot_info.u.cnx_resp.status = pTXQ_IPC->cnx_info.status;
    }
    return pqdata;
}

static _ipcq_data_t* _drv_swtoe_cb_hdl_cls_resp(u32 arg2, u32 arg3)
{
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    int                 cnx_id   = (int)arg2 & 0xFFFF;
    _ipcq_data_t*       pqdata   = NULL;

    CNX_SANITY(cnx_id);
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];
    if (NULL == (pqdata = kmalloc(sizeof(_ipcq_data_t), GFP_ATOMIC)))
    {
        printk("[%s][%d] kmalloc %d bytes fail\n", __FUNCTION__, __LINE__, sizeof(_ipcq_data_t));
        return NULL;
    }
    pqdata->prot_info.prot_cmd = DRV_SWTOE_GLUE_CLS_RESP;
    pqdata->prot_info.cnx_id   = cnx_id;
    return pqdata;
}

static void _drv_swtoe_cb_hdl_bind_resp(u32 arg2, u32 arg3)
{
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    int                 cnx_id   = (int)arg2 & 0xFFFF;
    sstoe_cnx_info*     pCNX     = NULL;

    CNX_SANITY(cnx_id);
    pTXQ_IPC         = &g_pIpcInfo->txq_info[cnx_id];
    pCNX             = &g_pCnxInfo[cnx_id];
    pCNX->bind_ready = (0x80 | pTXQ_IPC->cnx_info.status);
}

static _ipcq_data_t* _drv_swtoe_cb_hdl_lstn_resp(u32 arg2, u32 arg3)
{
    int           cnx_id = (int)arg2 & 0xFFFF;
    _ipcq_data_t* pqdata = NULL;

    CNX_SANITY(cnx_id);
    drv_swtoe_lstn_resp(cnx_id);

    if (NULL == (pqdata = kmalloc(sizeof(_ipcq_data_t), GFP_ATOMIC)))
    {
        printk("[%s][%d] kmalloc %d bytes fail\n", __FUNCTION__, __LINE__, sizeof(_ipcq_data_t));
        return NULL;
    }
    pqdata->prot_info.prot_cmd = DRV_SWTOE_GLUE_LSTN_RESP;
    pqdata->prot_info.cnx_id   = cnx_id;
    return pqdata;
}

int drv_swtoe_cb_hdl(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    char          bWakeUp = 1;
    unsigned long flag;
    _ipcq_data_t* pqdata = NULL;
    int           cnx_id = 0;

    if (SWTOE_IPC_ARG0 != arg0)
    {
        printk("[%s][%d] invalid IPC CMD from RTK (arg0, arg1, arg2, arg3) = (0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
               __FUNCTION__, __LINE__, (int)arg0, (int)arg1, (int)arg2, (int)arg3);
        return -1;
    }

    switch (arg1)
    {
        case SWTOE_IPC_ARG1_TXQ_GET:
            spin_lock_irqsave(&mutex_R2L, flag);
            SET_BITS_POS(_cnx_signal_r2l, arg2);
            SET_BITS(r2l_signal, R2L_SIGNAL_TCOM);
            TX_MEM_IPC_FREE_INC();
            spin_unlock_irqrestore(&mutex_R2L, flag);
            break;
        case SWTOE_IPC_ARG1_RXQ_GET:
            cnx_id = arg2 & 0x0000FFFF;
            // @FIXME : only trigger connection 0 for bypassing mode at this stage
            spin_lock_irqsave(&mutex_R2L, flag);
            SET_BITS(r2l_signal, R2L_SIGNAL_RCOM);
            spin_unlock_irqrestore(&mutex_R2L, flag);
            break;
        case SWTOE_IPC_ARG1_PROT_CNX_RESP:
            pqdata = _drv_swtoe_cb_hdl_cnx_resp(arg2, arg3);
            break;
        case SWTOE_IPC_ARG1_PROT_BIND_RESP:
            _drv_swtoe_cb_hdl_bind_resp(arg2, arg3);
            break;
        case SWTOE_IPC_ARG1_PROT_LSTN_RESP:
            pqdata = _drv_swtoe_cb_hdl_lstn_resp(arg2, arg3);
            break;
        case SWTOE_IPC_ARG1_PROT_CLS_RESP:
            pqdata = _drv_swtoe_cb_hdl_cls_resp(arg2, arg3);
            break;
        default:
            printk("[%s][%d] invalid IPC CMD from RTK (arg0, arg1, arg2, arg3) = (0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
                   __FUNCTION__, __LINE__, (int)arg0, (int)arg1, (int)arg2, (int)arg3);
            bWakeUp = 0;
            break;
    }
    if (pqdata)
    {
        spin_lock_irqsave(&mutex_R2L, flag);
        list_add_tail(&pqdata->list, &_ipcq_r2l);
        spin_unlock_irqrestore(&mutex_R2L, flag);
    }

    if (bWakeUp)
    {
        schedule_work(&swtoe_work);
    }
    return 0;
}

static void _swtoe_work_func_prot_cnx_resp(_ipcq_data_t* pqdata)
{
    drv_swtoe_glue_cnx_data cnx_data;
    int                     cnx_id = pqdata->prot_info.cnx_id;
    sstoe_cnx_info*         pCNX   = &g_pCnxInfo[cnx_id];

    if (DRV_SWTOE_GLUE_CNX_FAIL == pqdata->prot_info.prot_cmd)
    {
        (pCNX->cb_func)(cnx_id, DRV_SWTOE_GLUE_CNX_FAIL, NULL, pCNX->cb_data);
        return;
    }

    cnx_data.saddr = pqdata->prot_info.u.cnx_resp.saddr;
    cnx_data.daddr = pqdata->prot_info.u.cnx_resp.daddr;
    cnx_data.sport = pqdata->prot_info.u.cnx_resp.sport;
    cnx_data.dport = pqdata->prot_info.u.cnx_resp.dport;
    cnx_data.blk   = pqdata->prot_info.u.cnx_resp.blk;
    (pCNX->cb_func)(cnx_id, DRV_SWTOE_GLUE_CNX_OK, (void*)&cnx_data, pCNX->cb_data);
}

static void _swtoe_work_func_prot_cls_resp(_ipcq_data_t* pqdata)
{
    int             cnx_id = pqdata->prot_info.cnx_id;
    sstoe_cnx_info* pCNX   = &g_pCnxInfo[cnx_id];

    drv_swtoe_cnx_txq_free(cnx_id);
    (pCNX->cb_func)(cnx_id, DRV_SWTOE_GLUE_CLS_RESP, NULL, pCNX->cb_data);
    drv_swtoe_cnx_txq_free(cnx_id);
}

static void _swtoe_work_func_prot_lstn_resp(_ipcq_data_t* pqdata)
{
    int                     cnx_id = pqdata->prot_info.cnx_id;
    sstoe_cnx_info*         pCNX   = &g_pCnxInfo[cnx_id];
    drv_swtoe_glue_cnx_data lstn_data;

    lstn_data.saddr = pqdata->prot_info.u.cnx_resp.saddr;
    lstn_data.daddr = pqdata->prot_info.u.cnx_resp.daddr;
    lstn_data.sport = pqdata->prot_info.u.cnx_resp.sport;
    lstn_data.dport = pqdata->prot_info.u.cnx_resp.dport;
    (pCNX->cb_func)(cnx_id, DRV_SWTOE_GLUE_LSTN_RESP, (void*)&lstn_data, pCNX->cb_data);
}

static void _swtoe_work_func_prot(void)
{
    _ipcq_data_t*     pqdata = NULL;
    struct list_head *p_list, *p_list_tmp;
    // unsigned long flag;

    // @FIXME : spinlock is temporarily removed since listen response, however ...
    // spin_lock_irqsave(&mutex_R2L, flag);
    if (list_empty(&_ipcq_r2l))
    {
        // spin_unlock_irqrestore(&mutex_R2L, flag);
        return;
    }

    list_for_each_safe(p_list, p_list_tmp, &_ipcq_r2l)
    {
        pqdata = list_entry(p_list, _ipcq_data_t, list);
        switch (pqdata->prot_info.prot_cmd)
        {
            case DRV_SWTOE_GLUE_CNX_OK:
            case DRV_SWTOE_GLUE_CNX_FAIL:
                _swtoe_work_func_prot_cnx_resp(pqdata);
                break;
            case DRV_SWTOE_GLUE_CLS_RESP:
                _swtoe_work_func_prot_cls_resp(pqdata);
                break;
            case DRV_SWTOE_GLUE_LSTN_RESP:
                _swtoe_work_func_prot_lstn_resp(pqdata);
                break;
            default:
                printk("[%s][%d] invalid protocol CMD 0x%08x\n", __FUNCTION__, __LINE__,
                       (int)pqdata->prot_info.prot_cmd);
                break;
        }
        list_del(&pqdata->list);
        kfree(pqdata);
    }
    // spin_unlock_irqrestore(&mutex_R2L, flag);
}

static void swtoe_work_func_xmit(unsigned long* cnx_signal)
{
    int            i;
    unsigned long* p;
    long           base, pos;
    int            cnx_id;

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
            /// if ((HAS_BITS(pCNX->cb_mask, DRV_SWTOE_CB_RCOM)) && (pCNX->cb_func))
            if (1)
            {
                signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_TXQ_PUT, cnx_id & 0xFFFF, 0);
            }
        } while (pos < BITS_PER_LONG);
    }
}

static void swtoe_work_func_tcom(unsigned long* cnx_signal)
{
    int            i;
    unsigned long* p;
    long           base, pos;
    int            cnx_id;

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
            drv_swtoe_tx_consume(cnx_id);
        } while (pos < BITS_PER_LONG);
    }
    return;
}

static void swtoe_work_func(struct work_struct* work)
{
    unsigned long flag;
    int           bTCOM, bRCOM, bXMIT;

    /// protocol part (RTK to Linux)
    _swtoe_work_func_prot();

    /// Linux to RTK
    spin_lock_irqsave(&mutex_L2R, flag);
    bXMIT = HAS_BITS(l2r_signal, L2R_SIGNAL_XMIT);
    if (bXMIT)
    {
        memcpy(_cnx_signal_wq, _cnx_signal_l2r, sizeof(_cnx_signal_l2r));
        memset(_cnx_signal_l2r, 0, sizeof(_cnx_signal_l2r));
    }
    l2r_signal = 0;
    spin_unlock_irqrestore(&mutex_L2R, flag);
    if (bXMIT)
    {
        swtoe_work_func_xmit(_cnx_signal_wq);
    }

    /// RTK to Linux
    spin_lock_irqsave(&mutex_R2L, flag);
    bRCOM = HAS_BITS(r2l_signal, R2L_SIGNAL_RCOM);
    bTCOM = HAS_BITS(r2l_signal, R2L_SIGNAL_TCOM);
    if (bTCOM)
    {
        memcpy(_cnx_signal_wq, _cnx_signal_r2l, sizeof(_cnx_signal_r2l));
        memset(_cnx_signal_r2l, 0, sizeof(_cnx_signal_r2l));
    }
    r2l_signal = 0;
    spin_unlock_irqrestore(&mutex_R2L, flag);

    if (bRCOM)
    {
        drv_swtoe_rx_data_put();
    }
    if (bTCOM)
    {
        swtoe_work_func_tcom(_cnx_signal_wq);
    }
}

int _drv_swtoe_glue_ctrl_alloc(void)
{
    if (g_pIpcInfo)
    {
        printk("[%s][%d] IPC control has been allocated\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if (NULL
        == (g_pIpcInfo =
                (sstoe_ipc_ctrl_t*)dma_alloc_coherent(NULL, sizeof(sstoe_ipc_ctrl_t), &_ipc_ctrl_phys, GFP_KERNEL)))
    {
        printk("[%s][%d] IPC control allocate fail\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }
    memset(g_pIpcInfo, 0, sizeof(sstoe_ipc_ctrl_t));
    signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_CTRL_BUF_ALLOC, PA2BUS(_ipc_ctrl_phys),
                sizeof(sstoe_ipc_ctrl_t) & 0xFFFF);
    return 0;
}
