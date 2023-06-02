

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

// update global variables while init/probe
static void __init mdrvinfinityfb_updateinfo(void)
{
    static int main_videomemory_size=0;

    //default settings for Stretch mode
    genGOPGernelSettings.u32Width=sg_mdrvinfinityfb_default_var.xres;
    genGOPGernelSettings.u32Height=sg_mdrvinfinityfb_default_var.yres;
    genGOPGernelSettings.enBufferNum=FB_DOUBLE_BUFFER;
    genGOPGernelSettings.enStretchH_Ratio=FB_STRETCH_H_RATIO_1;
    genGOPGernelSettings.u32DisplayWidth=genGOPGernelSettings.u32Width*genGOPGernelSettings.enStretchH_Ratio;
    genGOPGernelSettings.u32DisplayHeight=genGOPGernelSettings.u32Height;

    //default settings for global structure of fb_var_screeninfo
    sg_mdrvinfinityfb_default_var.xres         = 0;
    sg_mdrvinfinityfb_default_var.yres         = 0;
    sg_mdrvinfinityfb_default_var.xres_virtual = sg_mdrvinfinityfb_default_var.xres;
    sg_mdrvinfinityfb_default_var.yres_virtual = sg_mdrvinfinityfb_default_var.yres * (genGOPGernelSettings.enBufferNum);//double buffer for pan display
    sg_mdrvinfinityfb_default_var.width        = sg_mdrvinfinityfb_default_var.xres;
    sg_mdrvinfinityfb_default_var.height       = sg_mdrvinfinityfb_default_var.yres;

    //GWin init, same size as screen
    genGWinInfo.u18HStart = 0;
    genGWinInfo.u18HEnd   = genGWinInfo.u18HStart + sg_mdrvinfinityfb_default_var.xres;
    genGWinInfo.u18VStart = 0;
    genGWinInfo.u18VEnd   = genGWinInfo.u18VStart + sg_mdrvinfinityfb_default_var.yres;
    main_videomemory_size = (sg_mdrvinfinityfb_default_var.xres * sg_mdrvinfinityfb_default_var.yres * ANDROID_BYTES_PER_PIXEL * (genGOPGernelSettings.enBufferNum));


    sg_videomemorysize = main_videomemory_size;

    //default settings for global structure of fb_fix_screeninfo
    sg_mdrvinfinityfb_fix.smem_start  = sg_G3D_fb2_bus_addr1;
    sg_mdrvinfinityfb_fix.smem_len    = sg_videomemorysize;
    sg_mdrvinfinityfb_fix.line_length = sg_mdrvinfinityfb_default_var.xres * ANDROID_BYTES_PER_PIXEL;
    sg_mdrvinfinityfb_fix.ypanstep    = 1;

    FBDBGMORE("[FB Driver] updateinfo: GOP_WIDTH = [%d], GOP_HEIGHT = [%d], GOPmemorysize = 0x%x \r\n",sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,(unsigned int)sg_videomemorysize);

}

// update global variables while setting new resolution by ioctl FBIOPUT_VSCREENINFO
static int mdrvinfinityfb_infinity_updateinfo_par(unsigned int setWidth,unsigned int setHeight)
{

    static int main_videomemory_size=0;

    //update settings for global structure of fb_var_screeninfo
    sg_mdrvinfinityfb_default_var.xres         = setWidth;
    sg_mdrvinfinityfb_default_var.yres         = setHeight;
    sg_mdrvinfinityfb_default_var.xres_virtual = sg_mdrvinfinityfb_default_var.xres;
    sg_mdrvinfinityfb_default_var.yres_virtual = sg_mdrvinfinityfb_default_var.yres * (genGOPGernelSettings.enBufferNum);//double buffer for pan display
    sg_mdrvinfinityfb_default_var.width        = sg_mdrvinfinityfb_default_var.xres;
    sg_mdrvinfinityfb_default_var.height       = sg_mdrvinfinityfb_default_var.yres;

    //update settings for global structure of Stetch mode
    genGOPGernelSettings.u32Width=sg_mdrvinfinityfb_default_var.xres;
    genGOPGernelSettings.u32Height=sg_mdrvinfinityfb_default_var.yres;
    genGOPGernelSettings.u32DisplayWidth=genGOPGernelSettings.u32Width*genGOPGernelSettings.enStretchH_Ratio;
    genGOPGernelSettings.u32DisplayHeight=genGOPGernelSettings.u32Height;

    //GWin init, same size as screen
    genGWinInfo.u18HStart = 0;
    genGWinInfo.u18HEnd   = genGWinInfo.u18HStart + sg_mdrvinfinityfb_default_var.xres;
    genGWinInfo.u18VStart = 0;
    genGWinInfo.u18VEnd   = genGWinInfo.u18VStart + sg_mdrvinfinityfb_default_var.yres;
    main_videomemory_size = (sg_mdrvinfinityfb_default_var.xres * sg_mdrvinfinityfb_default_var.yres * ANDROID_BYTES_PER_PIXEL * (genGOPGernelSettings.enBufferNum));

    // free memory if it has
    if(sg_videomemorysize > 0)
    {
        free_dmem(KEY_DMEM_FB_BUF,
                  PAGE_ALIGN(sg_videomemorysize),
                  sg_pG3D_fb2_vir_addr1,
                  sg_G3D_fb2_bus_addr1);
    }

    sg_videomemorysize = main_videomemory_size;

    FBDBG ("[FB Driver] updateinfo_par: GOP_WIDTH = [%d], GOP_HEIGHT = [%d], GOPmemorysize = 0x%x \r\n",sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,(unsigned int)sg_videomemorysize);

    // allocate new memory
    if (!(sg_pG3D_fb2_vir_addr1 = alloc_dmem(KEY_DMEM_FB_BUF,
                                             PAGE_ALIGN(sg_videomemorysize),
                                             &sg_G3D_fb2_bus_addr1)))
    {
        FBDBGERR("[FB Driver] Error: updateinfo_par: unable to allocate screen memory\n");
        return -ENOMEM;
    }

    // prevent initial garbage on screen ; to clean memory
    memset(CAST_2_IOMEM_CHAR_P sg_pG3D_fb2_vir_addr1,0, sg_videomemorysize);

    //update settings for global structure of fb_fix_screeninfo
    sg_mdrvinfinityfb_fix.smem_start  = sg_G3D_fb2_bus_addr1;
    sg_mdrvinfinityfb_fix.smem_len    = sg_videomemorysize;
    sg_mdrvinfinityfb_fix.line_length = sg_mdrvinfinityfb_default_var.xres * ANDROID_BYTES_PER_PIXEL;

    FBDBG("[FB Driver] vir=0x%x, phy=0x%x\n", (unsigned int)sg_pG3D_fb2_vir_addr1, sg_G3D_fb2_bus_addr1);


    return 0;
}

