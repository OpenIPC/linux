/*
 * mdrv_swtoe_prot.c- Sigmastar
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
#include <linux/skbuff.h>
#include <linux/in.h>
#include "drv_dualos.h"
#include "mdrv_swtoe.h"
#include "mdrv_swtoe_intl.h"

int drv_swtoe_create(int prot, int* cnx_id)
{
    int ret = 0;
    int txq_id;

    if (0 != (ret = drv_swtoe_cnx_txq_create(prot, &txq_id)))
    {
        return ret;
    }
    if (DRV_SWTOE_PROT_BYPASS != prot)
    {
        if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_CREATE, txq_id & 0xFFFF, 0)))
        {
            printk("[%s][%d] TX packet IPC fail %d\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }
    }
    *cnx_id = txq_id;
    return 0;
}

int drv_swtoe_connect(int cnx_id, struct sockaddr* addr, int blk)
{
    struct sockaddr_in* usin     = (struct sockaddr_in*)addr;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    int                 ret;

    /// support ipv4 at this stage. don't know how to deal with ipv6 now
    if (PF_INET != usin->sin_family)
    {
        printk("[%s][%d] not ipv4 protocol %d\n", __FUNCTION__, __LINE__, (int)usin->sin_family);
        return -1;
    }
    CNX_SANITY(cnx_id);

    pTXQ_IPC                 = &g_pIpcInfo->txq_info[cnx_id];
    pTXQ_IPC->cnx_info.cause = 0x0;
    pTXQ_IPC->cnx_info.blk   = blk;

    pTXQ_IPC->cnx_info.dport = htons(usin->sin_port);
    pTXQ_IPC->cnx_info.daddr = htonl(usin->sin_addr.s_addr);

    /// refine source address port later
    pTXQ_IPC->cnx_info.sport = htons(0);          /// any port
    pTXQ_IPC->cnx_info.saddr = htonl(INADDR_ANY); /// any address

    printk("[%s][%d] (daddr, dport) = (0x%08x, 0x%04x)\n", __FUNCTION__, __LINE__, pTXQ_IPC->cnx_info.daddr,
           pTXQ_IPC->cnx_info.dport);
    printk("[%s][%d] (saddr, sport) = (0x%08x, 0x%04x)\n", __FUNCTION__, __LINE__, pTXQ_IPC->cnx_info.saddr,
           pTXQ_IPC->cnx_info.sport);
    printk("[%s][%d] blocking mode = %d\n", __FUNCTION__, __LINE__, pTXQ_IPC->cnx_info.blk);

    if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_CNX_REQ, cnx_id & 0xFFFF, 0)))
    {
        printk("[%s][%d] IPC fail %d. (cnx_id, cmd) = (%d, 0x%08x)\n", __FUNCTION__, __LINE__, ret, cnx_id,
               SWTOE_IPC_ARG1_PROT_CNX_REQ);
        return -1;
    }
    return 0;
}

int drv_swtoe_disconnect(int cnx_id)
{
    int ret;

    if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_DISCNX_REQ, cnx_id & 0xFFFF, 0)))
    {
        printk("[%s][%d] IPC fail %d. (cnx_id, cmd) = (%d, 0x%08x)\n", __FUNCTION__, __LINE__, ret, cnx_id,
               SWTOE_IPC_ARG1_PROT_DISCNX_REQ);
        return -1;
    }
    return 0;
}

int drv_swtoe_shutdown(int cnx_id)
{
    int ret = 0;

    CNX_SANITY(cnx_id);
    if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_SHDN_REQ, cnx_id & 0xFFFF, 0)))
    {
        printk("[%s][%d] IPC fail %d. (cnx_id, cmd) = (%d, 0x%08x)\n", __FUNCTION__, __LINE__, ret, cnx_id,
               SWTOE_IPC_ARG1_PROT_SHDN_REQ);
        return -1;
    }
    /// @FIXME : how to handle the synchronization
    drv_swtoe_cnx_txq_free(cnx_id);
    return 0;
}

int drv_swtoe_close(int cnx_id)
{
    int ret = 0;

    CNX_SANITY(cnx_id);
    if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_CLS_REQ, cnx_id & 0xFFFF, 0)))
    {
        printk("[%s][%d] IPC fail %d. (cnx_id, cmd) = (%d, 0x%08x)\n", __FUNCTION__, __LINE__, ret, cnx_id,
               SWTOE_IPC_ARG1_PROT_CLS_REQ);
        return -1;
    }
    return 0;
}

int drv_swtoe_bind(int cnx_id, struct sockaddr* addr)
{
    struct sockaddr_in* usin     = (struct sockaddr_in*)addr;
    sstoe_ipc_txq_info* pTXQ_IPC = NULL;
    int                 ret;
    sstoe_cnx_info*     pCNX = &g_pCnxInfo[cnx_id];

    /// support ipv4 at this stage. don't know how to deal with ipv6 now
    if (PF_INET != usin->sin_family)
    {
        printk("[%s][%d] not ipv4 protocol %d\n", __FUNCTION__, __LINE__, (int)usin->sin_family);
        return -1;
    }
    CNX_SANITY(cnx_id);
    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];

    pCNX->bind_ready         = 0;
    pTXQ_IPC->cnx_info.blk   = 0;
    pTXQ_IPC->cnx_info.sport = htons(usin->sin_port);
    pTXQ_IPC->cnx_info.saddr = htonl(usin->sin_addr.s_addr);

    /// refine source address port later
    pTXQ_IPC->cnx_info.dport = htons(0);          /// any port
    pTXQ_IPC->cnx_info.daddr = htonl(INADDR_ANY); /// any address
    pTXQ_IPC->cnx_info.cause = 0x1;

    printk("[%s][%d] (daddr, dport) = (0x%08x, 0x%04x)\n", __FUNCTION__, __LINE__, pTXQ_IPC->cnx_info.daddr,
           pTXQ_IPC->cnx_info.dport);
    printk("[%s][%d] (saddr, sport) = (0x%08x, 0x%04x)\n", __FUNCTION__, __LINE__, pTXQ_IPC->cnx_info.saddr,
           pTXQ_IPC->cnx_info.sport);
    // printk("[%s][%d] blocking mode = %d\n", __FUNCTION__, __LINE__, pTXQ_IPC->cnx_info.blk);

    if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_BIND_REQ, cnx_id & 0xFFFF, 0)))
    {
        printk("[%s][%d] IPC fail %d. (cnx_id, cmd) = (%d, 0x%08x)\n", __FUNCTION__, __LINE__, ret, cnx_id,
               SWTOE_IPC_ARG1_PROT_CNX_REQ);
        return -1;
    }
    return 0;
}

int drv_swtoe_bind_status(int cnx_id)
{
    sstoe_cnx_info* pCNX = &g_pCnxInfo[cnx_id];

    CNX_SANITY(cnx_id);
    return (HAS_FLAGS(pCNX->bind_ready, 0x80)) ? -(pCNX->bind_ready & 0x1) : 1;
}

int drv_swtoe_lstn_start(int cnx_id, int backlog) /// max of backlog is 128, SOMAXCONN
{
    int ret = 0;

    CNX_SANITY(cnx_id);

    if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_LSTN_START, (backlog << 16) | (cnx_id & 0xFFFF), 0)))
    {
        printk("[%s][%d] IPC fail %d. (cnx_id, cmd) = (%d, 0x%08x)\n", __FUNCTION__, __LINE__, ret, cnx_id,
               SWTOE_IPC_ARG1_PROT_LSTN_START);
        return -1;
    }
    return 0;
}

int drv_swtoe_lstn_stop(int cnx_id)
{
    int ret = 0;

    CNX_SANITY(cnx_id);
    if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_LSTN_STOP, (cnx_id & 0xFFFF), 0)))
    {
        printk("[%s][%d] IPC fail %d. (cnx_id, cmd) = (%d, 0x%08x)\n", __FUNCTION__, __LINE__, ret, cnx_id,
               SWTOE_IPC_ARG1_PROT_LSTN_STOP);
        return -1;
    }
    return 0;
}

typedef struct
{
    struct list_head        list;
    drv_swtoe_glue_cnx_data lstn_data;
} lstn_data_t;

int drv_swtoe_lstn_resp(int cnx_id)
{
    sstoe_cnx_info* pCNX = &g_pCnxInfo[cnx_id];
    lstn_data_t*    plstn_data;

    CNX_SANITY(cnx_id);
    if (0x1 != g_pIpcInfo->lstn_data.lock_lx)
    {
        printk("[%s][%d] bad state for listen data lock %d (should be 1)\n", __FUNCTION__, __LINE__,
               g_pIpcInfo->lstn_data.lock_lx);
        return -1;
    }
    if (NULL == (plstn_data = kmalloc(sizeof(lstn_data_t), GFP_ATOMIC)))
    {
        printk("[%s][%d] kmalloc %d bytes fail\n", __FUNCTION__, __LINE__, sizeof(lstn_data_t));
        return -1;
    }

    plstn_data->lstn_data.saddr = htonl(g_pIpcInfo->lstn_data.lstn.saddr);
    plstn_data->lstn_data.daddr = htonl(g_pIpcInfo->lstn_data.lstn.daddr);
    plstn_data->lstn_data.sport = htons(g_pIpcInfo->lstn_data.lstn.sport);
    plstn_data->lstn_data.dport = htons(g_pIpcInfo->lstn_data.lstn.dport);
    list_add_tail(&plstn_data->list, &pCNX->lstn_queue);
    g_pIpcInfo->lstn_data.lock_lx = 0x2;
    return 0;
}

int drv_swtoe_lstn_empty(int cnx_id)
{
    sstoe_cnx_info* pCNX = &g_pCnxInfo[cnx_id];

    CNX_SANITY(cnx_id);
    return (list_empty(&pCNX->lstn_queue)) ? 1 : 0;
}

int drv_swtoe_lstn_remove(int cnx_id, drv_swtoe_lstn_data* pdrv_lstn_data)
{
    sstoe_cnx_info* pCNX = &g_pCnxInfo[cnx_id];
    lstn_data_t*    plstn_data;

    CNX_SANITY(cnx_id);
    if (list_empty(&pCNX->lstn_queue))
    {
        return -1;
    }

    plstn_data = list_first_entry(&pCNX->lstn_queue, lstn_data_t, list);
    memcpy(pdrv_lstn_data, plstn_data, sizeof(drv_swtoe_lstn_data));
    list_del(&plstn_data->list);
    return 0;
}

int drv_swtoe_lstn_clr(int cnx_id)
{
    drv_swtoe_lstn_data lstn_data;

    while (!drv_swtoe_lstn_remove(cnx_id, &lstn_data))
        ;

    return 0;
}

int drv_swtoe_clone(int cnx_id, int* new_cnx_id, drv_swtoe_lstn_data* plstn_data)
{
    sstoe_ipc_txq_info* pTXQ_IPC     = NULL;
    sstoe_ipc_txq_info* pTXQ_IPC_new = NULL;
    int                 ret;

    CNX_SANITY(cnx_id);

    pTXQ_IPC = &g_pIpcInfo->txq_info[cnx_id];

    if ((ret = drv_swtoe_create(pTXQ_IPC->prot, new_cnx_id)))
    {
        printk("[%s][%d] drv_swtoe_create fail\n", __FUNCTION__, __LINE__);
        return ret;
    }

    pTXQ_IPC_new = &g_pIpcInfo->txq_info[*new_cnx_id];

    pTXQ_IPC_new->cnx_info.dport = htons(plstn_data->dport);
    pTXQ_IPC_new->cnx_info.daddr = htonl(plstn_data->daddr);
    pTXQ_IPC_new->cnx_info.sport = htons(plstn_data->sport);
    pTXQ_IPC_new->cnx_info.saddr = htonl(plstn_data->saddr);

    return 0;
}

int drv_swtoe_acpt(int cnx_id, int new_cnx_id)
{
    int ret;

    CNX_SANITY(cnx_id);
    CNX_SANITY(new_cnx_id);

    if ((ret = signal_rtos(SWTOE_IPC_ARG0, SWTOE_IPC_ARG1_PROT_ACPT_REQ,
                           (((new_cnx_id & 0xFFFF) << 16) | (cnx_id & 0xFFFF)), 0)))
    {
        printk("[%s][%d] IPC fail %d. (cnx_id, new_cnx_id, cmd) = (%d, %d, 0x%08x)\n", __FUNCTION__, __LINE__, ret,
               cnx_id, new_cnx_id, SWTOE_IPC_ARG1_PROT_ACPT_REQ);
        return -1;
    }

    return 0;
}
