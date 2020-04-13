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
// MIPI RX P2 power down
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c00), 0x40);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c02), 0x03);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c08), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c09), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c10), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c11), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c18), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c19), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c40), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c41), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c46), 0xa0);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c47), 0x04);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c0b), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c13), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c1b), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c43), 0x08);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x122c49), 0x08);
