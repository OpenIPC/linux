/**
    IPL_Ctrl Layer

    @file       IPL_Ctrl.h
    @ingroup    mILibIPH
    @note

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _CTL_IPP_H_
#define _CTL_IPP_H_

#include "kflow_common/type_vdo.h"
#include "ctl_ipp_util.h"
#include "ipp_event.h"

/**********************************/
/*  ipp_ctl_ipl task ctrl         */
/**********************************/
ER ctl_ipp_open_tsk(void);
ER ctl_ipp_close_tsk(void);
ER ctl_ipp_set_resume(BOOL b_flush_evt);
ER ctl_ipp_set_pause(BOOL b_wait_end, BOOL b_flush_evt);
ER ctl_ipp_wait_pause_end(void);


/**********************************/
/*  ipp_ctl_ipl handle ctrl       */
/**********************************/
#define CTL_IPP_HDR_MAX_FRAME_NUM (2)

typedef void (*CTL_IPP_DUMP_FP)(char *fmt, ...);

typedef enum {
	CTL_IPP_FLOW_UNKNOWN = 0,
	CTL_IPP_FLOW_RAW,
	CTL_IPP_FLOW_DIRECT_RAW,
	CTL_IPP_FLOW_CCIR,
	CTL_IPP_FLOW_DIRECT_CCIR,
	CTL_IPP_FLOW_IME_D2D,
	CTL_IPP_FLOW_IPE_D2D,
	CTL_IPP_FLOW_VR360,
	CTL_IPP_FLOW_DCE_D2D,
	CTL_IPP_FLOW_CAPTURE_RAW,
	CTL_IPP_FLOW_CAPTURE_CCIR,
	CTL_IPP_FLOW_MAX
} CTL_IPP_FLOW_TYPE;

#define CTL_IPP_IS_CAPTURE_FLOW(flow) ((flow == CTL_IPP_FLOW_CAPTURE_RAW) || (flow == CTL_IPP_FLOW_CAPTURE_CCIR))
#define CTL_IPP_IS_DIRECT_FLOW(flow) ((flow == CTL_IPP_FLOW_DIRECT_RAW) || (flow == CTL_IPP_FLOW_DIRECT_CCIR))

typedef enum {
	CTL_IPP_CBEVT_IN_BUF_PROCEND = 0,
	CTL_IPP_CBEVT_IN_BUF_DROP,
	CTL_IPP_CBEVT_IN_BUF_PROCSTART,
	CTL_IPP_CBEVT_IN_BUF_DIR_PROCEND,
	CTL_IPP_CBEVT_IN_BUF_DIR_DROP,
	CTL_IPP_CBEVT_IN_BUF_MAX
} CTL_IPP_CBEVT_IN_BUF_MSG;

typedef enum {
	CTL_IPP_CBEVT_IN_BUF        = 0 | IPP_EVENT_ISR_TAG,
	CTL_IPP_CBEVT_OUT_BUF       = 1 | IPP_EVENT_ISR_TAG,
	CTL_IPP_CBEVT_ENG_RHE_ISR   = 2 | IPP_EVENT_ISR_TAG,
	CTL_IPP_CBEVT_ENG_IFE_ISR   = 3 | IPP_EVENT_ISR_TAG,
	CTL_IPP_CBEVT_ENG_DCE_ISR   = 4 | IPP_EVENT_ISR_TAG,
	CTL_IPP_CBEVT_ENG_IPE_ISR   = 5 | IPP_EVENT_ISR_TAG,
	CTL_IPP_CBEVT_ENG_IME_ISR   = 6 | IPP_EVENT_ISR_TAG,
	CTL_IPP_CBEVT_ENG_IFE2_ISR  = 7 | IPP_EVENT_ISR_TAG,
	CTL_IPP_CBEVT_DATASTAMP     = 8 | IPP_EVENT_ISR_TAG,	/* p_in: CTL_IPP_DS_CB_INPUT_INFO; p_out: CTL_IPP_DS_CB_OUTPUT_INFO */
	CTL_IPP_CBEVT_PRIMASK       = 9 | IPP_EVENT_ISR_TAG,	/* p_in: CTL_IPP_PM_CB_INPUT_INFO; p_out: CTL_IPP_PM_CB_OUTPUT_INFO */
	CTL_IPP_CBEVT_MAX = 10,
} CTL_IPP_CBEVT_TYPE;

typedef enum {
	CTL_IPP_BUF_UNKNOWN = 0,
	CTL_IPP_BUF_PRIVATE,
	CTL_IPP_BUF_OUTPUT,
} CTL_IPP_BUF_TYPE;

