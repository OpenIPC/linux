/**
	@brief Header file of definition of vendor user-space net flow sample.

	@file vendor_ai.h

	@ingroup vendor_ai

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_NET_H_
#define _VENDOR_AI_NET_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "hd_common.h"
#include "vendor_ai_dla/vendor_ai_dla.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#ifndef VENDOR_AI_VERSION
#define VENDOR_AI_VERSION                           0x020000
#endif
	
#define MAX_PROC_CNT       128


/* ai layer id */
#define VENDOR_AI_NET_PARAM_LAYER(layer_id)         (0x80000000 | ((layer_id) & 0xffff))  ///< layer
#define VENDOR_AI_MAXLAYER                          0xffff ///< max layer id (auto)


/* ai i/o buffer path_id */
#define VENDOR_AI_NET_PARAM_IN(layer_id, in_id)     (0x90000000 | (((in_id)  & 0xfff) << 16) | ((layer_id) & 0xffff)) ///< input buffer
#define VENDOR_AI_NET_PARAM_OUT(layer_id, out_id)   (0xa0000000 | (((out_id) & 0xfff) << 16) | ((layer_id) & 0xffff)) ///< output buffer


/* ai dep buffer path_id */
#define VENDOR_AI_NET_PARAM_DEP(dep_id)             (0xb0000000 | ((dep_id) & 0xffff))  ///< dependent buffer


/* ai buf format */
#define HD_VIDEO_PXLFMT_AI_SINT8                    0xa1080000 ///< signed int 8-bits
#define HD_VIDEO_PXLFMT_AI_UINT8                    0xa2080000 ///< unsigned int 8-bits
#define HD_VIDEO_PXLFMT_AI_SINT16                   0xa1100000 ///< signed int 16-bits
#define HD_VIDEO_PXLFMT_AI_UINT16                   0xa2100000 ///< unsigned int 16-bits
#define HD_VIDEO_PXLFMT_AI_SINT32                   0xa1200000 ///< signed int 32-bits
#define HD_VIDEO_PXLFMT_AI_UINT32                   0xa2200000 ///< unsigned int 32-bits
#define HD_VIDEO_PXLFMT_AI_FLOAT32                  0xa3200000 ///< float 32-bits

#define HD_VIDEO_PXLFMT_CLASS_AI                    0xa
#define HD_VIDEO_PXLFMT_TYPE_MASK                   0x0f000000
#define HD_VIDEO_PXLFMT_BITS_MASK                   0x00ff0000
#define HD_VIDEO_PXLFMT_INT_MASK                    0x0000ff00
#define HD_VIDEO_PXLFMT_FRAC_MASK                   0x000000ff
#define HD_VIDEO_PXLFMT_SIGN(pxlfmt)                ((((pxlfmt) & HD_VIDEO_PXLFMT_TYPE_MASK) >> 24) & 0x01)
#define HD_VIDEO_PXLFMT_BITS(pxlfmt)                (((pxlfmt) & HD_VIDEO_PXLFMT_BITS_MASK) >> 16)
#define HD_VIDEO_PXLFMT_INT(pxlfmt)                 (INT8)(((pxlfmt) & HD_VIDEO_PXLFMT_INT_MASK) >> 8)
#define HD_VIDEO_PXLFMT_FRAC(pxlfmt)                (INT8)((pxlfmt) & HD_VIDEO_PXLFMT_FRAC_MASK)

