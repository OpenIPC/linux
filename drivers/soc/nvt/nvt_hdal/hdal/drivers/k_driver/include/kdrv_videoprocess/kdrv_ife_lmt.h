
/**
 * @file kdrv_ife_lmt.h
 * @brief parameter limitation of KDRV ifE
 * @author ISP
 * @date in the year 2019
 */

#ifndef _KDRV_IFE_LMT_H_
#define _KDRV_IFE_LMT_H_

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

//=====================================================
// input / output limitation for Dram to Dram mode
#define IFE_SRCBUF_D2D_WMIN         64
#define IFE_SRCBUF_D2D_WMAX         8188
#define IFE_SRCBUF_D2D_WALIGN       4

#define IFE_SRCBUF_D2D_HMIN         64
#define IFE_SRCBUF_D2D_HMAX         8190
#define IFE_SRCBUF_D2D_HALIGN       2

#define IFE_SRCBUF_D2D_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IFE_SRCBUF_D2D_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


//=====================================================
// input / output limitation for IPP mode
#define IFE_SRCBUF_IPP_WMIN         64
#define IFE_SRCBUF_IPP_WMAX         8188
#define IFE_SRCBUF_IPP_WALIGN       4

#define IFE_SRCBUF_IPP_HMIN         64
#define IFE_SRCBUF_IPP_HMAX         8190
#define IFE_SRCBUF_IPP_HALIGN       2

#define IFE_SRCBUF_IPP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IFE_SRCBUF_IPP_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE
