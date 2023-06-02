
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


#include <linux/pfn.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>               /* for dma_alloc_coherent */
#include <linux/slab.h>
#include <linux/vmalloc.h>          /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>      /* for dma_alloc_coherent */
#include <linux/platform_device.h>
#include <linux/io.h>
#include <asm/uaccess.h>            /* access_ok(), and VERIFY_WRITE/READ */
#include <linux/ioctl.h>            /* for _IO() macro */
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/time.h>             /* do_gettimeofday() */
#include "gop/mdrv_gop.h"
#include "../include/ms_types.h"
#include "../include/ms_platform.h"


#define FB_DEBUG 1
#define FB_FUNBUG 1
#define fake_HW_acceleration 0


#if defined(CONFIG_ARCH_CHICAGO)
#define UBOOT_SHOW_LOGO 0
#elif defined(CONFIG_ARCH_CEDRIC)
#define UBOOT_SHOW_LOGO 1
#else
#define UBOOT_SHOW_LOGO 1
#endif



#if (FB_DEBUG==1)
#define FBDBG(fmt, arg...) printk(KERN_INFO fmt, ##arg)
#else
#define FBDBG(fmt, arg...)
#endif

#define MstarTrace          printk

#if 0
#if (FB_FUNBUG==1)
#define MstarTrace          printk
#else
#define MstarTrace          if(0) printk
#endif
#endif

#define MstarFB_ENTER(...)  MstarTrace("============ FB ============\r\n%s START\n", __func__)
#define MstarFB_LEAVE(...)  MstarTrace("%s END\n============ FB ============\r\n", __func__)






#ifdef mdrv_GEN_FB_C     /* in frame buffer driver */
#define PRIVATE_FB_DATA
#else   /* in user space */
#define PRIVATE_FB_DATA const
#endif /* mdrv_GEN_FB_C */





/* ========================================================================= */
/* define type cast macro */
#define CAST_2_USER_VOID_P          (void __user *)
#define CAST_2_USER_UINT_P          (u32 __user *)
#define CAST_2_USER_TEXTURE_INFO_P  (TEXTURE_INFO __user *)
#define CAST_2_IOMEM_CHAR_P         (u8 __iomem *)
#define CAST_2_ULONG                (unsigned long)



/* ========================================================================= */

/*
 * Ioctl definitions
 */

/* Use 'F' as magic number.
   in Documnentation/ioctl-number.txt says 'F'    all    linux/fb.h */

#define G3D_IOC_MAGIC  'F'


#define G3D_GETFBPHYADDRESS1    _IO(G3D_IOC_MAGIC,    9)
#define G3D_GETFBPHYADDRESS2    _IO(G3D_IOC_MAGIC,   10)
#define G3D_GETCOMMANDQVIRTUAL  _IO(G3D_IOC_MAGIC,   11)
#define G3D_GETREGISTERVIRTUAL  _IO(G3D_IOC_MAGIC,   14)
#define G3D_GETTEXTUREVIRTUAL   _IO(G3D_IOC_MAGIC,   18)    /* can not use 15 */
#define G3D_FREETEXTUREVIRTUAL  _IO(G3D_IOC_MAGIC,   19)  /* can not use 16 */
#define SET_DISPLAY_COLOR       _IO(G3D_IOC_MAGIC,   20)  /* FOR test case 2, display black, white, red,green, blue,    blue. */
#define CAPTURE_SCREEN          _IO(G3D_IOC_MAGIC,   21)  //Edgar
#define ENABLE_GWIN             _IO(G3D_IOC_MAGIC,   22)  //Edgar
#define G3D_GETDWINPHYADDRESS1  _IO(G3D_IOC_MAGIC,   23)  //Edgar
#define DISABLE_GWIN            _IO(G3D_IOC_MAGIC,   24)  //Edgar
#define G3D_GETDWINPHYADDRESS2  _IO(G3D_IOC_MAGIC,   25)  //Edgar
#define SET_ALPHA               _IO(G3D_IOC_MAGIC,   26)  //Edgar 21Apr2009
#define SET_COLORKEY            _IO(G3D_IOC_MAGIC,   27)  //Edgar 28Apr2009
#define DISABLE_COLORKEY        _IO(G3D_IOC_MAGIC,   28)  //Edgar 28Apr2009
#define CONFIG_PANEL            _IO(G3D_IOC_MAGIC,   29)  //Edgar 18Jun2009
#define SWITCH_GWIN_TO_SCALAR   _IO(G3D_IOC_MAGIC,   30)  //Edgar 01Jul2009
#define SWITCH_GWIN_TO_SCALAR   _IO(G3D_IOC_MAGIC,   30)  //Edgar 01Jul2009
#define FBIOGET_VIDEOMEMSIZE    _IO(G3D_IOC_MAGIC,   31)
#define FBIOGET_CURRENTBUFFERID    _IO(G3D_IOC_MAGIC,   32)
#define USE_CALIBRATION_FRAMEBUFFER        _IO(G3D_IOC_MAGIC,   33)
#define CANCEL_USE_CALIBRATION_FRAMEBUFFER        _IO(G3D_IOC_MAGIC,   34)
#define G3D_GET_EXTDISP_BUFFER  _IO(G3D_IOC_MAGIC,   100)
#define G3D_GET_CALIBRATION_FRAMEBUFFER_IN_USE  _IOR(G3D_IOC_MAGIC,   101, int)

