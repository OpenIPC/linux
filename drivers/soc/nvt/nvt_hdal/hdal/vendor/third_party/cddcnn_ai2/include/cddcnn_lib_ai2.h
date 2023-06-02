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

#include "vendor_ai.h"
#include "kflow_ai_net/kflow_ai_net.h"
	
#include <arm_neon.h>


#define NN_CDDCNN_DRAW           ENABLE
#define LIMIT_FDET_CDD           ENABLE



#define CDD_PPS_MAX_NUM          (1000)

#define KBYTE                   (1024)
#define MBYTE					(1024 * 1024)

#define CDD_MAX_MEM_SIZE 	    (10 * MBYTE)
#define CDD_IO_MAX_MEM			(8 * MBYTE / 2)
#define SCALE_BUF_SIZE          (1 * MBYTE)

#define MAX_CLASS_OBJ_NUM		(1024)
#define YUV_WIDTH			1024
#define YUV_HEIGHT			576
#define MAX_FRAME_WIDTH		1920
#define MAX_FRAME_HEIGHT	1080
#if (YUV_WIDTH == 1024)
#define NEON_BATCH			128
#else
#define NEON_BATCH			144
#endif

#define ROI_MODE1_WIDTH          	        680 //640 or 600, 600 or 560
#define ROI_MODE1_HEIGHT  			        384
#define ROI_MODE2_WIDTH          	        640 //640 or 600, 600 or 560
#define ROI_MODE2_HEIGHT  			        360
#define ROI_MODE3_WIDTH 					600
#define ROI_MODE3_HEIGHT					336


#define SOURCE_PATH 			HD_VIDEOPROC_0_OUT_1
#define EXTEND_PATH				HD_VIDEOPROC_0_OUT_6
#define OSG_LCD_WIDTH       	(960)
#define OSG_LCD_HEIGHT      	(240)


#define MAX_FEATURE_WIDTH       (160)
#define MAX_FEATURE_HEIGHT      (92)
#define CLS_LAYER_OUT_CHANNEL   (12)

typedef struct _BACKBONE_ONE_PRANCH_OUT {
	VENDOR_AIS_FLOW_MEM_PARM cls_prob_layer;
	VENDOR_AIS_FLOW_MEM_PARM bbox_pred_layer;	
} BACKBONE_ONE_PRANCH_OUT;


typedef struct _BACKBONE_OUT {
	BACKBONE_ONE_PRANCH_OUT stride32;
	BACKBONE_ONE_PRANCH_OUT stride16;
	BACKBONE_ONE_PRANCH_OUT stride8;
} BACKBONE_OUT;

typedef struct _LIMIT_PARAM {
	FLOAT score_thrs[25];
	INT32 box_sizes[25];
	FLOAT ratiow;
	FLOAT ratioh;
	INT32 limit_size;
	INT32 sm_thr_num;
	FLOAT k[25];
	INT32 limit_module;
	UINT32 max_distance;
} LIMIT_PARAM;


typedef struct _PROPOSAL_PARAM {
	INT32 backbone_out_shifts[3][2];
	INT32 backbone_out_sizes[3][2];
	INT32 backbone_out_indexs[3][2];
	FLOAT anchor_params[3][6][4];
	INT32 strides[3];
	INT32 input_size[2];
	FLOAT score_thres;
	FLOAT nms_thres;
	UINT32 run_id;
	INT32 open_cddout;
	INT32 cont_pre_rslt;
} PROPOSAL_PARAM;

typedef struct _CDDCNN_RESULT
{
	INT32   category;
	FLOAT 	score;
	FLOAT 	x1;
	FLOAT 	y1;
	FLOAT 	x2;
	FLOAT 	y2;
}CDDCNN_RESULT;

typedef struct _CDD_IRECT {
	FLOAT  x1;                           ///< x coordinate of the top-left point of the rectangle
	FLOAT  y1;                           ///< y coordinate of the top-left point of the rectangle
	FLOAT  x2;                           ///< rectangle width
	FLOAT  y2;                           ///< rectangle height
} CDD_IRECT;

typedef struct _CDDCNN_MEM {
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
} CDDCNN_MEM;

/*typedef struct _CDDCNN_MEM {
	INT32 out_num;
	MEM_PARM io_mem;
	MEM_PARM model_mem;
	MEM_PARM input_mem;
	MEM_PARM pps_result;
	MEM_PARM out_result;
	MEM_PARM backbone_output;
	MEM_PARM reshape_cls;
	MEM_PARM final_result;
	MEM_PARM scale_buf;
	MEM_PARM cls_mem;
	MEM_PARM cls_io_mem;
	MEM_PARM cls_input_buf;
} CDDCNN_MEM;*/






VOID gfx_img_to_vendor(VENDOR_AI_BUF* out_img, HD_GFX_IMG_BUF* in_img, UINT32 va);
HD_RESULT ai_crop_img(HD_GFX_IMG_BUF *dst_img, VENDOR_AI_BUF *src_img, HD_GFX_SCALE_QUALITY method, CDD_IRECT *roi);
HD_RESULT cddcnn_init(PROPOSAL_PARAM* proposal_params, BACKBONE_OUT* backbone_outputs, LIMIT_PARAM* limit_para, CHAR *para_file);
HD_RESULT cddcnn_process(PROPOSAL_PARAM* proposal_params, CDDCNN_MEM* cddcnn_mem, LIMIT_PARAM* limit_para, BACKBONE_OUT* backbone_outputs, VENDOR_AI_BUF* src_img,UINT32 cdd_max_distance_mode);
VOID print_cddcnn_results(CDDCNN_MEM* cddcnn_mem, LIMIT_PARAM *limit_para);
UINT32 assign_cddcnn_mem(VENDOR_AIS_FLOW_MEM_PARM* buf, CDDCNN_MEM* cdd_mem, UINT32 model_size, INT32 need_input);
HD_RESULT cddcnn_version_check(VENDOR_AIS_FLOW_MEM_PARM *buf);
VOID cddcnn_get_version(VOID);
HD_RESULT get_cdd_mem(VENDOR_AIS_FLOW_MEM_PARM *buf, VENDOR_AIS_FLOW_MEM_PARM *req_mem, UINT32 req_size, UINT32 align_size);
HD_RESULT get_post_mem(VENDOR_AIS_FLOW_MEM_PARM* buf, CDDCNN_MEM* cdd_mem);





