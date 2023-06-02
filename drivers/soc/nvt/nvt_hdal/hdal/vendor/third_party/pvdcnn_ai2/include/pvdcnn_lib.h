#ifndef PVDCNN_LIB_H
#define PVDCNN_LIB_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>
#include <arm_neon.h>

#include "hdal.h"
#include "hd_type.h"
#include "hd_gfx.h"

#define PVDCNN_AI2_LIB    1

#if PVDCNN_AI2_LIB
#include "vendor_ai.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#else
#include "vendor_ai/vendor_ai.h"
//#include "net_util_sample.h"
#include "net_gen_sample/net_gen_sample.h"
#include "net_flow_sample/net_common.h"
#include "net_flow_sample/net_flow_sample.h"
#include "net_pre_sample/net_pre_sample.h"
#include "net_post_sample/net_post_sample.h"
#include "net_flow_user_sample/net_flow_user_sample.h"
#endif
/* -----------------------------------------------------------------------------------------*/

typedef struct
{
	UINT32 net_id;
	FLOAT conf_thr;
	FLOAT conf_thr2;
	FLOAT nms_thr;
	UINT32 reserved;
} PVDCNN_INIT_PRMS;


typedef struct
{
	HD_URECT box;
	FLOAT confidence;
	UINT32 class_type;
} PVDCNN_RSLT;

UINT32 pvdcnn_calcbuffsize(VOID);
HD_RESULT pvdcnn_init(VENDOR_AIS_FLOW_MEM_PARM buf, PVDCNN_INIT_PRMS *p_init_prms);
HD_RESULT pvdcnn_uninit(VENDOR_AIS_FLOW_MEM_PARM buf);

HD_RESULT pvdcnn_set_img(VENDOR_AIS_FLOW_MEM_PARM buf, HD_GFX_IMG_BUF *p_img);
HD_RESULT pvdcnn_process(VENDOR_AIS_FLOW_MEM_PARM buf);
UINT32 pvdcnn_get_result(VENDOR_AIS_FLOW_MEM_PARM buf, PVDCNN_RSLT *p_obj_out, HD_URECT *p_coord, UINT32 max_out_num);

VOID pvdcnn_set_conf_thresh(VENDOR_AIS_FLOW_MEM_PARM buf, FLOAT thresh);
FLOAT pvdcnn_get_conf_thresh(VENDOR_AIS_FLOW_MEM_PARM buf);
VOID pvdcnn_set_conf_thresh2(VENDOR_AIS_FLOW_MEM_PARM buf, FLOAT thresh);
FLOAT pvdcnn_get_conf_thresh2(VENDOR_AIS_FLOW_MEM_PARM buf);
HD_RESULT pvdcnn_get_img(VENDOR_AIS_FLOW_MEM_PARM buf, HD_GFX_IMG_BUF *p_img, UINT32 is_va);



//just for test
VOID pvdcnn_readbmpheader(UINT32 *p_height, UINT32 *p_width, UINT32 *p_bitcount, CHAR *p_file);
VOID pvdcnn_readbmpbody(UINT8 *p_data, UINT32 height, UINT32 width, UINT32 bitcount, UINT8 *p_colortable, UINT8 flipupdown, CHAR *p_file);
VOID pvdcnn_writebmpfile(UINT8 *p_data, UINT32 lineoffset, UINT32 height, UINT32 width, UINT32 bitcount, UINT8 *p_colortable, UINT8 flipupdown, CHAR *p_file);
UINT32 pvdcnn_load_bin(UINT32 addr, const CHAR *p_filename);
VOID pvdcnn_draw_rslt(PVDCNN_RSLT *p_rslt, UINT32 rslt_num, UINT8 *p_gray, UINT32 lineoffset, UINT32 height, UINT32 width);
VOID pvdcnn_print_rslt(PVDCNN_RSLT *p_rslt, UINT32 rslt_num, const CHAR *p_str);
VOID pvdcnn_write_rslt(PVDCNN_RSLT *p_rslt, UINT32 rslt_num, CHAR *p_filename);
VOID pvdcnn_get_version(VOID);


#endif