#define HD_VIDEO_PXLFMT_AI_SFIXED8(frac_bits)       (HD_VIDEO_PXLFMT_AI_SINT8 | (((8-(frac_bits)-1)&0xff) << 8) | ((frac_bits)&0xff))   ///< signed fixpoint 8-bits
#define HD_VIDEO_PXLFMT_AI_UFIXED8(frac_bits)       (HD_VIDEO_PXLFMT_AI_UINT8 | (((8-(frac_bits))&0xff) << 8) | ((frac_bits)&0xff))   ///< unsigned fixpoint 8-bits
#define HD_VIDEO_PXLFMT_AI_SFIXED16(frac_bits)      (HD_VIDEO_PXLFMT_AI_SINT16 | (((16-(frac_bits)-1)&0xff) << 8) | ((frac_bits)&0xff))   ///< signed fixpoint 16-bits
#define HD_VIDEO_PXLFMT_AI_UFIXED16(frac_bits)      (HD_VIDEO_PXLFMT_AI_UINT16 | (((16-(frac_bits))&0xff) << 8) | ((frac_bits)&0xff))   ///< unsigned fixpoint 16-bits
#define HD_VIDEO_PXLFMT_AI_SFIXED32(frac_bits)      (HD_VIDEO_PXLFMT_AI_SINT32 | (((32-(frac_bits)-1)&0xff) << 8) | ((frac_bits)&0xff))   ///< signed fixpoint 32-bits
#define HD_VIDEO_PXLFMT_AI_UFIXED32(frac_bits)      (HD_VIDEO_PXLFMT_AI_UINT32 | (((32-(frac_bits))&0xff) << 8) | ((frac_bits)&0xff))   ///< unsigned fixpoint 32-bits


/* ai buf format (addiitonal) */
#define HD_VIDEO_PXLFMT_BGR888_PLANAR               0x2B180888  ///< 3 plane, pixel=B(w,h), G(w,h), R(w,h)
#define HD_VIDEO_PXLFMT_UV                          0x51100044  ///< 1 plane, pixel=UV(w,h)
#define HD_VIDEO_PXLFMT_R8                          0x21080800  ///< 1 plane, pixel=R(w,h)
#define	HD_VIDEO_PXLFMT_G8                          0x21080080  ///< 1 plane, pixel=G(w,h)
#define	HD_VIDEO_PXLFMT_B8                          0x21080008  ///< 1 plane, pixel=B(w,h) 



/* ai schd parm */
#define VENDOR_AI_FAIR_CORE_ALL                     0x000000ff ///< fair: all core
#define VENDOR_AI_FAIR_CORE(ch)                     ((ch) & 0xff) ///< fair: select core (default 0)
#define VENDOR_AI_CAPA_RATE(r)                      (((r) & 0xff) << 16) ///< capacity: max rate (TODO)


/********************************************************************
	TYPE DEFINITION
********************************************************************/

//											  set/get			  set/get/start
//												^ | 				 ^ |
//												| v 				 | v
//	[UNINIT] -- init   --> [INIT] -- open  --> [OPEN]  -- start --> [START]
//			<-- uninit --		 <-- close --		  <-- stop	--
//
typedef enum _VENDOR_AI_PROC_STATE {
	VENDOR_AI_PROC_STATE_UNINIT = 0,
	VENDOR_AI_PROC_STATE_INIT,
	VENDOR_AI_PROC_STATE_OPEN,
	VENDOR_AI_PROC_STATE_START,
} VENDOR_AI_PROC_STATE;

/**
	@name scheduler for all proc handle
*/

typedef enum {
	VENDOR_AI_PROC_SCHD_FAIR          = 0,  ///< overlapping with fair core (default)
	VENDOR_AI_PROC_SCHD_CAPACITY      = 1,  ///< overlapping with max rate (TODO)
	VENDOR_AI_PROC_SCHD_FIFO          = 2,	///< first in first out
} VENDOR_AI_PROC_SCHD;

/**
    @name proc_id max count
*/
typedef struct _VENDOR_AI_CFG_PROC_COUNT {
	UINT32 max_proc_count;                  ///< supported max count of proc_id
} VENDOR_AI_NET_CFG_PROC_COUNT;

