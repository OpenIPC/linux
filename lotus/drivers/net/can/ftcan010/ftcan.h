/* SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __FTCAN_H
#define __FTCAN_H


/* CAN registers structure */
struct can_regs {
	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * CAN_Cantrol |    CE1     |    CE0     |    IRE     |     CRR    | 0x0000
	 *              .....................................................
	 * CAN_Status  |  RESERVED  |     TS     |    TRBS    |     BS     | 0x0004
	 *             .....................................................
	 */
	union {
		struct {
			u8 crr; /* Command Request Register */
			u8 ire; /* Interrupt Request Enable Register */
			u8 ce0; /* Control Enable 0 Register */
			u8 ce1; /* Control Enable 1 Register */
		} byte;
		u32 all;
	} can_cantrol;
	union {
		struct {
			u8 bsr; /* Bus Status Register */
			u8 trbs; /* Transmit and Receive Buffer Status Register */
			u8 ts; /* Transmit Status Register */
			u8 reserved3;
		} byte;
		u32 all;
	} can_status;

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * CAN_TR0     |    TEIM    |    TEIL    |   TREIE    |    TFD     | 0x0008
	 *             .....................................................
	 *             |  RESERVED  |    TBIH    |   TBILDL   |    TEIH    | 0x000C
	 *             .....................................................
	 * CAN_TR1     |    TEIM    |    TEIL    |   TREIE    |    TFD     | 0x0010
	 *             .....................................................
	 *             |  RESERVED  |    TBIH    |   TBILDL   |    TEIH    | 0x0014
	 *             .....................................................
	 * CAN_TR2     |    TEIM    |    TEIL    |   TREIE    |    TFD     | 0x0018
	 *             .....................................................
	 *             |  RESERVED  |    TBIH    |   TBILDL   |    TEIH    | 0x001C
	 *             .....................................................
	 */
	/* Transmit Register 0,1,2 */
	struct {
		union {
			struct {
				u8 tfd; /* Transmit Flexible Data-Rate Register */
				/* Transmit Remote and Extended Identifier Enable Register */
				u8 treie;
				u8 teil; /* Transmit Extended Identifier Low Register */
				u8 teim; /* Transmit Extended Identifier Middle Register */
			} byte;
		u32 all;
		} tx_l;
		union{
			struct {
				u8 teih; /* Transmit Extended Identifier High Register */
				/* Transmit Base Identifier Low and Data Length Register */
				u8 tbildl;
				u8 tbih; /* Transmit Base Identifier High Register */
				u8 reserved;
			} byte;
			u32 all;
		} tx_h;
	} can_tr[3];

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * CAN_AFIR0   |   AFBIH    |  AFBILEIH  |   AFEIM    |   AFEIL    | 0x0020
	 *             .....................................................
	 * CAN_AFIR1   |   AFBIH    |  AFBILEIH  |   AFEIM    |   AFEIL    | 0x0024
	 *             .....................................................
	 * CAN_AFIR2   |   AFBIH    |  AFBILEIH  |   AFEIM    |   AFEIL    | 0x0028
	 *             .....................................................
	 * CAN_AFIR3   |   AFBIH    |  AFBILEIH  |   AFEIM    |   AFEIL    | 0x002C
	 *             .....................................................
	 * CAN_AFIR4   |   AFBIH    |  AFBILEIH  |   AFEIM    |   AFEIL    | 0x0030
	 *             .....................................................
	 * CAN_AFIR5   |   AFBIH    |  AFBILEIH  |   AFEIM    |   AFEIL    | 0x0034
	 *             .....................................................
	 * AFR_Data    |   AFDB1_1  |   AFDB0_1  |   AFDB1_0  |   AFDB0_0  | 0x0038
	 *             .....................................................
	 * AFR_Control |  RESERVED  |  RESERVED  |    AFC1    |    AFC0    | 0x003C
	 *             .....................................................
	 */
	struct {
		/* Acceptance Filter Identifier Register 0,1,2,3,4,5 */
		union {
			struct {
				/* Acceptance Filter Extended Identifier Low Register */
				u8 afeil;
				/* Acceptance Filter Extended Identifier Middle Register */
				u8 afeim;
				/**
				 * Acceptance Filter Base Identifier Low and
				 * Extended Identifier High Register
				 */
				u8 afbileih;
				/* Acceptance Filter Base Identifier High Register */
				u8 afbih;
			} byte;
			u32 all;
			} identifier[6];
		/* Acceptance Filter Data 0,1 Register 0,1 */
		union {
			struct {
				/* Acceptance Filter Data Byte0 Register */
				u8 afdb0_0;
				/* Acceptance Filter Data Byte1 Register */
				u8 afdb1_0;
				/* Acceptance Filter Data Byte0 Register */
				u8 afdb0_1;
				/* Acceptance Filter Data Byte1 Register */
				u8 afdb1_1;
			} byte;
			u32 all;
		} data;
		/* Acceptance Filter Control Register 0,1 */
		union {
			struct {
				u8 afc0; /* Acceptance Filter Control 0 Register */
				u8 afc1; /* Acceptance Filter Control 1 Register */
				u8 reserved2;
				u8 reserved3;
			} byte;
			u32 all;
		} control;
	} can_filter;

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * CAN_AFMIR0  |   FMBIH    |  FMBILEIH  |   FMEIM    |   FMEIL    | 0x0040
	 *             .....................................................
	 * CAN_AFMIR1  |   FMBIH    |  FMBILEIH  |   FMEIM    |   FMEIL    | 0x0044
	 *             .....................................................
	 * AFMR_Data   |   FMDB1_1  |   FMDB0_1  |   FMDB1_0  |   FMDB0_0  | 0x0048
	 *             .....................................................
	 * AFMR_Control│  RESERVED  |  RESERVED  |  RESERVED  |    FMC     | 0x004C
	 *             .....................................................
	 */
	struct {
		/* Acceptance Filter Mask Register 0,1 */
		union {
			struct {
				/* Filter Mask Extended Identifier Low Register */
				u8 fmeil;
				/* Filter Mask Extended Identifier Middle Register */
				u8 fmeim;
				/**
				 * Filter Mask Base Identifier Low and
				 * Extended Identifier High Register
				 */
				u8 fmbileih;
				/* Filter Mask Base Identifier High Register */
				u8 fmbih;
			} byte;
		u32 all;
		} identifier[2];
		/* Acceptance Filter Mask Data 0,1 Register 0,1 */
		union {
			struct {
				u8 fmdb0_0; /* Filter Mask Data Byte0 Register */
				u8 fmdb1_0; /* Filter Mask Data Byte1 Register */
				u8 fmdb0_1; /* Filter Mask Data Byte0 Register */
				u8 fmdb1_1; /* Filter Mask Data Byte1 Register */
			} byte;
			u32 all;
		} data;
		/* Acceptance Filter Mask Control Register */
		union {
			struct {
				u8 fmc; /* Filter Mask Control Register */
				u8 reserved1;
				u8 reserved2;
				u8 reserved3;
			} byte;
		u32 all;
		} control;
	} can_mask;

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * CAN_RR0     |    REIM    |    REIL    |   RREIE    |    RFD     | 0x0050
	 *             .....................................................
	 *             |  RESERVED  |    RBIH    |   RBILDL   |    REIH    | 0x0054
	 *             .....................................................
	 * CAN_RR1     |    REIM    |    REIL    |   RREIE    |    RFD     | 0x0058
	 *             .....................................................
	 *             |  RESERVED  |    RBIH    |   RBILDL   |    REIH    | 0x005C
	 *             .....................................................
	 */
	/* Received Register 0,1 */
	struct {
		union {
			struct {
				/* Received Flexible Data-Rate Register */
				u8 rfd;
				/* Received Remote and Extended Identifier Enable Register */
				u8 rreie;
				/* Received Extended Identifier Low Register */
				u8 reil;
				/* Received Extended Identifier Middle Register */
				u8 reim;
			} byte;
		u32 all;
		} rx_l;
		union {
			struct {
				/* Received Extended Identifier High Register */
				u8 reih;
				/* Received Base Identifier Low and Data Length Register */
				u8 rbildl;
				/* Received Base Identifier High Register */
				u8 rbih;
				u8 reserved3;
			} byte;
			u32 all;
		} rx_h;
	} can_rr[2];

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * CAN_NBTC    |   NBTC3    |   NBTC2    |   NBTC1    |   NBTC0    | 0x0060
	 *             .....................................................
	 *             |  RESERVED  |  RESERVED  |  RESERVED  |   NBTC4    | 0x0064
	 *             .....................................................
	 * CAN_DBTC    |   DBTC3    |   DBTC2    |   DBTC1    |   DBTC0    | 0x0068
	 *             .....................................................
	 */
	/* Bit Timing Register */
	struct {
		union {
			struct {
				/* Nominal Bit Timing Configuration 0 Register */
				u8 nbtc0;
				/* Nominal Bit Timing Configuration 1 Register */
				u8 nbtc1;
				/* Nominal Bit Timing Configuration 2 Register */
				u8 nbtc2;
				/* Nominal Bit Timing Configuration 3 Register */
				u8 nbtc3;
			} byte;
			u32 all;
		} nominal_l;
		union {
			struct {
				/* Nominal Bit Timing Configuration 4 Register */
				u8 nbtc4;
				u8 reserved1;
				u8 reserved2;
				u8 reserved3;
			} byte;
			u32 all;
		} nominal_h;
		union {
			struct {
				u8 dbtc0; /* Data Bit Timing Configuration 0 Register */
				u8 dbtc1; /* Data Bit Timing Configuration 1 Register */
				u8 dbtc2; /* Data Bit Timing Configuration 2 Register */
				u8 dbtc3; /* Data Bit Timing Configuration 3 Register */
			} byte;
			u32 all;
		} data;
	} can_bit_timing;

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * Err_Cntr    |    TECH    |    TECL    |    RECH    |    RECL    | 0x006C
	 *             .....................................................
	 * Err_Status  |  RESERVED  |  RESERVED  |    TED     |     ET     | 0x0070
	 *             .....................................................
	 */
	/* Error Monitor Register */
	struct {
		union {
			struct {
				u8 recl; /* Received Error Counter Low 8-bits Register */
				u8 rech; /* Received Error Counter High Register */
				u8 tecl; /* Transmit Error Counter Low Register */
				u8 tech; /* Transmit Error Counter High Register */
			} byte;
			u32 all;
		} counter;
		union {
			struct {
				u8 et; /* Error Type Register */
				/* Transmitter and Receive Error Detected Register */
				u8 ted;
				u8 reserved2;
				u8 reserved3;
			} byte;
			u32 all;
		} status;
	} can_error_monitor;

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * CAN_TS      |  RBTSH_1   |  RBTSL_1   |  RBTSH_0   |  RBTSL_0   | 0x0074
	 *             .....................................................
	 */
	/* Time Stamp Register 0,1 */
	union {
		struct {
			u8 rbtsl_0; /* Receive FIFO 0 Time Stamp low Register */
			u8 rbtsh_0; /* Receive FIFO 0 Time Stamp High Register */
			u8 rbtsl_1; /* Receive FIFO 1 Time Stamp low Register */
			u8 rbtsh_1; /* Receive FIFO 1 Time Stamp High Register */
		} byte;
		u32 all;
	} can_timestamp;

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * CAN_IRR     |  RESERVED  |  RESERVED  |  RESERVED  |     IR     | 0x0078
	 *             .....................................................
	 */
	/* Interrupt Request Register */
	union {
		struct {
			u8 ir; /* Interrupt Request Register */
			u8 reserved1;
			u8 reserved2;
			u8 reserved3;
		} byte;
		u32 all;
	} can_interrupt;

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * Tx0_Data0   |   BYTE_3   |   BYTE_2   |   BYTE_1   |   BYTE_0   | 0x007C
	 *             .....................................................
	 * Tx0_Data1   |   BYTE_7   |   BYTE_6   |   BYTE_5   |   BYTE_4   | 0x0080
	 *             .....................................................
	 *             |    ...     |    ...     |    ...     |    ...     |   ...
	 *             .....................................................
	 * Tx0_Data15  |   BYTE_63  |   BYTE_62  |   BYTE_61  |   BYTE_60  | 0x00B8
	 *             .....................................................
	 * Tx1_Data0   |   BYTE_3   |   BYTE_2   |   BYTE_1   |   BYTE_0   | 0x00BC
	 *             .....................................................
	 * Tx1_Data1   |   BYTE_7   |   BYTE_6   |   BYTE_5   |   BYTE_4   | 0x00C0
	 *             .....................................................
	 *             |    ...     |    ...     |    ...     |    ...     |   ...
	 *             .....................................................
	 * Tx1_Data15  |   BYTE_63  |   BYTE_62  |   BYTE_61  |   BYTE_60  | 0x00F8
	 *             .....................................................
	 * Tx2_Data0   |   BYTE_3   |   BYTE_2   |   BYTE_1   |   BYTE_0   | 0x00FC
	 *             .....................................................
	 * Tx2_Data1   |   BYTE_7   |   BYTE_6   |   BYTE_5   |   BYTE_4   | 0x0100
	 *             .....................................................
	 *             |    ...     |    ...     |    ...     |    ...     |   ...
	 *             .....................................................
	 * Tx2_Data15  |   BYTE_63  |   BYTE_62  |   BYTE_61  |   BYTE_60  | 0x0138
	 *             .....................................................
	 */
	/* Transmit Data Register x_n (x=0~63 and n=0,1,2) */
	struct {
		union {
			struct {
				u8 byte_0;
				u8 byte_1;
				u8 byte_2;
				u8 byte_3;
			} byte;
			u32 all;
		} data[16];
	} can_td[3];

