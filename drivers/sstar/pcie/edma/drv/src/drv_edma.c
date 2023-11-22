/*
 * drv_edma.c- Sigmastar
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
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include "cam_sysfs.h"
#include "drv_edma.h"
#include "hal_edma.h"

/* Options */
#define USE_CAMOS_IRQ  0
#define _DRV_EDMA_DBG_ 0

/* Local variables */
static eDmaDev *m_edmaDev  = NULL;
static u32      m_priority = 0;

static int _drvEDMA_ProcessReqQueue(u8 id, eDmaDir dir);

static int edma_throughput = 0;
module_param(edma_throughput, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(edma_throughput, "Throughput statistic)");

static ssize_t _dbgRequest(u32 idx, eDmaRequest *req, char *buf)
{
    u32     i;
    ssize_t count = 0;

    count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
                      "Req[%02d]: d_%d | b_%d_%px | s_%d | cb_%px\r\n", idx, req->dir, req->u32BurstCnt, req->burst,
                      req->state, req->fpCallback);

    for (i = 0; i < req->u32BurstCnt; i++)
    {
        count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "(%08X %08X %08X) ",
                          (u32)req->burst[i].sar, (u32)req->burst[i].dar, req->burst[i].sz);
        if (i & 0x1)
            count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\r\n");
    }
    count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\r\n");
    return count;
}

static ssize_t _dbgReqQueue(u8 id, eDmaDir dir, char *buf)
{
    struct CamOsListHead_t *head;
    eDmaRequest *           req;
    char *                  str_dir;
    u32                     i     = 0;
    ssize_t                 count = 0;

    if (dir == EDMA_WRITE_DIR)
    {
        head    = &m_edmaDev[id].reqWrQueue;
        str_dir = "WR";
    }
    else
    {
        head    = &m_edmaDev[id].reqRdQueue;
        str_dir = "RD";
    }

    CamOsSpinLockIrqSave(&m_edmaDev[id].lock[dir]);
    if (!CAM_OS_LIST_EMPTY(head))
    {
        count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
                          "----------------- Dump_%d %s request queue -----------------\r\n", id, str_dir);
        CAM_OS_LIST_FOR_EACH_ENTRY(req, head, list)
        {
            count += _dbgRequest(i, req, buf + count);
            i++;
        }
        count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
                          "-----------------------------------------------------------\r\n");
    }
    CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);

    return count;
}

static ssize_t _dbgFreeCh(u8 id, eDmaDir dir, char *buf)
{
    struct CamOsListHead_t *head;
    eDmaChannel *           ch;
    char *                  str_dir;
    u32                     i     = 0;
    ssize_t                 count = 0;

    if (dir == EDMA_WRITE_DIR)
    {
        head    = &m_edmaDev[id].freeWrCh;
        str_dir = "WR";
    }
    else
    {
        head    = &m_edmaDev[id].freeRdCh;
        str_dir = "RD";
    }

    CamOsSpinLockIrqSave(&m_edmaDev[id].lock[dir]);
    if (!CAM_OS_LIST_EMPTY(head))
    {
        count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
                          "----------------- Dump_%d %s free channel ------------------\r\n", id, str_dir);
        CAM_OS_LIST_FOR_EACH_ENTRY(ch, head, list)
        {
            count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
                              "Ch[%02d]: i_%d | d_%d | ll va_%px miu_%llx\r\n", i, ch->index, ch->dir, ch->ll_virt,
                              ch->ll_miu);
            i++;
        }
        count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
                          "-----------------------------------------------------------\r\n");
    }
    CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);

    return count;
}

#if _DRV_EDMA_DBG_
static void _dbgCh(eDmaChannel *ch)
{
    u32 i = 0;

    CamOsPrintf(KERN_INFO "[DBG] Ch[%d], dir %d, burst %d\r\n", ch->index, ch->dir, ch->cnt);
    CamOsPrintf(KERN_INFO "------------------------------------------\r\n");
    CamOsPrintf(KERN_INFO "LL virt %px, miu %llx\r\n", ch->ll_virt, ch->ll_miu);
    CamOsPrintf(KERN_INFO "cb-tcb-llp-lie-rie-sz-sar_l-sar_h-dar_l-dar_h\r\n");
    for (i = 0; i <= ch->cnt; i++)
    {
        CamOsPrintf(KERN_INFO "%d-%d-%d-%d-%d-%d-%X-%X-%X-%X\r\n", ch->ll_virt[i].cb, ch->ll_virt[i].tcb,
                    ch->ll_virt[i].llp, ch->ll_virt[i].lie, ch->ll_virt[i].rie, ch->ll_virt[i].xfer_size,
                    ch->ll_virt[i].sar_l, ch->ll_virt[i].sar_h, ch->ll_virt[i].dar_l, ch->ll_virt[i].dar_h);
    }
    CamOsPrintf(KERN_INFO "------------------------------------------\r\n");
}
#endif