/**
	@name ai implement version
*/
#define VENDOR_AI_IMPL_VERSION_LENGTH	20
typedef struct _VENDOR_AI_NET_CFG_IMPL_VERSION {
	CHAR vendor_ai_impl_version[VENDOR_AI_IMPL_VERSION_LENGTH];
	CHAR kflow_ai_impl_version[VENDOR_AI_IMPL_VERSION_LENGTH];
	CHAR kdrv_ai_impl_version[VENDOR_AI_IMPL_VERSION_LENGTH];
} VENDOR_AI_NET_CFG_IMPL_VERSION;

/**
	@name ai parameter id
*/
typedef enum _VENDOR_AI_CFG_ID {

    //set before init
    VENDOR_AI_CFG_PLUGIN_ENGINE       = 0,	///< set/get, config engine pluign
    VENDOR_AI_CFG_PROC_SCHD           = 1,  ///< set/get, config scheduler for all proc handle
    VENDOR_AI_CFG_PROC_COUNT          = 2,  ///< get    , max proc count , using VENDOR_AI_NET_CFG_PROC_COUNT struct
    VENDOR_AI_CFG_IMPL_VERSION        = 3,  ///< get    , implement version, using VENDOR_AI_NET_CFG_IMPL_VERSION struct
    VENDOR_AI_CFG_PROC_CHK_INTERVAL   = 4,  ///< set/get, config timeout check interval (ms) (default 100, should > 100)
    ENUM_DUMMY4WORD(VENDOR_AI_CFG_ID)
} VENDOR_AI_CFG_ID;

/**
	@name struct of model memory
*/
typedef struct _VENDOR_AI_NET_CFG_MODEL {
	UINT32 pa;                              ///< physical address
	UINT32 va;                              ///< virtual address
	UINT32 size;                            ///< buffer size
} VENDOR_AI_NET_CFG_MODEL;

/**
	@name struct of work memory
*/
typedef struct _VENDOR_AI_NET_CFG_WORKBUF {
	UINT32 pa;                              ///< physical address
	UINT32 va;                              ///< virtual address
	UINT32 size;                            ///< buffer size
} VENDOR_AI_NET_CFG_WORKBUF;


typedef enum {
	VENDOR_AI_NET_JOB_OPT_LINEAR      = 0,  ///< sequential trigger jobs (default)
	VENDOR_AI_NET_JOB_OPT_LINEAR_O1   = 1,  ///< sequential trigger jobs, with subgraph optimize level 1
	VENDOR_AI_NET_JOB_OPT_GRAPH       = 10, ///< parallel trigger jobs
	VENDOR_AI_NET_JOB_OPT_GRAPH_O1    = 11, ///< parallel trigger jobs, with subgraph optimize level 1
	VENDOR_AI_NET_JOB_OPT_GRAPH_O2    = 12, ///< parallel trigger jobs, with subgraph optimize level 2 (TODO)
	VENDOR_AI_NET_JOB_OPT_GRAPH_O3    = 13, ///< parallel trigger jobs, with subgraph optimize level 3 (TODO)
} VENDOR_AI_NET_JOB_OPT;

/**
	@name parameters of job optimize
*/
typedef struct _VENDOR_AI_NET_CFG_JOB_OPT {
	VENDOR_AI_NET_JOB_OPT  method;          ///< job optimize method
	INT32 wait_ms;                          ///< -1: sync mode, 0: async mode, >0: async mode with timeout
	UINT32 schd_parm;                       ///< scheduler additional parameter, if not used, should set to 0
} VENDOR_AI_NET_CFG_JOB_OPT;


typedef enum {
	VENDOR_AI_NET_BUF_OPT_FULL        = 0,  ///< allocate each buffer
	VENDOR_AI_NET_BUF_OPT_SHRINK      = 1,  ///< shrink buffer space (default)
	VENDOR_AI_NET_BUF_OPT_SHRINK_O1   = 2,  ///< shrink buffer space with buffer optimize level 1
	VENDOR_AI_NET_BUF_OPT_SHRINK_O2   = 3,  ///< shrink buffer space with buffer optimize level 2
	VENDOR_AI_NET_BUF_OPT_SHRINK_O3   = 4,  ///< shrink buffer space with buffer optimize level 3 (TODO)
	VENDOR_AI_NET_BUF_OPT_NONE        = -1, ///< do nothing, using buffer space already in model bin
} VENDOR_AI_NET_BUF_OPT;