typedef enum {
	CTL_IPP_BUF_PRI_LCA = 0,
	CTL_IPP_BUF_PRI_MO,
	CTL_IPP_BUF_PRI_MV,
	//CTL_IPP_BUF_PRI_DEFOG,
	//CTL_IPP_BUF_PRI_ETH,
	CTL_IPP_BUF_PRI_MAX,
} CTL_IPP_BUF_PRI_ITEM;

typedef enum {
	CTL_IPP_ALGID_IQ = 0,
	CTL_IPP_ALGID_MAX,
} CTL_IPP_ALGID_TYPE;

/*
	stripe rule select
	for stripe auto mode usage -> kdrv_dce auto calculate stripe(select in iq config)
	rule definition is define in kdrv_dce
	rule1: for gdc + low latency
	rule2: low latency best
	rule3: 2dlut best
	rule4: gdc best
*/
typedef enum {
	CTL_IPP_STRP_RULE_1 = 0,
	CTL_IPP_STRP_RULE_2,
	CTL_IPP_STRP_RULE_3,
	CTL_IPP_STRP_RULE_4,
	CTL_IPP_STRP_RULE_MAX,
} CTL_IPP_STRP_RULE_SELECT;

typedef struct {
	UINT32	buf_id;
	UINT32	data_addr;
	UINT32	rev;
	INT32   err_msg;
} CTL_IPP_EVT;

typedef struct {
	UINT32	buf_id[CTL_IPP_HDR_MAX_FRAME_NUM];
	UINT32	data_addr[CTL_IPP_HDR_MAX_FRAME_NUM];
	UINT32	rev;
	INT32   err_msg;
} CTL_IPP_EVT_HDR;

typedef struct {
	UINT32	buf_id;
	UINT32	data_addr;
	UINT32	rev;
	INT32   err_msg;
} CTL_IPP_EVT_PATTERN_PASTE;

typedef struct {
	UINT32	buf_id;
	UINT32	buf_addr;
	UINT32  reserved; /* for stack usage, this used to be VDO_FRAME vdo_frm; but not used */
} CTL_IPP_DIR_EVT;

typedef union {
	CTL_IPP_EVT dft_evt;		/* default event type */
	CTL_IPP_DIR_EVT dir_evt;	/* direct mode event type */
} CTL_IPP_EVT_UNION;

typedef struct {
	CTL_IPP_CBEVT_TYPE cbevt;
	IPP_EVENT_FP fp;
} CTL_IPP_REG_CB_INFO;

typedef struct {
	CTL_IPP_ALGID_TYPE type;
	UINT32 id;
} CTL_IPP_ALGID;

typedef struct {
	CTL_IPP_IN_CROP_MODE mode;
	URECT crp_window;	/* only effect when CTL_IPP_IN_CROP_USER */
} CTL_IPP_IN_CROP;

typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	BOOL enable;
	VDO_PXLFMT fmt;
	UINT32 lofs;
	USIZE size;
	URECT crp_window;
} CTL_IPP_OUT_PATH;

typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	BOOL enable;
} CTL_IPP_OUT_PATH_FLIP;

typedef struct {
	UINT32 start_addr;
	UINT32 size;
} CTL_IPP_BUFCFG;

typedef struct {
	USIZE max_size;
	VDO_PXLFMT pxlfmt;
	CTL_IPP_FUNC func_en;
	UINT32 buf_size;
	UINT32 max_strp_num;
} CTL_IPP_PRIVATE_BUF;

/**
	Buffer info for bufio_callback used
*/
typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	UINT32 buf_size;
	UINT32 buf_id;
	UINT32 buf_addr;
	VDO_FRAME vdo_frm;
	/* reserved usage of vdo_frm
		reserve[0:3] low latency(VDO_LOW_DLY_INFO)
		reserve[0:1] height aling {MAKEFOURCC('H', 'A', 'L', 'N'), height}
		reserve[2:3] slice index {MAKEFOURCC('S', 'L', 'I', 'C'), (slice_cnt << 16 | slice_idx)}
	*/
	UINT32 lock_cnt;
	INT32 err_msg;
} CTL_IPP_OUT_BUF_INFO;

/**
	flush config
	after flush a path, all buffer io callback of the path is finished
*/
typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
} CTL_IPP_FLUSH_CONFIG;

/**
	output buffer mode
	one buffer mode will skip buf_tag check in direct_raw flow
	default is disable
*/
typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	UINT32 one_buf_mode_en;
} CTL_IPP_OUT_PATH_BUFMODE;

/**
	output path md enable
	enable will attach md info as metadata to path output
	must enable 3dnr first to activate this function
	default is disable
*/
typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	UINT32 enable;
} CTL_IPP_OUT_PATH_MD;

/**
	output path yuv buffer height alignment
	will use vdoframe.reserve[0], [1]
	VDO_FRAME.reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N'); // height align
	VDO_FRAME.reserved[1] = y_height after align
*/
typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	UINT32 align;
} CTL_IPP_OUT_PATH_HALIGN;