static u8 _drvEDMA_ProcessCallback(eDmaDev *edma, bool dir_wr, eDmaReqState state)
{
    u8                      ch = 0, processed = 0;
    u32                     bitmap = 0, *ch_map = NULL;
    eDmaChannel *           ch_pool = NULL;
    eDmaRequest *           req;
    CamOsSpinlock_t *       lock;
    struct CamOsListHead_t *ch_free;
    struct timespec64       now, diff;

    if (dir_wr)
    {
        lock    = &edma->lock[EDMA_WRITE_DIR];
        ch_pool = edma->wrChPool;
        ch_free = &edma->freeWrCh;
        if (state == EDMA_REQ_ST_COMP)
        {
            ch_map = &edma->doneWrChMap;
            bitmap = edma->doneWrChMap;
        }
        else
        {
            ch_map = &edma->abortWrChMap;
            bitmap = edma->abortWrChMap;
        }
    }
    else
    {
        lock    = &edma->lock[EDMA_READ_DIR];
        ch_pool = edma->rdChPool;
        ch_free = &edma->freeRdCh;
        if (state == EDMA_REQ_ST_COMP)
        {
            ch_map = &edma->doneRdChMap;
            bitmap = edma->doneRdChMap;
        }
        else
        {
            ch_map = &edma->abortRdChMap;
            bitmap = edma->abortRdChMap;
        }
    }

    if (edma_throughput > 0)
    {
        ktime_get_ts64(&now);
    }

    while (bitmap)
    {
        if (bitmap & 1)
        {
            req = ch_pool[ch].req;
            if (req)
            {
                req->state = state;
                req->fpCallback(req);
                CamOsSpinLockIrqSave(lock);
                if (edma_throughput > 0)
                {
                    int i;
                    diff                      = timespec64_sub(now, req->ts);
                    edma->stats.start[dir_wr] = timespec64_add(edma->stats.start[dir_wr], diff);
                    for (i = 0; i < req->u32BurstCnt; i++)
                    {
                        edma->stats.bytes[dir_wr] += req->burst[i].sz;
                    }
                }
                ch_pool[ch].req = NULL;
                ch_pool[ch].cnt = 0;
                /* return back the channel */
                CAM_OS_LIST_ADD_TAIL(&ch_pool[ch].list, ch_free);
                *ch_map &= ~(1 << ch); // clear bit of the corresponding channel
                CamOsSpinUnlockIrqRestore(lock);
                processed++;
            }
        }
        ch++;
        bitmap = bitmap >> 1;
    }

    return processed;
}

static void _drvEDMA_ThreadWrite(void *arg)
{
    eDmaDev *edma      = arg;
    u32      processed = 0;
    eDmaDir  dir       = EDMA_WRITE_DIR;

    while ((CamOsThreadShouldStop() != CAM_OS_OK) && edma->activate[dir])
    {
        CamOsTsemDownInterruptible(&edma->sem[dir]);

        if ((CamOsThreadShouldStop() == CAM_OS_OK) || !edma->activate[dir])
        {
            break;
        }
        processed += _drvEDMA_ProcessCallback(edma, true, EDMA_REQ_ST_COMP);
        processed += _drvEDMA_ProcessCallback(edma, true, EDMA_REQ_ST_ABORT);
        if (processed && (CamOsThreadShouldStop() != CAM_OS_OK) && edma->activate[dir])
        {
            _drvEDMA_ProcessReqQueue(edma->id, dir);
        }
    }
}

static void _drvEDMA_ThreadRead(void *arg)
{
    eDmaDev *edma      = arg;
    u32      processed = 0;
    eDmaDir  dir       = EDMA_READ_DIR;

    while ((CamOsThreadShouldStop() != CAM_OS_OK) && edma->activate[dir])
    {
        CamOsTsemDownInterruptible(&edma->sem[dir]);

        if ((CamOsThreadShouldStop() == CAM_OS_OK) || !edma->activate[dir])
        {
            break;
        }
        processed += _drvEDMA_ProcessCallback(edma, false, EDMA_REQ_ST_COMP);
        processed += _drvEDMA_ProcessCallback(edma, false, EDMA_REQ_ST_ABORT);
        if (processed && (CamOsThreadShouldStop() != CAM_OS_OK) && edma->activate[dir])
        {
            _drvEDMA_ProcessReqQueue(edma->id, dir);
        }
    }
}

