#ifndef _EMU_H265_ENC_H_
#define _EMU_H265_ENC_H_
//
#include "kwrap/type.h"

// for hw trigger //
#include "h26x.h"

// for sw driver //
#include "h26x_def.h"

// for emulation //
#include "emu_h26x_common.h"
#define SUPPORT_SHARPEN           1

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define H26X_EMU_ENDLESS          0
#define H26X_EMU_ROTATE           1
#define H26X_EMU_NROUT            1 // nr out on/off
#define H26X_EMU_OSD              1
#define H26X_EMU_SRC_CMPRS        1
#define H26X_EMU_SMARTREC         1 //520 disable
#define H26X_EMU_SCD              0
#define H26X_EMU_NROUT_ONLY       0 // nr out on/off
#define H26X_EMU_MT               1

#if H26X_EMU_NROUT_ONLY
#define EMU_CMP_BS                0
#define EMU_CMP_BS_CHKSUM         0
#define EMU_CMP_REC               0
#define EMU_CMP_NAL               0
#define EMU_CMP_SIDEINFO          0
#define EMU_CMP_PSNR              0
#define EMU_CMP_NROUT             0 //compare tnr_yuv.dat
#define EMU_CMP_NROUT_CHKSUM      1 //compare tnr out checksum
#define EMU_CMP_COLMV			  0
#define EMU_CMP_DRAM_CHKSUM		  0
#elif H26X_EMU_MT
#define EMU_CMP_BS                0  //x
#define EMU_CMP_BS_CHKSUM         0  //disable: execute very long time
#define EMU_CMP_REC               0
#define EMU_CMP_NAL               0
#define EMU_CMP_SIDEINFO          0
#define EMU_CMP_PSNR              0
#define EMU_CMP_NROUT             0 //compare tnr_yuv.dat
#define EMU_CMP_NROUT_CHKSUM      0 //compare tnr out checksum
#define EMU_CMP_COLMV			  0
#define EMU_CMP_DRAM_CHKSUM		  0 //sw checksum default set 1
#elif 1
#define EMU_CMP_BS                1  //x
#define EMU_CMP_BS_CHKSUM         0  //disable: execute very long time
#define EMU_CMP_REC               0
#define EMU_CMP_NAL               1
#define EMU_CMP_SIDEINFO          1
#define EMU_CMP_PSNR              1
#define EMU_CMP_NROUT             0 //compare tnr_yuv.dat
#define EMU_CMP_NROUT_CHKSUM      1 //compare tnr out checksum
#define EMU_CMP_COLMV			  1
#define EMU_CMP_DRAM_CHKSUM		  0 //sw checksum default set 1
#endif

#define H26X_EMU_MAX_BUF          0
#define H26X_EMU_SRC_LINEOFFSET   0           // could be set enalbe while H26X_EMU_MAX_BUF == Enable//
#define H26X_EMU_REC_LINEOFFSET   0           // could be set enalbe while H26X_EMU_MAX_BUF == Enable//
#define H26X_EMU_OSD_LINEOFFSET   0           // could be set enalbe while H26X_EMU_MAX_BUF == Enable//
#define H26X_EMU_TEST_SREST       0
#define H26X_EMU_TEST_BSOUT		  0//1			    //// rnd_bs_buf , rnd_bs_buf_32b
//#define H26X_EMU_TEST_WP          0
//#define H26X_EMU_TEST_CLK         0
#define H26X_EMU_TEST_RAN_SLICE	  0//1			  // need close EMU_CMP_BS //
//#define H26X_EMU_TEST_BW          0            // could be set enalbe while H26X_EMU_TEST_BW_DISABLE == Disable//
//#define H26X_EMU_TEST_BW_DISABLE  0            // could be set enalbe while H26X_EMU_TEST_BW == Disable
                                               // and H26X_EMU_TEST_SREST and H26X_EMU_TEST_BSDMA and H26X_EMU_TEST_WP = Disable//
                                               //  H26X_EMU_TEST_SREST,H26X_EMU_TEST_BW ) = Disable //
//#define H26X_EMU_GATING           0
//#define H26X_EMU_DRAM_SEL         0
#define H26X_EMU_DRAM_2           0

