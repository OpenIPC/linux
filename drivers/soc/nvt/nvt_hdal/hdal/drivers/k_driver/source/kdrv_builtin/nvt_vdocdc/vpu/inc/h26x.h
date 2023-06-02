#ifndef _H26x_H_
#define _H26x_H_

#include "kwrap/type.h"
#include "kwrap/perf.h"
#include "h26x_def.h"

#ifdef __FREERTOS
#include "plat/dma.h"
#include "dma_protected.h"
#endif

#define H26X_FINISH_INT			0x00000001  /* Picture Finish Interrupt. */
#define H26X_BSDMA_INT			0x00000010	/* BSDAM empty interrupt */
#define H26X_ERR_INT			0x00000100	/* Error interrupt */
#define H26X_TIME_OUT_INT		0x00000200	/* Timeout interrupt */
#define H26X_FRAME_TIME_OUT_INT	0x00000400	/* Frame Timeout interrupt */
#define H26X_BSOUT_INT			0x00001000	/* Bitstream buffer full interrupt */
#define H26X_FBC_ERR_INT		0x00100000	/* FBC decompression error interrupt */
#define H26X_SRC_DECMP_ERR_INT  0x01000000  /* source decompression error interrupt */
#define H26X_SRC_D2D_OV_INT  	0x02000000  /* source low latency overwrite interrupt */
#define H26X_SWRST_FINISH_INT	0x10000000	/* swrest finish interrupt */
#define H26X_DMACH_DIS_INT					0x40000000	/* dma channel disable finish interrupt */

// APB set //
#define H26X_REG_BASIC_START_OFFSET			0x20
#define H26X_REG_BASIC_FINISH_OFFSET		0x24C
#define H26X_REG_REPORT_START_OFFSET		0x250
#define H26X_REG_REPORT_FINISH_OFFSET		0x344
#define H26X_REG_BASIC_2_START_OFFSET		0x348
#define H26X_REG_BASIC_2_FINISH_OFFSET		0x3C4
#define H26X_REG_REPORT_2_START_OFFSET 		0x3C8
#define H26X_REG_REPORT_2_FINISH_OFFSET 	0x3F0
#define H26X_REG_PLUG_IN_START_OFFSET		0x400
#define H26X_REG_PLUG_IN_FINISH_OFFSET		0x74C	// OSG //
#define H26X_REG_PLUG_IN_2_START_OFFSET		0x750 	//due to apb_wr_reg_burst need < 256, so split two parts
#define H26X_REG_PLUG_IN_2_FINISH_OFFSET 	0x8D4
#define H26X_REG_STABLE_LEN_START_OFFSET	0x8D8	// stable length, shall not using in link-list //
#define H26X_REG_STABLE_LEN_FINISH_OFFSET	0x8DC
#define H26X_REG_PLUG_IN_3_START_OFFSET		0x900 	// source decompress //
#define H26X_REG_PLUG_IN_3_FINISH_OFFSET  	0x9B4

// APB size //
#define H26X_REG_BASIC_SIZE					(((H26X_REG_BASIC_FINISH_OFFSET - H26X_REG_BASIC_START_OFFSET)>>2) + 1)
#define H26X_REG_REPORT_SIZE				(((H26X_REG_REPORT_FINISH_OFFSET - H26X_REG_REPORT_START_OFFSET)>>2) + 1)
#define H26X_REG_BASIC_2_SIZE				(((H26X_REG_BASIC_2_FINISH_OFFSET - H26X_REG_BASIC_2_START_OFFSET)>>2) + 1)
#define H26X_REG_REPORT_2_SIZE				(((H26X_REG_REPORT_2_FINISH_OFFSET - H26X_REG_REPORT_2_START_OFFSET)>>2) + 1)
#define H26X_REG_PLUG_IN_SIZE				(((H26X_REG_PLUG_IN_FINISH_OFFSET - H26X_REG_PLUG_IN_START_OFFSET)>>2) + 1)
#define H26X_REG_PLUG_IN_2_SIZE				(((H26X_REG_PLUG_IN_2_FINISH_OFFSET - H26X_REG_PLUG_IN_2_START_OFFSET)>>2) + 1)
#define H26X_REG_PLUG_IN_3_SIZE				(((H26X_REG_PLUG_IN_3_FINISH_OFFSET - H26X_REG_PLUG_IN_3_START_OFFSET)>>2) + 1)

