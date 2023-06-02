
/**
 * @file kdrv_dce_lmt.h
 * @brief parameter limitation of KDRV DCE
 * @author ISP
 * @date in the year 2019
 */

#ifndef _KDRV_DCE_LMT_H_
#define _KDRV_DCE_LMT_H_

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

//=====================================================
// input / output limitation for Dram to Dram mode
#define DCE_SRCBUF_D2D_WMIN         32
#define DCE_SRCBUF_D2D_WMAX         8188
#define DCE_SRCBUF_D2D_WALIGN       4

#define DCE_SRCBUF_D2D_HMIN         32
#define DCE_SRCBUF_D2D_HMAX         8190
#define DCE_SRCBUF_D2D_HALIGN       2

#define DCE_SRCBUF_D2D_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define DCE_SRCBUF_D2D_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


//=====================================================
// input / output limitation for IPP mode
#define DCE_SRCBUF_IPP_WMIN         32
#define DCE_SRCBUF_IPP_WMAX         8188
#define DCE_SRCBUF_IPP_WALIGN       4

#define DCE_SRCBUF_IPP_HMIN         32
#define DCE_SRCBUF_IPP_HMAX         8190
#define DCE_SRCBUF_IPP_HALIGN       2

#define DCE_SRCBUF_IPP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define DCE_SRCBUF_IPP_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE