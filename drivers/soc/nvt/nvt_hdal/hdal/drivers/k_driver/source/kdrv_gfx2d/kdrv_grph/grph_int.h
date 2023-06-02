/*
    Graphic module internal header

    Graphic module internal header

    @file       grph_int.h
    @ingroup    mIDrvIPP_Graph
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _GRPH_INT_H
#define _GRPH_INT_H

#if defined __UITRON || defined __ECOS
#include "DrvCommon.h"
#include "cache.h"
#include "limits.h"
#include "nvtDrvProtected.h"
#include "grph_lmt.h"

#elif defined(__FREERTOS)
#include <stdlib.h>
#include "string.h"
#include "limits.h"
#include "io_address.h"
#include "rcw_macro.h"
#include "kwrap/type.h"
#include "grph_compatible.h"
#include "graphic_dbg.h"

#else
//#include <linux/list.h>
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#include "grph_compatible.h"
#include "graphic_dbg.h"

#endif
#include <kwrap/error_no.h>
#include "grph_ll.h"

//#include "grph_lmt.h"
#include "grph_platform.h"
#include "plat/top.h"

//#define _EMULATION_		(0)

#define GRPH_NT96650A_PATCH     (DISABLE)
#define GRPH_GOP_PATCH          (DISABLE)   // for patch GOP0/GOP1 on 650A
#define GRPH_GOP1_PATCH         (DISABLE)   // for patch GOP1 hang on 650B

#define GRPH_GOP01_OVERFLOW_PATCH   (DISABLE)
#define GRPH_GOP1_HALT_PATCH        (DISABLE)
#define GRPH_GOP01_8N4_PATCH        (DISABLE)
#define GRPH_GOP01_16N8_PATCH       (DISABLE)

#define GRPH_LL_BUF_SIZE            (2048)	// link-list depth 4

#define GRPH_LL_MAX_COUNT           (4)

#define GRPH_PRPTY_MAX_CNT          (8)

//
//  graphic register access definition
//
#define GRPH_REG_ADDR(ofs)       (IOADDR_GRAPHIC_REG_BASE+(ofs))
#define GRPH2_REG_ADDR(ofs)      (IOADDR_GRAPHIC2_REG_BASE+(ofs))
#define GRPH_GETREG(ofs)         INW(GRPH_REG_ADDR(ofs))
#define GRPH_SETREG(ofs, value)  OUTW(GRPH_REG_ADDR(ofs), (value))
#define GRPH2_GETREG(ofs)        INW(GRPH2_REG_ADDR(ofs))
#define GRPH2_SETREG(ofs, value) OUTW(GRPH2_REG_ADDR(ofs), (value))

//
//  graphic internal register definition
//

#define GRPH_CK_MSK                 0x000000FF
#define GRPH_CK_RGB4_MSK            0x0000000F
#define GRPH_CK_RGB5_MSK            0x0000001F
#define GRPH_CK16BIT_MSK            0x0000FFFF
#define GRPH_SHF_MSK                0x0000000F
#define GRPH_TEXT_MSK               0xFFFFFFFF
#define GRPH_SQ1_MSK                0x0F000000
#define GRPH_SQ2_MSK                0x007FFF00
#define GRPH_COLORCONST_MSK         0x0000FF00
#define GRPH_ALPHACONST_MSK         0x00FF0000
#define GRPH_THR_MSK                0x000000FF
#define GRPH_ACC_THR_MSK            0x00000FFF
#define GRPH_CHECKSUM_MSK           0x80000000

#define GRPH_BLD_WGT_MSK2           0xFFFF
#define GRPH_BLEND_WA(x)            ((x) & 0xFF)
#define GRPH_BLEND_WB(x)            (((x) & 0xFF)<<8)
#define GRPH_BLEND_MSK              0x000000FF

#define GRPH_CKEY_FILTER_MSK        0x20000000



//
//  Graphic Engine Internal Status
//
typedef enum {
	GRPH_ENGINE_IDLE,
	GRPH_ENGINE_READY,
	GRPH_ENGINE_RUN,

	ENUM_DUMMY4WORD(GRPH_ENGINE_STATUS)
} GRPH_ENGINE_STATUS;

//
//  Graphic Engine Register Domain
//
typedef enum {
    GRPH_REG_DOMAIN_APB,            ///< register on APB
    GRPH_REG_DOMAIN_LL,             ///< register on LL (Link List)

    ENUM_DUMMY4WORD(GRPH_REG_DOMAIN)
} GRPH_REG_DOMAIN;

/*
    Video cover comparative directive

    @note For GRPH_VDOCOV_DESC.
*/
typedef enum {
	GRPH_VDOCOV_COMPARATIVE_GTLT,   ///< compare with > and <
	GRPH_VDOCOV_COMPARATIVE_GTLE,   ///< compare with > and <=
	GRPH_VDOCOV_COMPARATIVE_GELT,   ///< compare with >= and <
	GRPH_VDOCOV_COMPARATIVE_GELE,   ///< compare with >= and <=

	ENUM_DUMMY4WORD(GRPH_VDOCOV_COMPARATIVE)
} GRPH_VDOCOV_COMPARATIVE;

/*
    Video cover quadrilateral type

    @note For graph_adjustVCOVOrigin()
*/
typedef enum {
    GRPH_QUAD_TYPE_NORMAL,          ///< normal quadrilateral
    GRPH_QUAD_TYPE_INNER,           ///< inner quadrilateral (for draw hollow)

    ENUM_DUMMY4WORD(GRPH_QUAD_TYPE)
} GRPH_QUAD_TYPE;

typedef struct struct_point {
	struct struct_point *p_clockwise;    // next point in clockwise
	struct struct_point *p_counterclock; // next point in counter clockwise
	struct struct_point *p_next;
	struct struct_point *p_prev;
	INT32 x;
	INT32 y;
} STRUCT_POINT;

typedef struct _VCOV_ISR_CONTEXT {
	GRPH_FORMAT fmt;
	GRPH_IMG img_a;
	GRPH_IMG img_c;
	UINT32 x_coord_ofs;
	UINT32 remain_width;
	UINT32 slice_ofs;
	UINT32 mosaic_width;
	UINT32 mosaic_height;
} VCOV_ISR_CONTEXT;

/*
	Request link-list element

*/
typedef struct _GRPH_REQ_LL_NODE {
	KDRV_GRPH_TRIGGER_PARAM	trig_param;
    GRPH_IMG			imges[3];
	GRPH_PROPERTY		properties[GRPH_PRPTY_MAX_CNT];

	GRPH_INOUTOP		inout_ops[6];   // max 3 image, but image may be UV packed
	GRPH_CKEYFILTER		ckeyfilter;

	GRPH_QUAD_DESC		quad_desc;
} GRPH_REQ_LL_NODE;


/*
	Request list element

	Store request from kdrv_grph_trigger().
	Each GRPH_REQ_LIST_NODE describes a link list burst.

*/
typedef struct _GRPH_REQ_LIST_NODE {
    GRPH_REQ_LL_NODE    v_ll_req[GRPH_LL_MAX_COUNT];

	KDRV_CALLBACK_FUNC	callback;

	KDRV_GRPH_EVENT_CB_INFO cb_info;

//	struct list_head	list;
} GRPH_REQ_LIST_NODE;

extern void grph_isr_bottom(GRPH_ID id, UINT32 events);
extern ER graph_trigger(GRPH_ID id);
//extern ER graph_trigger(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param,
//                KDRV_CALLBACK_FUNC *p_cb_func);
//extern ER graph_enqueue(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param,
//                KDRV_CALLBACK_FUNC *p_cb_func);

#endif
