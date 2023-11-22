/*
 * drv_edma.h- Sigmastar
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

#ifndef _DRV_EDMA_H_
#define _DRV_EDMA_H_

#include "cam_os_wrapper.h"

#define EDMA_LL_SIZE (36)

struct _eDmaRequest;
struct _eDmaChannel;

/*
 * eDMA direction
 */
typedef enum
{
    EDMA_WRITE_DIR = 0,
    EDMA_READ_DIR,
    EDMA_DIR_NUM
} eDmaDir;

/*
 * Request state
 */
typedef enum
{
    EDMA_REQ_ST_IDLE = 0, // request is just created
    EDMA_REQ_ST_WAIT,     // request is waiting for service
    EDMA_REQ_ST_PROCESS,  // request is in service
    EDMA_REQ_ST_COMP,     // request is completed
    EDMA_REQ_ST_ABORT,    // request is aborted
    EDMA_REQ_ST_DROP,     // request is dropped
} eDmaReqState;

/*
 * Linked List Element
 */
typedef struct
{
    u32 cb : 1;  // Cycle Bit
    u32 tcb : 1; // Toggle Cycle Bit
    u32 llp : 1; // Load Link Pointer
    u32 lie : 1; // Local Interrupt Enable
    u32 rie : 1; // Remote Interrupt Enable
    u32 resv_5 : 27;
    u32 xfer_size; // transfer size
    u32 sar_l;     // source address low 32-bit
    u32 sar_h;     // source address high 32-bit
    u32 dar_l;     // destination address low 32-bit
    u32 dar_h;     // destination address high 32-bit
} eDmaElement;

/*
 * DMA burst
 */
typedef struct
{
    u64 sar; /* DMA source address */
    u64 dar; /* DMA destination address */
    u32 sz;  /* transfer size */
} eDmaBurst;

/*
 * DMA request
 */
typedef struct _eDmaRequest
{
    eDmaDir              dir;
    eDmaBurst *          burst;
    u32                  u32BurstCnt;
    struct _eDmaChannel *ch;    /* the edma channel handling the request */
    eDmaReqState         state; /* current state of request */
    // callback
    void (*fpCallback)(struct _eDmaRequest *req);
    void *                 pCbData[4];
    struct CamOsListHead_t list;
    struct timespec64      ts; /* time stamp */
} eDmaRequest;

/*
 * eDMA channel
 */
typedef struct _eDmaChannel
{
    u32                    index;   /* index of channel */
    u32                    cnt;     /* number of data elements in LL structure */
    eDmaDir                dir;     /* direction of channel */
    eDmaElement *          ll_virt; /* virtual addr of LL structure */
    ss_miu_addr_t          ll_miu;  /* miu addr of LL structure */
    eDmaRequest *          req;     /* request which is being handled by the channel */
    struct CamOsListHead_t list;
} eDmaChannel;

/*
 * eDMA
 */
typedef struct _eDmaStatistic
{
    u64               bytes[EDMA_DIR_NUM];
    struct timespec64 start[EDMA_DIR_NUM];
} eDmaStatistic;

/*
 * eDMA device handler
 */
typedef struct
{
    u32                    u32Irq;     /* irq number of DMA interrupt */
    char                   irqName[8]; /* irq name */
    u16                    u16WrChCnt; /* number of write channels */
    u16                    u16RdChCnt; /* number of read channels */
    u32                    llBufSz;    /* size of linked list structure buffer */
    void *                 llBuf;      /* virtual addr of linked list structure buffer */
    ss_phys_addr_t         llBufDma;   /* physical addr of linked list structure buffer */
    eDmaChannel *          wrChPool;   /* write channels pool */
    eDmaChannel *          rdChPool;   /* read channels pool */
    struct CamOsListHead_t reqWrQueue; /* write requests queue */
    struct CamOsListHead_t reqRdQueue; /* read requests queue */
    struct CamOsListHead_t freeWrCh;   /* list of free write channel */
    struct CamOsListHead_t freeRdCh;   /* list of free read channel */
    u32                    doneWrChMap;
    u32                    doneRdChMap;
    u32                    abortWrChMap;
    u32                    abortRdChMap;
    CamOsThread            thread[EDMA_DIR_NUM];   /* thread to process callbacks */
    CamOsTsem_t            sem[EDMA_DIR_NUM];      /* semaphore to wake up thread */
    CamOsSpinlock_t        lock[EDMA_DIR_NUM];     /* lock for DMA device */
    u8                     activate[EDMA_DIR_NUM]; /* ready flag of DMA device */
    u8                     id;                     /* device id */
    eDmaStatistic          stats;                  /* statistic data */
} eDmaDev;

int          drvEDMA_Prepare(u8 id, eDmaDir dir);
void         drvEDMA_Cleanup(u8 id, eDmaDir dir);
eDmaRequest *drvEDMA_AllocRequest(eDmaDir dir, u32 burst);
int          drvEDMA_SubmitRequest(u8 id, eDmaRequest *req);
void         drvEDMA_ReleaseRequest(eDmaRequest *req);

#endif /* _DRV_EDMA_H_ */
