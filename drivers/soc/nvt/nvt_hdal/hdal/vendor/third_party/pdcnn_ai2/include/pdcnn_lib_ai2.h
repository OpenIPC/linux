/********************************************************************
	INCLUDE FILES
********************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_type.h"
#include "hd_common.h"
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "hd_gfx.h"

#include "vendor_ai.h"
#include "kflow_ai_net/kflow_ai_net.h"

#define NN_PDCNN_DRAW           ENABLE
#if 0
#define PD_MAX_DISTANCE_MODE    2
#endif
#define LIMIT_FDET_PD           ENABLE

#define PD_MAX_MEM_SIZE 	    (15 * 1024 * 1024)
#define PD_SCALE_BUF_SIZE       (2 * 1024 * 576)

#define PD_YUV_WIDTH			960
#define PD_YUV_HEIGHT			540
#define PD_CLS_WIDTH			96
#define PD_CLS_HEIGHT			96
#define PD_MAX_FRAME_WIDTH		2560
#define PD_MAX_FRAME_HEIGHT		1440
#if (PD_YUV_WIDTH == 1024)
#define PD_NEON_BATCH			128
#else
#define PD_NEON_BATCH			120 //144 or 120
#endif

#define PD_VERSION_A  "1"
#define PD_VERSION_B  "8"
#define PD_VERSION_C  "3"

typedef struct _PD_BACKBONE_PRANCH_OUT {
	VENDOR_AIS_FLOW_MEM_PARM cls_prob_layer;
	VENDOR_AIS_FLOW_MEM_PARM bbox_pred_layer;	
} PD_BACKBONE_PRANCH_OUT;

typedef struct _PD_BACKBONE_OUT {
	PD_BACKBONE_PRANCH_OUT stride32;
	PD_BACKBONE_PRANCH_OUT stride16;
	PD_BACKBONE_PRANCH_OUT stride8;
} PD_BACKBONE_OUT;

typedef struct _PD_LIMIT_PARAM {
	FLOAT score_thrs[25];
	INT32 box_sizes[25];
	INT32 limit_size;
	INT32 sm_thr_num;
	FLOAT k[25];
	INT32 limit_module;
	UINT32 max_distance;
	UINT32 limit_fdet;
} PD_LIMIT_PARAM;


typedef struct _PD_PROPOSAL_PARAM {
	INT32 backbone_out_shifts[3][2];
	INT32 backbone_out_sizes[3][2];
	INT32 backbone_out_indexs[3][2];
	FLOAT anchor_params[3][8][4];
	INT32 strides[3];
	INT32 input_size[2];
	FLOAT score_thres;
	FLOAT nms_thres;
	FLOAT ratiow;
	FLOAT ratioh;
	UINT32 run_id;
	INT32 open_pdout;
	INT32 cont_pre_rslt;
} PD_PROPOSAL_PARAM;

typedef struct _PDCNN_RESULT
{
	INT32   category;
	FLOAT 	score;
	FLOAT 	x1;
	FLOAT 	y1;
	FLOAT 	x2;
	FLOAT 	y2;
}PDCNN_RESULT;

typedef struct _PD_IRECT {
	FLOAT  x1;                           ///< x coordinate of the top-left point of the rectangle
	FLOAT  y1;                           ///< y coordinate of the top-left point of the rectangle
	FLOAT  x2;                           ///< rectangle width
	FLOAT  y2;                           ///< rectangle height
} PD_IRECT;

typedef struct _PDCNN_MEM {
    INT32 out_num;
	VENDOR_AIS_FLOW_MEM_PARM io_mem;
	VENDOR_AIS_FLOW_MEM_PARM model_mem;
	VENDOR_AIS_FLOW_MEM_PARM input_mem;
	VENDOR_AIS_FLOW_MEM_PARM pps_result;
    VENDOR_AIS_FLOW_MEM_PARM out_result;
	VENDOR_AIS_FLOW_MEM_PARM final_result;
	VENDOR_AIS_FLOW_MEM_PARM backbone_output;
	VENDOR_AIS_FLOW_MEM_PARM reshape_cls;
	VENDOR_AIS_FLOW_MEM_PARM scale_buf;
} PDCNN_MEM;

VOID pdcnn_get_version(VOID);
HD_RESULT pdcnn_version_check(VENDOR_AIS_FLOW_MEM_PARM *buf);
VOID pd_gfx_img_to_vendor(VENDOR_AI_BUF* out_img, HD_GFX_IMG_BUF* in_img, UINT32 va);
HD_RESULT pdcnn_crop_img(HD_GFX_IMG_BUF *dst_img, VENDOR_AI_BUF *src_img, HD_GFX_SCALE_QUALITY method, PD_IRECT *roi);
HD_RESULT pdcnn_init(PD_PROPOSAL_PARAM* proposal_params, PD_BACKBONE_OUT* backbone_outputs, PD_LIMIT_PARAM* limit_para, CHAR *para_file);
HD_RESULT pdcnn_process(PD_PROPOSAL_PARAM* proposal_params, PDCNN_MEM* pdcnn_mem, PD_LIMIT_PARAM* limit_para, PD_BACKBONE_OUT* backbone_outputs, VENDOR_AI_BUF* src_img, UINT32 max_distance);
VOID print_pdcnn_results(PDCNN_MEM* pdcnn_mem, FLOAT ratiow, FLOAT ratioh);
HD_RESULT get_pd_mem(VENDOR_AIS_FLOW_MEM_PARM *buf, VENDOR_AIS_FLOW_MEM_PARM *req_mem, UINT32 req_size, UINT32 align_size);
HD_RESULT get_pd_post_mem(VENDOR_AIS_FLOW_MEM_PARM* buf, PDCNN_MEM *pd_mem);
UINT32 pdcnn_nonmax_suppress(PDCNN_RESULT *p_boxes, INT32 num, FLOAT ratio, INT32 method, INT32 after_num);
