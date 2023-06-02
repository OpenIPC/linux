/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : ASIC low level controller
--
------------------------------------------------------------------------------*/
#ifndef __ENC_ASIC_CONTROLLER_H__
#define __ENC_ASIC_CONTROLLER_H__

#include "base_type.h"
#include "enccfg.h"
#include "ewl.h"
#include "encswhwregisters.h"
#include "hevcencapi.h"

/* HW status register bits */
#define ASIC_STATUS_ALL                 0x3FD

#define ASIC_STATUS_FUSE                0x200
#define ASIC_STATUS_SLICE_READY         0x100
#define ASIC_STATUS_HW_TIMEOUT          0x040
#define ASIC_STATUS_BUFF_FULL           0x020
#define ASIC_STATUS_HW_RESET            0x010
#define ASIC_STATUS_ERROR               0x008
#define ASIC_STATUS_FRAME_READY         0x004

#define ASIC_IRQ_LINE                   0x001

#define ASIC_STATUS_ENABLE              0x001

#define ASIC_HEVC_BYTE_STREAM           0x00
#define ASIC_HEVC_NAL_UNIT              0x01

#define ASIC_PENALTY_UNDEFINED          -1

#define ASIC_PENALTY_TABLE_SIZE         128

#define ASIC_FRAME_BUF_LUM_MAX          (HEVCENC_MAX_REF_FRAMES+1)
#define ASIC_FRAME_BUF_CHR_MAX          (HEVCENC_MAX_REF_FRAMES+1)

#define MAX_CU_WIDTH 64

typedef enum
{
  IDLE = 0,   /* Initial state, both HW and SW disabled */
  HWON_SWOFF, /* HW processing, SW waiting for HW */
  HWON_SWON,  /* Both HW and SW processing */
  HWOFF_SWON, /* HW is paused or disabled, SW is processing */
  DONE
} bufferState_e;

typedef enum
{
  ASIC_HEVC = 1,
  ASIC_VP9 = 2
} asicCodingType_e;

typedef enum
{
  ASIC_P_16x16 = 0,
  ASIC_P_16x8 = 1,
  ASIC_P_8x16 = 2,
  ASIC_P_8x8 = 3,
  ASIC_I_4x4 = 4,
  ASIC_I_16x16 = 5
} asicMbType_e;

typedef enum
{
  ASIC_INTER = 0,
  ASIC_INTRA = 1,
  ASIC_MVC = 2,
  ASIC_MVC_REF_MOD = 3
} asicFrameCodingType_e;

enum
{
  ASIC_PENALTY_I16MODE0 = 0,
  ASIC_PENALTY_I16MODE1,
  ASIC_PENALTY_I16MODE2,
  ASIC_PENALTY_I16MODE3,
  ASIC_PENALTY_I4MODE0,
  ASIC_PENALTY_I4MODE1,
  ASIC_PENALTY_I4MODE2,
  ASIC_PENALTY_I4MODE3,
  ASIC_PENALTY_I4MODE4,
  ASIC_PENALTY_I4MODE5,
  ASIC_PENALTY_I4MODE6,
  ASIC_PENALTY_I4MODE7,
  ASIC_PENALTY_I4MODE8,
  ASIC_PENALTY_I4MODE9,
  ASIC_PENALTY_I16FAVOR,
  ASIC_PENALTY_I4_PREV_MODE_FAVOR,
  ASIC_PENALTY_COST_INTER,
  ASIC_PENALTY_DMV_COST_CONST,
  ASIC_PENALTY_INTER_FAVOR,
  ASIC_PENALTY_SKIP,
  ASIC_PENALTY_GOLDEN,
  ASIC_PENALTY_SPLIT4x4,
  ASIC_PENALTY_SPLIT8x4,
  ASIC_PENALTY_SPLIT8x8,
  ASIC_PENALTY_SPLIT16x8,
  ASIC_PENALTY_SPLIT_ZERO,
  ASIC_PENALTY_DMV_4P,
  ASIC_PENALTY_DMV_1P,
  ASIC_PENALTY_DMV_QP,
  ASIC_PENALTY_DZ_RATE0,
  ASIC_PENALTY_DZ_RATE1,
  ASIC_PENALTY_DZ_RATE2,
  ASIC_PENALTY_DZ_RATE3,
  ASIC_PENALTY_DZ_SKIP0,
  ASIC_PENALTY_DZ_SKIP1,

  ASIC_PENALTY_AMOUNT
};

typedef struct
{
  u32 irqDisable;
  u32 inputReadChunk;
  u32 asic_axi_readID;
  u32 asic_axi_writeID;
  u32 asic_stream_swap;
  u32 asic_pic_swap;
  u32 asic_roi_map_qp_delta_swap;
  u32 asic_ctb_rc_mem_out_swap;
  u32 asic_burst_length;
  u32 asic_burst_scmd_disable;
  u32 asic_burst_incr;
  u32 asic_data_discard;
  u32 asic_clock_gating;
  u32 asic_axi_dual_channel;


  u32 nal_unit_type;
  u32 nuh_temporal_id;

  u32 codingType;
  u32 sliceSize;
  u32 sliceNum;
  u32 outputStrmBase;
  u32 outputStrmSize;

  u32 frameCodingType;
  u32 poc;

  u32 inputLumBase;
  u32 inputCbBase;
  u32 inputCrBase;

  //current picture
  u32 reconLumBase;
  u32 reconCbBase;
  u32 reconCrBase;
  u32 reconL4nBase;


  u32 minCbSize;
  u32 maxCbSize; /* ctb size */

  u32 minTrbSize;
  u32 maxTrbSize;

  u32 picWidth;
  u32 picHeight;

  u32 pps_deblocking_filter_override_enabled_flag;
  u32 slice_deblocking_filter_override_flag;


  u32 qp;
  u32 qpMin;
  u32 qpMax;
  u32 rcQpDeltaRange;


  u32 picInitQp;
  u32 diffCuQpDeltaDepth;

  i32 cbQpOffset;
  i32 crQpOffset;

  u32 saoEnable;

  u32 maxTransHierarchyDepthInter;
  u32 maxTransHierarchyDepthIntra;

  u32 cuQpDeltaEnabled;
  u32 log2ParellelMergeLevel;
  u32 numShortTermRefPicSets;
  u32 rpsId;
  u32 numNegativePics;
  u32 numPositivePics;
  i32 delta_poc0;
  i32 l0_delta_poc[2];
  i32 used_by_curr_pic0;
  i32 l0_used_by_curr_pic[2];
  i32 delta_poc1;
  i32 l1_delta_poc[2];
  i32 used_by_curr_pic1;
  i32 l1_used_by_curr_pic[2];

  //reference picture list
  u32 pRefPic_recon_l0[3][2]; /* separate luma and chroma addresses */

  u32 pRefPic_recon_l0_4n[2];

  u32 pRefPic_recon_l1[3][2]; /* separate luma and chroma addresses */

  u32 pRefPic_recon_l1_4n[2];

  //compress
  u32 ref_l0_luma_compressed[2];
  u32 ref_l0_chroma_compressed[2];
  u32 ref_l0_luma_compress_tbl_base[2];
  u32 ref_l0_chroma_compress_tbl_base[2];

  u32 ref_l1_luma_compressed[2];
  u32 ref_l1_chroma_compressed[2];
  u32 ref_l1_luma_compress_tbl_base[2];
  u32 ref_l1_chroma_compress_tbl_base[2];

  u32 recon_luma_compress;
  u32 recon_chroma_compress;
  u32 recon_luma_compress_tbl_base;
  u32 recon_chroma_compress_tbl_base;

  u32 active_l0_cnt;
  u32 active_l1_cnt;
  u32 active_override_flag;

  u32 hevcStrmMode;   /* 0 - byte stream, 1 - NAL units */

  // intra setup
  u32 strong_intra_smoothing_enabled_flag;
  u32 constrained_intra_pred_flag;

  u32 scaling_list_enabled_flag;

  u32 cirStart;
  u32 cirInterval;

  u32 intraAreaTop;
  u32 intraAreaLeft;
  u32 intraAreaBottom;
  u32 intraAreaRight;

  u32 roi1Top;
  u32 roi1Left;
  u32 roi1Bottom;
  u32 roi1Right;

  u32 roi2Top;
  u32 roi2Left;
  u32 roi2Bottom;
  u32 roi2Right;

  i32 roi1DeltaQp;
  i32 roi2DeltaQp;

  u32 rcRoiEnable;
  u32 roiMapDeltaQpAddr;


  u32 roiUpdate;
  u32 filterDisable;
  i32 tc_Offset;
  i32 beta_Offset;

  u32 intraPenaltyPic4x4;
  u32 intraPenaltyPic8x8;
  u32 intraPenaltyPic16x16;
  u32 intraPenaltyPic32x32;
  u32 intraMPMPenaltyPic1;
  u32 intraMPMPenaltyPic2;
  u32 intraMPMPenaltyPic3;

  u32 intraPenaltyRoi14x4;
  u32 intraPenaltyRoi18x8;
  u32 intraPenaltyRoi116x16;
  u32 intraPenaltyRoi132x32;
  u32 intraMPMPenaltyRoi11;
  u32 intraMPMPenaltyRoi12;
  u32 intraMPMPenaltyRoi13;

  u32 intraPenaltyRoi24x4;
  u32 intraPenaltyRoi28x8;
  u32 intraPenaltyRoi216x16;
  u32 intraPenaltyRoi232x32;
  u32 intraMPMPenaltyRoi21;
  u32 intraMPMPenaltyRoi22;
  u32 intraMPMPenaltyRoi23;

  u32 sizeTblBase;
  u32 lamda_SAO_luma;
  u32 lamda_SAO_chroma;
  u32 lamda_motion_sse;
  u32 lamda_motion_sse_roi1;
  u32 lamda_motion_sse_roi2;
  u32 skip_chroma_dc_threadhold;
  u32 bits_est_tu_split_penalty;
  u32 bits_est_bias_intra_cu_8;
  u32 bits_est_bias_intra_cu_16;
  u32 bits_est_bias_intra_cu_32;
  u32 bits_est_bias_intra_cu_64;
  u32 inter_skip_bias;
  u32 bits_est_1n_cu_penalty;
  u32 lambda_motionSAD;
  u32 lambda_motionSAD_ROI1;
  u32 lambda_motionSAD_ROI2;

  u32 recon_chroma_half_size;

  u32 inputImageFormat;

  u32 outputBitWidthLuma;
  u32 outputBitWidthChroma;


  u32 inputImageRotation;

  u32 inputChromaBaseOffset;
  u32 inputLumaBaseOffset;


  u32 pixelsOnRow;

  u32 xFill;
  u32 yFill;


  u32 colorConversionCoeffA;
  u32 colorConversionCoeffB;
  u32 colorConversionCoeffC;
  u32 colorConversionCoeffE;
  u32 colorConversionCoeffF;
  u32 rMaskMsb;
  u32 gMaskMsb;
  u32 bMaskMsb;

  u32 scaledLumBase;
  u32 scaledWidth;
  u32 scaledHeight;
  u32 scaledWidthRatio;
  u32 scaledHeightRatio;
  u32 scaledSkipLeftPixelColumn;
  u32 scaledSkipTopPixelRow;
  u32 scaledVertivalWeightEn;
  u32 scaledHorizontalCopy;
  u32 scaledVerticalCopy;
  u32 scaledOutSwap;
  u32 chromaSwap;
  u32 nalUnitSizeSwap;

  u32 compress_coeff_scan_base;
  u32 buswidth;

  u32 cabac_init_flag;
  u32 buffer_full_continue;

  u32 sliceReadyInterrupt;

  u32 asicHwId;

  u32 targetPicSize;

  u32 minPicSize;

  u32 maxPicSize;

  u32 averageQP;

  u32 nonZeroCount;

  u32 intraCu8Num;
  u32 skipCu8Num;
  u32 PBFrame4NRdCost;
  u32 SSEDivide256;
  //for noise reduction
  u32 noiseReductionEnable;
  u32 noiseLow;
  //u32 firstFrameSigma;
  u32 nrMbNumInvert;
  u32 nrSliceQPPrev;
  u32 nrThreshSigmaCur;
  u32 nrSigmaCur;
  u32 nrThreshSigmaCalced;
  u32 nrFrameSigmaCalced;
  // for HEVC
  #ifndef CTBRC_STRENGTH
  u32 lambda_sse_me[16];
  u32 lambda_satd_me[16];
  u32 lambda_satd_ims[16];
  #else
  u32 lambda_sse_me[32];
  u32 lambda_satd_me[32];
  u32 lambda_satd_ims[32];
  #endif
  u32 intra_size_factor[4];
  u32 intra_mode_factor[3];


  u32 ctbRcBitMemAddrCur;
  u32 ctbRcBitMemAddrPre;


  u32 ctbRcThrdMin;

  u32 ctbRcThrdMax;

  u32 ctbBitsMin;

  u32 ctbBitsMax;
  u32 totalLcuBits;
  u32 bitsRatio;
#ifdef CTBRC_STRENGTH
  i32 qpfrac;
  u32 sumOfQP;
  u32 sumOfQPNumber;
  u32 offsetMBComplexity;
  u32 qpDeltaMBGain;
  u32 picComplexity;
  u32 rcBlockSize;
  i32 offsetSliceQp;
#endif

  //  for vp9
  u32 mvRefIdx[2];
  u32 ref2Enable;

  u32 ipolFilterMode;
  u32 disableQuarterPixelMv;  //not add into registers

  u32 splitMvMode;          //not add into registers
  u32 partitionBase[8];
  u32 qpY1QuantDc[4];
  u32 qpY1QuantAc[4];
  u32 qpY2QuantDc[4];
  u32 qpY2QuantAc[4];
  u32 qpChQuantDc[4];
  u32 qpChQuantAc[4];
  u32 qpY1ZbinDc[4];
  u32 qpY1ZbinAc[4];
  u32 qpY2ZbinDc[4];
  u32 qpY2ZbinAc[4];
  u32 qpChZbinDc[4];
  u32 qpChZbinAc[4];
  u32 qpY1RoundDc[4];
  u32 qpY1RoundAc[4];
  u32 qpY2RoundDc[4];
  u32 qpY2RoundAc[4];
  u32 qpChRoundDc[4];
  u32 qpChRoundAc[4];
  u32 qpY1DequantDc[4];
  u32 qpY1DequantAc[4];
  u32 qpY2DequantDc[4];
  u32 qpY2DequantAc[4];
  u32 qpChDequantDc[4];
  u32 qpChDequantAc[4];
  u32 filterLevel[4];
  u32 boolEncValue;
  u32 boolEncValueBits;
  u32 boolEncRange;
  u32 dctPartitions;
  u32 filterSharpness;
  u32 segmentEnable;
  u32 segmentMapUpdate;
  i32 lfRefDelta[4];
  i32 lfModeDelta[4];
  u32 avgVar;
  u32 invAvgVar;
  u32 cabacCtxBase;
  u32 probCountBase;

  //ref pic lists modification
  u32 lists_modification_present_flag;
  u32 ref_pic_list_modification_flag_l0;
  u32 list_entry_l0[2];
  u32 ref_pic_list_modification_flag_l1;
  u32 list_entry_l1[2];

  u32 regMirror[ASIC_SWREG_AMOUNT];
} regValues_s;