//#define H26X_EMU_PASS             0
//#define H26X_EMU_FILEERR          1
//#define H26X_EMU_FAIL             2

#define H265_AUTO_JOB_SEL	      (0) //0
#define H265_ENDLESS_RUN	      (0)
#define H265_TEST_MAIN_LOOP       (0) // enable H265_AUTO_JOB_SEL
//#define WRITE_TNR
//#define WRITE_REC
//#define WRITE_SRC
//#define WRITE_BS
//#define WRITE_DEBUG_BS
//#define WRITE_DEBUG_SRCOUT
//#define WRITE_DEBUG_REC
//#define WRITE_DEBUG_SRC

#define H26X_LINEOFFSET_MAX       5120
#define H26X_OSD_LINEOFFSET_MAX   128
#define H26X_OSD_WINDOW_MAX       10

#define H26X_MAX_BSCMD_NUM        (80*5) // 5120/64 = 80, tile need 5X
//#define H26X_MAX_BS_LEN           0x1400000 //hk 0x600000
#if H26X_EMU_MT
#define H26X_MAX_BS_LEN           0x50000 //hk 0x600000
#else
#define H26X_MAX_BS_LEN           0x1410000 //hk 0x600000
#endif


#if H26X_EMU_MAX_BUF
#define H26X_CMB_BUF_MAX_SIZE     0x400000
#define H26X_MAX_HEADER_LEN       0x600000
#else
#define H26X_CMB_BUF_MAX_SIZE     0x2000
#define H26X_MAX_HEADER_LEN       0x10000
#endif

//#define H26X_NAL_LEN_MAX_SIZE     129*4   //129x32 bit
#define H26X_NAL_LEN_MAX_SIZE     (H26X_MAX_BSCMD_NUM*4)   //129x32 bit

#define MASK_REG_NUM              (81)
#define TMNR_REG_NUM              (60)
#define H26X_MAX_LL_SIZE      	  0x1000 // TMP size

#define WDATA_PATTERN_SIZE        (0x22C-0xA0)
#define WDATA_SIZE                (0x22C-0x20)
#define RDATA_SIZE                (0x334-0x250)
#define OSG_SIZE                  (0x50C-0x400)

#define HEVC_EMU_INFO_ROTATE      0
#define HEVC_EMU_INFO_SRCMP       2
#define HEVC_EMU_INFO_MBPQ        3
#define HEVC_EMU_INFO_OSD         5
#define HEVC_EMU_INFO_TMNR        6
#define HEVC_EMU_INFO_MASK        7
#define HEVC_EMU_INFO_TILE        8
#define HEVC_EMU_INFO_ECLS        9
#define HEVC_EMU_INFO_SVC        10

#define bs_reset_value (0xAB)