/**
	@name parameters of buffer optimize
*/
typedef struct _VENDOR_AI_NET_CFG_BUF_OPT {
	VENDOR_AI_NET_BUF_OPT  method;          ///< buffer optimize method
	HD_COMMON_MEM_DDR_ID ddr_id;            ///< ddr id
} VENDOR_AI_NET_CFG_BUF_OPT;

/**
	@name net info
*/
typedef struct _VENDOR_AI_NET_INFO {

	// model job info
	UINT32 model_layer_cnt;                 ///< model layer count
	UINT32 model_bind_cnt;                  ///< model bind count
	// model buf info
	UINT32 model_buf_cnt;                   ///< model total io buffer count
	UINT32 model_dep_cnt;                   ///< model total dep buffer count
	// model buf size
	UINT32 model_buf_size;                  ///< model buffer size
	UINT32 model_part1_size;                ///< -model part1 size (header, job-desc, io-buffer-desc, job-workload)
	UINT32 model_part2_size;                ///< -model part2 size (weight-buffer-desc, weight-buffer)

	// open buf info
	UINT32 total_buf_cnt;                   ///< total buffer count
	UINT32 in_buf_cnt;                      ///< in buffer count
	UINT32 out_buf_cnt;                     ///< out buffer count
	UINT32 in_buf_type;                     ///< in buffer type (signature)
	UINT32 out_buf_type;                    ///< out buffer type (signature)
	// open buf size
	UINT32 work_buf_size;                   ///< work buffer size
	UINT32 in_buf_size;                     ///< -in buffer size
	UINT32 io_buf_size;                     ///< -io buffer size
	UINT32 out_buf_size;                    ///< -out buffer size
	UINT32 misc_buf_size;                   ///< -misc buffer size

	// start job info
	UINT32 total_job_cnt;                   ///< total job count
	UINT32 total_bind_cnt;	                ///< total bind count
	UINT32 in_job_cnt;                      ///< in job count
	UINT32 out_job_cnt;                     ///< out job count

} VENDOR_AI_NET_INFO;

/**
	@name input buffer / output buffer
*/
typedef struct _VENDOR_AI_BUF {
	UINT32 sign;                            ///< signature = MAKEFOURCC('A','B','U','F')
	UINT32 chunk_size;                      ///< sizeof(this sturct) - 8;
	HD_COMMON_MEM_DDR_ID ddr_id;            ///< ddr id
	UINT32 pa;                              ///< physical address
	UINT32 va;                              ///< virtual address
	UINT32 size;                            ///< buffer size
	HD_VIDEO_PXLFMT fmt;                    ///< format
	UINT32 width;                           ///< width
	UINT32 height;                          ///< height
	UINT32 channel;                         ///< channel
	UINT32 batch_num;                       ///< number of batch
	UINT32 time;                            ///< number of time
	union {
		UINT32 reserve;                     ///< reserved
		FLOAT scale_ratio;                  ///< for fixed and float
	};
	UINT32 line_ofs;                        ///< line offset
	UINT32 channel_ofs;                     ///< channel offset
	UINT32 batch_ofs;                       ///< batch offset
	UINT32 time_ofs;                        ///< time offset
	CHAR   layout[8];                       ///< layout order of W,H,C,N
	CHAR  *name;                            ///< buffer name, eg: "mylayer.out0"
	CHAR  *op_name;                         ///< related operation name
} VENDOR_AI_BUF;

/**
	@name struct for finding buffer path
*/
typedef struct _VENDOR_AI_BUF_NAME {
	CHAR   name[100];                       ///< buffer name, eg: "mylayer.out0"
	UINT32 path_id;                         ///< path_id with VENDOR_AI_NET_PARAM_OUT() value
} VENDOR_AI_BUF_NAME;