static u32 _drvEDMA_Interrupt(eDmaDev *edma)
{
    u8  id = edma->id;
    u32 wr_done, wr_abort, rd_done, rd_abort;

    wr_done  = halEDMA_GetDoneIntrSt(id, true);
    rd_done  = halEDMA_GetDoneIntrSt(id, false);
    wr_abort = halEDMA_GetAbortIntrSt(id, true);
    rd_abort = halEDMA_GetAbortIntrSt(id, false);
    halEDMA_ClrDoneIntrSt(id, wr_done, true);
    halEDMA_ClrDoneIntrSt(id, rd_done, false);
    halEDMA_ClrAbortIntrSt(id, wr_abort, true);
    halEDMA_ClrAbortIntrSt(id, rd_abort, false);

    CamOsSpinLockIrqSave(&edma->lock[EDMA_WRITE_DIR]);
    edma->doneWrChMap |= wr_done;
    edma->abortWrChMap |= wr_abort;
    CamOsSpinUnlockIrqRestore(&edma->lock[EDMA_WRITE_DIR]);
    CamOsSpinLockIrqSave(&edma->lock[EDMA_READ_DIR]);
    edma->doneRdChMap |= rd_done;
    edma->abortRdChMap |= rd_abort;
    CamOsSpinUnlockIrqRestore(&edma->lock[EDMA_READ_DIR]);

    return (wr_done | wr_abort | rd_done | rd_abort);
}

#if USE_CAMOS_IRQ
static void _drvEDMA_ISR(u32 nIrq, void *edmaDev)
{
    eDmaDev *edma = (eDmaDev *)edmaDev;

    if (_drvEDMA_Interrupt(edma))
    {
        if (edma->doneWrChMap || edma->abortWrChMap)
            CamOsTsemUp(&edma->sem[EDMA_WRITE_DIR]);

        if (edma->doneRdChMap || edma->abortRdChMap)
            CamOsTsemUp(&edma->sem[EDMA_READ_DIR]);
    }
}
#else
static irqreturn_t _drvEDMA_ISR(int irq, void *edmaDev)
{
    eDmaDev *edma = (eDmaDev *)edmaDev;

    if (_drvEDMA_Interrupt(edma))
    {
        if (edma->doneWrChMap || edma->abortWrChMap)
            CamOsTsemUp(&edma->sem[EDMA_WRITE_DIR]);

        if (edma->doneRdChMap || edma->abortRdChMap)
            CamOsTsemUp(&edma->sem[EDMA_READ_DIR]);
    }

    return IRQ_HANDLED;
}
#endif

static int _drvEDMA_StartXfer(u8 id, eDmaChannel *ch, eDmaRequest *req)
{
    u32 i;
    int err = 0;

    ch->cnt = req->u32BurstCnt;
    ch->req = req;
    memset(ch->ll_virt, 0, sizeof(eDmaElement) * ch->cnt);

    for (i = 0; i < ch->cnt; i++)
    {
        ch->ll_virt[i].cb        = 1;
        ch->ll_virt[i].xfer_size = req->burst[i].sz;
        ch->ll_virt[i].sar_l     = (req->burst[i].sar & 0xFFFFFFFF);
        ch->ll_virt[i].sar_h     = (req->burst[i].sar >> 32);
        ch->ll_virt[i].dar_l     = (req->burst[i].dar & 0xFFFFFFFF);
        ch->ll_virt[i].dar_h     = (req->burst[i].dar >> 32);
    }
    /* Enable interrupt of the last data element when callback is assigned */
    if (req->fpCallback)
    {
        ch->ll_virt[i - 1].lie = 1;
        ch->ll_virt[i - 1].rie = 1;
    }
    /* The last element is a list element */
    memset(&ch->ll_virt[i], 0, sizeof(eDmaElement));
    ch->ll_virt[i].tcb   = 1;
    ch->ll_virt[i].llp   = 1;
    ch->ll_virt[i].sar_l = (ch->ll_miu & 0xFFFFFFFF); // points to the start addr of LL structure
    ch->ll_virt[i].sar_h = (ch->ll_miu >> 32);
    CamOsMemFlush(ch->ll_virt, sizeof(eDmaElement) * (ch->cnt + 1));
    CamOsMiuPipeFlush();
#if _DRV_EDMA_DBG_
    _dbgCh(ch);
#endif
    CamOsSpinLockIrqSave(&m_edmaDev[id].lock[ch->dir]);
    if (edma_throughput > 0)
    {
        ktime_get_ts64(&req->ts);
    }
    err = halEDMA_StartXfer(id, ch->index, ch->dir == EDMA_WRITE_DIR ? true : false, ch->ll_miu);
    CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[ch->dir]);

    return err;
}

static int _drvEDMA_ProcessReqQueue(u8 id, eDmaDir dir)
{
    int                     err = 0, ret = 0;
    struct CamOsListHead_t *req_q;
    struct CamOsListHead_t *free_ch;
    eDmaChannel *           ch  = NULL;
    eDmaRequest *           req = NULL;

    ch  = NULL;
    req = NULL;
    if (dir == EDMA_WRITE_DIR)
    {
        // process write request queue
        req_q   = &m_edmaDev[id].reqWrQueue;
        free_ch = &m_edmaDev[id].freeWrCh;
    }
    else
    {
        // process read request queue
        req_q   = &m_edmaDev[id].reqRdQueue;
        free_ch = &m_edmaDev[id].freeRdCh;
    }
    /* is there any pending request & also a free channel available? */
    CamOsSpinLockIrqSave(&m_edmaDev[id].lock[dir]);
    if (!CAM_OS_LIST_EMPTY(req_q) && !CAM_OS_LIST_EMPTY(free_ch))
    {
        ch = CAM_OS_LIST_FIRST_ENTRY(free_ch, eDmaChannel, list);
        CAM_OS_LIST_DEL(&ch->list);
        req = CAM_OS_LIST_FIRST_ENTRY(req_q, eDmaRequest, list);
        CAM_OS_LIST_DEL(&req->list);
    }
    CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);

    if (ch && req)
    {
        // bundle channel & request
        ch->req    = req;
        req->ch    = ch;
        req->state = EDMA_REQ_ST_PROCESS;
        err        = _drvEDMA_StartXfer(id, ch, req);
        if (err)
        {
            ch->req    = NULL;
            req->ch    = NULL;
            req->state = EDMA_REQ_ST_WAIT;
            CamOsSpinLockIrqSave(&m_edmaDev[id].lock[dir]);
            CAM_OS_LIST_ADD_TAIL(&ch->list, free_ch); // return back channel
            CAM_OS_LIST_ADD_TAIL(&req->list, req_q);  // insert to request queue again
            CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);
            ret = err;
        }
    }

    return ret;
}

int drvEDMA_SubmitRequest(u8 id, eDmaRequest *req)
{
    if (!req)
    {
        return -EFAULT;
    }
    if ((req->u32BurstCnt > (EDMA_LL_SIZE - 1)) || (req->u32BurstCnt == 0))
    {
        CamOsPrintf(KERN_ERR "DMA req with invalid burst\r\n");
        return -EINVAL;
    }

    CamOsSpinLockIrqSave(&m_edmaDev[id].lock[req->dir]);

    /* insert to request queue */
    if (req->dir == EDMA_WRITE_DIR)
        CAM_OS_LIST_ADD_TAIL(&req->list, &m_edmaDev[id].reqWrQueue);
    else
        CAM_OS_LIST_ADD_TAIL(&req->list, &m_edmaDev[id].reqRdQueue);
    req->state = EDMA_REQ_ST_WAIT;

    CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[req->dir]);

    /* start DMA transfer if any free channel available */
    return _drvEDMA_ProcessReqQueue(id, req->dir);
}
EXPORT_SYMBOL(drvEDMA_SubmitRequest);

eDmaRequest *drvEDMA_AllocRequest(eDmaDir dir, u32 burst)
{
    eDmaRequest *req;

    req = CamOsMemAlloc(sizeof(eDmaDev));
    if (!req)
    {
        return NULL;
    }
    req->burst = CamOsMemAlloc(sizeof(eDmaBurst) * burst);
    if (!req->burst)
    {
        CamOsMemRelease(req);
        return NULL;
    }

    req->dir         = dir;
    req->u32BurstCnt = burst;
    req->ch          = NULL;
    req->fpCallback  = NULL;
    CAM_OS_INIT_LIST_HEAD(&req->list);
    req->state = EDMA_REQ_ST_IDLE;

    return req;
}
EXPORT_SYMBOL(drvEDMA_AllocRequest);

void drvEDMA_ReleaseRequest(eDmaRequest *req)
{
    if (req)
    {
        if (req->state == EDMA_REQ_ST_PROCESS)
        {
            CamOsPrintf(KERN_DEBUG "eDMA request in in-processing\r\n");
            return;
        }
        if (req->burst)
        {
            CamOsMemRelease(req->burst);
        }
        CamOsMemRelease(req);
    }
}
EXPORT_SYMBOL(drvEDMA_ReleaseRequest);