/**
	output path push order
	push from small to big
*/
typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	UINT8 order;
} CTL_IPP_OUT_PATH_ORDER;

/**
	output path region
	enable will output image data to region of background
*/
typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	UINT8 enable;
	UINT32 bgn_lofs;
	USIZE bgn_size;
	UPOINT region_ofs;
} CTL_IPP_OUT_PATH_REGION;

/**
	pattern paste config
	use in ctl_ipp_ioctl_sndevt_pattern_paste flow
*/
typedef struct {
	UINT32 bgn_color[3];

	URECT pat_win;
	CTL_IPP_DS_CST pat_cst_info;
	CTL_IPP_DS_PLT pat_plt_info;
	CTL_IPP_DS_INFO pat_info;
} CTL_IPP_PATTERN_PASTE_INFO;

/**
	slice push config
	enable seperate for each path
	cnt is the total slice number
	ex: cnt = 8, push 8 times per frame
*/
typedef struct {
	UINT8 enable[CTL_IPP_OUT_PATH_ID_MAX];
	UINT8 cnt;
} CTL_IPP_SLICE_PUSH;

/* *
	CTL_IPP_GET/SET ITEM
*/
typedef enum {
	CTL_IPP_ITEM_FLOWTYPE = 0,      /* flow type.                       GET-Only,   data_type: CTL_IPP_FLOW_TYPE        */
	CTL_IPP_ITEM_REG_CB_IMM,        /* Callback function register.      SET-Only,   data_type: CTL_IPP_REG_CB_INFO      */
	CTL_IPP_ITEM_UNREG_CB_IMM,      /* Callback function unregister.    SET-Only,   data_type: CTL_IPP_REG_CB_INFO      */
	CTL_IPP_ITEM_ALGID_IMM,         /* IPP ALG ISP id                   GET/SET,    data_type: CTL_IPP_ALGID            */
	CTL_IPP_ITEM_BUFCFG,            /* ipp inner buffer configuration.  GET-Only,   data_type: CTL_IPP_BUFCFG           */
	CTL_IPP_ITEM_IN_CROP,           /* Input Crop window.               GET/SET,    data_type: CTL_IPP_IN_CROP          */
	CTL_IPP_ITEM_OUT_PATH,          /* Output path Info.                GET/SET,    data_type: CTL_IPP_OUT_PATH         */
	CTL_IPP_ITEM_FUNCEN,            /* ipp function enable.             GET/SET,    data_type: CTL_IPP_FUNC             */
	CTL_IPP_ITEM_SCL_METHOD,        /* ipp output scale method.         GET/SET,    data_type: CTL_IPP_SCL_METHOD_SEL   */
	CTL_IPP_ITEM_FLIP,              /* ipp flip/mirror.                 GET/SET,    data_type: CTL_IPP_FLIP_TYPE        */
	CTL_IPP_ITEM_3DNR_REFPATH_SEL,  /* ipp 3dnr reference path select.  GET/SET,    data_type: CTL_IPP_OUT_PATH_ID      */
	CTL_IPP_ITEM_OUT_COLORSPACE,	/* ipp output color space			GET/SET,	data_type: CTL_IPP_OUT_COLOR_SPACE	*/
	CTL_IPP_ITEM_APPLY,             /* Apply parameters to next trigger SET-Only,   data_type: UINT32                   */
	CTL_IPP_ITEM_BUFQUY,            /* ipp get private buf              GET-Only,   data_type: CTL_IPP_PRIVATE_BUF      */
	CTL_IPP_ITEM_FLUSH,             /* ipp flush all buffer(in&out)     SET-Only,   data_type: CTL_IPP_FLUSH_CONFIG or NULL */
	CTL_IPP_ITEM_PUSHEVT_INQ,       /* ipp get push event number in que GET-Only,   data_type: UINT32                   */
	CTL_IPP_ITEM_OUT_PATH_FLIP,     /* Output path flip(ime only)       GET/SET,    data_type: CTL_IPP_OUT_PATH_FLIP    */
	CTL_IPP_ITEM_LOW_DELAY_PATH_SEL,/* low delay mode path_sel          GET/SET,    data_type: CTL_IPP_OUT_PATH_ID      */
	CTL_IPP_ITEM_LOW_DELAY_BP,		/* low delay mode bp timing(lines)  GET/SET,    data_type: UINT32                   */
	CTL_IPP_ITEM_OUT_PATH_BUFMODE,	/* ipp output path buffer mode      GET/SET,    data_type: CTL_IPP_OUT_PATH_BUFMODE */
	CTL_IPP_ITEM_OUT_PATH_MD,		/* ipp output path with md enable   GET/SET,    data_type: CTL_IPP_OUT_PATH_MD      */
	CTL_IPP_ITEM_STRP_RULE,			/* ipp stripe rule select(dram mode)GET/SET,    data_type: CTL_IPP_STRP_RULE_SELECT */
	CTL_IPP_ITEM_OUT_PATH_HALIGN,	/* ipp output path height align     GET/SET,    data_type: CTL_IPP_OUT_PATH_HALIGN  */
	CTL_IPP_ITEM_OUT_PATH_ORDER_IMM,/* ipp output path push order       GET/SET,    data_type: CTL_IPP_OUT_PATH_ORDER   */
	CTL_IPP_ITEM_OUT_PATH_REGION,	/* ipp output path region           GET/SET,    data_type: CTL_IPP_OUT_PATH_REGION  */
	CTL_IPP_ITEM_PATTERN_PASTE,		/* use pattern image as output      GET/SET,    data_type: CTL_IPP_PATTERN_PASTE_INFO */
	CTL_IPP_ITEM_SLICE_PUSH_IMM,    /* slice push configuration         GET/SET,    data_type: CTL_IPP_SLICE_PUSH       */
	CTL_IPP_ITEM_MAX
} CTL_IPP_ITEM;

