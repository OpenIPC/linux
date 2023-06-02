/*
 * Faraday FOTG200 USB OTG controller
 *
 * Copyright (C) 2013 Faraday Technology Corporation
 * Author: Yuan-Hsin Chen <yhchen@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NVT680_H
#define _NVT680_H

#include <linux/kernel.h>

#define NVT680_MAX_NUM_EP		5 /* ep0...ep8 */



struct nvt680_request {
	struct usb_request	req;
	struct list_head	queue;
};

struct nvt680_ep {
	struct usb_ep		ep;
	struct nvt680_udc	*nvt680;

	struct list_head	queue;
	unsigned			stall:1;
	unsigned			wedged:1;
	unsigned			use_dma:1;

	unsigned char		epnum;
	unsigned char		type;
	unsigned char		dir_in;
	unsigned int		maxp;

	dma_addr_t 			d_adr;
	unsigned int		d_len;

	const struct usb_endpoint_descriptor	*desc;
};

struct nvt680_udc {
	spinlock_t					lock; /* protect the struct */
	void __iomem				*reg;

	int							irq_no;

	struct usb_gadget			gadget;
	struct usb_gadget_driver	*driver;

	struct nvt680_ep			*ep[NVT680_MAX_NUM_EP];

	struct usb_request			*ep0_req;	/* for internal request */
	__le16						ep0_data;
	u8							ep0_dir;	/* 0/0x80  out/in */

	u8							ep_2_fifo[NVT680_MAX_NUM_EP]; 	/* ep to fifo mapping */
	u8							fifo_2_ep[8];					/* fifo to ep mapping */
	u8							fifo_vld_msk;
	u8							fifo_vld_idx;
	u32							ep_en;
};

#define gadget_to_nvt680(g)	container_of((g), struct nvt680_udc, gadget)


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


////////////////////////////////////////////////////////////////////////////////////


typedef unsigned int        UBITFIELD;  ///< Unsigned bit field
#define REGVALUE            UINT32

