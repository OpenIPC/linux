/*
 * drv_camclk_module.c- Sigmastar
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

#include "ms_platform.h"
#include "ms_msys.h"
#include "cam_os_wrapper.h"
#include "drv_camclk_Api.h"
#include "drv_camclk_DataType.h"
#include "drv_camclk.h"
#include "camclk_dbg.h"
#include "sys_sys_isw_cli.h"
//-------------------------------------------------------------------------------------------------
// Define & Macro
//-------------------------------------------------------------------------------------------------

#define DRV_CAMCLK_DEVICE_COUNT    1
#define DRV_CAMCLK_DEVICE_NAME     "camclk"
#define DRV_CAMCLK_DEVICE_MAJOR    0x8a
#define DRV_CAMCLK_DEVICE_MINOR    0x0
#define DRV_CAMCLK_DEVICE_NODE     "camdriver,camclk"
#define DRV_CAMCLK_DEVICEINIT_NODE "camdriver,camclkinit"
//-------------------------------------------------------------------------------------------------
// Prototype
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    int    argc;
    char **argv;
} CamclkStringConfig_t;
//-------------------------------------------------------------------------------------------------
// Variable
//-------------------------------------------------------------------------------------------------
u8 u8init = 0;

void DrvCamclkUtCaseC(const char *buf, u32 n);
void DrvCamclkUtCaseD(const char *buf, u32 n);
void DrvCamclkUtCaseE(const char *buf, u32 n);
void DrvCamclkUtCaseF(const char *buf, u32 n);
void DrvCamclkUtCaseG(const char *buf, u32 n);
void DrvCamclkUtCaseS(const char *buf, u32 n);
void CamClkPrintInfo(char *buf)
{
    char *cur   = buf;
    char *token = NULL;
    char  del[] = "\n";
    do
    {
        token = strsep(&cur, del);
        CamOsPrintf("%s\n", token);
    } while (token);
}
void CamClkGetDebuginfo(CamclkStringConfig_t *pstStrCfg)
{
#ifdef CONFIG_CAM_CLK_SYSFS
    char *str;

    str = CamOsMemAlloc(4096);
    if (!strcmp(pstStrCfg->argv[0], "clkinfo"))
    {
        DrvCamclkDebugClkShow(str);
    }
    else if (!strcmp(pstStrCfg->argv[0], "handlerinfo"))
    {
        DrvCamclkDebugHandlerShow(str);
    }
    CamClkPrintInfo(str);
    CamOsMemRelease(str);
#endif
}

void CamClkCliParser(CamclkStringConfig_t *pstStrCfg)
{
#ifdef CAMCLK_RTK_UNITTEST
    u8    u8level;
    char  str[128];
    char *strstart = 0;
    // char blank = ' ';
    char cmd;
#endif
    if (NULL != pstStrCfg)
    {
#ifdef CONFIG_CAM_CLK_SYSFS
        if (pstStrCfg->argc == 1)
        {
            CamClkGetDebuginfo(pstStrCfg);
            return;
        }
#endif
#ifdef CAMCLK_RTK_UNITTEST
        memset(str, ' ', 128);
        strstart = str;
        for (u8level = 0; u8level < pstStrCfg->argc; u8level++)
        {
            CAMCLKERR("%s\n", pstStrCfg->argv[u8level]);
            if (u8level > 0)
            {
                CAMCLKERR("strstart:%p\n", strstart);
                strstart += CamOsSnprintf(strstart, 128, "%s ", pstStrCfg->argv[u8level]);
                CAMCLKERR("%s\n", str);
            }
        }
        cmd = pstStrCfg->argv[0][0];
        switch (cmd)
        {
            case 'C':
                DrvCamclkUtCaseC(str, 0);
                break;
            case 'D':
                DrvCamclkUtCaseD(str, 0);
                break;
            case 'E':
                DrvCamclkUtCaseE(str, 0);
                break;
            case 'F':
                DrvCamclkUtCaseF(str, 0);
                break;
            case 'G':
                DrvCamclkUtCaseG(str, 0);
                break;
            case 'S':
                DrvCamclkUtCaseS(str, 0);
                break;
            default:
                CAMCLKERR("RGN UT CMD NOT SUPPORT:%c\n", pstStrCfg->argv[0]);
                break;
        }
#endif
    }
}
static char _szCamClkCliAHelpTxt[]  = "camclk:clkinfo/Handlerinfo\n";
static char _szCamClkCliAUsageTxt[] = "echo C/E/F/...\n";
/*=============================================================*/
// Global Variable definition
/*=============================================================*/
int _CamClkCli(CLI_t *pCli, char *p);

CliParseToken_t g_atCamclkMenuTbl[] = {{"echo", _szCamClkCliAHelpTxt, _szCamClkCliAUsageTxt, _CamClkCli, NULL},
                                       PARSE_TOKEN_DELIMITER};
int             _CamClkCli(CLI_t *pvCli, char *p)
{
    u32                  idx;
    CLI_t *              pCli = pvCli;
    CamclkStringConfig_t stTest;
    char **              ppargv = CamOsMemAlloc(320 * sizeof(char *));

    stTest.argv = ppargv;
    stTest.argc = CliTokenCount(pCli);
    for (idx = 0; idx < stTest.argc; idx++)
    {
        pCli->tokenLvl++;
        stTest.argv[idx] = CliTokenPop(pCli);
    }
    CamClkCliParser(&stTest);
    CamOsMemRelease(ppargv);
    return 0;
}

void _DrvCamClkModuleInit(void)
{
    if (u8init == 0)
    {
        CAM_CLK_REQOSLOCK();
        CamClkInit();
        DrvCamClkOsPrepareShareMemory(DRV_CAMCLK_SHAREMEM_TOPCURRENT);
    }
}

void _DrvCamClkModuleDeInit(void)
{
    if (u8init)
    {
        CAMCLKERR("[%s @ %d]\n", __FUNCTION__, __LINE__);
        DrvCamClkOsDestroyShareMemory(DRV_CAMCLK_SHAREMEM_TOPCURRENT);
        CamClkDeinit();
        CAM_CLK_FREEOSLOCK();
    }
}

void CamClk_init(void *p)
{
    CAM_CLK_RECORD("CamClkInit+");
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);
    _DrvCamClkModuleInit();
    u8init = 1;
    CAM_CLK_RECORD("CamClkInit-");
}