// link-list mode //
#define H26X_MAX_DUMMY_WRITE				(0x10)
#define H26X_REPORT_BUF_SIZE				(SIZE_256X(H26X_REG_REPORT_SIZE<<2))
#define H26X_REPORT_2_BUF_SIZE				(SIZE_256X(H26X_REG_REPORT_2_SIZE<<2))

typedef enum _H26xStatus{
    H26X_STATUS_ERROR_SRC_BUF_IS_NOT_USED = -12,  /*!< Current source buffer is not used. */
    H26X_STATUS_ERROR_YADDR_INVALID       = -11,  /*!< Luma address of current picture is invalid. */
    H26X_STATUS_ERROR_DISPLAY_FULL        = -10,  /*!< All display-buffer is full. */
    H26X_STATUS_ERROR_DISPLAY_EMPTY       = -9,   /*!< There is no extra buffer to be displayed. */
    H26X_STATUS_ERROR_REC_BUF_FULL        = -8,   /*!< All recontruction buffer is full. */
    H26X_STATUS_ERROR_REGOBJ_EMPTY        = -7,   /*!< There is no register object for h26x to process. */
    H26X_STATUS_ERROR_REGOBJ_FULL         = -6,
    H26X_STATUS_ERROR_UNLOCK              = -5,   /*!< h26x module unlock error. */
    H26X_STATUS_ERROR_LOCK                = -4,   /*!< h26x module lock error. */
    H26X_STATUS_ERROR_CLOSE               = -3,   /*!< h26x module close error. */
    H26X_STATUS_ERROR_OPEN                = -2,   /*!< h26x module open error. */
    H26X_STATUS_ERROR                     = -1,   /*!< Error. */
    H26X_STATUS_OK                        = 0,    /*!< OK. */
    ENUM_DUMMY4WORD(H26xStatus),
}H26xStatus;

