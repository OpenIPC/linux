
/**
 * @file kdrv_grph_lmt.h
 * @brief parameter limitation of KDRV ISE
 * @author ISP
 * @date in the year 2019
 */

#ifndef _KDRV_ISE_LIMIT_H_
#define _KDRV_ISE_LIMIT_H_

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

//=====================================================
// input limitation for Y-8bit
#define ISE_SRCBUF_Y8BIT_WMIN         8
#define ISE_SRCBUF_Y8BIT_WMAX         65535
#define ISE_SRCBUF_Y8BIT_WALIGN       1

#define ISE_SRCBUF_Y8BIT_HMIN         8
#define ISE_SRCBUF_Y8BIT_HMAX         65535
#define ISE_SRCBUF_Y8BIT_HALIGN       1

#define ISE_SRCBUF_Y8BIT_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_SRCBUF_Y8BIT_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE

//=====================================================
// input limitation for Y-4bit
#define ISE_SRCBUF_Y4BIT_WMIN         8
#define ISE_SRCBUF_Y4BIT_WMAX         65534
#define ISE_SRCBUF_Y4BIT_WALIGN       2

#define ISE_SRCBUF_Y4BIT_HMIN         8
#define ISE_SRCBUF_Y4BIT_HMAX         65535
#define ISE_SRCBUF_Y4BIT_HALIGN       1

#define ISE_SRCBUF_Y4BIT_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_SRCBUF_Y4BIT_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


//=====================================================
// input limitation for UV-packing
#define ISE_SRCBUF_UVP_WMIN         8
#define ISE_SRCBUF_UVP_WMAX         65535
#define ISE_SRCBUF_UVP_WALIGN       1

#define ISE_SRCBUF_UVP_HMIN         8
#define ISE_SRCBUF_UVP_HMAX         65535
#define ISE_SRCBUF_UVP_HALIGN       1

#define ISE_SRCBUF_UVP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_SRCBUF_UVP_ADDR_ALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// input limitation for YUV-packing
#define ISE_SRCBUF_YUVP_WMIN         8
#define ISE_SRCBUF_YUVP_WMAX         65535
#define ISE_SRCBUF_YUVP_WALIGN       1

#define ISE_SRCBUF_YUVP_HMIN         8
#define ISE_SRCBUF_YUVP_HMAX         65535
#define ISE_SRCBUF_YUVP_HALIGN       1

#define ISE_SRCBUF_YUVP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_SRCBUF_YUVP_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE

//=====================================================
// input limitation for RGB565
#define ISE_SRCBUF_RGB565_WMIN         8
#define ISE_SRCBUF_RGB565_WMAX         65535
#define ISE_SRCBUF_RGB565_WALIGN       1

#define ISE_SRCBUF_RGB565_HMIN         8
#define ISE_SRCBUF_RGB565_HMAX         65535
#define ISE_SRCBUF_RGB565_HALIGN       1

#define ISE_SRCBUF_RGB565_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_SRCBUF_RGB565_ADDR_ALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// input limitation for RGB1555
#define ISE_SRCBUF_RGB1555_WMIN         8
#define ISE_SRCBUF_RGB1555_WMAX         65535
#define ISE_SRCBUF_RGB1555_WALIGN       1

#define ISE_SRCBUF_RGB1555_HMIN         8
#define ISE_SRCBUF_RGB1555_HMAX         65535
#define ISE_SRCBUF_RGB1555_HALIGN       1

#define ISE_SRCBUF_RGB1555_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_SRCBUF_RGB1555_ADDR_ALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// input limitation for RGB4444
#define ISE_SRCBUF_RGB4444_WMIN         8
#define ISE_SRCBUF_RGB4444_WMAX         65535
#define ISE_SRCBUF_RGB4444_WALIGN       1

#define ISE_SRCBUF_RGB4444_HMIN         8
#define ISE_SRCBUF_RGB4444_HMAX         65535
#define ISE_SRCBUF_RGB4444_HALIGN       1

#define ISE_SRCBUF_RGB4444_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_SRCBUF_RGB4444_ADDR_ALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// input limitation for RGB8888
#define ISE_SRCBUF_RGB8888_WMIN         8
#define ISE_SRCBUF_RGB8888_WMAX         65535
#define ISE_SRCBUF_RGB8888_WALIGN       1

#define ISE_SRCBUF_RGB8888_HMIN         8
#define ISE_SRCBUF_RGB8888_HMAX         65535
#define ISE_SRCBUF_RGB8888_HALIGN       1