typedef struct _H26XEncRegCfg{
	UINT32 uiH26XFunc0;
	UINT32 uiH26XFunc1;
	UINT32 uiH26X_SRC_FRAME_SIZE; //seq 0
	UINT32 uiH26XFrameSize;	//seq 1
    UINT32 uiRES_B0;//uiH26XRdo4
	UINT32 uiH26XPic0;
	UINT32 uiH26XPic1;
	UINT32 uiH26XPic2;
	UINT32 uiH26XPic3;
	UINT32 uiH26XPic4;
	UINT32 uiH26XPic5;
	UINT32 uiH26X_DIST_FACTOR;	//pic 6
	UINT32 uiH26XGdr0;
	UINT32 uiH26XGdr1;
	UINT32 uiH26XFro0;
	UINT32 uiH26XFro1;
	UINT32 uiH26XFro2;
	UINT32 uiH26XFro3;
	UINT32 uiH26XFro4;
	UINT32 uiH26XFro5;
	UINT32 uiH26XFro6;
	UINT32 uiH26XFro7;
	UINT32 uiH26XFro8;
	UINT32 uiH26XFro9;
	UINT32 uiH26XFro10;
	UINT32 uiH26XFro11;
	UINT32 uiH26XFro12;
	UINT32 uiH26XFro13;
	UINT32 uiH26XFro14;
	UINT32 uiH26XFro15;
	UINT32 uiH26XFro16;
	UINT32 uiH26XFro17;
	UINT32 uiH26XFro18;
	UINT32 uiH26XFro19;
	UINT32 uiH26XFro20;
	UINT32 uiH26XRrc0;
	UINT32 uiH26XRrc1;
	UINT32 uiH26XRrc2;
	UINT32 uiH26XRrc3;
	UINT32 uiH26XRrc4;
	UINT32 uiH26XRrc5;
	UINT32 uiH26XRrc6;
	UINT32 uiH26XRrc7;
	UINT32 uiH26XRrc8;
	UINT32 uiH26XRrc9;
	UINT32 uiH26XRrc10;
	UINT32 uiH26XSraq0;
	UINT32 uiH26XSraq1;
	UINT32 uiH26XSraq2;
	UINT32 uiH26XSraq3;
	UINT32 uiH26XSraq4;
	UINT32 uiH26XSraq5;
	UINT32 uiH26XSraq6;
	UINT32 uiH26XSraq7;
	UINT32 uiH26XSraq8;
	UINT32 uiH26XSraq9;
	UINT32 uiH26XSraq10;
	UINT32 uiH26XSraq11;
	UINT32 uiH26XSraq12;
	UINT32 uiH26XSraq13;
	UINT32 uiH26XSraq14;
	UINT32 uiH26XSraq15;
	UINT32 uiH26XSraq16;
	UINT32 uiH26XLpm0;
	UINT32 uiH26XRmd0;
	UINT32 uiH26XRnd0;
	UINT32 uiH26XRnd1;
	UINT32 uiH26XVar0;
	UINT32 uiH26XVar1;
	UINT32 uiH26XIme0;
	UINT32 uiH26XScene0;
	UINT32 uiH26XRes0;
	UINT32 uiH26XRoi0;
	UINT32 uiH26XRoi1;
	UINT32 uiH26XRoi2;
	UINT32 uiH26XRoi3;
	UINT32 uiH26XRoi4;
	UINT32 uiH26XRoi5;
	UINT32 uiH26XRoi6;
	UINT32 uiH26XRoi7;
	UINT32 uiH26XRoi8;
	UINT32 uiH26XRoi9;
	UINT32 uiH26XRoi10;
	UINT32 uiH26XRoi11;
	UINT32 uiH26XRoi12;
	UINT32 uiH26XRoi13;
	UINT32 uiH26XRoi14;
	UINT32 uiH26XRoi15;
	UINT32 uiH26XRoi16;
	UINT32 uiH26XRoi17;
	UINT32 uiH26XRoi18;
	UINT32 uiH26XRoi19;
	UINT32 uiH26XRoi20;
	UINT32 uiH26XRdo0;
	UINT32 uiH26XRdo1;
	UINT32 uiH26XRdo2;
	UINT32 uiH26XRdo3;
	UINT32 uiH26XMaq0;
	UINT32 uiH26XMaq1;
	UINT32 uiH26X_JND_0;
	UINT32 uiH26XChk0;
	UINT32 uiH26XRecChkSum;			//chk 1
	UINT32 uiH26X_SrcChkSum_Y; 		//chk 2
	UINT32 uiH26X_SrcChkSum_C; 		//chk 3
	UINT32 uiH26X_TnrOutChkSumY; 	//chk 4
	UINT32 uiH26X_TnrOutChkSumC; 	//chk 5
	UINT32 uiH26XBsLen; 			//chk 6
	UINT32 uiH26XBsChkSum;			//chk 7
	UINT32 uiH26XChk8;
	UINT32 uiH26XSaoInfo;   		//chk 9
	UINT32 uiH26X_RC_EST_BITLEN;    //chk 10
	UINT32 uiH26X_RC_RDO_COST_LOW;	//chk 11
	UINT32 uiH26X_RC_RDO_COST_HIGH;	//chk 12
	UINT32 uiH26X_RC_Frm_Size;		//chk 13
	UINT32 uiH26X_RC_Frm_cost_Low;	//chk 14
	UINT32 uiH26X_RC_Frm_cost_High;	//chk 15
	UINT32 uiH26X_RC_Frm_cmplxty_Low;	//chk 16
	UINT32 uiH26X_RC_Frm_cmplxty_High;	//chk 17
	UINT32 uiH26X_RC_Frm_Coeff;		//chk 18
	UINT32 uiH26X_RC_Frm_qp_sum;	//chk 19
	UINT32 uiH26X_RC_Frm_SSE_DIST_Low;	//chk 20
	UINT32 uiH26X_RC_Frm_SSE_DIST_High;	//chk 21
	UINT32 uiH26X_SRAQ_2; //chk 22
	UINT32 uiH26X_ImeChkSum_lo;
	UINT32 uiH26X_ImeChkSum_hi;
	UINT32 uiH26X_ECChkSum;
	UINT32 uiH26X_SideInfoChkSum;
	UINT32 uiH26X_SliceNum;
	UINT32 uiH26X_stat_Num_INTER;
	UINT32 uiH26X_stat_Num_SKIP;
	UINT32 uiH26X_stat_Num_MERGE;
#if SUPPORT_SHARPEN
    UINT32 uiH26X_stat_Num_IRA4;
#endif
	UINT32 uiH26X_stat_Num_IRA8;
	UINT32 uiH26X_stat_Num_IRA16;
	UINT32 uiH26X_stat_Num_IRA32;
	UINT32 uiH26X_stat_Num_CU64;
	UINT32 uiH26X_stat_Num_CU32;
	UINT32 uiH26X_stat_Num_CU16;
	UINT32 uiH26X_HW_INTER_SCD;
	UINT32 uiH26X_HW_IRAnG_SCD;
	UINT32 uiH26X_YMSE_Low;
	UINT32 uiH26X_YMSE_High;
	UINT32 uiH26X_UMSE_Low;
	UINT32 uiH26X_UMSE_High;
	UINT32 uiH26X_VMSE_Low;
	UINT32 uiH26X_VMSE_High;
	UINT32 uiH26X_YMSE_ROI_Low;
	UINT32 uiH26X_YMSE_ROI_High;
	UINT32 uiH26X_UMSE_ROI_Low;
	UINT32 uiH26X_UMSE_ROI_High;
	UINT32 uiH26X_VMSE_ROI_Low;
	UINT32 uiH26X_VMSE_ROI_High;
	UINT32 uiH26X_ROI_NUMBER;
	UINT32 uiH26X_CRC_Y_HIT_SUM;
	UINT32 uiH26X_CRC_YC_HIT_SUM;
	UINT32 EC_RealLenSum;
	UINT32 EC_SkipLenSum;
	UINT32 EC_TotalLenSum;
	UINT32 uiH26X_TMNR_Y_SUM;
	UINT32 uiH26X_TMNR_C_SUM;
	UINT32 uiH26X_MASK_Y_SUM;
	UINT32 uiH26X_MASK_C_SUM;
	UINT32 uiH26X_OSG_Y_SUM;
	UINT32 uiH26X_OSG_C_SUM;
	UINT32 uiH26X_OSG_Y0_SUM;
	UINT32 uiH26X_OSG_C0_SUM;
    UINT32 uiH26XTile0;
    UINT32 uiH26XTile1;
    UINT32 uiH26XTile2;
    UINT32 uiH26X_RC_Frm_Coeff2; //CHK_60 // 0x340
    UINT32 uiH26XRrc5_2;       //TILE_23_RPC // 0x3BC
    UINT32 uiH26X_RC_Frm_Coeff3; //CHK_61 // 0x344
	UINT32 uiH26X_JND_Y_SUM;
	UINT32 uiH26X_JND_C_SUM;
    UINT32 uiH26X_520ECO;
	#if 0
	//hk: 528 add
	UINT32 uiH26XSraq17;
	UINT32 uiH26XRmd1;
	UINT32 uiH26XJnd1;
	UINT32 uiH26XBgr0;
	UINT32 uiH26XBgr1;
	UINT32 uiH26XBgr2;


	UINT32 uiH26XBgr3;//////////


	UINT32 uiH26XRrc11;
	UINT32 uiH26XRrc12;
	UINT32 uiH26XRrc13;
	UINT32 uiH26XRrc14;
	UINT32 uiH26XRrc15;
	UINT32 uiH26XRrc16;
	UINT32 uiH26XRrc17;
    #endif
#if 1
	UINT32 uiH26X_YMSE_MOTION_Low;
	UINT32 uiH26X_YMSE_MOTION_High;
	UINT32 uiH26X_UMSE_MOTION_Low;
	UINT32 uiH26X_UMSE_MOTION_High;
	UINT32 uiH26X_VMSE_MOTION_Low;
	UINT32 uiH26X_VMSE_MOTION_High;
#endif
#if 0
	UINT32 uiH26X_YMSE_BGR_Low;
	UINT32 uiH26X_YMSE_BGR_High;
	UINT32 uiH26X_UMSE_BGR_Low;
	UINT32 uiH26X_UMSE_BGR_High;
	UINT32 uiH26X_VMSE_BGR_Low;
	UINT32 uiH26X_VMSE_BGR_High;
#endif
	UINT32 uiH26X_MOTION_NUMBER;
    UINT32 uiH26X_MOTION_CUNT;
#if 0
	UINT32 uiH26X_BGR_NUMBER;
    UINT32 uiH26X_JND_GRAD;//uiH26X_JND_0_SUM
    UINT32 uiH26X_JND_GRAD_CNT;//uiH26X_JND_1_SUM
	UINT32 uiH26XTnrCfg0;//3DNR_0
    UINT32 uiH26XTnrP2PRefCfg;//3DNR_1
    UINT32 uiH26XTnrP2PCfg0;//3DNR_2
    UINT32 uiH26XTnrP2PCfg1;//3DNR_3
    UINT32 uiH26XTnrP2PCfg2;//3DNR_4
    UINT32 uiH26XTnrP2PCfg3;//3DNR_5
    UINT32 uiH26XTnrP2PCfg4;//3DNR_6
    UINT32 uiH26XTnrMctfCfg0;//3DNR_7
    UINT32 uiH26XTnrMctfCfg1;//3DNR_8
    UINT32 uiH26XTnrMctfCfg;//3DNR_9
    UINT32 uiH26XTnrP2PClampCfg;//3DNR_10
    UINT32 uiH26XTnrBoarderCheck;//3DNR_11
    UINT32 uiH26XTnrMCTFClampCfg;//3DNR_12
	UINT32 uiH26X_TNR_MOTION_TH; //3DNR_13
	UINT32 uiH26X_TNR_P2P_TH;  //3DNR_14
	UINT32 uiH26X_TNR_MCTF_TH;  //3DNR_15
	UINT32 uiH26X_TNR_P2P_WEIGHT;  //3DNR_16
	UINT32 uiH26X_TNR_MCTF_WEIGHT; 	//3DNR_17
	UINT32 uiH26X_TNR_P2P_CFG_6; //3DNR_18

	UINT32 uiH26X_Motion_Cunt; //CHK_5
	UINT32 uiH26X_LMTBL[26];
	UINT32 uiH26X_SLMTBL[18];
	#endif
	UINT32 uiH26XRmd1;
	UINT32 uiH26XRdo5;
	UINT32 uiH26XSkip0;
	UINT32 uiH26XSkip1;
#if SUPPORT_SHARPEN
    UINT32 uiH26X_SPN[11];
#endif
}__attribute__((packed))H26XEncRegCfg;

