/*
 * arch/arm/plat-ambarella/include/plat/udc.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_UDC_H__
#define __PLAT_AMBARELLA_UDC_H__

/* ==========================================================================*/
#define USBDC_OFFSET			0x6000
#define USBDC_BASE			(AHB_BASE + USBDC_OFFSET)
#define USBDC_REG(x)			(USBDC_BASE + (x))

/* ==========================================================================*/

/*----------------------------------------------
    macros
----------------------------------------------*/
#define USB_READ_REG(adr)	(*(volatile unsigned long *)(adr))
#define USB_WRITE_REG(adr, val)	(*(volatile unsigned long *)(adr)=(val))
#define USB_SET_REG(adr, val)	(USB_WRITE_REG((adr), (USB_READ_REG(adr) | (val))))
#define USB_CLR_REG(adr, val)	(USB_WRITE_REG((adr), (USB_READ_REG(adr) & ~(val))))

//-------------------------------------
// USB RxFIFO and TxFIFO depth (single or multiple)
//-------------------------------------
#define USB_RXFIFO_DEPTH_CTRLOUT	(256 << 16)	// shared
#define USB_RXFIFO_DEPTH_BULKOUT	(256 << 16)	// shared
#define USB_RXFIFO_DEPTH_INTROUT	(256 << 16)	// shared
#define USB_TXFIFO_DEPTH_CTRLIN		(64 / 4)	// 16 32-bit
#define USB_TXFIFO_DEPTH_BULKIN		(1024 / 4)	// 256 32-bit
#define USB_TXFIFO_DEPTH_INTRIN		(512 / 4)	// 128 32-bit
#define USB_TXFIFO_DEPTH_ISOIN		((512 * 2) / 4)	// 128 32-bit

#define USB_TXFIFO_DEPTH		(64 / 4 + 4 * 512 / 4)	// 528 32-bit
#define USB_RXFIFO_DEPTH		(256)			// 256 32-bit

//-------------------------------------
// USB memory map
//-------------------------------------
#define USB_EP_IN_BASE		USBDC_BASE		// IN endpoint specific registers
#define USB_EP_OUT_BASE		(USBDC_BASE + 0x0200)	// OUT endpoint specific registers
#define USB_DEV_BASE		(USBDC_BASE + 0x0400)	// device specific registers
#define USB_UDC_BASE		(USBDC_BASE + 0x0504)	// UDC specific registers, 0x0500 is reserved for control endpoint (RO)
#define USB_RXFIFO_BASE		(USBDC_BASE + 0x0800)	// RxFIFO
#define USB_TXFIFO_BASE		(USB_RXFIFO_BASE + USB_RXFIFO_DEPTH)	// TxFIFO

//-------------------------------------
// USB register address
//-------------------------------------
#define	USB_EP_IN_CTRL_REG(n)		(USB_EP_IN_BASE + 0x0000 + 0x0020 * (n))
#define	USB_EP_IN_STS_REG(n)		(USB_EP_IN_BASE + 0x0004 + 0x0020 * (n))
#define	USB_EP_IN_BUF_SZ_REG(n)		(USB_EP_IN_BASE + 0x0008 + 0x0020 * (n))
#define	USB_EP_IN_MAX_PKT_SZ_REG(n)	(USB_EP_IN_BASE + 0x000c + 0x0020 * (n))
#define	USB_EP_IN_DAT_DESC_PTR_REG(n)	(USB_EP_IN_BASE + 0x0014 + 0x0020 * (n))
#define USB_EP_IN_WR_CFM_REG		(USB_EP_IN_BASE + 0x001c + 0x0020 * (n))	// for slave-only mode

