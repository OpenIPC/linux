
#ifndef _IPE_ENG_LIMIT_H_
#define _IPE_ENG_LIMIT_H_

//=====================================================
// input / output limitation for Dram to Dram mode
#define IPE_SSDRV_SRCBUF_D2D_WMIN         32
#define IPE_SSDRV_SRCBUF_D2D_WMAX         33652
#define IPE_SSDRV_SRCBUF_D2D_WALIGN       4

#define IPE_SSDRV_SRCBUF_D2D_HMIN         16
#define IPE_SSDRV_SRCBUF_D2D_HMAX         8190
#define IPE_SSDRV_SRCBUF_D2D_HALIGN       4

#define IPE_SSDRV_SRCBUF_D2D_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IPE_SSDRV_SRCBUF_D2D_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


//=====================================================
// input / output limitation for IPP mode
#define IPE_SSDRV_SRCBUF_IPP_WMIN         32
#define IPE_SSDRV_SRCBUF_IPP_WMAX         33652
#define IPE_SSDRV_SRCBUF_ALLDIR_WMAX      2688
#define IPE_SSDRV_SRCBUF_IPP_WALIGN       4

#define IPE_SSDRV_SRCBUF_IPP_HMIN         16
#define IPE_SSDRV_SRCBUF_IPP_HMAX         8190
#define IPE_SSDRV_SRCBUF_IPP_HALIGN       4

#define IPE_SSDRV_SRCBUF_IPP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IPE_SSDRV_SRCBUF_IPP_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


//=====================================================
// LCE input / output limitation
#define IPE_SSDRV_LCE_WMIN         4
#define IPE_SSDRV_LCE_WMAX         32

#define IPE_SSDRV_LCE_HMIN         4
#define IPE_SSDRV_LCE_HMAX         32

#define IPE_SSDRV_LCE_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IPE_SSDRV_LCE_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE




extern INT32 ipe_eng_chk_limitation(UINT32 reg_base_addr, UINT32 reg_flag_addr);



#endif