typedef struct _H26XEncTmnrDbgSum{
	UINT32 ui_src_y_sum;
	UINT32 ui_src_c_sum;
	UINT32 ui_read_y_sum;
	UINT32 ui_read_c_sum;
	UINT32 ui_write_y_sum;
	UINT32 ui_write_c_sum;
	UINT32 ui_hist_sum;
	UINT32 ui_mot_in_sum;
	UINT32 ui_mot_out_sum;
}H26XEncTmnrDbgSum;

typedef struct _h265_perf_t_{
	UINT64 cycle_sum;
	UINT64 bslen_sum;
	UINT64 cycle_max;
	UINT64 cycle_max_bslen;
    float cycle_max_avg;
	float cycle_max_bitlen_avg;
    UINT64 cycle_max_frm;
}h265_perf_t;

typedef struct _H26XEncInfoCfg{
    UINT8 uiYuvName[256];
    INT32 dump_src_en;
    UINT32 uiWidth;
    UINT32 uiHeight;
    UINT32 uiFrameNum;
    UINT32 uiCommandLen;
}H26XEncInfoCfg;

typedef struct _H26XEncSeqCfg{
    H26XEncInfoCfg uiH26XInfoCfg;
    UINT32 uiTmp;
}H26XEncSeqCfg;

typedef struct _H26XEncPicCfg{
    H26XEncRegCfg uiH26XRegCfg;
    UINT32 uiH26XSrcLineOffset;
    UINT32 uiH26XRecLineOffset;
    UINT32 uiIlfLineOffset;
    UINT32 uiNROutLineOffset;
	UINT32 uiSrcDecOutLineOffset;
    UINT32 uiTileIlfLineOffset[H26X_MAX_TILE_NUM - 1];
    UINT32 uiTileSize[H26X_MAX_TILE_NUM];
}H26XEncPicCfg;