#define	USB_EP_OUT_CTRL_REG(n)		(USB_EP_OUT_BASE + 0x0000 + 0x0020 * (n))
#define	USB_EP_OUT_STS_REG(n)		(USB_EP_OUT_BASE + 0x0004 + 0x0020 * (n))
#define	USB_EP_OUT_PKT_FRM_NUM_REG(n)	(USB_EP_OUT_BASE + 0x0008 + 0x0020 * (n))
#define	USB_EP_OUT_MAX_PKT_SZ_REG(n) 	(USB_EP_OUT_BASE + 0x000c + 0x0020 * (n))
#define	USB_EP_OUT_SETUP_BUF_PTR_REG(n) (USB_EP_OUT_BASE + 0x0010 + 0x0020 * (n))
#define	USB_EP_OUT_DAT_DESC_PTR_REG(n)	(USB_EP_OUT_BASE + 0x0014 + 0x0020 * (n))
#define USB_EP_OUT_RD_CFM_ZO_REG	(USB_EP_OUT_BASE + 0x001c + 0x0020 * (n))	// for slave-only mode

#define USB_DEV_CFG_REG			(USB_DEV_BASE + 0x0000)
#define USB_DEV_CTRL_REG		(USB_DEV_BASE + 0x0004)
#define USB_DEV_STS_REG			(USB_DEV_BASE + 0x0008)
#define USB_DEV_INTR_REG		(USB_DEV_BASE + 0x000c)
#define USB_DEV_INTR_MSK_REG		(USB_DEV_BASE + 0x0010)
#define USB_DEV_EP_INTR_REG		(USB_DEV_BASE + 0x0014)
#define USB_DEV_EP_INTR_MSK_REG		(USB_DEV_BASE + 0x0018)
#define USB_DEV_TEST_MODE_REG		(USB_DEV_BASE + 0x001c)

#define USB_UDC_REG(n)			(USB_UDC_BASE + 0x0004 * (n))	// EP0 is reserved for control endpoint

//-------------------------------------
// USB register fields
//-------------------------------------
//-----------------
// for endpoint specific registers
//-----------------
// for USB_EP_IN_CTRL_REG(n) and USB_EP_OUT_CTRL_REG(n)	// default
#define USB_EP_STALL			0x00000001		// 0 (RW)
#define USB_EP_FLUSH			0x00000002		// 0 (RW)
#define USB_EP_SNOOP			0x00000004		// 0 (RW) - for OUT EP only
#define USB_EP_POLL_DEMAND		0x00000008		// 0 (RW) - for IN EP only
#define USB_EP_TYPE_CTRL		0x00000000		// 00 (RW)
#define USB_EP_TYPE_ISO			0x00000010		// 00 (RW)
#define USB_EP_TYPE_BULK		0x00000020		// 00 (RW)
#define USB_EP_TYPE_INTR		0x00000030		// 00 (RW)
#define USB_EP_NAK_STS			0x00000040		// 0 (RO) - set by UDC after SETUP and STALL
#define USB_EP_SET_NAK			0x00000080		// 0 (WO)
#define USB_EP_CLR_NAK			0x00000100		// 0 (WO)
#define USB_EP_RCV_RDY			0x00000200		// 0 (RW)(D) - set by APP when APP is ready for DMA
														//			   clear by UDC when end of each packet if USB_DEV_DESC_UPD is set
														//			   clear by UDC when end of payload if USB_DEV_DESC_UPD is clear

// for USB_EP_IN_STS_REG(n) and USB_EP_OUT_STS_REG(n)
#define USB_EP_OUT_PKT_MSK		0x00000030		// 00 (R/WC) - 01 for OUT and 10 for SETUP - for OUT EP only
#define USB_EP_OUT_PKT			0x00000010
#define USB_EP_SETUP_PKT		0x00000020
#define USB_EP_IN_PKT			0x00000040		// 0 (R/WC) - for IN EP only
#define USB_EP_BUF_NOT_AVAIL		0x00000080		// 0 (R/WC)(D) - set by UDC when descriptor is not available
														//	         	 clear by APP when interrupt is serviced
#define USB_EP_HOST_ERR			0x00000200		// 0 (R/WC) - set by DMA when DMA is error
#define USB_EP_TRN_DMA_CMPL		0x00000400		// 0 (R/WC)(D) - set by DMA when DMA is completed
#define USB_EP_RCV_CLR_STALL            0x02000000              // 0 (R/WC) - Received Clear Stall indication
														//				 clear by APP when interrupt is serviced
