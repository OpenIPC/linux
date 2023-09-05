/*
* hal_card_regs.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: joe.su <joe.su@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

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
    static IPEmType ip;
    static U8_T bank;
    void* pIPBANKArr[3][3] = {
    {(void*)(A_FCIE1_0_BANK), (void*)(A_FCIE1_1_BANK), (void*)(A_FCIE1_2_BANK)}, \
    {(void*)(A_FCIE2_0_BANK), (void*)(A_FCIE2_1_BANK), (void*)(A_FCIE2_2_BANK)}, \
    {(void*)(A_FCIE3_0_BANK), (void*)(A_FCIE3_1_BANK), (void*)(A_FCIE3_2_BANK)}\
    };
    if(eIP != ip) {/*printk("Get IP%d\n", eIP);*/ ip = eIP;}
    if(u8Bank != bank) {printk("new bank%d\n", u8Bank); bank = u8Bank;}

    return pIPBANKArr[eIP][u8Bank];

}