	/**
	 *                 Byte 3       Byte 2       Byte 1       Byte 0     Offset
	 *             .....................................................
	 * Rx0_Data0   |   BYTE_3   |   BYTE_2   |   BYTE_1   |   BYTE_0   | 0x013C
	 *             .....................................................
	 * Rx0_Data1   |   BYTE_7   |   BYTE_6   |   BYTE_5   |   BYTE_4   | 0x0140
	 *             .....................................................
	 *             |    ...     |    ...     |    ...     |    ...     |   ...
	 *             .....................................................
	 * Rx0_Data15  |   BYTE_63  |   BYTE_62  |   BYTE_61  |   BYTE_60  | 0x0178
	 *             .....................................................
	 * Rx1_Data0   |   BYTE_3   |   BYTE_2   |   BYTE_1   |   BYTE_0   | 0x017C
	 *             .....................................................
	 * Rx1_Data1   |   BYTE_7   |   BYTE_6   |   BYTE_5   |   BYTE_4   | 0x0180
	 *             .....................................................
	 *             |    ...     |    ...     |    ...     |    ...     |   ...
	 *             .....................................................
	 * Rx1_Data15  |   BYTE_63  |   BYTE_62  |   BYTE_61  |   BYTE_60  | 0x01B8
	 *             .....................................................
	 */
	/* Received Data Register x_n (x=0~63 and n=0,1) */
	struct {
		union {
			struct {
				u8 byte_0;
				u8 byte_1;
				u8 byte_2;
				u8 byte_3;
			} byte;
			u32 all;
		} data[16];
	} can_rd[2];
};

