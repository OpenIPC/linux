/*
    @file       h26x_reg.h
    @ingroup    mIH26X

    @brief      Header file for H264 module register.
                This file is the header file that define register for H264 module.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/

/*
    @addtogroup mIH264
*/

#ifndef _H26X_REG_H_
#define _H26X_REG_H_

#include "h26x.h"

#define APB_TYPE					98560
//#define IOADDR_H26X_REG_BASE		IOADDR_H26X_REG_BASE

#define H26X_SWRST					0x00000010
#define H26X_START					0x00000100
#define H26X_LOAD					0x80000000
#define H26X_INIT_INT_STATUS		0x01101711

// CTRL bit set //
#define H26X_SWRST_BIT				4
#define H26X_START_BIT				8
#define H26X_BUSY_BIT				12
#define H26X_DRM_SEL_BIT			16
#define H26X_BSDMA_CMD_EN_BIT			20
#define H26X_BSOUT_EN_BIT			24
#define H26X_DMA_CH_DIS_BIT			27
#define H26X_CHKSUM_EN_BIT			28
#define H26X_CLOCK_EN_BIT	        29
#define H26X_CHK_SUN_MUX_EN_BIT		30
#define H26X_PCLOCK_EN_BIT	        31

typedef struct _H26X_HW_REG{
    volatile UINT32 CTRL;                       // 0x000
    volatile UINT32 INT_FLAG;                   // 0x004
    volatile UINT32 SRAM_PW_CFG;				// 0x008
    volatile UINT32 HW_VERSION;                 // 0x00C
	volatile UINT32 LLC_CFG_0;					// 0x010
	volatile UINT32 LLC_CFG_1;					// 0x014
	volatile UINT32 LLC_CFG_2;					// 0x018
	volatile UINT32 INT_EN;						// 0x01C
	volatile UINT32 DEC0_CFG;					// 0x020
	volatile UINT32 SRC_Y_ADDR;					// 0x024
	volatile UINT32 SRC_C_ADDR;					// 0x028
	volatile UINT32 REC_Y_ADDR;					// 0x02C
	volatile UINT32 REC_C_ADDR;					// 0x030
	volatile UINT32 REF_Y_ADDR;					// 0x034
	volatile UINT32 REF_C_ADDR;					// 0x038
	volatile UINT32 TNR_OUT_Y_ADDR;				// 0x03C
	volatile UINT32 TNR_OUT_C_ADDR;				// 0x040
	volatile UINT32 COL_MVS_WR_ADDR;			// 0x044
	volatile UINT32 COL_MVS_RD_ADDR;			// 0x048
	volatile UINT32 SIDE_INFO_WR_ADDR_0;		// 0x04C
	volatile UINT32 RES_050;					// 0x050
	volatile UINT32 SIDE_INFO_RD_ADDR_0;		// 0x054
	volatile UINT32 RES_058;					// 0x058
	volatile UINT32 RRC_WR_ADDR;				// 0x05C
	volatile UINT32 RRC_RD_ADDR;				// 0x060
	volatile UINT32 QP_MAP_ADDR;				// 0x064
	volatile UINT32 NAL_LEN_OUT_ADDR;			// 0x068
	volatile UINT32 BSDMA_CMD_BUF_ADDR;			// 0x06C
	volatile UINT32 BSOUT_BUF_ADDR;				// 0x070
	volatile UINT32 RES_074;					// 0x074
	volatile UINT32 TIMEOUT_CNT_MAX;			// 0x078
	volatile UINT32 BSOUT_BUF_SIZE;				// 0x07C
	volatile UINT32 NAL_HDR_LEN_TOTAL_LEN;		// 0x080
	volatile UINT32 SRC_LINE_OFFSET;			// 0x084
	volatile UINT32 REC_LINE_OFFSET;			// 0x088
	volatile UINT32 TNR_OUT_LINE_OFFSET;		// 0x08C
	volatile UINT32 SIDE_INFO_LINE_OFFSET;		// 0x090
	volatile UINT32 SIDE_INFO_LINE_OFFSET2;		// 0x094
	volatile UINT32 SIDE_INFO_LINE_OFFSET3;		// 0x098
	volatile UINT32 QP_MAP_LINE_OFFSET;			// 0x09C
	volatile UINT32 FUNC_CFG[2];				// 0x0A0 ~ 0x0A4
	volatile UINT32 SEQ_CFG[2];					// 0x0A8 ~ 0x0AC
	volatile UINT32 RDO_CFG_4;					// 0x0B0
	volatile UINT32 PIC_CFG;					// 0x0B4
	volatile UINT32 QP_CFG[2];					// 0x0B8 ~ 0x0BC
	volatile UINT32 ILF_CFG[2];					// 0x0C0 ~ 0x0C4
	volatile UINT32 AEAD_CFG;					// 0x0C8
	volatile UINT32 DSF_CFG;					// 0x0CC
	volatile UINT32 GDR_CFG[2];					// 0x0D0 ~ 0x0D4
	volatile UINT32 FRO_CFG[21];				// 0x0D8 ~ 0x128
	volatile UINT32 RRC_CFG[11];				// 0x12C ~ 0x154
	volatile UINT32 SRAQ_CFG[17];				// 0x158 ~ 0x198
	volatile UINT32 LPM_CFG;					// 0x19C
	volatile UINT32 RMD_CFG;					// 0x1A0
	volatile UINT32 RND_CFG[2];					// 0x1A4 ~ 0x1A8
	volatile UINT32 VAR_CFG[2];					// 0x1AC ~ 0x1B0
	volatile UINT32 IME_CFG;					// 0x1B4
	volatile UINT32 SCD_CFG;					// 0x1B8
	volatile UINT32 DEC1_CFG;					// 0x1BC
	volatile UINT32 ROI_CFG[21];				// 0x1C0 ~ 0x210
	volatile UINT32 RDO_CFG[4];					// 0x214 ~ 0x220
    volatile UINT32 MAQ_CFG[2];					// 0x224 ~ 0x228
    volatile UINT32 RES_22C;					// 0x22C
    volatile UINT32 MOTION_BIT_ADDR[3];			// 0x230 ~ 0x238
    volatile UINT32 DBG_CFG;					// 0x23C
    volatile UINT32 DBG_REPORT[3];				// 0x240 ~ 0x248
    volatile UINT32 DEB_CFG;					// 0x24C
    volatile UINT32 CHK_REPORT[62];			    // 0x250 ~ 0x344
    volatile UINT32 ESKIP_CFG[2];			    // 0x348 ~ 0x34C
    volatile UINT32 TILE_CFG[27];            	// 0x350 ~ 0x3B8
    volatile UINT32 RRC_CFG_11;      			// 0x3BC
	volatile UINT32 RMD_CFG_1;		        	// 0x3C0
	volatile UINT32 RDO_CFG_5;					// 0x3C4
    volatile UINT32 CHK_REPORT_2[11];			// 0x3C8 ~ 0x3F0
    volatile UINT32 RES_3F4_3FC[3];			    // 0x3F4 ~ 0x3FC
    volatile UINT32 OSG_CFG[309];			    // 0x400 ~ 0x8D0
    volatile UINT32 RES_8D4;					// 0x8D4
    volatile UINT32 NAL_REPORT[2];				// 0x8D8 ~ 0x8DC
    volatile UINT32 RES_8E0_8FC[8];				// 0x8D4 ~ 0x8FC
    volatile UINT32 SDE_CFG[46];				// 0x900 ~ 0x9B4
    volatile UINT32 SPN_CFG[11];				// 0x9B8 ~ 0x9E0
}H26X_HW_REG;

#endif
