#ifndef _VDOENC_BUILTIN_H
#define _VDOENC_BUILTIN_H
#if defined(__KERNEL__)

#include "kwrap/type.h"
#include "kdrv_ipp_builtin.h"
#include "sie_init.h"

typedef KDRV_IPP_BUILTIN_FMD_CB_INFO VDOENC_BUILTIN_FMD_INFO;

#define VDOENC_BUILTIN_YUVQ_MAX            2
#define VDOENC_MD_MAP_MAX_SIZE             8196           ///< same value as NMR_VDOENC_MD_MAP_MAX_SIZE
#define VDOENC_BUF_RESERVED_BYTES          4              ///< same value as NMR_VDOENC_BUF_RESERVED_BYTES
#define VDOENC_BUILTIN_JOBQ_MAX            150

#define ISP_ID_MAX 5 ///< need to define in sie_init.h

#define VDOENC_BUILTIN_TSK_PRI          3               // task priority, SIE(2) > IPP(3) = VDOENC(3)

#define TNR_DBG 0
#define SPN_DBG 0

#define TNR_ENABLE 0
#define SPN_ENABLE 1

typedef enum {
	BUILTIN_VDOENC_H265,
	BUILTIN_VDOENC_H264,
	BUILTIN_VDOENC_MJPEG,
} BUILTIN_VDOENC_CODEC;

typedef enum {
	// module 1
	BUILTIN_VDOENC_INIT_PARAM_ENC_EN,
	BUILTIN_VDOENC_INIT_PARAM_CODEC,
	BUILTIN_VDOENC_INIT_PARAM_DIRECT,
	BUILTIN_VDOENC_INIT_PARAM_WIDTH,
	BUILTIN_VDOENC_INIT_PARAM_HEIGHT,
	BUILTIN_VDOENC_INIT_PARAM_FRAME_RATE,
	BUILTIN_VDOENC_INIT_PARAM_PROFILE,
	BUILTIN_VDOENC_INIT_PARAM_LEVEL_IDC,
	BUILTIN_VDOENC_INIT_PARAM_GOP_NUM,
	BUILTIN_VDOENC_INIT_PARAM_LTR_INTERVAL,
	BUILTIN_VDOENC_INIT_PARAM_LTR_PRE_REF,
	BUILTIN_VDOENC_INIT_PARAM_GRAY_EN,
	BUILTIN_VDOENC_INIT_PARAM_SRC_OUT,
	BUILTIN_VDOENC_INIT_PARAM_SVC,
	BUILTIN_VDOENC_INIT_PARAM_ENTROPY,
	BUILTIN_VDOENC_INIT_PARAM_BSQ_MAX,
	BUILTIN_VDOENC_INIT_PARAM_SEC,
	BUILTIN_VDOENC_INIT_PARAM_JPEG_QUALITY,
	BUILTIN_VDOENC_INIT_PARAM_JPEG_FPS,
	BUILTIN_VDOENC_INIT_PARAM_JPEG_MAX_MEM_SIZE,
	BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_CODEC,
	BUILTIN_VDOENC_INIT_PARAM_BS_START,
	BUILTIN_VDOENC_INIT_PARAM_BS_END,
} BUILTIN_VDOENC_INIT_PARAM;

typedef enum {
	BUILTIN_VDOENC_DTSI_PARAM_ENC_EN,
	BUILTIN_VDOENC_DTSI_PARAM_CODECTYPE,
	BUILTIN_VDOENC_DTSI_PARAM_SRC_VPRC_DEV,
	BUILTIN_VDOENC_DTSI_PARAM_SRC_VPRC_PATH,
	BUILTIN_VDOENC_DTSI_PARAM_BITRATE,
	BUILTIN_VDOENC_DTSI_PARAM_FRAMERATE,
	BUILTIN_VDOENC_DTSI_PARAM_SEC,
	BUILTIN_VDOENC_DTSI_PARAM_GOP,
	BUILTIN_VDOENC_DTSI_PARAM_QP,
	BUILTIN_VDOENC_DTSI_PARAM_JPEG_QUALITY,
	BUILTIN_VDOENC_DTSI_PARAM_JPEG_FPS,
	BUILTIN_VDOENC_DTSI_PARAM_SVC_LAYER,
	BUILTIN_VDOENC_DTSI_PARAM_LTR_INTERVAL,
	BUILTIN_VDOENC_DTSI_PARAM_D2D,
	BUILTIN_VDOENC_DTSI_PARAM_GDC,
	BUILTIN_VDOENC_DTSI_PARAM_COLMV,
	BUILTIN_VDOENC_DTSI_PARAM_QUALITY_LV,
	BUILTIN_VDOENC_DTSI_PARAM_ISP_ID,
} BUILTIN_VDOENC_DTSI_PARAM;