/************** The definition of bit and field in CAN registers **************/
/* Bit definition for Control Enable 1 register (CE1)*/
#define  CAN_CE1_OMR_B       ((u8)0x03) /* Operation mode request */
#define  CAN_CE1_RT_B        ((u8)0x18) /* Retransmission */
#define  CAN_CE1_TOE_B       ((u8)0x20) /* Transmit overload enable */
#define  CAN_CE1_RR_B        ((u8)0x40) /* Reset request */
#define  CAN_CE1_OMR_W       ((u32)0x07000000)
#define  CAN_CE1_RT_W        ((u32)0x18000000)
#define  CAN_CE1_TOE_W       ((u32)0x20000000)
#define  CAN_CE1_RR_W        ((u32)0x40000000)

/* Bit definition for Control Enable 1 register (CE0)*/
#define  CAN_CE0_TSE_B       ((u8)0x80)     /* Time stamp enable */
#define  CAN_CE0_ENDB_B      ((u8)0x40)     /* Debounced enable */
#define  CAN_CE0_TSE_W       ((u32)0x00800000)
#define  CAN_CE0_ENDB_W      ((u32)0x00400000)

/* Bit definition for Interrupt Request Enable Register (IRE)*/
#define  CAN_IRE_RIE         ((u8)0x08) /* Receive interrupt enable */
#define  CAN_IRE_TIE         ((u8)0x10) /* Transmit interrupt enable */
#define  CAN_IRE_EIE         ((u8)0x20) /* Error interrupt enable */
#define  CAN_IRE_OIE         ((u8)0x40) /* Overrun interrupt enable */
#define  CAN_IRE_WIE         ((u8)0x80) /* Wake-up interrupt enable */
#define  CAN_IRE_RIE_W       ((u32)0x00000800)
#define  CAN_IRE_TIE_W       ((u32)0x00001000)
#define  CAN_IRE_EIE_W       ((u32)0x00002000)
#define  CAN_IRE_OIE_W       ((u32)0x00004000)
#define  CAN_IRE_WIE_W       ((u32)0x00008000)

#define  CAN_IR_MASK_ALL_W   (CAN_IRE_OIE_W | CAN_IRE_EIE_W | \
			      CAN_IRE_TIE_W | CAN_IRE_RIE_W)

/* Bit definition for Command Request Register (CR)*/
#define  CAN_CR_RRBA1_B      ((u8)0x02)     /* Release receive buffer1 all */
#define  CAN_CR_RRBA0_B      ((u8)0x04)     /* Release receive buffer0 all */
#define  CAN_CR_BTR2_B       ((u8)0x08)     /* Buffer2 transmission request */
#define  CAN_CR_BTR1_B       ((u8)0x10)     /* Buffer1 transmission request */
#define  CAN_CR_BTR0_B       ((u8)0x20)     /* Buffer0 transmission request */
#define  CAN_CR_RRB1_B       ((u8)0x40)     /* Release receive buffer1 */
#define  CAN_CR_RRB0_B       ((u8)0x80)     /* Release receive buffer0 */
#define  CAN_CR_RRBA1_W      ((u32)0x00000002) /* Release receive buffer1 all */
#define  CAN_CR_RRBA0_W      ((u32)0x00000004) /* Release receive buffer0 all */
#define  CAN_CR_BTR2_W       ((u32)0x00000008)
#define  CAN_CR_BTR1_W       ((u32)0x00000010)
#define  CAN_CR_BTR0_W       ((u32)0x00000020)
#define  CAN_CR_RRB1_W       ((u32)0x00000040)
#define  CAN_CR_RRB0_W       ((u32)0x00000080)
#define  CAN_CR_BTR_ALL_W    (CAN_CR_BTR0_W | CAN_CR_BTR1_W | CAN_CR_BTR2_W)


