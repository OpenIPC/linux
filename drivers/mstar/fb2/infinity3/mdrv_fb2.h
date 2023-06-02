
/******************************************************************************
 *-----------------------------------------------------------------------------
 *
 *    Copyright (c) 2011 MStar Semiconductor, Inc.  All rights reserved.
 *
 *-----------------------------------------------------------------------------
 * FILE NAME      mdrv_fb.h
 * DESCRIPTION
 *          Including some MACRO needed in mdrvfb.c
 *                    Defined CLRREG16(), SETREG16()
 *          (refer to include/asm-arm/arch-msb25xx/io.h)
 *                    Defined used MASK and setting value
 *                    Defined CHIPTOP, OP2, VOP, GOP Base Addres in Physical Address
 *                    and theirs offset    (drvop2.h and drvgop.h)
 *                    Declared Boot_splash array. the image content shown in boot
 *          (boot_splasy.h)
 *            refering sources list
 *                    drvop2.h and drvgop.h
 *              boot_splasy.h
 *
 * AUTHOR         Chun Fan
 *
 * HISTORY
 *                2008/05/06    Chun    initial version
 *                2008/10/02    Chun    add MSB251x code, add CONFIG_ARCH_MSB251X
 *                                      add some IP base address Macro
 *                                      BK_LPLL_BASE, BK_DAC_BASE,
 *                                      and OP2_DITH_REG_BASE
 *
 *        linux/drivers/video/mdrvfb.h    --    mdrvfb frame buffer device
 ******************************************************************************/

//--------------------------------------------------------------------------------------------------
//  Includes
//--------------------------------------------------------------------------------------------------
#include <linux/pfn.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>                //for dma_alloc_coherent
#include <linux/slab.h>
#include <linux/vmalloc.h>           //seems do not need this
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <asm/uaccess.h>             //access_ok(), and VERIFY_WRITE/READ
#include <linux/ioctl.h>             //for _IO() macro
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/time.h>              //do_gettimeofday()
#include "gop2/mdrv_gop2.h"
#include "inv_color/mdrv_gop2_inv_color.h"
#include "../include/ms_types.h"
#include "../include/ms_platform.h"
#include "../include/ms_msys.h"      //for dma_alloc_coherent
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/kthread.h>

#include "mdrv_fb_io.h"
#include "mdrv_fb_st.h"
#include "mdrv_isp_io_st.h"

//--------------------------------------------------------------------------------------------------
//  Defines
//--------------------------------------------------------------------------------------------------
#define FB2_DEBUG 0  //0 for only error message, 1 for normal debug message, 2 for more message

#if (FB2_DEBUG==0)
#define FB2DBGERR(fmt, arg...)     printk(KERN_INFO fmt, ##arg)
#define FB2DBG(fmt, arg...)
#define FB2DBGMORE(fmt, arg...)
#elif (FB2_DEBUG==1)
#define FB2DBGERR(fmt, arg...)     printk(KERN_INFO fmt, ##arg)
#define FB2DBG(fmt, arg...)        printk(KERN_INFO fmt, ##arg)
#define FB2DBGMORE(fmt, arg...)
#elif (FB2_DEBUG==2)
#define FB2DBGERR(fmt, arg...)     printk(KERN_INFO fmt, ##arg)
#define FB2DBG(fmt, arg...)        printk(KERN_INFO fmt, ##arg)
#define FB2DBGMORE(fmt, arg...)    printk(KERN_INFO fmt, ##arg)
#else
#define FB2DBGERR(fmt, arg...)
#define FB2DBG(fmt, arg...)
#define FB2DBGMORE(fmt, arg...)
#endif

#define CAST_2_USER_VOID_P        (void __user *)
#define CAST_2_USER_UINT_P        (unsigned int __user *)
#define CAST_2_IOMEM_CHAR_P       (u8 __iomem *)

#define ANDROID_NUMBER_OF_BUFFERS 4
#define ANDROID_FB_FORMAT         _8Bit_Palette
#define defult_XRES               0     //init value for xres of fb_var_screeninfo, do not assign value while init
#define defult_YRES               0     //init value for yres of fb_var_screeninfo, do not assign value while init
#if  (ANDROID_FB_FORMAT == RGB565)
#define BPX                       16    //bits per pixel
#define ANDROID_BYTES_PER_PIXEL   2
#elif ((ANDROID_FB_FORMAT == ARGB8888) || (ANDROID_FB_FORMAT == ABGR8888))
#define BPX                       32    //bits per pixel
#define ANDROID_BYTES_PER_PIXEL   4
#elif (ANDROID_FB_FORMAT == _8Bit_Palette)
#define BPX                       8     //bits per pixel
#define ANDROID_BYTES_PER_PIXEL   1
#else
    #ERROR("Not Support")
