/*
 * cam_dev_wrapper_test.c- Sigmastar
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vm_types.ht"
#include "sys_traces.ho"

#include "sys_sys_isw_cli.h"
#include "sys_sys_isw_uart.h"
#include "sys_sys_core.h"

#include "cam_os_wrapper.h"
#include "cam_dev_wrapper.h"

static void _TestCamDevPollThread_scl1_scl2(void *arg)
{
    int           i;
    int           handle[2];
    struct pollfd fds[2];
    int           nRetFd = -1;

    memset(fds, 0, sizeof(fds));
    handle[0] = CamDevOpen("/dev/mscldma1");
    if (handle[0] == CAM_OS_FAIL)
    {
        CamOsPrintf("CamDevOpen /dev/mscldma1 failed\n\r");
        return;
    }

    handle[1] = CamDevOpen("/dev/mscldma2");
    if (handle[1] == CAM_OS_FAIL)
    {
        CamOsPrintf("CamDevOpen /dev/mscldma2 failed\n\r");
        return;
    }

    CamOsPrintf("_TestCamDevPoll: CamDevOpen devfd=%d, %d\n\r", handle[0], handle[1]);

    for (i = 0; i < 2; i++)
    {
        fds[i].fd     = handle[i];
        fds[i].events = POLLIN;
    }

    for (i = 0; i < 10; i++)
    {
        if ((nRetFd = CamDevPoll(fds, 2, 200)) < 0)
        {
            CamOsPrintf("CamDevPoll failed\n\r");
            return;
        }
        CamOsPrintf("CamDevPoll Loop(%d) nRetFd=%d\n\r", i, nRetFd);
        CamOsMsSleep(10);
    }

    CamDevClose(handle[0]);
    CamDevClose(handle[1]);
}

static void _TestCamDevPollThread_1(void *arg)
{
    int           i;
    int           handle = *((int *)arg);
    struct pollfd fds[1];
    int           nRetFd   = -1;
    static int    nTimeout = 200;

    memset(fds, 0, sizeof(fds));
    fds[0].fd     = handle;
    fds[0].events = POLLIN;

    for (i = 0; i < 10; i++)
    {
        if ((nRetFd = CamDevPoll(fds, 1, 500)) < 0)
        {
            CamOsPrintf("CamDevPoll failed\n\r");
            return;
        }
        CamOsPrintf("_TestCamDevPollThread[%d] Loop(%d) nRetFd=%d\n\r", handle, i, nRetFd);
        CamOsMsSleep(nTimeout);
    }
    nTimeout += 100;
}

/* a single poll with 2 fds: scl1 & scl2 */
static void _TestCamDevPoll_case1()
{
    CamOsThreadAttrb_t attr;
    CamOsThread        thread;
    CamOsRet_e         eRet;

    attr.nPriority  = 50;
    attr.nStackSize = 4096;
    eRet            = CamOsThreadCreate(&thread, &attr, (void *)_TestCamDevPollThread_scl1_scl2, NULL);
    if (CAM_OS_OK != eRet)
    {
        CamOsPrintf("%s : Create poll thread fail\n\r", __FUNCTION__);
    }
    CamOsThreadJoin(thread);
}

/* two polls for the same fd */
static void _TestCamDevPoll_case2()
{
    CamOsThreadAttrb_t attr;
    CamOsThread        thread1, thread2;
    CamOsRet_e         eRet;
    int                handle;

    handle = CamDevOpen("/dev/mscldma1");
    if (handle == CAM_OS_FAIL)
    {
        CamOsPrintf("CamDevOpen /dev/mscldma1 failed\n\r");
        return;
    }

    CamOsPrintf("_TestCamDevPoll: poll devfd=%d\n\r", handle);

    attr.nPriority  = 50;
    attr.nStackSize = 4096;
    eRet            = CamOsThreadCreate(&thread1, &attr, (void *)_TestCamDevPollThread_1, &handle);
    if (CAM_OS_OK != eRet)
    {
        CamOsPrintf("%s : Create poll thread fail\n\r", __FUNCTION__);
    }
    eRet = CamOsThreadCreate(&thread2, &attr, (void *)_TestCamDevPollThread_1, &handle);
    if (CAM_OS_OK != eRet)
    {
        CamOsPrintf("%s : Create poll thread fail\n\r", __FUNCTION__);
    }

    CamOsThreadJoin(thread1);
    CamOsThreadJoin(thread2);
    CamDevClose(handle);
}

//=============================================================================
extern void TestCamDrvPoll_byCamDev(void);
extern void TestCamDrvPoll_Self(void);

s32 CamDevIoWrapperTest(CLI_t *pCli, char *p)
{
    int   i, ParamCnt, ret = -1;
    u32   case_num = 0;
    char *pEnd;
    /* char * value[10]; */

    ParamCnt = CliTokenCount(pCli);

    if (ParamCnt < 1)
    {
        return eCLI_PARSE_INPUT_ERROR;
    }

    for (i = 0; i < ParamCnt; i++)
    {
        pCli->tokenLvl++;
        p = CliTokenPop(pCli);
        if (i == 0)
        {
            // CLIDEBUG(("p: %s, len: %d\n", p, strlen(p)));
            //*pV = _strtoul(p, &pEnd, base);
            case_num = strtoul(p, &pEnd, 10);
            // CLIDEBUG(("*pEnd = %d\n", *pEnd));
            if (p == pEnd || *pEnd)
            {
                cliPrintf("Invalid input\n");
                return eCLI_PARSE_ERROR;
            }
        }
        /* else{
             value[i-1] = p;
        }*/
    }

    switch (case_num)
    {
        case 1:
            _TestCamDevPoll_case1();
            break;
        case 2:
            _TestCamDevPoll_case2();
            break;
        case 3:
            TestCamDrvPoll_byCamDev();
            break;
        case 4:
            TestCamDrvPoll_Self();
            break;
        default:
            ret = -1;
    }

    if (ret < 0)
        return eCLI_PARSE_ERROR;

    return eCLI_PARSE_OK;
}