/* Bit definition for Transmit Status Register (TS)*/
#define  CAN_TS_BTS          ((u8)0x03) /* Buffer transmit status */
#define  CAN_TS_BTLA2        ((u8)0x04) /* Buffer2 transmit lost arbitration */
#define  CAN_TS_BTLA1        ((u8)0x08) /* Buffer1 transmit lost arbitration */
#define  CAN_TS_BTLA0        ((u8)0x10) /* Buffer0 transmit lost arbitration */
#define  CAN_TS_BTA2         ((u8)0x20) /* Buffer2 transmit abort */
#define  CAN_TS_BTA1         ((u8)0x40) /* Buffer1 transmit abort */
#define  CAN_TS_BTA0         ((u8)0x80) /* Buffer0 transmit abort */
#define  CAN_TS_MASK_W     ((u32)0x00FF0000) /* Buffer0 transmit status mask */
#define  CAN_TS_SHIFT        ((u8)16)   /* Buffer0 transmit status shift */


/* Bit definition for Transmit and Receive Buffer Status Register (TRBS)*/
#define  CAN_TRBS_BRS1_B     ((u8)0x06) /* Buffer1 receive status */
#define  CAN_TRBS_BRS0_B     ((u8)0x18) /* Buffer0 receive status */
#define  CAN_TRBS_BRS1_W     ((u32)0x00000600)
#define  CAN_TRBS_BRS0_W     ((u32)0x00001800)
#define  CAN_TRBS_SHIFT      ((u8)8)   /* TRBS shift */


#define  CAN_TRBS_BRS0_SHIFT (11)
#define  CAN_TRBS_BRS1_SHIFT (9)

/* Bit definition for Bus Status Register (BS)*/
#define  CAN_BS_DO           ((u8)0x0C) /* Data overrun */
#define  CAN_BS_RS           ((u8)0x10) /* Receive status */
#define  CAN_BS_TS           ((u8)0x20) /* Transmit status */
#define  CAN_BS_EW           ((u8)0x40) /* Error warning */
#define  CAN_BS_BO           ((u8)0x80) /* Bus-Off */

#define  CAN_BS_DO_W         ((u32)0x0000000C) /* Data overrun */
#define  CAN_BS_RS_W         ((u32)0x00000010) /* Receive status */
#define  CAN_BS_TS_W         ((u32)0x00000020) /* Transmit status */
#define  CAN_BS_EW_W         ((u32)0x00000040) /* Error warning */
#define  CAN_BS_BO_W         ((u32)0x00000080) /* Bus-Off */


/* Bit definition for Transmit Base Identifier High Register (TBIH)*/
#define  CAN_TBIH            ((u8)0xFF) /* The high 8-bit of the base transmit identifier */

/* Bit definition for Transmit Base Identifier Low and Data Length Register (TBILDL)*/
#define  CAN_TBILDL_TDL      ((u8)0x0F) /* The transmit data payload length */
#define  CAN_TBILDL_TBIL     ((u8)0xE0) /* The low 3-bit of the base transmit identifier */

/* Bit definition for Transmit Extended Identifier High Register (TEIH)*/
#define  CAN_TEIH            ((u8)0xFF) /* The high 8-bit of the extended transmit identifier */

/* Bit definition for Transmit Extended Identifier Middle Register (TEIM)*/
#define  CAN_TEIM            ((u8)0xFF) /* The middle 8-bit of the extended transmit identifier */

/* Bit definition for Transmit Extended Identifier Low Register (TEIL)*/
#define  CAN_TEIL            ((u8)0xC0) /* The low 2-bit of the extended transmit identifier */

/* Bit definition for Transmit Remote and Extended Identifier Enable Register (TREIE)*/
#define  CAN_TREIE_TEIR      ((u8)0x40) /* Transmit extended  identifier request bit */
#define  CAN_TREIE_TRTR      ((u8)0x80) /* Transmit remote transmission request bit */

/* Bit definition for Transmit Flexible Data-Rate Register (TFD)*/
#define  CAN_TFD_TBRS        ((u8)0x40) /* The transmit bit rate switch bit */
#define  CAN_TFD_TEDL        ((u8)0x80) /* The transmit extended data length bit */

/* Bit definition for Acceptance Filter Base Identifier High Register (AFBIH)*/
/* The high 8-bit of the acceptance filter base received identifier */
#define  CAN_AFBIH           ((u8)0xFF)

/**
 * Bit definition for Acceptance Filter Base Identifier Low and
 * Extended Identifier High (AFBILEIH)
 */
/* The high 5-bit of the acceptance filter extended identifier */
#define  CAN_AFBILEIH_AFEIH  ((u8)0x1F)
/* The low 3-bit of the acceptance filter base identifier */
#define  CAN_AFBILEIH_AFBIL  ((u8)0xE0)

/* Bit definition for Acceptance Filter Extended Identifier Middle Register (AFEIM)*/
/* The middle 8-bit of the acceptance filter extended identifier */
#define  CAN_AFEIM           ((u8)0xFF)

/* Bit definition for Acceptance Filter Extended Identifier Low Register (AFEIL)*/
/* The low 5-bit of the acceptance filter extended identifier */
#define  CAN_AFEIL           ((u8)0xF8)

/**
 * Bit definition for Acceptance Filter register in 32-bit
 * (Combine the AFBIH, AFBILEIH, AFEIM, and AFEIL)
 */
#define  CAN_AFBI_W          ((u32)0xFFE00000)
#define  CAN_AFEI_W          ((u32)0x001FFFF8)

/* Bit definition for Acceptance Filter Data Byte0  Register (AFDB0)*/
#define  CAN_AFDB0           ((u8)0xFF)     /* The byte0 of the acceptance data filter */

/* Bit definition for Acceptance Filter Data Byte1  Register (AFDB1)*/
#define  CAN_AFDB1           ((u8)0xFF)     /* The byte1 of the acceptance data filter */

/* Bit definition for Acceptance Filter Data Byte 0 and 1 register in 32-bit */
#define  CAN_FIFO0_AFDB0_W   ((u32)0x000000FF)
#define  CAN_FIFO0_AFDB1_W   ((u32)0x0000FF00)
#define  CAN_FIFO1_AFDB0_W   ((u32)0x00FF0000)
#define  CAN_FIFO1_AFDB1_W   ((u32)0xFF000000)

