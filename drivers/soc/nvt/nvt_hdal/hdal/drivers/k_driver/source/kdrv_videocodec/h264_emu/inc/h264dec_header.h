#ifndef _H264DEC_HEADER_H_
#define _H264DEC_HEADER_H_

#include "kwrap/type.h"

#include "h26x_bitstream.h"
#include "h264dec_api.h"

typedef struct _H264DecSpsObj{
    UINT32 uiProfileIdc;
    UINT32 uiLevelIdc;
    UINT32 uiSpsId;
    UINT32 uiChormaFormatIdc;
    UINT32 uiBitDepthLumaMinus8;
    UINT32 uiBitDepthChromaMinus8;
    UINT32 uiLog2MaxFrmMinus4;
    UINT32 uiPocType;
    UINT32 uiLog2MaxPocLsbMinus4;
    UINT32 uiNumRefFrm;
    UINT32 uiMbWidth;
    UINT32 uiMbHeight;
    UINT32 uiFrmMbsOnly;
    UINT32 uiTrans8x8;
    UINT32 uiCropping;
    UINT32 uiCropLeftOffset;
    UINT32 uiCropRightOffset;
    UINT32 uiCropTopOffset;
    UINT32 uiCropBtmOffset;
    UINT32 uiVuiParameterPresentFlag;
}H264DecSpsObj;

typedef struct _H264DecPpsObj{
    UINT32 uiPpsId;
    UINT32 uiSpsId;
    UINT32 uiEntropyCoding;
    UINT32 uiNumRefIdxL0Minus1;
    UINT32 uiNumRefIdxL1Minus1;
    UINT32 uiPicInitQPMinus26;
    UINT32 uiPicInitQSMinus26;
    UINT32 uiChromQPIdxoffset;
    UINT32 uiDelockingFilterControlPresentFlag;
    UINT32 uiPicTrans8x8Mode;
    UINT32 uiPicSecondChormaQPIdxoffset;
}H264DecPpsObj;

typedef struct _H264DecMMCO_{
	UINT32 uiEnable;
	UINT32 uiSTPicNum;
	UINT32 uiLTPicNum;
}H264DecMMCO;

typedef struct _H264DecSliceObj{
    UINT32 uiNalRefIdc;
    UINT32 uiNalUnitType;
    UINT32 uiFirstMbInSlice;
    SLICE_TYPE eSliceType;
    SLICE_TYPE ePreSliceType;
    UINT32 uiPpsId;
    UINT32 uiFrmNum;
    UINT32 uiIdrFlag;
    UINT32 uiPreIdrFlag;
    UINT32 uiIdrPicId;
    INT32 iPocLsb;
    INT32 iPoc;
    UINT32 uiDirectModeType;
    UINT32 uiNumRefIdxL0;
    UINT32 uiNumRefIdxL1;
    UINT32 uiRefPicListReorderL0;
    UINT32 uiRefPicListReorderL1;
    UINT32 uiCabacInitIdc;
    UINT32 uiQPDelta;
    UINT32 uiQP;
    UINT32 uiDisableDBFilterIdc;
    UINT32 uiAlphaC0OffsetDiv2;
    UINT32 uiBetaOffsetDiv2;
    UINT32 uiAbsDiffPicNumL0;
    UINT32 uiAbsDiffPicNumL1;
    UINT32 uiLongTermPicNum;
    UINT32 uipreAbsDiffPicNumL0;

	H264DecMMCO sMMCO;
}H264DecSliceObj;


typedef struct _H264DecSVCObj{
    UINT32 uiSVCFlag;
    UINT32 uiLayer;
    UINT32 uiTemporalId[3];
    UINT32 uiFrmRateInfoFlag;
    UINT32 uiAvgFrmRate[3];
    UINT32 uiFrmSizeInfoFlag;
    UINT32 uiWidth;
    UINT32 uiHeight;
    UINT32 uiTid;       // every frame's temporal id
}H264DecSVCObj;

#define DecNalBufSize 512
typedef struct _H264DecHdrObj{
    H264DecSpsObj   sH264DecSpsObj;
    H264DecPpsObj   sH264DecPpsObj;
    H264DecSliceObj sH264DecSliceObj;
    H264DecSVCObj   sH264DecSvcObj;
	UINT8 ucH264DecNalBuf[DecNalBufSize];
}H264DecHdrObj;

extern INT32 H264DecSps(H264DecSpsObj *pH264DecSpsObj,bstream *pBstream, H26XDEC_VUI *pVui);
extern INT32 H264DecPps(H264DecSpsObj *pH264DecSpsObj,H264DecPpsObj *pH264DecPpsObj,bstream *pBstream);
extern INT32 H264DecSlice(H264DEC_INFO *pH264DecInfo,H264DecHdrObj *pH264DecHdrObj,UINT32 uiBsAddr,UINT32 uiBsLen);
extern INT32 H264DecSeqHeader(H264DEC_INIT *pH264DecInit,H264DecHdrObj *pH264DecHdrObj, H26XDEC_VUI *pVui);

#endif