typedef enum _H26XREPORT_{
	H26X_CYCLE_CNT = 0,
	H26X_REC_CHKSUM,
	H26X_SRC_Y_DMA_CHKSUM,
	H26X_SRC_C_DMA_CHKSUM,
	H26X_TNR_OUT_Y_CHKSUM,
	H26X_SCD_REPORT,
	H26X_BS_LEN,
	H26X_BS_CHKSUM,
	H26X_QP_CHKSUM,
	H26X_ILF_DIS_CTB,
	H26X_RRC_BIT_LEN,
	H26X_RRC_RDOPT_COST_LSB,
	H26X_RRC_RDOPT_COST_MSB,
	H26X_RRC_SIZE,
	H26X_RRC_FRM_COST_LSB,
	H26X_RRC_FRM_COST_MSB,
	H26X_RRC_FRM_COMPLEXITY_LSB,
	H26X_RRC_FRM_COMPLEXITY_MSB,
	H26X_RRC_COEFF,
	H26X_RRC_QP_SUM,
	H26X_RRC_SSE_DIST_LSB,
	H26X_RRC_SSE_DIST_MSB,
	H26X_SRAQ_ISUM_ACT_LOG,
	H26X_PSNR_FRM_Y_LSB,
	H26X_PSNR_FRM_Y_MSB,
	H26X_PSNR_FRM_U_LSB,
	H26X_PSNR_FRM_U_MSB,
	H26X_PSNR_FRM_V_LSB,
	H26X_PSNR_FRM_V_MSB,
	H26X_PSNR_ROI_Y_LSB,
	H26X_PSNR_ROI_Y_MSB,
	H26X_PSNR_ROI_U_LSB,
	H26X_PSNR_ROI_U_MSB,
	H26X_PSNR_ROI_V_LSB,
	H26X_PSNR_ROI_V_MSB,
	H26X_IME_CHKSUM_LSB,
	H26X_IME_CHKSUM_MSB,
	H26X_EC_CHKSUM,
	H26X_SIDE_INFO_CHKSUM,
	H26X_ROI_CNT,
    H26X_STABLE_BS_LENGTH,
    H26X_STABLE_SLICE_NUMBERS,
	H26X_STATS_INTER_CNT,
	H26X_STATS_SKIP_CNT,
	H26X_STATS_MERGE_CNT,
	H26X_STATS_IRA4_CNT,
	H26X_STATS_IRA8_CNT,
	H26X_STATS_IRA16_CNT,
	H26X_STATS_IRA32_CNT,
	H26X_STATS_CU64_CNT,
	H26X_STATS_CU32_CNT,
	H26X_STATS_CU16_CNT,
	H26X_STATS_SCD_INTER_CNT,
	H26X_STATS_SCD_IRANG_CNT,
	H26X_JND_Y_CHKSUM,
	H26X_JND_C_CHKSUM,
	H26X_OSG_0_Y_CHKSUM,
	H26X_OSG_0_C_CHKSUM,
	H26X_OSG_Y_CHKSUM,
	H26X_OSG_C_CHKSUM,
	H26X_RRC_COEFF2,
	H26X_RRC_COEFF4,
	H26X_REPORT_NUMBER
}H26XREPORT;

typedef enum _H26XREPORT_2_{
    H26X_PSNR_MOT_Y_LSB = 0,
    H26X_PSNR_MOT_Y_MSB,
    H26X_PSNR_MOT_U_LSB,
    H26X_PSNR_MOT_U_MSB,
    H26X_PSNR_MOT_V_LSB,
    H26X_PSNR_MOT_V_MSB,
    H26X_MOTION_NUM,
    H26X_CRC_HIT_Y_CNT,
    H26X_CRC_HIT_C_CNT,
    H26X_EC_REAL_LEN_SUM,
    H26X_EC_SKIP_LEN_SUM,
	H26X_REPORT_2_NUMBER
}H26XREPORT_2;

typedef struct _H26XOsdUnitSet{
    UINT32 uiGlobal;                     // 0x000
    UINT32 uiGraphic0;                   // 0x004
    UINT32 uiGraphic1;                   // 0x008
    UINT32 uiDisplay0;                   // 0x00C
    UINT32 uiDisplay1;                   // 0x010
    UINT32 uiMask;                       // 0x014
    UINT32 uiGCAC1;                      // 0x018
    UINT32 uiGCAC2;                      // 0x01C
    UINT32 uiGraphic2;                   // 0x020
}H26XOsdUnitSet;

typedef struct _H26XOsdSet{
    H26XOsdUnitSet uiOsdUnit[32];        // 0x000 ~ 0x47C
    UINT32 uiGCAC1;                      // 0x480
    UINT32 uiGCAC0;                      // 0x484
    UINT32 uiCST0;                       // 0x488
    UINT32 uiCST1;                       // 0x48c
    UINT32 uiCST2;                       // 0x490
    UINT32 uiPALETTE[16];                // 0x494 ~
}H26XOsdSet;

