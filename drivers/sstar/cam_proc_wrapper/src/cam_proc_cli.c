/*
 * cam_proc_cli.c- Sigmastar
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cam_os_wrapper.h"
#include "cam_proc_common.h"
#include "cam_proc_rtos_util.h"
#include "sys_sys_isw_cli.h"
#include "sys_sys_isw_uart.h"

int CamProcCat(char *p)
{
    CamProcSeqFile_t seqFile = {0};

    seqFile.buf  = CamOsMemCalloc(256, 1);
    seqFile.size = 256;
    seqFile.used = 0;

    CamProcExecRead(p, &seqFile);

    CamOsPrintf("%s", seqFile.buf);

    CamOsMemRelease(seqFile.buf);

    return eCLI_PARSE_OK;
}

int CamProcEcho(char *p)
{
    // u32 nStrLen;
    char *entryPath;
    char *pOutChar;

    // UartSendTrace("p=%s\n",p);

    pOutChar = strstr(p, ">");

    if (!pOutChar)
        return eCLI_PARSE_ERROR;

    entryPath = pOutChar + 1; // skip ">"
    entryPath = strstr(entryPath, "/proc");

    if (!entryPath)
        return eCLI_PARSE_ERROR;

    entryPath += strlen("/proc");

    *(p + (pOutChar - p - 1)) = 0;
    CamProcExecWrite(entryPath, p);

    return eCLI_PARSE_OK;
}

int CamProcLs(char *p)
{
    CamProcListEntry(p);

    return eCLI_PARSE_OK;
}

static int _Cat(CLI_t *pCli, char *p)
{
    char *pTmpChar = NULL;
    bool  bOrgBufferModeEnable;
    int   Ret;

    if (CliTokenCount(pCli) != 1)
    {
        return eCLI_PARSE_INVALID_PARAMETER;
    }

    pTmpChar = CliTokenPop(pCli);
    pTmpChar = strstr(pTmpChar, "/proc");

    if (!pTmpChar)
    {
        CamOsPrintf("Not support real file or path error\n");
        return eCLI_PARSE_ERROR;
    }

#if defined(__CONSOLE__)
    bOrgBufferModeEnable = console_modify_buffer(0, 1, 0);
#else
    bOrgBufferModeEnable = drv_uart_dbg_is_buf_mode_enable();
    if (bOrgBufferModeEnable)
    {
        drv_uart_dbg_set_buf_mode_en_no_print(FALSE);
        drv_uart_dbg_flush_buf_data_to_uart();
    }
#endif

    Ret = CamProcCat(pTmpChar + strlen("/proc"));

    if (bOrgBufferModeEnable)
    {
#if defined(__CONSOLE__)
        console_modify_buffer(bOrgBufferModeEnable, 0, 0);
#else
        drv_uart_dbg_set_buf_mode_en_no_print(TRUE);
#endif
    }
    return Ret;
}
SS_RTOS_CLI_CMD(cat, "show context of a proc node", "Usage: cat /proc/cmdq/fps", _Cat);

static int _Echo(CLI_t *pCli, char *p)
{
    char *pTmpChar = NULL;
    int   Ret;
    char  aStrBuf[512];

    CliReassembleToken(pCli, aStrBuf, sizeof(aStrBuf));
    pTmpChar = strstr(aStrBuf, "/proc");

    if (!pTmpChar)
    {
        CamOsPrintf("Not support real file or path error\n");
        return eCLI_PARSE_ERROR;
    }

    Ret = CamProcEcho(aStrBuf);

    return Ret;
}
SS_RTOS_CLI_CMD(echo, "write string into a proc node", "Usage: echo 30 > /proc/cmdq/fps", _Echo);

static int _Procls(CLI_t *pCli, char *p)
{
    char *pTmpChar = NULL;
    int   Ret;
    char  aStrBuf[128];

    CliReassembleToken(pCli, aStrBuf, sizeof(aStrBuf));
    pTmpChar = strstr(aStrBuf, "/proc");

    if (!pTmpChar)
    {
        aStrBuf[0] = '/';
        aStrBuf[1] = 0;
        Ret        = CamProcLs(aStrBuf);
    }
    else
    {
        Ret = CamProcLs(pTmpChar + strlen("/proc"));
    }

    return Ret;
}
SS_RTOS_CLI_CMD(procls, "show directory entries and nodes of proc", "Usage: procls /proc", _Procls);
