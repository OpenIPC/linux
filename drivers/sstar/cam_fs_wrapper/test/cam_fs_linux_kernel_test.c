/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sigmastar Confidential
Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/


///////////////////////////////////////////////////////////////////////////////
/// @file      cam_fs_linux_kernel_test.c
/// @brief     Cam FS Wrapper Test Code for Linux Kernel Space
///////////////////////////////////////////////////////////////////////////////

#include <linux/kernel.h>
#include <linux/module.h>
#include "cam_os_wrapper.h"
#include "cam_fs_wrapper.h"

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SStar CamFS Linux Kernel Test");
MODULE_LICENSE("GPL");

static char *filename = "";
module_param(filename, charp, 0660);

static int __init KernelTestInit(void)
{
    CamFsFd tFD;
    void *pBuf = NULL;
    u32 nReadRet = 0;
    s32 filelen;

    CamOsPrintf("Test CamFs Start\n");

    if (CAM_FS_OK != CamFsOpen(&tFD, filename, O_RDONLY, 0))
    {
        CamOsPrintf("Open %s FAIL\n", filename);
    }
    else
    {
        CamOsPrintf("Open %s SUCCESS\n", filename);

        filelen = CamFsSeek(tFD, 0, SEEK_END);
        CamOsPrintf("file len: %d\n", filelen);
        CamFsSeek(tFD, 0, SEEK_SET);
        pBuf = CamOsMemAlloc(filelen);
        nReadRet = CamFsRead(tFD, pBuf, filelen);
        CamOsHexdump(pBuf, filelen);
        CamOsMemRelease(pBuf);

        CamFsClose(tFD);
    }

    CamOsPrintf("Test CamFs End\n");
    return 0;
}

static void __exit KernelTestExit(void)
{
    CamOsPrintf(KERN_INFO "Goodbye\n");
}

module_init(KernelTestInit);
module_exit(KernelTestExit);
