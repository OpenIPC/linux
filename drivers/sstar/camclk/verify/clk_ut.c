/*
 * clk_ut.c- Sigmastar
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

//==============================================================================
//
//                              INCLUDE FILES
//
//==============================================================================

#include "cam_os_wrapper.h"
#include "drv_camclk_st.h"
#include "drv_camclk.h"
#include "drv_camclk_Api.h"
#include "hal_camclk_if.h"
#include "camclk_dbg.h"
#define MAX_CTX_CNT 64
#define PAGE_SIZE   4096
u32  gu32HandlerCnt = 0;
void DrvCamclkUtCaseS(const char *buf, u32 n)
{
    int                  u32para[2];
    void *               phandler = NULL;
    long                 Temp;
    const char *         str = buf;
    CAMCLK_Set_Attribute stCfg;

    sscanf(str, "%d %d %lx", &u32para[0], &u32para[1], &Temp);
    CAMCLKINFO("CAMCLK UT S CID:%d Select:%d Create/ID:%x\n", u32para[0], u32para[1], Temp);

    if (Temp == 0)
    {
        CamClkRegister((u8 *)"Temp", u32para[0], &phandler);
    }
    else
    {
        phandler = (void *)Temp;
    }
    stCfg.eSetType            = CAMCLK_SET_ATTR_PARENT;
    stCfg.attribute.u32Parent = u32para[1];
    CamClkAttrSet(phandler, &stCfg);
    if ((Temp == 0))
    {
        CamClkUnregister(phandler);
    }
}

void DrvCamclkUtCaseE(const char *buf, u32 n)
{
    int         u32para[2];
    const char *str      = buf;
    void *      phandler = NULL;
    long        Temp;
    sscanf(str, "%d %d %lx", &u32para[0], &u32para[1], &Temp);
    CAMCLKINFO("CAMCLK UT E CID:%d enable:%d Create/ID:%x\n", u32para[0], u32para[1], Temp);
    if (Temp == 0)
    {
        CamClkRegister((u8 *)"Temp", u32para[0], &phandler);
    }
    else
    {
        phandler = (void *)Temp;
    }
    CamClkSetOnOff(phandler, (u8)u32para[1]);
    if ((Temp == 0))
    {
        CamClkUnregister(phandler);
    }
}
void DrvCamclkUtCaseF(const char *buf, u32 n)
{
    int                  u32para[3];
    void *               phandler = NULL;
    long                 Temp;
    const char *         str = buf;
    CAMCLK_Set_Attribute stCfg;

    sscanf(str, "%d %d %d %lx", &u32para[0], &u32para[1], &u32para[2], &Temp);
    CAMCLKINFO("CAMCLK UT F CID:%d Freq:%d Type:%d Create/ID:%x\n", u32para[0], u32para[1], u32para[2], Temp);
    if (Temp == 0)
    {
        CamClkRegister((u8 *)"Temp", u32para[0], &phandler);
    }
    else
    {
        phandler = (void *)Temp;
    }
    stCfg.eRoundType        = u32para[2];
    stCfg.eSetType          = CAMCLK_SET_ATTR_RATE;
    stCfg.attribute.u32Rate = u32para[1];
    CamClkAttrSet(phandler, &stCfg);
    if ((Temp == 0))
    {
        CamClkUnregister(phandler);
    }
}
void DrvCamclkUtCaseG(const char *buf, u32 n)
{
    void *               phandler = NULL;
    long                 Temp;
    const char *         str = buf;
    CAMCLK_Get_Attribute stCfg;
    int                  idx;

    sscanf(str, "%lx", &Temp);
    phandler = (void *)Temp;
    CAMCLKINFO("CAMCLK UT G DID:%p\n", phandler);
    CamClkAttrGet(phandler, &stCfg);
    CAMCLKERR("CAMCLK Freq:%d Parent cnt:%d ", stCfg.u32Rate, stCfg.u32NodeCount);
    for (idx = 0; idx < stCfg.u32NodeCount; idx++)
    {
        CAMCLKERR("   Parent ID:%d", stCfg.u32Parent[idx]);
    }
    CAMCLKERR("\n");
}
void DrvCamclkUtCaseC(const char *buf, u32 n)
{
    int         u32para;
    u8          hname[32];
    const char *str = buf;
    void *      pv;

    sscanf(str, "%d %s", &u32para, hname);
    CAMCLKINFO("CAMCLK UT C CID:%d Name:%s\n", u32para, hname);
    if (gu32HandlerCnt >= MAX_CTX_CNT)
    {
        CAMCLKERR("Ctx > 64\n");
        return;
    }
    CamClkRegister((u8 *)hname, u32para, &pv);
    CAMCLKERR("Ctx = %p Cnt= %d\n", pv, gu32HandlerCnt);
    gu32HandlerCnt++;
}
void DrvCamclkUtCaseD(const char *buf, u32 n)
{
    void *      phandler = NULL;
    long        Temp;
    const char *str = buf;

    sscanf(str, "%lx", &Temp);
    // sscanf(str, "%p", &phandler);
    phandler = (void *)Temp;
    CAMCLKINFO("CAMCLK UT D DID:%p\n", phandler);
    if (phandler)
    {
        CamClkUnregister(phandler);
        gu32HandlerCnt--;
    }
}
void DrvCamclkUtCmdParser(const char *buf, u32 n)
{
    char        cmd;
    int         u32size = 0;
    const char *str     = buf;
    // 1.choose cmd
    // 2.parser buf
    u32size += sscanf(str, "%c", &cmd);
    str += (u32size + 1);
    CAMCLKINFO("CAMCLK UT CMD:%c u32size:%d n:%d @%x\n", cmd, u32size, n, (u32)str);
    switch (cmd)
    {
        case 'C':
            DrvCamclkUtCaseC(str, n - (u32size + 1));
            break;
        case 'D':
            DrvCamclkUtCaseD(str, n - (u32size + 1));
            break;
        case 'E':
            DrvCamclkUtCaseE(str, n - (u32size + 1));
            break;
        case 'F':
            DrvCamclkUtCaseF(str, n - (u32size + 1));
            break;
        case 'G':
            DrvCamclkUtCaseG(str, n - (u32size + 1));
            break;
        case 'S':
            DrvCamclkUtCaseS(str, n - (u32size + 1));
            break;
        default:
            CAMCLKINFO("RGN UT CMD NOT SUPPORT:%c\n", cmd);
            break;
    }
}
u32 DrvCamclkUtShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += CamOsSnprintf(str, end - str, "======================== CamClk Ut ========================\n");
    str += CamOsSnprintf(str, end - str, "Case      CMD                                            Description\n");
    str += CamOsSnprintf(str, end - str, "create    echo C (CID) (NAME) > test                     Create instance\n");
    str += CamOsSnprintf(str, end - str, "destroy   echo D (HandleID) > test                       Destroy instance\n");
    str += CamOsSnprintf(str, end - str, "On/Off    echo E (CID) 0/1 (Create/ID) > test            On/Off Clock \n");
    str += CamOsSnprintf(str, end - str, "Freq      echo F (CID) (Freq) (Type) (Create/ID) > test  Set Clock Freq\n");
    str += CamOsSnprintf(str, end - str, "Get       echo G (Handle ID) > test                      Get Attr\n");
    str += CamOsSnprintf(str, end - str, "Sel       echo S (CID) (sel) (Create/ID) > test          Set Clock Parent\n");
    str += CamOsSnprintf(str, end - str, "======================== CamClk Ut ========================\n");
    return (str - buf);
}