#define USB_EP_RCV_SET_STALL            0x04000000		// 0 (R/WC) - Received Set Stall indication
#define USB_EP_RX_PKT_SZ		0x007ff800		// bit mask (RW) - receive packet size in RxFIFO (e.g. SETUP=64, BULK=512)

#define USB_EP_TXFIFO_EMPTY		0x08000000		// 0 (R/WC) - TxFIFO is empty after DMA transfer done

// for USB_EP_IN_BUF_SZ_REG(n) and USB_EP_OUT_PKT_FRM_NUM_REG(n)
#define USB_EP_TXFIFO_DEPTH		0x0000ffff		// bit mask (RW) - for IN EP only
#define USB_EP_FRM_NUM			0x0000ffff		// bit mask (RW) - for OUT EP only

// for USB_EP_IN_MAX_PKT_SZ_REG(n) and USB_EP_OUT_MAX_PKT_SZ_REG(n)
#define USB_EP_RXFIFO_DEPTH		0xffff0000		// bit mask (RW) - for OUT EP only
#define USB_EP_MAX_PKT_SZ		0x0000ffff		// bit mask (RW)

//-----------------
// for device specific registers
//-----------------
// for USB_DEV_CFG_REG
#define USB_DEV_SPD_HI			0x00000000		// 00 (RW) - PHY CLK = 30 or 60 MHz
#define USB_DEV_SPD_FU			0x00000001		// 00 (RW) - PHY CLK = 30 or 60 MHz
#define USB_DEV_SPD_LO			0x00000002		// 00 (RW) - PHY CLK = 6 MHz
#define USB_DEV_SPD_FU48		0x00000003		// 00 (RW) - PHY CLK = 48 MHz

#define USB_DEV_REMOTE_WAKEUP_EN	0x00000004		// 0 (RW)

#define USB_DEV_BUS_POWER		0x00000000		// 0 (RW)
#define USB_DEV_SELF_POWER		0x00000008		// 0 (RW)

#define USB_DEV_SYNC_FRM_EN		0x00000010		// 0 (RW)

#define USB_DEV_PHY_16BIT		0x00000000		// 0 (RW)
#define USB_DEV_PHY_8BIT		0x00000020		// 0 (RW)

#define USB_DEV_UTMI_DIR_UNI		0x00000000		// 0 (RW) - UDC20 reserved to 0
#define USB_DEV_UTMI_DIR_BI		0x00000040		// 0 (RW)

#define USB_DEV_STS_OUT_NONZERO		0x00000180		// bit mask (RW)

#define USB_DEV_PHY_ERR			0x00000200		// 0 (RW)

#define USB_DEV_SPD_FU_TIMEOUT		0x00001c00		// bit mask (RW)
#define USB_DEV_SPD_HI_TIMEOUT		0x0000e000		// bit mask (RW)

#define USB_DEV_HALT_ACK		0x00000000		// 0 (RW) - ACK Clear_Feature (ENDPOINT_HALT) of EP0
#define USB_DEV_HALT_STALL		0x00010000		// 1 (RW) - STALL Clear_Feature (ENDPOINT_HALT) of EP0

#define USB_DEV_CSR_PRG_EN		0x00020000		// 1 (RW) - enable CSR_DONE of USB_DEV_CFG_REG

#define USB_DEV_SET_DESC_STALL		0x00000000		// 0 (RW) - STALL Set_Descriptor
#define USB_DEV_SET_DESC_ACK		0x00040000		// 1 (RW) - ACK Set_Descriptor

#define USB_DEV_SDR			0x00000000		// 0 (RW)
#define USB_DEV_DDR			0x00080000		// 1 (RW)

// for USB_DEV_CTRL_REG
#define USB_DEV_REMOTE_WAKEUP		0x00000001		// 0 (RW)
#define USB_DEV_RCV_DMA_EN		0x00000004		// 0 (RW)(D)
#define USB_DEV_TRN_DMA_EN		0x00000008		// 0 (RW)(D)

#define USB_DEV_DESC_UPD_PYL		0x00000000		// 0 (RW)(D)
#define USB_DEV_DESC_UPD_PKT		0x00000010		// 0 (RW)(D)