typedef struct _H26XTileSet{
    UINT32 TILE_0_MODE;             // 0x350
    UINT32 TILE_WIDTH_0;            // 0x354
    UINT32 TILE_WIDTH_1;            // 0x358
    UINT32 TILE_EXTRA_WR_Y_ADDR[H26X_MAX_TILE_NUM - 1];  // 0x35C ~ 0x368
    UINT32 TILE_EXTRA_WR_C_ADDR[H26X_MAX_TILE_NUM - 1];  // 0x36C ~ 0x378
    UINT32 TILE_EXTRA_RD_Y_ADDR[H26X_MAX_TILE_NUM - 1];  // 0x37C ~ 0x388
    UINT32 TILE_EXTRA_RD_C_ADDR[H26X_MAX_TILE_NUM - 1];  // 0x38C ~ 0x398
    UINT32 TILE_SIDE_INFO_WR_ADDR[H26X_MAX_TILE_NUM - 1];  // 0x39C ~ 0x3A8
    UINT32 TILE_SIDE_INFO_RD_ADDR[H26X_MAX_TILE_NUM - 1];  // 0x3AC ~ 0x3B8
    UINT32 TILE_23_RRC;            // 0x3BC
}H26XTileSet;

typedef struct _H26XRegSet{
	UINT32 DEC0_CFG;				// 0x020
	UINT32 SRC_Y_ADDR;				// 0x024
	UINT32 SRC_C_ADDR;				// 0x028
	UINT32 REC_Y_ADDR;				// 0x02C
	UINT32 REC_C_ADDR;				// 0x030
	UINT32 REF_Y_ADDR;				// 0x034
	UINT32 REF_C_ADDR;				// 0x038
	UINT32 TNR_OUT_Y_ADDR;			// 0x03C
	UINT32 TNR_OUT_C_ADDR;			// 0x040
	UINT32 COL_MVS_WR_ADDR;			// 0x044
	UINT32 COL_MVS_RD_ADDR;			// 0x048
	UINT32 SIDE_INFO_WR_ADDR_0;		// 0x04C
	UINT32 RES_050;					// 0x050
	UINT32 SIDE_INFO_RD_ADDR_0;		// 0x054
	UINT32 RES_058;					// 0x058
	UINT32 RRC_WR_ADDR;				// 0x05C
	UINT32 RRC_RD_ADDR;				// 0x060
	UINT32 QP_MAP_ADDR;				// 0x064
	UINT32 NAL_LEN_OUT_ADDR;		// 0x068
	UINT32 BSDMA_CMD_BUF_ADDR;		// 0x06C
	UINT32 BSOUT_BUF_ADDR;			// 0x070
	UINT32 RES_074;              	// 0x074
	UINT32 TIMEOUT_CNT_MAX;         // 0x078
	UINT32 BSOUT_BUF_SIZE;			// 0x07C
	UINT32 NAL_HDR_LEN_TOTAL_LEN;	// 0x080
	UINT32 SRC_LINE_OFFSET;			// 0x084
	UINT32 REC_LINE_OFFSET;			// 0x088
	UINT32 TNR_OUT_LINE_OFFSET;		// 0x08C
	UINT32 SIDE_INFO_LINE_OFFSET;	// 0x090
	UINT32 SIDE_INFO_LINE_OFFSET2; 	// 0x094
	UINT32 SIDE_INFO_LINE_OFFSET3; 	// 0x098
	UINT32 QP_MAP_LINE_OFFSET;		// 0x09C
	UINT32 FUNC_CFG[2];				// 0x0A0 ~ 0x0A4
	UINT32 SEQ_CFG[2];				// 0x0A8 ~ 0x0AC
	UINT32 RDO_CFG_4;		    	// 0x0B0
	UINT32 PIC_CFG;					// 0x0B4
	UINT32 QP_CFG[2];				// 0x0B8 ~ 0x0BC
	UINT32 ILF_CFG[2];				// 0x0C0 ~ 0x0C4
	UINT32 AEAD_CFG;				// 0x0C8
	UINT32 DSF_CFG;					// 0x0CC
	UINT32 GDR_CFG[2];				// 0x0D0 ~ 0x0D4
	UINT32 FRO_CFG[21];				// 0x0D8 ~ 0x128
	UINT32 RRC_CFG[11];				// 0x12C ~ 0x154
	UINT32 SRAQ_CFG[17];			// 0x158 ~ 0x198
	UINT32 LPM_CFG;					// 0x19C
	UINT32 RMD_CFG;					// 0x1A0
	UINT32 RND_CFG[2];				// 0x1A4 ~ 0x1A8
	UINT32 VAR_CFG[2];				// 0x1AC ~ 0x1B0
	UINT32 IME_CFG;					// 0x1B4
	UINT32 SCD_CFG;					// 0x1B8
	UINT32 DEC1_CFG;				// 0x1BC
	UINT32 ROI_CFG[21];				// 0x1C0 ~ 0x210
	UINT32 RDO_CFG[4];				// 0x214 ~ 0x220
    UINT32 MAQ_CFG[2];				// 0x224 ~ 0x228
    UINT32 RES_22C;					// 0x22C
	UINT32 MOTION_BIT_ADDR[3]; 		// 0x230 ~ 0x238
    UINT32 DBG_CFG;					// 0x23C
    UINT32 DBG_REPORT[3];			// 0x240 ~ 0x248
    UINT32 DEB_CFG;					// 0x24C
    UINT32 CHK_REPORT[62];			// 0x250 ~ 0x344
    UINT32 ESKIP_CFG[2];			// 0x348 ~ 0x34C
    UINT32 TILE_CFG[27];            // 0x350 ~ 0x3B8
    UINT32 RRC_CFG_11;      		// 0x3BC
	UINT32 RMD_CFG_1;				// 0x3C0
	UINT32 RDO_CFG_5;               // 0x3C4
    UINT32 CHK_REPORT_2[11];		// 0x3C8 ~ 0x3F0
    UINT32 RES_3F4_3FC[3];			// 0x3F4 ~ 0x3FC
    UINT32 OSG_CFG[309];			// 0x400 ~ 0x8D0
    UINT32 RES_8D4;					// 0x8D4
    UINT32 NAL_REPORT[2];			// 0x8D8 ~ 0x8DC
    UINT32 RES_8E0_8FC[8];			// 0x8D4 ~ 0x8FC
    UINT32 SDE_CFG[46];				// 0x900 ~ 0x9B4
    UINT32 SPN_CFG[11];				// 0x9B8 ~ 0x9E0
}H26XRegSet;

