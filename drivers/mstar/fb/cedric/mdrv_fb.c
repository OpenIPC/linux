

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
#include "mdrv_fb_io.h"
#include "mdrv_fb_st.h"
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
dma_addr_t  sg_G3D_fb2_bus_addr1 = 0;

extern int mstar_sca_drv_get_pnl_width(void);
extern int mstar_sca_drv_get_pnl_height(void);
extern int mstar_sca_drv_get_pnl_refresh_rate(void);
extern void mstar_sca_drv_init_pnl(BOOLEAN bSkipTime);

static struct platform_device *psg_mdrv_fbdev0;

FB_GOP_GWIN_CONFIG genGWinInfo={0};

//static DECLARE_MUTEX(mstar_panel_alloc_sem);

module_param(sg_videomemorysize, ulong, 0);

static void __init mdrvfb2_updateinfo(void)
{

	static int main_videomemory_size=0;
    FBDBG("mdrvfb2_updateinfo  !\n");

    sg_mdrvfb2_default_var.xres = 720;//fix size in Infinity//mstar_sca_drv_get_pnl_width();
    sg_mdrvfb2_default_var.yres = 480;//fix size in Infinity//mstar_sca_drv_get_pnl_height();
    //GWin init, same size as screen
    genGWinInfo.u18HStart = 0;
    genGWinInfo.u18HEnd   = genGWinInfo.u18HStart+sg_mdrvfb2_default_var.xres;
    genGWinInfo.u18VStart = 0;
    genGWinInfo.u18VEnd   = genGWinInfo.u18VStart+sg_mdrvfb2_default_var.yres;
    main_videomemory_size = (sg_mdrvfb2_default_var.xres*sg_mdrvfb2_default_var.yres * ANDROID_BYTES_PER_PIXEL * (ANDROID_NUMBER_OF_BUFFERS+1));

    sg_mdrvfb2_default_fbmode.refresh=mstar_sca_drv_get_pnl_refresh_rate();

    sg_videomemorysize = main_videomemory_size;

    FBDBG ("mdrvfb2_updateinfo: LCD_PANEL_WIDTH = [%d] \r\n",sg_mdrvfb2_default_var.xres);
    FBDBG ("mdrvfb2_updateinfo: LCD_PANEL_HEIGHT = [%d] \r\n",sg_mdrvfb2_default_var.yres);
    FBDBG ("mdrvfb2_updateinfo: sg_videomemorysize = 0x%x \r\n",(unsigned int)sg_videomemorysize);

}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb2_mmap
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

static int mdrvfb2_mmap(struct fb_info *pinfo, struct vm_area_struct *vma)
{
    size_t size;
    size = 0;

    MstarFB_ENTER();


    if (NULL == pinfo)
    {
        FBDBG("ERROR: mdrvfb2_mmap, pinfo is NULL pointer !\n");
        return -ENOTTY;
    }
    if (NULL == vma)
    {
        FBDBG("ERROR: mdrvfb2_mmap, vma is NULL pointer !\n");
        return -ENOTTY;
    }

    size = vma->vm_end - vma->vm_start;

    vma->vm_pgoff += ((sg_G3D_fb2_bus_addr1 ) >> PAGE_SHIFT);

    FBDBG(
    "mdrvfb2_mmap vma->vm_start=%x\n vma->vm_end=%x\n vma->vm_pgoff =%x\n",
    (unsigned int) vma->vm_start, (unsigned int)vma->vm_end ,
    (unsigned int)vma->vm_pgoff);

    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot))
        return -EAGAIN;


    MstarFB_LEAVE();
    return 0;
}

static const char* KEY_DMEM_FB_BUF="FB_BUFFER";


static void* alloc_dmem(const char* name, unsigned int size, dma_addr_t *addr)
{
	MSYS_DMEM_INFO dmem;
	memcpy(dmem.name,name,strlen(name)+1);
	dmem.length=size;
	if(0!=msys_request_dmem(&dmem)){
		return NULL;
	}

	*addr=dmem.phys;
	return (void *)((uintptr_t)dmem.kvirt);
}

static void free_dmem(const char* name, unsigned int size, void *virt, dma_addr_t addr)
{
	MSYS_DMEM_INFO dmem;
	memcpy(dmem.name,name,strlen(name)+1);
	dmem.length=size;
	dmem.kvirt=(unsigned long long)((uintptr_t)virt);
	dmem.phys=(unsigned long long)((uintptr_t)addr);
	msys_release_dmem(&dmem);
}


// FB Ioctl Functions

int _MDrv_FBIO_IOC_Get_SUPINFO(unsigned long arg)
{
    FB_GOP_SUPINF_CONFIG stCfg;

    // enable gwin is in GOP01

    stCfg.bKeyAlpha   = 1;
    stCfg.bConstAlpha = 1;
    stCfg.bPixelAlpha = 1;
    stCfg.bColFmt[COLORFMT_ARGB8888] = 1;
    stCfg.bColFmt[COLORFMT_RGB565]   = 1;
    stCfg.bColFmt[COLORFMT_YUV422]   = 1;
    stCfg.u32MaxWidth = 1920;
    stCfg.u32MaxWidth = 1080;

    if(copy_to_user((FB_GOP_SUPINF_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_SUPINF_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Get_GWIN(unsigned long arg)
{
    unsigned long GOP_Reg_Base;
    FB_GOP_GWIN_CONFIG stCfg;

    // enable gwin is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    stCfg.u18HStart = genGWinInfo.u18HStart;
    stCfg.u18HEnd   = genGWinInfo.u18HEnd;
    stCfg.u18VStart = genGWinInfo.u18VStart;
    stCfg.u18VEnd   = genGWinInfo.u18VEnd;

    if(copy_to_user((FB_GOP_GWIN_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_GWIN_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_GWIN(unsigned long arg)
{
    unsigned long GOP_Reg_Base;
    FB_GOP_GWIN_CONFIG stCfg;
    unsigned short MIU_BUS = 0x04;
    unsigned short u16FrameBuffer_Bytes_Per_Pixel = ANDROID_BYTES_PER_PIXEL;
    unsigned long u32TempVal = 0;
    unsigned long GOP_Reg_DB=0x0400;

    // gwin info is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    if(copy_from_user(&stCfg, (FB_GOP_GWIN_CONFIG __user *)arg, sizeof(FB_GOP_GWIN_CONFIG)))
    {
        return -EFAULT;
    }

    genGWinInfo.u18HStart = stCfg.u18HStart;
    genGWinInfo.u18HEnd   = stCfg.u18HEnd;
    genGWinInfo.u18VStart = stCfg.u18VStart;
    genGWinInfo.u18VEnd   = stCfg.u18VEnd;

    // GOP Display Setting, GWIN
    // 2.1 GWIN display area in panel : : H Start postion and end information
    u32TempVal=(genGWinInfo.u18HStart*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_04,u32TempVal); // H start
    u32TempVal=(genGWinInfo.u18HEnd*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_05,u32TempVal); // H end

    // 2.2 GWIN  display area in panel : V Start postion and end information
    OUTREG16(GOP_Reg_Base+REG_GOP_06,genGWinInfo.u18VStart); // V start line
    OUTREG16(GOP_Reg_Base+REG_GOP_08,genGWinInfo.u18VEnd); // V end line

    //write gop register
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

    return 0;
}


int _MDrv_FBIO_IOC_Get_EnGOP(unsigned long arg)
{
    unsigned long GOP_Reg_Base;
    unsigned char enGOP;
    // enable gwin is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    // get enable/disable gwin
    enGOP=(unsigned char)INREGMSK16(GOP_Reg_Base+REG_GOP_00, 0x0001);

    if(copy_to_user((unsigned char __user *)arg, &enGOP, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_EnGOP(unsigned long arg)
{
    unsigned long GOP_Reg_Base;
    unsigned char enGOP;
    unsigned long GOP_Reg_DB=0x0400;
    // enable gwin is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    if(copy_from_user(&enGOP, (unsigned char __user *)arg, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    // enable/disable gwin
    if(enGOP)
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00,0x0001,0x0001);
    else
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00,0x0000,0x0001);

    //write gop register
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

    return 0;
}


int _MDrv_FBIO_IOC_Get_Alpha(unsigned long arg)
{
    unsigned long GOP_Reg_Base;
    FB_GOP_ALPHA_CONFIG stCfg;
    // alpha info is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    // enable/disable
    stCfg.bEn=TRUE;

    // constant or pixel alpha
    if(INREGMSK16(GOP_Reg_Base+REG_GOP_00, 0x4000)>>14)
        stCfg.enAlphaType = PIXEL_ALPHA;
    else
        stCfg.enAlphaType = CONST_ALPHA;

    // alhap value
    stCfg.u8Alpha = (unsigned char)INREGMSK16(GOP_Reg_Base+REG_GOP_0A, 0x00FF);

    if(copy_to_user((FB_GOP_ALPHA_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_Constant_Alpha(unsigned long arg)
{
    FB_GOP_ALPHA_CONFIG stCfg;
    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    GOP_Set_Constant_Alpha(0, (int)stCfg.enAlphaType, stCfg.u8Alpha);

    return 0;
}

int _MDrv_FBIO_IOC_Get_Color_Key(unsigned long arg)
{
    unsigned long GOP_Reg_Base;
    FB_GOP_COLORKEY_CONFIG stCfg;
    // colorkey setting is in GOP00
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    if(copy_from_user(&stCfg, (FB_GOP_COLORKEY_CONFIG __user *)arg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    // enable/disable
    stCfg.bEn = (INREGMSK16(GOP_Reg_Base+REG_GOP_00,0x0800)>>11);

    // colorkey value
    stCfg.u8R = (unsigned char)INREGMSK16(GOP_Reg_Base+REG_GOP_25,0x00FF);
    stCfg.u8G = (unsigned char)(INREGMSK16(GOP_Reg_Base+REG_GOP_24,0xFF00)>>8);
    stCfg.u8B = (unsigned char)INREGMSK16(GOP_Reg_Base+REG_GOP_24,0x00FF);

    if(copy_to_user((FB_GOP_COLORKEY_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_Color_Key(unsigned long arg)
{
    FB_GOP_COLORKEY_CONFIG stCfg;
    if(copy_from_user(&stCfg, (FB_GOP_COLORKEY_CONFIG __user *)arg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    GOP_Set_Color_Key(0, stCfg.bEn, stCfg.u8R, stCfg.u8G, stCfg.u8B);

    return 0;
}

int _MDrv_FBIO_IOC_imageblit(struct fb_info *info,unsigned long arg)
{
    struct fb_image stCfg;
    //struct fb_image image;
    if(copy_from_user(&stCfg, (struct fb_image __user *)arg, sizeof(const struct fb_image)))
    {
        return -EFAULT;
    }

    if (NULL == stCfg.data)
    {
        FBDBG("ERROR:in mdrvfb2_ioctl, fb_image.data is NULL pointer\r\n");
        return -EFAULT;
    }

    FBDBG( "[FB Driver] Imgblit test, dst (x,y)=(%d,%d), width=%d, height=%d, colordepth=%d \n" ,stCfg.dx,stCfg.dy,stCfg.width,stCfg.height,stCfg.depth);

    sys_imageblit(info,&stCfg);

    return 0;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb2_ioctl
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
mdrvfb2_ioctl(struct fb_info *pinfo, unsigned int cmd, unsigned long arg)
{
    int err=0, ret=0;
    unsigned int ret_Phy=0;

    if (NULL == pinfo)
    {
        FBDBG("ERROR:in mdrvfb2_ioctl, pinfo is NULL pointer\r\n");
        return -ENOTTY;
    }

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != G3D_IOC_MAGIC_INFINITY)
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
        case IOCTL_FB_GETFBPHYADDRESS:
            ret_Phy = sg_G3D_fb2_bus_addr1;
            FBDBG( "[FB Driver] Phy Addr =%x\r\n",ret_Phy);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;

        case IOCTL_FB_GETVIDEOMEMSIZE:
            ret_Phy = sg_videomemorysize;
            FBDBG( "[FB Driver] Kernel VideoMemSize =0x%x \n" ,(unsigned int)sg_videomemorysize);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;

        case IOCTL_FB_GETFBSUPPORTINF:
            FBDBG( "[FB Driver] Get Framebuffer Support Information.\n");
            ret=_MDrv_FBIO_IOC_Get_SUPINFO(arg);
            break;

        case IOCTL_FB_GETGWININFO:
            FBDBG( "[FB Driver] Get Framebuffer GWIN Information.\n");
            ret=_MDrv_FBIO_IOC_Get_GWIN(arg);
            break;

        case IOCTL_FB_SETGWININFO:
            FBDBG( "[FB Driver] Set Framebuffer GWIN Information.\n");
            ret=_MDrv_FBIO_IOC_Set_GWIN(arg);
            break;

        case IOCTL_FB_GETENABLEGOP:
            FBDBG( "[FB Driver] Get GOP Enable/Disable.\n");
            ret=_MDrv_FBIO_IOC_Get_EnGOP(arg);
            break;

        case IOCTL_FB_SETENABLEGOP:
            FBDBG( "[FB Driver] Set GOP Enable/Disable.\n");
            ret=_MDrv_FBIO_IOC_Set_EnGOP(arg);
            break;

        case IOCTL_FB_GETALPHA:
            FBDBG( "[FB Driver] Get Alpha Information.\n");
            ret=_MDrv_FBIO_IOC_Get_Alpha(arg);
            break;

        case IOCTL_FB_SETALPHA:
            FBDBG( "[FB Driver] Set Alpha Information.\n");
            ret=_MDrv_FBIO_IOC_Set_Constant_Alpha(arg);
            break;

        case IOCTL_FB_GETCOLORKEY:
            FBDBG( "[FB Driver] Get Colorkey Information.\n");
            ret=_MDrv_FBIO_IOC_Get_Color_Key(arg);
            break;

        case IOCTL_FB_SETCOLORKEY:
            FBDBG( "[FB Driver] Set Colorkey Information.\n");
            ret=_MDrv_FBIO_IOC_Set_Color_Key(arg);
            break;

        case IOCTL_FB_IMAGEBLIT:
            FBDBG( "[FB Driver] Use Imageblit.\n");
            ret=_MDrv_FBIO_IOC_imageblit(pinfo,arg);
            break;

        default:  /* redundant, as cmd was checked against MAXNR */
            FBDBG("in default ioct\r\n");
            return -ENOTTY;
    }
    return ret;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb2_platform_release
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

static void mdrvfb2_platform_release(struct device *device)
{

    MstarFB_ENTER();

    if (NULL == device)
    {
        FBDBG("ERROR: in mdrvfb2_platform_release, \
                device is NULL pointer !\r\n");
    }
    else
    {
        FBDBG("in mdrvfb2_platform_release, module unload!\n");
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
static int mdrvfb2_suspend(struct platform_device *pdev,
                 pm_message_t state)
{
    FBDBG(" in mdrvfb2_suspend \r\n");
    return 0;
}

static int mdrvfb2_resume(struct platform_device *pdev)
{
     dma_addr_t fb2_PHY_ADD_SHOW=0;
     FBDBG("  mdrvfb2_resume \r\n");

     if(unlikely(useCalibrationFrameBuffer))
     {
         fb2_PHY_ADD_SHOW = sg_G3D_fb2_bus_addr1 + ((sg_mdrvfb2_default_var.xres * sg_mdrvfb2_default_var.yres * ANDROID_BYTES_PER_PIXEL)*ANDROID_NUMBER_OF_BUFFERS);
         GOP_Setmode(TRUE, 0,sg_mdrvfb2_default_var.xres,sg_mdrvfb2_default_var.yres,fb2_PHY_ADD_SHOW,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
     }
     else
     {
         GOP_Setmode(TRUE, 0,sg_mdrvfb2_default_var.xres,sg_mdrvfb2_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
     }

    //printk("fb2 resume sleep ! \n");
     //msleep(5000);
    return 0;
}
/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb2_probe
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

static int mdrvfb2_probe(struct platform_device *dev)
{
    struct fb_info *pinfo = NULL;
    int retval = -ENOMEM;

    int fb_format=0;

    MstarFB_ENTER();

    if (NULL == dev)
    {
        FBDBG("ERROR: in mdrvfb2_prob: dev is NULL pointer \r\n");
        return -ENOTTY;
    }

    fb_format=ANDROID_FB_FORMAT;

    // static global variable, pass at probe(), used by dma alloc and dma free in ioctl(),
    psg_mdrv_fbdev0 = dev;

    FBDBG("mdrvfb2: sg_videomemorysize is 0x%X to allocate screen memory\n",(unsigned int)sg_videomemorysize);

    /* For G3D frame buffer2 */
    if (!(sg_pG3D_fb2_vir_addr1 = alloc_dmem(KEY_DMEM_FB_BUF,
                                                    PAGE_ALIGN(sg_videomemorysize),
                                                    &sg_G3D_fb2_bus_addr1)))
    {
        FBDBG("mdrvfb2: unable to allocate screen memory\n");
        return retval;
    }

    //printk("---------1 \n");
    //msleep(6000);
    // prevent initial garbage on screen ; to clean memory
    memset(CAST_2_IOMEM_CHAR_P sg_pG3D_fb2_vir_addr1,OTHER_COLOR, sg_videomemorysize);

    FBDBG("mdrvfb2 vir=0x%x, phy=0x%x\n", (unsigned int)sg_pG3D_fb2_vir_addr1, sg_G3D_fb2_bus_addr1);

    sg_mdrvfb2_fix.smem_start = sg_G3D_fb2_bus_addr1;
    sg_mdrvfb2_fix.smem_len = sg_videomemorysize;

    pinfo = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
    if (!pinfo)
        goto err;


    pinfo->screen_base = CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1);
    FBDBG( "sg_mdrvfb2_fix.smem_start=0x%x ",(unsigned int) sg_mdrvfb2_fix.smem_start);

    //printk("---------2 \n");
    //msleep(6000);

    pinfo->fbops = &sg_mdrvfb2_ops;
    pinfo->var = sg_mdrvfb2_default_var;
    pinfo->fix = sg_mdrvfb2_fix;
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
    pinfo->fix.line_length      = sg_mdrvfb2_default_var.xres * ANDROID_BYTES_PER_PIXEL;
    pinfo->fix.accel            = FB_ACCEL_NONE;
    pinfo->fix.ypanstep         = 1;

    pinfo->var.xres             = sg_mdrvfb2_default_var.xres;
    pinfo->var.yres             = sg_mdrvfb2_default_var.yres;
    pinfo->var.xres_virtual     = sg_mdrvfb2_default_var.xres;
    pinfo->var.yres_virtual     = sg_mdrvfb2_default_var.yres * ANDROID_NUMBER_OF_BUFFERS;
    pinfo->var.bits_per_pixel   = BPX;
    pinfo->var.activate         = FB_ACTIVATE_NOW;
    pinfo->var.height           = sg_mdrvfb2_default_var.yres;
    pinfo->var.width            = sg_mdrvfb2_default_var.xres;
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
    // same define as cedric
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
	else
	{

		 printk(KERN_ERR"ERROR!! mdrvfb2_probe fb_format =%d Not Support!!",fb_format);
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
    		modelist->mode.refresh=sg_mdrvfb2_default_fbmode.refresh;
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
//   #if (UBOOT_SHOW_LOGO==0)


    GOP_Setmode(FALSE, 0,sg_mdrvfb2_default_var.xres,sg_mdrvfb2_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
    //set to sca2 for test Infinity in cedric
    #define mdrv_BASE_REG_GOP00_PA        GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x120200/2))
    #define mdrv_BASE_REG_DIP             GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x133400/2))
    #define mdrv_BASE_REG_CLKGEN          GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x100B00/2))
    OUTREG16(mdrv_BASE_REG_GOP00_PA + (0x7E<<2),0x924);
    OUTREG16(mdrv_BASE_REG_GOP00_PA + (0x7B<<2),0x04);
    OUTREG16(mdrv_BASE_REG_GOP00_PA + (0x01<<2),((INREG16(mdrv_BASE_REG_GOP00_PA + (0x01<<2))&0xFFF0)|0x07));
    OUTREG16(mdrv_BASE_REG_DIP+ (0x10<<2),((INREG16(mdrv_BASE_REG_DIP + (0x10<<2)))&0xFFEE)|0x11);
    OUTREG16(mdrv_BASE_REG_CLKGEN+ (0x40<<2),((INREG16(mdrv_BASE_REG_CLKGEN + (0x40<<2)))&0xFFF0)|0x04);
    GOP_Set_OutFormat(0,1);


   //#else
   //     GOP_Change_Alpha(FALSE, 0,sg_mdrvfb2_default_var.xres,sg_mdrvfb2_default_var.yres,sg_G3D_fb2_bus_addr2,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
//   #endif


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

    free_dmem(KEY_DMEM_FB_BUF,
                      PAGE_ALIGN(sg_videomemorysize),
                      sg_pG3D_fb2_vir_addr1,
                      sg_G3D_fb2_bus_addr1);

    return retval;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb2_remove
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
static int mdrvfb2_remove(struct platform_device *dev)
{
    struct fb_info *pinfo;
    pinfo = 0;


    MstarFB_ENTER();


    if (NULL == dev)
    {
        FBDBG("ERROR: mdrvfb2_remove: dev is NULL pointer \n");
        return -ENOTTY;
    }

    pinfo = platform_get_drvdata(dev);
    if (pinfo)
    {
        unregister_framebuffer(pinfo);

        free_dmem(KEY_DMEM_FB_BUF,
                          PAGE_ALIGN(sg_videomemorysize),
                          sg_pG3D_fb2_vir_addr1,
                          sg_G3D_fb2_bus_addr1);

        framebuffer_release(pinfo);
    }

    MstarFB_LEAVE();
    return 0;
}

static int mdrvfb2_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{

    //MstarFB_ENTER();
    dma_addr_t  fb2_PHY_ADD_SHOW=0;

    if( unlikely(useCalibrationFrameBuffer))
    {
        return 0;
    }
    else
    {

        if (info->var.yoffset >= (info->var.height << 1))
        {
            fb2_PHY_ADD_SHOW = info->fix.smem_start + ((info->var.width * info->var.height * ANDROID_BYTES_PER_PIXEL) << 1);
        }
        else if (info->var.yoffset >= info->var.height)
        {
            fb2_PHY_ADD_SHOW = info->fix.smem_start + (info->var.width * info->var.height * ANDROID_BYTES_PER_PIXEL);
        }
        else
        {
            fb2_PHY_ADD_SHOW = info->fix.smem_start;
        }

        GOP_Pan_Display(0, fb2_PHY_ADD_SHOW);
    }

    return 0;
}


/* check var to see if supported by this device.  Probably doesn't
 * need modifying.
 */
static int mdrvfb2_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
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
static int __init mdrvfb2_init(void)
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
    mdrvfb2_updateinfo();

    ret = platform_driver_register(&sg_mdrvfb2_driver);

    if (!ret)
    {   /* register driver sucess */
        /* register device */
        ret = platform_device_register(&sg_mdrvfb2_device);
        if (ret)    /* if register device fail, then unregister the driver.*/
        {
            platform_driver_unregister(&sg_mdrvfb2_driver);
            printk(KERN_INFO"mdrvfb2_device register failed...\n");
        }
        else
        {
            printk(KERN_INFO"mdrvfb2_device register success...\n");
        }
    }


    MstarFB_LEAVE();

    return ret;
}


module_init(mdrvfb2_init);   /* tell kernel where the init function */

#ifdef MODULE

/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvfb2_exit
+------------------------------------------------------------------------------
| DESCRIPTION : cleanup the driver from kernel
|
| RETURN      : no return value
|
| Variable Name : no variable
+------------------------------------------------------------------------------
*/

static void __exit mdrvfb2_exit(void)
{

       MstarFB_ENTER();

       platform_device_unregister(&sg_mdrvfb2_device);

       platform_driver_unregister(&sg_mdrvfb2_driver);

       MstarFB_LEAVE();

}

module_exit(mdrvfb2_exit); /* tel kernel where is the clean up function */

MODULE_LICENSE("GPL");
/* use GPL license to avoid Kernel to complain this driver */
#endif      /* MODULE */



//MODULE_LICENSE("GPL")