/* Bit definition for Acceptance Filter Control Register 0 (AFC0)*/
#define  CAN_AFC0_RB1DFB1E_B ((u8)0x04)     /* Receive buffer1 data filter byte1 enable */
#define  CAN_AFC0_RB1DFB0E_B ((u8)0x08)     /* Receive buffer1 data filter byte0 enable */
#define  CAN_AFC0_RB0DFB1E_B ((u8)0x10)     /* Receive buffer0 data filter byte1 enable */
#define  CAN_AFC0_RB0DFB0E_B ((u8)0x20)     /* Receive buffer0 data filter byte0 enable */
#define  CAN_AFC0_RBAFG1E_B  ((u8)0x40)     /* Receive buffer1 acceptance filter group enable */
#define  CAN_AFC0_RBAFG0E_B  ((u8)0x80)     /* Receive buffer0 acceptance filter group enable */
#define  CAN_AFC0_RB1DFB1E_W ((u32)0x00000004)
#define  CAN_AFC0_RB1DFB0E_W ((u32)0x00000008)
#define  CAN_AFC0_RB0DFB1E_W ((u32)0x00000010)
#define  CAN_AFC0_RB0DFB0E_W ((u32)0x00000020)
#define  CAN_AFC0_RBAFG1E_W  ((u32)0x00000040)
#define  CAN_AFC0_RBAFG0E_W  ((u32)0x00000080)

/* Bit definition for Acceptance Filter Control Register 1 (AFC1)*/
#define  CAN_AFC1_AFT2_B     ((u8)0x0C)     /* Acceptance frame type 2 */
#define  CAN_AFC1_AFT1_B     ((u8)0x30)     /* Acceptance frame type 1 */
#define  CAN_AFC1_AFT0_B     ((u8)0xC0)     /* Acceptance frame type 0 */
#define  CAN_AFC1_AFT2_W     ((u32)0x00000C00)
#define  CAN_AFC1_AFT1_W     ((u32)0x00003000)
#define  CAN_AFC1_AFT0_W     ((u32)0x0000C000)
#define  CAN_AFC1_AFT_W      ((u32)0x0000FC00)

/* Bit definition for Filter Mask Base Identifier High Register (FMBIH)*/
/* Mask bits for the high 8-bit of the acceptance filter base received identifier */
#define  CAN_FMBIH           ((u8)0xFF)

/**
 * Bit definition for Filter Mask Base Identifier Low and
 * Extended Identifier High Register (FMBILEIH)
 */
/* Mask bits for the high 5-bit of the acceptance filter extended identifier */
#define  CAN_FMBILEIH_FMEIH  ((u8)0x1F)
/* Mask bits for the low 3-bit of the acceptance filter base identifier */
#define  CAN_FMBILEIH_FMBIL  ((u8)0xE0)

/* Bit definition for Filter Mask Extended Identifier Middle Register (FMEIM) */
/* Mask bits for the middle 8-bit of the acceptance filter extended identifier */
#define  CAN_FMBIH           ((u8)0xFF)

/* Bit definition for Filter Mask Extended Identifier Low Register (FMEIL) */
/* Mask bits for the low 5-bit of the acceptance filter extended identifier */
#define  CAN_FMEIL           ((u8)0xF8)

/**
 * Bit definition for Filter Mask register in 32-bit
 * (Combine the FMBIH, FMBILEIH, FMEIM, and FMEIL)
 */
#define  CAN_FMBI_W          ((u32)0xFFE00000)
#define  CAN_FMEI_W          ((u32)0x001FFFF8)

/* Bit definition for Filter Mask Data Byte0 Register (FMDB0)*/
#define  CAN_FMDB0           ((u8)0xFF)     /* Mask bits for the acceptance data byte 0 filter */

/* Bit definition for Filter Mask Data Byte1 Register (FMDB1)*/
#define  CAN_FMDB1           ((u8)0xFF)     /* Mask bits for the acceptance data byte 1 filter */

/* Bit definition for Filter Mask Data Byte 0 and 1 register in 32-bit */
#define  CAN_FIFO0_FMDB0_W   ((u32)0x000000FF)
#define  CAN_FIFO0_FMDB1_W   ((u32)0x0000FF00)
#define  CAN_FIFO1_FMDB0_W   ((u32)0x00FF0000)
#define  CAN_FIFO1_FMDB1_W   ((u32)0xFF000000)

/* Bit definition for Filter Mask Control Register (FMC)*/
#define  CAN_FMC_RB1DFMB1E   ((u8)0x02)     /* Receive buffer1 data filter mask byte1 enable */
#define  CAN_FMC_RB1DFMB0E   ((u8)0x04)     /* Receive buffer1 data filter mask byte0 enable */
#define  CAN_FMC_RB0DFMB1E   ((u8)0x08)     /* Receive buffer0 data filter mask byte1 enable */
#define  CAN_FMC_RB0DFMB0E   ((u8)0x10)     /* Receive buffer0 data filter mask byte0 enable */
#define  CAN_FMC_RBAFM1E     ((u8)0x20)     /* Receive buffer1 acceptance filter mask enable */
#define  CAN_FMC_RBAFM0E     ((u8)0x40)     /* Receive buffer0 acceptance filter mask enable */
#define  CAN_FMC_FMA         ((u8)0x80)     /* Filter mask all */
#define  CAN_FMC_RB1DFMB1E_W ((u32)0x00000002)
#define  CAN_FMC_RB1DFMB0E_W ((u32)0x00000004)
#define  CAN_FMC_RB0DFMB1E_W ((u32)0x00000008)
#define  CAN_FMC_RB0DFMB0E_W ((u32)0x00000010)
#define  CAN_FMC_RBAFM1E_W   ((u32)0x00000020)
#define  CAN_FMC_RBAFM0E_W   ((u32)0x00000040)
#define  CAN_FMC_FMA_W       ((u32)0x00000080)

/* Bit definition for Received Base Identifier High Register (RBIH)*/
#define  CAN_RBIH            ((u8)0xFF)     /* The high 8-bit of the base received identifier */

/* Bit definition for Received Base Identifier Low and Data Length Register (RBILDL)*/
#define  CAN_RBILDL_RBIL     ((u8)0xE0)     /* The low 3-bit of the base received identifier */
#define  CAN_RBILDL_RDL_B    ((u8)0x0F)     /* The received data payload length */
#define  CAN_RBILDL_RDL_W    ((u32)0x00000F00)
#define  CAN_RBILDL_RDL_SHIFT (8)