typedef struct _H26XEncDramInfo{
	UINT32 uiH26XBsDramSum;
	UINT32 uiH26XSrcOutDramSum;
}H26XEncDramInfo;

//#define FRM_IDX_MAX 19
#define FRM_IDX_MAX 17
typedef enum
{
    RC_B_FRAME  = 0,
    RC_P_FRAME  = 1,
    RC_I_FRAME  = 2,
	RC_P2_FRAME = 3,
	RC_P3_FRAME = 4,
	RC_NUM_FRAME_TYPES
} RC_FRAME_TYPE;
typedef struct _H26XEncAddr{
    UINT32 uiSrcYAddr,uiSrcUVAddr;
    UINT32 uiRefAndRecYAddr[FRM_IDX_MAX],uiRefAndRecUVAddr[FRM_IDX_MAX];
    UINT32 uiRef0Idx;
    UINT32 uiRecIdx;
    UINT32 uiPicBsAddr;
    UINT32 uiColInfoRdAndWtAddr;
    UINT32 uiRecURAddr;
	UINT32 uiEDKTabl[FRM_IDX_MAX][9];
    UINT32 uiIlfSideInfoAddr[FRM_IDX_MAX];
    UINT32 uiIlfExtraSideInfoAddr[FRM_IDX_MAX]; // for tile mode
    UINT32 uiIlfRIdx;
    UINT32 uiIlfWIdx;
    UINT32 uiCMDBufAddr;
    UINT32 uiHwBsAddr;
    UINT32 uiCacheSramOpt;

    UINT32 uiHwHeaderNum;
    UINT32 uiHwHeaderAddr;
    UINT32 uiHwHeaderSize[H26X_MAX_BSCMD_NUM];
	UINT32 uiHwHeaderSizeTmp[H26X_MAX_BSCMD_NUM];

    UINT32 uiColMvsAddr[FRM_IDX_MAX];
    UINT32 uiColMvRIdx;
    UINT32 uiColMvWIdx;
    UINT32 uiUserQpAddr;
    UINT32 uiNROutYAddr;
    UINT32 uiNROutUVAddr;
	UINT32 uiNalLenDumpAddr;
	UINT32 uiLLCmdAddr;
	//UINT32 uiApbRdReg0DumpAddr;

   // UINT32 uiVirTotalBinAddr;

	//reg 0x258, 0x25c
	UINT32 uiRCStatReadfrmAddr[RC_NUM_FRAME_TYPES];
  	UINT32 uiRCStatWritefrmAddr[RC_NUM_FRAME_TYPES];

	//reg 0x2d0, 2d4
	UINT32 uiRecExtraYAddr[FRM_IDX_MAX][H26X_MAX_TILE_NUM - 1];
	UINT32 uiRecExtraCAddr[FRM_IDX_MAX][H26X_MAX_TILE_NUM - 1];
    UINT32 uiRecExtraStatus;
    UINT32 uiRecExtraEnable;
    UINT32 uiRecExtraRingY[FRM_IDX_MAX][H26X_MAX_TILE_NUM];
    UINT32 uiRecExtraRingC[FRM_IDX_MAX][H26X_MAX_TILE_NUM];
    UINT32 uiRecExtraRingIdx[FRM_IDX_MAX];
    UINT32 uiOsdAddr[10];
    UINT32 uiOsgCacAddr;

	//TMNR
	UINT32 uiTmnrRefAndRecYAddr[2];
	UINT32 uiTmnrRefAndRecCAddr[2];
	UINT32 uiTmnrRefAndRecMotAddr[2];
	UINT32 uiTmnrInfoAddr;
    UINT32 uiMotionBitAddr[3];

}H26XEncAddr;

