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
// MIPI COMBO TX power down
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150800), 0x40);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150802), 0x03);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150808), 0x20);
//[4]: LPTX0_EN
//[7:6]: HSTX0_EN[1:0]  
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150809), 0x01);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x15080e), 0x20);
//[4]: LPTX1_EN
//[7:6]: HSTX1_EN[1:0]  
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x15080f), 0x01);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150814), 0x20);
//[4]: LPTX2_EN[0]
//[7:6]: HSTX2_EN[1:0]  
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150815), 0x01);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150822), 0x20);
//[4]: LPTX3_EN[0]
//[7:6]: HSTX3_EN[1:0]  
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150823), 0x01);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150828), 0x20);
//[4]: LPTX4_EN[0]
//[7:6]: HSTX4_EN[1:0]  
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150829), 0x01);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150851), 0x01);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150856), 0x20);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x150859), 0x7C);
OUTREG8(GET_REG8_ADDRE(RIU_BASE_ADDR, 0x171006), 0x00);