// Register Cache Word type defintion header
#define REGDEF_BEGIN(name)      \
	typedef union                   \
	{                               \
		REGVALUE    reg;            \
		struct                      \
		{

// Register Cache Word bit defintion
#define REGDEF_BIT(field, bits) \
	UBITFIELD   field : bits;

// Register Cache Word type defintion trailer
#define REGDEF_END(name)        \
	} bit;                      \
	} T_##name;                     \
	STATIC_ASSERT(sizeof(T_##name) == sizeof(REGVALUE));


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
REGDEF_BIT(RESM_SE0_CNT, 6)
REGDEF_BIT(, 18)
REGDEF_END(USB_HCMISC_REG)

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
REGDEF_BIT(, 4)
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
REGDEF_BIT(BPLGRMV, 1)           // Mini-B plug remove (reserved in NT96650)
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
REGDEF_BIT(BPLGRMV_EN, 1)        // Enable BPLGRMV interrupt (reserved in NT96650)
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
REGDEF_BIT(, 1)
REGDEF_BIT(DMA_RST, 1)           // (Hidden bit : DMA reset)
REGDEF_BIT(FORCE_FS, 1)          // Force the device to full-speed
REGDEF_BIT(, 1)
REGDEF_BIT(FORCE_DMA_ABORT, 1)   // (Hidden bit : force DMA abort for UCLK)
REGDEF_BIT(, 4)
REGDEF_BIT(RESET_DEBOUNCE_INTERVAL, 8)
REGDEF_BIT(, 8)
REGDEF_END(USB_DEVMAINCTRL_REG)

// 0x104, Device Address Register
#define USB_DEVADDR_REG_OFS             0x104
REGDEF_BEGIN(USB_DEVADDR_REG)
REGDEF_BIT(DEVADR, 7)            // Device address
REGDEF_BIT(AFT_CONF, 1)          // After set configuration
REGDEF_BIT(, 24)
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
REGDEF_BIT(, 25)
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
REGDEF_BIT(, 29)
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
REGDEF_BIT(, 29)
REGDEF_END(USB_DEVINTGROUPMASK_REG)

// 0x134, Device Mask of Interrupt Source Group 0 Register
#define USB_DEVINTMASKGROUP0_REG_OFS    0x134
REGDEF_BEGIN(USB_DEVINTMASKGROUP0_REG)
REGDEF_BIT(MCX_SETUP_INT, 1)     // Mask endpoint 0 setup data received interrupt
REGDEF_BIT(MCX_IN_INT, 1)        // Mask the interrupt bits of endpoint 0 for IN
REGDEF_BIT(MCX_OUT_INT, 1)       // Mask the interrupt bits of endpoint 0 for OUT
REGDEF_BIT(MCX_COMEND, 1)        // Mask the host end of the command (entering status stage) interrupt
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
REGDEF_BIT(, 8)
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
REGDEF_BIT(MDMA2_CMPLT, 1)       // Mask DMA completion interrupt
REGDEF_BIT(MDMA3_CMPLT, 1)       // Mask DMA completion interrupt
REGDEF_BIT(MDMA4_CMPLT, 1)       // Mask DMA completion interrupt
REGDEF_BIT(MDMA5_CMPLT, 1)       // Mask DMA completion interrupt
REGDEF_BIT(MDEV_IDLE, 1)         // Mask Dev_Idle interrupt
REGDEF_BIT(MDEV_WAKEUP_BYVBUS, 1) // Mask Dev_Wakeup_byVBUS interrupt
REGDEF_BIT(, 17)
REGDEF_END(USB_DEVINTMASKGROUP2_REG)

// 0x140, Device Interrupt Group Register
#define USB_DEVINTGROUP_REG_OFS         0x140
REGDEF_BEGIN(USB_DEVINTGROUP_REG)
REGDEF_BIT(INT_G0, 1)            // Indicate some interrupts occurred in Group 0
REGDEF_BIT(INT_G1, 1)            // Indicate some interrupts occurred in Group 1
REGDEF_BIT(INT_G2, 1)            // Indicate some interrupts occurred in Group 2
REGDEF_BIT(, 29)
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
REGDEF_BIT(, 8)
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
REGDEF_BIT(DMA2_CMPLT, 1)        // DMA2 completion interrupt
REGDEF_BIT(DMA3_CMPLT, 1)        // DMA3 completion interrupt
REGDEF_BIT(DMA4_CMPLT, 1)        // DMA4 completion interrupt
REGDEF_BIT(DMA5_CMPLT, 1)        // DMA5 completion interrupt
REGDEF_BIT(DEV_IDLE, 1)          // Dev is in idle state
REGDEF_BIT(DEV_WAKEUP_BYVBUS, 1) // Dev Wakeup byVBUS
REGDEF_BIT(, 17)
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
REGDEF_BIT(ISO_ABT_ERR_EP10, 1)  // Endpoint 10 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP11, 1)  // Endpoint 11 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP12, 1)  // Endpoint 12 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP13, 1)  // Endpoint 13 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP14, 1)  // Endpoint 14 encounters an isochronous sequential abort
REGDEF_BIT(ISO_ABT_ERR_EP15, 1)  // Endpoint 15 encounters an isochronous sequential abort
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

// 0x160~0x198, Device Endpoint 1~15 Max-Packet-Size Register
#define USB_DEVMAXPS_EP1_REG_OFS        0x160
#define USB_DEVMAXPS_EP2_REG_OFS        0x164
#define USB_DEVMAXPS_EP3_REG_OFS        0x168
#define USB_DEVMAXPS_EP4_REG_OFS        0x16C
#define USB_DEVMAXPS_EP5_REG_OFS        0x170
#define USB_DEVMAXPS_EP6_REG_OFS        0x174
#define USB_DEVMAXPS_EP7_REG_OFS        0x178
#define USB_DEVMAXPS_EP8_REG_OFS        0x17C
#define USB_DEVMAXPS_EP9_REG_OFS        0x180
#define USB_DEVMAXPS_EP10_REG_OFS       0x184
#define USB_DEVMAXPS_EP11_REG_OFS       0x188
#define USB_DEVMAXPS_EP12_REG_OFS       0x18C
#define USB_DEVMAXPS_EP13_REG_OFS       0x190
#define USB_DEVMAXPS_EP14_REG_OFS       0x194
#define USB_DEVMAXPS_EP15_REG_OFS       0x198

REGDEF_BEGIN(USB_DEVMAXPS_EP_REG)
REGDEF_BIT(MAXPS_EP, 11)         // Max packet size of endpoint
REGDEF_BIT(STL_EP, 1)            // Stall endpoint
REGDEF_BIT(RSTG_EP, 1)           // Reset toggle sequence for endpoint
REGDEF_BIT(TX_NUM_HBW_EP, 2)     // Transaction number for high bandwidth endpoint
REGDEF_BIT(TX0BYTE_IEP, 1)       // Transfer a zero-length data packet from ISO IN endpoint to host
REGDEF_BIT(DIR_EP, 1)
REGDEF_BIT(, 7)
REGDEF_BIT(FNO_IEP, 4)
REGDEF_BIT(FNO_OEP, 4)
REGDEF_END(USB_DEVMAXPS_EP_REG)

// 0x1A8, Device FIFO Config Register 0
#define USB_DEVBUFCFG0_REG_OFS         0x1A8
REGDEF_BEGIN(USB_DEVBUFCFG0_REG)
REGDEF_BIT(BUF0_TYP, 2)          // Transfer type of FIFO 0
REGDEF_BIT(BUF0_BLKNO, 2)        // Block number of FIFO 0
REGDEF_BIT(BUF0_BLKSZ, 1)        // Block size of FIFO 0
REGDEF_BIT(BUF0_DIR, 2)          // FIFO 0 direction
REGDEF_BIT(BUF0_EN, 1)           // Enable FIFO 0
REGDEF_BIT(BUF1_TYP, 2)          // Transfer type of FIFO 1
REGDEF_BIT(BUF1_BLKNO, 2)        // Block number of FIFO 1
REGDEF_BIT(BUF1_BLKSZ, 1)        // Block size of FIFO 1
REGDEF_BIT(BUF1_DIR, 2)          // FIFO 1 direction
REGDEF_BIT(BUF1_EN, 1)           // Enable FIFO 1
REGDEF_BIT(BUF2_TYP, 2)          // Transfer type of FIFO 2
REGDEF_BIT(BUF2_BLKNO, 2)        // Block number of FIFO 2
REGDEF_BIT(BUF2_BLKSZ, 1)        // Block size of FIFO 2
REGDEF_BIT(BUF2_DIR, 2)          // FIFO 2 direction
REGDEF_BIT(BUF2_EN, 1)           // Enable FIFO 2
REGDEF_BIT(BUF3_TYP, 2)          // Transfer type of FIFO 3
REGDEF_BIT(BUF3_BLKNO, 2)        // Block number of FIFO 3
REGDEF_BIT(BUF3_BLKSZ, 1)        // Block size of FIFO 3
REGDEF_BIT(BUF3_DIR, 2)          // FIFO 3 direction
REGDEF_BIT(BUF3_EN, 1)           // Enable FIFO 3
REGDEF_END(USB_DEVBUFCFG0_REG)

// 0x1AC, Device FIFO Config Register 1
#define USB_DEVBUFCFG1_REG_OFS         0x1AC
REGDEF_BEGIN(USB_DEVBUFCFG1_REG)
REGDEF_BIT(BUF4_TYP, 2)          // Transfer type of FIFO 4
REGDEF_BIT(BUF4_BLKNO, 2)        // Block number of FIFO 4
REGDEF_BIT(BUF4_BLKSZ, 1)        // Block size of FIFO 4
REGDEF_BIT(BUF4_DIR, 2)          // FIFO 4 direction
REGDEF_BIT(BUF4_EN, 1)           // Enable FIFO 4
REGDEF_BIT(BUF5_TYP, 2)          // Transfer type of FIFO 5
REGDEF_BIT(BUF5_BLKNO, 2)        // Block number of FIFO 5
REGDEF_BIT(BUF5_BLKSZ, 1)        // Block size of FIFO 5
REGDEF_BIT(BUF5_DIR, 2)          // FIFO 5 direction
REGDEF_BIT(BUF5_EN, 1)           // Enable FIFO 5
REGDEF_BIT(BUF6_TYP, 2)          // Transfer type of FIFO 6
REGDEF_BIT(BUF6_BLKNO, 2)        // Block number of FIFO 6
REGDEF_BIT(BUF6_BLKSZ, 1)        // Block size of FIFO 6
REGDEF_BIT(BUF6_DIR, 2)          // FIFO 6 direction
REGDEF_BIT(BUF6_EN, 1)           // Enable FIFO 6
REGDEF_BIT(BUF7_TYP, 2)          // Transfer type of FIFO 7
REGDEF_BIT(BUF7_BLKNO, 2)        // Block number of FIFO 7
REGDEF_BIT(BUF7_BLKSZ, 1)        // Block size of FIFO 7
REGDEF_BIT(BUF7_DIR, 2)          // FIFO 7 direction
REGDEF_BIT(BUF7_EN, 1)           // Enable FIFO 7
REGDEF_END(USB_DEVBUFCFG1_REG)

// 0x1B0, Device FIFO 0/1 Instruction and Byte Count Register
#define USB_DEVFIFOBYTECNT0_REG_OFS     0x1B0
REGDEF_BEGIN(USB_DEVFIFOBYTECNT0_REG)
REGDEF_BIT(BC_F0, 11)            // Out FIFO0 byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST0, 1)            // FIFO0 reset
REGDEF_BIT(, 3)
REGDEF_BIT(BC_F1, 11)            // Out FIFO1 byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST1, 1)            // FIFO1 reset
REGDEF_BIT(, 3)
REGDEF_END(USB_DEVFIFOBYTECNT0_REG)

// 0x1B4, Device FIFO 2/3 Instruction and Byte Count Register
#define USB_DEVFIFOBYTECNT1_REG_OFS     0x1B4
REGDEF_BEGIN(USB_DEVFIFOBYTECNT1_REG)
REGDEF_BIT(BC_F2, 11)            // Out FIFO2 byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST2, 1)            // FIFO2 reset
REGDEF_BIT(, 3)
REGDEF_BIT(BC_F3, 11)            // Out FIFO3 byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST3, 1)            // FIFO3 reset
REGDEF_BIT(, 3)
REGDEF_END(USB_DEVFIFOBYTECNT1_REG)

// 0x1B8, Device FIFO 4/5 Instruction and Byte Count Register
#define USB_DEVFIFOBYTECNT2_REG_OFS     0x1B8
REGDEF_BEGIN(USB_DEVFIFOBYTECNT2_REG)
REGDEF_BIT(BC_F4, 11)            // Out FIFO4 byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST4, 1)            // FIFO4 reset
REGDEF_BIT(, 3)
REGDEF_BIT(BC_F5, 11)            // Out FIFO5 byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST5, 1)            // FIFO5 reset
REGDEF_BIT(, 3)
REGDEF_END(USB_DEVFIFOBYTECNT2_REG)

// 0x1BC, Device FIFO 6/7 Instruction and Byte Count Register
#define USB_DEVFIFOBYTECNT3_REG_OFS     0x1BC
REGDEF_BEGIN(USB_DEVFIFOBYTECNT3_REG)
REGDEF_BIT(BC_F6, 11)            // Out FIFO6 byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST6, 1)            // FIFO6 reset
REGDEF_BIT(, 3)
REGDEF_BIT(BC_F7, 11)            // Out FIFO7 byte count
REGDEF_BIT(, 1)
REGDEF_BIT(FFRST7, 1)            // FIFO7 reset
REGDEF_BIT(, 3)
REGDEF_END(USB_DEVFIFOBYTECNT3_REG)

// 0x1C0, Device DMA Target FIFO Number Register
#define USB_DEVACCFIFO_REG_OFS          0x1C0
REGDEF_BEGIN(USB_DEVACCFIFO_REG)
REGDEF_BIT(ACC_F0_3, 3)          // Accessing FIFO 0-3
REGDEF_BIT(, 29)
REGDEF_END(USB_DEVACCFIFO_REG)

// 0x1C4, Device DMA Controller Parameter Setting 0 Register
#define USB_DEVDMACTRL0_REG_OFS         0x1C4
REGDEF_BEGIN(USB_DEVDMACTRL0_REG)
REGDEF_BIT(DMA_ABORT, 1)         // DMA abort
REGDEF_BIT(CLRFIFO_DMA_ABORT, 1) // Clear FIFO when DMA abort
REGDEF_BIT(USB_ACCESS_SELECT, 1) // access SDRAM or embedded ROM
REGDEF_BIT(, 29)
REGDEF_END(USB_DEVDMACTRL0_REG)

// 0x1C8, Device DMA Controller Parameter Setting 1 Register
#define USB_DEVDMACTRL1_REG_OFS         0x1C8
REGDEF_BEGIN(USB_DEVDMACTRL1_REG)
REGDEF_BIT(DMA_START, 1)         // DMA start
REGDEF_BIT(DMA_TYPE, 1)          // DMA type, 0: FIFO to memory, 1: memory to FIFO
REGDEF_BIT(DMA_IO, 1)            // DMA I/O to I/O
REGDEF_BIT(, 1)
REGDEF_BIT(REMOTE_WAKEUP, 1)     // Remote wake up trigger signal
REGDEF_BIT(DEVPHY_SUSPEND, 1)    // Device transceiver suspend mode
REGDEF_BIT(, 2)
REGDEF_BIT(DMA_LEN, 23)          // DMA length Max 8MB-1
REGDEF_BIT(, 1)
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
#define USB_DEVDMA2CTRLSTATUS_REG_OFS   0x1D8
#define USB_DEVDMA3CTRLSTATUS_REG_OFS   0x1DC
#define USB_DEVDMA4CTRLSTATUS_REG_OFS   0x1E0
#define USB_DEVDMA5CTRLSTATUS_REG_OFS   0x1E4

REGDEF_BEGIN(USB_DEVDMACTRLSTATUS_REG)
REGDEF_BIT(DMA_REMLEN, 23)       // Remaining length when DMA_ABORT
REGDEF_BIT(, 9)
REGDEF_END(USB_DEVDMACTRLSTATUS_REG)

// 0x200, DMA 2 Control Register 0
// 0x208, DMA 3 Control Register 0
// 0x210, DMA 4 Control Register 0
// 0x218, DMA 5 Control Register 0
#define USB_DEVDMA2CTRL0_REG_OFS        0x200
#define USB_DEVDMA3CTRL0_REG_OFS        0x208
#define USB_DEVDMA4CTRL0_REG_OFS        0x210
#define USB_DEVDMA5CTRL0_REG_OFS        0x218
REGDEF_BEGIN(USB_DEVDMA2CTRL0_REG)
REGDEF_BIT(DMA_START, 1)         // DMA Start Trigger Bit
REGDEF_BIT(DMA_TYPE, 1)          // DMA type, 0: FIFO to memory, 1: memory to FIFO
REGDEF_BIT(DMA_ABORT, 1)         // DMA Abort
REGDEF_BIT(DMA_CLRFIFO, 1)       // DMA Clear FIFO
REGDEF_BIT(, 4)
REGDEF_BIT(DMA_LEN, 23)          // DMA length Max 8MB-1
REGDEF_BIT(, 1)
REGDEF_END(USB_DEVDMA2CTRL0_REG)

// 0x204, DMA 2 Control Register 1
// 0x20C, DMA 3 Control Register 1
// 0x214, DMA 4 Control Register 1
// 0x21C, DMA 5 Control Register 1
#define USB_DEVDMA2CTRL1_REG_OFS        0x204
#define USB_DEVDMA3CTRL1_REG_OFS        0x20C
#define USB_DEVDMA4CTRL1_REG_OFS        0x214
#define USB_DEVDMA5CTRL1_REG_OFS        0x21C
REGDEF_BEGIN(USB_DEVDMA2CTRL1_REG)
REGDEF_BIT(DMA_MADDR, 32)        // DMA memory address
REGDEF_END(USB_DEVDMA2CTRL1_REG)

//
//  USB Device AUTO HEADER Transmission
//

// 0x278, ISO IN Auto Header Counter Register 0
#define USB_DEVISOIN_COUNT0_REG_OFS     0x278
REGDEF_BEGIN(USB_DEVISOIN_COUNT0_REG)
REGDEF_BIT(CNT_RST, 1)           // Counter Reset
REGDEF_BIT(, 31)
REGDEF_END(USB_DEVISOIN_COUNT0_REG)


// 0x280, ISO IN Auto Header 1 Control Register 0
// 0x2A0, ISO IN Auto Header 2 Control Register 0
#define USB_DEVISOIN_HDR1CTRL0_REG_OFS  0x280
#define USB_DEVISOIN_HDR2CTRL0_REG_OFS  0x2A0
REGDEF_BEGIN(USB_DEVISOIN_HDRCTRL0_REG)
REGDEF_BIT(HDR_EN, 1)            // Header Enable
REGDEF_BIT(HDR_LEN, 1)           // header length select
REGDEF_BIT(, 2)
REGDEF_BIT(HDR_START, 1)         // Video Start Flag
REGDEF_BIT(HDR_STOP, 1)          // Video Stop Flag
REGDEF_BIT(, 2)
REGDEF_BIT(FIFO_SEL, 2)          // FIFO Select
REGDEF_BIT(, 2)
REGDEF_BIT(ZB_LEN, 1)            // Handshake
REGDEF_BIT(, 3)
REGDEF_BIT(HWFID, 1)             // HW/SW FID Value
REGDEF_BIT(HWEOF, 1)             // HW/SW EOF Value
REGDEF_BIT(HWPTS, 1)             // HW/SW PTS Value
REGDEF_BIT(HWSCR, 1)             // HW/SW SCR Value
REGDEF_BIT(, 12)
REGDEF_END(USB_DEVISOIN_HDRCTRL0_REG)

// 0x284, ISO IN Auto Header 1 Header Register 0
// 0x2A4, ISO IN Auto Header 2 Header Register 0
#define USB_DEVISOIN_HDR1SW0_REG_OFS  0x284
#define USB_DEVISOIN_HDR2SW0_REG_OFS  0x2A4
REGDEF_BEGIN(USB_DEVISOIN_HDRSW0_REG)
REGDEF_BIT(HDR_LENGTH, 8)        // Header Length value in the header content
REGDEF_BIT(SWFID, 1)             // Software FID Value
REGDEF_BIT(SWEOF, 1)             // Software EOF Value
REGDEF_BIT(PTS, 1)               // Presentation Time Stamp field exist
REGDEF_BIT(SCR, 1)               // Source Clock Reference field exist
REGDEF_BIT(RES, 1)               // Reserved Field
REGDEF_BIT(STI, 1)               // Still Image for current frame
REGDEF_BIT(ERR, 1)               // Error
REGDEF_BIT(EOH, 1)               // End of Header
REGDEF_BIT(PTS15, 16)            // PTS[15:0]
REGDEF_END(USB_DEVISOIN_HDRSW0_REG)

// 0x288, ISO IN Auto Header 1 Header Register 1
// 0x2A8, ISO IN Auto Header 2 Header Register 1
#define USB_DEVISOIN_HDR1SW1_REG_OFS  0x288
#define USB_DEVISOIN_HDR2SW1_REG_OFS  0x2A8
REGDEF_BEGIN(USB_DEVISOIN_HDRSW1_REG)
REGDEF_BIT(PTS31, 16)            // PTS[31:16]
REGDEF_BIT(SCR15, 16)            // SCR[15:0]
REGDEF_END(USB_DEVISOIN_HDRSW1_REG)

// 0x28C, ISO IN Auto Header 1 Header Register 2
// 0x2AC, ISO IN Auto Header 2 Header Register 2
#define USB_DEVISOIN_HDR1SW2_REG_OFS  0x28C
#define USB_DEVISOIN_HDR2SW2_REG_OFS  0x2AC
REGDEF_BEGIN(USB_DEVISOIN_HDRSW2_REG)
REGDEF_BIT(SCR47, 16)            // SCR[47:16]
REGDEF_END(USB_DEVISOIN_HDRSW2_REG)

// 0x290, ISO IN Auto Header 1 Header Register 3
// 0x2B0, ISO IN Auto Header 2 Header Register 3
#define USB_DEVISOIN_HDR1SW3_REG_OFS  0x290
#define USB_DEVISOIN_HDR2SW3_REG_OFS  0x2B0
REGDEF_BEGIN(USB_DEVISOIN_HDRSW3_REG)
REGDEF_BIT(HWFID, 1)             // HW FID Value
REGDEF_BIT(HWEOF, 1)             // HW EOF Value
REGDEF_BIT(, 30)
REGDEF_END(USB_DEVISOIN_HDRSW3_REG)

// 0x294, ISO IN Auto Header 1 Header Register 4
// 0x2B4, ISO IN Auto Header 2 Header Register 4
#define USB_DEVISOIN_HDR1SW4_REG_OFS  0x294
#define USB_DEVISOIN_HDR2SW4_REG_OFS  0x2B4
REGDEF_BEGIN(USB_DEVISOIN_HDRSW4_REG)
REGDEF_BIT(PTS31, 32)             // PTS[31:0]
REGDEF_END(USB_DEVISOIN_HDRSW4_REG)

// 0x298, ISO IN Auto Header 1 Header Register 5
// 0x2B8, ISO IN Auto Header 2 Header Register 5
#define USB_DEVISOIN_HDR1SW5_REG_OFS  0x298
#define USB_DEVISOIN_HDR2SW5_REG_OFS  0x2B8
REGDEF_BEGIN(USB_DEVISOIN_HDRSW5_REG)
REGDEF_BIT(SCR31, 32)             // SCR[31:0]
REGDEF_END(USB_DEVISOIN_HDRSW5_REG)

// 0x29C, ISO IN Auto Header 1 Header Register 6
// 0x2BC, ISO IN Auto Header 2 Header Register 6
#define USB_DEVISOIN_HDR1SW6_REG_OFS  0x29C
#define USB_DEVISOIN_HDR2SW6_REG_OFS  0x2BC
REGDEF_BEGIN(USB_DEVISOIN_HDRSW6_REG)
REGDEF_BIT(SCR47, 16)             // SCR[47:32]
REGDEF_BIT(, 16)
REGDEF_END(USB_DEVISOIN_HDRSW6_REG)


//
//  USB Charger Related
//

#define USB_CHARGER_CONTROL_REG_OFS     0x300
REGDEF_BEGIN(USB_CHARGER_CONTROL_REG)
REGDEF_BIT(DCD_DETECT_EN, 1)
REGDEF_BIT(CHG_DETECT_EN, 1)
REGDEF_BIT(SECOND_CHG_DETECT_EN, 1)
REGDEF_BIT(reserved, 29)
REGDEF_END(USB_CHARGER_CONTROL_REG)

#define USB_DCD_COUNTER_REG_OFS         0x304
REGDEF_BEGIN(USB_DCD_COUNTER_REG)
REGDEF_BIT(DCD_COUNT, 22)
REGDEF_BIT(, 10)
REGDEF_END(USB_DCD_COUNTER_REG)

#define USB_BUS_STATUS_REG_OFS          0x308
REGDEF_BEGIN(USB_BUS_STATUS_REG)
REGDEF_BIT(reserved1, 6)
REGDEF_BIT(CHG_DET, 1)
REGDEF_BIT(reserved, 25)
REGDEF_END(USB_BUS_STATUS_REG)

#define USB_TOP_REG_OFS                 0x310
REGDEF_BEGIN(USB_TOP_REG)
REGDEF_BIT(VBUSI, 1)
REGDEF_BIT(ID, 1)
REGDEF_BIT(, 14)
REGDEF_BIT(AHBC_IDLE, 1)
REGDEF_BIT(, 15)
REGDEF_END(USB_TOP_REG)


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



















#endif