typedef enum {
	CTL_IPP_IOCTL_SNDEVT = 0,       	/* Send event.          data_type: CTL_IPP_EVT      */
	CTL_IPP_IOCTL_SNDSTART,         	/* Send event.          data_type: void		        */
	CTL_IPP_IOCTL_SNDSTOP,          	/* Send event.          data_type: void      	 	*/
	CTL_IPP_IOCTL_SNDEVT_HDR,       	/* Send event.          data_type: CTL_IPP_EVT_HDR  */
	CTL_IPP_IOCTL_SNDWAKEUP,        	/* Send event.          data_type: void		        */
	CTL_IPP_IOCTL_SNDSLEEP,         	/* Send event.          data_type: void      	 	*/
	CTL_IPP_IOCTL_DMA_ABORT,			/* Set dma abort.       data_type: void      	 	*/
	CTL_IPP_IOCTL_SNDEVT_PATTERN_PASTE,	/* Send event.          data_type: CTL_IPP_EVT_PATTERN_PASTE */
	CTL_IPP_IOCTL_MAX
} CTL_IPP_IOCTL;

typedef enum {
	CTL_IPP_VR360_TOP = 0,         /* VR360 process top position */
	CTL_IPP_VR360_BOTTOM,          /* VR360 process bottom position */
	CTL_IPP_VR360_MAX
} CTL_IPP_VR360_POSITION;

typedef enum {
	CTL_IPP_DIRECT_START 	= 0x00000001,      /* Direct mode start */
	CTL_IPP_DIRECT_STOP  	= 0x00000002,      /* Direct mode stop */
	CTL_IPP_DIRECT_PROCESS  = 0x00000004,	   /* Direct mode process */
	CTL_IPP_DIRECT_IN_RE 	= 0x00000008,	   /* Direct mode release input buffer*/
	CTL_IPP_DIRECT_IN_DROP 	= 0x00000010,	   /* Direct mode drop input buffer*/
	CTL_IPP_DIRECT_SKIP  	= 0x00000020,	   /* Direct mode skip cfg*/
	CTL_IPP_DIRECT_MAX
} CTL_IPP_DIRECT_EVENT;

typedef struct {
	UINT32 n;					/* kflow handle number */
	UINT32 kdrv_que_depth;		/* kdrv queue number. suggest 3 for best perfomance & memory usage */
	UINT32 kdrv_ll_en;			/* if enable, allocate linklist buffer for kdrv */
} CTL_IPP_CTX_BUF_CFG;

UINT32 ctl_ipp_query(CTL_IPP_CTX_BUF_CFG ctx_buf_cfg);
INT32 ctl_ipp_init(CTL_IPP_CTX_BUF_CFG ctx_buf_cfg, UINT32 buf_addr, UINT32 buf_size);
INT32 ctl_ipp_uninit(void);
UINT32 ctl_ipp_open(CHAR *name, CTL_IPP_FLOW_TYPE flow);
INT32 ctl_ipp_close(UINT32 hdl);
INT32 ctl_ipp_set(UINT32 hdl, UINT32 item, void *data);
INT32 ctl_ipp_get(UINT32 hdl, UINT32 item, void *data);
INT32 ctl_ipp_ioctl(UINT32 hdl, UINT32 item, void *data);

/* wait jarkko prototype */
INT32 ctl_ipp_direct_flow_cb(UINT32 event, void *p_in, void *p_out);
UINT32 ctl_ipp_get_dir_fp(void *data);

#endif // _CTL_IPP_H_
