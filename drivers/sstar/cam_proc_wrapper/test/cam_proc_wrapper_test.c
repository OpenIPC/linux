/*
 * cam_proc_wrapper_test.c- Sigmastar
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

#if defined(__KERNEL__)
#define CAM_OS_LINUX_KERNEL
#endif

#include "cam_proc_wrapper.h"
#include "cam_os_wrapper.h"
#ifdef CAM_OS_RTK
#include "sys_sys_isw_cli.h"
#elif defined(CAM_OS_LINUX_KERNEL)
#include <linux/module.h>

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SStar Proc Wrapper Test");
MODULE_LICENSE("GPL");
#endif

/*  This file is an example about how to create and remove below nodes/entries :
 *  /VENC/
 *  /VENC/FPS
 *  /VENC/Entry1/
 *  /VENC/Entry1/Resolution
 *  /VENC/Entry1/Entry2/
 *  /VENC/Entry1/Entry2/Bitrate
 *  /VDEC/
 *  /VDEC/Dec_FPS
 *  /VDEC/Dec_Res
 *  /VDEC/Dec_VBR
 */

//==================VENC==================
struct CamProcEntry_t *gpVENCRootDir;
struct CamProcEntry_t *gpVENCSubDir1;
struct CamProcEntry_t *gpVENCSubDir2;

u32 gVencFpsVal      = 30;
u32 gVencResolutionW = 1920;
u32 gVencResolutionH = 1080;
u32 gVencBitrateVal  = 8 * 1024 * 1024;

void VENCFpsCat(CamProcSeqFile_t *pSeq, void *pPrivData)
{
    CamProcSeqPrintf(pSeq, "VENC FPS is %d\n", gVencFpsVal);
}

void VENCFpsEcho(char *pBuf, int nLen, void *pPrivData)
{
    gVencFpsVal = atoi(pBuf);
}

void VENCResolutionCat(CamProcSeqFile_t *pSeq, void *pPrivData)
{
    CamProcSeqPrintf(pSeq, "VENC Resolution is %dx%d\n", gVencResolutionW, gVencResolutionH);
}

void VENCResolutionEcho(char *pBuf, int nLen, void *pPrivData)
{
    gVencResolutionW = atoi(pBuf);
    pBuf             = strchr(pBuf, ' ');
    pBuf++; // skip space
    gVencResolutionH = atoi(pBuf);
}

void VENCBitrateCat(CamProcSeqFile_t *pSeq, void *pPrivData)
{
    CamProcSeqPrintf(pSeq, "VENC bitrate is %d\n", gVencBitrateVal);
}

void VENCBitrateEcho(char *pBuf, int nLen, void *pPrivData)
{
    gVencBitrateVal = atoi(pBuf);
}

//==================VDEC==================
struct CamProcEntry_t *gpVDECRootDir;
u32                    gVDecFpsVal      = 25;
u32                    gVDecResolutionW = 1280;
u32                    gVDecResolutionH = 720;
u32                    gVDecBitrateVal  = 16 * 1024 * 1024;

void VDECFpsCat(CamProcSeqFile_t *pSeq, void *pPrivData)
{
    CamProcSeqPrintf(pSeq, "VDEC FPS is %d\n", gVDecFpsVal);
}

void VDECFpsEcho(char *pBuf, int nLen, void *pPrivData)
{
    gVDecFpsVal = atoi(pBuf);
}

void VDECResolutionCat(CamProcSeqFile_t *pSeq, void *pPrivData)
{
    CamProcSeqPrintf(pSeq, "VDEC Resolution is %dx%d\n", gVDecResolutionW, gVDecResolutionH);
}

void VDECResolutionEcho(char *pBuf, int nLen, void *pPrivData)
{
    gVDecResolutionW = atoi(pBuf);
    pBuf             = strchr(pBuf, ' ');
    pBuf++; // skip space
    gVDecResolutionH = atoi(pBuf);
}

void VDECBitrateCat(CamProcSeqFile_t *pSeq, void *pPrivData)
{
    CamProcSeqPrintf(pSeq, "VDEC bitrate is %d\n", gVDecBitrateVal);
}

void VDECBitrateEcho(char *pBuf, int nLen, void *pPrivData)
{
    gVDecBitrateVal = atoi(pBuf);
}