/* Bit definition for Received Extended Identifier High Register (REIH)*/
#define  CAN_REIH            ((u8)0xFF) /* The high 8-bit of the extended received identifier */

/* Bit definition for Received Extended Identifier Middle Register (REIM)*/
#define  CAN_REIM            ((u8)0xFF) /* The middle 8-bit of the received extended identifier */

/* Bit definition for Received Extended Identifier Low Register (REIL)*/
#define  CAN_REIL            ((u8)0xC0) /* The low 2-bit of the received extended identifier */

/* Bit definition for Received Extended and Based Identifier register in 32-bit */
#define  CAN_RBI_W           ((u32)0x00FFE000)
#define  CAN_REIH_W          ((u32)0x000000FF)
#define  CAN_REIL_W          ((u32)0xFFC00000)
#define  CAN_RBI_SHIFT       (13)
#define  CAN_REIH_SHIFT      (10)
#define  CAN_REIL_SHIFT      (22)


/* Bit definition for Received Remote and Extended Identifier Enable Register (RREIE)*/
#define  CAN_RREIE_REIE_B    ((u8)0x40)     /* Received extended identifier enable bit */
#define  CAN_RREIE_RRTR_B    ((u8)0x80)     /* Received remote transmission request bit */
#define  CAN_RREIE_REIE_W    ((u32)0x00004000)
#define  CAN_RREIE_RRTR_W    ((u32)0x00008000)

/* Bit definition for Received Flexible Data-Rate Register (RFD)*/
#define  CAN_RFD_RESI_B      ((u8)0x20)     /* The received error state indicator bit */
#define  CAN_RFD_RBRS_B      ((u8)0x40)     /* The received bit rate switch bit */
#define  CAN_RFD_REDL_B      ((u8)0x80)     /* The received extended data length bit */
#define  CAN_RFD_RESI_W      ((u32)0x00000020)
#define  CAN_RFD_RBRS_W      ((u32)0x00000040)
#define  CAN_RFD_REDL_W      ((u32)0x00000080)

/* Bit definition for Nominal Bit Timing Configuration 0 Register (NBTC0)*/
#define  CAN_NBTC0_NSJW      ((u8)0xF8) /* The nominal synchronization jump width length bits */

/* Bit definition for Nominal Bit Timing Configuration 1 Register (NBTC1)*/
#define  CAN_NBTC1_NBRP      ((u8)0xFF)     /* Nominal baud rate prescaler */

/* Bit definition for Nominal Bit Timing Configuration 2 Register (NBTC2)*/
#define  CAN_NBTC2_NPRSEG    ((u8)0x7E)     /* The bits of nominal Prop_seg */

/* Bit definition for Nominal Bit Timing Configuration 3 Register (NBTC3)*/
#define  CAN_NBTC3_NPSEG1    ((u8)0xF8)     /* The bits of nominal Phase_seg1 */

/* Bit definition for Nominal Bit Timing Configuration 4 Register (NBTC4)*/
#define  CAN_NBTC4_NPSEG2    ((u8)0xF8)     /* The bits of nominal Phase_seg2 */

/* Bit definition for Data Bit Timing Configuration 0 Register (DBTC0)*/
#define  CAN_DBTC0_DBRP      ((u8)0xFF)     /* Data baud rate prescaler */

/* Bit definition for Data Bit Timing Configuration 1 Register (DBTC1)*/
#define  CAN_DBTC1_DPRSEG    ((u8)0x78)     /* The bits of data Prop_seg */
#define  CAN_DBTC1_DSJW      ((u8)0x06)     /* The bits of data SJW */

/* Bit definition for Data Bit Timing Configuration 2 Register (DBTC2)*/
#define  CAN_DBTC2_DPSEG1    ((u8)0xE0)     /* The bits of data Phase_seg1 */

/* Bit definition for Data Bit Timing Configuration 3 Register (DBTC2)*/
#define  CAN_DBTC3_DPSEG2    ((u8)0xE0)     /* The bits of data Phase_seg2 */

/* Bit definition for Transmit and Receive Error Counter Register (TED)*/
#define  CAN_TED_BRE1        ((u8)0x08)     /* Buffer1 Receive error */
#define  CAN_TED_BRE0        ((u8)0x10)     /* Buffer0 Receive error */
#define  CAN_TED_BTE2        ((u8)0x20)     /* Buffer2 Transmitter error */
#define  CAN_TED_BTE1        ((u8)0x40)     /* Buffer1 Transmitter error */
#define  CAN_TED_BTE0        ((u8)0x80)     /* Buffer0 Transmitter error */
#define  CAN_TED_MSAK_W      ((u32)0x0000FF00) /*Transmitter and receive mask*/
#define  CAN_TED_SHIFT       ((u8)8)

/* Bit definition for Transmit Error Counter High Register (TECH)*/
#define  CAN_TECH            ((u8)0xFF)     /* Transmitter error counter high 8-bit register */

/* Bit definition for Transmit Error Counter Low Register (TECL)*/
#define  CAN_TECL            ((u8)0xFF)     /* Transmitter error counter low 8-bit register */

/* Bit definition for Received Error Counter High Register (RECH)*/
#define  CAN_RECH            ((u8)0xFF)     /* Receiver error counter high 8-bits register */

/* Bit definition for Received Error Counter Low Register (RECL)*/
#define  CAN_RECL            ((u8)0xFF)     /* Receiver error counter low 8-bits register */

/* Bit definition for Error Type Register (ET)*/
#define  CAN_ET_OE           ((u8)0x04)     /* Overrun Error */
#define  CAN_ET_AE           ((u8)0x08)     /* Acknowledgment error */
#define  CAN_ET_FE           ((u8)0x10)     /* Form error */
#define  CAN_ET_CE           ((u8)0x20)     /* CRC error */
#define  CAN_ET_SE           ((u8)0x40)     /* Stuff error */
#define  CAN_ET_BE           ((u8)0x80)     /* Bit error */
#define  CAN_ET_MASK_W       ((u32)0x000000FF) /* Error mask */


/* Bit definition for Receive Buffer Time Stamp High Register(RBTSH)*/
#define  CAN_RBTSH           ((u8)0xFF)     /* Receive buffer time stamp high 8-bit */

