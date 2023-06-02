
/**
 * @file kdrv_ipe_lmt.h
 * @brief parameter limitation of KDRV IPE
 * @author ISP
 * @date in the year 2019
 */

#ifndef _KDRV_IPE_LMT_H_
#define _KDRV_IPE_LMT_H_

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

//=====================================================
// input / output limitation for Dram to Dram mode
#define IPE_SRCBUF_D2D_WMIN         32
#define IPE_SRCBUF_D2D_WMAX         33652
#define IPE_SRCBUF_D2D_WALIGN       4

#define IPE_SRCBUF_D2D_HMIN         16
#define IPE_SRCBUF_D2D_HMAX         8190
#define IPE_SRCBUF_D2D_HALIGN       4

#define IPE_SRCBUF_D2D_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IPE_SRCBUF_D2D_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


//=====================================================
// input / output limitation for IPP mode
#define IPE_SRCBUF_IPP_WMIN         32
#define IPE_SRCBUF_IPP_WMAX         33652
#define IPE_SRCBUF_IPP_WALIGN       4

#define IPE_SRCBUF_IPP_HMIN         16
#define IPE_SRCBUF_IPP_HMAX         8190
#define IPE_SRCBUF_IPP_HALIGN       4

#define IPE_SRCBUF_IPP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IPE_SRCBUF_IPP_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE

#endif