static void drvEDMA_ReleaseAllRequests(u8 id, eDmaDir dir)
{
    eDmaRequest *           req, *tmp;
    eDmaChannel *           ch;
    struct CamOsListHead_t *ch_list;
    struct CamOsListHead_t *req_que;
    u32                     i, cnt;

    /* wake up thread to quit */
    if (m_edmaDev[id].thread[dir])
    {
        CamOsTsemUp(&m_edmaDev[id].sem[dir]);
        CamOsThreadStop(m_edmaDev[id].thread[dir]);
    }

    CamOsSpinLockIrqSave(&m_edmaDev[id].lock[dir]);
    /* remove all started requests */
    if (dir == EDMA_WRITE_DIR)
    {
        ch      = m_edmaDev[id].wrChPool;
        cnt     = m_edmaDev[id].u16WrChCnt;
        ch_list = &m_edmaDev[id].freeWrCh;
        req_que = &m_edmaDev[id].reqWrQueue;
    }
    else
    {
        ch      = m_edmaDev[id].rdChPool;
        cnt     = m_edmaDev[id].u16RdChCnt;
        ch_list = &m_edmaDev[id].freeRdCh;
        req_que = &m_edmaDev[id].reqRdQueue;
    }

    for (i = 0; i < cnt; i++)
    {
        if (ch->req)
        {
            /* release in-progress requests */
            if (ch->req->state != EDMA_REQ_ST_PROCESS)
            {
                CamOsPrintf(KERN_WARNING "eDMA req not in-process\r\n");
            }
            ch->req->state = EDMA_REQ_ST_DROP;
            drvEDMA_ReleaseRequest(ch->req);
            /* set channel as free */
            ch->req = NULL;
            ch->cnt = 0;
            CAM_OS_LIST_ADD_TAIL(&ch->list, ch_list); // return back channel
        }
        ch++;
    }

    /* remove requests in read/write queue */
    if (!CAM_OS_LIST_EMPTY(req_que))
    {
        CAM_OS_LIST_FOR_EACH_ENTRY_SAFE(req, tmp, req_que, list)
        {
            CAM_OS_LIST_DEL(&req->list); // remove request from queue
            req->state = EDMA_REQ_ST_DROP;
            drvEDMA_ReleaseRequest(req);
        }
    }

    CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);
}

int drvEDMA_Prepare(u8 id, eDmaDir dir)
{
    void (*entry)(void *);
    CamOsThreadAttrb_t attr = {0};

    if (dir >= EDMA_DIR_NUM)
    {
        CamOsPrintf(KERN_ERR "eDMA prepare with invalid dir %d\r\n", dir);
        return -EINVAL;
    }

    CamOsSpinLockIrqSave(&m_edmaDev[id].lock[dir]);
    if (m_edmaDev[id].activate[dir])
    {
        CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);
        return 0;
    }

    m_edmaDev[id].activate[dir] = 1;
    CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);

    if (dir == EDMA_WRITE_DIR)
    {
        m_edmaDev[id].doneWrChMap  = 0;
        m_edmaDev[id].abortWrChMap = 0;
        entry                      = _drvEDMA_ThreadWrite;
    }
    else
    {
        m_edmaDev[id].doneRdChMap  = 0;
        m_edmaDev[id].abortRdChMap = 0;
        entry                      = _drvEDMA_ThreadRead;
    }
    CamOsTsemDeinit(&m_edmaDev[id].sem[dir]);
    CamOsTsemInit(&m_edmaDev[id].sem[dir], 0);

    /* need a thread to handle callback */
    attr.nStackSize = 0;
    attr.szName     = m_edmaDev[id].irqName;
    attr.nPriority  = m_priority;
    if (CamOsThreadCreate(&m_edmaDev[id].thread[dir], &attr, (void *)entry, &m_edmaDev[id]) != CAM_OS_OK)
    {
        CamOsPrintf(KERN_ERR "eDMA thread create FAIL\r\n");
        m_edmaDev[id].activate[dir] = 0;
        return -ECHILD;
    }
    CamOsPrintf(KERN_ERR "eDMA dir %d thread %px\r\n", dir, m_edmaDev[id].thread[dir]);

    return 0;
}
EXPORT_SYMBOL(drvEDMA_Prepare);

void drvEDMA_Cleanup(u8 id, eDmaDir dir)
{
    if (dir >= EDMA_DIR_NUM)
    {
        CamOsPrintf(KERN_ERR "eDMA cleanup with invalid dir %d\r\n", dir);
        return;
    }

    CamOsSpinLockIrqSave(&m_edmaDev[id].lock[dir]);
    if (!m_edmaDev[id].activate[dir])
    {
        CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);
        return;
    }
    /* inform thread to quit */
    m_edmaDev[id].activate[dir] = 0;
    CamOsSpinUnlockIrqRestore(&m_edmaDev[id].lock[dir]);

    /* stop DMA transfer of all channels */
    // halEDMA_StopDev(id, (dir == EDMA_WRITE_DIR));

    drvEDMA_ReleaseAllRequests(id, dir);
}
EXPORT_SYMBOL(drvEDMA_Cleanup);