#endif


//--------------------------------------------------------------------------------------------------
//  Functions
//--------------------------------------------------------------------------------------------------
static int mdrvinfinityfb2_ioctl(struct fb_info *info, u32 cmd, unsigned long arg);
static int mdrvinfinityfb2_mmap(struct fb_info *info, struct vm_area_struct *vma);
static int mdrvinfinityfb2_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
static int mdrvinfinityfb2_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
static int mdrvinfinityfb2_set_par(struct fb_info *info);
static int mdrvinfinityfb2_suspend(struct platform_device *pdev,pm_message_t state);
static int mdrvinfinityfb2_resume(struct platform_device *pdev);
static int mdrvinfinityfb2_probe(struct platform_device *dev);
static int mdrvinfinityfb2_remove(struct platform_device *dev);

//--------------------------------------------------------------------------------------------------
//  Global variable
//--------------------------------------------------------------------------------------------------
struct clk **gop2_clks;                             /* clock define, one for palette sram clock, one for scaler fclk1 *///index=1 is scl_fclk
int num_parents_clocks_gop2                   = 0;  /* number of clocks*/
static void *sg_pG3D_fb2_vir_addr1_gop2       = 0;  /* virtual address of frame buffer */
static u_long sg_videomemorysize_gop2         = 0;  /* frame buffer size */
dma_addr_t sg_G3D_fb2_bus_addr1_gop2          = 0;  /* physical address of frame buffer */

FB_GOP_GWIN_CONFIG genGWinInfo_gop2           ={0}; /* global variable to record gwin information */
unsigned char genGWIN_gop2                    = 1;  /* global variable to record gwin is open or close *///open:1; close:0
FB_GOP_ALPHA_CONFIG genGOPALPHA_gop2          ={0}; /* global variable to record alpha blending information */
FB_GOP_COLORKEY_CONFIG genGOPCOLORKEY_gop2    ={0}; /* global variable to record color key information */

unsigned char genInv_Color_gop2               = 0;  /* global variable to record inverse color is open or close *///open:1; close:0
FB_GOP_INVCOLOR_AE_CONFIG genAE_info_gop2     ={0}; /* global variable to record AE information */
unsigned long genY_Thres_gop2                 = 0;  /* global variable to record Y Threshold */
FB_GOP_INVCOLOR_SCALER_CONFIG genSC_info_gop2 ={0}; /* global variable to record Scaler information */

static struct task_struct *pSWupdateInvThread_gop2=NULL;
unsigned char genTreadisError_gop2            = 0;

static struct task_struct *pSWupdateInvDebugThread_gop2=NULL;

