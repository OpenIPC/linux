

/******************************************************************************
 *-----------------------------------------------------------------------------
 *
 *  Copyright (c) 2011 MStar Semiconductor, Inc.  All rights reserved.
 *
 *-----------------------------------------------------------------------------
 * FILE NAME      mdrv_general_fb.c
 * DESCRIPTION    For mdrv LinuxBSP Display driver, use frame buffer
 *                architecture
 *
 * AUTHOR         Chun Fan
 *
 *
 * HISTORY        2008/05/06      initial version
 *                linux/drivers/video/mdrv_general_fb.c
 *                -- mdrv frame buffer device
 *****************************************************************************/


#define mdrv_GEN_FB_C


#include "mdrv_fb.h"
#include "ge/columbus2/mhal_ge.h"


//#include "../scaler/mdrv_sca_io.h"
/*
 *  RAM we reserve for the frame buffer. This defines the maximum screen
 *  size
 *
 *  The default can be overridden if the driver is compiled as a module
 */

/* must use u_long, or it will have following message
 * drivers/video/mdrv_general_fb.c: In function '__check_sg_videomemorysize':
 * drivers/video/mdrv_general_fb.c:54: warning:
 * return from incompatible pointer type
 */
 /* frame buffer size */
static u_long sg_videomemorysize;

/* physical address, used in probe(), ioctl(), remove(), CreateGWin() */
/* physical address of frame buffer 1 2  */
dma_addr_t  sg_G3D_Fb0_bus_addr1 = 0;

extern int mstar_sca_drv_get_pnl_width(void);
extern int mstar_sca_drv_get_pnl_height(void);
extern int mstar_sca_drv_get_pnl_refresh_rate(void);
extern void mstar_sca_drv_init_pnl(BOOLEAN bSkipTime);

#if defined(CONFIG_MS_CLONE_SCREEN)
#define OUTPUT_WIDTH 720
#define OUTPUT_HEIGHT 480

typedef unsigned short    MS_U16;
typedef unsigned char     MS_BOOL;

GE_DRAW_RECT drawbuf;
GE_BUFFER_INFO srcbuffer;
GE_BUFFER_INFO dstbuffer;
GE_POINT_t  stClipV0;
GE_POINT_t  stClipV1;
MS_U16 u16output_width = 720;
MS_U16 u16output_height = 480;
MS_BOOL boutput_interlace = 1;

extern unsigned char IsRTKCarBacking(void);
extern void SCA_CloneScr_GetOutputSize(MS_U16 *pu16Width, MS_U16 *pu16Height, MS_BOOL *bInterlace);
#endif

static struct platform_device *psg_mdrv_fbdev0;
//static DECLARE_MUTEX(mstar_panel_alloc_sem);

module_param(sg_videomemorysize, ulong, 0);

static void __init mdrvfb0_updateinfo(void)
{

	static int main_videomemory_size=0;
    FBDBG("mdrvfb0_updateinfo  !\n");

    sg_mdrvfb0_default_var.xres = mstar_sca_drv_get_pnl_width();//tPanelNANOVISON_45->m_wPanelWidth;
    sg_mdrvfb0_default_var.yres = mstar_sca_drv_get_pnl_height();//tPanelNANOVISON_45->m_wPanelHeight;
    main_videomemory_size = (sg_mdrvfb0_default_var.xres*sg_mdrvfb0_default_var.yres * ANDROID_BYTES_PER_PIXEL * (ANDROID_NUMBER_OF_BUFFERS+1));

    sg_mdrvfb0_default_fbmode.refresh=mstar_sca_drv_get_pnl_refresh_rate();
#if SUPPORT_EXTERNAL_DISPLAY
    sg_videomemorysize = main_videomemory_size + EXTERNAL_DISPLAY_FRAMEBUFFER_SIZE;
#else
    sg_videomemorysize = main_videomemory_size;
#endif
    FBDBG ("mdrvfb0_updateinfo: LCD_PANEL_WIDTH = [%d] \r\n",sg_mdrvfb0_default_var.xres);
    FBDBG ("mdrvfb0_updateinfo: LCD_PANEL_HEIGHT = [%d] \r\n",sg_mdrvfb0_default_var.yres);
    FBDBG ("mdrvfb0_updateinfo: sg_videomemorysize = 0x%x \r\n",(U32)sg_videomemorysize);

}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb0_mmap
+------------------------------------------------------------------------------
| DESCRIPTION : memory map function, Let user application can get virtual
|               address to access in user space. need pass in physical address
|
| RETURN      : When the operation is success, it return 0.
|               Otherwise Negative number will be returned.
|
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| pinfo              | x |   | default function type. we only check if NULL
|--------------------+---+---+-------------------------------------------------
| vma                | x | x | we use Remap-pfn-range will mark the range VM_IO
|                    |   |   | and VM_RESERVED, and save these information in
|                    |   |   | vma
+------------------------------------------------------------------------------
*/

static int mdrvfb0_mmap(struct fb_info *pinfo, struct vm_area_struct *vma)
{
    size_t size;
    size = 0;

    MstarFB_ENTER();


    if (NULL == pinfo)
    {
        FBDBG("ERROR: mdrvfb0_mmap, pinfo is NULL pointer !\n");
        return -ENOTTY;
    }
    if (NULL == vma)
    {
        FBDBG("ERROR: mdrvfb0_mmap, vma is NULL pointer !\n");
        return -ENOTTY;
    }

    size = vma->vm_end - vma->vm_start;

    vma->vm_pgoff += ((sg_G3D_Fb0_bus_addr1 ) >> PAGE_SHIFT);

    FBDBG(
    "mdrvfb0_mmap vma->vm_start=%x\n vma->vm_end=%x\n vma->vm_pgoff =%x\n",
    (unsigned int) vma->vm_start, (unsigned int)vma->vm_end ,
    (unsigned int)vma->vm_pgoff);

    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot))
        return -EAGAIN;


    MstarFB_LEAVE();
    return 0;
}





/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb0_ioctl
+------------------------------------------------------------------------------
| DESCRIPTION : IO control code. we provide some customize ioctl code, GLES,
|                and test code. now, we provide the following IOCTL command
|                   G3D_GETVFRAMEVIRTUAL1
|                   G3D_GETVFRAMEVIRTUAL2
|                   G3D_GETCOMMANDQVIRTUAL
|                   G3D_GETREGISTERVIRTUAL
|                   G3D_GETTEXTUREVIRTUAL
|                   G3D_FREETEXTUREVIRTUAL
|                   SET_DISPLAY_COLOR
|                   G3D_IOC_MAXNR 20
|
|                   + define color
|                   COLOR_MASK
|                   WHITE_COLOR
|
| RETURN      : When the operation is success, it return 0.
|               some ioctl will return the physical address for GLES use
|               Otherwise Negative number will be returned.
|
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| pinfo              | x |   | default function type. we only check if NULL
|--------------------+---+---+-------------------------------------------------
| cmd                | x |   | pass in the ioctl command
|--------------------+---+---+-------------------------------------------------
| arg                | x | x | pass in/out argument, from user space or to user
|                    |   |   | space
+------------------------------------------------------------------------------
*/
/* I reserve the unsgined long here,
    because it is the ioctl() function template */
static int
mdrvfb0_ioctl(struct fb_info *pinfo, unsigned int cmd, unsigned long arg)
{
    dma_addr_t  Fb0_PHY_ADD_SHOW=0;//tracy 26Nov2012
    int err=0, ret=0;
    unsigned int ret_Phy=0;


    if (NULL == pinfo)
    {
        FBDBG("ERROR:in mdrvfb0_ioctl, pinfo is NULL pointer\r\n");
        return -ENOTTY;
    }

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != G3D_IOC_MAGIC)
        return -ENOTTY;
//    if (_IOC_NR(cmd) > G3D_IOC_MAXNR)
//        return -ENOTTY;

    /*
     * the type is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. Note that the type is user-oriented, while
     * verify_area is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */

    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, CAST_2_USER_VOID_P arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err =  !access_ok(VERIFY_READ, CAST_2_USER_VOID_P arg, _IOC_SIZE(cmd));

    /* check if occurred error */
    if (err)
        return -EFAULT;

    switch (cmd)
    {


        case G3D_GETFBPHYADDRESS1:
            ret_Phy = sg_G3D_Fb0_bus_addr1;

            //DBG("ret_Phy=%x\r\n",ret_Phy);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;



        case G3D_GETCOMMANDQVIRTUAL:

            FBDBG("in ioctl G3D_GETCOMMANDQVIRTUAL not support \r\n");

            break;


        case G3D_FREETEXTUREVIRTUAL:

            FBDBG("in ioctl G3D_FREETEXTUREVIRTUAL not support \r\n");

            break;

        case G3D_GETREGISTERVIRTUAL:

            FBDBG("in ioctl G3D_GETREGISTERVIRTUAL not support \r\n");

            break;

        case FBIOGET_VIDEOMEMSIZE:
            ret_Phy = sg_videomemorysize;
            FBDBG( "KERNEL- VIDEOMEMSIZE =0x%x" ,(u32)sg_videomemorysize);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);

            break;
        case FBIOGET_CURRENTBUFFERID:
            if (pinfo->var.yoffset >= (pinfo->var.height << 1))
            {
                ret_Phy = 3;
            }
            else if (pinfo->var.yoffset >= pinfo->var.height)
            {
                ret_Phy = 2;
            }
            else
            {
                ret_Phy = 1;
            }


            printk(KERN_INFO "FBIOGET_CURRENTBUFFERID BUFFERID = %d \n",ret_Phy);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;
        case CANCEL_USE_CALIBRATION_FRAMEBUFFER:
            useCalibrationFrameBuffer=0;
            printk("useCalibrationFrameBuffer = %d \n", useCalibrationFrameBuffer);
            Fb0_PHY_ADD_SHOW = sg_G3D_Fb0_bus_addr1;
            GOP_Pan_Display(0, Fb0_PHY_ADD_SHOW);

            break;
        case USE_CALIBRATION_FRAMEBUFFER:
            useCalibrationFrameBuffer=1;
            printk("useCalibrationFrameBuffer = %d \n", useCalibrationFrameBuffer);
            Fb0_PHY_ADD_SHOW = sg_G3D_Fb0_bus_addr1 + ((sg_mdrvfb0_default_var.xres * sg_mdrvfb0_default_var.yres * ANDROID_BYTES_PER_PIXEL)*ANDROID_NUMBER_OF_BUFFERS);
            GOP_Pan_Display(0, Fb0_PHY_ADD_SHOW);
            break;

        case G3D_GET_CALIBRATION_FRAMEBUFFER_IN_USE:
            ret_Phy = useCalibrationFrameBuffer;
            //DBG("ret_Phy=%x\r\n",ret_Phy);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;

        default:  /* redundant, as cmd was checked against MAXNR */
            FBDBG("in default ioct\r\n");
            return -ENOTTY;
    }
    return ret;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb0_platform_release
+------------------------------------------------------------------------------
| DESCRIPTION : when module is unloaded,  this function will be called
|               This is called when the reference count goes to zero.
|
| RETURN      : no return value
|
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| device             | x |   | default function type. we only check if NULL
+------------------------------------------------------------------------------
*/

static void mdrvfb0_platform_release(struct device *device)
{

    MstarFB_ENTER();

    if (NULL == device)
    {
        FBDBG("ERROR: in mdrvfb0_platform_release, \
                device is NULL pointer !\r\n");
    }
    else
    {
        FBDBG("in mdrvfb0_platform_release, module unload!\n");
    }

    MstarFB_LEAVE();
}

#if 0
void    mdrvgop_DispCreateGWin2(struct fb_info *WG)
{

    int xx=0;
    xx++;

    FBDBG("fb%d: mdrv frame buffer device, of video memory\n", WG->node);
}
#endif
static int mdrvfb0_suspend(struct platform_device *pdev,
                 pm_message_t state)
{
    FBDBG(" in mdrvfb0_suspend \r\n");
    return 0;
}

static int mdrvfb0_resume(struct platform_device *pdev)
{
     dma_addr_t Fb0_PHY_ADD_SHOW=0;
     FBDBG("  mdrvfb0_resume \r\n");

     if(unlikely(useCalibrationFrameBuffer))
     {
         Fb0_PHY_ADD_SHOW = sg_G3D_Fb0_bus_addr1 + ((sg_mdrvfb0_default_var.xres * sg_mdrvfb0_default_var.yres * ANDROID_BYTES_PER_PIXEL)*ANDROID_NUMBER_OF_BUFFERS);
         GOP_Setmode(TRUE, 0,sg_mdrvfb0_default_var.xres,sg_mdrvfb0_default_var.yres,Fb0_PHY_ADD_SHOW,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
     }
     else
     {
         GOP_Setmode(TRUE, 0,sg_mdrvfb0_default_var.xres,sg_mdrvfb0_default_var.yres,sg_G3D_Fb0_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
     }

#if defined(CONFIG_MS_CLONE_SCREEN)
    SCA_CloneScr_GetOutputSize(&u16output_width, &u16output_height, &boutput_interlace);
    u16output_height = boutput_interlace ? u16output_height * 2 : u16output_height;
    GOP_Setmode(TRUE, 1,(int)u16output_width, (int)u16output_height,sg_G3D_Fb0_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, (int)boutput_interlace);

    stClipV0.x = 0;
    stClipV0.y = 0;
    stClipV1.x = u16output_width;
    stClipV1.y = u16output_height;

    MHal_GE_SetClip(&stClipV0, &stClipV1);
    MHal_GE_SetSrcBufferInfo(&srcbuffer, 0);
    MHal_GE_SetDstBufferInfo(&dstbuffer, 0);
    MHal_GE_SetAlphaSrcFrom( ABL_FROM_ASRC );
#endif

    //printk("fb0 resume sleep ! \n");
     //msleep(5000);
    return 0;
}
/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb0_probe
+------------------------------------------------------------------------------
| DESCRIPTION : Initialization. Initial HW, fill table, when load module,
|               this function will be called.
|
| RETURN      : When the operation is success, it return 0.
|               Otherwise Negative number will be returned.
|
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| dev                | x |   | default function type. we only check if NULL
+------------------------------------------------------------------------------
*/

static int mdrvfb0_probe(struct platform_device *dev)
{
    struct fb_info *pinfo = NULL;
    int retval = -ENOMEM;

    int fb_format=0;

    MstarFB_ENTER();

    if (NULL == dev)
    {
        FBDBG("ERROR: in mdrvfb0_prob: dev is NULL pointer \r\n");
        return -ENOTTY;
    }

    fb_format=ANDROID_FB_FORMAT;

    // static global variable, pass at probe(), used by dma alloc and dma free in ioctl(),
    psg_mdrv_fbdev0 = dev;

//    FBDBG("mdrvfb0: sg_videomemorysize is 0x%X to allocate screen memory\n",(u32)sg_videomemorysize);

    FBDBG("mdrvfb0: sg_videomemorysize is 0x%X to allocate screen memory\n",(u32)sg_videomemorysize);


    /* For G3D frame buffer2 */
    if (!(sg_pG3D_Fb0_vir_addr1 = dma_alloc_coherent(&dev->dev,
                                                    PAGE_ALIGN(sg_videomemorysize),
                                                    &sg_G3D_Fb0_bus_addr1,
                                                    GFP_KERNEL)))
    {
        FBDBG("mdrvfb0: unable to allocate screen memory\n");
        return retval;
    }

    //printk("---------1 \n");
    //msleep(6000);
    // prevent initial garbage on screen ; to clean memory
//    memset(CAST_2_IOMEM_CHAR_P sg_pG3D_Fb0_vir_addr1,WHITE_COLOR, sg_videomemorysize);
    memset(CAST_2_IOMEM_CHAR_P sg_pG3D_Fb0_vir_addr1,OTHER_COLOR, sg_videomemorysize);

//    FBDBG("mdrvfb0 vir1=0x%x, phy1=0x%x\n", (u32)sg_pG3D_Fb0_vir_addr1, sg_G3D_Fb0_bus_addr1);
    FBDBG("mdrvfb0 vir2=0x%x, phy2=0x%x\n", (u32)sg_pG3D_Fb0_vir_addr1, sg_G3D_Fb0_bus_addr1);

    sg_mdrvfb0_fix.smem_start = sg_G3D_Fb0_bus_addr1;
    sg_mdrvfb0_fix.smem_len = sg_videomemorysize;

    pinfo = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
    if (!pinfo)
        goto err;


    pinfo->screen_base = CAST_2_IOMEM_CHAR_P( sg_pG3D_Fb0_vir_addr1);
    FBDBG( "sg_mdrvfb0_fix.smem_start=0x%x ",(U32) sg_mdrvfb0_fix.smem_start);

    //printk("---------2 \n");
    //msleep(6000);

    pinfo->fbops = &sg_mdrvfb0_ops;
    pinfo->var = sg_mdrvfb0_default_var;
    pinfo->fix = sg_mdrvfb0_fix;
    pinfo->pseudo_palette = pinfo->par;
    pinfo->par = NULL;
    pinfo->flags = FBINFO_FLAG_DEFAULT;


    /* allocate color map */
    /* 256 is following to the vfb.c */
    retval = fb_alloc_cmap(&pinfo->cmap, 256, 0);
    if (retval < 0)
        goto err1;


    pinfo->flags                = FBINFO_FLAG_DEFAULT;
    pinfo->fix.type             = FB_TYPE_PACKED_PIXELS;
    pinfo->fix.visual           = FB_VISUAL_TRUECOLOR;
    pinfo->fix.line_length      = sg_mdrvfb0_default_var.xres * ANDROID_BYTES_PER_PIXEL;
    pinfo->fix.accel            = FB_ACCEL_NONE;
    pinfo->fix.ypanstep         = 1;

    pinfo->var.xres             = sg_mdrvfb0_default_var.xres;
    pinfo->var.yres             = sg_mdrvfb0_default_var.yres;
    pinfo->var.xres_virtual     = sg_mdrvfb0_default_var.xres;
    pinfo->var.yres_virtual     = sg_mdrvfb0_default_var.yres * ANDROID_NUMBER_OF_BUFFERS;
    pinfo->var.bits_per_pixel   = BPX;
    pinfo->var.activate         = FB_ACTIVATE_NOW;
    pinfo->var.height           = sg_mdrvfb0_default_var.yres;
    pinfo->var.width            = sg_mdrvfb0_default_var.xres;
    pinfo->fix.smem_len         = sg_videomemorysize;

    //printk("---------3 \n");
    //msleep(6000);


	if  (RGB565==fb_format)
	{

		pinfo->var.red.offset       = 11;
		pinfo->var.red.length       = 5;
		pinfo->var.green.offset     = 5;
		pinfo->var.green.length     = 6;
		pinfo->var.blue.offset      = 0;
		pinfo->var.blue.length      = 5;
	}
	else if(ARGB8888==fb_format)
	{
		pinfo->var.grayscale = 0;

		pinfo->var.transp.length    = 8;
		pinfo->var.red.length       = 8;
		pinfo->var.green.length     = 8;
		pinfo->var.blue.length      = 8;

		pinfo->var.transp.offset    = 24;
		pinfo->var.red.offset       = 16;
		pinfo->var.green.offset     = 8;
		pinfo->var.blue.offset      = 0;
	}
#if defined(CONFIG_ARCH_CEDRIC)
	else if(RGBA8888==fb_format)
	{
		pinfo->var.grayscale = 0;

		pinfo->var.transp.length    = 8;
		pinfo->var.red.length       = 8;
		pinfo->var.green.length     = 8;
		pinfo->var.blue.length      = 8;

		pinfo->var.transp.offset    = 0;
		pinfo->var.red.offset       = 24;
		pinfo->var.green.offset     = 16;
		pinfo->var.blue.offset      = 8;
	}
#endif
	else
	{

		 printk(KERN_ERR"ERROR!! mdrvfb0_probe fb_format =%d Not Support!!",fb_format);
		 while(1);

	}
    /* register mdrvfb to fbmem.c */
    retval = register_framebuffer(pinfo);
    if (retval < 0)
        goto err2;

    platform_set_drvdata(dev, pinfo);

    //patch some setting due to our panel setting may have problems
    {
    	struct list_head *pos;
    	struct fb_modelist *modelist;

    	list_for_each(pos, &pinfo->modelist) {
    		modelist = list_entry(pos, struct fb_modelist, list);
    		modelist->mode.refresh=sg_mdrvfb0_default_fbmode.refresh;
    	}
    }

    //printk("---------4 \n");
    //msleep(6000);

    /* ======================================================== */
    /* HW INIT related code */
    /* ======================================================== */
    //SETREG16(MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_58 , 0x1);    /* power switch enable */

    //CLRREG16(MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_57, 0x0200);    /* external device power on output enable */
    //SETREG16(MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_57, 0x0002);    /* external device power on enable */

    /* 0. CHIPTOP Register - BIT_9 : Enable RGB output, Bit_14 reg_OP2_en */
    //SETREG16(MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_09 , 0x4200);   /*  */

    //ms_writew( 0x0040,MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_26); //add

    /* h'28 CLK_GE clock control */
    /*
    //weichun temp disable
    CLRREG16(MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_28, 0x1F);
    SETREG16(MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_28 , 0x8);
    */
    /*h'2A CLK_GOPG0 clock control */
    /*

    */
    /* 2. Set CLK_ODCLK's source is LPLL */
    //ms_writew( 0x1C00,MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_1B);


    //CLRREG16(0xA0002604 , 0x0008);

    //ms_writew(0, MS_BASE_REG_CHIPTOP_PA + REG_CHIPTOP_2C );

    /* Create GWIN, make a window to show graph, setting HW(GOP) */
    //mdrvgop_DispCreateGWin();

    // FB info
    // FB Width
    // FB Pitch
    // FB Height
    // FB Starting address
    // FB Format

   //  View info

  //  mdrvgop_DispCreateGWin2(pinfo);
    //mdrvgop_DispCreateGWin(sg_G3D_Fb_bus_addr1,sg_G3D_Fb_bus_addr2,sg_mdrvfb_default_var.xres,sg_mdrvfb_default_var.yres,ANDROID_BYTES_PER_PIXEL);
   #if (UBOOT_SHOW_LOGO==0)
        GOP_Setmode(FALSE, 0,sg_mdrvfb0_default_var.xres,sg_mdrvfb0_default_var.yres,sg_G3D_Fb0_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
   //#else
   //     GOP_Change_Alpha(FALSE, 0,sg_mdrvfb0_default_var.xres,sg_mdrvfb0_default_var.yres,sg_G3D_Fb0_bus_addr2,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
   #endif

#if defined(CONFIG_MS_CLONE_SCREEN)
    SCA_CloneScr_GetOutputSize(&u16output_width, &u16output_height, &boutput_interlace);
    u16output_height = boutput_interlace ? u16output_height * 2 : u16output_height;

    GOP_Setmode(FALSE, 1,(int)u16output_width, (int)u16output_height,sg_G3D_Fb0_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, (int)boutput_interlace);

    drawbuf.srcblk.x = 0;
    drawbuf.srcblk.y = 0;
    drawbuf.srcblk.width = mstar_sca_drv_get_pnl_width();
    drawbuf.srcblk.height = mstar_sca_drv_get_pnl_height();

    drawbuf.dstblk.x = 0;
    drawbuf.dstblk.y = 0;
    drawbuf.dstblk.width = (int)u16output_width;
    drawbuf.dstblk.height = (int)u16output_height;


    srcbuffer.u32Addr = sg_G3D_Fb0_bus_addr2;
    srcbuffer.u32ColorFmt = 0xf;
    srcbuffer.u32Height = mstar_sca_drv_get_pnl_height();
    srcbuffer.u32Width = mstar_sca_drv_get_pnl_width();
    srcbuffer.u32Pitch = (srcbuffer.u32Width << 2);

    dstbuffer.u32Addr = sg_G3D_Fb0_bus_addr1;
    dstbuffer.u32ColorFmt = 0xf;
    dstbuffer.u32Height = (int)u16output_height;
    dstbuffer.u32Width = (int)u16output_width;
    dstbuffer.u32Pitch = (dstbuffer.u32Width << 2);

    stClipV0.x = 0;
    stClipV0.y = 0;
    stClipV1.x = u16output_width;
    stClipV1.y = u16output_height;

    MHal_GE_SetClip(&stClipV0, &stClipV1);

    MHal_GE_SetSrcBufferInfo(&srcbuffer, 0);
    MHal_GE_SetDstBufferInfo(&dstbuffer, 0);
    MHal_GE_SetAlphaSrcFrom( ABL_FROM_ASRC );
#endif

//    FBDBG("fb%d: mdrv frame buffer device, using %ldK of video memory\n", pinfo->node, sg_videomemorysize >> 10);

    //printk("---------5 \n");
    //msleep(6000);
    MstarFB_LEAVE();
    return 0;

err2:
    fb_dealloc_cmap(&pinfo->cmap);
err1:
    framebuffer_release(pinfo);
err:

    dma_free_coherent(&dev->dev,
                      PAGE_ALIGN(sg_videomemorysize),
                      sg_pG3D_Fb0_vir_addr1,
                      sg_G3D_Fb0_bus_addr1);




    return retval;
}




/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb0_remove
+------------------------------------------------------------------------------
| DESCRIPTION : we free memory, release frame buffer here
|
| RETURN      : When the operation is success, it return 0.
|               Otherwise Negative number will be returned.
|
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| dev                | x |   | default function type. we only check if NULL
+------------------------------------------------------------------------------
*/
static int mdrvfb0_remove(struct platform_device *dev)
{
    struct fb_info *pinfo;
    pinfo = 0;


    MstarFB_ENTER();


    if (NULL == dev)
    {
        FBDBG("ERROR: mdrvfb0_remove: dev is NULL pointer \n");
        return -ENOTTY;
    }

    pinfo = platform_get_drvdata(dev);
    if (pinfo)
    {
        unregister_framebuffer(pinfo);

        dma_free_coherent(&dev->dev,
                          PAGE_ALIGN(sg_videomemorysize),
                          sg_pG3D_Fb0_vir_addr1,
                          sg_G3D_Fb0_bus_addr1);

#if defined(CONFIG_ARCH_MSB251X)
        dma_free_coherent(&dev->dev,
                          PAGE_ALIGN(sg_videomemorysize),
                          sg_pG3D_Fb0_vir_addr3,
                          sg_G3D_Fb0_phy_addr3);
#endif



        framebuffer_release(pinfo);
    }

    MstarFB_LEAVE();
    return 0;
}

static int mdrvfb0_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{

    //MstarFB_ENTER();
    dma_addr_t  Fb0_PHY_ADD_SHOW=0;

    if( unlikely(useCalibrationFrameBuffer))
    {
        return 0;
    }
    else
    {

        if (info->var.yoffset >= (info->var.height << 1))
        {
            Fb0_PHY_ADD_SHOW = info->fix.smem_start + ((info->var.width * info->var.height * ANDROID_BYTES_PER_PIXEL) << 1);
        }
        else if (info->var.yoffset >= info->var.height)
        {
            Fb0_PHY_ADD_SHOW = info->fix.smem_start + (info->var.width * info->var.height * ANDROID_BYTES_PER_PIXEL);
        }
        else
        {
            Fb0_PHY_ADD_SHOW = info->fix.smem_start;
        }

        GOP_Pan_Display(0, Fb0_PHY_ADD_SHOW);

#if defined(CONFIG_MS_CLONE_SCREEN)
        if (!IsRTKCarBacking())
        {
            static MS_U16 u16PreH = 480;
            static MS_U16 u16PreW = 720;
            static MS_BOOL bPreInterlace = 1;

            srcbuffer.u32Addr = Fb0_PHY_ADD_SHOW;

            SCA_CloneScr_GetOutputSize(&u16output_width, &u16output_height, &boutput_interlace);
            u16output_height = boutput_interlace ? u16output_height * 2 : u16output_height;

            if (info->var.yoffset >= (info->var.height << 1))
            {
                dstbuffer.u32Addr = sg_G3D_Fb0_bus_addr1 + ((u16output_width * u16output_height * ANDROID_BYTES_PER_PIXEL) << 1);
            }
            else if (info->var.yoffset >= info->var.height)
            {
                dstbuffer.u32Addr = sg_G3D_Fb0_bus_addr1 + (u16output_width * u16output_height * ANDROID_BYTES_PER_PIXEL);
            }
            else
            {
                dstbuffer.u32Addr = sg_G3D_Fb0_bus_addr1;
            }



            if(u16PreW != u16output_width || u16PreH != u16output_height || bPreInterlace != boutput_interlace)
            {
                GOP_Setmode(FALSE, 1,(int)u16output_width, (int)u16output_height,
                    dstbuffer.u32Addr, ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, (int)boutput_interlace);

                stClipV0.x = 0;
                stClipV0.y = 0;
                stClipV1.x = u16output_width;
                stClipV1.y = u16output_height;

                MHal_GE_SetClip(&stClipV0, &stClipV1);
            }
            u16PreW = u16output_width;
            u16PreH = u16output_height;
            bPreInterlace = boutput_interlace;

            drawbuf.dstblk.x = 0;
            drawbuf.dstblk.y = 0;
            drawbuf.dstblk.width = (int)u16output_width;
            drawbuf.dstblk.height = (int)u16output_height;


            dstbuffer.u32Height = (int)u16output_height;
            dstbuffer.u32Width = (int)u16output_width;
            dstbuffer.u32Pitch = (dstbuffer.u32Width << 2);


            MHal_GE_SetSrcBufferInfo(&srcbuffer, 0);
            MHal_GE_SetDstBufferInfo(&dstbuffer, 0);
            MHal_GE_BitBlt(&drawbuf, GEDRAW_FLAG_SCALE);
            GOP_Pan_Display(1, dstbuffer.u32Addr);
        }
#endif

    }

    return 0;
}




/* check var to see if supported by this device.  Probably doesn't
 * need modifying.
 */
static int mdrvfb0_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{

    //MstarFB_ENTER();

    if((var->rotate & 1) != (info->var.rotate & 1))
    {
        if((var->xres != info->var.yres) ||
           (var->yres != info->var.xres) ||
           (var->xres_virtual != info->var.yres) ||
           (var->yres_virtual >
            info->var.xres * ANDROID_NUMBER_OF_BUFFERS) ||
           (var->yres_virtual < info->var.xres ))
           {
            return -EINVAL;
        }
    }
    else
    {
        if((var->xres != info->var.xres) ||
           (var->yres != info->var.yres) ||
           (var->xres_virtual != info->var.xres) ||
           (var->yres_virtual >
            info->var.yres * ANDROID_NUMBER_OF_BUFFERS) ||
           (var->yres_virtual < info->var.yres ))
        {
            return -EINVAL;
        }
    }
    if((var->xoffset != info->var.xoffset) ||
       (var->bits_per_pixel != info->var.bits_per_pixel) ||
       (var->grayscale != info->var.grayscale))
    {
        return -EINVAL;
    }

    //MstarFB_LEAVE();

    return 0;
}





/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb_init
+------------------------------------------------------------------------------
| DESCRIPTION : register the driver to kernel,
|               to tell it what facility or function we provide
|
| RETURN      : When the operation is success, it return 0.
|               Otherwise Negative number will be returned.
|
| Variable Name : no variable
+------------------------------------------------------------------------------
*/

/* register the driver to kernel,
    to tell it what facility or function we provide */
static int __init mdrvfb0_init(void)
{
    int ret = 0;

    MstarFB_ENTER();

    /* register the driver into kernel */

    // how to get the Panel type and what kind of output display device ?


    //MApi_PNL_Init(tPanelNANOVISON_45);
    #if(UBOOT_SHOW_LOGO==1)
    mstar_sca_drv_init_pnl(TRUE);
    #else
    mstar_sca_drv_init_pnl(FALSE);
    #endif
    mdrvfb0_updateinfo();



    ret = platform_driver_register(&sg_mdrvfb0_driver);

    if (!ret)
    {   /* register driver sucess */
        /* register device */
        ret = platform_device_register(&sg_mdrvfb0_device);
        if (ret)    /* if register device fail, then unregister the driver.*/
        {
            platform_driver_unregister(&sg_mdrvfb0_driver);
            printk(KERN_INFO"mdrvfb0_device register failed...\n");
        }
        else
        {
            printk(KERN_INFO"mdrvfb0_device register success...\n");
        }
    }


    MstarFB_LEAVE();

    return ret;
}


void fb0_fake_imageblit(struct fb_info *info, const struct fb_image *image)
{

        MstarFB_ENTER();

        sys_imageblit(info,image);

        MstarFB_LEAVE();

}

void fb0_fake_copyarea(struct fb_info *info, const struct fb_copyarea *region)
{

        MstarFB_ENTER();

        sys_copyarea(info,region);

        MstarFB_LEAVE();

}

void fb0_fake_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{

       MstarFB_ENTER();

       sys_fillrect(info,rect);

       MstarFB_LEAVE();

}

module_init(mdrvfb0_init);   /* tell kernel where the init function */

#ifdef MODULE

/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb0_exit
+------------------------------------------------------------------------------
| DESCRIPTION : cleanup the driver from kernel
|
| RETURN      : no return value
|
| Variable Name : no variable
+------------------------------------------------------------------------------
*/

static void __exit mdrvfb0_exit(void)
{

       MstarFB_ENTER();

       platform_device_unregister(&sg_mdrvfb0_device);

       platform_driver_unregister(&sg_mdrvfb0_driver);

       MstarFB_LEAVE();

}

module_exit(mdrvfb0_exit); /* tel kernel where is the clean up function */

MODULE_LICENSE("GPL");
/* use GPL license to avoid Kernel to complain this driver */
#endif      /* MODULE */





//MODULE_LICENSE("GPL")
