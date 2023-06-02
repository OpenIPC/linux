/*
    Header file for USB controller register

    This file is the header file that define register for USB module

    @file       usb_reg.h
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef __USB2DEV_REG_H__
#define __USB2DEV_REG_H__

#include "rcw_macro.h"
#include "kwrap/type.h"

//-------------------------------------------------------
//  USB Host registers (EHCI)
//-------------------------------------------------------

// 0x00, HC Capability Register
#define USB_HCCAP_REG_OFS               0x00
REGDEF_BEGIN(USB_HCCAP_REG)
REGDEF_BIT(CAP_LENGTH, 8)        // Capability registers length
REGDEF_BIT(, 8)
REGDEF_BIT(HOST_CTRL_INT_VER, 16) // Host controller interface version number
REGDEF_END(USB_HCCAP_REG)

// 0x04, HC Structural parameters
#define USB_HCSTRUCTPARAM_REG_OFS       0x04
REGDEF_BEGIN(USB_HCSTRUCTPARAM_REG)
REGDEF_BIT(N_PORTS, 4)           // Number of ports.
REGDEF_BIT(, 28)
REGDEF_END(USB_HCSTRUCTPARAM_REG)

// 0x08, HC Capability parameters
#define USB_HCCAPPARAM_REG_OFS          0x08
REGDEF_BEGIN(USB_HCCAPPARAM_REG)
REGDEF_BIT(, 1)
REGDEF_BIT(PROG_FR_LIST_FLAG, 1) // Programmable frame list flag
REGDEF_BIT(ASCH_PARK_CAP, 1)     // Asynchronous schedule park capability
REGDEF_BIT(, 29)
REGDEF_END(USB_HCCAPPARAM_REG)

// 0x10, HC USB Command Register
#define USB_MCR_REG_OFS                 0x10
REGDEF_BEGIN(USB_MCR_REG)
REGDEF_BIT(RS, 1)                // Run/stop
REGDEF_BIT(HC_RESET, 1)          // Host controller reset
REGDEF_BIT(FRL_SIZE, 2)          // Frame list size
REGDEF_BIT(PSCH_EN, 1)           // Periodic schedule enable
REGDEF_BIT(ASCH_EN, 1)           // Asynchronous schedule enable
REGDEF_BIT(INT_OAAD, 1)          // Interrupt on asynchronous advance doorbell
REGDEF_BIT(, 1)
REGDEF_BIT(ASYN_PK_CNT, 2)       // Asynchronous schedule park mode count
REGDEF_BIT(, 1)
REGDEF_BIT(ASYN_PK_EN, 1)        // Asynchronous schedule park mode enable
REGDEF_BIT(, 4)
REGDEF_BIT(INT_THRC, 8)          // Interrupt threshold control
REGDEF_BIT(, 8)
REGDEF_END(USB_MCR_REG)

// 0x14, HC USB Status Register
#define USB_HCUSBSTATUS_REG_OFS         0x14
REGDEF_BEGIN(USB_HCUSBSTATUS_REG)
REGDEF_BIT(USB_INT, 1)           // USB interrupt
REGDEF_BIT(USBERR_INT, 1)        // USB error interrupt
REGDEF_BIT(PO_CHG_DET, 1)        // Port change detect
REGDEF_BIT(FRL_ROL, 1)           // Frame list rollover
REGDEF_BIT(H_SYSERR, 1)          // Host system error
REGDEF_BIT(INT_OAA, 1)           // Interrupt on async advance
REGDEF_BIT(, 6)
REGDEF_BIT(HC_HALTED, 1)         // Host controller halted
REGDEF_BIT(RECLAMATION, 1)       // read only, used to detect empty asynchronous schedule
REGDEF_BIT(PSCH_STS, 1)          // Periodic schedule status
REGDEF_BIT(ASCH_STS, 1)          // Asynchronous schedule status
REGDEF_BIT(, 16)
REGDEF_END(USB_HCUSBSTATUS_REG)

// 0x18, HC USB Interrupt Enable Register
#define USB_HCUSBINTEN_REG_OFS          0x18
REGDEF_BEGIN(USB_HCUSBINTEN_REG)
REGDEF_BIT(USB_INT_EN, 1)        // USB interrupt enable
REGDEF_BIT(USBERR_INT_EN, 1)     // USB error interrupt enable
REGDEF_BIT(PO_CHG_INT_EN, 1)     // Port change interrupt enable
REGDEF_BIT(FRL_ROL_EN, 1)        // Frame list rollover enable
REGDEF_BIT(H_SYSERR_EN, 1)       // Host system error enable
REGDEF_BIT(INT_OAA_EN, 1)        // Interrupt on async advance enable
REGDEF_BIT(, 26)
REGDEF_END(USB_HCUSBINTEN_REG)

// 0x1C, HC Frame Index Register
#define USB_HCFRAMEINDEX_REG_OFS        0x1c
REGDEF_BEGIN(USB_HCFRAMEINDEX_REG)
REGDEF_BIT(FRINDEX, 14)          // index the frame into the periodic frame list
REGDEF_BIT(, 18)
REGDEF_END(USB_HCFRAMEINDEX_REG)

// 0x24, HC Periodic Frame List Base Address Register
#define USB_HCPERI_BASEADR_REG_OFS      0x24
REGDEF_BEGIN(USB_HCPERI_BASEADR_REG)
REGDEF_BIT(, 12)
REGDEF_BIT(PERI_BASADR, 20)      // Periodic frame list base address
REGDEF_END(USB_HCPERI_BASEADR_REG)

// 0x28, HC Current Asynchronous List Address Register
#define USB_HCASYNC_LISTADR_REG_OFS     0x28
REGDEF_BEGIN(USB_HCASYNC_LISTADR_REG)
REGDEF_BIT(, 5)
REGDEF_BIT(ASYNC_LADR, 27)        // Current asynchronous list address
REGDEF_END(USB_HCASYNC_LISTADR_REG)

// 0x30, HC Port Status and Control Register
#define USB_HCPORTSTACTRL_REG_OFS       0x30
REGDEF_BEGIN(USB_HCPORTSTACTRL_REG)
REGDEF_BIT(CONN_STS, 1)          // Current connect status
REGDEF_BIT(CONN_CHG, 1)          // Connect status change
REGDEF_BIT(PO_EN, 1)             // Port enable/disable
REGDEF_BIT(PO_EN_CHG, 1)         // Port enable/disable change
REGDEF_BIT(, 2)
REGDEF_BIT(F_PO_RESM, 1)         // Force port resume
REGDEF_BIT(PO_SUSP, 1)           // Fort suspend
REGDEF_BIT(PO_RESET, 1)          // 0: port is not in reset, 1: port is in reset
REGDEF_BIT(, 1)
REGDEF_BIT(LINE_STS, 2)          // Line status
REGDEF_BIT(, 4)
REGDEF_BIT(PORT_TEST, 4)         // Port test control
REGDEF_BIT(HC_TST_PKDONE, 1)     // Data transfer is done for the test packet
REGDEF_BIT(, 11)
REGDEF_END(USB_HCPORTSTACTRL_REG)

// 0x40, HC Misc. Register
#define USB_HCMISC_REG_OFS              0x40
REGDEF_BEGIN(USB_HCMISC_REG)
REGDEF_BIT(ASYN_SCH_SLPT, 2)     // Asynchronous Schedule Sleep Timer Controls the Asynchronous Schedule Sleep Timer
REGDEF_BIT(EOF1_Time, 2)         // EOF1 timing points
REGDEF_BIT(EOF2_Time, 2)         // EOF2 timing points
REGDEF_BIT(HostPhy_Suspend, 1)   // Host transceiver suspend mode
REGDEF_BIT(, 1)
REGDEF_BIT(HC_WKP_DET_EN, 1)
REGDEF_BIT(HC_CONN_DET_EN, 1)
REGDEF_BIT(, 6)
REGDEF_BIT(RESM_SE0_CNT, 6)
REGDEF_BIT(, 10)
REGDEF_END(USB_HCMISC_REG)

// 0x44, HC FS EOF1 Timing Point Register
#define USB_HCFSEOF_REG_OFS             0x44
REGDEF_BEGIN(USB_HCFSEOF_REG)
REGDEF_BIT(FS_EOF1_TIME, 12)     // Full-speed EOF 1 timing points, unit: UCLK T
REGDEF_BIT(FS_EOF1_TIME_125, 3)  // Full-speed EOF 1 timing points, unit: 125 us
REGDEF_BIT(, 17)
REGDEF_END(USB_HCFSEOF_REG)

// 0x48, HC HS EOF1 Timing Point Register
#define USB_HCHSEOF_REG_OFS             0x44
REGDEF_BEGIN(USB_HCHSEOF_REG)
REGDEF_BIT(FS_EOF1_TIME, 12)     // High-speed EOF 1 timing points, unit: UCLK T
REGDEF_BIT(, 20)
REGDEF_END(USB_HCHSEOF_REG)

//-------------------------------------------------------
//  USB OTG registers
//-------------------------------------------------------

// 0x80, OTG Control/Status Register
#define USB_OTGCTRLSTATUS_REG_OFS       0x80
REGDEF_BEGIN(USB_OTGCTRLSTATUS_REG)
REGDEF_BIT(B_BUS_REQ, 1)         // Device bus request (reserved in NT96650)
REGDEF_BIT(B_HNP_EN, 1)          // Inform B-device enabled to perform HNP (reserved in NT96650)
REGDEF_BIT(B_DSCHRG_VBUS, 1)     // B-device discharge Vbus (reserved in NT96650)
REGDEF_BIT(, 1)
REGDEF_BIT(A_BUS_REQ, 1)         // A-device bus request
REGDEF_BIT(A_BUS_DROP, 1)        // A-device bus drop
REGDEF_BIT(A_SET_B_HNP_EN, 1)    // Indicates to A-device that HNP function of B-device had been enabled (reserved in NT96650)
REGDEF_BIT(A_SRP_DET_EN, 1)      // Device SRP detection enable (reserved in NT96650)
REGDEF_BIT(A_SRP_RESP_TYP, 1)    // SRP response type, 0: Vbus pulsing, 1: data-line pulsing (reserved in NT96650)
REGDEF_BIT(ID_FLT_SEL, 1)        // Select a timer to filter out noise of ID from UTMI+
REGDEF_BIT(Vbus_FLT_SEL, 1)      // Select a timer to filter out noise of Vbus_VLD from UTMI+
REGDEF_BIT(HDISCON_FLT_SEL, 1)   // Select a timer to filter out noise of HDISCON from UTMI+
REGDEF_BIT(, 1)
REGDEF_BIT(IDPULLUP, 1)          // ID Pull-up control
REGDEF_BIT(, 2)
REGDEF_BIT(B_SESS_END, 1)        // B-device session end (reserved in NT96650)
REGDEF_BIT(B_SESS_VLD, 1)        // B-device session valid (reserved in NT96650)
REGDEF_BIT(A_SESS_VLD, 1)        // A-device session valid (reserved in NT96650)
REGDEF_BIT(VBUS_VLD, 1)          // A-device Vbus valid
REGDEF_BIT(CROLE, 1)             // Current role
REGDEF_BIT(ID, 1)                // Current ID
REGDEF_BIT(HOST_SPD_TYP, 2)      // Host speed type. 00: FS, 01: LS, 10: HS
REGDEF_BIT(, 8)
REGDEF_END(USB_OTGCTRLSTATUS_REG)

// 0x84, OTG Interrupt status Register
#define USB_OTGINTSTATUS_REG_OFS        0x84
REGDEF_BEGIN(USB_OTGINTSTATUS_REG)
REGDEF_BIT(B_SRP_DN, 1)          // B-device SRP done (reserved in NT96650)
REGDEF_BIT(, 3)
REGDEF_BIT(A_SRP_DET, 1)         // A-device detects SRP from B-device (reserved in NT96650)
REGDEF_BIT(A_VBUS_ERR, 1)        // A-device Vbus error (reserved in NT96650)
REGDEF_BIT(B_SESS_END, 1)        // Vbus below B_SESS_END (reserved in NT96650)
REGDEF_BIT(, 1)
REGDEF_BIT(RLCHG, 1)             // Role change (reserved in NT96650)
REGDEF_BIT(IDCHG, 1)             // ID change
REGDEF_BIT(OVC, 1)               // Over current detection
REGDEF_BIT(A_WAIT_CONN, 1)       // A-device wait for connection, 1 means A-device is waiting for peripheral connection.
REGDEF_BIT(APLGRMV, 1)           // Mini-A plug remove (reserved in NT96650)
REGDEF_BIT(, 19)
REGDEF_END(USB_OTGINTSTATUS_REG)

// 0x88, OTG Interrupt Enable Register
#define USB_OTGINTEN_REG_OFS            0x88
REGDEF_BEGIN(USB_OTGINTEN_REG)
REGDEF_BIT(B_SRP_DN_EN, 1)       // Enable B_SRP_DN interrupt (reserved in NT96650)
REGDEF_BIT(, 3)
REGDEF_BIT(A_SRP_DET_EN, 1)      // Enable A_SRP_DET interrupt (reserved in NT96650)
REGDEF_BIT(A_VBUS_ERR_EN, 1)     // Enable A_VBUS_ERR interrupt (reserved in NT96650)
REGDEF_BIT(B_SESS_END_EN, 1)     // Enable B_SESS_END_EN interrupt (reserved in NT96650)
REGDEF_BIT(, 1)
REGDEF_BIT(RLCHG_EN, 1)          // Enable RLCHG interrupt (reserved in NT96650)
REGDEF_BIT(IDCHG_EN, 1)          // Enable IDCHG interrupt
REGDEF_BIT(OVC_EN, 1)            // Enable OVC interrupt
REGDEF_BIT(A_WAIT_CONN_EN, 1)    // Enable A_WAIT_CONN interrupt
REGDEF_BIT(APLGRMV_EN, 1)        // Enable APLGRMV interrupt (reserved in NT96650)
REGDEF_BIT(, 19)
REGDEF_END(USB_OTGINTEN_REG)

// 0xC0, Global HC/OTG/DEV Interrupt Status Register
#define USB_GLOBALINTSTATUS_REG_OFS     0xC0
REGDEF_BEGIN(USB_GLOBALINTSTATUS_REG)
REGDEF_BIT(DEV_INT, 1)           // Device interrupt
REGDEF_BIT(OTG_INT, 1)           // OTG interrupt
REGDEF_BIT(HC_INT, 1)            // HC interrupt
REGDEF_BIT(, 29)
REGDEF_END(USB_GLOBALINTSTATUS_REG)

// 0xC4, Global Mask of HC/OTG/DEV Interrupt
#define USB_GLOBALINTMASK_REG_OFS       0xC4
REGDEF_BEGIN(USB_GLOBALINTMASK_REG)
REGDEF_BIT(MDEV_INT, 1)          // Mask the interrupt bits of the Device interrupt
REGDEF_BIT(MOTG_INT, 1)          // Mask the interrupt bits of the OTG interrupt
REGDEF_BIT(MHC_INT, 1)           // Mask the interrupt bits of the HC interrupt
REGDEF_BIT(INT_POLARITY, 1)      // Control the polarity of the system interrupt signal sys_int_n
REGDEF_BIT(, 28)
REGDEF_END(USB_GLOBALINTMASK_REG)


//-------------------------------------------------------
//  USB Device registers
//-------------------------------------------------------

// 0x100, Device Main Control Register
#define USB_DEVMAINCTRL_REG_OFS       0x100
REGDEF_BEGIN(USB_DEVMAINCTRL_REG)
REGDEF_BIT(CAP_RMWAKUP, 1)       // Capability of remote wakeup
REGDEF_BIT(HALF_SPEED, 1)        // Half speed enable
REGDEF_BIT(GLINT_EN, 1)          // Global interrupt enable
REGDEF_BIT(GOSUSP, 1)            // Go suspend
REGDEF_BIT(SFRST, 1)             // Device software reset
REGDEF_BIT(CHIP_EN, 1)           // Chip enable
REGDEF_BIT(HS_EN, 1)             // High speed status, 0: full speed, 1: high speed
REGDEF_BIT(SYSBUS_WIDTH, 1)      // System bus width, 0: 8 bits, 1: 32 bits
REGDEF_BIT(, 1)
REGDEF_BIT(FORCE_FS, 1)          // Force the device to full-speed
REGDEF_BIT(IDLE_DEGLITCH, 2)     // Idle line state deglitch
REGDEF_BIT(LPM_BESL_MAX, 4)      // Maximum BESL
REGDEF_BIT(LPM_BESL, 4)
REGDEF_BIT(, 1)
REGDEF_BIT(LPM_EN, 1)            // LPM feature, 0: disable, 1: enable
REGDEF_BIT(LPM_ACCEPT, 1)        // LPM transaction, 0: NYET, 1: ACK
REGDEF_BIT(, 5)
REGDEF_END(USB_DEVMAINCTRL_REG)

// 0x104, Device Address Register
#define USB_DEVADDR_REG_OFS             0x104
REGDEF_BEGIN(USB_DEVADDR_REG)
REGDEF_BIT(DEVADR, 7)            // Device address
REGDEF_BIT(AFT_CONF, 1)          // After set configuration
REGDEF_BIT(, 8)
REGDEF_BIT(RST_DEBOUNCE_INVL, 8) // Interval of device to detect USB RESET from host after DISCONNECT is cleared
REGDEF_BIT(, 8)
REGDEF_END(USB_DEVADDR_REG)

// 0x108, Device Test Register
#define USB_DEVTEST_REG_OFS             0x108
REGDEF_BEGIN(USB_DEVTEST_REG)
REGDEF_BIT(TST_CLRFF, 1)         // Clear FIFO
REGDEF_BIT(TST_LPCX, 1)          // Loop back test for control endpoint
REGDEF_BIT(TST_CLREA, 1)         // Clear external side address
REGDEF_BIT(TST_DISCRC, 1)        // Disable CRC
REGDEF_BIT(TST_DISTOG, 1)        // Disable toggle sequence
REGDEF_BIT(TST_MOD, 1)           // test mode
REGDEF_BIT(DISGENSOF, 1)         // Disable generation of SOF
REGDEF_BIT(TST_MOD_TYPE, 1)      // test mode type 0: normal, 1: mutual
REGDEF_BIT(, 24)
REGDEF_END(USB_DEVTEST_REG)

// 0x10C, Device SOF Frame Number Register
#define USB_DEVSOFNUM_REG_OFS           0x10C
REGDEF_BEGIN(USB_DEVSOFNUM_REG)
REGDEF_BIT(SOFN, 11)             // SOF frame number
REGDEF_BIT(USOFN, 3)             // SOF micro frame number
REGDEF_BIT(, 18)
REGDEF_END(USB_DEVSOFNUM_REG)

// 0x110, Device SOF Mask Timer Register
#define USB_DEVSOFTIMERMASK_REG_OFS     0x110
REGDEF_BEGIN(USB_DEVSOFTIMERMASK_REG)
REGDEF_BIT(SOFTM, 16)            // SOF mask timer. Time since the last SOF in the 30MHz clock bit
REGDEF_BIT(, 16)
REGDEF_END(USB_DEVSOFTIMERMASK_REG)

// 0x114, PHY Test Mode Selector Register
#define USB_PHYTSTSELECT_REG_OFS        0x114
REGDEF_BEGIN(USB_PHYTSTSELECT_REG)
REGDEF_BIT(UNPLUG, 1)            // 1 : soft-detachment
REGDEF_BIT(TST_JSTA, 1)          // 1 : the D+/D- is set to the high speed J state
REGDEF_BIT(TST_KSTA, 1)          // 1 : the D+/D- is set to the high speed K state
REGDEF_BIT(TST_SEN0NAK, 1)       // 1 : the D+/D- lines are set to the HS, quiescent state
REGDEF_BIT(TST_PKT, 1)           // Test mode for packet
REGDEF_BIT(, 27)
REGDEF_END(USB_PHYTSTSELECT_REG)

// 0x120, Device CX Configuration and FIFO Empty Status Register
#define USB_DEVCXCFGFIFOSTATUS_REG_OFS  0x120
REGDEF_BEGIN(USB_DEVCXCFGFIFOSTATUS_REG)
REGDEF_BIT(CX_DONE, 1)           // Data transfer is done for CX
REGDEF_BIT(TST_PKDONE, 1)        // Data transfer is done for test packet
REGDEF_BIT(CX_STL, 1)            // Stall CX
REGDEF_BIT(CX_CLR, 1)            // Clear CX FIFO data
REGDEF_BIT(CX_FUL, 1)            // CX FIFO is full
REGDEF_BIT(CX_EMP, 1)            // CX FIFO is empty
REGDEF_BIT(, 2)
REGDEF_BIT(F_EMP, 8)             // FIFO is empty
REGDEF_BIT(CX_FNT_IN, 7)
REGDEF_BIT(, 1)
REGDEF_BIT(CX_FNT_OUT, 7)        // CX FIFO byte count
REGDEF_BIT(CX_DATAPORT_EN, 1)
REGDEF_END(USB_DEVCXCFGFIFOSTATUS_REG)

// 0x124, Device Idle Counter
#define USB_DEVIDLECNT_REG_OFS          0x124
REGDEF_BEGIN(USB_DEVIDLECNT_REG)
REGDEF_BIT(IDLE_CNT, 3)          // Control the timing delay from GOSUSP to device entered suspend mode
REGDEF_BIT(, 13)
REGDEF_BIT(F_EMP8, 8)
REGDEF_BIT(, 8)
REGDEF_END(USB_DEVIDLECNT_REG)

// 0x128, Device CX Data Port Register
#define USB_DEVCXDATAPORT_REG_OFS       0x128
REGDEF_BEGIN(USB_DEVCXDATAPORT_REG)
REGDEF_BIT(CX_DATAPORT, 32)       // CX Data Port Register
REGDEF_END(USB_DEVCXDATAPORT_REG)

// 0x130, Device Mask of Interrupt Group Register
#define USB_DEVINTGROUPMASK_REG_OFS     0x130
REGDEF_BEGIN(USB_DEVINTGROUPMASK_REG)
REGDEF_BIT(MINT_G0, 1)           // Mask of source group 0 interrupt
REGDEF_BIT(MINT_G1, 1)           // Mask of source group 1 interrupt
REGDEF_BIT(MINT_G2, 1)           // Mask of source group 2 interrupt
REGDEF_BIT(MINT_G3, 1)           // Mask of source group 3 interrupt
REGDEF_BIT(, 28)
REGDEF_END(USB_DEVINTGROUPMASK_REG)

// 0x134, Device Mask of Interrupt Source Group 0 Register
#define USB_DEVINTMASKGROUP0_REG_OFS    0x134
REGDEF_BEGIN(USB_DEVINTMASKGROUP0_REG)
REGDEF_BIT(MCX_SETUP_INT, 1)     // Mask endpoint 0 setup data received interrupt
REGDEF_BIT(MCX_IN_INT, 1)        // Mask the interrupt bits of endpoint 0 for IN
REGDEF_BIT(MCX_OUT_INT, 1)       // Mask the interrupt bits of endpoint 0 for OUT
REGDEF_BIT(MCX_COMEND, 1)        // Indicate the control transfer has entered status stage
REGDEF_BIT(MCX_COMFAIL_INT, 1)   // Mask interrupt of host emits extra IN or OUT data interrupt
REGDEF_BIT(MCX_COMABORT_INT, 1)  // Mask interrupt of control transfer command abort
REGDEF_BIT(, 26)
REGDEF_END(USB_DEVINTMASKGROUP0_REG)

// 0x138, Device Mask of Interrupt Source Group 1 Register
#define USB_DEVINTMASKGROUP1_REG_OFS    0x138
REGDEF_BEGIN(USB_DEVINTMASKGROUP1_REG)
REGDEF_BIT(MF0_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 0
REGDEF_BIT(MF0_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 0
REGDEF_BIT(MF1_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 1
REGDEF_BIT(MF1_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 1
REGDEF_BIT(MF2_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 2
REGDEF_BIT(MF2_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 2
REGDEF_BIT(MF3_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 3
REGDEF_BIT(MF3_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 3
REGDEF_BIT(MF4_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 4
REGDEF_BIT(MF4_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 4
REGDEF_BIT(MF5_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 5
REGDEF_BIT(MF5_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 5
REGDEF_BIT(MF6_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 6
REGDEF_BIT(MF6_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 6
REGDEF_BIT(MF7_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 7
REGDEF_BIT(MF7_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 7
REGDEF_BIT(MF0_IN_INT, 1)        // Mask the IN interrupt of FIFO 0
REGDEF_BIT(MF1_IN_INT, 1)        // Mask the IN interrupt of FIFO 1
REGDEF_BIT(MF2_IN_INT, 1)        // Mask the IN interrupt of FIFO 2
REGDEF_BIT(MF3_IN_INT, 1)        // Mask the IN interrupt of FIFO 3
REGDEF_BIT(MF4_IN_INT, 1)        // Mask the IN interrupt of FIFO 4
REGDEF_BIT(MF5_IN_INT, 1)        // Mask the IN interrupt of FIFO 5
REGDEF_BIT(MF6_IN_INT, 1)        // Mask the IN interrupt of FIFO 6
REGDEF_BIT(MF7_IN_INT, 1)        // Mask the IN interrupt of FIFO 7
REGDEF_BIT(MF8_IN_INT, 1)        // Mask the IN interrupt of FIFO 8
REGDEF_BIT(MF9_IN_INT, 1)        // Mask the IN interrupt of FIFO 9
REGDEF_BIT(MF10_IN_INT, 1)        // Mask the IN interrupt of FIFO 10
REGDEF_BIT(MF11_IN_INT, 1)        // Mask the IN interrupt of FIFO 11
REGDEF_BIT(MF12_IN_INT, 1)        // Mask the IN interrupt of FIFO 12
REGDEF_BIT(MF13_IN_INT, 1)        // Mask the IN interrupt of FIFO 13
REGDEF_BIT(MF14_IN_INT, 1)        // Mask the IN interrupt of FIFO 14
REGDEF_BIT(MF15_IN_INT, 1)        // Mask the IN interrupt of FIFO 15
REGDEF_END(USB_DEVINTMASKGROUP1_REG)

// 0x13C, Device Mask of Interrupt Source Group 2 Register
#define USB_DEVINTMASKGROUP2_REG_OFS    0x13C
REGDEF_BEGIN(USB_DEVINTMASKGROUP2_REG)
REGDEF_BIT(MUSBRST_INT, 1)       // Mask USB reset interrupt
REGDEF_BIT(MSUSP_INT, 1)         // Mask suspend interrupt
REGDEF_BIT(MRESM_INT, 1)         // Mask resume interrupt
REGDEF_BIT(MSEQ_ERR_INT, 1)      // Mask ISO sequential error interrupt
REGDEF_BIT(MSEQ_ABORT_INT, 1)    // Mask ISO sequential abort interrupt
REGDEF_BIT(MTX0BYTE_INT, 1)      // Mask transferred zero-length data packet interrupt
REGDEF_BIT(MRX0BYTE_INT, 1)      // Mask received zero-length data packet interrupt
REGDEF_BIT(MDMA_CMPLT, 1)        // Mask DMA completion interrupt
REGDEF_BIT(MDMA_ERROR, 1)        // Mask DMA error interrupt
REGDEF_BIT(MDEV_IDLE, 1)         // Mask Dev_Idle interrupt
REGDEF_BIT(MDEV_WAKEUP_BYVBUS, 1) // Mask Dev_Wakeup_byVBUS interrupt
REGDEF_BIT(, 5)
REGDEF_BIT(MF8_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 8
REGDEF_BIT(MF8_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 8
REGDEF_BIT(MF9_OUT_INT, 1)       // Mask the OUT interrupt of FIFO 9
REGDEF_BIT(MF9_SPK_INT, 1)       // Mask the short packet interrupt of FIFO 9
REGDEF_BIT(MF10_OUT_INT, 1)      // Mask the OUT interrupt of FIFO 10
REGDEF_BIT(MF10_SPK_INT, 1)      // Mask the short packet interrupt of FIFO 10
REGDEF_BIT(MF11_OUT_INT, 1)      // Mask the OUT interrupt of FIFO 11
REGDEF_BIT(MF11_SPK_INT, 1)      // Mask the short packet interrupt of FIFO 11
REGDEF_BIT(MF12_OUT_INT, 1)      // Mask the OUT interrupt of FIFO 12
REGDEF_BIT(MF12_SPK_INT, 1)      // Mask the short packet interrupt of FIFO 12
REGDEF_BIT(MF13_OUT_INT, 1)      // Mask the OUT interrupt of FIFO 13
REGDEF_BIT(MF13_SPK_INT, 1)      // Mask the short packet interrupt of FIFO 13
REGDEF_BIT(MF14_OUT_INT, 1)      // Mask the OUT interrupt of FIFO 14
REGDEF_BIT(MF14_SPK_INT, 1)      // Mask the short packet interrupt of FIFO 14
REGDEF_BIT(MF15_OUT_INT, 1)      // Mask the OUT interrupt of FIFO 15
REGDEF_BIT(MF15_SPK_INT, 1)      // Mask the short packet interrupt of FIFO 15
REGDEF_END(USB_DEVINTMASKGROUP2_REG)

// 0x140, Device Interrupt Group Register
#define USB_DEVINTGROUP_REG_OFS         0x140
REGDEF_BEGIN(USB_DEVINTGROUP_REG)
REGDEF_BIT(INT_G0, 1)            // Indicate some interrupts occurred in Group 0
REGDEF_BIT(INT_G1, 1)            // Indicate some interrupts occurred in Group 1
REGDEF_BIT(INT_G2, 1)            // Indicate some interrupts occurred in Group 2
REGDEF_BIT(INT_G3, 1)            // Indicate some interrupts occurred in Group 3
REGDEF_BIT(, 28)
REGDEF_END(USB_DEVINTGROUP_REG)

// 0x144, Device Interrupt Source Group 0 Register
#define USB_DEVINTGROUP0_REG_OFS        0x144
REGDEF_BEGIN(USB_DEVINTGROUP0_REG)
REGDEF_BIT(CX_SETUP_INT, 1)      // will remain asserted until firmware read data from CX FIFO
REGDEF_BIT(CX_IN_INT, 1)         // Indicate that firmware should write data for control-read transfer to CX FIFO
REGDEF_BIT(CX_OUT_INT, 1)        // Indicate the control transfer contains valid data for control-write transfers
REGDEF_BIT(CX_COMEND, 1)         // Indicate the control transfer has entered status stage
REGDEF_BIT(CX_COMFAIL_INT, 1)    // Indicate the control transfer is abnormally terminated.
REGDEF_BIT(CX_COMABORT_INT, 1)   // Indicate a command abort event occurred
REGDEF_BIT(, 26)
REGDEF_END(USB_DEVINTGROUP0_REG)

// 0x148, Device Interrupt Source Group 1 Register
#define USB_DEVINTGROUP1_REG_OFS        0x148
REGDEF_BEGIN(USB_DEVINTGROUP1_REG)
REGDEF_BIT(F0_OUT_INT, 1)        // FIFO 0 is ready to be read
REGDEF_BIT(F0_SPK_INT, 1)        // short packet data is received in FIFO 0
REGDEF_BIT(F1_OUT_INT, 1)        // FIFO 1 is ready to be read
REGDEF_BIT(F1_SPK_INT, 1)        // short packet data is received in FIFO 1
REGDEF_BIT(F2_OUT_INT, 1)        // FIFO 2 is ready to be read
REGDEF_BIT(F2_SPK_INT, 1)        // short packet data is received in FIFO 2
REGDEF_BIT(F3_OUT_INT, 1)        // FIFO 3 is ready to be read
REGDEF_BIT(F3_SPK_INT, 1)        // short packet data is received in FIFO 3
REGDEF_BIT(F4_OUT_INT, 1)        // FIFO 4 is ready to be read
REGDEF_BIT(F4_SPK_INT, 1)        // short packet data is received in FIFO 4
REGDEF_BIT(F5_OUT_INT, 1)        // FIFO 5 is ready to be read
REGDEF_BIT(F5_SPK_INT, 1)        // short packet data is received in FIFO 5
REGDEF_BIT(F6_OUT_INT, 1)        // FIFO 6 is ready to be read
REGDEF_BIT(F6_SPK_INT, 1)        // short packet data is received in FIFO 6
REGDEF_BIT(F7_OUT_INT, 1)        // FIFO 7 is ready to be read
REGDEF_BIT(F7_SPK_INT, 1)        // short packet data is received in FIFO 7
REGDEF_BIT(F0_IN_INT, 1)         // FIFO 0 is ready to be written
REGDEF_BIT(F1_IN_INT, 1)         // FIFO 1 is ready to be written
REGDEF_BIT(F2_IN_INT, 1)         // FIFO 2 is ready to be written
REGDEF_BIT(F3_IN_INT, 1)         // FIFO 3 is ready to be written
REGDEF_BIT(F4_IN_INT, 1)         // FIFO 4 is ready to be written
REGDEF_BIT(F5_IN_INT, 1)         // FIFO 5 is ready to be written
REGDEF_BIT(F6_IN_INT, 1)         // FIFO 6 is ready to be written
REGDEF_BIT(F7_IN_INT, 1)         // FIFO 7 is ready to be written
REGDEF_BIT(F8_IN_INT, 1)         // FIFO 8 is ready to be written
REGDEF_BIT(F9_IN_INT, 1)         // FIFO 9 is ready to be written
REGDEF_BIT(F10_IN_INT, 1)        // FIFO 10 is ready to be written
REGDEF_BIT(F11_IN_INT, 1)        // FIFO 11 is ready to be written
REGDEF_BIT(F12_IN_INT, 1)        // FIFO 12 is ready to be written
REGDEF_BIT(F13_IN_INT, 1)        // FIFO 13 is ready to be written
REGDEF_BIT(F14_IN_INT, 1)        // FIFO 14 is ready to be written
REGDEF_BIT(F15_IN_INT, 1)        // FIFO 15 is ready to be written)
REGDEF_END(USB_DEVINTGROUP1_REG)

// 0x14C, Device Interrupt Source Group 2 Register
#define USB_DEVINTGROUP2_REG_OFS        0x14C
REGDEF_BEGIN(USB_DEVINTGROUP2_REG)
REGDEF_BIT(USBRST_INT, 1)        // USB reset interrupt
REGDEF_BIT(SUSP_INT, 1)          // suspend interrupt
REGDEF_BIT(RESM_INT, 1)          // resume interrupt
REGDEF_BIT(SEQ_ERR_INT, 1)       // ISO sequential error interrupt
REGDEF_BIT(SEQ_ABORT_INT, 1)     // ISO sequential abort interrupt
REGDEF_BIT(TX0BYTE_INT, 1)       // transferred zero-length data packet interrupt
REGDEF_BIT(RX0BYTE_INT, 1)       // received zero-length data packet interrupt
REGDEF_BIT(DMA_CMPLT, 1)         // DMA completion interrupt
REGDEF_BIT(DMA_ERROR, 1)         // DMA error interrupt
REGDEF_BIT(DEV_IDLE, 1)          // Dev is in idle state
REGDEF_BIT(DEV_WAKEUP_BYVBUS, 1) // Dev Wakeup byVBUS
REGDEF_BIT(, 5)
REGDEF_BIT(F8_OUT_INT, 1)        // FIFO 8 is ready to be read
REGDEF_BIT(F8_SPK_INT, 1)        // short packet data is received in FIFO 8
REGDEF_BIT(F9_OUT_INT, 1)        // FIFO 9 is ready to be read
REGDEF_BIT(F9_SPK_INT, 1)        // short packet data is received in FIFO 9
REGDEF_BIT(F10_OUT_INT, 1)       // FIFO 10 is ready to be read
REGDEF_BIT(F10_SPK_INT, 1)       // short packet data is received in FIFO 10
REGDEF_BIT(F11_OUT_INT, 1)       // FIFO 11 is ready to be read
REGDEF_BIT(F11_SPK_INT, 1)       // short packet data is received in FIFO 11
REGDEF_BIT(F12_OUT_INT, 1)        // FIFO 12 is ready to be read
REGDEF_BIT(F12_SPK_INT, 1)        // short packet data is received in FIFO 12
REGDEF_BIT(F13_OUT_INT, 1)        // FIFO 13 is ready to be read
REGDEF_BIT(F13_SPK_INT, 1)        // short packet data is received in FIFO 13
REGDEF_BIT(F14_OUT_INT, 1)       // FIFO 14 is ready to be read
REGDEF_BIT(F14_SPK_INT, 1)       // short packet data is received in FIFO 14
REGDEF_BIT(F15_OUT_INT, 1)       // FIFO 15 is ready to be read
REGDEF_BIT(F15_SPK_INT, 1)       // short packet data is received in FIFO 15
REGDEF_END(USB_DEVINTGROUP2_REG)

// 0x150, Device Receive Zero-Length Data Packet Register
#define USB_DEVRX0BYTE_REG_OFS          0x150
REGDEF_BEGIN(USB_DEVRX0BYTE_REG)
REGDEF_BIT(RX0BYTE_EP1, 1)       // Endpoint 1 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP2, 1)       // Endpoint 2 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP3, 1)       // Endpoint 3 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP4, 1)       // Endpoint 4 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP5, 1)       // Endpoint 5 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP6, 1)       // Endpoint 6 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP7, 1)       // Endpoint 7 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP8, 1)       // Endpoint 8 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP9, 1)       // Endpoint 9 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP10, 1)      // Endpoint 10 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP11, 1)      // Endpoint 11 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP12, 1)      // Endpoint 12 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP13, 1)      // Endpoint 13 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP14, 1)      // Endpoint 14 receives a zero-length data packet
REGDEF_BIT(RX0BYTE_EP15, 1)      // Endpoint 15 receives a zero-length data packet
REGDEF_BIT(, 17)
REGDEF_END(USB_DEVRX0BYTE_REG)

// 0x154, Device Transfer Zero-Length Data Packet Register
#define USB_DEVTX0BYTE_REG_OFS          0x154
REGDEF_BEGIN(USB_DEVTX0BYTE_REG)
REGDEF_BIT(TX0BYTE_EP1, 1)       // Endpoint 1 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP2, 1)       // Endpoint 2 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP3, 1)       // Endpoint 3 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP4, 1)       // Endpoint 4 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP5, 1)       // Endpoint 5 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP6, 1)       // Endpoint 6 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP7, 1)       // Endpoint 7 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP8, 1)       // Endpoint 8 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP9, 1)       // Endpoint 9 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP10, 1)      // Endpoint 10 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP11, 1)      // Endpoint 11 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP12, 1)      // Endpoint 12 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP13, 1)      // Endpoint 13 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP14, 1)      // Endpoint 14 transfers a zero-length data packet
REGDEF_BIT(TX0BYTE_EP15, 1)      // Endpoint 15 transfers a zero-length data packet
REGDEF_BIT(, 17)
REGDEF_END(USB_DEVTX0BYTE_REG)

// 0x158, Device Isochronous Sequential Error/Abort Register
#define USB_DEVISOERRABT_REG_OFS        0x158
REGDEF_BEGIN(USB_DEVISOERRABT_REG)
REGDEF_BIT(ISO_ABT_ERR_EP1, 1)   // Endpoint 1 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP2, 1)   // Endpoint 2 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP3, 1)   // Endpoint 3 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP4, 1)   // Endpoint 4 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP5, 1)   // Endpoint 5 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP6, 1)   // Endpoint 6 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP7, 1)   // Endpoint 7 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP8, 1)   // Endpoint 8 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP9, 1)   // Endpoint 9 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP10, 1)   // Endpoint 10 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP11, 1)   // Endpoint 11 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP12, 1)   // Endpoint 12 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP13, 1)   // Endpoint 12 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP14, 1)   // Endpoint 12 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP15, 1)   // Endpoint 12 encounters an isochronous sequential abort
REGDEF_BIT(, 1)
REGDEF_BIT(ISO_SEQ_ERR_EP1, 1)   // Endpoint 1 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP2, 1)   // Endpoint 2 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP3, 1)   // Endpoint 3 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP4, 1)   // Endpoint 4 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP5, 1)   // Endpoint 5 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP6, 1)   // Endpoint 6 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP7, 1)   // Endpoint 7 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP8, 1)   // Endpoint 8 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP9, 1)   // Endpoint 9 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP10, 1)  // Endpoint 10 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP11, 1)  // Endpoint 11 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP12, 1)  // Endpoint 12 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP13, 1)  // Endpoint 13 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP14, 1)  // Endpoint 14 encounters an isochronous sequential error
REGDEF_BIT(ISO_SEQ_ERR_EP15, 1)  // Endpoint 15 encounters an isochronous sequential error
REGDEF_BIT(, 1)
REGDEF_END(USB_DEVISOERRABT_REG)

// 0x160~0x17C, Device IN Endpoint 1~8 MaxPacketSize Register
#define USB_DEVMAXPS_INEP1_REG_OFS      0x160
#define USB_DEVMAXPS_INEP2_REG_OFS      0x164
#define USB_DEVMAXPS_INEP3_REG_OFS      0x168
#define USB_DEVMAXPS_INEP4_REG_OFS      0x16C
#define USB_DEVMAXPS_INEP5_REG_OFS      0x170
#define USB_DEVMAXPS_INEP6_REG_OFS      0x174
#define USB_DEVMAXPS_INEP7_REG_OFS      0x178
#define USB_DEVMAXPS_INEP8_REG_OFS      0x17C
#define USB_DEVMAXPS_INEP9_REG_OFS      0x260
#define USB_DEVMAXPS_INEP10_REG_OFS     0x264
#define USB_DEVMAXPS_INEP11_REG_OFS     0x268
#define USB_DEVMAXPS_INEP12_REG_OFS     0x26C
#define USB_DEVMAXPS_INEP13_REG_OFS     0x270
#define USB_DEVMAXPS_INEP14_REG_OFS     0x274
#define USB_DEVMAXPS_INEP15_REG_OFS     0x278

REGDEF_BEGIN(USB_DEVMAXPS_INEP_REG)
REGDEF_BIT(MAXPS_IEP, 11)        // Max packet size of IN endpoint
REGDEF_BIT(STL_IEP, 1)           // Stall IN endpoint
REGDEF_BIT(RSTG_IEP, 1)          // Reset toggle sequence for IN endpoint
REGDEF_BIT(TX_NUM_HBW_IEP, 2)    // Transaction number for high bandwidth endpoint
REGDEF_BIT(TX0BYTE_IEP, 1)       // Transfer a zero-length data packet from endpoint to host
REGDEF_BIT(, 16)
REGDEF_END(USB_DEVMAXPS_INEP_REG)

// 0x180~0x19C, Device OUT Endpoint 1~8 MaxPacketSize Register
#define USB_DEVMAXPS_OUTEP1_REG_OFS     0x180
#define USB_DEVMAXPS_OUTEP2_REG_OFS     0x184
#define USB_DEVMAXPS_OUTEP3_REG_OFS     0x188
#define USB_DEVMAXPS_OUTEP4_REG_OFS     0x18C
#define USB_DEVMAXPS_OUTEP5_REG_OFS     0x190
#define USB_DEVMAXPS_OUTEP6_REG_OFS     0x194
#define USB_DEVMAXPS_OUTEP7_REG_OFS     0x198
#define USB_DEVMAXPS_OUTEP8_REG_OFS     0x19C
#define USB_DEVMAXPS_OUTEP9_REG_OFS     0x280
#define USB_DEVMAXPS_OUTEP10_REG_OFS    0x284
#define USB_DEVMAXPS_OUTEP11_REG_OFS    0x288
#define USB_DEVMAXPS_OUTEP12_REG_OFS    0x28C
#define USB_DEVMAXPS_OUTEP13_REG_OFS    0x290
#define USB_DEVMAXPS_OUTEP14_REG_OFS    0x294
#define USB_DEVMAXPS_OUTEP15_REG_OFS    0x298

REGDEF_BEGIN(USB_DEVMAXPS_OUTEP_REG)
REGDEF_BIT(MAXPS_OEP, 11)        // Max packet size of OUT endpoint
REGDEF_BIT(STL_OEP, 1)           // Stall OUT endpoint
REGDEF_BIT(RSTG_OEP, 1)          // Reset toggle sequence for OUT endpoint
REGDEF_BIT(, 19)
REGDEF_END(USB_DEVMAXPS_OUTEP_REG)

// 0x1A0, Device Endpoint 1~4 Map Register
#define USB_DEVEPMAP1_4_REG_OFS         0x1A0
REGDEF_BEGIN(USB_DEVEPMAP1_4_REG)
REGDEF_BIT(FNO_IEP1, 4)          // FIFO number for IN endpoint 1
REGDEF_BIT(FNO_OEP1, 4)          // FIFO number for OUT endpoint 1
REGDEF_BIT(FNO_IEP2, 4)          // FIFO number for IN endpoint 2
REGDEF_BIT(FNO_OEP2, 4)          // FIFO number for OUT endpoint 2
REGDEF_BIT(FNO_IEP3, 4)          // FIFO number for IN endpoint 3
REGDEF_BIT(FNO_OEP3, 4)          // FIFO number for OUT endpoint 3
REGDEF_BIT(FNO_IEP4, 4)          // FIFO number for IN endpoint 4
REGDEF_BIT(FNO_OEP4, 4)          // FIFO number for OUT endpoint 4
REGDEF_END(USB_DEVEPMAP1_4_REG)

// 0x1A4, Device Endpoint 5~8 Map Register
#define USB_DEVEPMAP5_8_REG_OFS         0x1A4
REGDEF_BEGIN(USB_DEVEPMAP5_8_REG)
REGDEF_BIT(FNO_IEP5, 4)          // FIFO number for IN endpoint 5
REGDEF_BIT(FNO_OEP5, 4)          // FIFO number for OUT endpoint 5
REGDEF_BIT(FNO_IEP6, 4)          // FIFO number for IN endpoint 6
REGDEF_BIT(FNO_OEP6, 4)          // FIFO number for OUT endpoint 6
REGDEF_BIT(FNO_IEP7, 4)          // FIFO number for IN endpoint 7
REGDEF_BIT(FNO_OEP7, 4)          // FIFO number for OUT endpoint 7
REGDEF_BIT(FNO_IEP8, 4)          // FIFO number for IN endpoint 8
REGDEF_BIT(FNO_OEP8, 4)          // FIFO number for OUT endpoint 8
REGDEF_END(USB_DEVEPMAP5_8_REG)

// 0x1A0, Device Endpoint 9~11 Map Register
#define USB_DEVEPMAP9_12_REG_OFS         0x2A0
REGDEF_BEGIN(USB_DEVEPMAP9_12_REG)
REGDEF_BIT(FNO_IEP9, 4)          // FIFO number for IN endpoint 9
REGDEF_BIT(FNO_OEP9, 4)          // FIFO number for OUT endpoint 9
REGDEF_BIT(FNO_IEP10, 4)         // FIFO number for IN endpoint 10
REGDEF_BIT(FNO_OEP10, 4)         // FIFO number for OUT endpoint 10
REGDEF_BIT(FNO_IEP11, 4)         // FIFO number for IN endpoint 11
REGDEF_BIT(FNO_OEP11, 4)         // FIFO number for OUT endpoint 11
REGDEF_BIT(FNO_IEP12, 4)         // FIFO number for IN endpoint 12
REGDEF_BIT(FNO_OEP12, 4)         // FIFO number for OUT endpoint 12
REGDEF_END(USB_DEVEPMAP9_12_REG)

// 0x1A0, Device Endpoint 9~11 Map Register
#define USB_DEVEPMAP13_15_REG_OFS         0x2A4
REGDEF_BEGIN(USB_DEVEPMAP13_15_REG)
REGDEF_BIT(FNO_IEP13, 4)          // FIFO number for IN endpoint 13
REGDEF_BIT(FNO_OEP13, 4)          // FIFO number for OUT endpoint 13
REGDEF_BIT(FNO_IEP14, 4)         // FIFO number for IN endpoint 14
REGDEF_BIT(FNO_OEP14, 4)         // FIFO number for OUT endpoint 14
REGDEF_BIT(FNO_IEP15, 4)         // FIFO number for IN endpoint 15
REGDEF_BIT(FNO_OEP15, 4)         // FIFO number for OUT endpoint 15
REGDEF_BIT(, 8)
REGDEF_END(USB_DEVEPMAP13_15_REG)


// 0x1A8, Device FIFO Map Register
#define USB_DEVFIFOMAP_REG_OFS          0x1A8
REGDEF_BEGIN(USB_DEVFIFOMAP_REG)
REGDEF_BIT(EPNO_FIFO0, 4)        // Endpoint number for FIFO 0
REGDEF_BIT(Dir_FIFO0, 2)         // FIFO 0 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO1, 4)        // Endpoint number for FIFO 1
REGDEF_BIT(Dir_FIFO1, 2)         // FIFO 1 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO2, 4)        // Endpoint number for FIFO 2
REGDEF_BIT(Dir_FIFO2, 2)         // FIFO 2 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO3, 4)        // Endpoint number for FIFO 3
REGDEF_BIT(Dir_FIFO3, 2)         // FIFO 3 direction
REGDEF_BIT(, 2)
REGDEF_END(USB_DEVFIFOMAP_REG)

// 0x1AC, Device FIFO Configuration
#define USB_DEVFIFOCFG_REG_OFS          0x1AC
REGDEF_BEGIN(USB_DEVFIFOCFG_REG)
REGDEF_BIT(BLK_TYP_F0, 2)        // Transfer type of FIFO 0
REGDEF_BIT(BLKNO_F0, 2)          // Block number of FIFO 0
REGDEF_BIT(BLKSZ_F0, 1)          // Block size of FIFO 0
REGDEF_BIT(EN_F0, 1)             // Enable FIFO 0
REGDEF_BIT(PINGPONG0, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F1, 2)        // Transfer type of FIFO 1
REGDEF_BIT(BLKNO_F1, 2)          // Block number of FIFO 1
REGDEF_BIT(BLKSZ_F1, 1)          // Block size of FIFO 1
REGDEF_BIT(EN_F1, 1)             // Enable FIFO 1
REGDEF_BIT(PINGPONG1, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F2, 2)        // Transfer type of FIFO 2
REGDEF_BIT(BLKNO_F2, 2)          // Block number of FIFO 2
REGDEF_BIT(BLKSZ_F2, 1)          // Block size of FIFO 2
REGDEF_BIT(EN_F2, 1)             // Enable FIFO 2
REGDEF_BIT(PINGPONG2, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F3, 2)        // Transfer type of FIFO 3
REGDEF_BIT(BLKNO_F3, 2)          // Block number of FIFO 3
REGDEF_BIT(BLKSZ_F3, 1)          // Block size of FIFO 3
REGDEF_BIT(EN_F3, 1)             // Enable FIFO 3
REGDEF_BIT(PINGPONG3, 1)
REGDEF_BIT(, 1)
REGDEF_END(USB_DEVFIFOCFG_REG)

// 0x1B0~0x1BC, Device FIFO 0~3 Instruction and Byte Count Register
#define USB_DEVFIFO0BYTECNT_REG_OFS     0x1B0
#define USB_DEVFIFO1BYTECNT_REG_OFS     0x1B4
#define USB_DEVFIFO2BYTECNT_REG_OFS     0x1B8
#define USB_DEVFIFO3BYTECNT_REG_OFS     0x1BC
#define USB_DEVFIFO4BYTECNT_REG_OFS     0x1F0
#define USB_DEVFIFO5BYTECNT_REG_OFS     0x1F4
#define USB_DEVFIFO6BYTECNT_REG_OFS     0x1F8
#define USB_DEVFIFO7BYTECNT_REG_OFS     0x1FC
#define USB_DEVFIFO8BYTECNT_REG_OFS     0x200
#define USB_DEVFIFO9BYTECNT_REG_OFS     0x204
#define USB_DEVFIFO10BYTECNT_REG_OFS    0x208
#define USB_DEVFIFO11BYTECNT_REG_OFS    0x20C
#define USB_DEVFIFO12BYTECNT_REG_OFS    0x210
#define USB_DEVFIFO13BYTECNT_REG_OFS    0x214
#define USB_DEVFIFO14BYTECNT_REG_OFS    0x218
#define USB_DEVFIFO15BYTECNT_REG_OFS    0x21C

REGDEF_BEGIN(USB_DEVFIFOBYTECNT_REG)
REGDEF_BIT(BC_F, 11)             // Out FIFO byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST, 1)             // FIFO reset
REGDEF_BIT(, 19)
REGDEF_END(USB_DEVFIFOBYTECNT_REG)

// 0x1C0, Device DMA Target FIFO Number Register
#define USB_DEVACCFIFO_REG_OFS          0x1C0
REGDEF_BEGIN(USB_DEVACCFIFO_REG)
REGDEF_BIT(ACC_F0_3, 17)          // Accessing FIFO 0-3
REGDEF_BIT(, 15)
REGDEF_END(USB_DEVACCFIFO_REG)

// 0x1C8, Device DMA Controller Parameter Setting 1 Register
#define USB_DEVDMACTRL1_REG_OFS         0x1C8
REGDEF_BEGIN(USB_DEVDMACTRL1_REG)
REGDEF_BIT(DMA_START, 1)         // DMA start
REGDEF_BIT(DMA_TYPE, 1)          // DMA type, 0: FIFO to memory, 1: memory to FIFO
REGDEF_BIT(DMA_IO, 1)            // DMA I/O to I/O
REGDEF_BIT(DMA_ABORT, 1)         // DMA abort
REGDEF_BIT(CLRFIFO_DMA_ABORT, 1) // Clear FIFO when DMA abort
REGDEF_BIT(REMOTE_WAKEUP, 1)     // Remote wake up trigger signal
REGDEF_BIT(, 2)
REGDEF_BIT(DMA_LEN, 23)          // DMA length Max 8MB-1
REGDEF_BIT(DEVPHY_SUSPEND, 1)    // Device transceiver suspend mode
REGDEF_END(USB_DEVDMACTRL1_REG)

// 0x1CC, Device DMA Controller Parameter Setting 2 Register
#define USB_DEVDMACTRL2_REG_OFS         0x1CC
REGDEF_BEGIN(USB_DEVDMACTRL2_REG)
REGDEF_BIT(DMA_MADDR, 32)        // DMA memory address
REGDEF_END(USB_DEVDMACTRL2_REG)

// 0x1D0, Device DMA Controller Parameter Setting 3 Register
#define USB_DEVDMACTRL3_REG_OFS         0x1D0
REGDEF_BEGIN(USB_DEVDMACTRL3_REG)
REGDEF_BIT(SETUP_CMD_RPORT, 32)  // SETUP_CMD_RPORT
REGDEF_END(USB_DEVDMACTRL3_REG)

// 0x1D4, DMA Controller Status Register
#define USB_DEVDMACTRLSTATUS_REG_OFS    0x1D4
REGDEF_BEGIN(USB_DEVDMACTRLSTATUS_REG)
REGDEF_BIT(DMA_REMLEN, 23)       // Remaining length when DMA_ABORT
REGDEF_BIT(, 9)
REGDEF_END(USB_DEVDMACTRLSTATUS_REG)


// 0x1D8, Device FIFO Map Register
#define USB_DEVFIFOMAP1_REG_OFS          0x1D8
REGDEF_BEGIN(USB_DEVFIFOMAP1_REG)
REGDEF_BIT(EPNO_FIFO4, 4)        // Endpoint number for FIFO 4
REGDEF_BIT(Dir_FIFO4, 2)         // FIFO 4 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO5, 4)        // Endpoint number for FIFO 5
REGDEF_BIT(Dir_FIFO5, 2)         // FIFO 5 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO6, 4)        // Endpoint number for FIFO 6
REGDEF_BIT(Dir_FIFO6, 2)         // FIFO 6 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO7, 4)        // Endpoint number for FIFO 7
REGDEF_BIT(Dir_FIFO7, 2)         // FIFO 7 direction
REGDEF_BIT(, 2)
REGDEF_END(USB_DEVFIFOMAP1_REG)

// 0x1DC, Device FIFO Configuration
#define USB_DEVFIFOCFG1_REG_OFS          0x1DC
REGDEF_BEGIN(USB_DEVFIFOCFG1_REG)
REGDEF_BIT(BLK_TYP_F4, 2)        // Transfer type of FIFO 4
REGDEF_BIT(BLKNO_F4, 2)          // Block number of FIFO 4
REGDEF_BIT(BLKSZ_F4, 1)          // Block size of FIFO 4
REGDEF_BIT(EN_F4, 1)             // Enable FIFO 4
REGDEF_BIT(PINGPONG4, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F5, 2)        // Transfer type of FIFO 5
REGDEF_BIT(BLKNO_F5, 2)          // Block number of FIFO 5
REGDEF_BIT(BLKSZ_F5, 1)          // Block size of FIFO 5
REGDEF_BIT(EN_F5, 1)             // Enable FIFO 5
REGDEF_BIT(PINGPONG5, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F6, 2)        // Transfer type of FIFO 6
REGDEF_BIT(BLKNO_F6, 2)          // Block number of FIFO 6
REGDEF_BIT(BLKSZ_F6, 1)          // Block size of FIFO 6
REGDEF_BIT(EN_F6, 1)             // Enable FIFO 6
REGDEF_BIT(PINGPONG6, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F7, 2)        // Transfer type of FIFO 7
REGDEF_BIT(BLKNO_F7, 2)          // Block number of FIFO 7
REGDEF_BIT(BLKSZ_F7, 1)          // Block size of FIFO 7
REGDEF_BIT(EN_F7, 1)             // Enable FIFO 7
REGDEF_BIT(PINGPONG7, 1)
REGDEF_BIT(, 1)
REGDEF_END(USB_DEVFIFOCFG1_REG)


// 0x1E0, Device FIFO Map Register
#define USB_DEVFIFOMAP2_REG_OFS          0x1E0
REGDEF_BEGIN(USB_DEVFIFOMAP2_REG)
REGDEF_BIT(EPNO_FIFO8, 4)        // Endpoint number for FIFO 8
REGDEF_BIT(Dir_FIFO8, 2)         // FIFO 8 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO9, 4)        // Endpoint number for FIFO 9
REGDEF_BIT(Dir_FIFO9, 2)         // FIFO 9 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO10, 4)       // Endpoint number for FIFO 10
REGDEF_BIT(Dir_FIFO10, 2)        // FIFO 10 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO11, 4)       // Endpoint number for FIFO 11
REGDEF_BIT(Dir_FIFO11, 2)        // FIFO 11 direction
REGDEF_BIT(, 2)
REGDEF_END(USB_DEVFIFOMAP2_REG)

// 0x1E8, Device FIFO Map Register
#define USB_DEVFIFOMAP3_REG_OFS          0x1E8
REGDEF_BEGIN(USB_DEVFIFOMAP3_REG)
REGDEF_BIT(EPNO_FIFO12, 4)        // Endpoint number for FIFO 12
REGDEF_BIT(Dir_FIFO12, 2)         // FIFO 12 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO13, 4)        // Endpoint number for FIFO 13
REGDEF_BIT(Dir_FIFO13, 2)         // FIFO 13 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO14, 4)       // Endpoint number for FIFO 14
REGDEF_BIT(Dir_FIFO14, 2)        // FIFO 14 direction
REGDEF_BIT(, 2)
REGDEF_BIT(EPNO_FIFO15, 4)       // Endpoint number for FIFO 15
REGDEF_BIT(Dir_FIFO15, 2)        // FIFO 15 direction
REGDEF_BIT(, 2)
REGDEF_END(USB_DEVFIFOMAP3_REG)


// 0x1E4, Device FIFO Configuration
#define USB_DEVFIFOCFG2_REG_OFS          0x1E4
REGDEF_BEGIN(USB_DEVFIFOCFG2_REG)
REGDEF_BIT(BLK_TYP_F8, 2)        // Transfer type of FIFO 8
REGDEF_BIT(BLKNO_F8, 2)          // Block number of FIFO 8
REGDEF_BIT(BLKSZ_F8, 1)          // Block size of FIFO 8
REGDEF_BIT(EN_F8, 1)             // Enable FIFO 8
REGDEF_BIT(PINGPONG8, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F9, 2)        // Transfer type of FIFO 9
REGDEF_BIT(BLKNO_F9, 2)          // Block number of FIFO 9
REGDEF_BIT(BLKSZ_F9, 1)          // Block size of FIFO 9
REGDEF_BIT(EN_F9, 1)             // Enable FIFO 9
REGDEF_BIT(PINGPONG9, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F10, 2)       // Transfer type of FIFO 10
REGDEF_BIT(BLKNO_F10, 2)         // Block number of FIFO 10
REGDEF_BIT(BLKSZ_F10, 1)         // Block size of FIFO 10
REGDEF_BIT(EN_F10, 1)            // Enable FIFO 10
REGDEF_BIT(PINGPONG10, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F11, 2)       // Transfer type of FIFO 11
REGDEF_BIT(BLKNO_F11, 2)         // Block number of FIFO 11
REGDEF_BIT(BLKSZ_F11, 1)         // Block size of FIFO 11
REGDEF_BIT(EN_F11, 1)            // Enable FIFO 11
REGDEF_BIT(PINGPONG11, 1)
REGDEF_BIT(, 1)
REGDEF_END(USB_DEVFIFOCFG2_REG)

// 0x1EC, Device FIFO Configuration
#define USB_DEVFIFOCFG3_REG_OFS          0x1EC
REGDEF_BEGIN(USB_DEVFIFOCFG3_REG)
REGDEF_BIT(BLK_TYP_F12, 2)        // Transfer type of FIFO 12
REGDEF_BIT(BLKNO_F12, 2)          // Block number of FIFO 12
REGDEF_BIT(BLKSZ_F12, 1)          // Block size of FIFO 12
REGDEF_BIT(EN_F12, 1)             // Enable FIFO 12
REGDEF_BIT(PINGPONG12, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F13, 2)        // Transfer type of FIFO 13
REGDEF_BIT(BLKNO_F13, 2)          // Block number of FIFO 13
REGDEF_BIT(BLKSZ_F13, 1)          // Block size of FIFO 13
REGDEF_BIT(EN_F13, 1)             // Enable FIFO 13
REGDEF_BIT(PINGPONG13, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F14, 2)       // Transfer type of FIFO 14
REGDEF_BIT(BLKNO_F14, 2)         // Block number of FIFO 14
REGDEF_BIT(BLKSZ_F14, 1)         // Block size of FIFO 14
REGDEF_BIT(EN_F14, 1)            // Enable FIFO 14
REGDEF_BIT(PINGPONG14, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(BLK_TYP_F15, 2)       // Transfer type of FIFO 15
REGDEF_BIT(BLKNO_F15, 2)         // Block number of FIFO 15
REGDEF_BIT(BLKSZ_F15, 1)         // Block size of FIFO 15
REGDEF_BIT(EN_F15, 1)            // Enable FIFO 15
REGDEF_BIT(PINGPONG15, 1)
REGDEF_BIT(, 1)
REGDEF_END(USB_DEVFIFOCFG3_REG)


// 0x400, DMA Controller Status Register
#define USB_PHYTOP_REG_OFS    0x400
REGDEF_BEGIN(USB_PHYTOP_REG)
REGDEF_BIT(PHY_WR_CYCLE_CNT, 4)
REGDEF_BIT(PHY_RD_CYCLE_CNT, 4)
REGDEF_BIT(PHY_RES, 5)
REGDEF_BIT(, 2)
REGDEF_BIT(PHY_RINT_EN, 1)
REGDEF_BIT(PHY_POR, 1)
REGDEF_BIT(PHY_POR_RESET, 1)
REGDEF_BIT(PHY_SIDDQ, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(USB_VBUSI, 1)
REGDEF_BIT(USB_ID, 1)
REGDEF_BIT(, 10)
REGDEF_END(USB_PHYTOP_REG)

// 0x300 Device Virtual DMA CXF Parameter Setting 1 Register
// 0x308 Device Virtual DMA FIFO0 Parameter Setting 1 Register
// 0x310 Device Virtual DMA FIFO1 Parameter Setting 1 Register
// 0x318 Device Virtual DMA FIFO2 Parameter Setting 1 Register
// 0x320 Device Virtual DMA FIFO3 Parameter Setting 1 Register
#define USB_DEVVIRTUAL_DMACXF1_REG_OFS        0x300
#define USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS  0x308
#define USB_DEVVIRTUAL_DMAFIFO1_PAR1_REG_OFS  0x310
#define USB_DEVVIRTUAL_DMAFIFO2_PAR1_REG_OFS  0x318
#define USB_DEVVIRTUAL_DMAFIFO3_PAR1_REG_OFS  0x320
#define USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS  0x340
#define USB_DEVVIRTUAL_DMAFIFO5_PAR1_REG_OFS  0x348
#define USB_DEVVIRTUAL_DMAFIFO6_PAR1_REG_OFS  0x350
#define USB_DEVVIRTUAL_DMAFIFO7_PAR1_REG_OFS  0x358
#define USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS  0x360
#define USB_DEVVIRTUAL_DMAFIFO9_PAR1_REG_OFS  0x368
#define USB_DEVVIRTUAL_DMAFIFO10_PAR1_REG_OFS 0x370
#define USB_DEVVIRTUAL_DMAFIFO11_PAR1_REG_OFS 0x378
#define USB_DEVVIRTUAL_DMAFIFO12_PAR1_REG_OFS 0x380
#define USB_DEVVIRTUAL_DMAFIFO13_PAR1_REG_OFS 0x388
#define USB_DEVVIRTUAL_DMAFIFO14_PAR1_REG_OFS 0x390
#define USB_DEVVIRTUAL_DMAFIFO15_PAR1_REG_OFS 0x398

REGDEF_BEGIN(USB_DEVVIRTUAL_DMACXF1_REG)
REGDEF_BIT(VDMA_START_CXF, 1)     // Virtual DMA Start for CXF
REGDEF_BIT(VDMA_TYPE_CXF, 1)      // Virtual DMA Type for CXF
REGDEF_BIT(VDMA_IO_CXF, 1)        // Virtual DMA IO to IO for CXF
REGDEF_BIT(VDMA_ABORT_FIFO, 1)
REGDEF_BIT(VDMA_ABORT_CLRFIFO, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(VDMA_LEN_CXF, 23)       // Virtual DMA Length for CXF
REGDEF_BIT(, 1)
REGDEF_END(USB_DEVVIRTUAL_DMACXF1_REG)

// 0x304 Device Virtual DMA CXF Parameter Setting 2 Register
// 0x30C Device Virtual DMA FIFO0 Parameter Setting 2 Register
// 0x314 Device Virtual DMA FIFO1 Parameter Setting 2 Register
// 0x31C Device Virtual DMA FIFO2 Parameter Setting 2 Register
// 0x324 Device Virtual DMA FIFO3 Parameter Setting 2 Register
#define USB_DEVVIRTUAL_DMACXF2_REG_OFS        0x304
#define USB_DEVVIRTUAL_DMAFIFO0_PAR2_REG_OFS  0x30C
#define USB_DEVVIRTUAL_DMAFIFO1_PAR2_REG_OFS  0x314
#define USB_DEVVIRTUAL_DMAFIFO2_PAR2_REG_OFS  0x31C
#define USB_DEVVIRTUAL_DMAFIFO3_PAR2_REG_OFS  0x324
#define USB_DEVVIRTUAL_DMAFIFO4_PAR2_REG_OFS  0x344
#define USB_DEVVIRTUAL_DMAFIFO5_PAR2_REG_OFS  0x34C
#define USB_DEVVIRTUAL_DMAFIFO6_PAR2_REG_OFS  0x354
#define USB_DEVVIRTUAL_DMAFIFO7_PAR2_REG_OFS  0x35C
#define USB_DEVVIRTUAL_DMAFIFO8_PAR2_REG_OFS  0x364
#define USB_DEVVIRTUAL_DMAFIFO9_PAR2_REG_OFS  0x36C
#define USB_DEVVIRTUAL_DMAFIFO10_PAR2_REG_OFS 0x374
#define USB_DEVVIRTUAL_DMAFIFO11_PAR2_REG_OFS 0x37C
#define USB_DEVVIRTUAL_DMAFIFO12_PAR2_REG_OFS 0x384
#define USB_DEVVIRTUAL_DMAFIFO13_PAR2_REG_OFS 0x38C
#define USB_DEVVIRTUAL_DMAFIFO14_PAR2_REG_OFS 0x394
#define USB_DEVVIRTUAL_DMAFIFO15_PAR2_REG_OFS 0x39C

REGDEF_BEGIN(USB_DEVVIRTUAL_DMACXF2_REG)
REGDEF_BIT(VDMA_MADDR_CXF, 32)    // Virtual DMA Address for CXF
REGDEF_END(USB_DEVVIRTUAL_DMACXF2_REG)


// 0x328, Device Interrupt Source Group 3 Register
#define USB_DEVINTGROUP3_REG_OFS        0x328
REGDEF_BEGIN(USB_DEVINTGROUP3_REG)
REGDEF_BIT(VDMA_CMPLT_CXF, 1)      // VDMA completion interrupt for CXF
REGDEF_BIT(VDMA_CMPLT_F0, 1)       // VDMA completion interrupt for FIFO0
REGDEF_BIT(VDMA_CMPLT_F1, 1)       // VDMA completion interrupt for FIFO1
REGDEF_BIT(VDMA_CMPLT_F2, 1)       // VDMA completion interrupt for FIFO2
REGDEF_BIT(VDMA_CMPLT_F3, 1)       // VDMA completion interrupt for FIFO3
REGDEF_BIT(VDMA_CMPLT_F4, 1)       // VDMA completion interrupt for FIFO4
REGDEF_BIT(VDMA_CMPLT_F5, 1)       // VDMA completion interrupt for FIFO5
REGDEF_BIT(VDMA_CMPLT_F6, 1)       // VDMA completion interrupt for FIFO6
REGDEF_BIT(VDMA_CMPLT_F7, 1)       // VDMA completion interrupt for FIFO7
REGDEF_BIT(VDMA_CMPLT_F8, 1)       // VDMA completion interrupt for FIFO8
REGDEF_BIT(VDMA_CMPLT_F9, 1)       // VDMA completion interrupt for FIFO9
REGDEF_BIT(VDMA_CMPLT_F10, 1)      // VDMA completion interrupt for FIFO10
REGDEF_BIT(VDMA_CMPLT_F11, 1)      // VDMA completion interrupt for FIFO11
REGDEF_BIT(VDMA_CMPLT_F12, 1)      // VDMA completion interrupt for FIFO12
REGDEF_BIT(VDMA_CMPLT_F13, 1)      // VDMA completion interrupt for FIFO13
REGDEF_BIT(VDMA_CMPLT_F14, 1)      // VDMA completion interrupt for FIFO14
REGDEF_BIT(VDMA_ERROR_CXF, 1)      // VDMA error interrupt for CXF
REGDEF_BIT(VDMA_ERROR_F0, 1)       // VDMA error interrupt for FIFO0
REGDEF_BIT(VDMA_ERROR_F1, 1)       // VDMA error interrupt for FIFO1
REGDEF_BIT(VDMA_ERROR_F2, 1)       // VDMA error interrupt for FIFO2
REGDEF_BIT(VDMA_ERROR_F3, 1)       // VDMA error interrupt for FIFO3
REGDEF_BIT(VDMA_ERROR_F4, 1)       // VDMA error interrupt for FIFO4
REGDEF_BIT(VDMA_ERROR_F5, 1)       // VDMA error interrupt for FIFO5
REGDEF_BIT(VDMA_ERROR_F6, 1)       // VDMA error interrupt for FIFO6
REGDEF_BIT(VDMA_ERROR_F7, 1)       // VDMA error interrupt for FIFO7
REGDEF_BIT(VDMA_ERROR_F8, 1)       // VDMA error interrupt for FIFO8
REGDEF_BIT(VDMA_ERROR_F9, 1)       // VDMA error interrupt for FIFO9
REGDEF_BIT(VDMA_ERROR_F10, 1)      // VDMA error interrupt for FIFO10
REGDEF_BIT(VDMA_ERROR_F11, 1)      // VDMA error interrupt for FIFO11
REGDEF_BIT(VDMA_ERROR_F12, 1)      // VDMA error interrupt for FIFO12
REGDEF_BIT(VDMA_ERROR_F13, 1)      // VDMA error interrupt for FIFO13
REGDEF_BIT(VDMA_ERROR_F14, 1)      // VDMA error interrupt for FIFO14
REGDEF_END(USB_DEVINTGROUP3_REG)

// 0x32C, Device Mask of Interrupt Source Group 3 Register
#define USB_DEVINTMASKGROUP3_REG_OFS    0x32C
REGDEF_BEGIN(USB_DEVINTMASKGROUP3_REG)
REGDEF_BIT(MVDMA_CMPLT_CXF, 1)      // VDMA completion interrupt for CXF
REGDEF_BIT(MVDMA_CMPLT_F0, 1)       // VDMA completion interrupt for FIFO0
REGDEF_BIT(MVDMA_CMPLT_F1, 1)       // VDMA completion interrupt for FIFO1
REGDEF_BIT(MVDMA_CMPLT_F2, 1)       // VDMA completion interrupt for FIFO2
REGDEF_BIT(MVDMA_CMPLT_F3, 1)       // VDMA completion interrupt for FIFO3
REGDEF_BIT(MVDMA_CMPLT_F4, 1)       // VDMA completion interrupt for FIFO4
REGDEF_BIT(MVDMA_CMPLT_F5, 1)       // VDMA completion interrupt for FIFO5
REGDEF_BIT(MVDMA_CMPLT_F6, 1)       // VDMA completion interrupt for FIFO6
REGDEF_BIT(MVDMA_CMPLT_F7, 1)       // VDMA completion interrupt for FIFO7
REGDEF_BIT(MVDMA_CMPLT_F8, 1)       // VDMA completion interrupt for FIFO8
REGDEF_BIT(MVDMA_CMPLT_F9, 1)       // VDMA completion interrupt for FIFO9
REGDEF_BIT(MVDMA_CMPLT_F10, 1)      // VDMA completion interrupt for FIFO10
REGDEF_BIT(MVDMA_CMPLT_F11, 1)      // VDMA completion interrupt for FIFO11
REGDEF_BIT(MVDMA_CMPLT_F12, 1)      // VDMA completion interrupt for FIFO11
REGDEF_BIT(MVDMA_CMPLT_F13, 1)      // VDMA completion interrupt for FIFO11
REGDEF_BIT(MVDMA_CMPLT_F14, 1)      // VDMA completion interrupt for FIFO11
REGDEF_BIT(MVDMA_ERROR_CXF, 1)      // VDMA error interrupt for CXF
REGDEF_BIT(MVDMA_ERROR_F0, 1)       // VDMA error interrupt for FIFO0
REGDEF_BIT(MVDMA_ERROR_F1, 1)       // VDMA error interrupt for FIFO1
REGDEF_BIT(MVDMA_ERROR_F2, 1)       // VDMA error interrupt for FIFO2
REGDEF_BIT(MVDMA_ERROR_F3, 1)       // VDMA error interrupt for FIFO3
REGDEF_BIT(MVDMA_ERROR_F4, 1)       // VDMA error interrupt for FIFO4
REGDEF_BIT(MVDMA_ERROR_F5, 1)       // VDMA error interrupt for FIFO5
REGDEF_BIT(MVDMA_ERROR_F6, 1)       // VDMA error interrupt for FIFO6
REGDEF_BIT(MVDMA_ERROR_F7, 1)       // VDMA error interrupt for FIFO7
REGDEF_BIT(MVDMA_ERROR_F8, 1)       // VDMA error interrupt for FIFO8
REGDEF_BIT(MVDMA_ERROR_F9, 1)       // VDMA error interrupt for FIFO9
REGDEF_BIT(MVDMA_ERROR_F10, 1)      // VDMA error interrupt for FIFO10
REGDEF_BIT(MVDMA_ERROR_F11, 1)      // VDMA error interrupt for FIFO11
REGDEF_BIT(MVDMA_ERROR_F12, 1)      // VDMA error interrupt for FIFO11
REGDEF_BIT(MVDMA_ERROR_F13, 1)      // VDMA error interrupt for FIFO11
REGDEF_BIT(MVDMA_ERROR_F14, 1)      // VDMA error interrupt for FIFO11
REGDEF_END(USB_DEVINTMASKGROUP3_REG)

#define USB_DEVINTGROUP3EXT_REG_OFS    0x3A0
REGDEF_BEGIN(USB_DEVINTGROUP3EXT_REG)
REGDEF_BIT(VDMA_CMPLT_F15,  1)       // VDMA completion interrupt for FIFO0
REGDEF_BIT(VDMA_ERROR_F15,  1)       // VDMA error interrupt for FIFO0
REGDEF_BIT(,  14)
REGDEF_BIT(MVDMA_CMPLT_F15, 1)       // VDMA completion interrupt for FIFO0
REGDEF_BIT(MVDMA_ERROR_F15, 1)       // VDMA error interrupt for FIFO0
REGDEF_BIT(,  14)
REGDEF_END(USB_DEVINTGROUP3EXT_REG)



// 0x330, Device Virtual DMA Control Register
#define USB_DEVVDMA_CTRL_REG_OFS       0x330
REGDEF_BEGIN(USB_DEVVDMA_CTRL_REG)
REGDEF_BIT(VDMA_EN, 1)              // VDMA Channel Enable
REGDEF_BIT(, 31)
REGDEF_END(USB_DEVVDMA_CTRL_REG)

// 0x334, Device LPM Capability Register
#define USB_DEVLPM_CAPS_REG_OFS        0x334
REGDEF_BEGIN(USB_DEVLPM_CAPS_REG)
REGDEF_BIT(LPM_WAKEUP_EN, 1)        // Enable LPM Wakeup capability
REGDEF_BIT(, 31)
REGDEF_END(USB_DEVLPM_CAPS_REG)

// 0x338, Device Interrupt Source Group 4 Register
#define USB_DEVINTGROUP4_REG_OFS       0x338
REGDEF_BEGIN(USB_DEVINTGROUP4_REG)
REGDEF_BIT(L1_INT, 1)               // L1 Interrupt. L1-state-change interrupt
REGDEF_BIT(, 31)
REGDEF_END(USB_DEVINTGROUP4_REG)

// 0x33C, Device Mask of Interrupt Source Group 4 Register
#define USB_DEVINTMASKGROUP4_REG_OFS   0x33C
REGDEF_BEGIN(USB_DEVINTMASKGROUP4_REG)
REGDEF_BIT(ML1_INT, 1)               // Mask L1 Interrupt. Mask active L1 state Change Interrupt bit
REGDEF_BIT(, 31)
REGDEF_END(USB_DEVINTMASKGROUP4_REG)

//
//  USB Charger Related
//

#define USB_CHARGER_CONTROL_REG_OFS     0x3C0
REGDEF_BEGIN(USB_CHARGER_CONTROL_REG)
REGDEF_BIT(DCD_DETECT_EN, 1)
REGDEF_BIT(CHG_DETECT_EN, 1)
REGDEF_BIT(SECOND_CHG_DETECT_EN, 1)
REGDEF_BIT(reserved, 29)
REGDEF_END(USB_CHARGER_CONTROL_REG)

#define USB_DCD_COUNTER_REG_OFS         0x3C4
REGDEF_BEGIN(USB_DCD_COUNTER_REG)
REGDEF_BIT(DCD_COUNT, 22)
REGDEF_BIT(, 10)
REGDEF_END(USB_DCD_COUNTER_REG)

#define USB_BUS_STATUS_REG_OFS          0x3C8
REGDEF_BEGIN(USB_BUS_STATUS_REG)
REGDEF_BIT(reserved1, 6)
REGDEF_BIT(CHG_DET, 1)
REGDEF_BIT(reserved, 25)
REGDEF_END(USB_BUS_STATUS_REG)


//
//  USB PHY Related
//

// 0x1000, USB PHY Registers Base
#define USB_PHY_BASE_OFS                0x1000

// PHY ADDR 0x05
#define USB_PHYCTRL05_REG_OFS           0x1014
REGDEF_BEGIN(USB_PHYCTRL05_REG)
REGDEF_BIT(DISCON_SEL, 2)
REGDEF_BIT(SQ_SEL, 3)
REGDEF_BIT(VSQ_MODE_SEL, 2)
REGDEF_BIT(CHIRP_MAX_SQ_EN, 1)
REGDEF_BIT(reserved1, 24)
REGDEF_END(USB_PHYCTRL05_REG)

// PHY ADDR 0x06
#define USB_PHYCTRL06_REG_OFS           0x1018
REGDEF_BEGIN(USB_PHYCTRL06_REG)
REGDEF_BIT(CHIRP_MAX_SQ_INV, 1)
REGDEF_BIT(SWCTRL, 3)
REGDEF_BIT(CHG_VTUNE, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(reserved2, 24)
REGDEF_END(USB_PHYCTRL06_REG)

// PHY ADDR 0x08
#define USB_PHYCTRL08_REG_OFS          0x1020
REGDEF_BEGIN(USB_PHYCTRL08_REG)
REGDEF_BIT(FSLS_TX_SLEW_RATE_SEL, 2)
REGDEF_BIT(FSLS_TX_CURRENT_SEL, 2)
REGDEF_BIT(LS_FILT_LEN, 2)
REGDEF_BIT(, 1)
REGDEF_BIT(LS_FILT_EN, 1)
REGDEF_BIT(reserved2, 24)
REGDEF_END(USB_PHYCTRL08_REG)

// PHY ADDR 0x0B
#define USB_PHYCTRL0B_REG_OFS          0x102C
REGDEF_BEGIN(USB_PHYCTRL0B_REG)
REGDEF_BIT(DATARPDM, 1)
REGDEF_BIT(DATARPDP, 1)
REGDEF_BIT(DATARPUSEL, 1)
REGDEF_BIT(SPSEL, 1)
REGDEF_BIT(DATARPU2, 1)
REGDEF_BIT(DATARPU1, 1)
REGDEF_BIT(reserved2, 26)
REGDEF_END(USB_PHYCTRL0B_REG)

// PHY ADDR 0x0C
#define USB_PHYCTRL0C_REG_OFS          0x1030
REGDEF_BEGIN(USB_PHYCTRL0C_REG)
REGDEF_BIT(DATARPDM, 1)
REGDEF_BIT(DATARPDP, 1)
REGDEF_BIT(DATARPUSEL, 1)
REGDEF_BIT(SPSEL, 1)
REGDEF_BIT(DATARPU2, 1)
REGDEF_BIT(DATARPU1, 1)
REGDEF_BIT(DMPD20K, 1)
REGDEF_BIT(DPPD20K, 1)
REGDEF_BIT(reserved2, 24)
REGDEF_END(USB_PHYCTRL0C_REG)

// PHY ADDR 0x0E
#define USB_PHYCTRL0E_REG_OFS          0x1038
REGDEF_BEGIN(USB_PHYCTRL0E_REG)
REGDEF_BIT(TX_P_SAVE, 1)
REGDEF_BIT(ZRX_EN, 1)
REGDEF_BIT(EN_TX_P_SAVE, 1)
REGDEF_BIT(EN_ZRX_EN, 1)
REGDEF_BIT(reserved1, 28)
REGDEF_END(USB_PHYCTRL0E_REG)

// PHY ADDR 0x37
#define USB_PHYCTRL37_REG_OFS           0x10DC
REGDEF_BEGIN(USB_PHYCTRL37_REG)
REGDEF_BIT(DBG_DO_SEL, 4)
REGDEF_BIT(IN_NAK_EN, 1)
REGDEF_BIT(NEGATIVE_EN, 1)
REGDEF_BIT(HSTXEN, 1)
REGDEF_BIT(reserved1, 25)
REGDEF_END(USB_PHYCTRL37_REG)

// PHY ADDR 0x3A
#define USB_PHYCTRL3A_REG_OFS           0x10E8
REGDEF_BEGIN(USB_PHYCTRL3A_REG)
REGDEF_BIT(IP50USEL, 1)
REGDEF_BIT(IREFSEL, 2)
REGDEF_BIT(IP20USEL, 2)
REGDEF_BIT(IN40USEL, 2)
REGDEF_BIT(reserved1, 25)
REGDEF_END(USB_PHYCTRL3A_REG)

// PHY ADDR 0x3C
#define USB_PHYCTRL3C_REG_OFS      0x10F0
REGDEF_BEGIN(USB_PHYCTRL3C_REG)
REGDEF_BIT(RCMP_OUT_SW, 1)
REGDEF_BIT(RCMP_OUT_MODE, 1)
REGDEF_BIT(RCMP_EN_SW, 1)
REGDEF_BIT(RCMP_EN_MODE, 1)
REGDEF_BIT(RSEL_SHIFT, 4)
REGDEF_BIT(reserved1, 24)
REGDEF_END(USB_PHYCTRL3C_REG)

// PHY ADDR 0x50
#define USB_PHYCTRL50_REG_OFS      0x1140
REGDEF_BEGIN(USB_PHYCTRL50_REG)
REGDEF_BIT(RINTCMP_EN, 1)
REGDEF_BIT(RINT_CMP_EN_MODE, 1)
REGDEF_BIT(RINTCAL_TRIG, 1)
REGDEF_BIT(RINTCAL_DONE, 1)
REGDEF_BIT(RINTRES_INVERT, 1)
REGDEF_BIT(RINTRES_MODE, 1)
REGDEF_BIT(RINTCAL_ERR, 1)
REGDEF_BIT(reserved1, 25)
REGDEF_END(USB_PHYCTRL50_REG)

// PHY ADDR 0x52
#define USB_PHYCTRL52_REG_OFS      0x1148
REGDEF_BEGIN(USB_PHYCTRL52_REG)
REGDEF_BIT(RES, 5)
REGDEF_BIT(RINT_EN, 1)
REGDEF_BIT(SW_RINT, 1)
REGDEF_BIT(reserved1, 25)
REGDEF_END(USB_PHYCTRL52_REG)

#endif // __USB_REG_H__