int drvEDMA_Probe(struct platform_device *pdev)
{
    u16           i;
    u8            id, dev_num;
    int           err;
    ss_miu_addr_t ll_miu = 0;
    eDmaElement * ll;
    eDmaChannel * ch;

    if (m_edmaDev)
    {
        CamOsPrintf(KERN_DEBUG "eDMA has been initialized!\r\n");
        return 0;
    }

    if (of_property_read_u32(pdev->dev.of_node, "thread_priority", &m_priority))
        m_priority = 0;

    dev_num = sstar_pcieif_num_of_intf();

    m_edmaDev = CamOsMemAlloc(sizeof(eDmaDev) * dev_num);
    if (!m_edmaDev)
    {
        return -ENOMEM;
    }

    for (id = 0; id < dev_num; id++)
    {
        m_edmaDev[id].id = id;
        CamOsSpinInit(&m_edmaDev[id].lock[EDMA_WRITE_DIR]);
        CamOsSpinInit(&m_edmaDev[id].lock[EDMA_READ_DIR]);
        CamOsTsemInit(&m_edmaDev[id].sem[EDMA_WRITE_DIR], 0);
        CamOsTsemInit(&m_edmaDev[id].sem[EDMA_READ_DIR], 0);

        CAM_OS_INIT_LIST_HEAD(&m_edmaDev[id].reqWrQueue);
        CAM_OS_INIT_LIST_HEAD(&m_edmaDev[id].reqRdQueue);
        CAM_OS_INIT_LIST_HEAD(&m_edmaDev[id].freeWrCh);
        CAM_OS_INIT_LIST_HEAD(&m_edmaDev[id].freeRdCh);

        /* Get the number of write channels supported */
        m_edmaDev[id].u16WrChCnt = halEDMA_GetWrChCnt(id);
        /* Get the number of read channels supported */
        m_edmaDev[id].u16RdChCnt = halEDMA_GetRdChCnt(id);

        /* Allocate Link List structure for all channels */
        m_edmaDev[id].llBufSz =
            sizeof(eDmaElement) * EDMA_LL_SIZE * (m_edmaDev[id].u16WrChCnt + m_edmaDev[id].u16RdChCnt);
        m_edmaDev[id].llBufDma = CamOsContiguousMemAlloc(m_edmaDev[id].llBufSz);
        if (m_edmaDev[id].llBufDma == CAM_OS_FAIL)
        {
            m_edmaDev[id].llBufDma = 0;
            err                    = -ENOMEM;
            goto err_ll_free;
        }
        m_edmaDev[id].llBuf = CamOsMemMap(m_edmaDev[id].llBufDma, m_edmaDev[id].llBufSz, true);
        if (!m_edmaDev[id].llBuf)
        {
            err = -EFAULT;
            goto err_ll_free;
        }
        ll_miu = CamOsMemPhysToMiu(m_edmaDev[id].llBufDma);
        ll     = m_edmaDev[id].llBuf;

        /* Allocate channels pool */
        m_edmaDev[id].wrChPool = CamOsMemAlloc(sizeof(eDmaChannel) * m_edmaDev[id].u16WrChCnt);
        if (!m_edmaDev[id].wrChPool)
        {
            err = -ENOMEM;
            goto err_ch_free;
        }
        m_edmaDev[id].rdChPool = CamOsMemAlloc(sizeof(eDmaChannel) * m_edmaDev[id].u16RdChCnt);
        if (!m_edmaDev[id].rdChPool)
        {
            CamOsMemRelease(m_edmaDev[id].wrChPool);
            err = -ENOMEM;
            goto err_ch_free;
        }

        /* Init write channels */
        ch = m_edmaDev[id].wrChPool;
        for (i = 0; i < m_edmaDev[id].u16WrChCnt; i++)
        {
            ch->index   = i;
            ch->cnt     = 0;
            ch->dir     = EDMA_WRITE_DIR;
            ch->ll_virt = ll;
            ch->ll_miu  = ll_miu;
            ch->req     = NULL;
            CAM_OS_LIST_ADD_TAIL(&ch->list, &m_edmaDev[id].freeWrCh);
            ch++;
            ll += EDMA_LL_SIZE;
            ll_miu += (sizeof(eDmaElement) * EDMA_LL_SIZE);
        }
        /* Init read channels */
        ch = m_edmaDev[id].rdChPool;
        for (i = 0; i < m_edmaDev[id].u16RdChCnt; i++)
        {
            ch->index   = i;
            ch->cnt     = 0;
            ch->dir     = EDMA_READ_DIR;
            ch->ll_virt = ll;
            ch->ll_miu  = ll_miu;
            ch->req     = NULL;
            CAM_OS_LIST_ADD_TAIL(&ch->list, &m_edmaDev[id].freeRdCh);
            ch++;
            ll += EDMA_LL_SIZE;
            ll_miu += (sizeof(eDmaElement) * EDMA_LL_SIZE);
        }

        /* Disable eDMA initially */
        halEDMA_StopDev(id, true);
        halEDMA_StopDev(id, false);
        m_edmaDev[id].doneWrChMap              = 0;
        m_edmaDev[id].doneRdChMap              = 0;
        m_edmaDev[id].abortWrChMap             = 0;
        m_edmaDev[id].abortRdChMap             = 0;
        m_edmaDev[id].activate[EDMA_WRITE_DIR] = 0;
        m_edmaDev[id].activate[EDMA_READ_DIR]  = 0;

        /* Make eDMA ready to use */
        drvEDMA_Prepare(id, EDMA_WRITE_DIR);
        drvEDMA_Prepare(id, EDMA_READ_DIR);

        /* Request IRQ */
        CamOsSnprintf(m_edmaDev[id].irqName, sizeof(m_edmaDev[id].irqName), "edma%d", id);
        m_edmaDev[id].u32Irq = platform_get_irq(pdev, id);
#if USE_CAMOS_IRQ
        err = CamOsIrqRequest(m_edmaDev[id].u32Irq, _drvEDMA_ISR, m_edmaDev[id].irqName, &m_edmaDev[id]);
#else
        err = request_irq(m_edmaDev[id].u32Irq, _drvEDMA_ISR, IRQF_SHARED, m_edmaDev[id].irqName, &m_edmaDev[id]);
#endif
        if (err)
        {
            // TODO: IRQ shared with MSI chained interrupt in RC
            // goto err_irq;
        }
    }

    return 0;

    // err_irq:
    for (id = 0; id < dev_num; id++)
    {
        if (m_edmaDev[id].wrChPool)
        {
            CamOsMemRelease(m_edmaDev[id].wrChPool);
            m_edmaDev[id].wrChPool = NULL;
        }
        if (m_edmaDev[id].rdChPool)
        {
            CamOsMemRelease(m_edmaDev[id].rdChPool);
            m_edmaDev[id].rdChPool = NULL;
        }
    }

err_ch_free:
    for (id = 0; id < dev_num; id++)
    {
        /* Free LinkedList structure */
        if (m_edmaDev[id].llBuf)
            CamOsMemUnmap(m_edmaDev[id].llBuf, m_edmaDev[id].llBufSz);
        if (m_edmaDev[id].llBufDma)
            CamOsContiguousMemRelease(m_edmaDev[id].llBufDma);
        m_edmaDev[id].llBuf    = NULL;
        m_edmaDev[id].llBufDma = 0;
    }
err_ll_free:
    /* Free eDMA device */
    if (m_edmaDev)
        CamOsMemRelease(m_edmaDev);
    m_edmaDev = NULL;
    CamOsPrintf(KERN_ERR "eDMA init failed\r\n");

    return err;
}

