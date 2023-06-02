
/**
 * @file kdrv_ime_lmt.h
 * @brief parameter limitation of KDRV IME
 * @author ISP
 * @date in the year 2019
 */

#ifndef _KDRV_IME_LIMIT_H_
#define _KDRV_IME_LIMIT_H_

#include "comm/drv_lmt.h"
#include "kdrv_type.h"


//=====================================================
// input limitation
#define IME_SRCBUF_WMIN         4
#define IME_SRCBUF_WMAX         65532
#define IME_SRCBUF_WALIGN       4

#define IME_SRCBUF_HMIN         4
#define IME_SRCBUF_HMAX         65532
#define IME_SRCBUF_HALIGN       4

#define IME_SRCBUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_SRCBUF_ADDRALIGN    DRV_LIMIT_ALIGN_WORD

//=====================================================
// lca limitation
#define IME_LCABUF_WMIN         40
#define IME_LCABUF_WMAX         65532
#define IME_LCABUF_WALIGN       4

#define IME_LCABUF_HMIN         30
#define IME_LCABUF_HMAX         65532
#define IME_LCABUF_HALIGN       2

#define IME_LCABUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_LCABUF_ADDRALIGN    DRV_LIMIT_ALIGN_WORD


//=====================================================
// output limitation
#define IME_DSTBUF_WMIN         4
#define IME_DSTBUF_WMAX         65532
#define IME_DSTBUF_WALIGN       2

#define IME_DSTBUF_HMIN         4
#define IME_DSTBUF_HMAX         65532
#define IME_DSTBUF_HALIGN       2

#define IME_DSTBUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_DSTBUF_ADDRALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// scaling limitation
// bilinear
#define IME_SCALE_BILINEAR_UP_WMAX      31.99
#define IME_SCALE_BILINEAR_UP_HMAX      31.99
#define IME_SCALE_BILINEAR_DOWN_WMAX    31.99
#define IME_SCALE_BILINEAR_DOWN_HMAX    31.99

//=====================================================
// bicubic
#define IME_SCALE_BICUBIC_UP_WMAX      31.99
#define IME_SCALE_BICUBIC_UP_HMAX      31.99
#define IME_SCALE_BICUBIC_DOWN_WMAX    31.99
#define IME_SCALE_BICUBIC_DOWN_HMAX    31.99

//=====================================================
// nearest
#define IME_SCALE_NEAREST_UP_WMAX      31.99
#define IME_SCALE_NEAREST_UP_HMAX      31.99
#define IME_SCALE_NEAREST_DOWN_WMAX    31.99
#define IME_SCALE_NEAREST_DOWN_HMAX    31.99

//=====================================================
// integration
#define IME_SCALE_INTG_DOWN_WMAX    15.99
#define IME_SCALE_INTG_DOWN_HMAX    15.99


//=====================================================
// OSD input limitation
#define IME_OSDBUF_WMIN         8
#define IME_OSDBUF_WMAX         8192
#define IME_OSDBUF_WALIGN       2

#define IME_OSDBUF_HMIN         8
#define IME_OSDBUF_HMAX         8192
#define IME_OSDBUF_HALIGN       2

#define IME_OSDBUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_OSDBUF_ADDRALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// Privacy-mask pixelation input limitation
#define IME_PM_PXLBUF_WMIN         2
#define IME_PM_PXLBUF_WMAX         2048
#define IME_PM_PXLBUF_WALIGN       2

#define IME_PM_PXLBUF_HMIN         2
#define IME_PM_PXLBUF_HMAX         2048
#define IME_PM_PXLBUF_HALIGN       2

#define IME_PM_PXLBUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_PM_PXLBUF_ADDRALIGN    DRV_LIMIT_ALIGN_HALFWORD
//=====================================================


#endif