static int mdrvinfinityfb_infinity_SetPalette(FB_GOP_PaletteEntry *pPalEntry, int intPalStart,int intPalEnd)
{
    int i   = 0;
    int ret = -1;

    //open scl0 fclk to load palette
    if (IS_ERR(gop_clks[1]))
    {
        FBDBGERR( "[FB Driver] Error: SetPalette Fail to get gop fclk of scl!\n" );
        return ret;
    }
    else
    {
        clk_prepare_enable(gop_clks[1]);
    }

    FBDBGMORE("Palette value:(B,G,R,A)\n");

    //write palette
    for(i=intPalStart;i<=intPalEnd;i++)
    {
        GOP_Set_Palette_RIU(0, i, pPalEntry[i].BGRA.u8A, pPalEntry[i].BGRA.u8R, pPalEntry[i].BGRA.u8G, pPalEntry[i].BGRA.u8B);

        //record in global variable
        stGOPCurrentPalInfo[i].BGRA.u8B=pPalEntry[i].BGRA.u8B;
        stGOPCurrentPalInfo[i].BGRA.u8G=pPalEntry[i].BGRA.u8G;
        stGOPCurrentPalInfo[i].BGRA.u8R=pPalEntry[i].BGRA.u8R;
        stGOPCurrentPalInfo[i].BGRA.u8A=pPalEntry[i].BGRA.u8A;
        FBDBGMORE("(%d,%d,%d,%d)\n",stGOPCurrentPalInfo[i].BGRA.u8B,stGOPCurrentPalInfo[i].BGRA.u8G,stGOPCurrentPalInfo[i].BGRA.u8R,stGOPCurrentPalInfo[i].BGRA.u8A);
    }

    //close clk
    clk_disable_unprepare(gop_clks[1]);

    return 0;
}

static int mdrvinfinityfb_set_par(struct fb_info *info)
{
    int ret = 0;
    FBDBG( "[FB Driver] Test I3!\n" );

    //Stretch mode settings, if use back to this to set resolution, it will set to default
    genGOPGernelSettings.enStretchH_Ratio=FB_STRETCH_H_RATIO_1;
    genGOPGernelSettings.enBufferNum=FB_DOUBLE_BUFFER;

    ret=mdrvinfinityfb_infinity_updateinfo_par(info->var.xres,info->var.yres);

    info->fix.smem_start  = sg_mdrvinfinityfb_fix.smem_start;
    info->fix.smem_len    = sg_mdrvinfinityfb_fix.smem_len;

    info->var.width       = info->var.xres;
    info->var.height      = info->var.yres;

    info->screen_base     = (char *)CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1);
    info->fix.line_length = info->var.xres * ANDROID_BYTES_PER_PIXEL;

    //GOP_Setmode(FALSE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
    GOP_Set_Outputmode(0, sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL);

    return ret;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb_mmap
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
static int mdrvinfinityfb_mmap(struct fb_info *pinfo, struct vm_area_struct *vma)
{
    size_t size;
    size = 0;

    if (sg_videomemorysize<1)
    {
        FBDBGERR("[FB Driver] ERROR: mmap, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if (NULL == pinfo)
    {
        FBDBGERR("[FB Driver] ERROR: mmap, pinfo is NULL pointer !\n");
        return -ENOTTY;
    }
    if (NULL == vma)
    {
        FBDBGERR("[FB Driver] ERROR: mmap, vma is NULL pointer !\n");
        return -ENOTTY;
    }

    size = vma->vm_end - vma->vm_start;

    vma->vm_pgoff += ((sg_G3D_fb2_bus_addr1 ) >> PAGE_SHIFT);

    FBDBG(
    "[FB Driver] mmap vma->vm_start=%x\n vma->vm_end=%x\n vma->vm_pgoff =%x\n",
    (unsigned int) vma->vm_start, (unsigned int)vma->vm_end ,
    (unsigned int)vma->vm_pgoff);

    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

//sw update inverse color's inverse table thread when rotate case happens
static int mdrvinfinityfb_SWupdate_InvTable(void *arg)
{
    int ret=0;
    while(1)
    {
        if (kthread_should_stop()) break;

        ret=GOP0_invColor_CPU_Upate_InvTable(genAE_info.u32AEBlkWidth,
                                             genAE_info.u32AEBlkHeight,
                                             genAE_info.u32AEDisplayWidth,
                                             genAE_info.u32AEDisplayHeight);
        if(!ret)//if update wrong, break
        {
            FBDBGERR("[FB Driver] ERROR: mdrvinfinityfb_SWupdate_InvTable wrong!\n");
            genTreadisError=1;
            break;
        }


    }

   return 0;

}

static int mdrvinfinityfb_SWupdate_InvTable_debugmode(void *arg)
{

    while(1)
    {
        if (kthread_should_stop()) break;

        GOP0_invColor_DebugMode_UpdateInvTable();
        msleep(1000);
    }

   return 0;

}


//check whether need doing sw update inverse color's inverse table or not
static void mdrvinfinityfb_checkInvTable_UpdateMode(void)
{
    int idx=0;
    int ret;

    if((genInv_Color==1) && (genSC_info.bRotateEn==1))
    {
        idx=1;
    }
    else
    {
        idx=0;
    }

    if(genTreadisError==1)
    {
        pSWupdateInvThread=NULL;
        genTreadisError=0;
    }

    if(idx==1)//enable thread
    {
        if(pSWupdateInvThread==NULL)
        {
            pSWupdateInvThread = kthread_create(mdrvinfinityfb_SWupdate_InvTable,(void *)&pSWupdateInvThread,"GOP0_SWupdate_InvColor");
            if (IS_ERR(pSWupdateInvThread))
            {
                ret = PTR_ERR(pSWupdateInvThread);
                pSWupdateInvThread = NULL;
                FBDBGERR("[FB Driver] ERROR: mdrvinfinityfb_checkInvTable_UpdateMode wrong! Create thread fail! ret=%d\n",ret);
            }
            else
            {
                wake_up_process(pSWupdateInvThread);
            }

        }
    }
    else if (idx==0 && (pSWupdateInvThread!=NULL))//disable thread
    {
        kthread_stop(pSWupdateInvThread);
        pSWupdateInvThread = NULL;
    }

}

//sw patch for inverse color hw error
static int mdrvinfinityfb_InverseColor_RefineBlkHeightMode(int u32ScalerWidth, int u32BlkHeight)
{
    if((u32ScalerWidth<=1040)&&(u32ScalerWidth>=976))
    {
        u32BlkHeight=u32BlkHeight*2;
    }
    else if((u32ScalerWidth<=960)&&(u32ScalerWidth>=912))
    {
        u32BlkHeight=u32BlkHeight*3;
    }
    else if((u32ScalerWidth<=896)&&(u32ScalerWidth>=480))
    {
        u32BlkHeight=u32BlkHeight*2;
    }
    else if((u32ScalerWidth<=464)&&(u32ScalerWidth>=400))
    {
        u32BlkHeight=u32BlkHeight*3;
    }
    else if((u32ScalerWidth<=384)&&(u32ScalerWidth>=272))
    {
        u32BlkHeight=u32BlkHeight*4;
    }
    else if(u32ScalerWidth==256)
    {
        u32BlkHeight=u32BlkHeight*8;
    }

    return u32BlkHeight;
}


// FB Ioctl Functions

int _MDrv_FBIO_IOC_Get_SUPINFO(unsigned long arg)
{
    FB_GOP_SUPINF_CONFIG stCfg;

    // enable gwin is in GOP01

    stCfg.bKeyAlpha   = 1;
    stCfg.bConstAlpha = 1;
    stCfg.bPixelAlpha = 1;
    stCfg.bColFmt[COLORFMT_ARGB8888] = 0;
    stCfg.bColFmt[COLORFMT_RGB565]   = 0;
    stCfg.bColFmt[COLORFMT_YUV422]   = 0;
    stCfg.bColFmt[COLORFMT_I8PALETTE]= 1;
    stCfg.u32MaxWidth = 1920;
    stCfg.u32MaxHeight= 1080;

    if(copy_to_user((FB_GOP_SUPINF_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_SUPINF_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Get_GWIN(unsigned long arg)
{
    FB_GOP_GWIN_CONFIG stCfg;

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
    unsigned long GOP_Reg_Base = 0;
    FB_GOP_GWIN_CONFIG stCfg;
    unsigned short MIU_BUS     = 0x04;
    unsigned short u16FrameBuffer_Bytes_Per_Pixel = ANDROID_BYTES_PER_PIXEL;
    unsigned long u32TempVal   = 0;
    unsigned long GOP_Reg_DB   = GOP_BANK_DOUBLE_WR_G0;
    int Panel_Width            = 0;
    int Panel_Height           = 0;

    if (sg_videomemorysize<1)
    {
        FBDBGERR("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_GWIN, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&stCfg, (FB_GOP_GWIN_CONFIG __user *)arg, sizeof(FB_GOP_GWIN_CONFIG)))
    {
        return -EFAULT;
    }

    Panel_Width  = (int)(stCfg.u18HEnd-stCfg.u18HStart+1);
    Panel_Height = (int)(stCfg.u18VEnd-stCfg.u18VStart+1);

    /// GOP0_0 settings
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    // GOP global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,(Panel_Width >>1)+1);//miu efficiency = Stretch Window H size (unit:2 pixel) /2 +1

    // GOP display area global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_30,Panel_Width >>1); //Stretch Window H size (unit:2 pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_31,Panel_Height);    //Stretch window V size

    // gwin info is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP01_PA==mdrv_BASE_REG_GOP00_PA)
    {
        SwitchSubBank(1);
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

    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP01_PA==mdrv_BASE_REG_GOP00_PA)
    {
        SwitchSubBank(0);
    }

    //write gop register
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

    return 0;
}


int _MDrv_FBIO_IOC_Get_EnGOP(unsigned long arg)
{
    unsigned char enGOP;

    GOP_Get_Enable_GWIN(0, &enGOP);

    //remove check scaler's switch later in I3
    if(enGOP && INREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_60,0x0001))
        enGOP=1;
    else
        enGOP=0;

    if(copy_to_user((unsigned char __user *)arg, &enGOP, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_EnGOP(unsigned long arg)
{
    unsigned char enGOP;

    if (sg_videomemorysize<1)
    {
        FBDBGERR("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_EnGOP, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&enGOP, (unsigned char __user *)arg, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] enable GOP=%d \n",enGOP);

    // record in global variable
    genGWIN=enGOP;

    // this switch will let scaler handle, remove later in I3
    // enable/disable gop switch at scaler
    if(enGOP)
        OUTREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_60,0x0001,0x0001);
    else
        OUTREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_60,0x0000,0x0001);

    GOP_Set_Enable_GWIN(0,enGOP);

    return 0;
}


int _MDrv_FBIO_IOC_Get_Alpha(unsigned long arg)
{
    FB_GOP_ALPHA_CONFIG stCfg;
    int alpha_type  = 0;
    int alpha_value = 0;

    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    // get alpha blending settings from register
    GOP_Get_Alpha(0,&alpha_type,&alpha_value);

    // copy result from register
    stCfg.bEn         = genGOPALPHA.bEn;
    stCfg.enAlphaType = (FB_GOP_ALPHA_TYPE)alpha_type;
    stCfg.u8Alpha     = (unsigned char)alpha_value;

    // record settings in global variables
    genGOPALPHA.enAlphaType=stCfg.enAlphaType;
    genGOPALPHA.u8Alpha=stCfg.u8Alpha;

    if(copy_to_user((FB_GOP_ALPHA_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_Constant_Alpha(unsigned long arg)
{
    FB_GOP_ALPHA_CONFIG stCfg;

    if (sg_videomemorysize<1)
    {
        FBDBGERR("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_Constant_Alpha, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }
    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] Set alpha enable=%d, type=%d, constant alpha value=0x%x \n",(int)stCfg.bEn, (int)stCfg.enAlphaType, stCfg.u8Alpha);

    if(stCfg.bEn)
    {
        GOP_Set_Constant_Alpha(0, (int)stCfg.enAlphaType, stCfg.u8Alpha);
    }
    else
    {
        GOP_Set_Constant_Alpha(0, CONST_ALPHA, 0);//close alpha blending, let gop always shows
    }

    genGOPALPHA.bEn=stCfg.bEn;
    genGOPALPHA.enAlphaType=stCfg.enAlphaType;
    genGOPALPHA.u8Alpha=stCfg.u8Alpha;

    return 0;
}

int _MDrv_FBIO_IOC_Get_Color_Key(unsigned long arg)
{
    FB_GOP_COLORKEY_CONFIG stCfg;
    int Enable = 0;
    int Red    = 0;
    int Green  = 0;
    int Blue   = 0;

    if(copy_from_user(&stCfg, (FB_GOP_COLORKEY_CONFIG __user *)arg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    GOP_Get_Color_Key(0,&Enable,&Red,&Green,&Blue);

    // enable/disable
    stCfg.bEn = (unsigned char)Enable;

    // colorkey value
    stCfg.u8R = (unsigned char)Red;
    stCfg.u8G = (unsigned char)Green;
    stCfg.u8B = (unsigned char)Blue;

    genGOPCOLORKEY.bEn=stCfg.bEn;
    genGOPCOLORKEY.u8R=stCfg.u8R;
    genGOPCOLORKEY.u8G=stCfg.u8G;
    genGOPCOLORKEY.u8B=stCfg.u8B;

    if(copy_to_user((FB_GOP_COLORKEY_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_Color_Key(unsigned long arg)
{
    FB_GOP_COLORKEY_CONFIG stCfg;

    if (sg_videomemorysize<1)
    {
        FBDBGERR("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_Color_Key, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }
    if(copy_from_user(&stCfg, (FB_GOP_COLORKEY_CONFIG __user *)arg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] Set ColorKey enable=%d, R=0x%x, G=0x%x, B=0x%x\n", (int)stCfg.bEn, stCfg.u8R, stCfg.u8G, stCfg.u8B);

    GOP_Set_Color_Key(0, stCfg.bEn, stCfg.u8R, stCfg.u8G, stCfg.u8B);
    genGOPCOLORKEY.bEn=stCfg.bEn;
    genGOPCOLORKEY.u8R=stCfg.u8R;
    genGOPCOLORKEY.u8G=stCfg.u8G;
    genGOPCOLORKEY.u8B=stCfg.u8B;

    return 0;
}

int _MDrv_FBIO_IOC_imageblit(struct fb_info *info,unsigned long arg)
{
    int ret = 0;

// this ioctl is unused, the same fuction can be accessed by fb_ops -> fb_imageblit
#if 0
    struct fb_image stCfg;
    int i = 0;


    if (sg_videomemorysize<1)
    {
        FBDBGERR("[FB Driver] ERROR: _MDrv_FBIO_IOC_imageblit, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&stCfg, (struct fb_image __user *)arg, sizeof(const struct fb_image)))
    {
        return -EFAULT;
    }

    if (NULL == stCfg.data)
    {
        FBDBGERR("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, fb_image.data is NULL pointer\r\n");
        return -EFAULT;
    }


    if (stCfg.fg_color>255)
    {
        FBDBGERR("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }

    if (stCfg.bg_color>255)
    {
        FBDBGERR("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }

    if (stCfg.width>1920)
    {
        FBDBGERR("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }
    if (stCfg.height>1920)
    {
        FBDBGERR("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }

    if (info->fix.visual == FB_VISUAL_TRUECOLOR || info->fix.visual == FB_VISUAL_DIRECTCOLOR )
    {
        for(i=0;i<(stCfg.width*stCfg.height);i++)
        {
            if(stCfg.data[i]>255)
            {
                ret = 1;
            }
        }
        if(ret)
        {
            FBDBGERR("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
            return -EFAULT;
        }
    }


    FBDBGMORE( "[FB Driver] Imgblit test, dst (x,y)=(%d,%d), width=%d, height=%d, colordepth=%d \n" ,stCfg.dx,stCfg.dy,stCfg.width,stCfg.height,stCfg.depth);

    sys_imageblit(info,&stCfg);
#endif

    return ret;
}

int _MDrv_FBIO_IOC_Set_Palette(unsigned long arg)
{
    FB_GOP_PaletteEntry stCfg[256];

    if(copy_from_user(&stCfg, (FB_GOP_PaletteEntry __user *)arg, sizeof(FB_GOP_PaletteEntry)*256))
    {
        return -EFAULT;
    }

    mdrvinfinityfb_infinity_SetPalette(stCfg,0,255);

    return 0;
}

int _MDrv_FBIO_IOC_Get_General_Config(unsigned long arg)
{
    FB_GOP_RESOLUTION_STRETCH_H_CONFIG stCfg;

    if(copy_from_user(&stCfg, (FB_GOP_RESOLUTION_STRETCH_H_CONFIG __user *)arg, sizeof(FB_GOP_RESOLUTION_STRETCH_H_CONFIG)))
    {
        return -EFAULT;
    }

    stCfg.u32Height   = genGOPGernelSettings.u32Height;
    stCfg.u32Width    = genGOPGernelSettings.u32Width;
    stCfg.enBufferNum = genGOPGernelSettings.enBufferNum;
    stCfg.u32DisplayHeight = genGOPGernelSettings.u32DisplayHeight;
    stCfg.u32DisplayWidth  = genGOPGernelSettings.u32DisplayWidth;
    stCfg.enStretchH_Ratio = genGOPGernelSettings.enStretchH_Ratio;

    if(copy_to_user((FB_GOP_RESOLUTION_STRETCH_H_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_RESOLUTION_STRETCH_H_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_General_Config(struct fb_info *info,unsigned long arg)
{
    FB_GOP_RESOLUTION_STRETCH_H_CONFIG stCfg;
    int MiuBusLen=128;//miu bus length in iNfinity
    int DisplayPerPixel=0;
    int max_x_res=2560;
    int max_y_res=2048;
    int ret = 0;

    if(copy_from_user(&stCfg, (FB_GOP_RESOLUTION_STRETCH_H_CONFIG __user *)arg, sizeof(FB_GOP_RESOLUTION_STRETCH_H_CONFIG)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] Set General_Config GOP_Width=%d, GOP_Height=%d, Display_Width=%d, Display_Height=%d, Buffer Num=%d, Stretch Ratio=%d\n",(int)stCfg.u32Width,(int)stCfg.u32Height,(int)stCfg.u32DisplayWidth,(int)stCfg.u32DisplayHeight,(int)stCfg.enBufferNum,(int)stCfg.enStretchH_Ratio);

    ///check variable valid/invalid
    DisplayPerPixel=MiuBusLen/(ANDROID_BYTES_PER_PIXEL*8);

    //single/double buffer check
    if((stCfg.enBufferNum!=FB_SINGLE_BUFFER)&&(stCfg.enBufferNum!=FB_DOUBLE_BUFFER))
    {
        FBDBG( "[FB Driver] Error: Set_General_Config: buffer num should be 1 or 2 !\n ");
        return -EINVAL;
    }
    //stretch ratio check
    if((stCfg.enStretchH_Ratio!=FB_STRETCH_H_RATIO_1)&&(stCfg.enStretchH_Ratio!=FB_STRETCH_H_RATIO_2)&&(stCfg.enStretchH_Ratio!=FB_STRETCH_H_RATIO_4)&&(stCfg.enStretchH_Ratio!=FB_STRETCH_H_RATIO_8))
    {
        FBDBG( "[FB Driver] Error: Set_General_Config: stretch H ratio should be 1, 2, 4 or 8 !\n ");
        return -EINVAL;
    }
    //x & y should > 0
    if((stCfg.u32Height<1)||(stCfg.u32Width<1)||(stCfg.u32DisplayHeight<1)||(stCfg.u32DisplayWidth<1))
    {
        FBDBG( "[FB Driver] Error: Set_General_Config: buffer x or y size should be > 0 !\n ");
        return -EINVAL;
    }
    //x & y should < 1920
    if((stCfg.u32Height>1920)||(stCfg.u32Width>1920)||(stCfg.u32DisplayHeight>1920)||(stCfg.u32DisplayWidth>1920))
    {
        FBDBG( "[FB Driver] Error: Set_General_Config: buffer x or y size should be < 1920 !\n ");
        return -EINVAL;
    }
    //max size for display is 1920*1088
    if(((stCfg.u32Height * stCfg.u32Width)>(max_x_res*max_y_res))||((stCfg.u32DisplayHeight* stCfg.u32DisplayWidth)>(max_x_res*max_y_res)))
    {
        FBDBG( "[FB Driver] Error: Set_General_Config: total buffer size should be lower than 1920*1088!\n ");
        return -EINVAL;
    }
    //miu alignment
    if(((stCfg.u32Width % DisplayPerPixel)!=0)||((stCfg.u32DisplayWidth% DisplayPerPixel)!=0))
    {
        FBDBG( "[FB Driver] Error: Set_General_Config: Display Per Pixel for Buffer Width should be %d!\n ",DisplayPerPixel);
        return -EINVAL;
    }
    //Stretch H parameter check
    if((stCfg.u32Width*stCfg.enStretchH_Ratio)!=stCfg.u32DisplayWidth)
    {
        FBDBG( "[FB Driver] Error: Set_General_Config: DisplayWidth should = Width * StretchH_Ratio!\n ");
        return -EINVAL;
    }
    //Stretch V parameter check
    if(stCfg.u32Height != stCfg.u32DisplayHeight)
    {
        FBDBG( "[FB Driver] Error: Set_General_Config: DisplayHeight should = Height!\n");
        return -EINVAL;
    }

    //after checking valid parameter, save to global variable
    genGOPGernelSettings.u32Width         = stCfg.u32Width;
    genGOPGernelSettings.u32Height        = stCfg.u32Height;
    genGOPGernelSettings.u32DisplayWidth  = stCfg.u32DisplayWidth;
    genGOPGernelSettings.u32DisplayHeight = stCfg.u32DisplayHeight;
    genGOPGernelSettings.enBufferNum      = stCfg.enBufferNum;
    genGOPGernelSettings.enStretchH_Ratio = stCfg.enStretchH_Ratio;

    info->var.xres=stCfg.u32Width;
    info->var.yres=stCfg.u32Height;

    ret=mdrvinfinityfb_infinity_updateinfo_par(info->var.xres,info->var.yres);

    info->fix.smem_start=sg_mdrvinfinityfb_fix.smem_start;
    info->fix.smem_len=sg_mdrvinfinityfb_fix.smem_len;

    info->var.width=info->var.xres;
    info->var.height=info->var.yres;

    info->screen_base = (char *)CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1);
    info->fix.line_length      = info->var.xres * ANDROID_BYTES_PER_PIXEL;

    GOP_Setmode_Stretch_H(FALSE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0,(int)genGOPGernelSettings.enStretchH_Ratio);

    return ret;
}


int _MDrv_FBIO_IOC_Set_EnInvColor(unsigned long arg)
{
    unsigned char enInvColor;
    int ret=0;

    if (sg_videomemorysize<1)
    {
        FBDBGERR("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_EnInvColor, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&enInvColor, (unsigned char __user *)arg, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] enable inverse color=%d \n",(int)enInvColor);

    // record in global variable
    genInv_Color=enInvColor;

    GOP0_invColor_Enable(enInvColor);

    //check whether do sw update inverse color
    mdrvinfinityfb_checkInvTable_UpdateMode();

    if(enInvColor==1)
    {
        GOP0_invColor_Debug_Mode();

        if(pSWupdateInvDebugThread==NULL)
        {
            pSWupdateInvDebugThread = kthread_create(mdrvinfinityfb_SWupdate_InvTable_debugmode,(void *)&pSWupdateInvDebugThread,"GOP0_debug_SWupdate_InvColor");
            if (IS_ERR(pSWupdateInvDebugThread))
            {
                ret=PTR_ERR(pSWupdateInvDebugThread);
                pSWupdateInvDebugThread = NULL;
                FBDBGERR("[FB Driver] ERROR: debug mode! Create thread fail!%d\n",ret);
            }
            else
            {
                wake_up_process(pSWupdateInvDebugThread);
            }
        }
    }
    else if((enInvColor==0)&&(pSWupdateInvDebugThread!=NULL))
    {
        kthread_stop(pSWupdateInvDebugThread);
        pSWupdateInvDebugThread = NULL;
    }

    return 0;
}

int _MDrv_FBIO_IOC_Set_AEinfoConfig(unsigned long arg)
{
    FB_GOP_INVCOLOR_AE_CONFIG stAECfg;
    unsigned long tempcorpAEwidth=0;
    unsigned long tempcorpAEheight=0;
    int tempAEblkwidth=0;
    int tempAEblkheight=0;
    int tempAEblkxnum=0;

    if(copy_from_user(&stAECfg, (FB_GOP_INVCOLOR_AE_CONFIG __user *)arg, sizeof(FB_GOP_INVCOLOR_AE_CONFIG)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] set AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d\n",
    (int)stAECfg.u32AEDisplayWidth,(int)stAECfg.u32AEDisplayHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight);

    // record in global variable
    genAE_info.u32AEDisplayWidth  = stAECfg.u32AEDisplayWidth;
    genAE_info.u32AEDisplayHeight = stAECfg.u32AEDisplayHeight;
    genAE_info.u32AEBlkWidth      = stAECfg.u32AEBlkWidth;
    genAE_info.u32AEBlkHeight     = stAECfg.u32AEBlkHeight;

    // when scaling happens, change AE block size with this scale
    if(genSC_info.bScalingEn)
    {
        if((genAE_info.u32AEBlkWidth!=0)&&(genAE_info.u32AEBlkHeight!=0)&&(genAE_info.u32AEDisplayWidth!=0)&&(genAE_info.u32AEDisplayHeight!=0))
        {
            if(genSC_info.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info.u32CropYend-genSC_info.u32CropYstart;
                    tempcorpAEheight=genSC_info.u32CropXend-genSC_info.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info.u32CropXend-genSC_info.u32CropXstart;
                    tempcorpAEheight=genSC_info.u32CropYend-genSC_info.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerHeight/(int)genAE_info.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerWidth/(int)genAE_info.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerWidth/(int)genAE_info.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerHeight/(int)genAE_info.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FBDBGERR( "[FB Driver] Error: _MDrv_FBIO_IOC_Set_AEinfoConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP0_invColor_Set_AE_Config((int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEDisplayHeight);
        }
        else
        {
            GOP0_invColor_Set_AE_Config((int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEDisplayWidth);
        }
    }

    if(genSC_info.bCropEn)//if crop enable, reset crop information when AE info change
    {
        if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP0_invColor_Set_Crop_Config((int)genSC_info.u32CropXstart,(int)genSC_info.u32CropYstart,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEDisplayHeight);
        }
        else
        {
            GOP0_invColor_Set_Crop_Config((int)genSC_info.u32CropXstart,(int)genSC_info.u32CropYstart,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEDisplayWidth);
        }
    }

    return 0;
}

int _MDrv_FBIO_IOC_Set_YThres(unsigned long arg)
{
    unsigned long YThres;

    if(copy_from_user(&YThres, (unsigned long __user *)arg, sizeof(unsigned long)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] set Y threshold=%d \n",(int)YThres);

    // record in global variable
    genY_Thres=YThres;

    GOP0_invColor_Set_Y_Threshold((int)YThres);

    return 0;
}

int _MDrv_FBIO_IOC_Set_ScalerinfoConfig(unsigned long arg)
{
    FB_GOP_INVCOLOR_SCALER_CONFIG stSclCfg;
    unsigned long tempcorpAEwidth=0;
    unsigned long tempcorpAEheight=0;
    int tempAEblkwidth=0;
    int tempAEblkheight=0;
    int tempAEblkxnum=0;
    int tempAEblkynum=0;

    if(copy_from_user(&stSclCfg, (FB_GOP_INVCOLOR_SCALER_CONFIG __user *)arg, sizeof(FB_GOP_INVCOLOR_SCALER_CONFIG)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] set Scl config, CropEn=%d, ScalingEn=%d, RotateEn=%d\n",
    (int)stSclCfg.bCropEn,(int)stSclCfg.bScalingEn,(int)stSclCfg.bRotateEn);

    if((stSclCfg.u32ScalerWidth<1)||(stSclCfg.u32ScalerWidth>1920))
    {
        FBDBGERR( "[FB Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! parameter wrong\n");
        return -EFAULT;
    }
    if((stSclCfg.u32ScalerHeight<1)||(stSclCfg.u32ScalerHeight>1920))
    {
        FBDBGERR( "[FB Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! parameter wrong\n");
        return -EFAULT;
    }

    // record in global variable
    genSC_info.bCropEn        = stSclCfg.bCropEn;
    genSC_info.bScalingEn     = stSclCfg.bScalingEn;
    genSC_info.u32ScalerWidth = stSclCfg.u32ScalerWidth;
    genSC_info.u32ScalerHeight= stSclCfg.u32ScalerHeight;
    genSC_info.u32CropXstart  = stSclCfg.u32CropXstart;
    genSC_info.u32CropXend    = stSclCfg.u32CropXend;
    genSC_info.u32CropYstart  = stSclCfg.u32CropYstart;
    genSC_info.u32CropYend    = stSclCfg.u32CropYend;
    // rotate will be get from ISP driver
    //genSC_info.bRotateEn      = stSclCfg.bRotateEn;
    //genSC_info.enRotateA      = stSclCfg.enRotateA;

    if(stSclCfg.bCropEn)
    {
        if((genAE_info.u32AEBlkWidth!=0)&&(genAE_info.u32AEBlkHeight!=0)&&(genAE_info.u32AEDisplayWidth!=0)&&(genAE_info.u32AEDisplayHeight!=0))
        {
            if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
            {
                GOP0_invColor_Set_Crop_Config((int)stSclCfg.u32CropXstart,(int)stSclCfg.u32CropYstart,(int)genAE_info.u32AEBlkHeight,(int)genAE_info.u32AEBlkWidth,(int)genAE_info.u32AEDisplayHeight);
            }
            else
            {
                GOP0_invColor_Set_Crop_Config((int)stSclCfg.u32CropXstart,(int)stSclCfg.u32CropYstart,(int)genAE_info.u32AEBlkWidth,(int)genAE_info.u32AEBlkHeight,(int)genAE_info.u32AEDisplayWidth);
            }
        }
        else
        {
            FBDBGERR( "[FB Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! No AE info\n");
            return -EFAULT;
        }
    }

    // when scaling happens, change AE block size with this scale
    if(stSclCfg.bScalingEn)
    {
        if((genAE_info.u32AEBlkWidth!=0)&&(genAE_info.u32AEBlkHeight!=0)&&(genAE_info.u32AEDisplayWidth!=0)&&(genAE_info.u32AEDisplayHeight!=0))
        {
            if(stSclCfg.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info.u32CropYend-genSC_info.u32CropYstart;
                    tempcorpAEheight=genSC_info.u32CropXend-genSC_info.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info.u32AEDisplayWidth/(int)genAE_info.u32AEBlkWidth);
                    if(tempAEblkheight > 0)
                    {
                        while(((genSC_info.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkheight++;
                            if(tempAEblkheight>genSC_info.u32ScalerWidth)
                            {
                                tempAEblkheight=genSC_info.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkwidth++;
                        }
                    }
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info.u32CropXend-genSC_info.u32CropXstart;
                    tempcorpAEheight=genSC_info.u32CropYend-genSC_info.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayWidth/(int)genAE_info.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    FBDBGMORE("[FB Driver] crop:x_start=%d, x_end=%d\n",(int)genSC_info.u32CropXstart,(int)genSC_info.u32CropXend);
                    FBDBGMORE("[FB Driver] AE:D_width=%d, Blk_width=%d\n",(int)genAE_info.u32AEDisplayWidth,(int)genAE_info.u32AEBlkWidth);
                    FBDBGMORE("[FB Driver] tempcorpAEwidth=%d, tempcorpAEheight=%d\n",(int)tempcorpAEwidth,(int)tempcorpAEheight);
                    FBDBGMORE("[FB Driver] before tempAEblkwidth=%d, tempAEblkheight=%d\n",(int)tempAEblkwidth,(int)tempAEblkheight);
                    if(tempAEblkwidth > 0)
                    {
                        while(((genSC_info.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkwidth++;
                            if(tempAEblkwidth>genSC_info.u32ScalerWidth)
                            {
                                tempAEblkwidth=genSC_info.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkheight++;
                        }
                    }
                    FBDBGMORE("[FB Driver] after tempAEblkwidth=%d, tempAEblkheight=%d\n",(int)tempAEblkwidth,(int)tempAEblkheight);

                    //sw patch for hw error
                    tempAEblkheight=mdrvinfinityfb_InverseColor_RefineBlkHeightMode((int)genSC_info.u32ScalerWidth,tempAEblkheight);
                    FBDBGMORE("[FB Driver] after sw patch tempAEblkwidth=%d, tempAEblkheight=%d\n",(int)tempAEblkwidth,(int)tempAEblkheight);

                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerHeight/(int)genAE_info.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerWidth/(int)genAE_info.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info.u32AEDisplayWidth/(int)genAE_info.u32AEBlkWidth);
                    if(tempAEblkheight > 0)
                    {
                        while(((genSC_info.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkheight++;
                            if(tempAEblkheight>genSC_info.u32ScalerWidth)
                            {
                                tempAEblkheight=genSC_info.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkwidth++;
                        }
                    }
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerWidth/genAE_info.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerHeight/genAE_info.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayWidth/(int)genAE_info.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    FBDBGMORE("[FB Driver] crop:x_start=%d, x_end=%d\n",genSC_info.u32CropXstart,genSC_info_.u32CropXend);
                    FBDBGMORE("[FB Driver] AE:D_width=%d, Blk_width=%d\n",genAE_info.u32AEDisplayWidth,genAE_info.u32AEBlkWidth);
                    FBDBGMORE("[FB Driver] before tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    if(tempAEblkwidth > 0)
                    {
                        while(((genSC_info.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkwidth++;
                            if(tempAEblkwidth>genSC_info.u32ScalerWidth)
                            {
                                tempAEblkwidth=genSC_info.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkheight++;
                        }
                    }
                    FBDBGMORE("[FB Driver] after tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);

                    //sw patch for hw error
                    tempAEblkheight=mdrvinfinityfb_InverseColor_RefineBlkHeightMode((int)genSC_info.u32ScalerWidth,tempAEblkheight);
                    FBDBGMORE("[FB Driver] after sw patch tempAEblkwidth=%d, tempAEblkheight=%d\n",(int)tempAEblkwidth,(int)tempAEblkheight);

                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FBDBGERR( "[FB Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP0_invColor_Set_AE_Config((int)genAE_info.u32AEBlkHeight,(int)genAE_info.u32AEBlkWidth,(int)genAE_info.u32AEDisplayHeight);
        }
        else
        {
            GOP0_invColor_Set_AE_Config((int)genAE_info.u32AEBlkWidth,(int)genAE_info.u32AEBlkHeight,(int)genAE_info.u32AEDisplayWidth);
        }
    }

    return 0;
}

int _MDrv_FBIO_IOC_AutoAEConfig(unsigned long arg)
{
    FB_GOP_INVCOLOR_AE_CONFIG stGOPAEConfig={0};
    isp_isr_ae_img_info stISPAEconfig={0};
    unsigned long tempcorpAEwidth=0;
    unsigned long tempcorpAEheight=0;
    int tempAEblkwidth=0;
    int tempAEblkheight=0;
    int tempAEblkxnum=0;
    int tempAEblkynum=0;
    int InvTblupdateMode=0;

    isp_get_ae_img_info(&stISPAEconfig);

    if((stISPAEconfig.blk_w<1)||(stISPAEconfig.blk_h<1)||(stISPAEconfig.img_w<1)||(stISPAEconfig.img_h<1))
    {
        FBDBGERR( "[FB Driver] Error: auto AE config fail!!! No proper AE value\n");
        FBDBGERR( "[FB Driver] auto AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d, rotate en=%d\n",
        (int)stGOPAEConfig.u32AEDisplayWidth,(int)stGOPAEConfig.u32AEDisplayHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,genSC_info.bRotateEn);
        return -EFAULT;
    }

    stGOPAEConfig.u32AEBlkWidth= (unsigned long)stISPAEconfig.blk_w;
    stGOPAEConfig.u32AEBlkHeight= (unsigned long)stISPAEconfig.blk_h;
    stGOPAEConfig.u32AEDisplayWidth= (unsigned long)stISPAEconfig.img_w;
    stGOPAEConfig.u32AEDisplayHeight= (unsigned long)stISPAEconfig.img_h;

    // record in global variable
    genAE_info.u32AEDisplayWidth  = stGOPAEConfig.u32AEDisplayWidth;
    genAE_info.u32AEDisplayHeight = stGOPAEConfig.u32AEDisplayHeight;
    genAE_info.u32AEBlkWidth      = stGOPAEConfig.u32AEBlkWidth;
    genAE_info.u32AEBlkHeight     = stGOPAEConfig.u32AEBlkHeight;
    if(stISPAEconfig.rot==1)
    {
        genSC_info.bRotateEn=1;
        InvTblupdateMode=1;//by cpu
    }
    else
    {
        genSC_info.bRotateEn=0;
        InvTblupdateMode=0;//by hw engine
    }

    GOP0_invColor_Set_UpdateMode(InvTblupdateMode);

    FBDBG( "[FB Driver] auto AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d, rotate en=%d\n",
    (int)stGOPAEConfig.u32AEDisplayWidth,(int)stGOPAEConfig.u32AEDisplayHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,genSC_info.bRotateEn);

    // when scaling happens, change AE block size with this scale
    if(genSC_info.bScalingEn)
    {
        if((genAE_info.u32AEBlkWidth!=0)&&(genAE_info.u32AEBlkHeight!=0)&&(genAE_info.u32AEDisplayWidth!=0)&&(genAE_info.u32AEDisplayHeight!=0))
        {
            if(genSC_info.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info.u32CropYend-genSC_info.u32CropYstart;
                    tempcorpAEheight=genSC_info.u32CropXend-genSC_info.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info.u32AEDisplayWidth/(int)genAE_info.u32AEBlkWidth);
                    while(((genSC_info.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkheight++;
                        if(tempAEblkheight>genSC_info.u32ScalerWidth)
                        {
                            tempAEblkheight=genSC_info.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkwidth++;
                    }
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info.u32CropXend-genSC_info.u32CropXstart;
                    tempcorpAEheight=genSC_info.u32CropYend-genSC_info.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayWidth/(int)genAE_info.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    while(((genSC_info.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkwidth++;
                        if(tempAEblkwidth>genSC_info.u32ScalerWidth)
                        {
                            tempAEblkwidth=genSC_info.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkheight++;
                    }
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerHeight/(int)genAE_info.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerWidth/(int)genAE_info.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info.u32AEDisplayWidth/(int)genAE_info.u32AEBlkWidth);
                    while(((genSC_info.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkheight++;
                        if(tempAEblkheight>genSC_info.u32ScalerWidth)
                        {
                            tempAEblkheight=genSC_info.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkwidth++;
                    }
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth+1,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info.u32AEBlkWidth*(int)genSC_info.u32ScalerWidth/genAE_info.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info.u32AEBlkHeight*(int)genSC_info.u32ScalerHeight/genAE_info.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info.u32AEDisplayWidth/(int)genAE_info.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info.u32AEDisplayHeight/(int)genAE_info.u32AEBlkHeight);
                    while(((genSC_info.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkwidth++;
                        if(tempAEblkwidth>genSC_info.u32ScalerWidth)
                        {
                            tempAEblkwidth=genSC_info.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkheight++;
                    }
                    GOP0_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FBDBGERR( "[FB Driver] Error: _MDrv_FBIO_IOC_AutoAEConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP0_invColor_Set_AE_Config((int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEDisplayHeight);
        }
        else
        {
            GOP0_invColor_Set_AE_Config((int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEDisplayWidth);
        }
    }

    if(genSC_info.bCropEn)//if crop enable, reset crop information when AE info change
    {
        if(genSC_info.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP0_invColor_Set_Crop_Config((int)genSC_info.u32CropXstart,(int)genSC_info.u32CropYstart,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEDisplayHeight);
        }
        else
        {
            GOP0_invColor_Set_Crop_Config((int)genSC_info.u32CropXstart,(int)genSC_info.u32CropYstart,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEDisplayWidth);
        }
    }

    //return ae settings to user
    if(copy_to_user((FB_GOP_INVCOLOR_AE_CONFIG __user *)arg, &stGOPAEConfig, sizeof(FB_GOP_INVCOLOR_AE_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb_ioctl
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
mdrvinfinityfb_ioctl(struct fb_info *pinfo, unsigned int cmd, unsigned long arg)
{
    int err=0, ret=0;
    unsigned int ret_Phy=0;

    if (NULL == pinfo)
    {
        FBDBGERR("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, pinfo is NULL pointer\r\n");
        return -ENOTTY;
    }

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != G3D_IOC_MAGIC_INFINITY)
        return -ENOTTY;

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

        case IOCTL_FB_SETPALETTE:
            FBDBG( "[FB Driver] Set Palette.\n");
            ret=_MDrv_FBIO_IOC_Set_Palette(arg);
            break;

        case IOCTL_FB_GETGENERALCONFIG:
            FBDBG( "[FB Driver] Get General Config.\n");
            ret=_MDrv_FBIO_IOC_Get_General_Config(arg);
            break;

        case IOCTL_FB_SETGENERALCONFIG:
            FBDBG( "[FB Driver] Set General Config.\n");
            ret=_MDrv_FBIO_IOC_Set_General_Config(pinfo,arg);
            break;

        case IOCTL_FB_SETENABLEINVCOLOR:
            FBDBG( "[FB Driver] Set Enable/Disable inverse color.\n");
            ret=_MDrv_FBIO_IOC_Set_EnInvColor(arg);
            break;

        case IOCTL_FB_SETAECONFIG:
            FBDBG( "[FB Driver] Set AE configurations.\n");
            ret=_MDrv_FBIO_IOC_Set_AEinfoConfig(arg);
            break;

        case IOCTL_FB_SETYTHRES:
            FBDBG( "[FB Driver] Set Y threshold.\n");
            ret=_MDrv_FBIO_IOC_Set_YThres(arg);
            break;

        case IOCTL_FB_SETSCALERCONFIG:
            FBDBG( "[FB Driver] Set Scl configurations.\n");
            ret=_MDrv_FBIO_IOC_Set_ScalerinfoConfig(arg);
            break;

        case IOCTL_FB_AUTOUPDATEAE:
            FBDBG( "[FB Driver] auto AE.\n");
            ret=_MDrv_FBIO_IOC_AutoAEConfig(arg);
            break;

        default:  /* redundant, as cmd was checked against MAXNR */
            FBDBGERR("[FB Driver] in default ioct\r\n");
            return -ENOTTY;
    }
    return ret;
}

static ssize_t gopinfo_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t n)
{
    return 0;
}

static ssize_t gopinfo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "[FB/GOP Driver Information]\n");
    str += scnprintf(str, end - str, "[Framebuffer]\n");
    str += scnprintf(str, end - str, " Width          : %4d pixel\n",sg_mdrvinfinityfb_default_var.xres);
    str += scnprintf(str, end - str, " Height         : %4d pixel\n",sg_mdrvinfinityfb_default_var.yres);
    str += scnprintf(str, end - str, " BPP            : %4d bytes per pixel\n",ANDROID_BYTES_PER_PIXEL);
    str += scnprintf(str, end - str, " Memory Size    : %d bytes\n",(unsigned int)sg_videomemorysize);
    str += scnprintf(str, end - str, " Memory Phy_Addr: 0x%x\n",sg_G3D_fb2_bus_addr1);
    str += scnprintf(str, end - str, " Memory Vir_Addr: 0x%x\n",(unsigned int)sg_pG3D_fb2_vir_addr1);
    str += scnprintf(str, end - str, "[GOP(GWIN)]\n");
    if(genGWIN==0)
        str += scnprintf(str, end - str, " GWIN Status    : Close\n");
    else if(genGWIN==1)
        str += scnprintf(str, end - str, " GWIN Status    : Open\n");
    else
        str += scnprintf(str, end - str, " GWIN Status    : Unknown\n");

    str += scnprintf(str, end - str, " H Start        : %4d\n",genGWinInfo.u18HStart);
    str += scnprintf(str, end - str, " H End          : %4d\n",genGWinInfo.u18HEnd);
    str += scnprintf(str, end - str, " V Start        : %4d\n",genGWinInfo.u18VStart);
    str += scnprintf(str, end - str, " V End          : %4d\n",genGWinInfo.u18VEnd);

    if(genGOPALPHA.bEn==0)
        str += scnprintf(str, end - str, " Alpha Status   : Close\n");
    else if(genGOPALPHA.bEn==1)
        str += scnprintf(str, end - str, " Alpha Status   : Open\n");
    else
        str += scnprintf(str, end - str, " Alpha Status   : Unknown\n");


    if(genGOPALPHA.enAlphaType==PIXEL_ALPHA)
        str += scnprintf(str, end - str, " Alpha Type     : Pixel Alpha\n");
    else if(genGOPALPHA.enAlphaType==CONST_ALPHA)
    {
        str += scnprintf(str, end - str, " Alpha Type     : Constant Alpha\n");
        str += scnprintf(str, end - str, " Alpha Value    : 0x%2x\n",genGOPALPHA.u8Alpha);
    }
    else
        str += scnprintf(str, end - str, " Alpha Type     : wrong alpha type\n");

    if(genGOPCOLORKEY.bEn==0)
        str += scnprintf(str, end - str, " Colorkey Status: Close\n");
    else if(genGOPCOLORKEY.bEn==1)
    {
        str += scnprintf(str, end - str, " Colorkey Status: Open\n");
        str += scnprintf(str, end - str, " Red            : 0x%2x\n",genGOPCOLORKEY.u8R);
        str += scnprintf(str, end - str, " Green          : 0x%2x\n",genGOPCOLORKEY.u8G);
        str += scnprintf(str, end - str, " Blue           : 0x%2x\n",genGOPCOLORKEY.u8B);
    }
    else
        str += scnprintf(str, end - str, " Colorkey Status: Unknown\n");


    if (str > buf)  str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);

}

DEVICE_ATTR(gopinfo, 0644, gopinfo_show, gopinfo_store);


static int mdrvinfinityfb_suspend(struct platform_device *pdev,
                 pm_message_t state)
{
    int clockIdxtemp=0;
    FBDBG("[FB Driver] in mdrvinfinityfb_suspend \r\n");

    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            clk_disable_unprepare(gop_clks[clockIdxtemp]);
        }
    }
    //clk_disable_unprepare(gop_psram_clk);
    return 0;
}

static int mdrvinfinityfb_resume(struct platform_device *pdev)
{
    //dma_addr_t fb2_PHY_ADD_SHOW=0;
    int ret=-1;
    int clockIdxtemp=0;

    // open all needed clocks
    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            if (IS_ERR(gop_clks[clockIdxtemp]))
            {
                FBDBGERR( "[FB Driver] ERROR: Fail to get gop clk! Clock idx=%d\n",clockIdxtemp);
                kfree(gop_clks);
                return ret;
            }
            else
            {
                clk_prepare_enable(gop_clks[clockIdxtemp]);
            }
        }
    }

    FBDBG("[FB Driver] mdrvinfinityfb_resume \r\n");

    GOP_Setmode(TRUE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    //set palette
    mdrvinfinityfb_infinity_SetPalette(stGOPCurrentPalInfo,0,255);

    return 0;
}
/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb_probe
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
static int mdrvinfinityfb_probe(struct platform_device *dev)
{
    struct fb_info *pinfo = NULL;
    int retval = -ENOMEM;
    int ret=-1;
    int clockIdxtemp=0;

    if (NULL == dev)
    {
        FBDBGERR("[FB Driver] ERROR: in mdrvinfinityfb_prob: dev is NULL pointer \r\n");
        return -ENOTTY;
    }

    // open all needed clocks
    num_parents_clocks = of_clk_get_parent_count(dev->dev.of_node);
    gop_clks=kzalloc(((sizeof(struct clk *) * num_parents_clocks)),GFP_KERNEL);

    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks ; clockIdxtemp++)
    {
        gop_clks[clockIdxtemp]= of_clk_get(dev->dev.of_node, clockIdxtemp);
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            if (IS_ERR(gop_clks[clockIdxtemp]))
            {
                FBDBGERR( "[FB Driver] ERROR: Fail to get gop clk! Clock idx=%d\n",clockIdxtemp);
                kfree(gop_clks);
                return ret;
            }
            else
            {
                clk_prepare_enable(gop_clks[clockIdxtemp]);
            }
        }
    }

    // init global variable
    mdrvinfinityfb_updateinfo();

    pinfo = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
    if (!pinfo)
        goto err;

    // copy data for register framebuffer
    pinfo->screen_base = (char *)CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1);
    pinfo->fbops = &sg_mdrvinfinityfb_ops;
    pinfo->var = sg_mdrvinfinityfb_default_var;
    pinfo->fix = sg_mdrvinfinityfb_fix;
    pinfo->pseudo_palette = pinfo->par;
    pinfo->par = NULL;
    pinfo->flags = FBINFO_FLAG_DEFAULT;

    // allocate color map
    // 256 is following to the vfb.c
    retval = fb_alloc_cmap(&pinfo->cmap, 256, 0);
    if (retval < 0)
        goto err1;

    // register our fb to fbmem.c
    retval = register_framebuffer(pinfo);
    if (retval < 0)
        goto err2;

    // set data to device
    platform_set_drvdata(dev, pinfo);

    // set gop register settings
    GOP_Setmode(FALSE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    // set gop inverse color init settings
    GOP0_invColor_init();

    // set init palette
    mdrvinfinityfb_infinity_SetPalette(DefaultPaletteEntry,0,255);

    // register sysfs
    ret = device_create_file(&dev->dev, &dev_attr_gopinfo);
    if (ret != 0)
    {
      dev_err(&dev->dev,
       "[FB Driver] Failed to create ptgen_call sysfs files: %d\n", ret);
    }
    else
    {
        FBDBGMORE("[FB Driver] device attr ok\n");
    }

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
| FUNCTION    : mdrvinfinityfb_remove
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
static int mdrvinfinityfb_remove(struct platform_device *dev)
{
    struct fb_info *pinfo;
    int clockIdxtemp=0;
    pinfo = 0;

    if (NULL == dev)
    {
        FBDBGERR("[FB Driver] ERROR: mdrvinfinityfb_remove: dev is NULL pointer \n");
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

    // close all clocks
    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            clk_disable_unprepare(gop_clks[clockIdxtemp]);
        }
    }
    return 0;
}

static int mdrvinfinityfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    dma_addr_t  fb2_pandisplay_PHY_ADD_SHOW=0;

    FBDBGMORE( "[FB Driver]%s: var->yoffset=%x! info->var.height=%x\n ",__func__,var->yoffset,info->var.height);

    if(genGOPGernelSettings.enBufferNum==FB_DOUBLE_BUFFER)//double buffer
    {
        if (var->yoffset >= info->var.height)
        {
            fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start + (info->var.width * info->var.height * ANDROID_BYTES_PER_PIXEL);

            FBDBGMORE( "[FB Driver]%s: use buffer 1! Addr=%x\n ",__func__,fb2_pandisplay_PHY_ADD_SHOW);
        }
        else
        {
            fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start;

            FBDBGMORE( "[FB Driver]%s: use buffer 0 Addr=%x!\n ",__func__,fb2_pandisplay_PHY_ADD_SHOW);
        }
    }
    else
    {
        fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start;

        FBDBGMORE( "mdrvinfinityfb_pan_display: use buffer 0 Addr=%x!\n ",fb2_pandisplay_PHY_ADD_SHOW);
    }

    GOP_Pan_Display(0, fb2_pandisplay_PHY_ADD_SHOW);

    return 0;
}


//check var to see if supported by this device, defined for infinity
static int mdrvinfinityfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    int MiuBusLen=128;//miu bus length in iNfinity
    int DisplayPerPixel=0;
    int max_x_res=2560;
    int max_y_res=2048;

    DisplayPerPixel=MiuBusLen/(ANDROID_BYTES_PER_PIXEL*8);

    //MstarFB_ENTER();

    if((var->yres<1)||(var->xres<1))
    {
        FBDBGERR( "[FB Driver] Error: fb_check_var: buffer x or y size should be > 0 !\n ");
        return -EINVAL;
    }
    if((var->yres * var->xres)>(max_x_res*max_y_res))
    {
        FBDBGERR( "[FB Driver] Error: fb_check_var: total buffer size should be lower than 1920*1088!\n ");
        return -EINVAL;
    }

    if((var->xres % DisplayPerPixel)!=0)
    {
        FBDBGERR( "[FB Driver] Error: fb_check_var: Display Per Pixel for Buffer Width should be %d!\n ",DisplayPerPixel);
        return -EINVAL;
    }

    //MstarFB_LEAVE();

    return 0;
}

module_platform_driver(sg_mdrvinfinityfb_driver);

MODULE_LICENSE("GPL");