//// {B, G, R, Alpha}
static FB_GOP_PaletteEntry DefaultPaletteEntry[GOP_PALETTE_ENTRY_NUM] =
{
    {{  0,  0,  0,  0}}, {{  0,  0,128,  0}}, {{  0,128,  0,  0}}, {{  0,128,128,  0}},
    {{128,  0,  0,  0}}, {{128,  0,128,  0}}, {{128,128,  0,  0}}, {{192,192,192,  0}},
    {{192,220,192,  0}}, {{240,202,166,  0}}, {{  0, 32, 64,  0}}, {{  0, 32, 96,  0}},
    {{  0, 32,128,  0}}, {{  0, 32,160,  0}}, {{  0, 32,192,  0}}, {{  0, 32,224,  0}},
    {{  0, 64,  0,  0}}, {{  0, 64, 32,  0}}, {{  0, 64, 64,  0}}, {{  0, 64, 96,  0}},
    {{  0, 64,128,  0}}, {{  0, 64,160,  0}}, {{  0, 64,192,  0}}, {{  0, 64,224,  0}},
    {{  0, 96,  0,  0}}, {{  0, 96, 32,  0}}, {{  0, 96, 64,  0}}, {{  0, 96, 96,  0}},
    {{  0, 96,128,  0}}, {{  0, 96,160,  0}}, {{  0, 96,192,  0}}, {{  0, 96,224,  0}},
    {{  0,128,  0,  0}}, {{  0,128, 32,  0}}, {{  0,128, 64,  0}}, {{  0,128, 96,  0}},
    {{  0,128,128,  0}}, {{  0,128,160,  0}}, {{  0,128,192,  0}}, {{  0,128,224,  0}},
    {{  0,160,  0,  0}}, {{  0,160, 32,  0}}, {{  0,160, 64,  0}}, {{  0,160, 96,  0}},
    {{  0,160,128,  0}}, {{  0,160,160,  0}}, {{  0,160,192,  0}}, {{  0,160,224,  0}},
    {{  0,192,  0,  0}}, {{  0,192, 32,  0}}, {{  0,192, 64,  0}}, {{  0,192, 96,  0}},
    {{  0,192,128,  0}}, {{  0,192,160,  0}}, {{  0,192,192,  0}}, {{  0,192,224,  0}},
    {{  0,224,  0,  0}}, {{  0,224, 32,  0}}, {{  0,224, 64,  0}}, {{  0,224, 96,  0}},
    {{  0,224,128,  0}}, {{  0,224,160,  0}}, {{  0,224,192,  0}}, {{  0,224,224,  0}},
    {{ 64,  0,  0,  0}}, {{ 64,  0, 32,  0}}, {{ 64,  0, 64,  0}}, {{ 64,  0, 96,  0}},
    {{ 64,  0,128,  0}}, {{ 64,  0,160,  0}}, {{ 64,  0,192,  0}}, {{ 64,  0,224,  0}},
    {{ 64, 32,  0,  0}}, {{ 64, 32, 32,  0}}, {{ 64, 32, 64,  0}}, {{ 64, 32, 96,  0}},
    {{ 64, 32,128,  0}}, {{ 64, 32,160,  0}}, {{ 64, 32,192,  0}}, {{ 64, 32,224,  0}},
    {{ 64, 64,  0,  0}}, {{ 64, 64, 32,  0}}, {{ 64, 64, 64,  0}}, {{ 64, 64, 96,  0}},
    {{ 64, 64,128,  0}}, {{ 64, 64,160,  0}}, {{ 64, 64,192,  0}}, {{ 64, 64,224,  0}},
    {{ 64, 96,  0,  0}}, {{ 64, 96, 32,  0}}, {{ 64, 96, 64,  0}}, {{ 64, 96, 96,  0}},
    {{ 64, 96,128,  0}}, {{ 64, 96,160,  0}}, {{ 64, 96,192,  0}}, {{ 64, 96,224,  0}},
    {{ 64,128,  0,  0}}, {{ 64,128, 32,  0}}, {{ 64,128, 64,  0}}, {{ 64,128, 96,  0}},
    {{ 64,128,128,  0}}, {{ 64,128,160,  0}}, {{ 64,128,192,  0}}, {{ 64,128,224,  0}},
    {{ 64,160,  0,  0}}, {{ 64,160, 32,  0}}, {{ 64,160, 64,  0}}, {{ 64,160, 96,  0}},
    {{ 64,160,128,  0}}, {{ 64,160,160,  0}}, {{ 64,160,192,  0}}, {{ 64,160,224,  0}},
    {{ 64,192,  0,  0}}, {{ 64,192, 32,  0}}, {{ 64,192, 64,  0}}, {{ 64,192, 96,  0}},
    {{ 64,192,128,  0}}, {{ 64,192,160,  0}}, {{ 64,192,192,  0}}, {{ 64,192,224,  0}},
    {{ 64,224,  0,  0}}, {{ 64,224, 32,  0}}, {{ 64,224, 64,  0}}, {{ 64,224, 96,  0}},
    {{ 64,224,128,  0}}, {{ 64,224,160,  0}}, {{ 64,224,192,  0}}, {{ 64,224,224,  0}},
    {{128,  0,  0,  0}}, {{128,  0, 32,  0}}, {{128,  0, 64,  0}}, {{128,  0, 96,  0}},
    {{128,  0,128,  0}}, {{128,  0,160,  0}}, {{128,  0,192,  0}}, {{128,  0,224,  0}},
    {{128, 32,  0,  0}}, {{128, 32, 32,  0}}, {{128, 32, 64,  0}}, {{128, 32, 96,  0}},
    {{128, 32,128,  0}}, {{128, 32,160,  0}}, {{128, 32,192,  0}}, {{128, 32,224,  0}},
    {{128, 64,  0,  0}}, {{128, 64, 32,  0}}, {{128, 64, 64,  0}}, {{128, 64, 96,  0}},
    {{128, 64,128,  0}}, {{128, 64,160,  0}}, {{128, 64,192,  0}}, {{128, 64,224,  0}},
    {{128, 96,  0,  0}}, {{128, 96, 32,  0}}, {{128, 96, 64,  0}}, {{128, 96, 96,  0}},
    {{128, 96,128,  0}}, {{128, 96,160,  0}}, {{128, 96,192,  0}}, {{128, 96,224,  0}},
    {{128,128,  0,  0}}, {{128,128, 32,  0}}, {{128,128, 64,  0}}, {{128,128, 96,  0}},
    {{128,128,128,  0}}, {{128,128,160,  0}}, {{128,128,192,  0}}, {{128,128,224,  0}},
    {{128,160,  0,  0}}, {{128,160, 32,  0}}, {{128,160, 64,  0}}, {{128,160, 96,  0}},
    {{128,160,128,  0}}, {{128,160,160,  0}}, {{128,160,192,  0}}, {{128,160,224,  0}},
    {{128,192,  0,  0}}, {{128,192, 32,  0}}, {{128,192, 64,  0}}, {{128,192, 96,  0}},
    {{128,192,128,  0}}, {{128,192,160,  0}}, {{128,192,192,  0}}, {{128,192,224,  0}},
    {{128,224,  0,  0}}, {{128,224, 32,  0}}, {{128,224, 64,  0}}, {{128,224, 96,  0}},
    {{128,224,128,  0}}, {{128,224,160,  0}}, {{128,224,192,  0}}, {{128,224,224,  0}},
    {{192,  0,  0,  0}}, {{192,  0, 32,  0}}, {{192,  0, 64,  0}}, {{192,  0, 96,  0}},
    {{192,  0,128,  0}}, {{192,  0,160,  0}}, {{192,  0,192,  0}}, {{192,  0,224,  0}},
    {{192, 32,  0,  0}}, {{192, 32, 32,  0}}, {{192, 32, 64,  0}}, {{192, 32, 96,  0}},
    {{192, 32,128,  0}}, {{192, 32,160,  0}}, {{192, 32,192,  0}}, {{192, 32,224,  0}},
    {{192, 64,  0,  0}}, {{192, 64, 32,  0}}, {{192, 64, 64,  0}}, {{192, 64, 96,  0}},
    {{192, 64,128,  0}}, {{192, 64,160,  0}}, {{192, 64,192,  0}}, {{192, 64,224,  0}},
    {{192, 96,  0,  0}}, {{192, 96, 32,  0}}, {{192, 96, 64,  0}}, {{192, 96, 96,  0}},
    {{192, 96,128,  0}}, {{192, 96,160,  0}}, {{192, 96,192,  0}}, {{192, 96,224,  0}},
    {{192,128,  0,  0}}, {{192,128, 32,  0}}, {{192,128, 64,  0}}, {{192,128, 96,  0}},
    {{192,128,128,  0}}, {{192,128,160,  0}}, {{192,128,192,  0}}, {{192,128,224,  0}},
    {{192,160,  0,  0}}, {{192,160, 32,  0}}, {{192,160, 64,  0}}, {{192,160, 96,  0}},
    {{192,160,128,  0}}, {{192,160,160,  0}}, {{192,160,192,  0}}, {{192,160,224,  0}},
    {{192,192,  0,  0}}, {{192,192, 32,  0}}, {{192,192, 64,  0}}, {{192,192, 96,  0}},
    {{192,192,128,  0}}, {{192,192,160,  0}}, {{240,251,255,  0}}, {{164,160,160,  0}},
    {{128,128,128,  0}}, {{  0,  0,255,  0}}, {{  0,255,  0,  0}}, {{  0,255,255,  0}},
    {{255,  0,  0,  0}}, {{255,  0,255,  0}}, {{255,255,  0,  0}}, {{255,255,255,  0}}
 };


