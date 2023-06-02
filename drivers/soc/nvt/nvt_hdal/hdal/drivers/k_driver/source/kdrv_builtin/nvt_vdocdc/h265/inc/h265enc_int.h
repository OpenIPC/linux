#ifndef _H265_INT_H_
#define _H265_INT_H_

#include "kwrap/type.h"

#include "h26x.h"
#include "h26x_def.h"
#include "h26x_common.h"

#include "h265_def.h"
#include "h265enc_api.h"

#define RC_RDMODEL_WIN_SIZE 16
#define MAX_SUB_LAYER_SIZE (7)
#define MAX_ST_RPS_SIZE                (64)
#define MAX_NUM_REF_PICS               (16)

typedef struct _H265EncSao_ {
	UINT32 sao_en;           //sample_adaptive_offset_enabled_flag
	UINT32 sao_luma_flag;
	UINT32 sao_chroma_flag;
	UINT32 sao_merge_Penalty;
	UINT32 sao_boundary;
} H265EncSao;

typedef struct _H265EncIlf_ {
	UINT32  ilf_db_en;
	UINT32  ilf_db_disable;          //deblocking_filter_disabled_flag
	UINT32  db_control_present;      //deblocking_filter_control_present_flag
	UINT32  llf_bata_offset_div2;
	UINT32  llf_alpha_c0_offset_div2;
	UINT32  ilf_db_override_en;      //deblocking_filter_override_enabled_flag
	UINT32  ilf_across_slice_en;     //loop_filter_across_slices_enabled_flag
	UINT32  ilf_across_tile_en;     //loop_filter_across_tile_enabled_flag
} H265EncIlf;

typedef struct {
	UINT32 NumNegativePics;
	//UINT32 NumPositivePics;
	UINT32 NumDeltaPocs;
	INT32 DeltaPocS0[MAX_NUM_REF_PICS]; //ue(v)
	//INT32 DeltaPocS1[MAX_NUM_REF_PICS]; //ue(v)

	UINT16 UsedByCurrPicS0[MAX_NUM_REF_PICS]; //MAX_NUM_REF_PICS bits
	//UINT16 UsedByCurrPicS1[MAX_NUM_REF_PICS]; //MAX_NUM_REF_PICS bits

	UINT32 curr_num_delta_pocs;
} STRPS; //short_term_ref_pic_set


typedef struct _H265EncSeqCfg_{
    UINT16	usWidth;
    UINT16	usHeight;
	UINT16  usLcuWidth;
	UINT16  usLcuHeight;
	UINT32  uiTotalLCUs;

	UINT32	uiDisplayWidth;
	UINT32	uiGopNum;
    UINT16	usLog2MaxFrm;
    UINT16	usLog2MaxPoc;
	INT16   cPocLsb[2];
	BOOL    bTileEn; //tiles_enabled_flag
	UINT8   ucTileNum;	// TODO : for saving tile number in each frame //
    UINT32  uiTileWidth[H26X_MAX_TILE_NUM];

    BOOL    bSimpleMergeDis;
    BOOL    bTu4Dis;

	// reference frame //
    UINT8	ucSVCLayer;
	//UINT32	uiLTRInterval;
	//BOOL	bLTRPreRef;
	UINT8	ucNumRefIdxL0;

	// rate control //
	UINT32	uiBitRate;
    UINT32	uiFrmRate;
	UINT8	ucInitIQP;
    UINT8	ucInitPQP;

	// function //
	BOOL	bFBCEn;
	BOOL	bGrayEn;
	BOOL	bFastSearchEn;
	BOOL	bHwPaddingEn;
	UINT8	ucRotate;
    BOOL    bD2dEn;
    BOOL    bGdcEn;
    BOOL    bColMvEn;

	H265EncSao sao_attr;
    H265EncIlf ilf_attr;

	// header //
	BOOL	bVUIEn;
    UINT16	usSarWidth;
    UINT16	usSarHeight;
    UINT8   ucMatrixCoef;
    UINT8   ucTransferCharacteristics;
    UINT8   ucColourPrimaries;
    UINT8   ucVideoFormat;
    UINT8   ucColorRange;
    UINT8   bTimeingPresentFlag;

    UINT32  uiNumStRps;                // num_short_term_ref_pic_sets
    UINT32  uiLTRpicFlg;               // long_term_ref_pics_present_flag
    UINT32  uiNumLTRpsSps;             // num_long_term_ref_pics_sps
    UINT32  uiTempMvpEn;               // sps_temporal_mvp_enabled_flag
    UINT32  uiSliceTempMvpEn;          // slice_temporal_mvp_enabled_flag
    UINT32  uiChromaIdc;               // chroma_format_idc
    UINT32  uiChromaArrayType;
    UINT32  uiSepColorPlaneFlg;        // separate_colour_plane_flag
    UINT32  uiUsrQpSize;               // 0 : 64x64, 1 : 16x16
    UINT32  uiMaxSubLayerMinus1;       // vps_max_sub_layers_minus1, sps_max_sub_layers_minus1
    UINT32  uiTempIdNestFlg;           // vps_temporal_id_nesting_flag, sps_temporal_id_nesting_flag
    UINT32  iRefFrmNum;
    STRPS   short_term_ref_pic_set[MAX_ST_RPS_SIZE];
    UINT32  uiSTRpsIdx;                // slice stRpsIdx
    INT32   iPocLT[MAX_NUM_REF_PICS];  // slice Poc lt
    UINT32  uiUsedbyCurrLTFlag[MAX_NUM_REF_PICS]; // used_by_curr_pic_lt_flag[ i ]
    UINT32  uiLTInterval;              // long term interval
    UINT32  uiLTRPreRef;
    UINT32  uiNumLTPic;                // num_long_term_pics
    UINT32  uilayer_order_cnt;
    UINT32  uiDistFactor;
    UINT32  uiEn2xP;
    UINT32  uiEn4xP;
    UINT32  bEnableLt;                 // Big P frame
    UINT32  u32LtInterval;             // Big P frame interval

    UINT32  Tid1x;
    UINT32  Tid2x;
    UINT32  Tid4x;
    UINT32  TidLt;

	UINT8  ucLevelIdc;
	// SEI //
	UINT8  ucSEIIdfEn;
	// MDBitMap //
	UINT32  uiPrjMode;
	// quality level, for setting tile and flexible search range
	HEVC_QLVL_TYPE eQLevel;
}H265EncSeqCfg;