typedef struct _h26x_llc_cb_{
	void (* get_interrupt)(UINT32 interrupt);
} h26x_llc_cb;
extern h26x_llc_cb *h26x_llc_cb_init(void);

// attach system function //
extern void h26x_powerOn(void);
extern void h26x_powerOff(void);
extern H26xStatus h26x_open(void);
extern H26xStatus h26x_close(void);
extern void h26x_resetINT(void);
extern UINT32 h26x_waitINT(void);
extern UINT32 h26x_checkINT(void);
extern void h26x_resetHW(void);
extern void h26x_module_reset(void);
extern void h26x_isr(void);
extern UINT32 h26x_exit_isr(void);
extern void h26x_resetIntStatus(void);

// set hw function //
extern void h26x_setClk(UINT32 h26x_clk);
extern void h26x_setAPBAddr(UINT32 addr);
extern void h26x_setRstAddr(UINT32 addr);
extern void h26x_reset(void);
extern void h26x_start(void);
extern void h26x_setDramBurstLen(UINT32 uiSel);
extern void h26x_setBsDmaEn(void);
extern void h26x_setBsOutEn(void);
extern UINT32 h26x_setDmaChDis(void);
extern void h26x_setChkSumEn(BOOL enable);
extern void h26x_setAPBReadLoadSet(UINT32 uiSel);
extern void h26x_setAPBLoadEn(void);
extern void h26x_setNextBsBuf(UINT32 uiAddr, UINT32 uiSize);
extern void h26x_setNextBsDmaBuf(UINT32 uiAddr);
extern void h26x_setBsLen(UINT32 uiSize);
extern void h26x_setUnLock(void);
extern void h26x_setLock(void);
extern void h26x_clearIntStatus(UINT32 uiVal);
extern void h26x_setSRAMMode(UINT32 uiCycle, UINT32 uiLightSleepEn, UINT32 uiSleepDownEn);