FB_GOP_PaletteEntry stGOPCurrentPalInfo_GOP2[GOP_PALETTE_ENTRY_NUM]={};


static const char* KEY_DMEM_FB2_BUF="FB2_BUFFER";


//-------------------------------------------------------------------------------------------------
//  Data structure
//-------------------------------------------------------------------------------------------------
static struct fb_var_screeninfo sg_mdrvinfinityfb2_default_var =
{
    .xres           = defult_XRES, /* visible resolution*/ //display x size
    .yres           = defult_YRES,                    //display y size
    .xres_virtual   = defult_XRES, /* virtual resolution*/ //framebuffer x size
    .yres_virtual   = defult_YRES,                    //framebuffer y size
    .xoffset        = 0,           /* offset from virtual to visible */
    .yoffset        = 0,

    .bits_per_pixel = BPX,

#if  (ANDROID_FB_FORMAT  == _8Bit_Palette)
    .grayscale = 0,
    .transp    = {0,0,0},
    .red       = {0,0,0},  /* {offset,length,!= 0 : MSB is right} */
    .green     = {0,0,0},
    .blue      = {0,0,0},
#elif (ANDROID_FB_FORMAT == RGB565)   //Data: R G B, 5 6 5
    .red       = {11,5,0}, /* {offset,length,!= 0 : MSB is right} */
    .green     = {5,6,0},
    .blue      = {0,5,0},
#elif (ANDROID_FB_FORMAT == ARGB8888) //Data: R G B A, 8 8 8 8
    .grayscale = 0,
    .transp    = {0,8,0},
    .red       = {24,8,0}, /* {offset,length,!= 0 : MSB is right} */
    .green     = {16,8,0},
    .blue      = {8,8,0},
#endif