typedef struct _h265_emu_file_t_{
    H26XFile reg_cfg;
    H26XFile info_cfg;
    H26XFile src;
    H26XFile srcbs_y;
    H26XFile srcbs_c;
    H26XFile slice_header_len;
    H26XFile slice_header;
    H26XFile mbqp;
    H26XFile maq;
#if EMU_CMP_BS
    H26XFile bs;
#endif
#if EMU_CMP_REC
    H26XFile rec_y;
    H26XFile rec_c;
    H26XFile rec_y_len;
    H26XFile rec_c_len;
#endif
#if EMU_CMP_NAL
    H26XFile nal_len;
#endif
#if EMU_CMP_SIDEINFO
    H26XFile ilf_sideinfo;
#endif
#if EMU_CMP_NROUT
    H26XFile tnr_out;
#endif
#ifdef WRITE_SRC
    H26XFile hw_src;
#endif
#ifdef WRITE_BS
    H26XFile hw_bs;
#endif
#ifdef WRITE_TNR
    H26XFile hw_tnr;
#endif
#ifdef WRITE_REC
    H26XFile hw_rec;
#endif
#if H26X_EMU_OSD
    H26XFile osd;
    H26XFile osd_grp[H26X_OSD_WINDOW_MAX];
#endif
    H26XFile sde;
	H26XFile colmv;
	H26XFile draminfo;
}h265_emu_file_t;

