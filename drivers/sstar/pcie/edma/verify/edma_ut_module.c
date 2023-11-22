/*
 * edma_ut_module.c- Sigmastar
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

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "drv_edma.h"

#define EDMA_UT_REQ_MAX 15 * 4

static eDmaRequest *edma_req[EDMA_UT_REQ_MAX];

void edma_ut_callback(eDmaRequest *req)
{
    printk("CB: req state %d\r\n", req->state);
    drvEDMA_ReleaseRequest(req);
}

//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
static int probe(void)
{
    int i, j, b;

    drvEDMA_Prepare(EDMA_WRITE_DIR);
    drvEDMA_Prepare(EDMA_READ_DIR);

    for (i = 0; i < EDMA_UT_REQ_MAX; i += 4)
    {
        edma_req[i + 0] = drvEDMA_AllocRequest(EDMA_WRITE_DIR, (i >> 2) + 1);
        edma_req[i + 1] = drvEDMA_AllocRequest(EDMA_READ_DIR, (i >> 2) + 1);
        edma_req[i + 2] = drvEDMA_AllocRequest(EDMA_WRITE_DIR, (EDMA_UT_REQ_MAX - i) >> 2);
        edma_req[i + 3] = drvEDMA_AllocRequest(EDMA_READ_DIR, (EDMA_UT_REQ_MAX - i) >> 2);
    }

    for (i = 0; i < EDMA_UT_REQ_MAX; i += 4)
    {
        for (j = 0; j < 4; j++)
        {
            edma_req[i + j]->fpCallback = edma_ut_callback;
            for (b = 0; b < edma_req[i + j]->u32BurstCnt; b++)
            {
                edma_req[i + j]->burst[b].sar = 0x00000000 + (0x11111111 * b);
                edma_req[i + j]->burst[b].dar = 0xFFFFFFFF - (0x11111111 * b);
                edma_req[i + j]->burst[b].sz  = i + j;
            }
        }
    }

    for (i = 0; i < EDMA_UT_REQ_MAX; i += 4)
    {
        drvEDMA_SubmitRequest(0, edma_req[i + 0]);
        drvEDMA_SubmitRequest(0, edma_req[i + 1]);
        drvEDMA_SubmitRequest(1, edma_req[i + 2]);
        drvEDMA_SubmitRequest(1, edma_req[i + 3]);
    }

    return 0;
}

int _edma_ut_init(void)
{
    return probe();
}

void _edma_ut_exit(void)
{
    drvEDMA_Cleanup(EDMA_WRITE_DIR);
    drvEDMA_Cleanup(EDMA_READ_DIR);
    return;
}
module_init(_edma_ut_init);
module_exit(_edma_ut_exit);
MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("EDMA UT");