#define USB_DEV_LITTLE_ENDN		0x00000000		// 0 (RW)(D)
#define USB_DEV_BIG_ENDN		0x00000020		// 0 (RW)(D)

#define USB_DEV_PKT_PER_BUF_MD		0x00000000		// 0 (RW)(D) - packet-per-buffer mode
#define USB_DEV_BUF_FIL_MD		0x00000040		// 0 (RW)(D) - buffer fill mode

#define USB_DEV_THRESH_EN		0x00000080		// 0 (RW)(D) - for OUT EP only

#define USB_DEV_BURST_EN		0x00000100		// 0 (RW)(D)

#define USB_DEV_SLAVE_ONLY_MD		0x00000000		// 0 (RW)(D)
#define USB_DEV_DMA_MD			0x00000200		// 0 (RW)(D)

#define USB_DEV_SOFT_DISCON		0x00000400		// 0 (RW) - signal UDC20 to disconnect
#define USB_DEV_TIMER_SCALE_DOWN	0x00000800		// 0 (RW) - for gate-level simulation only
#define USB_DEV_NAK			0x00001000		// 0 (RW) - device NAK (applied to all EPs)
#define USB_DEV_CSR_DONE		0x00002000		// 0 (RW) - set to ACK Set_Configuration or Set_Interface if USB_DEV_CSR_PRG_EN
#define USB_DEV_FLUSH_RXFIFO		0x00004000
														//		    clear to NAK
#define USB_DEV_BURST_LEN		0x00070000		// bit mask (RW)
#define USB_DEV_THRESH_LEN		0x0f000000		// bit mask (RW)

// for USB_DEV_STS_REG
#define USB_DEV_CFG_NUM			0x0000000f		// bit mask (RO)
#define USB_DEV_INTF_NUM		0x000000f0		// bit mask (RO)
#define USB_DEV_ALT_SET			0x00000f00		// bit mask (RO)
#define USB_DEV_SUSP_STS		0x00001000		// bit mask (RO)

#define USB_DEV_ENUM_SPD		0x00006000		// bit mask (RO)
#define USB_DEV_ENUM_SPD_HI		0x00000000		// 00 (RO)
#define USB_DEV_ENUM_SPD_FU		0x00002000		// 00 (RO)
#define USB_DEV_ENUM_SPD_LO		0x00004000		// 00 (RO)
#define USB_DEV_ENUM_SPD_FU48		0x00006000		// 00 (RO)

#define USB_DEV_RXFIFO_EMPTY_STS	0x00008000		// bit mask (RO)
#define USB_DEV_PHY_ERR_STS		0x00010000		// bit mask (RO)
#define USB_DEV_FRM_NUM			0xfffc0000		// bit mask (RO)

// for USB_DEV_INTR_REG
#define USB_DEV_SET_CFG			0x00000001		// 0 (R/WC)
#define USB_DEV_SET_INTF		0x00000002		// 0 (R/WC)
#define USB_DEV_IDLE_3MS		0x00000004		// 0 (R/WC)
#define USB_DEV_RESET			0x00000008		// 0 (R/WC)
#define USB_DEV_SUSP			0x00000010		// 0 (R/WC)
#define USB_DEV_SOF			0x00000020		// 0 (R/WC)
#define USB_DEV_ENUM_CMPL		0x00000040		// 0 (R/WC)

// for USB_DEV_INTR_MSK_REG
#define USB_DEV_MSK_SET_CFG		0x00000001		// 0 (R/WC)
#define USB_DEV_MSK_SET_INTF		0x00000002		// 0 (R/WC)
#define USB_DEV_MSK_IDLE_3MS		0x00000004		// 0 (R/WC)
#define USB_DEV_MSK_RESET		0x00000008		// 0 (R/WC)
#define USB_DEV_MSK_SUSP		0x00000010		// 0 (R/WC)
#define USB_DEV_MSK_SOF			0x00000020		// 0 (R/WC)
#define USB_DEV_MSK_SPD_ENUM_CMPL	0x00000040		// 0 (R/WC)

