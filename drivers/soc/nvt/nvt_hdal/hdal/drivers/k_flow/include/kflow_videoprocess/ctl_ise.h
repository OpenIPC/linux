/**
    ISE_Ctrl Layer

    @file       ctl_ise.h
    @ingroup    mIISE_Ctrl
    @note

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _CTL_ISE_H_
#define _CTL_ISE_H_

#include "kwrap/error_no.h"
#include "kflow_common/type_vdo.h"

/* ise callback fp type */
typedef INT32 (*CTL_ISE_EVENT_FP)(UINT32 event, void *p_in, void *p_out);

/* ise error type */
#define CTL_ISE_E_OK				(E_OK)
#define CTL_ISE_E_SYS				(E_SYS)
#define CTL_ISE_E_PAR				(E_PAR)
#define CTL_ISE_E_NOMEM				(E_NOMEM)
#define CTL_ISE_E_QOVR				(E_QOVR)
#define CTL_ISE_E_ID				(E_ID)
#define CTL_ISE_E_INDATA			(-101)
#define CTL_ISE_E_FLUSH				(-102)
#define CTL_ISE_E_DROP_INPUT_ONLY	(-103)
#define CTL_ISE_E_KDRV_OPEN			(-200)	///< kdrv open error
#define CTL_ISE_E_KDRV_CLOSE		(-201)	///< kdrv close error
#define CTL_ISE_E_KDRV_SET			(-202)	///< kdrv set error
#define CTL_ISE_E_KDRV_GET			(-203)	///< kdrv get error
#define CTL_ISE_E_KDRV_TRIG			(-204)	///< kdrv trigger error
#define CTL_ISE_E_KDRV_STRP			(-205)	///< kdrv stripe cal error
#define CTL_ISE_E_KDRV_DROP			(-206)	///< kdrv stripe cal error

/* init/query config */
typedef struct {
	UINT32 handle_num;			/* number of handles */
	UINT32 queue_num;			/* number of queue nodes */
} CTL_ISE_CTX_BUF_CFG;

/* ise callback event type */
typedef enum {
	CTL_ISE_CBEVT_IN_BUF = 0,
	CTL_ISE_CBEVT_OUT_BUF,
	CTL_ISE_CBEVT_MAX,
} CTL_ISE_CBEVT_TYPE;

typedef struct {
	CTL_ISE_CBEVT_TYPE cbevt;
	CTL_ISE_EVENT_FP fp;
} CTL_ISE_REG_CB_INFO;

/* buffer io operation for both in/out buf callback */
typedef enum {
	CTL_ISE_BUF_NEW = 0,
	CTL_ISE_BUF_PUSH,
	CTL_ISE_BUF_LOCK,
	CTL_ISE_BUF_UNLOCK,
	CTL_ISE_BUF_IO_MAX,
} CTL_ISE_BUF_IO;


/* snd event information, also for in bufio callback */
typedef struct {
	UINT32	buf_id;
	UINT32	data_addr;
	UINT32	rev;
	INT32   err_msg;
} CTL_ISE_EVT;

/* in crop config */
typedef enum {
	CTL_ISE_IN_CROP_AUTO = 0,	/* reference input header information */
	CTL_ISE_IN_CROP_NONE,		/* no crop, process full image */
	CTL_ISE_IN_CROP_USER,		/* user define crop window */
	CTL_ISE_IN_CROP_MODE_MAX,
} CTL_ISE_IN_CROP_MODE;

typedef struct {
	CTL_ISE_IN_CROP_MODE mode;
	URECT crp_window;			/* only effect when CTL_ISE_IN_CROP_USER */
} CTL_ISE_IN_CROP;

/* out path config */
typedef enum {
	CTL_ISE_OUT_PATH_ID_1 = 0,
	CTL_ISE_OUT_PATH_ID_2,
	CTL_ISE_OUT_PATH_ID_3,
	CTL_ISE_OUT_PATH_ID_4,
	CTL_ISE_OUT_PATH_ID_MAX,
} CTL_ISE_OUT_PATH_ID;

typedef struct {
	CTL_ISE_OUT_PATH_ID pid;
	BOOL enable;
	VDO_PXLFMT fmt;
	USIZE bg_size;		/* background size(buffer size) */
	USIZE scl_size;		/* scale out size, output to dst_pos */
	URECT pre_scl_crop;	/* crop before scale */
	URECT post_scl_crop;/* crop after scale */
	URECT out_window;	/* output window base on background */
	UPOINT dst_pos;		/* destination position base on out window */
	URECT hole_region;	/* hole region */
} CTL_ISE_OUT_PATH;