/* Bit definition for Receive Buffer Time Stamp Low Register(RBTSL)*/
#define  CAN_RBTSL           ((u8)0xFF)     /* Receive buffer time stamp low 8-bit */

/* Bit definition for Interrupt Request Register (IR)*/
#define  CAN_IR_RB           ((u8)0x03)     /* Receive buffer field */
#define  CAN_IR_TB           ((u8)0x1C)     /* Transmit buffer field */
#define  CAN_IR_RBI1         ((u8)0x01)     /* Receive buffer1 interrupt */
#define  CAN_IR_RBI0         ((u8)0x02)     /* Receive buffer0 interrupt */
#define  CAN_IR_TBI2         ((u8)0x04)     /* Transmit buffer2 interrupt */
#define  CAN_IR_TBI1         ((u8)0x08)     /* Transmit buffer1 interrupt */
#define  CAN_IR_TBI0         ((u8)0x10)     /* Transmit buffer0 interrupt */
#define  CAN_IR_EIR          ((u8)0x20)     /* Error interrupt request */
#define  CAN_IR_OIR          ((u8)0x40)     /* Overrun interrupt request */
#define  CAN_IR_WIR          ((u8)0x80)     /* Wake-up interrupt request */

/* nominal bit timing configuration shift*/
#define CAN_NSJW_SHIFT       ((u8)3)        /* Synchronous jump width */
#define CAN_NBRP_SHIFT       ((u8)8)        /* Baud rate prescaler */
#define CAN_NPROP_SHIFT      ((u8)17)       /* Propagation time segment */
#define CAN_NPS1_SHIFT       ((u8)27)       /* Phase buffer segment 1 */
#define CAN_NPS2_SHIFT       ((u8)3)        /* Phase buffer segment 2 */

/* data bit timing configuration shift */
#define CAN_DSJW_SHIFT       ((u8)9)        /* Synchronous jump width */
#define CAN_DBRP_SHIFT       ((u8)0)        /* Baud rate prescaler */
#define CAN_DPROP_SHIFT      ((u8)11)       /* Propagation time segment */
#define CAN_DPS1_SHIFT       ((u8)21)       /* Phase buffer segment 1 */
#define CAN_DPS2_SHIFT       ((u8)29)       /* Phase buffer segment 2 */

/* CAN Modes */
#define CAN_MODE_CONFIG    ((u32)0x00000000)  /* Configuration mode */
#define CAN_MODE_NORMAL    ((u32)0x01000000)  /* Normal mode */
#define CAN_MODE_SLEEP     ((u32)0x02000000)  /* Sleep mode */
#define CAN_MODE_LISTEN    ((u32)0x03000000)  /* Listen mode */
#define CAN_MODE_LOOPBACK  ((u32)0x04000000)  /* Loopback mode */

/* Rx FIFO number */
#define CAN_RXFIFO_0       ((u8)0x0)
#define CAN_RXFIFO_1       ((u8)0x1)
#define CAN_RXFIFO_ALL     ((u8)0x2)

/* Tx buffer number */
#define CAN_TXBUFFER_0     ((s8)0x0)
#define CAN_TXBUFFER_1     ((s8)0x1)
#define CAN_TXBUFFER_2     ((s8)0x2)
#define CAN_TXBUFFER_ALL   ((s8)0x3)

/* Retransmission option */
//Always retransmit the message if the previous transmission is failed
#define CAN_RT_ALWAYS      ((u32)0x0)
#define CAN_RT_1TIME       ((u32)0x1)  //Only transmission one time whether is successful or not
#define CAN_RT_3TIMES      ((u32)0x2)  //Retransmission three times
#define CAN_RT_8TIMES      ((u32)0x3)  //Retransmission eight times

/* Bit in Frame */
#define CAN_RT_BASED       ((u8)0x0)  //Standard Identifier
#define CAN_ID_EXTENDED    ((u8)0x1)  //Extended Identifier
#define CAN_DATA_FRAME     ((u8)0x0)  //Data frame and RTR = 0
#define CAN_REMOTE_FRAME   ((u8)0x1)  //Remote frame and RTR = 1

/* Data Length Code */
#define CAN_DATABYTES_0    ((u8)0x00)
#define CAN_DATABYTES_1    ((u8)0x01)
#define CAN_DATABYTES_2    ((u8)0x02)
#define CAN_DATABYTES_3    ((u8)0x03)
#define CAN_DATABYTES_4    ((u8)0x04)
#define CAN_DATABYTES_5    ((u8)0x05)
#define CAN_DATABYTES_6    ((u8)0x06)
#define CAN_DATABYTES_7    ((u8)0x07)
#define CAN_DATABYTES_8    ((u8)0x08)
#define CAN_DATABYTES_12   ((u8)0x09)
#define CAN_DATABYTES_16   ((u8)0x0a)
#define CAN_DATABYTES_20   ((u8)0x0b)
#define CAN_DATABYTES_24   ((u8)0x0c)
#define CAN_DATABYTES_32   ((u8)0x0d)
#define CAN_DATABYTES_48   ((u8)0x0e)
#define CAN_DATABYTES_64   ((u8)0x0f)


/**				Identifier marcos
 *
 * | - - - - - - - - - - - - - - Arbitraion field - - - - - - - - - - - - - - - - |
 * |                          |   |   |                                           |
 * |.Based ID field (11 bits).|SRR|IDE|.....Extended ID field (29-11=18 bits).....|
 * |--------- 8 --------|- 3 -| 1 - 1 |-------- 8 --------|------- 8 -------|- 2 -|
 * |------- TBIH -------|TBIL-|       |------- TEIH ------|------ TEIM -----|TEIL-|
 *
 * Input id is 32-bit
 * These define macros can separate the id into several segments.
 */
#define CAN_BASEDID_H(StID)       ((StID & 0x7F8) >> 3)    //TBIH
#define CAN_BASEDID_L(StID)       ((StID & 0x7))           //TBIL
#define CAN_EXTENDEDID_H(ExID)    ((ExID & 0x3FC00) >> 10) //TEIH
#define CAN_EXTENDEDID_M(ExID)    ((ExID & 0x3FC) >> 2)    //TEIM
#define CAN_EXTENDEDID_L(ExID)    ((ExID & 0x3) << 6)      //TEIL
#define CAN_BASEDID(TBIH, TBIL)   ((TBIH << 3) | TBIL)
#define CAN_EXTENDEDID(TEIH, TEIM, TEIL)    ((TEIH << 10) | (TEIM << 2) | TEIL)