/**
	@name network parameter id
*/
typedef enum _VENDOR_AI_NET_PARAM_ID {

    VENDOR_AI_NET_PARAM_STATE                   = 0,  ///<     get, proc state, using VENDOR_AI_PROC_STATE type, 1=ready, 2=open, 3=start

    //set before open
    VENDOR_AI_NET_PARAM_CFG_MODEL               = 1,  ///< set/get, config model , using VENDOR_AI_NET_CFG_MODEL struct
    VENDOR_AI_NET_PARAM_CFG_MODEL_RESINFO       = 2,  ///< set	  , set difference resolution model bin, using VENDOR_AI_NET_CFG_MODEL struct
    VENDOR_AI_NET_PARAM_CFG_SHAREMODEL          = 3,  ///< set/get, config share model  , using VENDOR_AI_NET_CFG_MODEL struct
    VENDOR_AI_NET_PARAM_CFG_JOB_OPT             = 4,  ///< set/get, config job optimize , using VENDOR_AI_NET_CFG_JOB_OPT struct
    VENDOR_AI_NET_PARAM_CFG_BUF_OPT             = 5,  ///< set/get, config buf optimize , using VENDOR_AI_NET_CFG_BUF_OPT struct
    VENDOR_AI_NET_PARAM_CFG_USER_POSTPROC       = 6,  ///< set    , config user postproc, using void* (get_user_postproc)(void) to return VENDOR_AI_ENGINE_PLUGIN struct pointer

    //get after open
    VENDOR_AI_NET_PARAM_INFO                    = 20, ///< get	  , get net info, using VENDOR_AI_NET_INFO struct
    VENDOR_AI_NET_PARAM_IN_PATH_LIST            = 21, ///< get    , get path_id of multiple input buffer, using UINT32 array
    VENDOR_AI_NET_PARAM_OUT_PATH_LIST           = 22, ///< get	  , get path_id of multiple output buffer, using UINT32 array
    VENDOR_AI_NET_PARAM_OUT_PATH_BY_NAME        = 24, ///< get	  , get output path by output name, using VENDOR_AI_BUF_NAME struct
    VENDOR_AI_NET_PARAM_LAST_LAYER_LABELNUM     = 25, ///< get	  , get last layer label number

    //set after open ~ before start
    VENDOR_AI_NET_PARAM_CFG_WORKBUF             = 41, ///< get	  , set work memory, using VENDOR_AI_NET_CFG_WORKBUF struct
    VENDOR_AI_NET_PARAM_RES_ID                  = 42, ///< set/get, set resolution by id (0 means default resolution)
    VENDOR_AI_NET_PARAM_RES_DIM                 = 43, ///< set/get, set resolution by dim, using HD_DIM struct
    
    //get/set after start ~ before stop
    //VENDOR_AI_NET_PARAM_IN(layer_id, in_id),        ///< set    , set single input buffer, using VENDOR_AI_BUF struct
                                                      ///< get    , get single input buffer, using VENDOR_AI_BUF struct
    //VENDOR_AI_NET_PARAM_OUT(layer_id, out_id),      ///< get    , get single output buffer, using VENDOR_AI_BUF struct
    //VENDOR_AI_NET_PARAM_DEP(dep_id),                ///< get    , get dependent buffer size, using UINT32
                                                      ///< push_in, push_in dependent buffer for signal "ready to use"
                                                      ///< push_out, push_out dependent buffer for waiting "use finished"
    VENDOR_AI_NET_PARAM_IN_BUF_LIST             = 51, ///< set    , set multiple input buffer, using related output struct array (TODO)
                                                      ///< get    , get multiple input buffer, using related output struct array (TODO)
    VENDOR_AI_NET_PARAM_OUT_BUF_LIST            = 52, ///< get	  , get multiple output buffer, using related output struct array (TODO)
	
    ENUM_DUMMY4WORD(VENDOR_AI_NET_PARAM_ID)
} VENDOR_AI_NET_PARAM_ID;