#define H26X_MAX_WPBUF_NUM 100

typedef struct _h265_emu_t_{
    UINT32 uiWidth;
    UINT32 uiHeight;
    UINT32 uiSrcWidth;
    UINT32 uiSrcHeight;
    UINT32 uiSrcFrmSize;
    UINT32 uiFrameSize;
	UINT32 uiRCSize;//hk
	UINT32 uiUserQp_Size;//hk
    UINT32 uiRecLineOffset;
    UINT32 uiNrOurOffset;
    UINT32 uiIlfLineOffset;
    UINT32 uiIlfLineOffset_cmodel;
    UINT32 uiTileNum;
    UINT32 uiTileWidth[H26X_MAX_TILE_NUM];
    UINT32 ColMvs_Size;
    h265_emu_file_t file;
    UINT32 uiVirSwTnroutYAddr;
    UINT32 uiVirSwTnroutUVAddr;
    UINT32 uiOsdEn;
    UINT32 uiOsdHackCfg;
    UINT32 uiSrccomprsEn;
    UINT32 uiSrccomprsLumaEn;
    UINT32 uiSrccomprsChromaEn;
    UINT32 uiSrcCbCrEn;
    UINT32 uiVirEmuSrcYAddr,uiVirEmuSrcUVAddr;
    UINT32 uiVirHwSrcYAddr,uiVirHwSrcUVAddr;
    UINT32 uiVirSwIlfInfoAddr;
    UINT32 IlfSideInfo_Mall_Size;
    UINT32 uiEcEnable;
#if EMU_CMP_REC
    UINT32 uiVirTmpRecExtraYAddr;
    UINT32 uiVirTmpRecExtraCAddr;
#endif
	UINT32     uiAPBAddr;
    H26XRegSet *pH26XRegSet;
    H26XEncSeqCfg sH26XEncSeqCfg;
    UINT8 g_main_argv[0x3000];
    H26XEncPicCfg sH26XEncPicCfg;
    H26XEncAddr   sH26XVirEncAddr;
    UINT32 uiTmpRecYAddr,uiTmpRecCAddr;
    UINT32 uiTmpIlfSideInfoAddr;
    UINT32 uiTmpColMvsAddr;
    UINT32 uiVirtmpAddr;
    UINT32 uiPicType;
    UINT32 uiSliceSize;
    UINT32 Y_Size;
    UINT32 UV_Size;
	UINT32 uiTmnrMotSize;
	UINT32 uiTmnrInfoSize;
	UINT32 uiTmnrInfoSampleSize;
    UINT32 uiRotateEn;
    INT32  iRotateAngle;
    UINT32 uiRotateVal;
    INT32  iTestNROutEn;
    INT32  iNROutEn;
    INT32  iNROutInterleave;
    UINT32 uiSmartRecEn;
    UINT32 uiBsSegmentSize;   // for H26X_EMU_TEST_BSOUT
    UINT32 uiBsSegmentOffset; // for H26X_EMU_TEST_BSOUT
    UINT32 uiHwBsMaxLen; // for H26X_EMU_TEST_BSOUT
    UINT32 uiHwBsMaxMallocLen;
    UINT32 uiBsCurrestSize;
    UINT8  ucFileDir[256];
    UINT32 i_cnt;
    UINT32 p_cnt;
    UINT32 g_SrcInMode;
    INT32  g_EmuInfo0;
    INT32  g_EmuInfo1;
    INT32  g_EmuInfo2;
    INT32  g_EmuInfo3;
    INT32  g_EmuInfo4;
    INT32  g_EmuInfo5;
    INT32  g_EmuInfo6;
    UINT32 uiWriteSrcY;
    UINT32 uiWriteSrcC;
	h265_perf_t perf;
    UINT32 uiOsgGraphMaxSize[32];
    UINT8  uiMotionBitIdx[3]; // 0 : cur, 1: pre 1, 2: pre 2
    UINT8  uiMotionBitNum;
    UINT32 y_chk_sum[2];
    UINT32 c_chk_sum[2];
    UINT32 wp[H26X_MAX_WPBUF_NUM];
	UINT32 wpcnt;
	UINT32 wprcnt;
	UINT32 src_out_only;
	UINT32 src_d2d_en;
    UINT32 src_d2d_mode;
	UINT32 bs_buf_32b;
    UINT32 stable_bs_len;
	UINT32 uiNalLen[H26X_MAX_BSCMD_NUM];
    UINT32 uiSliceNum;
	H26XEncDramInfo sH26XEncDramInfo;
    UINT32 tile_extra_method;
}h265_emu_t;