/* Acceptance frame type (AFC register)*/
#define CAN_ALL                     ((u8)0x00)
#define CAN_ONLYNONFD               ((u8)0x04)
#define CAN_ONLYFD                  ((u8)0x08)
#define CAN_ONLYDATA                ((u8)0x10)
#define CAN_ONLYREMOTE              ((u8)0x20)
#define CAN_ONLYBASED               ((u8)0x40)
#define CAN_ONLYEXTENDED            ((u8)0x80)
#define CAN_NONFD_DATA              ((u8)0x14)
#define CAN_NONFD_REMOTE            ((u8)0x24)
#define CAN_NONFD_BASED             ((u8)0x44)
#define CAN_NONFD_BASED_DATA        ((u8)0x54)
#define CAN_NONFD_BASED_REMOTE      ((u8)0x64)
#define CAN_NONFD_EXTENDED          ((u8)0x84)
#define CAN_NONFD_EXTENDED_DATA     ((u8)0x94)
#define CAN_NONFD_EXTENDED_REMOTE   ((u8)0xA4)
#define CAN_FD_DATA                 ((u8)0x18)
#define CAN_FD_BASED                ((u8)0x48)
#define CAN_FD_BASED_DATA           ((u8)0x58)
#define CAN_FD_EXTENDED             ((u8)0x88)
#define CAN_FD_EXTENDED_DATA        ((u8)0x98)

#define CAN_TXTIMESTAMP(BYTE1, BYTE0)    ((BYTE1 << 8) | BYTE0)
#define CAN_RXTIMESTAMP(RBTSH, RBTSL)    ((RBTSH << 8) | RBTSL)

#define CAN_TEC(TECH, TECL)    ((TECH << 8) | TECL)
#define CAN_REC(RECH, RECL)    ((RECH << 8) | RECL)

/* CAN_STATUS_BIT */
#define CAN_BTA0_STATUS_BIT    ((u8)0x1C)
#define CAN_BTA1_STATUS_BIT    ((u8)0x18)
#define CAN_BTA2_STATUS_BIT    ((u8)0x14)
#define CAN_BTLA0_STATUS_BIT   ((u8)0x10)
#define CAN_BTLA1_STATUS_BIT   ((u8)0x0C)
#define CAN_BTLA2_STATUS_BIT   ((u8)0x08)
#define CAN_BTS_STATUS_BIT     ((u8)0x20)
#define CAN_BTS0_STATUS_BIT    ((u8)0x1D)
#define CAN_BTS1_STATUS_BIT    ((u8)0x19)
#define CAN_BTS2_STATUS_BIT    ((u8)0x0D)
#define CAN_BRS0_STATUS_BIT    ((u8)0x2D)
#define CAN_BRS1_STATUS_BIT    ((u8)0x25)
#define CAN_BO_STATUS_BIT      ((u8)0x1E)
#define CAN_EW_STATUS_BIT      ((u8)0x1A)
#define CAN_TS_STATUS_BIT      ((u8)0x16)
#define CAN_RS_STATUS_BIT      ((u8)0x12)
#define CAN_TC_STATUS_BIT      ((u8)0x0E)
#define CAN_RC_STATUS_BIT      ((u8)0x0A)
#define CAN_DO_STATUS_BIT      ((u8)0x2A)
#define CAN_WIR_STATUS_BIT     ((u8)0x1F)
#define CAN_OIR_STATUS_BIT     ((u8)0x1B)
#define CAN_EIR_STATUS_BIT     ((u8)0x17)
#define CAN_TBI0_STATUS_BIT    ((u8)0x13)
#define CAN_TBI1_STATUS_BIT    ((u8)0x0F)
#define CAN_TBI2_STATUS_BIT    ((u8)0x0B)
#define CAN_RBI0_STATUS_BIT    ((u8)0x07)
#define CAN_RBI1_STATUS_BIT    ((u8)0x03)

#define FTCAN_TIMEOUT          (1 * HZ)
#define DEFAULT_STATUS         ((u8)0xFF)
#define RX_BUFFER0_DEPTH       ((u32)3)
#define RX_BUFFER1_DEPTH       ((u32)3)
#define LOTUS_CAN_SET_MODE_RETRIES ((u32)255)
#define DRIVER_NAME            "ftcan"
#define CRG_ADDR_BASE          ((phys_addr_t)0x120101BC)
#define CRG_OFFESET_SIZE       (4)
#define CRG_CAN_OFFESET        (0x1BC)
#define FTCAN_CLK_24M          ((u32)(24 * 1000 * 1000))
#define FTCAN_CLK_99M          ((u32)(100 * 1000 * 1000))

enum ftcan_ip_type {
	FT_CAN = 0,
	FT_CANFD,
};

struct ftcan_devtype_data {
	enum ftcan_ip_type cantype;
	u32 flags;
	const struct can_bittiming_const *bittiming_const;
	const struct can_bittiming_const *data_bittiming_const;
};

/**
 * struct ftcan_priv - This definition define CAN driver instance
 * @can:			CAN private data structure.
 * @tx_lock:			Lock for synchronizing TX interrupt handling
 * @tx_max:			Maximum number packets the driver can send
 * @napi:			NAPI structure
 * @dev:			Network device data structure
 * @reg_base:			Ioremapped address to registers
 * @irq_flags:			For request_irq()
 * @devtype:			Device type specific constants
 * @tx_buffer			Pointer to three TX buffers
 */
struct ftcan_priv {
	struct can_priv can;
	spinlock_t tx_lock; /* Lock for synchronizing TX interrupt handling */
	u32 tx_max;
#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
	struct napi_struct napi_rx0;
#ifdef CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT
	struct napi_struct napi_rx1;
#endif /* CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT */
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */
	struct device *dev;
	struct can_regs __iomem *reg_base;
	unsigned long irq_flags;
	struct ftcan_devtype_data devtype;
	/**
	 * Three transmit buffers with prioritization
	 * buffer0 transmission request has the highest priority to transmit.
	 * buffer1 transmission request has the middle priority to transmit.
	 * buffer2 transmission request has the lowest priority to transmit.
	 */
	struct sk_buff *tx_buffer[3];
	u32 retransmit_times;
};
#endif /* __FTCAN_H */