/**
	@name operator id
*/
typedef enum _VENDOR_AI_OP {
    VENDOR_AI_OP_FC                             = 1,  ///< fully connect (vector dot vector)
	VENDOR_AI_OP_PREPROC							= 2,  ///< image transform (color convert, crop, scale, sub, crop, pad and rotate)
    ENUM_DUMMY4WORD(VENDOR_AI_OP)
} VENDOR_AI_OP;

typedef struct _VENDOR_AI_OP_PREPROC_PARAM {
	HD_DIM scale_dim;                  ///< scale dim, set (0,0) to OFF
    VENDOR_AI_BUF p_out_sub;           ///< out subtract plane, packed format, set NULL to OFF
	UINT32 out_sub_color[3];           ///< out subtract constant value, set [0,0,0] to OFF
	HD_IRECT in_crop_win;              ///< in crop window, coord is based on in_buf.dim, set (0,0,0,0) to OFF (TODO)
	HD_IRECT out_crop_win;             ///< out crop window, coord is based on scale dim, set (0,0,0,0) to OFF (TODO)
	HD_IPOINT out_crop_pt;             ///< out crop window, coord is based on out_buf.dim, set (0,0) to OFF (TODO)
	HD_DIM pad_dim;                    ///< out padding window, coord is based on out_buf.dim, set (0,0) to OFF (TODO)
	UINT32 pad_color[3];               ///< out padding constant value, set [???] to OFF (TODO)
	UINT32 dir;                        ///< out direct: mirror-x, mirror-y, rotate, set 0 to OFF, use HD_VIDEO_DIR_XXXX flags (TODO)
} VENDOR_AI_OP_PREPROC_PARAM;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

extern HD_RESULT vendor_ai_init (VOID);
extern HD_RESULT vendor_ai_uninit (VOID);

extern HD_RESULT vendor_ai_get_id (UINT32* proc_id);
extern HD_RESULT vendor_ai_release_id (UINT32 proc_id);

// general config
extern HD_RESULT vendor_ai_cfg_set (VENDOR_AI_CFG_ID cfg_id, void* p_param);
extern HD_RESULT vendor_ai_cfg_get (VENDOR_AI_CFG_ID cfg_id, void* p_param);

// static network
extern HD_RESULT vendor_ai_net_set (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID param_id, void* p_param);
extern HD_RESULT vendor_ai_net_get (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID param_id, void* p_param);
extern HD_RESULT vendor_ai_net_open (UINT32 proc_id);
extern HD_RESULT vendor_ai_net_close (UINT32 proc_id);
extern HD_RESULT vendor_ai_net_start (UINT32 proc_id);
extern HD_RESULT vendor_ai_net_stop (UINT32 proc_id);
extern HD_RESULT vendor_ai_net_proc (UINT32 proc_id);
extern HD_RESULT vendor_ai_net_proc_buf (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID in_param_id, void* p_in_buf, VENDOR_AI_NET_PARAM_ID out_param_id, void* p_out_buf);
extern HD_RESULT vendor_ai_net_push_in_buf (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID in_param_id, void* p_in_buf, INT32 wait_ms);
extern HD_RESULT vendor_ai_net_pull_out_buf (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID out_param_id, void* p_out_buf, INT32 wait_ms);

// dynamic operator
extern HD_RESULT vendor_ai_op_open (UINT32 proc_id);
extern HD_RESULT vendor_ai_op_close (UINT32 proc_id);
extern HD_RESULT vendor_ai_op_proc (UINT32 proc_id, VENDOR_AI_OP op_id, void* p_op_param, UINT32 in_cnt, VENDOR_AI_BUF* p_in_buf, UINT32 out_cnt, VENDOR_AI_BUF* p_out_buf);


#endif  /* _VENDOR_AI_NET_H_ */
