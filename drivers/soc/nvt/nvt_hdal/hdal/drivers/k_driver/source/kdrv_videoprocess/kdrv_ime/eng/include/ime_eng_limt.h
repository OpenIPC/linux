
#ifndef _IME_ENG_LIMIT_H_
#define _IME_ENG_LIMIT_H_

//=====================================================
// input limitation
#define IME_SSDRV_SRCBUF_WMIN         4
#define IME_SSDRV_SRCBUF_WMAX         65532
#define IME_SSDRV_SRCBUF_ALLDIR_WMAX  2688

#define IME_SSDRV_SRCBUF_WALIGN       4

#define IME_SSDRV_SRCBUF_HMIN         4
#define IME_SSDRV_SRCBUF_HMAX         65532
#define IME_SSDRV_SRCBUF_HALIGN       4

#define IME_SSDRV_SRCBUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_SSDRV_SRCBUF_ADDRALIGN    DRV_LIMIT_ALIGN_WORD

//=====================================================
// lca limitation
#define IME_SSDRV_LCABUF_WMIN         40
#define IME_SSDRV_LCABUF_WMAX         65532
#define IME_SSDRV_LCABUF_WALIGN       4

#define IME_SSDRV_LCABUF_HMIN         30
#define IME_SSDRV_LCABUF_HMAX         65532
#define IME_SSDRV_LCABUF_HALIGN       2

#define IME_SSDRV_LCABUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_SSDRV_LCABUF_ADDRALIGN    DRV_LIMIT_ALIGN_WORD


//=====================================================
// output limitation
#define IME_SSDRV_DSTBUF_WMIN         4
#define IME_SSDRV_DSTBUF_WMAX         65532
#define IME_SSDRV_DSTBUF_WALIGN       2

#define IME_SSDRV_DSTBUF_HMIN         4
#define IME_SSDRV_DSTBUF_HMAX         65532
#define IME_SSDRV_DSTBUF_HALIGN       2

#define IME_SSDRV_DSTBUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_SSDRV_DSTBUF_ADDRALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// scaling limitation
#define IME_SSDRV_SCALE_MAX      2097145

//=====================================================
// integration
#define IME_SSDRV_SCALE_INTG_MIN    131072    // 2x
#define IME_SSDRV_SCALE_INTG_MAX    1048576   // 16x


//=====================================================
// OSD input limitation
#define IME_SSDRV_OSDBUF_WMIN         8
#define IME_SSDRV_OSDBUF_WMAX         8192
#define IME_SSDRV_OSDBUF_WALIGN       2

#define IME_SSDRV_OSDBUF_HMIN         8
#define IME_SSDRV_OSDBUF_HMAX         8192
#define IME_SSDRV_OSDBUF_HALIGN       2

#define IME_SSDRV_OSDBUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_SSDRV_OSDBUF_ADDRALIGN    DRV_LIMIT_ALIGN_HALFWORD

//=====================================================
// Privacy-mask pixelation input limitation
#define IME_SSDRV_PM_PXLBUF_WMIN         2
#define IME_SSDRV_PM_PXLBUF_WMAX         2048
#define IME_SSDRV_PM_PXLBUF_WALIGN       2

#define IME_SSDRV_PM_PXLBUF_HMIN         2
#define IME_SSDRV_PM_PXLBUF_HMAX         2048
#define IME_SSDRV_PM_PXLBUF_HALIGN       2

#define IME_SSDRV_PM_PXLBUF_LOFFALIGN    DRV_LIMIT_ALIGN_WORD
#define IME_SSDRV_PM_PXLBUF_ADDRALIGN    DRV_LIMIT_ALIGN_HALFWORD
//=====================================================



extern INT32 ime_eng_chk_limitation(UINT32 reg_base_addr, UINT32 reg_flag_addr);



#endif