typedef enum {
	BUILTIN_VDOENC_PATH_ID_0 = 0,
	BUILTIN_VDOENC_PATH_ID_1 = 1,
	BUILTIN_VDOENC_PATH_ID_2 = 2,
	BUILTIN_VDOENC_PATH_ID_3 = 3,
	BUILTIN_VDOENC_PATH_ID_4 = 4,
	BUILTIN_VDOENC_PATH_ID_5 = 5,
	BUILTIN_VDOENC_PATH_ID_MAX = 6,
} BUILTIN_VDOENC_PATH_ID;

typedef enum {
	BUILTIN_VDOENC_ISP_ITEM_TNR   = 0,      // support get/set, using KDRV_VDOENC_3DNR struct
	BUILTIN_VDOENC_ISP_ITEM_RATIO = 1,      // support get    , using KDRV_VDOENC_ISP_RATIO struct
	BUILTIN_VDOENC_ISP_ITEM_SPN   = 2,      // support get/set, using KDRV_H26XENC_SPN struct
} BUILTIN_VDOENC_ISP_ITEM;

typedef enum {
	BUILTIN_ISP_EVENT_NONE					= 0x00000000,
	BUILTIN_ISP_EVENT_ENC_TNR				= 0x00400000,   ///< ENC 3DNR
	BUILTIN_ISP_EVENT_ENC_SHARPEN			= 0x01000000,	///< ENC Sharpen
} BUILTIN_ISP_EVENT;

typedef struct {
	/* ime output buffer info */
	UINT32 width;
	UINT32 height;
	UINT32 fmt;

	/* venc internal buffer info */
	UINT32 max_blk_addr;
	UINT32 max_blk_size;
} VDOENC_BUILTIN_INIT_INFO;

typedef struct {
	UINT32 codec_mem_addr;
	UINT32 codec_mem_size;
	UINT32 bs_start_addr;                               ///< [w] output bit-stream start address if enc space not enough
	UINT32 bs_addr_1;                                   ///< [w] 1st output bit-stream address
	UINT32 bs_size_1;                                   ///< [w] 1st output bit-stream size
	UINT32 bs_end_addr;                                 ///< [w] output bit-stream end address
	UINT32 bs_min_i_size;
	UINT32 bs_min_p_size;
	UINT32 interrupt;
	UINT32 timestamp;
	UINT32 base_qp;
	UINT32 frm_type;
} VDOENC_BUILTIN_PARAM;

typedef struct {
	UINT32 y_addr;
	UINT32 c_addr;
	UINT32 width;
	UINT32 height;
	UINT32 y_line_offset;
	UINT32 c_line_offset;
	UINT32 timestamp;
	BOOL   release_flag;
} VDOENC_BUILTIN_YUV_INFO;

typedef struct {
	UINT32 Addr;    							//BS buffer starting address
	UINT32 Size;    							//BS buffer size
	UINT32 temproal_id;
	UINT32 re_encode_en;
	UINT32 timestamp;
	UINT32 nxt_frm_type;
	UINT32 base_qp;
	UINT32 bs_size_1;
	UINT32 frm_type;
	UINT32 encode_time;
	BOOL isKeyFrame;
} VDOENC_BUILTIN_BS_INFO, *PVDOENC_BUILTIN_BS_INFO;

typedef struct {
	UINT32 pathID;
} VDOENC_BUILTIN_JOB_INFO, *PVDOENC_BUILTIN_JOB_INFO;

typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	VDOENC_BUILTIN_YUV_INFO     Queue[VDOENC_BUILTIN_YUVQ_MAX];
} VDOENC_BUILTIN_YUVQ, *PVDOENC_BUILTIN_YUVQ;

typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	VDOENC_BUILTIN_BS_INFO      *Queue;
} VDOENC_BUILTIN_BSQ, *PVDOENC_BUILTIN_BSQ;

typedef struct {
	VDOENC_BUILTIN_YUVQ           yuvQueue;
	VDOENC_BUILTIN_BSQ            bsQueue;
	VDOENC_BUILTIN_PARAM          venc_param;
} VDOENC_BUILTIN_OBJ, *PVDOENC_BUILTIN_OBJ;

typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	VDOENC_BUILTIN_JOB_INFO     *Queue;
} VDOENC_BUILTIN_JOBQ, *PVDOENC_BUILTIN_JOBQ;