// get hw function //
extern UINT32 h26x_getClk(void);
extern UINT32 h26x_getDramBurstLen(void);
extern UINT32 h26x_getIntEn(void);
extern UINT32 h26x_getIntStatus(void);
extern UINT32 h26x_getHwRegSize(void);
extern UINT32 h26x_getDummyWTSize(void);

// prepare trigger hw encode register //
extern void h26x_setIntEn(UINT32 uiVal);
extern void h26x_setEncDirectRegSet(UINT32 uiAPBAddr);
extern void h26x_setEncLLRegSet(UINT32 uiJobNum, UINT32 uiJob0APBAddr);
extern void h26x_setLLDummyWriteNum(UINT32 uiVal);
extern void h26x_setHWTimeoutCnt(UINT32 uiVal);
// get hw status //
extern BOOL h26x_getBusyStatus(void);
extern void h26x_getEncReport(UINT32 uiEncReport[H26X_REPORT_NUMBER]);
extern void h26x_getEncReport2(UINT32 uiEncReport[H26X_REPORT_2_NUMBER]);
extern UINT32 h26x_getCurJobNum(void);

// debug tools //
extern void h26x_prtMem(UINT32 uiMemAddr,UINT32 uiMemLen);
extern void h26x_prtReg(void);
//extern void h26x_prtDebug(void); // debug 370 need to modify //
extern UINT32 h26x_getDbg1(UINT32 uiSel);
extern UINT32 h26x_getDbg2(UINT32 uiSel);
extern UINT32 h26x_getDbg3(UINT32 uiSel);
extern void h26x_getDebug(void);

// get physical address //
extern UINT32 h26x_getPhyAddr(UINT32 uiAddr);
extern void h26x_flushCache(UINT32 uiAddr, UINT32 uiSize);
extern void h26x_cache_clean(UINT32 uiAddr, UINT32 uiSize);
extern void h26x_cache_invalidate(UINT32 uiAddr, UINT32 uiSize);
extern void h26x_setBSDMA(UINT32 uiBSDMAAddr, UINT32 uiHwHeaderNum, UINT32 uiHwHeaderAddr, UINT32 *uiHwHeaderSize);
extern void h26x_setBsOutAddr(UINT32 uiAddr, UINT32 uiSize);
extern UINT32 h26x_getBsOutAddr(void);
extern UINT32 h26x_getTmnrSumY(void);
extern UINT32 h26x_getTmnrSumC(void);
extern UINT32 h26x_getMaskSumY(void);
extern UINT32 h26x_getMaskSumC(void);
extern UINT32 h26x_getOsgSumY(void);
extern UINT32 h26x_getOsgSumC(void);
extern void h26x_setDebugSel(UINT32 uiVal370,UINT32 uiVal374,UINT32 uiVal378);
extern UINT32 h26x_getCheckSumSelResult(void);
extern void h26x_setUnitChecksum(UINT32 uiVal);
extern UINT32 h26x_getUnitChecksum(void);
extern void h26x_setCodecClock(BOOL enable);
extern void h26x_setCodecPClock(BOOL enable);
extern UINT32 h26x_getBslen(void);
//extern UINT32 h26x_getChipId(void);
extern void h26x_tick_open(void);
extern void h26x_tick_close(void);
extern VOS_TICK h26x_tick_result(void);

// for kdrv //
extern void h26x_create_resource(void);
extern void h26x_release_resource(void);

// for efuse check //
BOOL h26x_efuse_check(UINT32 uiWidth, UINT32 uiHeight);

// low latency patch //
extern void h26x_low_latency_patch(UINT32 uiAPBAddr);

#endif	//_H26x_H_