/* buffer info for out bufio_callback used */
typedef struct {
	CTL_ISE_OUT_PATH_ID pid;
	UINT32 buf_size;
	UINT32 buf_id;
	UINT32 buf_addr;
	VDO_FRAME vdo_frm;
	UINT32 lock_cnt;
	INT32 err_msg;
} CTL_ISE_OUT_BUF_INFO;

/* private buffer query info */
typedef enum {
	CTL_ISE_2DLUT_SZ_ZERO = 0,
	CTL_ISE_2DLUT_SZ_9X9,
	CTL_ISE_2DLUT_SZ_65X65,
	CTL_ISE_2DLUT_SZ_129X129,
	CTL_ISE_2DLUT_SZ_257X257,
} CTL_ISE_2DLUT_SZ_SEL;

typedef struct {
	UINT32 buf_addr;					/* only use in set api */
	UINT32 buf_size;					/* set: check buffer enough; get: return buffer size */
} CTL_ISE_PRIVATE_BUF;

/* ise flush config for CTL_ISE_ITEM_FLUSH
	NULL -> flush all path
	else, flush pid in config
*/
typedef struct {
	CTL_ISE_OUT_PATH_ID pid;
} CTL_ISE_FLUSH_CONFIG;

/**
	output path yuv buffer height alignment
	will use vdoframe.reserve[0], [1]
	VDO_FRAME.reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N'); // height align
	VDO_FRAME.reserved[1] = y_height after align
*/
typedef struct {
	CTL_ISE_OUT_PATH_ID pid;
	UINT32 align;
} CTL_ISE_OUT_PATH_HALIGN;

/* ise get/set item */
typedef enum {
	CTL_ISE_ITEM_REG_CB_IMM = 0,    /* Callback function register.      SET-Only,   data_type: CTL_ISE_REG_CB_INFO          */
	CTL_ISE_ITEM_ALGID_IMM,         /* ise ALG ISP id                   GET/SET,    data_type: UINT32                       */
	CTL_ISE_ITEM_FLUSH,             /* ise flush all buffer(in&out)     SET-Only,   data_type: CTL_ISE_FLUSH_CONFIG or NULL */
	CTL_ISE_ITEM_APPLY,             /* Apply parameters to next trigger SET-Only,   data_type: UINT32                       */
	CTL_ISE_ITEM_IN_CROP,           /* ise input crop config            GET/SET,    data_type: CTL_ISE_IN_CROP              */
	CTL_ISE_ITEM_OUT_PATH,          /* ise output path config           GET/SET,    data_type: CTL_ISE_OUT_PATH             */
	CTL_ISE_ITEM_PUSHEVT_INQ,       /* ise get push event number in que GET-Only,   data_type: UINT32                       */
	CTL_ISE_ITEM_PRIVATE_BUF,       /* ise get/set private buffer       GET-Only,   data_type: CTL_ISE_PRIVATE_BUF          */
	CTL_ISE_ITEM_OUT_PATH_HALIGN,   /* ise output path config           GET/SET,    data_type: CTL_ISE_OUT_PATH_HALIGN      */
	CTL_ISE_ITEM_MAX
} CTL_ISE_ITEM;

/* ise ioctl item */
typedef enum {
	CTL_ISE_IOCTL_SNDEVT = 0,       /* Send event.          data_type: CTL_ISE_EVT      */
	CTL_ISE_IOCTL_MAX
} CTL_ISE_IOCTL;

UINT32 ctl_ise_query(CTL_ISE_CTX_BUF_CFG ctx_buf_cfg);
INT32 ctl_ise_init(CTL_ISE_CTX_BUF_CFG ctx_buf_cfg, UINT32 buf_addr, UINT32 buf_size);
INT32 ctl_ise_uninit(void);
UINT32 ctl_ise_open(CHAR *name);
INT32 ctl_ise_close(UINT32 hdl);
INT32 ctl_ise_set(UINT32 hdl, UINT32 item, void *data);
INT32 ctl_ise_get(UINT32 hdl, UINT32 item, void *data);
INT32 ctl_ise_ioctl(UINT32 hdl, UINT32 item, void *data);

void ctl_ise_dump_all(int (*dump)(const char *fmt, ...));

#endif
