/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#ifndef RATE_CONTROL_PICTURE_H
#define RATE_CONTROL_PICTURE_H

#include "base_type.h"
#include "sw_picture.h"
#include "enccommon.h"
#include "hevcSei.h"

#ifdef VSB_TEMP_TEST
#include "video_statistic.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif



  //#include "enccommon.h"
  //#include "H264Sei.h"

  enum
  {
    HEVCRC_OVERFLOW = -1
  };

#define RC_CBR_HRD  0   /* 1 = Constant bit rate model. Must use filler
  * data to conform */

#define CTRL_LEVELS          7  /* DO NOT CHANGE THIS */
#define CHECK_POINTS_MAX    10  /* DO NOT CHANGE THIS */
#define RC_TABLE_LENGTH     10  /* DO NOT CHANGE THIS */

#ifndef CTBRC_STRENGTH
#define QP_FRACTIONAL_BITS  0
#else
#define QP_FRACTIONAL_BITS  8
#endif

#define RC_MOVING_AVERAGE_FRAMES 60
  typedef struct {
      i32 frame[120];
      i32 length;
      i32 count;
      i32 pos;
      i32 frameRateNumer;
      i32 frameRateDenom;
  } rc_ma_s;
  typedef struct
  {
    i64  a1;               /* model parameter */
    i64  a2;               /* model parameter */
    i32  qp_prev;          /* previous QP */
    i32  qs[RC_TABLE_LENGTH + 1]; /* quantization step size */
    i32  bits[RC_TABLE_LENGTH + 1]; /* Number of bits needed to code residual */
    i32  pos;              /* current position */
    i32  len;              /* current lenght */
    i32  zero_div;         /* a1 divisor is 0 */
    i32  cbr;
    i32  weight;
} linReg_s;

  typedef struct
  {
    i32 wordError[CTRL_LEVELS]; /* Check point error bit */
    i32 qpChange[CTRL_LEVELS];  /* Check point qp difference */
    i32 wordCntTarget[CHECK_POINTS_MAX];    /* Required bit count */
    i32 wordCntPrev[CHECK_POINTS_MAX];  /* Real bit count */
    i32 checkPointDistance;
    i32 checkPoints;
  } hevcQpCtrl_s;

  /* Virtual buffer */
  typedef struct
  {
    i32 bufferSize;          /* size of the virtual buffer */
    i32 minBitRate;
    i32 maxBitRate;
    i32 bitRate;             /* input bit rate per second */
    i32 bitPerPic;           /* average number of bits per picture */
    i32 picTimeInc;          /* timeInc since last coded picture */
    i32 timeScale;           /* input frame rate numerator */
    i32 unitsInTic;          /* input frame rate denominator */
    i32 virtualBitCnt;       /* virtual (channel) bit count */
    i32 realBitCnt;          /* real bit count */
    i32 bufferOccupancy;     /* number of bits in the buffer */
    i32 skipFrameTarget;     /* how many frames should be skipped in a row */
    i32 skippedFrames;       /* how many frames have been skipped in a row */
    i32 nonZeroTarget;
    i32 bucketFullness;      /* Leaky Bucket fullness */
    i32 bucketLevel;         /* Leaky Bucket fullness + virtualBitCnt */
    /* new rate control */
    i32 windowRem;
    i32 seconds;             /* Full seconds elapsed */
    i32 averageBitRate;      /* This buffer average bitrate for full seconds */
  } hevcVirtualBuffer_s;

  typedef struct
  {
    true_e picRc;
    true_e ctbRc;             /* ctb header qp can vary, check point rc */
    true_e picSkip;          /* Frame Skip enable */
    true_e hrd;              /* HRD restrictions followed or not */
    u32 fillerIdx;
    i32 picArea;
    i32 ctbPerPic;            /* Number of macroblock per picture */
    i32 ctbRows;              /* ctb rows in picture */
    i32 ctbSize;              /* ctb size */
    i32 coeffCntMax;         /* Number of coeff per picture */
    i32 nonZeroCnt;
    i32 srcPrm;              /* Source parameter */
    i32 qpSum;               /* Qp sum counter */
    i32 qpNum;
    float averageQp;
    u32 sliceTypeCur;
    u32 sliceTypePrev;
    u32 sliceEncodedTypePrev;
    true_e frameCoded;       /* Pic coded information */
    i32 fixedQp;             /* Pic header qp when fixed */
    i32 qpHdr;               /* Pic header qp of current voded picture */
    i32 qpMin;               /* Pic header minimum qp, user set */
    i32 qpMax;               /* Pic header maximum qp, user set */
    i32 qpHdrPrev;           /* Pic header qp of previous coded picture */
    i32 qpHdrLast;
    i32 frameBitCntLast;
    i32 targetPicSizeLast;
    i32 upBase;
    i32 qpLastCoded;         /* Quantization parameter of last coded mb */
    i32 qpTarget;            /* Target quantrization parameter */
    u32 estTimeInc;
    i32 outRateNum;
    i32 outRateDenom;
    i32 gDelaySum;
    i32 gInitialDelay;
    i32 gInitialDoffs;
    hevcQpCtrl_s qpCtrl;
    hevcVirtualBuffer_s virtualBuffer;
    sei_s sei;
    i32 gBufferMin, gBufferMax;
    /* new rate control */
    linReg_s linReg[4];       /* Data for R-Q model for inter frames */
    linReg_s rError[4];       /* Rate prediction error for inter frames(bits) */
    linReg_s intra;        /* Data for intra frames */
    linReg_s intraError;   /* Prediction error for intra frames */
    linReg_s gop;          /* Data for GOP */
    //linReg_s linReg_BFrame;       /* Data for R-Q model for B frames */
    //linReg_s rError_BFrame;       /* Rate prediction error for B frames(bits) */
    i32 targetPicSize;
    i32 minPicSizeI;
    i32 maxPicSizeI;
    i32 minPicSizeP;
    i32 maxPicSizeP;
    i32 minPicSizeB;
    i32 maxPicSizeB;
    i32 frameBitCnt;
    i32 tolMovingBitRate;
    float f_tolMovingBitRate;
    i32 monitorFrames;
    /* for gop rate control */
    i32 gopQpSum;           /* Sum of current GOP inter frame QPs */
    i32 gopQpDiv;
    i32 gopBitCnt;          /* Current GOP bit count so far */
    i32 gopAvgBitCnt;       /* Previous GOP average bit count */
    u32 frameCnt;
    i32 gopLen;
    i32 windowLen;          /* Bitrate window which tries to match target */
    i32 intraInterval;      /* Distance between two previous I-frames */
    i32 intraIntervalCtr;
    i32 intraQpDelta;
    i32 long_term_ref_pic_poc;
    i32 ltrInterval;
    i32 frameQpDelta;
    u32 fixedIntraQp;
    i32 bpp;
    i32 hierarchial_bit_allocation_total_weight;
    i32 hierarchial_bit_allocation_map[8][8];
    i32 hierarchial_alg_map[8][8];
    i32 hierarchial_decoding_order[8][8];
    i32 hierarchial_bit_allocation_bitmap[8];
    i32 hierarchial_bit_allocation_GOP_size;
    i32 hierarchial_bit_allocation_GOP_size_previous;
    i32 encoded_frame_number;
    u32 gopPoc;
    //CTB_RC
    u32 ctbRcBitsMin;
    u32 ctbRcBitsMax;
    u32 ctbRctotalLcuBit;
    u32 bitsRatio;
    u32 ctbRcThrdMin;
    u32 ctbRcThrdMax;
    i32 seqStart;
    u32 ctbMemCurAddr;
    u32 ctbMemPreAddr;
    u32* ctbMemCurVirtualAddr;   /* CTB_RC_0701   */
    u32* ctbMemPreVirtualAddr;   /* CTB_RC_0701   */

    rc_ma_s ma;

    u32 rcPicComplexity;
  } hevcRateControl_s;

  /*------------------------------------------------------------------------------
      Function prototypes
  ------------------------------------------------------------------------------*/
  bool_e HevcInitRc(hevcRateControl_s *rc, u32 newStream);
  void HevcBeforePicRc(hevcRateControl_s *rc, u32 timeInc, u32 sliceType);
  i32 HevcAfterPicRc(hevcRateControl_s *rc, u32 nonZeroCnt, u32 byteCnt,
                     u32 qpSum,u32 qpNum);
  u32 HevcFillerRc(hevcRateControl_s *rc, u32 frameCnt);
  i32 HevcCalculate(i32 a, i32 b, i32 c);


#ifdef __cplusplus
}
#endif


#endif