typedef struct _h265_pat_t_{
    UINT32 uiSrcNum;
    UINT32 uiPatNum;
    UINT32 uiPicNum;
}h265_pat_t;



typedef struct _h265_ctx_t_{
    h265_emu_t    emu;
    h265_pat_t    pat;
    UINT32        uiEncId;
	CODEC_TYPE    eCodecType;

}h265_ctx_t;


typedef struct _H26XOsdSet_Cmodel{
    H26XOsdUnitSet uiOsdUnit[H26X_OSD_WINDOW_MAX];        // 0x000 ~ 0x47C
    UINT32 uiGCAC1;                      // 0x480
    UINT32 uiGCAC0;                      // 0x484
    UINT32 uiCST0;                       // 0x488
    UINT32 uiCST1;                       // 0x48c
    UINT32 uiCST2;                       // 0x490
    UINT32 uiPALETTE[16];                // 0x494 ~
    UINT32 uiOVERLAP;                    //
}H26XOsdSet_Cmodel;


BOOL emu_h265_setup(h26x_ctrl_t *p_ctrl, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d, char src_path_d2d[2][128]);


BOOL emu_h265_close_one_sequence(h26x_job_t *p_job, h265_ctx_t *ctx, h26x_mem_t *p_mem);

BOOL emu_h265_prepare_one_sequence(h26x_job_t *p_job, h265_ctx_t *ctx, h26x_mem_t *p_mem, h26x_srcd2d_t *p_src_d2d);

BOOL emu_h265_prepare_one_pic(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item, h26x_srcd2d_t *p_src_d2d);

BOOL emu_h265_chk_one_pic(h26x_job_t *p_job, UINT32 interrupt, unsigned int job_num, unsigned int rec_out_en);

BOOL emu_h265_init(h26x_job_t *p_job, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d);

void emu_h265_set_nxt_bsbuf(h26x_job_t *p_job);
BOOL emu_h265_read_bs(h265_emu_t *emu);
BOOL emu_h265_read_nal(h265_emu_t *emu);
BOOL emu_h265_compare_bs(h26x_job_t *p_job,UINT32 compare_bs_size);
BOOL emu_h265_compare_nal(h26x_job_t *p_job, UINT32 uiSliceNum);
BOOL emu_h265_compare_stable_len(h26x_job_t *p_job);
void emu_h265_test_rec_lineoffset(h265_ctx_t *ctx);


//#endif // (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#endif	// _EMU_H265_ENC_H_