typedef struct
{
  const void *ewl;
  regValues_s regs;
  //source image

  //reconstructed  image
  EWLLinearMem_t internalreconLuma[ASIC_FRAME_BUF_LUM_MAX];
  EWLLinearMem_t internalreconLuma_4n[ASIC_FRAME_BUF_LUM_MAX];
  EWLLinearMem_t internalreconChroma[ASIC_FRAME_BUF_LUM_MAX];
  //EWLLinearMem_t internalreconCr[ASIC_FRAME_BUF_LUM_MAX];

  EWLLinearMem_t scaledImage;
  EWLLinearMem_t cabacCtx;
  EWLLinearMem_t mvOutput;
  EWLLinearMem_t probCount;
  EWLLinearMem_t segmentMap;
  u32 sizeTblSize;
  EWLLinearMem_t sizeTbl;
  EWLLinearMem_t compress_coeff_SACN;
  EWLLinearMem_t ctbRCCur;
  EWLLinearMem_t ctbRCPre;

  u32 traceRecon;

  //compressor table
  EWLLinearMem_t compressTbl[ASIC_FRAME_BUF_LUM_MAX];
} asicData_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 EncAsicControllerInit(asicData_s *asic);

i32 EncAsicMemAlloc_V2(asicData_s *asic, u32 width, u32 height,
                       u32 scaledWidth, u32 scaledHeight,
                       u32 encodingType, u32 numRefBuffsLum, u32 numRefBuffsChr, u32 compressor,
                       u32 bitDepthLuma,u32 bitDepthChroma);
void EncAsicMemFree_V2(asicData_s *asic);

/* Functions for controlling ASIC */

u32 EncAsicGetPerformance(const void *ewl);

void EncAsicGetRegisters(const void *ewl, regValues_s *val);
u32 EncAsicGetStatus(const void *ewl);
void EncAsicClearStatus(const void *ewl,u32 value);




void EncAsicFrameStart(const void *ewl, regValues_s *val);

void EncAsicStop(const void *ewl);

void EncAsicRecycleInternalImage(asicData_s *asic, u32 numViews, u32 viewId,
                                 u32 anchor, u32 numRefBuffsLum, u32 numRefBuffsChr);

i32 EncAsicCheckStatus_V2(asicData_s *asic,u32 status);
u32 *EncAsicGetMvOutput(asicData_s *asic, u32 mbNum);

#endif