typedef struct {
    BOOL  bEnable;          ///< [r/w] Enable sharpen
    UINT8 ucConEng;         ///< [r/w] The weight of the local contrast  0~8
    UINT8 ucSlopConEng;     ///< [r/w] Transition region slop 0~255
    UINT8 ucBHC;            ///< [r/w] Bright halo clip ratio 0~128
    UINT8 ucDHC;            ///< [r/w] Dark halo clip ratio 0~128
    UINT8 ucEWT;            ///< [r/w] Edge weight coring threshold 0~255
    UINT8 ucEWG;            ///< [r/w] Edge weight gain 0~255
    UINT8 ucEdgeSharpStr1;  ///< [r/w] Sharpen strength 1 of edge region 0~255
    UINT8 ucCT;             ///< [r/w] Coring threshold 0~255
    UINT8 ucNL;             ///< [r/w] Noise level 0~255
    UINT8 ucBIG;            ///< [r/w] Blending ratio of HPF results 0~128
    UINT16 usFlatTh;        ///< [r/w] flat region threshold 0~2047
    UINT16 usEdgeTh;        ///< [r/w] Edge region threshold 0~2047
    UINT8 ucLowWt;          ///< [r/w] Region classification weight threshold 1 0~16
    UINT8 ucHightWt;        ///< [r/w] Region classification weight threshold 2 0~16
    UINT16 usHLDFlatTh;     ///< [r/w] High light flat region threshold 0~2047
    UINT16 usHLDEdgeTh;     ///< [r/w] High light edge threshold 0~2047
    UINT8 ucHLDLowWt;       ///< [r/w] High light reigon classification weight threshold 1 0~16
    UINT8 ucHLDHightWt;     ///< [r/w] High light region classification weight threshold 2 0~16
    UINT8 ucEdgeStr;        ///< [r/w] Edge weight strength 0~255
    UINT8 ucTransitionStr;  ///< [r/w] Transition region weight strength 0~255
    UINT8 ucMotionStr;      ///< [r/w] Motion region edge weight 0~255
    UINT8 ucStaticStr;      ///< [r/w] Static region edge weight strength 0~255
    UINT8 ucHLDSlopConEng;  ///< [r/w] High light transition region slop 0~255
    UINT8 ucFlatStr;        ///< [r/w] Flag region weight strength 0~16
    UINT16 usHLDLumTh;      ///< [r/w] High light luminance  threshold 0~255
    UINT8 ucNC[17];         ///< [r/w] 17 control points of noise modulation curve
} VDOENC_BUILTIN_H26XENC_SPN;

int VdoEnc_builtin_get_dtsi_param(UINT32 pathID, BUILTIN_VDOENC_DTSI_PARAM param);
int  H264Enc_builtin_init(UINT32 pathID, VDOENC_BUILTIN_INIT_INFO *p_info);
int  H265Enc_builtin_init(UINT32 pathID, VDOENC_BUILTIN_INIT_INFO *p_info);
int  MJPGEnc_builtin_init(UINT32 pathID, VDOENC_BUILTIN_INIT_INFO *p_info);
void VdoEnc_BuiltIn_trig(VDOENC_BUILTIN_FMD_INFO *p_info, UINT32 reserved);
int VdoEnc_BuiltIn_TskStart_H26X(void);
int VdoEnc_BuiltIn_TskStop_H26X(void);
int VdoEnc_BuiltIn_TskStart_JPEG(void);
int VdoEnc_BuiltIn_TskStop_JPEG(void);
void VdoEnc_BuiltIn_Install_ID(void);
void VdoEnc_BuiltIn_Uninstall_ID(void);
int  VdoEnc_builtin_init(VDOENC_BUILTIN_INIT_INFO *p_info);

extern BOOL VdoEnc_Builtin_GetBS(UINT32 pathID, VDOENC_BUILTIN_BS_INFO *builtin_bs_info);
extern UINT32 VdoEnc_Builtin_HowManyInBSQ(UINT32 pathID);
extern BOOL VdoEnc_Builtin_GetEncVar(UINT32 pathID, void *kdrv_vdoenc_var);
extern UINT32 VdoEnc_Builtin_SetParam(UINT32 pathID, UINT32 Param, UINT32 Value);
extern UINT32 VdoEnc_Builtin_GetParam(UINT32 pathID, UINT32 Param, UINT32 *pValue);
extern UINT32 VdoEnc_Builtin_CheckBuiltinStop(UINT32 pathID);
extern void VdoEnc_Builtin_FreeQueMem(UINT32 pathID);
extern BOOL vdoenc_builtin_evt_fp_reg(CHAR *name, SIE_FB_ISR_FP fp);
extern BOOL vdoenc_builtin_evt_fp_unreg(CHAR *name);
extern BOOL vdoenc_builtin_set(UINT32 id, BUILTIN_VDOENC_ISP_ITEM item, void *data);
#endif // __KERNEL__
#endif //_VDOENC_BUILTIN_H