    .activate  = FB_ACTIVATE_NOW,
    .height    = defult_XRES, /* height of picture in mm */
    .width     = defult_YRES, /* width of picture in mm  */

    /*  timing settings, useless here*/
    .pixclock     = 100000, /* pixel clock in ps (pico seconds) */
    .left_margin  = 24,     /* time from sync to picture */
    .right_margin = 4,      /* time from picture to sync */
    .upper_margin = 6,      /* time from sync to picture */
    .lower_margin = 4,
    .hsync_len    = 64,     /* length of horizontal sync */
    .vsync_len    = 2,      /* length of vertical sync */
    .vmode        = FB_VMODE_NONINTERLACED,
};

/* Device independent unchangeable information */
static struct fb_fix_screeninfo sg_mdrvinfinityfb2_fix  =
{
    .id          = "FB2_gop2",            /* identification string eg "TT Builtin"*/
    .smem_start  = 0,                     /* Start of frame buffer mem*/
    .smem_len    = 0,                     /* Length of frame buffer mem*/
    .type        = FB_TYPE_PACKED_PIXELS, /* see FB_TYPE_*/
    .visual      = FB_VISUAL_TRUECOLOR,   /* see FB_VISUAL_*/
    .xpanstep    = 0,                     /* zero if no hardware panning*/
    .ypanstep    = ANDROID_NUMBER_OF_BUFFERS, /* zero if no hardware panning*/
    .ywrapstep   = 0,                     /* zero if no hardware ywrap*/
    .line_length = (defult_XRES * ANDROID_BYTES_PER_PIXEL), /* length of a line in bytes*/
    .mmio_start  = 0x20000000,            /* Start of Memory Mapped I/O*/
                                          /* (physical address)*/
    .mmio_len    = 0x10000000,            /* Length of Memory Mapped I/O*/
    .accel       = FB_ACCEL_NONE,         /* Type of acceleration available*/
};

static struct fb_ops sg_mdrvinfinityfb2_ops =
{
    .owner = THIS_MODULE,

    /* the folloing 3 function also refert to fb_sys_fops.ko */
    .fb_fillrect  = sys_fillrect,  /* Generic fillrect function, provide by Linux kernel, refer to sysfillrect.ko */
    .fb_copyarea  = sys_copyarea,  /* Generic copyarea function, provide by Linux kernel, refer to syscopyarea.ko */
    .fb_imageblit = sys_imageblit, /* Generic imgblt function, provide by Linux kernel, refer to sysimgblt.ko */

    .fb_mmap        = mdrvinfinityfb2_mmap, /* for non cached memory */
    .fb_ioctl       = mdrvinfinityfb2_ioctl,
    .fb_pan_display = mdrvinfinityfb2_pan_display,
    .fb_check_var   = mdrvinfinityfb2_check_var,
    .fb_set_par     = mdrvinfinityfb2_set_par,
};


static const struct of_device_id infinity_infinityfb2_dt_ids[] =
{
    { .compatible = "mstar,infinity-gop2" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, infinity_infinityfb2_dt_ids);

static struct platform_driver sg_mdrvinfinityfb2_driver  =
{
    .probe   = mdrvinfinityfb2_probe,   /* initiailize */
    .remove  = mdrvinfinityfb2_remove,  /* it free(mem), release framebuffer, free irq etc. */
    .suspend = mdrvinfinityfb2_suspend,
    .resume  = mdrvinfinityfb2_resume,

    .driver  =
    {
        .name           = "mdrvinfinityfb2",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(infinity_infinityfb2_dt_ids),
    },
};
