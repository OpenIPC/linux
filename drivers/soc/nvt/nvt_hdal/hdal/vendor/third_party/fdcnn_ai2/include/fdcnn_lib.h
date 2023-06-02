
#ifndef FDCNN_LIB_H
#define FDCNN_LIB_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "hdal.h"
#include "hd_type.h"
#include "hd_gfx.h"

#define FDCNN_AI2_LIB       (1)

#if FDCNN_AI2_LIB
#include "vendor_ai.h"
#include "kflow_ai_net/kflow_ai_net.h"
#else
#include "vendor_ai/vendor_ai.h"
#include "net_gen_sample/net_gen_sample.h"
#include "net_flow_sample/net_flow_sample.h"
#include "net_pre_sample/net_pre_sample.h"
#include "net_post_sample/net_post_sample.h"
#include "net_flow_user_sample/net_flow_user_sample.h"
#endif
//#include "net_util_sample.h"


/* -----------------------------------------------------------------------------------------*/

#define FDCNN_STA_OK			(0)
#define FDCNN_STA_ERR			(-1)

#define FDCNN_MAX_SCORE         (16383) // 2^14 - 1

typedef struct _FDCNN_RESULT
{
	INT32   category;
	INT32 	score;
	INT32 	x;
	INT32 	y;
	INT32 	w;
	INT32 	h;
	HD_IPOINT 	landmark_points[5]; //left_eye, right_eye, nose, left_mouth, right_mouth
}FDCNN_RESULT;

/**
     network type.

*/
typedef enum //FDCNN_NETWORK_TYPE
{
    FDCNN_NETWORK_V10 = 0, // FDCNN_NETWORK_V10 need 4 file
    PDCNN_NETWORK_V10 = 1, // PDCNN_NETWORK_V10 need 4 file
    FDCNN_NETWORK_V20 = 2, // FDCNN_NETWORK_V20 need 1 file
    FDCNN_NETWORK_V21 = 3, // FDCNN LIGHT
    ENUM_DUMMY4WORD(FDCNN_NETWORK_TYPE)
} FDCNN_NETWORK_TYPE;

/**
     model index.

*/
typedef enum
{
    FDCNN_FILE_1 = 1,
    FDCNN_FILE_2 = 2,
    FDCNN_FILE_3 = 3,
    FDCNN_FILE_4 = 4,
    ENUM_DUMMY4WORD(FDCNN_MODEL_INDEX)
} FDCNN_MODEL_INDEX;

/**
     Config ID.

*/
typedef enum
{
    FDCNN_CFG_SENSITIVITY   = 0,          ///< config FDCNN sensitivity, from 0 to 16384.
	ENUM_DUMMY4WORD(FDCNN_CONFIG)
} FDCNN_CONFIG;

/**
     fdcnn init parameter.

*/
typedef struct _FDCNN_INIT_PARM
{
    FDCNN_NETWORK_TYPE net_type;
	UINT32 net_id;                   ///< only using for FDCNN_NETWORK_V20, from 0 to 16.
} FDCNN_INIT_PARM;


HD_RESULT   fdcnn_init(VENDOR_AIS_FLOW_MEM_PARM buf, FDCNN_INIT_PARM net_parm);
HD_RESULT   fdcnn_uninit(VENDOR_AIS_FLOW_MEM_PARM buf);

INT32       fdcnn_calcbuffsize(FDCNN_NETWORK_TYPE net_type);

HD_RESULT   fdcnn_lock(VENDOR_AIS_FLOW_MEM_PARM buf, BOOL block);
UINT32      fdcnn_get_model_addr(VENDOR_AIS_FLOW_MEM_PARM buf, FDCNN_MODEL_INDEX index, FDCNN_NETWORK_TYPE net_type);
HD_RESULT   fdcnn_config(VENDOR_AIS_FLOW_MEM_PARM buf, FDCNN_CONFIG config_id, INT32 value);
HD_RESULT   fdcnn_getconfig(VENDOR_AIS_FLOW_MEM_PARM buf, FDCNN_CONFIG config_id, INT32 *p_value);

HD_RESULT   fdcnn_set_image(VENDOR_AIS_FLOW_MEM_PARM buf, HD_GFX_IMG_BUF *p_inputimg);
HD_RESULT   fdcnn_process(VENDOR_AIS_FLOW_MEM_PARM buf);
UINT32      fdcnn_getresults(VENDOR_AIS_FLOW_MEM_PARM buf, FDCNN_RESULT *p_face, HD_URECT* p_coord, UINT32 max_outnum);
HD_RESULT   fdcnn_get_image(VENDOR_AIS_FLOW_MEM_PARM buf, HD_GFX_IMG_BUF *p_img, UINT32 is_va); //if is_va, return va in p_phy_addr[]
VOID        fdcnn_dump_version(VOID); // fdcnn big version, fdcnn small version, model version, doc version

// only for test
BOOL fdcnn_readbmpheader(UINT32 *height, UINT32 *width, UINT32 *bitcount, CHAR *file);
BOOL fdcnn_readbmpbody(UINT8 *pdata, UINT32 height, UINT32 width, UINT32 bitcount, UINT8 *colortable, UINT8 flipupdown, CHAR *file);
BOOL fdcnn_writebmpfile(UINT8 *pdata, UINT32 lineoffset, UINT32 height, UINT32 width, UINT32 bitcount, \
                        UINT8 *colortable, UINT8 flipupdown, CHAR *file);

#endif

