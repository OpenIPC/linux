
#ifndef LIMIT_FDET_LIB_H
#define LIMIT_FDET_LIB_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "hdal.h"
#include "hd_type.h"
#include "hd_gfx.h"


#include "vendor_ai.h"
#include "kflow_ai_net/kflow_ai_net.h"

#define LIMIT_FDET_WIDTH			96
#define LIMIT_FDET_HEIGHT			96
#define LIMIT_FDET_VERSION_A          0
#define LIMIT_FDET_VERSION_B          2



typedef struct _LIMIT_FDET_PARAM {
	FLOAT score_thres;
	FLOAT ratiow;
	FLOAT ratioh;
	UINT32 run_id;
	UINT32 cls_id;
	UINT32 proc_num;
} LIMIT_FDET_PARAM;

typedef struct _LIMIT_FDET_MEM {
	VENDOR_AIS_FLOW_MEM_PARM io_mem;
	VENDOR_AIS_FLOW_MEM_PARM model_mem;
	VENDOR_AIS_FLOW_MEM_PARM input_mem;
	VENDOR_AIS_FLOW_MEM_PARM det_in_mem;
	VENDOR_AIS_FLOW_MEM_PARM det_out_mem;
} LIMIT_FDET_MEM;

typedef struct _LIMIT_FDET_RSLT {
	FLOAT score;
	UINT32 cls_id;	
} LIMIT_FDET_RSLT;

HD_RESULT limit_fdet_process(LIMIT_FDET_PARAM* proposal_params, LIMIT_FDET_MEM* limit_fdet_mem, VENDOR_AI_BUF* src_img);
HD_RESULT get_limit_fdet_mem(VENDOR_AIS_FLOW_MEM_PARM *buf, VENDOR_AIS_FLOW_MEM_PARM *req_mem, UINT32 req_size, UINT32 align_size);
HD_RESULT limit_fdet_version_check(VENDOR_AIS_FLOW_MEM_PARM *buf);


#endif