// for USB_DEV_EP_INTR_REG
#define USB_DEV_EP0_IN			0x00000001		// 0 (R/WC)
#define USB_DEV_EP1_IN			0x00000002		// 0 (R/WC)
#define USB_DEV_EP2_IN			0x00000004		// 0 (R/WC)
#define USB_DEV_EP3_IN			0x00000008		// 0 (R/WC)
#define USB_DEV_EP4_IN			0x00000010		// 0 (R/WC)
#define USB_DEV_EP5_IN			0x00000020		// 0 (R/WC)
#define USB_DEV_EP6_IN			0x00000040		// 0 (R/WC)
#define USB_DEV_EP7_IN			0x00000080		// 0 (R/WC)
#define USB_DEV_EP8_IN			0x00000100		// 0 (R/WC)
#define USB_DEV_EP9_IN			0x00000200		// 0 (R/WC)
#define USB_DEV_EP10_IN			0x00000400		// 0 (R/WC)
#define USB_DEV_EP11_IN			0x00000800		// 0 (R/WC)
#define USB_DEV_EP12_IN			0x00001000		// 0 (R/WC)
#define USB_DEV_EP13_IN			0x00002000		// 0 (R/WC)
#define USB_DEV_EP14_IN			0x00004000		// 0 (R/WC)
#define USB_DEV_EP15_IN			0x00008000		// 0 (R/WC)
#define USB_DEV_EP0_OUT			0x00010000		// 0 (R/WC)
#define USB_DEV_EP1_OUT			0x00020000		// 0 (R/WC)
#define USB_DEV_EP2_OUT			0x00040000		// 0 (R/WC)
#define USB_DEV_EP3_OUT			0x00080000		// 0 (R/WC)
#define USB_DEV_EP4_OUT			0x00100000		// 0 (R/WC)
#define USB_DEV_EP5_OUT			0x00200000		// 0 (R/WC)
#define USB_DEV_EP6_OUT			0x00400000		// 0 (R/WC)
#define USB_DEV_EP7_OUT			0x00800000		// 0 (R/WC)
#define USB_DEV_EP8_OUT			0x01000000		// 0 (R/WC)
#define USB_DEV_EP9_OUT			0x02000000		// 0 (R/WC)
#define USB_DEV_EP10_OUT		0x04000000		// 0 (R/WC)
#define USB_DEV_EP11_OUT		0x08000000		// 0 (R/WC)
#define USB_DEV_EP12_OUT		0x10000000		// 0 (R/WC)
#define USB_DEV_EP13_OUT		0x20000000		// 0 (R/WC)
#define USB_DEV_EP14_OUT		0x40000000		// 0 (R/WC)
#define USB_DEV_EP15_OUT		0x80000000		// 0 (R/WC)