void drvEDMA_Remove(void)
{
    u8 id, dev_num;

    if (!m_edmaDev)
    {
        CamOsPrintf(KERN_DEBUG "eDMA not initial yet!\r\n");
        return;
    }
    dev_num = sstar_pcieif_num_of_intf();

    for (id = 0; id < dev_num; id++)
    {
        /* Disable eDMA */
        /* Free irqs */

        /* Release eDMA linked list buffer */
        if (m_edmaDev[id].llBuf)
            CamOsMemUnmap(m_edmaDev[id].llBuf, m_edmaDev[id].llBufSz);
        if (m_edmaDev[id].llBufDma)
            CamOsContiguousMemRelease(m_edmaDev[id].llBufDma);

        /* Release eDMA channels */
        if (m_edmaDev[id].wrChPool)
            CamOsMemRelease(m_edmaDev[id].wrChPool);
        if (m_edmaDev[id].rdChPool)
            CamOsMemRelease(m_edmaDev[id].rdChPool);
    }

    /* Free eDMA devices */
    CamOsMemRelease(m_edmaDev);
    m_edmaDev = NULL;

    return;
}

static ssize_t wr_queue_0_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return _dbgReqQueue(0, EDMA_WRITE_DIR, buf);
}
static DEVICE_ATTR_RO(wr_queue_0);