#define ISE_SRCBUF_RGB8888_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_SRCBUF_RGB8888_ADDR_ALIGN    DRV_LIMIT_ALIGN_WORD

//=====================================================
// output limitation for Y-8bit
#define ISE_DSTBUF_Y8BIT_WMIN         8
#define ISE_DSTBUF_Y8BIT_WMAX         65535
#define ISE_DSTBUF_Y8BIT_WALIGN       1

#define ISE_DSTBUF_Y8BIT_HMIN         8
#define ISE_DSTBUF_Y8BIT_HMAX         65535
#define ISE_DSTBUF_Y8BIT_HALIGN       1

#define ISE_DSTBUF_Y8BIT_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_DSTBUF_Y8BIT_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE

//=====================================================
// output limitation for Y-4bit
#define ISE_DSTBUF_Y4BIT_WMIN         8
#define ISE_DSTBUF_Y4BIT_WMAX         65534
#define ISE_DSTBUF_Y4BIT_WALIGN       2

#define ISE_DSTBUF_Y4BIT_HMIN         8
#define ISE_DSTBUF_Y4BIT_HMAX         65535
#define ISE_DSTBUF_Y4BIT_HALIGN       1

#define ISE_DSTBUF_Y4BIT_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_DSTBUF_Y4BIT_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


//=====================================================
// output limitation for UV-packing
#define ISE_DSTBUF_UVP_WMIN         8
#define ISE_DSTBUF_UVP_WMAX         65535
#define ISE_DSTBUF_UVP_WALIGN       1

#define ISE_DSTBUF_UVP_HMIN         8
#define ISE_DSTBUF_UVP_HMAX         65535
#define ISE_DSTBUF_UVP_HALIGN       1

#define ISE_DSTBUF_UVP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_DSTBUF_UVP_ADDR_ALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// output limitation for YUV-packing
#define ISE_DSTBUF_YUVP_WMIN         8
#define ISE_DSTBUF_YUVP_WMAX         65535
#define ISE_DSTBUF_YUVP_WALIGN       1

#define ISE_DSTBUF_YUVP_HMIN         8
#define ISE_DSTBUF_YUVP_HMAX         65535
#define ISE_DSTBUF_YUVP_HALIGN       1

#define ISE_DSTBUF_YUVP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_DSTBUF_YUVP_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE

//=====================================================
// output limitation for RGB565
#define ISE_DSTBUF_RGB565_WMIN         8
#define ISE_DSTBUF_RGB565_WMAX         65535
#define ISE_DSTBUF_RGB565_WALIGN       1

#define ISE_DSTBUF_RGB565_HMIN         8
#define ISE_DSTBUF_RGB565_HMAX         65535
#define ISE_DSTBUF_RGB565_HALIGN       1

#define ISE_DSTBUF_RGB565_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_DSTBUF_RGB565_ADDR_ALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// output limitation for RGB1555
#define ISE_DSTBUF_RGB1555_WMIN         8
#define ISE_DSTBUF_RGB1555_WMAX         65535
#define ISE_DSTBUF_RGB1555_WALIGN       1

#define ISE_DSTBUF_RGB1555_HMIN         8
#define ISE_DSTBUF_RGB1555_HMAX         65535
#define ISE_DSTBUF_RGB1555_HALIGN       1

#define ISE_DSTBUF_RGB1555_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_DSTBUF_RGB1555_ADDR_ALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// output limitation for RGB4444
#define ISE_DSTBUF_RGB4444_WMIN         8
#define ISE_DSTBUF_RGB4444_WMAX         65535
#define ISE_DSTBUF_RGB4444_WALIGN       1

#define ISE_DSTBUF_RGB4444_HMIN         8
#define ISE_DSTBUF_RGB4444_HMAX         65535
#define ISE_DSTBUF_RGB4444_HALIGN       1

#define ISE_DSTBUF_RGB4444_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_DSTBUF_RGB4444_ADDR_ALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// output limitation for RGB8888
#define ISE_DSTBUF_RGB8888_WMIN         8
#define ISE_DSTBUF_RGB8888_WMAX         65535
#define ISE_DSTBUF_RGB8888_WALIGN       1

#define ISE_DSTBUF_RGB8888_HMIN         8
#define ISE_DSTBUF_RGB8888_HMAX         65535
#define ISE_DSTBUF_RGB8888_HALIGN       1

#define ISE_DSTBUF_RGB8888_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define ISE_DSTBUF_RGB8888_ADDR_ALIGN    DRV_LIMIT_ALIGN_WORD

#endif