//#define G3D_IOC_MAXNR 34  //Edgar

static int useCalibrationFrameBuffer = 0 ;


/* ========================================================================= */
/* define color */
#define COLOR_MASK        0xffff
#define WHITE_COLOR        0xffff
#define OTHER_COLOR        0xFF00


#define ANDROID_NUMBER_OF_BUFFERS 3


#if defined(CONFIG_ARCH_CEDRIC)
#define ANDROID_FB_FORMAT  RGBA8888
#elif defined(CONFIG_ARCH_CHICAGO)
#define ANDROID_FB_FORMAT  ARGB8888
#endif


#define defult_XRES    800   // PANEL_WIDTH
#define defult_YRES    480   //PANEL_HEIGHT
#if  (ANDROID_FB_FORMAT == RGB565)
#define BPX            16    /* bits per pixel */
#define ANDROID_BYTES_PER_PIXEL 2
#elif ((ANDROID_FB_FORMAT == ARGB8888) || (ANDROID_FB_FORMAT == RGBA8888))
#define BPX            32    /* bits per pixel */
#define ANDROID_BYTES_PER_PIXEL 4
//#elif (ANDROID_FB_FORMAT == ARGB8888)
//#define BPX            32    /* bits per pixel */
//#define ANDROID_BYTES_PER_PIXEL 4
#else
    #ERROR("Not Support")
#endif

#define SUPPORT_EXTERNAL_DISPLAY            1
#if SUPPORT_EXTERNAL_DISPLAY
#define EXTERNAL_DISPLAY_MAX_WIDTH          (720)
#define EXTERNAL_DISPLAY_MAX_HEIGHT         (576)
#define EXTERNAL_DISPLAY_MAX_BUF_NUM        (3)
#define EXTERNAL_DISPLAY_FRAMEBUFFER_SIZE   (4*EXTERNAL_DISPLAY_MAX_WIDTH*EXTERNAL_DISPLAY_MAX_HEIGHT*EXTERNAL_DISPLAY_MAX_BUF_NUM)
#endif

/* ========================================================================= */
/* prototype of function */

static s32 mdrvfb0_ioctl(struct fb_info *info, u32 cmd, unsigned long arg);
static s32 mdrvfb0_mmap(struct fb_info *info, struct vm_area_struct *vma);
static s32 mdrvfb0_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
static s32 mdrvfb0_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
void fb0_fake_fillrect(struct fb_info *info, const struct fb_fillrect *rect);
void fb0_fake_copyarea(struct fb_info *info, const struct fb_copyarea *region);
void fb0_fake_imageblit(struct fb_info *info, const struct fb_image *image);


static struct fb_var_screeninfo sg_mdrvfb0_default_var =
{
    .xres = defult_XRES,
    .yres = defult_YRES,
    .xres_virtual = defult_XRES,
    .yres_virtual = defult_YRES,
    .bits_per_pixel = BPX,
#if  (BPX == 32)
    .transp ={0,8,0},
    .red = { 24, 8, 0 }, /* {offset,length,!= 0 : MSB is right} */
    .green = { 16, 8, 0 },
    .blue = { 8, 8, 0 },
#elif (BPX == 16)
    .red = { 11, 5, 0 }, /* {offset,length,!= 0 : MSB is right} */
    .green = { 5, 6, 0 },
    .blue = { 0, 5, 0 },
#endif
    .activate = FB_ACTIVATE_NOW,
    .height = -1, /* =maximum, in fb.h is delcalred as _u32* */
    .width = -1,

    /*  timing useless ? use the vfb default */
    .pixclock = 100000,//20000, /* pixel clock in ps (pico seconds) */
    .left_margin = 24,//64, /* time from sync to picture */
    .right_margin = 4,//64, /* time from picture to sync */
    .upper_margin = 6,//32, /* time from sync to picture */
    .lower_margin = 4,//32,
    .hsync_len = 64,    /* length of horizontal sync */
    .vsync_len = 2,     /* length of vertical sync */
    .vmode = FB_VMODE_NONINTERLACED,
    /* non interlaced, the most basical framebuffer */
};

/* Device independent unchangeable information */
static struct fb_fix_screeninfo sg_mdrvfb0_fix  =
{
    .id = "mdrv FB",
    .type = FB_TYPE_PACKED_PIXELS,    /* see FB_TYPE_* */
    .visual = FB_VISUAL_TRUECOLOR,    /* see FB_VISUAL_* */
    .xpanstep = 0,  /* zero if no hardware panning */
    .ypanstep = 0,
    .ywrapstep = 0,
    .line_length = (defult_XRES *  BPX  >> 3), /* length of a line in bytes */
    .mmio_start=0x20000000,    /* Start of Memory Mapped I/O   */
                    /* (physical address) */
    .mmio_len=0x10000000,            /* Length of Memory Mapped I/O  */
    .accel = FB_ACCEL_NONE,
    /* Indicate to driver which    specific chip/card we have */
};

static struct fb_videomode sg_mdrvfb0_default_fbmode;

#if fake_HW_acceleration
static struct fb_ops sg_mdrvfb0_ops =
{

    .owner = THIS_MODULE,

    /* the folloing 3 function also refert to fb_sys_fops.ko */
    .fb_fillrect = fb0_fake_fillrect,

    /* Generic fillrect function, provide by Linux kernel, refer to sysfillrect.ko */
    .fb_copyarea = fb0_fake_copyarea,

    /* Generic copyarea function, provide by Linux kernel, refer to syscopyarea.ko */
    .fb_imageblit = fb0_fake_imageblit,

    /* Generic imgblt function, provide by Linux kernel, refer to sysimgblt.ko */
    .fb_mmap = mdrvfb0_mmap, /* for non cached memory */

    /* vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); */
    .fb_ioctl = mdrvfb0_ioctl,

    .fb_pan_display = mdrvfb0_pan_display,

    .fb_check_var = mdrvfb0_check_var,

};
#else
static struct fb_ops sg_mdrvfb0_ops =
{

    .owner = THIS_MODULE,

    /* the folloing 3 function also refert to fb_sys_fops.ko */
    .fb_fillrect = sys_fillrect,

    /* Generic fillrect function, provide by Linux kernel, refer to sysfillrect.ko */
    .fb_copyarea = sys_copyarea,

    /* Generic copyarea function, provide by Linux kernel, refer to syscopyarea.ko */
    .fb_imageblit = sys_imageblit,

    /* Generic imgblt function, provide by Linux kernel, refer to sysimgblt.ko */
    .fb_mmap = mdrvfb0_mmap, /* for non cached memory */

    /* vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); */
    .fb_ioctl = mdrvfb0_ioctl,

    .fb_pan_display = mdrvfb0_pan_display,

    .fb_check_var = mdrvfb0_check_var,

};
#endif
//extern void MDrv_Scaler_Enable_GOP(BOOL);
extern void sys_fillrect(struct fb_info *p, const struct fb_fillrect *rect);

/* virtual address    in Kernel space, used in probe(), remove() */
//static void *sg_pG3D_Fb0_vir_addr1 = 0; /* virtual address of frame buffer 1 */
static void *sg_pG3D_Fb0_vir_addr1 = 0; /* virtual address of frame buffer 2 */

#if 1 //def CONFIG_PM
static int mdrvfb0_suspend(struct platform_device *pdev,
                 pm_message_t state);

static int mdrvfb0_resume(struct platform_device *pdev);
#else
#define mdrvfb0_suspend    NULL
#define mdrvfb0_resume    NULL
#endif



static int mdrvfb0_probe(struct platform_device *dev);
static int mdrvfb0_remove(struct platform_device *dev);
static void mdrvfb0_platform_release(struct device *device);
/* device .name and driver .name must be the same, then it will call
   probe function */
static struct platform_driver sg_mdrvfb0_driver  =
{
    .probe  = mdrvfb0_probe,   /* initiailize */
    .remove = mdrvfb0_remove,  /* it free(mem),
                                    release framebuffer, free irq etc. */
    .suspend    = mdrvfb0_suspend,
    .resume        = mdrvfb0_resume,

    .driver =
    {
        .name = "mdrvfb0",
        .owner    = THIS_MODULE,
    },
};

static u64 sg_mdrvfb0_device_lcd_dmamask = 0xffffffffUL;
/* for 0x08000000 */
static struct platform_device sg_mdrvfb0_device =
{
    .name = "mdrvfb0",
    .id = 0,
    .dev =
    {
        .release = mdrvfb0_platform_release,
        .dma_mask = &sg_mdrvfb0_device_lcd_dmamask,
        .coherent_dma_mask = 0xffffffffUL
    }
};