static ssize_t rd_queue_0_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return _dbgReqQueue(0, EDMA_READ_DIR, buf);
}
static DEVICE_ATTR_RO(rd_queue_0);

static ssize_t wr_queue_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return _dbgReqQueue(1, EDMA_WRITE_DIR, buf);
}
static DEVICE_ATTR_RO(wr_queue_1);

static ssize_t rd_queue_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return _dbgReqQueue(1, EDMA_READ_DIR, buf);
}
static DEVICE_ATTR_RO(rd_queue_1);

static ssize_t free_ch_0_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t count = 0;

    count += _dbgFreeCh(0, EDMA_WRITE_DIR, buf + count);
    count += _dbgFreeCh(0, EDMA_READ_DIR, buf + count);

    return count;
}
static DEVICE_ATTR_RO(free_ch_0);

static ssize_t free_ch_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t count = 0;

    count += _dbgFreeCh(1, EDMA_WRITE_DIR, buf + count);
    count += _dbgFreeCh(1, EDMA_READ_DIR, buf + count);

    return count;
}
static DEVICE_ATTR_RO(free_ch_1);

static u64 calculate_rate(const char *ops, u64 size, struct timespec64 *duration)
{
    u64 rate, ns;

    /* convert both size (stored in 'rate') and time in terms of 'ns' */
    ns   = timespec64_to_ns(duration);
    rate = size * NSEC_PER_SEC;

    /* Divide both size (stored in 'rate') and ns by a common factor */
    while (ns > UINT_MAX)
    {
        rate >>= 1;
        ns >>= 1;
    }

    if (!ns)
        return 0;

    /* calculate the rate */
    do_div(rate, (uint32_t)ns);
    return rate;
}

static ssize_t statistic_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t     count                 = 0;
    char const *str_dir[EDMA_DIR_NUM] = {"RD", "WR"};
    int         dir;
    int         id;

    if (edma_throughput <= 0)
    {
        count +=
            snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "edma_throughput:%d\n", edma_throughput);
    }

    for (id = 0; id < sstar_pcieif_num_of_intf(); id++)
    {
        for (dir = 0; dir < EDMA_DIR_NUM; dir++)
        {
            eDmaStatistic *stats;
            stats = &m_edmaDev[id].stats;
            count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
                              "%s[%d] bytes:%lld, time:%lld.%09ld Rate:%llu KB/s\n", str_dir[dir], id,
                              stats->bytes[dir], stats->start[dir].tv_sec, stats->start[dir].tv_nsec,
                              calculate_rate("", count, &stats->start[dir]));
        }
    }
    return count;
}
static DEVICE_ATTR_RO(statistic);

static int sstar_edma_probe(struct platform_device *pdev)
{
    CamDeviceCreateFile(&pdev->dev, &dev_attr_free_ch_0);
    CamDeviceCreateFile(&pdev->dev, &dev_attr_free_ch_1);
    CamDeviceCreateFile(&pdev->dev, &dev_attr_wr_queue_0);
    CamDeviceCreateFile(&pdev->dev, &dev_attr_rd_queue_0);
    CamDeviceCreateFile(&pdev->dev, &dev_attr_wr_queue_1);
    CamDeviceCreateFile(&pdev->dev, &dev_attr_rd_queue_1);
    CamDeviceCreateFile(&pdev->dev, &dev_attr_statistic);
    return drvEDMA_Probe(pdev);
}

static int sstar_edma_remove(struct platform_device *pdev)
{
    u8 id, dev_num;

    /* Disable eDMA */
    dev_num = sstar_pcieif_num_of_intf();
    for (id = 0; id < dev_num; id++)
    {
        halEDMA_StopDev(id, true);
        halEDMA_StopDev(id, false);
    }
    /* Free buffers */
    drvEDMA_Remove();
    return 0;
}

static const struct of_device_id ss_plat_edma_of_match[] = {
    {.compatible = "sstar,edma", 0},
};

static struct platform_driver ss_plat_edma_driver = {
    .driver =
        {
            .name           = "sstar-edma",
            .owner          = THIS_MODULE,
            .of_match_table = ss_plat_edma_of_match,
        },
    .probe  = sstar_edma_probe,
    .remove = sstar_edma_remove,
};

static int __init sstar_edma_init_driver(void)
{
    return platform_driver_register(&ss_plat_edma_driver);
}

static void __exit sstar_edma_exit_driver(void)
{
    platform_driver_unregister(&ss_plat_edma_driver);
}

device_initcall_sync(sstar_edma_init_driver);
module_exit(sstar_edma_exit_driver);

MODULE_DESCRIPTION("Sstar PCIe eDMA driver");
MODULE_AUTHOR("SSTAR");
MODULE_LICENSE("GPL");