typedef struct _H265EncPicCfg_{
	SLICE_TYPE ePicType;

    UINT8	ucSliceQP;

    //UINT32	uiPicCnt;          ///< every picture ++
    UINT32	uiPicCntInGop;     ///< Nth picture in gop

	UINT16  usFrmNum;
	UINT16  usFrmForIdrPicId;
	INT16	iPoc;
	UINT32  uiNthInGop;        ///< Nth picture in gop

	UINT32  uiTemporalId[2];

	// NT96510 //
	INT32   iQpCbOffset;        // -12 ~ 12
	INT32   iQpCrOffset;        // -12 ~ 12
	UINT32  uiFrameSkip;
	CU16_QP_MODE uiUsrQpMode;

    UINT32  uiTileSizeSideInfo[H26X_MAX_TILE_NUM];

	UINT32  uiDepSlcEn;  //dependent_slice_segments_enabled_flag
	UINT32  uiNumExSlcHdrBit; //num_extra_slice_header_bits
	UINT32  uiOutPresFlg; //output_flag_present_flag
	UINT32  uiListModPreFlg; //lists_modification_present_flag
	UINT32  uiCabacInitPreFlg; //cabac_init_present_flag
	UINT32  uiWeiPredFlg; //weighted_pred_flag
	UINT32  uiPpsSlcChrQpOfsPreFlg;//pps_slice_chroma_qp_offsets_present_flag
	UINT32  uiEntroCodSyncEn;//entropy_coding_sync_enabled_flag
	UINT32  uiSlcSegHdrExtPreFlg;//slice_segment_header_extension_present_flag
	UINT32  uiCuQpDeltaEnabledFlag; //cu_qp_delta_enabled_flag

	UINT32	uiSrcTimeStamp;
	UINT32  uiLastRecIsLT;
	UINT32  uiColRefIsLT;
	UINT32  uiLastRec;

	UINT32	uiPicHdrLen;
	SLICE_TYPE eNxtPicType;
	UINT32  uiEncRatio;
	UINT32  uiPrePicCnt;

    UINT32  uiRecExtraStatus;
    UINT32  uiRecExtraEnable;

	INT32  iRefAndRecPOC[FRM_IDX_MAX];
	INT32  iColRefAndRecPOC[FRM_IDX_MAX];
	UINT32	uiRefAndRecIsIntra[FRM_IDX_MAX];
	UINT32	uiRefAndRecIsLT[FRM_IDX_MAX];
    BOOL   bFBCEn[FRM_IDX_MAX];

}H265EncPicCfg;

typedef struct _H265ENC_CTX_{
    H265EncSeqCfg	stSeqCfg;
    H265EncPicCfg	stPicCfg;
	H265EncRdo   stRdo;
	H265EncFroCfg   stFroCfg;
    H265EncQpRelatedCfg stQPCfg;
    H265EncTileCfg  stTileCfg;

	UINT32 uiRecYLineOffset;
	UINT32 uiRecCLineOffset;
	UINT32 uiSideInfoLineOffset[H26X_MAX_TILE_NUM];
    UINT32 uiSizeSideInfo[H26X_MAX_TILE_NUM];

	H26XFRM_IDX eRecIdx;
	H26XFRM_IDX eRefIdx;

	UINT32 uiSeqHdrLen;
	UINT32 uiVPSHdrLen;
	UINT32 uiSPSHdrLen;
	UINT32 uiPPSHdrLen;
	UINT32 uiSvcLable;
#if 0
	// rate control //
	H265EncRCParam  sH265EncRcParam;
	H26XEncRcCBR    sH265EncRcCBRCfg;
	H26XEncRcVBR    sH265EncRcVBRCfg;
	H26XEncRcFixQP  sH265EncRcFixQpCfg;

	// use avc + hevc common struct definition //
	H26XEncAddr  sH265VirEncAddr;
    H26XRegSet   sH265EncRegSet;
#endif
} H265ENC_CTX;

extern INT32 h265Enc_initCfg(H265ENC_INIT *pInit, H26XENC_VAR *pVar, H265EncTileCfg *pTileCfg);
extern INT32 h265Enc_preparePicCfg(H265ENC_CTX *pVdoCtx, H265ENC_INFO *pInfo, H26XENC_VAR *pVar);
extern void h265Enc_updatePicCnt(H26XENC_VAR *pVar);
extern void h265Enc_modifyRefFrm(H265ENC_CTX *pVdoCtx, H26XENC_VAR *pVar, BOOL bFBCEn);

extern void h265Enc_updateRowRcCfg(H26XENC_VAR *pVar);
#endif

