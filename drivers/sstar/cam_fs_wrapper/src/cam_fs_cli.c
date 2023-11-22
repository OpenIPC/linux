/*
 * cam_fs_cli.c- Sigmastar
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

///////////////////////////////////////////////////////////////////////////////
/// @file      cam_fs_cli.c
/// @brief     Cam FS CLI Wrapper Source File for
///            1. RTK OS
///////////////////////////////////////////////////////////////////////////////

#if defined(__KERNEL__)
#define CAM_OS_LINUX_KERNEL
#endif

#ifdef CAM_OS_RTK
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "sys_sys_isw_cli.h"
#include "sys_sys_boot_timestamp.h"
#include "cam_os_wrapper.h"
#include "cam_fs_wrapper.h"
#include "sys_sys_md5.h"

#if defined(_SUPPORT_NAND_)
#include "drv_spinand.h"
#elif defined(_SUPPORT_NOR_)
#include "drv_spinor.h"
#endif

#if defined(_SUPPORT_NAND_)
#define CAMFS_CLI_FLASH_ENABLE DrvSpinandAvailable
#define CAMFS_CLI_FLASH_PROBE  DrvSpinandProbe
#elif defined(_SUPPORT_NOR_)
#define CAMFS_CLI_FLASH_ENABLE DrvSpinorAvailable
#define CAMFS_CLI_FLASH_PROBE  DrvSpinorProbe
#elif defined(_SUPPORT_NAND_ONEBIN_)
#define CAMFS_CLI_FLASH_ENABLE(a)
#define CAMFS_CLI_FLASH_PROBE(a)
#elif defined(_SUPPORT_NOR_ONEBIN_)
#define CAMFS_CLI_FLASH_ENABLE(a)
#define CAMFS_CLI_FLASH_PROBE(a)
#endif

#ifndef bool
typedef unsigned char bool;
#endif

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

#define CAMFS_CLI_BUFFER_SIZE (1024)

#if defined(__VER_CAMFS__)
static CamFsFd g_camFSTestFd;
static bool    g_camFSTestOpened;
static s32     g_camFSTestMode;

static s32 CamFs_cli_open(CLI_t *pCli, char *p)
{
    char *     path;
    u32        flags;
    u32        mode;
    CamFsRet_e eRet;

    if (CliTokenCount(pCli) != 3)
    {
        CamOsPrintf("CamFs_cli_open:open path flags mode\n");
        return 0;
    }

    path = CliTokenPop(pCli);
    if (!path || strlen(path) <= 0)
    {
        CamOsPrintf("CamFs_cli_open:path is empty\n");
        return 0;
    }

    if (CliTokenPopNum(pCli, &flags, 0) != eCLI_PARSE_OK)
    {
        CamOsPrintf("CamFs_cli_open:invalid flags\n");
        return 0;
    }

    if (CliTokenPopNum(pCli, &mode, 0) != eCLI_PARSE_OK)
    {
        CamOsPrintf("CamFs_cli_open:invalid flags\n");
        return 0;
    }

    if (g_camFSTestOpened)
    {
        CamFsClose(g_camFSTestFd);
        g_camFSTestOpened = false;
    }
    g_camFSTestMode = mode;

    eRet              = CamFsOpen(&g_camFSTestFd, path, flags, 0777);
    g_camFSTestOpened = eRet == CAM_FS_OK ? true : false;
    CamOsPrintf("CamFs_cli_open:%s!\n", eRet == CAM_FS_OK ? "success" : "failure");
    return 0;
}

static s32 CamFs_cli_close(CLI_t *pCli, char *p)
{
    if (CliTokenCount(pCli) > 0)
    {
        CamOsPrintf("CamFs_cli_close: too many arguments\n");
        return 0;
    }

    if (g_camFSTestOpened)
    {
        CamFsClose(g_camFSTestFd);
        g_camFSTestOpened = false;
    }

    return 0;
}

static s32 CamFs_cli_read(CLI_t *pCli, char *p)
{
    u32            size, i;
    s32            ret;
    unsigned char *buffer;

    if (CliTokenCount(pCli) != 1)
    {
        CamOsPrintf("CamFs_cli_read:read size\n");
        return 0;
    }

    if (CliTokenPopNum(pCli, &size, 0) != eCLI_PARSE_OK)
    {
        CamOsPrintf("CamFs_cli_read:invalid size\n");
        return 0;
    }

    if (size <= 0)
    {
        CamOsPrintf("CamFs_cli_read: size %d is invalid\n", size);
        return 0;
    }

    if (size > CAMFS_CLI_BUFFER_SIZE)
    {
        CamOsPrintf("CamFs_cli_read: size %d is too big -> %d\n", size, CAMFS_CLI_BUFFER_SIZE);
        size = CAMFS_CLI_BUFFER_SIZE;
    }

    if (!g_camFSTestOpened)
    {
        CamOsPrintf("CamFs_cli_read:no opening file\n");
        return 0;
    }

    // One byte for terminating '\0'
    buffer = CamOsMemAlloc(size + 1);
    if (!buffer)
    {
        CamOsPrintf("CamFs_cli_read:out of memory\n");
        return 0;
    }
    memset(buffer, 0, size + 1);
    buffer[0] = '\n';

    ret = CamFsRead(g_camFSTestFd, buffer, size);

    if (g_camFSTestMode == 0)
    {
        CamOsPrintf("CamFs_cli_read[text][%d]:%s!\n", ret, buffer);
    }
    else
    {
        CamOsPrintf("CamFs_cli_read[hex][%d]:", ret);
        for (i = 0; i < size; i++)
        {
            CamOsPrintf("%02x", (u32)buffer[i]);
        }
        CamOsPrintf("\n");
    }
    CamOsMemRelease(buffer);
    return 0;
}

static s32 CamFs_cli_write(CLI_t *pCli, char *p)
{
    s32            size, i, j;
    s32            ret;
    unsigned char *buffer = NULL;
    char *         content;

    if (CliTokenCount(pCli) != 1)
    {
        CamOsPrintf("CamFs_cli_write:write string\n");
        return 0;
    }

    content = CliTokenPop(pCli);
    if (!content)
    {
        CamOsPrintf("CamFs_cli_write:empty content!\n");
        return 0;
    }

    if (!g_camFSTestOpened)
    {
        CamOsPrintf("CamFs_cli_write:no opening file\n");
        return 0;
    }

    size = strlen(content);
    if (size > CAMFS_CLI_BUFFER_SIZE)
    {
        CamOsPrintf("CamFs_cli_write: content exceeds %d\n", CAMFS_CLI_BUFFER_SIZE);
        return 0;
    }

    if (g_camFSTestMode == 0)
    {
        ret = CamFsWrite(g_camFSTestFd, content, size);
        CamOsPrintf("CamFs_cli_write[text]:%d\n", ret);
    }
    else if (g_camFSTestMode == 1)
    {
        unsigned char lo, hi;

        if ((size % 2) != 0)
        {
            CamOsPrintf(
                "CamFs_cli_write[hex]: invalid size %d "
                "((%d % 2) != 0)\n",
                size, size);
            return 0;
        }

        buffer = CamOsMemAlloc(size / 2);
        if (!buffer)
        {
            CamOsPrintf("CamFs_cli_write:out of memory\n");
            return 0;
        }

        for (i = 0, j = 0; i < size; i += 2, j++)
        {
            if (content[i] >= 'A' && content[i] <= 'Z')
            {
                hi = content[i] - 'A' + 10;
            }
            else if (content[i] >= 'a' && content[i] <= 'z')
            {
                hi = content[i] - 'a' + 10;
            }
            else if (content[i] >= '0' && content[i] <= '9')
            {
                hi = content[i] - '0';
            }
            else
            {
                CamOsPrintf("CamFs_cli_write:invalid value 0x%x at %d\n", (u32)content[i], i);
                CamOsMemRelease(buffer);
                return 0;
            }

            if (content[i + 1] >= 'A' && content[i + 1] <= 'Z')
            {
                lo = content[i + 1] - 'A' + 10;
            }
            else if (content[i + 1] >= 'a' && content[i + 1] <= 'z')
            {
                lo = content[i + 1] - 'a' + 10;
            }
            else if (content[i + 1] >= '0' && content[i + 1] <= '9')
            {
                lo = content[i + 1] - '0';
            }
            else
            {
                CamOsPrintf("CamFs_cli_write:invalid value 0x%x at %d\n", (u32)content[i + 1], i + 1);
                CamOsMemRelease(buffer);
                return 0;
            }
            buffer[j] = (unsigned char)(hi << 4 | lo);
        }
        ret = CamFsWrite(g_camFSTestFd, buffer, size / 2);
        CamOsPrintf("CamFs_cli_write[hex]:%d\n", ret);
        CamOsMemRelease(buffer);
    }

    return 0;
}

static s32 CamFs_cli_seek(CLI_t *pCli, char *p)
{
    u32 offset;
    u32 whence;
    s32 ret;

    if (CliTokenCount(pCli) < 2)
    {
        CamOsPrintf("CamFs_cli_seek: too few arguments\n");
        return 0;
    }

    if (CliTokenPopNum(pCli, &offset, 0) != eCLI_PARSE_OK)
    {
        CamOsPrintf("CamFs_cli_seek:invalid offset\n");
        return 0;
    }

    if (CliTokenPopNum(pCli, &whence, 0) != eCLI_PARSE_OK)
    {
        CamOsPrintf("CamFs_cli_seek:invalid whence\n");
        return 0;
    }

    if (!g_camFSTestOpened)
    {
        CamOsPrintf("CamFs_cli_seek:no opening file\n");
        return 0;
    }

    ret = CamFsSeek(g_camFSTestFd, offset, whence);
    CamOsPrintf("CamFs_cli_seek:%d\n", ret);
    return 0;
}

static s32 CamFs_cli_cook(CLI_t *pCli, char *p)
{
    char *     path;
    u32        mode;
    s32        ret, i;
    CamFsFd    fd;
    CamFsRet_e eRet;

    if (CliTokenCount(pCli) < 2)
    {
        CamOsPrintf("CamFs_cli_cook: too few arguments\n");
        return 0;
    }

    path = CliTokenPop(pCli);
    if (!path || strlen(path) <= 0)
    {
        CamOsPrintf("CamFs_cli_cook:path is empty\n");
        return 0;
    }

    if (CliTokenPopNum(pCli, &mode, 0) != eCLI_PARSE_OK)
    {
        CamOsPrintf("CamFs_cli_cook:invalid mode argument\n");
        return 0;
    }

    if (mode == 0)
    {
        unsigned char *buffer;
        SSRtosMD5_CTX  md5;
        unsigned char  md5sum[16];
        char           str[64];

        eRet = CamFsOpen(&fd, path, O_RDONLY, 0777);
        if (eRet != CAM_FS_OK)
        {
            CamOsPrintf("CamFs_cli_cook: failed to open %s\n", path);
            return 0;
        }

        buffer = (unsigned char *)CamOsMemAlloc(CAMFS_CLI_BUFFER_SIZE);
        if (buffer == NULL)
        {
            CamFsClose(fd);
            CamOsPrintf("CamFs_cli_read:out of memory\n");
            return 0;
        }
        SSRtosMD5Init(&md5);
        while (0 < (ret = CamFsRead(fd, buffer, CAMFS_CLI_BUFFER_SIZE)))
        {
            SSRtosMD5Update(&md5, buffer, ret);
        }
        SSRtosMD5Final(&md5, md5sum);
        CamOsMemRelease(buffer);
        CamFsClose(fd);

        for (i = 0; i < 16; i++)
        {
            snprintf(str + i * 2, sizeof(str) - i * 2, "%02x", md5sum[i]);
        }
        CamOsPrintf("CamFs_cli_cook:md5sum:%s\n", str);
    }
    else
    {
        CamOsPrintf("CamFs_cli_cook:unknown mode %d\n", mode);
    }
    return 0;
}

static s32 CamFs_cli_flash(CLI_t *pCli, char *p)
{
    char *cmd;

    if (CliTokenCount(pCli) < 1)
    {
        CamOsPrintf("CamFs_cli_flash: too few arguments\n");
        return 0;
    }

    cmd = CliTokenPop(pCli);
    if (strcmp(cmd, "enable") == 0)
    {
        CAMFS_CLI_FLASH_ENABLE(1);
    }
    else if (strcmp(cmd, "disable") == 0)
    {
        CAMFS_CLI_FLASH_ENABLE(0);
    }
    else
    {
        CamOsPrintf("CamFs_cli_flash: unknown cmd %s\n", cmd);
    }
    return 0;
}

static s32 CamFs_cli_cp(CLI_t *pCli, char *p)
{
    char *         srcPath;
    char *         dstPath;
    s32            ret;
    CamFsFd        srcFd;
    CamFsFd        dstFd;
    CamFsRet_e     eRet;
    unsigned char *buffer;

    if (CliTokenCount(pCli) < 2)
    {
        CamOsPrintf("CamFs_cli_cook: too few arguments\n");
        return 0;
    }

    srcPath = CliTokenPop(pCli);
    if (!srcPath || strlen(srcPath) <= 0)
    {
        CamOsPrintf("CamFs_cli_cp:src path is empty\n");
        return 0;
    }

    dstPath = CliTokenPop(pCli);
    if (!dstPath || strlen(dstPath) <= 0)
    {
        CamOsPrintf("CamFs_cli_cp:dst path is empty\n");
        return 0;
    }

    eRet = CamFsOpen(&srcFd, srcPath, O_RDONLY, 0777);
    if (eRet != CAM_FS_OK)
    {
        CamOsPrintf("CamFs_cli_cp: failed to open %s\n", srcPath);
        return 0;
    }

    eRet = CamFsOpen(&dstFd, dstPath, O_RDWR | O_CREAT, 0777);
    if (eRet != CAM_FS_OK)
    {
        CamOsPrintf("CamFs_cli_cp: failed to open %s\n", dstPath);
        CamFsClose(srcFd);
        return 0;
    }

    buffer = (unsigned char *)CamOsMemAlloc(CAMFS_CLI_BUFFER_SIZE);
    if (buffer == NULL)
    {
        CamFsClose(dstFd);
        CamFsClose(srcFd);
        CamOsPrintf("CamFs_cli_read:out of memory\n");
        return 0;
    }

    while (0 < (ret = CamFsRead(srcFd, buffer, CAMFS_CLI_BUFFER_SIZE)))
    {
        ret = CamFsWrite(dstFd, buffer, ret);
        if (ret <= 0)
            break;
    }
    CamOsMemRelease(buffer);
    CamFsClose(dstFd);
    CamFsClose(srcFd);
    return 0;
}

int CamFs_test(CLI_t *pCli, char *p)
{
    char *cmd;
    s32   argc;

    argc = CliTokenCount(pCli);
    CamOsPrintf("CamFs_test:%s,%d\n", p ? p : "NULL", argc);

    if (argc < 1)
    {
        CamOsPrintf("CamFs_test: too few arguments!\n");
        return 0;
    }

    cmd = CliTokenPop(pCli);
    if (strcmp(cmd, "open") == 0)
    {
        CamFs_cli_open(pCli, p);
    }
    else if (strcmp(cmd, "close") == 0)
    {
        CamFs_cli_close(pCli, p);
    }
    else if (strcmp(cmd, "read") == 0)
    {
        CamFs_cli_read(pCli, p);
    }
    else if (strcmp(cmd, "write") == 0)
    {
        CamFs_cli_write(pCli, p);
    }
    else if (strcmp(cmd, "seek") == 0)
    {
        CamFs_cli_seek(pCli, p);
    }
    else if (strcmp(cmd, "cook") == 0)
    {
        CamFs_cli_cook(pCli, p);
    }
    else if (strcmp(cmd, "flash") == 0)
    {
        CamFs_cli_flash(pCli, p);
    }
    else if (strcmp(cmd, "cp") == 0)
    {
        CamFs_cli_cp(pCli, p);
    }
    else
    {
        CamOsPrintf("CamFs_test:unknown cmd %s\n", cmd);
    }

    return 0;
}
SS_RTOS_CLI_CMD(camfs, "Menu CamFS", "", CamFs_test);
#endif
#endif
