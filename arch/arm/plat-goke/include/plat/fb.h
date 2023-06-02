/*!
*****************************************************************************
** \file        arch/arm/mach-gk/include/plat/fb.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __PLAT_FB_H
#define __PLAT_FB_H

/* ==========================================================================*/
#define GK_CLUT_BYTES           (3)
#define GK_CLUT_TABLE_SIZE      (256 * GK_CLUT_BYTES)
#define GK_BLEND_TABLE_SIZE     (256)

/* ==========================================================================*/
#ifndef __ASSEMBLER__
#include "linux/fb.h"
enum gk_fb_color_format {
    GK_FB_COLOR_AUTO = 0,

    GK_FB_COLOR_CLUT_8BPP,
    GK_FB_COLOR_RGB565,

    GK_FB_COLOR_BGR565,
    GK_FB_COLOR_AGBR4444,    //AYUV 4:4:4:4
    GK_FB_COLOR_RGBA4444,
    GK_FB_COLOR_BGRA4444,
    GK_FB_COLOR_ABGR4444,
    GK_FB_COLOR_ARGB4444,
    GK_FB_COLOR_AGBR1555,    //AYUV 1:5:5:5
    GK_FB_COLOR_GBR1555,    //YUV 1(ignored):5:5:5
    GK_FB_COLOR_RGBA5551,
    GK_FB_COLOR_BGRA5551,
    GK_FB_COLOR_ABGR1555,
    GK_FB_COLOR_ARGB1555,
    GK_FB_COLOR_AGBR8888,    //AYUV 8:8:8:8
    GK_FB_COLOR_RGBA8888,
    GK_FB_COLOR_BGRA8888,
    GK_FB_COLOR_ABGR8888,
    GK_FB_COLOR_ARGB8888,

    GK_FB_COLOR_YUV565,
    GK_FB_COLOR_AYUV4444,
    GK_FB_COLOR_AYUV1555,
    GK_FB_COLOR_YUV555,

    GK_FB_COLOR_UNSUPPORTED,  //Reserved only, not supported
};

typedef enum
{
    GK_DSP_ENCODE_MODE      = 0x00,
    GK_DSP_DECODE_MODE      = 0x01,
    GK_DSP_RESET_MODE       = 0x02,
    GK_DSP_UNKNOWN_MODE     = 0x03,
    GK_DSP_QUICKLOGO_MODE   = 0x04,
}gk_dsp_op_mode_e;

enum gk_fb_status {
    GK_FB_UNKNOWN_MODE      = 0x00,
    GK_FB_ACTIVE_MODE,
    GK_FB_STOP_MODE,
};

typedef int (*gk_fb_pan_display_fn)(struct fb_var_screeninfo *var,
    struct fb_info *info);
typedef int (*gk_fb_setcmap_fn)(struct fb_cmap *cmap,
    struct fb_info *info);
typedef int (*gk_fb_check_var_fn)(struct fb_var_screeninfo *var,
    struct fb_info *info);
typedef int (*gk_fb_set_par_fn)(struct fb_info *info);
typedef int (*gk_fb_blank_fn)(int blank_mode, struct fb_info *info);

struct gk_fb_cvs_buf {        //Conversion Buffer
    int                available;
    u8                *ping_buf;
    u32                ping_buf_size;
    u8                *pong_buf;
    u32                pong_buf_size;
};

struct gk_fb_media_info {
    struct fb_var_screeninfo    screen_var;
    struct fb_fix_screeninfo    screen_fix;
    gk_dsp_op_mode_e            dsp_status;

    gk_fb_pan_display_fn    pan_display;
    gk_fb_setcmap_fn        setcmap;
    gk_fb_check_var_fn      check_var;
    gk_fb_set_par_fn        set_par;
    gk_fb_blank_fn          set_blank;
};

struct gk_platform_fb {
    struct mutex                lock;
    struct fb_var_screeninfo    screen_var;
    struct fb_fix_screeninfo    screen_fix;
    gk_dsp_op_mode_e            dsp_status;
    enum gk_fb_status       fb_status;
    u8                          clut_table[GK_CLUT_TABLE_SIZE];
    u8                          blend_table[GK_BLEND_TABLE_SIZE];
    enum gk_fb_color_format color_format;
    struct gk_fb_cvs_buf    conversion_buf;
    u32                         use_prealloc;
    u32                         prealloc_line_length;

    gk_fb_pan_display_fn    pan_display;
    gk_fb_setcmap_fn        setcmap;
    gk_fb_check_var_fn      check_var;
    gk_fb_set_par_fn        set_par;
    gk_fb_blank_fn          set_blank;

    struct fb_info              *proc_fb_info;
    struct proc_dir_entry       *proc_file;
    wait_queue_head_t           proc_wait;
    u32                         proc_wait_flag;
};

/* ==========================================================================*/

/* ==========================================================================*/
extern int gk_fb_get_platform_info(u32, struct gk_platform_fb *);
extern int gk_fb_set_media_info(u32, struct gk_fb_media_info *);
extern int gk_fb_update_info(u32 fb_id, int xres, int yres,
    int xvirtual, int yvirtual, int format, u32 bits_per_pixel,
    u32 smem_start, u32 smem_len);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

