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
// MIPI RX P1 power down
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122900), 0x40);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122902), 0x03);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122908), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122909), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122910), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122911), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122918), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122919), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122940), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122941), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122946), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122947), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x12290b), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122913), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x12291b), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122943), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122949), 0x08);