// for USB_DEV_EP_INTR_MSK_REG
#define USB_DEV_MSK_EP0_IN		0x00000001		// 0 (R/WC)
#define USB_DEV_MSK_EP1_IN		0x00000002		// 0 (R/WC)
#define USB_DEV_MSK_EP2_IN		0x00000004		// 0 (R/WC)
#define USB_DEV_MSK_EP3_IN		0x00000008		// 0 (R/WC)
#define USB_DEV_MSK_EP4_IN		0x00000010		// 0 (R/WC)
#define USB_DEV_MSK_EP5_IN		0x00000020		// 0 (R/WC)
#define USB_DEV_MSK_EP6_IN		0x00000040		// 0 (R/WC)
#define USB_DEV_MSK_EP7_IN		0x00000080		// 0 (R/WC)
#define USB_DEV_MSK_EP8_IN		0x00000100		// 0 (R/WC)
#define USB_DEV_MSK_EP9_IN		0x00000200		// 0 (R/WC)
#define USB_DEV_MSK_EP10_IN		0x00000400		// 0 (R/WC)
#define USB_DEV_MSK_EP11_IN		0x00000800		// 0 (R/WC)
#define USB_DEV_MSK_EP12_IN		0x00001000		// 0 (R/WC)
#define USB_DEV_MSK_EP13_IN		0x00002000		// 0 (R/WC)
#define USB_DEV_MSK_EP14_IN		0x00004000		// 0 (R/WC)
#define USB_DEV_MSK_EP15_IN		0x00008000		// 0 (R/WC)
#define USB_DEV_MSK_EP0_OUT		0x00010000		// 0 (R/WC)
#define USB_DEV_MSK_EP1_OUT		0x00020000		// 0 (R/WC)
#define USB_DEV_MSK_EP2_OUT		0x00040000		// 0 (R/WC)
#define USB_DEV_MSK_EP3_OUT		0x00080000		// 0 (R/WC)
#define USB_DEV_MSK_EP4_OUT		0x00100000		// 0 (R/WC)
#define USB_DEV_MSK_EP5_OUT		0x00200000		// 0 (R/WC)
#define USB_DEV_MSK_EP6_OUT		0x00400000		// 0 (R/WC)
#define USB_DEV_MSK_EP7_OUT		0x00800000		// 0 (R/WC)
#define USB_DEV_MSK_EP8_OUT		0x01000000		// 0 (R/WC)
#define USB_DEV_MSK_EP9_OUT		0x02000000		// 0 (R/WC)
#define USB_DEV_MSK_EP10_OUT		0x04000000		// 0 (R/WC)
#define USB_DEV_MSK_EP11_OUT		0x08000000		// 0 (R/WC)
#define USB_DEV_MSK_EP12_OUT		0x10000000		// 0 (R/WC)
#define USB_DEV_MSK_EP13_OUT		0x20000000		// 0 (R/WC)
#define USB_DEV_MSK_EP14_OUT		0x40000000		// 0 (R/WC)
#define USB_DEV_MSK_EP15_OUT		0x80000000		// 0 (R/WC)

// for USB_DEV_TEST_MODE_REG
#define USB_DEV_TEST_MD			0x00000001		// 0 (RW)

// for USB_UDC_REG
#define USB_UDC_EP0_NUM			0x00000000
#define USB_UDC_EP1_NUM			0x00000001
#define USB_UDC_EP2_NUM			0x00000002
#define USB_UDC_EP3_NUM			0x00000003
#define USB_UDC_EP4_NUM			0x00000004
#define USB_UDC_EP5_NUM			0x00000005
#define USB_UDC_EP6_NUM			0x00000006
#define USB_UDC_EP7_NUM			0x00000007
#define USB_UDC_EP8_NUM			0x00000008
#define USB_UDC_EP9_NUM			0x00000009
#define USB_UDC_EP10_NUM		0x0000000a
#define USB_UDC_EP11_NUM		0x0000000b
#define USB_UDC_EP12_NUM		0x0000000c
#define USB_UDC_EP13_NUM		0x0000000d
#define USB_UDC_EP14_NUM		0x0000000e
#define USB_UDC_EP15_NUM		0x0000000f

#define USB_UDC_OUT			0x00000000
#define USB_UDC_IN			0x00000010

#define USB_UDC_CTRL			0x00000000
#define USB_UDC_ISO			0x00000020
#define USB_UDC_BULK			0x00000040
#define USB_UDC_INTR			0x00000060

#define USB_EP_CTRL_MAX_PKT_SZ		64	// max packet size of control in/out endpoint
#define USB_EP_BULK_MAX_PKT_SZ_HI	512	// max packet size of bulk in/out endpoint (high speed)
#define USB_EP_BULK_MAX_PKT_SZ_FU	64	// max packet size of bulk in/out endpoint (full speed)
#define USB_EP_INTR_MAX_PKT_SZ		64	// max packet size of interrupt in/out endpoint
#define USB_EP_ISO_MAX_PKT_SZ		512	// max packet size of isochronous in/out endpoint

