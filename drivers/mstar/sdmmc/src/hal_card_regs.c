////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2012 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (MStar Confidential Information) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************************************
 *
 * FileName hal_card_regs.c
 *     @author jeremy.wang (2015/06/01)
 * Desc:
 *     For Dynamic IP Address Reading and Port Setting.
 *     We could get current REG/CIFC/CIFD Bank Address anytime!
 *     We use global varible to record current port setting of IP, then we could use it to decide reg postions.
 *     The goal is that we don't need to change HAL Level code. (But its h file code)
 *
 *     The limitations were listed as below:
 *     (1) This c file could not use project/cpu/icver/specific define option here, but its h file could.
 *
 ***************************************************************************************************************/

#include "../inc/hal_card_regs.h"

static volatile PortEmType gePort[3];

void Hal_CREG_SET_PORT(IPEmType eIP, PortEmType ePort)
{
    gePort[eIP] = ePort;
}

volatile PortEmType Hal_CREG_GET_PORT(IPEmType eIP)
{
    return gePort[eIP];
}

volatile void* Hal_CREG_GET_REG_BANK(IPEmType eIP, U8_T u8Bank)
{
    void* pIPBANKArr[3][3] ={
    {(void*)(A_FCIE1_0_BANK), (void*)(A_FCIE1_1_BANK), (void*)(A_FCIE1_2_BANK)}, \
    {(void*)(A_FCIE2_0_BANK), (void*)(A_FCIE2_1_BANK), (void*)(A_FCIE2_2_BANK)}, \
    {(void*)(A_FCIE3_0_BANK), (void*)(A_FCIE3_1_BANK), (void*)(A_FCIE3_2_BANK)}\
    };

    return pIPBANKArr[eIP][u8Bank];

}