#ifdef CAM_OS_RTK
static int CamProcTestInit(void)
#elif defined(CAM_OS_LINUX_KERNEL)
static int __init  CamProcTestInit(void)
#endif
{
    struct CamProcEntry_t *pTmpEntry;

    CamOsPrintf("Create VENC\n");
    //==================Create VENC==================
    gpVENCRootDir = CamProcMkdir("VENC", NULL);

    if (!gpVENCRootDir)
    {
        CamOsPrintf("%s L:%d, Mkdir fail in CamProcMkdir\n", __FUNCTION__, __LINE__);
    }

    pTmpEntry = CamProcCreate("FPS", gpVENCRootDir, VENCFpsCat, VENCFpsEcho, NULL);

    if (!pTmpEntry)
    {
        CamOsPrintf("%s L:%d, Create entry fail in CamProcCreate\n", __FUNCTION__, __LINE__);
    }

    gpVENCSubDir1 = CamProcMkdir("Entry1", gpVENCRootDir);

    if (!gpVENCSubDir1)
    {
        CamOsPrintf("%s L:%d, Mkdir fail in CamProcMkdir\n", __FUNCTION__, __LINE__);
    }

    pTmpEntry = CamProcCreate("Resolution", gpVENCSubDir1, VENCResolutionCat, VENCResolutionEcho, NULL);

    if (!pTmpEntry)
    {
        CamOsPrintf("%s L:%d, Create entry fail in CamProcCreate\n", __FUNCTION__, __LINE__);
    }

    gpVENCSubDir2 = CamProcMkdir("Entry2", gpVENCSubDir1);

    if (!gpVENCSubDir2)
    {
        CamOsPrintf("%s L:%d, Mkdir fail in CamProcMkdir\n", __FUNCTION__, __LINE__);
    }

    pTmpEntry = CamProcCreate("Bitrate", gpVENCSubDir2, VENCBitrateCat, VENCBitrateEcho, NULL);

    if (!pTmpEntry)
    {
        CamOsPrintf("%s L:%d, Create entry fail in CamProcCreate\n", __FUNCTION__, __LINE__);
    }

    CamOsPrintf("Create VDEC\n");
    //==================Create VDEC==================
    gpVDECRootDir = CamProcMkdir("VDEC", NULL);

    if (!gpVDECRootDir)
    {
        CamOsPrintf("%s L:%d, Mkdir fail in CamProcMkdir\n", __FUNCTION__, __LINE__);
    }

    pTmpEntry = CamProcCreate("Dec_FPS", gpVDECRootDir, VDECFpsCat, VDECFpsEcho, NULL);

    if (!pTmpEntry)
    {
        CamOsPrintf("%s L:%d, Create entry fail in CamProcCreate\n", __FUNCTION__, __LINE__);
    }

    pTmpEntry = CamProcCreate("Dec_Res", gpVDECRootDir, VDECResolutionCat, VDECResolutionEcho, NULL);

    if (!pTmpEntry)
    {
        CamOsPrintf("%s L:%d, Create entry fail in CamProcCreate\n", __FUNCTION__, __LINE__);
    }

    pTmpEntry = CamProcCreate("Dec_VBR", gpVDECRootDir, VDECBitrateCat, VDECBitrateEcho, NULL);

    if (!pTmpEntry)
    {
        CamOsPrintf("%s L:%d, Create entry fail in CamProcCreate\n", __FUNCTION__, __LINE__);
    }

    return 0;
}

#ifdef CAM_OS_RTK
static void CamProcTestExit(void)
#elif defined(CAM_OS_LINUX_KERNEL)
static void __exit CamProcTestExit(void)
#endif
{
    CamOsPrintf("Remove VENC\n");
    // Remove method can be one of below :
    //==================Remove VENC==================
    // 1.remove all sub entries/nodes by only one step
    CamProcRemoveEntry("VENC", NULL);

    CamOsPrintf("Remove VDEC\n");
    //==================Remove VDEC==================
    // 2-1.remove sub-nodes first
    CamProcRemoveEntry("Dec_FPS", gpVDECRootDir);
    CamProcRemoveEntry("Dec_Res", gpVDECRootDir);
    CamProcRemoveEntry("Dec_FPS", gpVDECRootDir); // Just test double free case
    CamProcRemoveEntry("Dec_VBR", gpVDECRootDir);
    // 2-2.then remove directory entries
    CamProcRemoveEntry("VDEC", NULL);
}

#ifdef CAM_OS_RTK
int CamProcWrapperTest(CLI_t *pCli, char *p)
{
    unsigned int nParam;

    if (CliTokenCount(pCli) != 1)
    {
        return eCLI_PARSE_ERROR;
    }

    if (CliTokenPopNum(pCli, &nParam, 0) != eCLI_PARSE_OK)
    {
        return eCLI_PARSE_ERROR;
    }

    if (nParam == 1)
    {
        CamProcTestInit();
    }
    else if (nParam == 2)
    {
        CamProcTestExit();
    }
    else
    {
        return eCLI_PARSE_ERROR;
    }

    return eCLI_PARSE_OK;
}
SS_RTOS_CLI_CMD(proctest, "Proc wrapper test", "param : 1 to create nodes,2 to remove nodes", CamProcWrapperTest);
#elif defined(CAM_OS_LINUX_KERNEL)
module_init(CamProcTestInit);
module_exit(CamProcTestExit);
#endif