#define USB_EP_CTRLIN_MAX_PKT_SZ	USB_EP_CTRL_MAX_PKT_SZ
#define USB_EP_CTRLOUT_MAX_PKT_SZ	USB_EP_CTRL_MAX_PKT_SZ
#define USB_EP_BULKIN_MAX_PKT_SZ_HI	USB_EP_BULK_MAX_PKT_SZ_HI
#define USB_EP_BULKOUT_MAX_PKT_SZ_HI	USB_EP_BULK_MAX_PKT_SZ_HI
#define USB_EP_BULKIN_MAX_PKT_SZ_FU	USB_EP_BULK_MAX_PKT_SZ_FU
#define USB_EP_BULKOUT_MAX_PKT_SZ_FU	USB_EP_BULK_MAX_PKT_SZ_FU
#define USB_EP_INTRIN_MAX_PKT_SZ	USB_EP_INTR_MAX_PKT_SZ
#define USB_EP_INTROUT_MAX_PKT_SZ	USB_EP_INTR_MAX_PKT_SZ
#define USB_EP_ISOIN_MAX_PKT_SZ		USB_EP_ISO_MAX_PKT_SZ
#define USB_EP_ISOOUT_MAX_PKT_SZ	USB_EP_ISO_MAX_PKT_SZ
#define USB_EP_ISOIN_TRANSACTIONS	1
#define USB_EP_ISOOUT_TRANSACTIONS	1

#define USB_UDC_MAX_PKT_SZ		(0x1fff << 19)
#define USB_UDC_CTRLIN_MAX_PKT_SZ	(USB_EP_CTRLIN_MAX_PKT_SZ << 19)
#define USB_UDC_CTRLOUT_MAX_PKT_SZ	(USB_EP_CTRLOUT_MAX_PKT_SZ << 19)
#define USB_UDC_BULKIN_MAX_PKT_SZ_HI	(USB_EP_BULKIN_MAX_PKT_SZ_HI << 19)
#define USB_UDC_BULKOUT_MAX_PKT_SZ_HI	(USB_EP_BULKOUT_MAX_PKT_SZ_HI << 19)
#define USB_UDC_BULKIN_MAX_PKT_SZ_FU	(USB_EP_BULKIN_MAX_PKT_SZ_FU << 19)
#define USB_UDC_BULKOUT_MAX_PKT_SZ_FU	(USB_EP_BULKOUT_MAX_PKT_SZ_FU << 19)
#define USB_UDC_INTRIN_MAX_PKT_SZ	(USB_EP_INTRIN_MAX_PKT_SZ << 19)
#define USB_UDC_INTROUT_MAX_PKT_SZ	(USB_EP_INTROUT_MAX_PKT_SZ << 19)
#define USB_UDC_ISOIN_MAX_PKT_SZ	(USB_EP_ISOIN_MAX_PKT_SZ << 19)
#define USB_UDC_ISOOUT_MAX_PKT_SZ	(USB_EP_ISOOUT_MAX_PKT_SZ << 19)

//-------------------------------------
// DMA status quadlet fields
//-------------------------------------
// IN / OUT descriptor specific
#define USB_DMA_RXTX_BYTES		0x0000ffff		// bit mask

// SETUP descriptor specific
#define USB_DMA_CFG_STS			0x0fff0000		// bit mask
#define USB_DMA_CFG_NUM			0x0f000000		// bit mask
#define USB_DMA_INTF_NUM		0x00f00000		// bit mask
#define USB_DMA_ALT_SET			0x000f0000		// bitmask
// ISO OUT descriptor only
#define USB_DMA_FRM_NUM			0x07ff0000		// bit mask
// IN / OUT descriptor specific
#define USB_DMA_LAST			0x08000000		// bit mask

#define USB_DMA_RXTX_STS		0x30000000		// bit mask
#define USB_DMA_RXTX_SUCC		0x00000000		// 00
#define USB_DMA_RXTX_DES_ERR		0x10000000		// 01
#define USB_DMA_RXTX_BUF_ERR		0x30000000		// 11

#define USB_DMA_BUF_STS 		0xc0000000		// bit mask
#define USB_DMA_BUF_HOST_RDY		0x00000000		// 00
#define USB_DMA_BUF_DMA_BUSY		0x40000000		// 01
#define USB_DMA_BUF_DMA_DONE		0x80000000		// 10
#define	USB_DMA_BUF_HOST_BUSY		0xc0000000		// 11

/* ==========================================================================*/

#endif

