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
// MIPI RX P3 power down
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123600), 0x40);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123602), 0x03);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123608), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123609), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123610), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123611), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123618), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123619), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123640), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123641), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123646), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123647), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x12360b), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123613), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x12361b), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123643), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x123649), 0x08);
