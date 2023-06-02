//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
//******************************************************************************
//<MStar Software>
// MIPI RX P0 power down
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122600), 0x40);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122602), 0x03);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122608), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122609), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122610), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122611), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122618), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122619), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122640), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122641), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122646), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122647), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x12260b), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122613), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x12261b), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122643), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122649), 0x08);
