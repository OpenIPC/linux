

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

#include "mdrv_fb1.h"

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
static void __init mdrvinfinityfb1_updateinfo(void)
{
    static int main_videomemory_size=0;

    //default settings for Stretch mode
    genGOPGernelSettings_gop1.u32Width=sg_mdrvinfinityfb1_default_var.xres;
    genGOPGernelSettings_gop1.u32Height=sg_mdrvinfinityfb1_default_var.yres;
    genGOPGernelSettings_gop1.enBufferNum=FB_DOUBLE_BUFFER;
    genGOPGernelSettings_gop1.enStretchH_Ratio=FB_STRETCH_H_RATIO_1;
    genGOPGernelSettings_gop1.u32DisplayWidth=genGOPGernelSettings_gop1.u32Width*genGOPGernelSettings_gop1.enStretchH_Ratio;
    genGOPGernelSettings_gop1.u32DisplayHeight=genGOPGernelSettings_gop1.u32Height;

    //default settings for global structure of fb_var_screeninfo
    sg_mdrvinfinityfb1_default_var.xres         = 0;
    sg_mdrvinfinityfb1_default_var.yres         = 0;
    sg_mdrvinfinityfb1_default_var.xres_virtual = sg_mdrvinfinityfb1_default_var.xres;
    sg_mdrvinfinityfb1_default_var.yres_virtual = sg_mdrvinfinityfb1_default_var.yres * (genGOPGernelSettings_gop1.enBufferNum);//double buffer for pan display
    sg_mdrvinfinityfb1_default_var.width        = sg_mdrvinfinityfb1_default_var.xres;
    sg_mdrvinfinityfb1_default_var.height       = sg_mdrvinfinityfb1_default_var.yres;

    //GWin init, same size as screen
    genGWinInfo_gop1.u18HStart = 0;
    genGWinInfo_gop1.u18HEnd   = genGWinInfo_gop1.u18HStart + sg_mdrvinfinityfb1_default_var.xres;
    genGWinInfo_gop1.u18VStart = 0;
    genGWinInfo_gop1.u18VEnd   = genGWinInfo_gop1.u18VStart + sg_mdrvinfinityfb1_default_var.yres;
    main_videomemory_size = (sg_mdrvinfinityfb1_default_var.xres * sg_mdrvinfinityfb1_default_var.yres * ANDROID_BYTES_PER_PIXEL * (genGOPGernelSettings_gop1.enBufferNum));


    sg_videomemorysize_gop1 = main_videomemory_size;

    //default settings for global structure of fb_fix_screeninfo
    sg_mdrvinfinityfb1_fix.smem_start  = sg_G3D_fb2_bus_addr1_gop1;
    sg_mdrvinfinityfb1_fix.smem_len    = sg_videomemorysize_gop1;
    sg_mdrvinfinityfb1_fix.line_length = sg_mdrvinfinityfb1_default_var.xres * ANDROID_BYTES_PER_PIXEL;
    sg_mdrvinfinityfb1_fix.ypanstep    = 1;

    FB1DBGMORE("[FB1 Driver] updateinfo: GOP_WIDTH = [%d], GOP_HEIGHT = [%d], GOPmemorysize = 0x%x \r\n",sg_mdrvinfinityfb1_default_var.xres,sg_mdrvinfinityfb1_default_var.yres,(unsigned int)sg_videomemorysize_gop1);

}

// update global variables while setting new resolution by ioctl FBIOPUT_VSCREENINFO
static int mdrvinfinityfb1_infinity_updateinfo_par(unsigned int setWidth,unsigned int setHeight)
{

    static int main_videomemory_size=0;

    //update settings for global structure of fb_var_screeninfo
    sg_mdrvinfinityfb1_default_var.xres         = setWidth;
    sg_mdrvinfinityfb1_default_var.yres         = setHeight;
    sg_mdrvinfinityfb1_default_var.xres_virtual = sg_mdrvinfinityfb1_default_var.xres;
    sg_mdrvinfinityfb1_default_var.yres_virtual = sg_mdrvinfinityfb1_default_var.yres * (genGOPGernelSettings_gop1.enBufferNum);//double buffer for pan display
    sg_mdrvinfinityfb1_default_var.width        = sg_mdrvinfinityfb1_default_var.xres;
    sg_mdrvinfinityfb1_default_var.height       = sg_mdrvinfinityfb1_default_var.yres;

    //update settings for global structure of Stetch mode
    genGOPGernelSettings_gop1.u32Width=sg_mdrvinfinityfb1_default_var.xres;
    genGOPGernelSettings_gop1.u32Height=sg_mdrvinfinityfb1_default_var.yres;
    genGOPGernelSettings_gop1.u32DisplayWidth=genGOPGernelSettings_gop1.u32Width*genGOPGernelSettings_gop1.enStretchH_Ratio;
    genGOPGernelSettings_gop1.u32DisplayHeight=genGOPGernelSettings_gop1.u32Height;

    //GWin init, same size as screen
    genGWinInfo_gop1.u18HStart = 0;
    genGWinInfo_gop1.u18HEnd   = genGWinInfo_gop1.u18HStart + sg_mdrvinfinityfb1_default_var.xres;
    genGWinInfo_gop1.u18VStart = 0;
    genGWinInfo_gop1.u18VEnd   = genGWinInfo_gop1.u18VStart + sg_mdrvinfinityfb1_default_var.yres;
    main_videomemory_size = (sg_mdrvinfinityfb1_default_var.xres * sg_mdrvinfinityfb1_default_var.yres * ANDROID_BYTES_PER_PIXEL * (genGOPGernelSettings_gop1.enBufferNum));

    // free memory if it has
    if(sg_videomemorysize_gop1 > 0)
    {
        free_dmem(KEY_DMEM_FB1_BUF,
                  PAGE_ALIGN(sg_videomemorysize_gop1),
                  sg_pG3D_fb2_vir_addr1_gop1,
                  sg_G3D_fb2_bus_addr1_gop1);
    }

    sg_videomemorysize_gop1 = main_videomemory_size;

    FB1DBG ("[FB1 Driver] updateinfo_par: GOP_WIDTH = [%d], GOP_HEIGHT = [%d], GOPmemorysize = 0x%x \r\n",sg_mdrvinfinityfb1_default_var.xres,sg_mdrvinfinityfb1_default_var.yres,(unsigned int)sg_videomemorysize_gop1);

    // allocate new memory
    if (!(sg_pG3D_fb2_vir_addr1_gop1 = alloc_dmem(KEY_DMEM_FB1_BUF,
                                             PAGE_ALIGN(sg_videomemorysize_gop1),
                                             &sg_G3D_fb2_bus_addr1_gop1)))
    {
        FB1DBGERR("[FB1 Driver] Error: updateinfo_par: unable to allocate screen memory\n");
        return -ENOMEM;
    }

    // prevent initial garbage on screen ; to clean memory
    memset(CAST_2_IOMEM_CHAR_P sg_pG3D_fb2_vir_addr1_gop1,0, sg_videomemorysize_gop1);

    //update settings for global structure of fb_fix_screeninfo
    sg_mdrvinfinityfb1_fix.smem_start  = sg_G3D_fb2_bus_addr1_gop1;
    sg_mdrvinfinityfb1_fix.smem_len    = sg_videomemorysize_gop1;
    sg_mdrvinfinityfb1_fix.line_length = sg_mdrvinfinityfb1_default_var.xres * ANDROID_BYTES_PER_PIXEL;

    FB1DBG("[FB1 Driver] vir=0x%x, phy=0x%x\n", (unsigned int)sg_pG3D_fb2_vir_addr1_gop1, sg_G3D_fb2_bus_addr1_gop1);


    return 0;
}

static int mdrvinfinityfb1_infinity_SetPalette(FB_GOP_PaletteEntry *pPalEntry, int intPalStart,int intPalEnd)
{
    int i   = 0;
    int ret = -1;

    //open scl0 fclk to load palette
    if (IS_ERR(gop1_clks[1]))
    {
        FB1DBGERR( "[FB1 Driver] Error: SetPalette Fail to get gop fclk of scl!\n" );
        return ret;
    }
    else
    {
        clk_prepare_enable(gop1_clks[1]);
    }

    FB1DBGMORE("Palette value:(B,G,R,A)\n");

    //write palette
    for(i=intPalStart;i<=intPalEnd;i++)
    {
        GOP1_Set_Palette_RIU(0, i, pPalEntry[i].BGRA.u8A, pPalEntry[i].BGRA.u8R, pPalEntry[i].BGRA.u8G, pPalEntry[i].BGRA.u8B);

        //record in global variable
        stGOPCurrentPalInfo_GOP1[i].BGRA.u8B=pPalEntry[i].BGRA.u8B;
        stGOPCurrentPalInfo_GOP1[i].BGRA.u8G=pPalEntry[i].BGRA.u8G;
        stGOPCurrentPalInfo_GOP1[i].BGRA.u8R=pPalEntry[i].BGRA.u8R;
        stGOPCurrentPalInfo_GOP1[i].BGRA.u8A=pPalEntry[i].BGRA.u8A;
        FB1DBGMORE("(%d,%d,%d,%d)\n",stGOPCurrentPalInfo_GOP1[i].BGRA.u8B,stGOPCurrentPalInfo_GOP1[i].BGRA.u8G,stGOPCurrentPalInfo_GOP1[i].BGRA.u8R,stGOPCurrentPalInfo_GOP1[i].BGRA.u8A);
    }

    //close clk
    clk_disable_unprepare(gop1_clks[1]);

    return 0;
}

static int mdrvinfinityfb1_set_par(struct fb_info *info)
{
    int ret = 0;
    FB1DBG( "[FB1 Driver] Test I3!\n" );

    //Stretch mode settings, if use back to this to set resolution, it will set to default
    genGOPGernelSettings_gop1.enStretchH_Ratio=FB_STRETCH_H_RATIO_1;
    genGOPGernelSettings_gop1.enBufferNum=FB_DOUBLE_BUFFER;

    ret=mdrvinfinityfb1_infinity_updateinfo_par(info->var.xres,info->var.yres);

    info->fix.smem_start  = sg_mdrvinfinityfb1_fix.smem_start;
    info->fix.smem_len    = sg_mdrvinfinityfb1_fix.smem_len;

    info->var.width       = info->var.xres;
    info->var.height      = info->var.yres;

    info->screen_base     = (char *)CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1_gop1);
    info->fix.line_length = info->var.xres * ANDROID_BYTES_PER_PIXEL;

    //GOP_Setmode(FALSE, 0,sg_mdrvinfinityfb1_default_var.xres,sg_mdrvinfinityfb1_default_var.yres,sg_G3D_fb2_bus_addr1_gop1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
    GOP1_Set_Outputmode(0, sg_mdrvinfinityfb1_default_var.xres,sg_mdrvinfinityfb1_default_var.yres,sg_G3D_fb2_bus_addr1_gop1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL);

    return ret;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb1_mmap
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
static int mdrvinfinityfb1_mmap(struct fb_info *pinfo, struct vm_area_struct *vma)
{
    size_t size;
    size = 0;

    if (sg_videomemorysize_gop1<1)
    {
        FB1DBGERR("[FB1 Driver] ERROR: mmap, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if (NULL == pinfo)
    {
        FB1DBGERR("[FB1 Driver] ERROR: mmap, pinfo is NULL pointer !\n");
        return -ENOTTY;
    }
    if (NULL == vma)
    {
        FB1DBGERR("[FB1 Driver] ERROR: mmap, vma is NULL pointer !\n");
        return -ENOTTY;
    }

    size = vma->vm_end - vma->vm_start;

    vma->vm_pgoff += ((sg_G3D_fb2_bus_addr1_gop1 ) >> PAGE_SHIFT);

    FB1DBG(
    "[FB1 Driver] mmap vma->vm_start=%x\n vma->vm_end=%x\n vma->vm_pgoff =%x\n",
    (unsigned int) vma->vm_start, (unsigned int)vma->vm_end ,
    (unsigned int)vma->vm_pgoff);

    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

//sw update inverse color's inverse table thread when rotate case happens
static int mdrvinfinityfb1_SWupdate_InvTable(void *arg)
{
    int ret=0;
    while(1)
    {
        if (kthread_should_stop()) break;

        ret=GOP1_invColor_CPU_Upate_InvTable(genAE_info_gop1.u32AEBlkWidth,
                                             genAE_info_gop1.u32AEBlkHeight,
                                             genAE_info_gop1.u32AEDisplayWidth,
                                             genAE_info_gop1.u32AEDisplayHeight);
        if(!ret)//if update wrong, break
        {
            FB1DBGERR("[FB1 Driver] ERROR: mdrvinfinityfb1_SWupdate_InvTable wrong!\n");
            genTreadisError_gop1=1;
            break;
        }


    }

   return 0;

}

static int mdrvinfinityfb1_SWupdate_InvTable_debugmode(void *arg)
{

    while(1)
    {
        if (kthread_should_stop()) break;

        GOP1_invColor_DebugMode_UpdateInvTable();
        msleep(1000);
    }

   return 0;

}


//check whether need doing sw update inverse color's inverse table or not
static void mdrvinfinityfb1_checkInvTable_UpdateMode(void)
{
    int idx=0;
    int ret;

    if((genInv_Color_gop1==1) && (genSC_info_gop1.bRotateEn==1))
    {
        idx=1;
    }
    else
    {
        idx=0;
    }

    if(genTreadisError_gop1==1)
    {
        pSWupdateInvThread_gop1=NULL;
        genTreadisError_gop1=0;
    }

    if(idx==1)//enable thread
    {
        if(pSWupdateInvThread_gop1==NULL)
        {
            pSWupdateInvThread_gop1 = kthread_create(mdrvinfinityfb1_SWupdate_InvTable,(void *)&pSWupdateInvThread_gop1,"GOP1_SWupdate_InvColor");
            if (IS_ERR(pSWupdateInvThread_gop1))
            {
                ret = PTR_ERR(pSWupdateInvThread_gop1);
                pSWupdateInvThread_gop1 = NULL;
                FB1DBGERR("[FB1 Driver] ERROR: mdrvinfinityfb1_checkInvTable_UpdateMode wrong! Create thread fail! ret=%d\n",ret);
            }
            else
            {
                wake_up_process(pSWupdateInvThread_gop1);
            }
        }
    }
    else if (idx==0 && (pSWupdateInvThread_gop1!=NULL))//disable thread
    {
        kthread_stop(pSWupdateInvThread_gop1);
        pSWupdateInvThread_gop1 = NULL;
    }

}



// FB Ioctl Functions

int _MDrv_FBIO_IOC_Get_SUPINFO_GOP1(unsigned long arg)
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


int _MDrv_FBIO_IOC_Get_GWIN_GOP1(unsigned long arg)
{
    FB_GOP_GWIN_CONFIG stCfg;

    stCfg.u18HStart = genGWinInfo_gop1.u18HStart;
    stCfg.u18HEnd   = genGWinInfo_gop1.u18HEnd;
    stCfg.u18VStart = genGWinInfo_gop1.u18VStart;
    stCfg.u18VEnd   = genGWinInfo_gop1.u18VEnd;

    if(copy_to_user((FB_GOP_GWIN_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_GWIN_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_GWIN_GOP1(unsigned long arg)
{
    unsigned long GOP_Reg_Base = 0;
    FB_GOP_GWIN_CONFIG stCfg;
    unsigned short MIU_BUS     = 0x04;
    unsigned short u16FrameBuffer_Bytes_Per_Pixel = ANDROID_BYTES_PER_PIXEL;
    unsigned long u32TempVal   = 0;
    unsigned long GOP_Reg_DB   = GOP_BANK_DOUBLE_WR_G1;

    if (sg_videomemorysize_gop1<1)
    {
        FB1DBGERR("[FB1 Driver] ERROR: _MDrv_FBIO_IOC_Set_GWIN, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    // gwin info is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP11_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP11_PA==mdrv_BASE_REG_GOP10_PA)
    {
        GOP1_SwitchSubBank(1);
    }

    if(copy_from_user(&stCfg, (FB_GOP_GWIN_CONFIG __user *)arg, sizeof(FB_GOP_GWIN_CONFIG)))
    {
        return -EFAULT;
    }

    genGWinInfo_gop1.u18HStart = stCfg.u18HStart;
    genGWinInfo_gop1.u18HEnd   = stCfg.u18HEnd;
    genGWinInfo_gop1.u18VStart = stCfg.u18VStart;
    genGWinInfo_gop1.u18VEnd   = stCfg.u18VEnd;

    // GOP Display Setting, GWIN
    // 2.1 GWIN display area in panel : : H Start postion and end information
    u32TempVal=(genGWinInfo_gop1.u18HStart*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_04,u32TempVal); // H start
    u32TempVal=(genGWinInfo_gop1.u18HEnd*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_05,u32TempVal); // H end

    // 2.2 GWIN  display area in panel : V Start postion and end information
    OUTREG16(GOP_Reg_Base+REG_GOP_06,genGWinInfo_gop1.u18VStart); // V start line
    OUTREG16(GOP_Reg_Base+REG_GOP_08,genGWinInfo_gop1.u18VEnd); // V end line

    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP11_PA==mdrv_BASE_REG_GOP10_PA)
    {
        GOP1_SwitchSubBank(0);
    }

    //write gop register
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

    return 0;
}


int _MDrv_FBIO_IOC_Get_EnGOP_GOP1(unsigned long arg)
{
    unsigned char enGOP;

    GOP1_Get_Enable_GWIN(0, &enGOP);

    //remove check scaler's switch later in I3
    if(enGOP && INREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_64,0x0001))
        enGOP=1;
    else
        enGOP=0;

    if(copy_to_user((unsigned char __user *)arg, &enGOP, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_EnGOP_GOP1(unsigned long arg)
{
    unsigned char enGOP;

    if (sg_videomemorysize_gop1<1)
    {
        FB1DBGERR("[FB1 Driver] ERROR: _MDrv_FBIO_IOC_Set_EnGOP, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&enGOP, (unsigned char __user *)arg, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    FB1DBG( "[FB1 Driver] enable GOP=%d \n",enGOP);

    // record in global variable
    genGWIN_gop1=enGOP;

    // this switch will let scaler handle, remove later in I3
    // enable/disable gop switch at scaler
    if(enGOP)
        OUTREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_64,0x0001,0x0001);
    else
        OUTREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_64,0x0000,0x0001);

    GOP1_Set_Enable_GWIN(0,enGOP);

    return 0;
}


int _MDrv_FBIO_IOC_Get_Alpha_GOP1(unsigned long arg)
{
    FB_GOP_ALPHA_CONFIG stCfg;
    int alpha_type  = 0;
    int alpha_value = 0;

    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    // get alpha blending settings from register
    GOP1_Get_Alpha(0,&alpha_type,&alpha_value);

    // copy result from register
    stCfg.bEn         = genGOPALPHA_gop1.bEn;
    stCfg.enAlphaType = (FB_GOP_ALPHA_TYPE)alpha_type;
    stCfg.u8Alpha     = (unsigned char)alpha_value;

    // record settings in global variables
    genGOPALPHA_gop1.enAlphaType=stCfg.enAlphaType;
    genGOPALPHA_gop1.u8Alpha=stCfg.u8Alpha;

    if(copy_to_user((FB_GOP_ALPHA_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_Constant_Alpha_GOP1(unsigned long arg)
{
    FB_GOP_ALPHA_CONFIG stCfg;

    if (sg_videomemorysize_gop1<1)
    {
        FB1DBGERR("[FB1 Driver] ERROR: _MDrv_FBIO_IOC_Set_Constant_Alpha, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }
    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    FB1DBG( "[FB1 Driver] Set alpha enable=%d, type=%d, constant alpha value=0x%x \n",(int)stCfg.bEn, (int)stCfg.enAlphaType, stCfg.u8Alpha);

    if(stCfg.bEn)
    {
        GOP1_Set_Constant_Alpha(0, (int)stCfg.enAlphaType, stCfg.u8Alpha);
    }
    else
    {
        GOP1_Set_Constant_Alpha(0, CONST_ALPHA, 0);//close alpha blending, let gop always shows
    }

    genGOPALPHA_gop1.bEn=stCfg.bEn;
    genGOPALPHA_gop1.enAlphaType=stCfg.enAlphaType;
    genGOPALPHA_gop1.u8Alpha=stCfg.u8Alpha;

    return 0;
}

int _MDrv_FBIO_IOC_Get_Color_Key_GOP1(unsigned long arg)
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

    GOP1_Get_Color_Key(0,&Enable,&Red,&Green,&Blue);

    // enable/disable
    stCfg.bEn = (unsigned char)Enable;

    // colorkey value
    stCfg.u8R = (unsigned char)Red;
    stCfg.u8G = (unsigned char)Green;
    stCfg.u8B = (unsigned char)Blue;

    genGOPCOLORKEY_gop1.bEn=stCfg.bEn;
    genGOPCOLORKEY_gop1.u8R=stCfg.u8R;
    genGOPCOLORKEY_gop1.u8G=stCfg.u8G;
    genGOPCOLORKEY_gop1.u8B=stCfg.u8B;

    if(copy_to_user((FB_GOP_COLORKEY_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_Color_Key_GOP1(unsigned long arg)
{
    FB_GOP_COLORKEY_CONFIG stCfg;

    if (sg_videomemorysize_gop1<1)
    {
        FB1DBGERR("[FB1 Driver] ERROR: _MDrv_FBIO_IOC_Set_Color_Key, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }
    if(copy_from_user(&stCfg, (FB_GOP_COLORKEY_CONFIG __user *)arg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    FB1DBG( "[FB1 Driver] Set ColorKey enable=%d, R=0x%x, G=0x%x, B=0x%x\n", (int)stCfg.bEn, stCfg.u8R, stCfg.u8G, stCfg.u8B);

    GOP1_Set_Color_Key(0, stCfg.bEn, stCfg.u8R, stCfg.u8G, stCfg.u8B);
    genGOPCOLORKEY_gop1.bEn=stCfg.bEn;
    genGOPCOLORKEY_gop1.u8R=stCfg.u8R;
    genGOPCOLORKEY_gop1.u8G=stCfg.u8G;
    genGOPCOLORKEY_gop1.u8B=stCfg.u8B;

    return 0;
}

int _MDrv_FBIO_IOC_imageblit_GOP1(struct fb_info *info,unsigned long arg)
{
    int ret = 0;

// this ioctl is unused, the same fuction can be accessed by fb_ops -> fb_imageblit
#if 0
    struct fb_image stCfg;
    int i = 0;


    if (sg_videomemorysize_gop1<1)
    {
        FB1DBGERR("[FB1 Driver] ERROR: _MDrv_FBIO_IOC_imageblit, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&stCfg, (struct fb_image __user *)arg, sizeof(const struct fb_image)))
    {
        return -EFAULT;
    }

    if (NULL == stCfg.data)
    {
        FB1DBGERR("[FB1 Driver] ERROR:in mdrvinfinityfb1_ioctl, fb_image.data is NULL pointer\r\n");
        return -EFAULT;
    }


    if (stCfg.fg_color>255)
    {
        FB1DBGERR("[FB1 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }

    if (stCfg.bg_color>255)
    {
        FB1DBGERR("[FB1 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }

    if (stCfg.width>1920)
    {
        FB1DBGERR("[FB1 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }
    if (stCfg.height>1920)
    {
        FB1DBGERR("[FB1 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
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
            FB1DBGERR("[FB1 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
            return -EFAULT;
        }
    }

    FB1DBGMORE( "[FB1 Driver] Imgblit test, dst (x,y)=(%d,%d), width=%d, height=%d, colordepth=%d \n" ,stCfg.dx,stCfg.dy,stCfg.width,stCfg.height,stCfg.depth);

    sys_imageblit(info,&stCfg);
#endif

    return ret;
}

int _MDrv_FBIO_IOC_Set_Palette_GOP1(unsigned long arg)
{
    FB_GOP_PaletteEntry stCfg[256];

    if(copy_from_user(&stCfg, (FB_GOP_PaletteEntry __user *)arg, sizeof(FB_GOP_PaletteEntry)*256))
    {
        return -EFAULT;
    }

    mdrvinfinityfb1_infinity_SetPalette(stCfg,0,255);

    return 0;
}

int _MDrv_FBIO_IOC_Get_General_Config_GOP1(unsigned long arg)
{
    FB_GOP_RESOLUTION_STRETCH_H_CONFIG stCfg;

    if(copy_from_user(&stCfg, (FB_GOP_RESOLUTION_STRETCH_H_CONFIG __user *)arg, sizeof(FB_GOP_RESOLUTION_STRETCH_H_CONFIG)))
    {
        return -EFAULT;
    }

    stCfg.u32Height   = genGOPGernelSettings_gop1.u32Height;
    stCfg.u32Width    = genGOPGernelSettings_gop1.u32Width;
    stCfg.enBufferNum = genGOPGernelSettings_gop1.enBufferNum;
    stCfg.u32DisplayHeight = genGOPGernelSettings_gop1.u32DisplayHeight;
    stCfg.u32DisplayWidth  = genGOPGernelSettings_gop1.u32DisplayWidth;
    stCfg.enStretchH_Ratio = genGOPGernelSettings_gop1.enStretchH_Ratio;

    if(copy_to_user((FB_GOP_RESOLUTION_STRETCH_H_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_RESOLUTION_STRETCH_H_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_General_Config_GOP1(struct fb_info *info,unsigned long arg)
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

    FB1DBG( "[FB1 Driver] Set General_Config GOP_Width=%d, GOP_Height=%d, Display_Width=%d, Display_Height=%d, Buffer Num=%d, Stretch Ratio=%d\n",(int)stCfg.u32Width,(int)stCfg.u32Height,(int)stCfg.u32DisplayWidth,(int)stCfg.u32DisplayHeight,(int)stCfg.enBufferNum,(int)stCfg.enStretchH_Ratio);

    ///check variable valid/invalid
    DisplayPerPixel=MiuBusLen/(ANDROID_BYTES_PER_PIXEL*8);

    //single/double buffer check
    if((stCfg.enBufferNum!=FB_SINGLE_BUFFER)&&(stCfg.enBufferNum!=FB_DOUBLE_BUFFER))
    {
        FB1DBG( "[FB1 Driver] Error: Set_General_Config: buffer num should be 1 or 2 !\n ");
        return -EINVAL;
    }
    //stretch ratio check
    if((stCfg.enStretchH_Ratio!=FB_STRETCH_H_RATIO_1)&&(stCfg.enStretchH_Ratio!=FB_STRETCH_H_RATIO_2)&&(stCfg.enStretchH_Ratio!=FB_STRETCH_H_RATIO_4)&&(stCfg.enStretchH_Ratio!=FB_STRETCH_H_RATIO_8))
    {
        FB1DBG( "[FB1 Driver] Error: Set_General_Config: stretch H ratio should be 1, 2, 4 or 8 !\n ");
        return -EINVAL;
    }
    //x & y should > 0
    if((stCfg.u32Height<1)||(stCfg.u32Width<1)||(stCfg.u32DisplayHeight<1)||(stCfg.u32DisplayWidth<1))
    {
        FB1DBG( "[FB1 Driver] Error: Set_General_Config: buffer x or y size should be > 0 !\n ");
        return -EINVAL;
    }
    //x & y should < 1920
    if((stCfg.u32Height>1920)||(stCfg.u32Width>1920)||(stCfg.u32DisplayHeight>1920)||(stCfg.u32DisplayWidth>1920))
    {
        FB1DBG( "[FB1 Driver] Error: Set_General_Config: buffer x or y size should be < 1920 !\n ");
        return -EINVAL;
    }
    //max size for display is 1920*1088
    if(((stCfg.u32Height * stCfg.u32Width)>(max_x_res*max_y_res))||((stCfg.u32DisplayHeight* stCfg.u32DisplayWidth)>(max_x_res*max_y_res)))
    {
        FB1DBG( "[FB1 Driver] Error: Set_General_Config: total buffer size should be lower than 1920*1088!\n ");
        return -EINVAL;
    }
    //miu alignment
    if(((stCfg.u32Width % DisplayPerPixel)!=0)||((stCfg.u32DisplayWidth% DisplayPerPixel)!=0))
    {
        FB1DBG( "[FB1 Driver] Error: Set_General_Config: Display Per Pixel for Buffer Width should be %d!\n ",DisplayPerPixel);
        return -EINVAL;
    }
    //Stretch H parameter check
    if((stCfg.u32Width*stCfg.enStretchH_Ratio)!=stCfg.u32DisplayWidth)
    {
        FB1DBG( "[FB1 Driver] Error: Set_General_Config: DisplayWidth should = Width * StretchH_Ratio!\n ");
        return -EINVAL;
    }
    //Stretch V parameter check
    if(stCfg.u32Height != stCfg.u32DisplayHeight)
    {
        FB1DBG( "[FB1 Driver] Error: Set_General_Config: DisplayHeight should = Height!\n");
        return -EINVAL;
    }

    //after checking valid parameter, save to global variable
    genGOPGernelSettings_gop1.u32Width         = stCfg.u32Width;
    genGOPGernelSettings_gop1.u32Height        = stCfg.u32Height;
    genGOPGernelSettings_gop1.u32DisplayWidth  = stCfg.u32DisplayWidth;
    genGOPGernelSettings_gop1.u32DisplayHeight = stCfg.u32DisplayHeight;
    genGOPGernelSettings_gop1.enBufferNum      = stCfg.enBufferNum;
    genGOPGernelSettings_gop1.enStretchH_Ratio = stCfg.enStretchH_Ratio;

    info->var.xres=stCfg.u32Width;
    info->var.yres=stCfg.u32Height;

    ret=mdrvinfinityfb1_infinity_updateinfo_par(info->var.xres,info->var.yres);

    info->fix.smem_start=sg_mdrvinfinityfb1_fix.smem_start;
    info->fix.smem_len=sg_mdrvinfinityfb1_fix.smem_len;

    info->var.width=info->var.xres;
    info->var.height=info->var.yres;

    info->screen_base = (char *)CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1_gop1);
    info->fix.line_length      = info->var.xres * ANDROID_BYTES_PER_PIXEL;

    GOP1_Setmode_Stretch_H(FALSE, 0,sg_mdrvinfinityfb1_default_var.xres,sg_mdrvinfinityfb1_default_var.yres,sg_G3D_fb2_bus_addr1_gop1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0,(int)genGOPGernelSettings_gop1.enStretchH_Ratio);

    return ret;
}


int _MDrv_FBIO_IOC_Set_EnInvColor_GOP1(unsigned long arg)
{
    unsigned char enInvColor;
    int ret=0;

    if (sg_videomemorysize_gop1<1)
    {
        FB1DBGERR("[FB1 Driver] ERROR: _MDrv_FBIO_IOC_Set_EnInvColor, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&enInvColor, (unsigned char __user *)arg, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    FB1DBG( "[FB1 Driver] enable inverse color=%d \n",enInvColor);

    // record in global variable
    genInv_Color_gop1=enInvColor;

    GOP1_invColor_Enable(enInvColor);

    //check whether do sw update inverse color
    mdrvinfinityfb1_checkInvTable_UpdateMode();

    if(enInvColor==1)
    {
        GOP1_invColor_Debug_Mode();

        if(pSWupdateInvDebugThread_gop1==NULL)
        {
            pSWupdateInvDebugThread_gop1 = kthread_create(mdrvinfinityfb1_SWupdate_InvTable_debugmode,(void *)&pSWupdateInvDebugThread_gop1,"GOP1_debug_SWupdate_InvColor");
            if (IS_ERR(pSWupdateInvDebugThread_gop1))
            {
                ret=PTR_ERR(pSWupdateInvDebugThread_gop1);
                pSWupdateInvDebugThread_gop1 = NULL;
                FB1DBGERR("[FB1 Driver] ERROR: debug mode! Create thread fail!%d\n",ret);
            }
            else
            {
                wake_up_process(pSWupdateInvDebugThread_gop1);
            }
        }
    }
    else if((enInvColor==0)&&(pSWupdateInvDebugThread_gop1!=NULL))
    {
        kthread_stop(pSWupdateInvDebugThread_gop1);
        pSWupdateInvDebugThread_gop1 = NULL;
    }

    return 0;
}

int _MDrv_FBIO_IOC_Set_AEinfoConfig_GOP1(unsigned long arg)
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

    FB1DBG( "[FB1 Driver] set AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d\n",
    (int)stAECfg.u32AEDisplayWidth,(int)stAECfg.u32AEDisplayHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight);

    // record in global variable
    genAE_info_gop1.u32AEDisplayWidth  = stAECfg.u32AEDisplayWidth;
    genAE_info_gop1.u32AEDisplayHeight = stAECfg.u32AEDisplayHeight;
    genAE_info_gop1.u32AEBlkWidth      = stAECfg.u32AEBlkWidth;
    genAE_info_gop1.u32AEBlkHeight     = stAECfg.u32AEBlkHeight;

    // when scaling happens, change AE block size with this scale
    if(genSC_info_gop1.bScalingEn)
    {
        if((genAE_info_gop1.u32AEBlkWidth!=0)&&(genAE_info_gop1.u32AEBlkHeight!=0)&&(genAE_info_gop1.u32AEDisplayWidth!=0)&&(genAE_info_gop1.u32AEDisplayHeight!=0))
        {
            if(genSC_info_gop1.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info_gop1.u32CropYend-genSC_info_gop1.u32CropYstart;
                    tempcorpAEheight=genSC_info_gop1.u32CropXend-genSC_info_gop1.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info_gop1.u32CropXend-genSC_info_gop1.u32CropXstart;
                    tempcorpAEheight=genSC_info_gop1.u32CropYend-genSC_info_gop1.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerHeight/(int)genAE_info_gop1.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerWidth/(int)genAE_info_gop1.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerWidth/(int)genAE_info_gop1.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerHeight/(int)genAE_info_gop1.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FB1DBGERR( "[FB1 Driver] Error: _MDrv_FBIO_IOC_Set_AEinfoConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP1_invColor_Set_AE_Config((int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEDisplayHeight);
        }
        else
        {
            GOP1_invColor_Set_AE_Config((int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEDisplayWidth);
        }
    }

    if(genSC_info_gop1.bCropEn)//if crop enable, reset crop information when AE info change
    {
        if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP1_invColor_Set_Crop_Config((int)genSC_info_gop1.u32CropXstart,(int)genSC_info_gop1.u32CropYstart,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEDisplayHeight);
        }
        else
        {
            GOP1_invColor_Set_Crop_Config((int)genSC_info_gop1.u32CropXstart,(int)genSC_info_gop1.u32CropYstart,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEDisplayWidth);
        }
    }

    return 0;
}

int _MDrv_FBIO_IOC_Set_YThres_GOP1(unsigned long arg)
{
    unsigned long YThres;

    if(copy_from_user(&YThres, (unsigned long __user *)arg, sizeof(unsigned long)))
    {
        return -EFAULT;
    }

    FB1DBG( "[FB1 Driver] set Y threshold=%d \n",(int)YThres);

    // record in global variable
    genY_Thres_gop1=YThres;

    GOP1_invColor_Set_Y_Threshold((int)YThres);

    return 0;
}

int _MDrv_FBIO_IOC_Set_ScalerinfoConfig_GOP1(unsigned long arg)
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

    FB1DBG( "[FB1 Driver] set Scl config, CropEn=%d, ScalingEn=%d, RotateEn=%d\n",
    (int)stSclCfg.bCropEn,(int)stSclCfg.bScalingEn,(int)stSclCfg.bRotateEn);

    if((stSclCfg.u32ScalerWidth<1)||(stSclCfg.u32ScalerWidth>1920))
    {
        FB1DBGERR( "[FB1 Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! parameter wrong\n");
        return -EFAULT;
    }
    if((stSclCfg.u32ScalerHeight<1)||(stSclCfg.u32ScalerHeight>1920))
    {
        FB1DBGERR( "[FB1 Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! parameter wrong\n");
        return -EFAULT;
    }

    // record in global variable
    genSC_info_gop1.bCropEn        = stSclCfg.bCropEn;
    genSC_info_gop1.bScalingEn     = stSclCfg.bScalingEn;
    genSC_info_gop1.u32ScalerWidth = stSclCfg.u32ScalerWidth;
    genSC_info_gop1.u32ScalerHeight= stSclCfg.u32ScalerHeight;
    genSC_info_gop1.u32CropXstart  = stSclCfg.u32CropXstart;
    genSC_info_gop1.u32CropXend    = stSclCfg.u32CropXend;
    genSC_info_gop1.u32CropYstart  = stSclCfg.u32CropYstart;
    genSC_info_gop1.u32CropYend    = stSclCfg.u32CropYend;
    // rotate will be get from ISP driver
    //genSC_info_gop1.bRotateEn      = stSclCfg.bRotateEn;
    //genSC_info_gop1.enRotateA      = stSclCfg.enRotateA;

    if(stSclCfg.bCropEn)
    {
        if((genAE_info_gop1.u32AEBlkWidth!=0)&&(genAE_info_gop1.u32AEBlkHeight!=0)&&(genAE_info_gop1.u32AEDisplayWidth!=0)&&(genAE_info_gop1.u32AEDisplayHeight!=0))
        {
            if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
            {
                GOP1_invColor_Set_Crop_Config((int)stSclCfg.u32CropXstart,(int)stSclCfg.u32CropYstart,(int)genAE_info_gop1.u32AEBlkHeight,(int)genAE_info_gop1.u32AEBlkWidth,(int)genAE_info_gop1.u32AEDisplayHeight);
            }
            else
            {
                GOP1_invColor_Set_Crop_Config((int)stSclCfg.u32CropXstart,(int)stSclCfg.u32CropYstart,(int)genAE_info_gop1.u32AEBlkWidth,(int)genAE_info_gop1.u32AEBlkHeight,(int)genAE_info_gop1.u32AEDisplayWidth);
            }
        }
        else
        {
            FB1DBGERR( "[FB1 Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! No AE info\n");
            return -EFAULT;
        }
    }

// when scaling happens, change AE block size with this scale
    if(stSclCfg.bScalingEn)
    {
        if((genAE_info_gop1.u32AEBlkWidth!=0)&&(genAE_info_gop1.u32AEBlkHeight!=0)&&(genAE_info_gop1.u32AEDisplayWidth!=0)&&(genAE_info_gop1.u32AEDisplayHeight!=0))
        {
            if(stSclCfg.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info_gop1.u32CropYend-genSC_info_gop1.u32CropYstart;
                    tempcorpAEheight=genSC_info_gop1.u32CropXend-genSC_info_gop1.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info_gop1.u32AEDisplayWidth/(int)genAE_info_gop1.u32AEBlkWidth);
                    if(tempAEblkheight > 0)
                    {
                        while(((genSC_info_gop1.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkheight++;
                            if(tempAEblkheight>genSC_info_gop1.u32ScalerWidth)
                            {
                                tempAEblkheight=genSC_info_gop1.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info_gop1.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkwidth++;
                        }
                    }
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info_gop1.u32CropXend-genSC_info_gop1.u32CropXstart;
                    tempcorpAEheight=genSC_info_gop1.u32CropYend-genSC_info_gop1.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayWidth/(int)genAE_info_gop1.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    FB1DBGMORE("[FB1 Driver] crop:x_start=%d, x_end=%d\n",genSC_info_gop1.u32CropXstart,genSC_info_gop1.u32CropXend);
                    FB1DBGMORE("[FB1 Driver] AE:D_width=%d, Blk_width=%d\n",genAE_info_gop1.u32AEDisplayWidth,genAE_info_gop1.u32AEBlkWidth);
                    FB1DBGMORE("[FB1 Driver] tempcorpAEwidth=%d, tempcorpAEheight=%d\n",tempcorpAEwidth,tempcorpAEheight);
                    FB1DBGMORE("[FB1 Driver] before tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    if(tempAEblkwidth > 0)
                    {
                        while(((genSC_info_gop1.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkwidth++;
                            if(tempAEblkwidth>genSC_info_gop1.u32ScalerWidth)
                            {
                                tempAEblkwidth=genSC_info_gop1.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info_gop1.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkheight++;
                        }
                    }
                    FB1DBGMORE("[FB1 Driver] after tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerHeight/(int)genAE_info_gop1.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerWidth/(int)genAE_info_gop1.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info_gop1.u32AEDisplayWidth/(int)genAE_info_gop1.u32AEBlkWidth);
                    if(tempAEblkheight > 0)
                    {
                        while(((genSC_info_gop1.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkheight++;
                            if(tempAEblkheight>genSC_info_gop1.u32ScalerWidth)
                            {
                                tempAEblkheight=genSC_info_gop1.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info_gop1.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkwidth++;
                        }
                    }
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerWidth/genAE_info_gop1.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerHeight/genAE_info_gop1.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayWidth/(int)genAE_info_gop1.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    FB1DBGMORE("[FB1 Driver] crop:x_start=%d, x_end=%d\n",genSC_info_gop1.u32CropXstart,genSC_info_gop1.u32CropXend);
                    FB1DBGMORE("[FB1 Driver] AE:D_width=%d, Blk_width=%d\n",genAE_info_gop1.u32AEDisplayWidth,genAE_info_gop1.u32AEBlkWidth);
                    FB1DBGMORE("[FB1 Driver] before tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    if(tempAEblkwidth > 0)
                    {
                        while(((genSC_info_gop1.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkwidth++;
                            if(tempAEblkwidth>genSC_info_gop1.u32ScalerWidth)
                            {
                                tempAEblkwidth=genSC_info_gop1.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info_gop1.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkheight++;
                        }
                    }
                    FB1DBGMORE("[FB1 Driver] after tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FB1DBGERR( "[FB1 Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP1_invColor_Set_AE_Config((int)genAE_info_gop1.u32AEBlkHeight,(int)genAE_info_gop1.u32AEBlkWidth,(int)genAE_info_gop1.u32AEDisplayHeight);
        }
        else
        {
            GOP1_invColor_Set_AE_Config((int)genAE_info_gop1.u32AEBlkWidth,(int)genAE_info_gop1.u32AEBlkHeight,(int)genAE_info_gop1.u32AEDisplayWidth);
        }
    }

    return 0;
}

int _MDrv_FBIO_IOC_AutoAEConfig_GOP1(unsigned long arg)
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
        FB1DBGERR( "[FB1 Driver] Error: auto AE config fail!!! No proper AE value\n");
        FB1DBGERR( "[FB1 Driver] auto AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d, rotate en=%d\n",
        (int)stGOPAEConfig.u32AEDisplayWidth,(int)stGOPAEConfig.u32AEDisplayHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,genSC_info_gop1.bRotateEn);
        return -EFAULT;
    }

    stGOPAEConfig.u32AEBlkWidth= (unsigned long)stISPAEconfig.blk_w;
    stGOPAEConfig.u32AEBlkHeight= (unsigned long)stISPAEconfig.blk_h;
    stGOPAEConfig.u32AEDisplayWidth= (unsigned long)stISPAEconfig.img_w;
    stGOPAEConfig.u32AEDisplayHeight= (unsigned long)stISPAEconfig.img_h;

    // record in global variable
    genAE_info_gop1.u32AEDisplayWidth  = stGOPAEConfig.u32AEDisplayWidth;
    genAE_info_gop1.u32AEDisplayHeight = stGOPAEConfig.u32AEDisplayHeight;
    genAE_info_gop1.u32AEBlkWidth      = stGOPAEConfig.u32AEBlkWidth;
    genAE_info_gop1.u32AEBlkHeight     = stGOPAEConfig.u32AEBlkHeight;
    if(stISPAEconfig.rot==1)
    {
        genSC_info_gop1.bRotateEn=1;
        InvTblupdateMode=1;//by cpu
    }
    else
    {
        genSC_info_gop1.bRotateEn=0;
        InvTblupdateMode=0;//by hw engine
    }

    GOP1_invColor_Set_UpdateMode(InvTblupdateMode);

    FB1DBG( "[FB1 Driver] auto AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d, rotate en=%d\n",
    (int)stGOPAEConfig.u32AEDisplayWidth,(int)stGOPAEConfig.u32AEDisplayHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,genSC_info_gop1.bRotateEn);

    // when scaling happens, change AE block size with this scale
    if(genSC_info_gop1.bScalingEn)
    {
        if((genAE_info_gop1.u32AEBlkWidth!=0)&&(genAE_info_gop1.u32AEBlkHeight!=0)&&(genAE_info_gop1.u32AEDisplayWidth!=0)&&(genAE_info_gop1.u32AEDisplayHeight!=0))
        {
            if(genSC_info_gop1.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info_gop1.u32CropYend-genSC_info_gop1.u32CropYstart;
                    tempcorpAEheight=genSC_info_gop1.u32CropXend-genSC_info_gop1.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info_gop1.u32AEDisplayWidth/(int)genAE_info_gop1.u32AEBlkWidth);
                    while(((genSC_info_gop1.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkheight++;
                        if(tempAEblkheight>genSC_info_gop1.u32ScalerWidth)
                        {
                            tempAEblkheight=genSC_info_gop1.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info_gop1.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkwidth++;
                    }
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info_gop1.u32CropXend-genSC_info_gop1.u32CropXstart;
                    tempcorpAEheight=genSC_info_gop1.u32CropYend-genSC_info_gop1.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayWidth/(int)genAE_info_gop1.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    while(((genSC_info_gop1.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkwidth++;
                        if(tempAEblkwidth>genSC_info_gop1.u32ScalerWidth)
                        {
                            tempAEblkwidth=genSC_info_gop1.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info_gop1.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkheight++;
                    }
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerHeight/(int)genAE_info_gop1.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerWidth/(int)genAE_info_gop1.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info_gop1.u32AEDisplayWidth/(int)genAE_info_gop1.u32AEBlkWidth);
                    while(((genSC_info_gop1.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkheight++;
                        if(tempAEblkheight>genSC_info_gop1.u32ScalerWidth)
                        {
                            tempAEblkheight=genSC_info_gop1.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info_gop1.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkwidth++;
                    }
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info_gop1.u32AEBlkWidth*(int)genSC_info_gop1.u32ScalerWidth/genAE_info_gop1.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop1.u32AEBlkHeight*(int)genSC_info_gop1.u32ScalerHeight/genAE_info_gop1.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop1.u32AEDisplayWidth/(int)genAE_info_gop1.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info_gop1.u32AEDisplayHeight/(int)genAE_info_gop1.u32AEBlkHeight);
                    while(((genSC_info_gop1.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkwidth++;
                        if(tempAEblkwidth>genSC_info_gop1.u32ScalerWidth)
                        {
                            tempAEblkwidth=genSC_info_gop1.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info_gop1.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkheight++;
                    }
                    GOP1_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FB1DBGERR( "[FB1 Driver] Error: _MDrv_FBIO_IOC_AutoAEConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP1_invColor_Set_AE_Config((int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEDisplayHeight);
        }
        else
        {
            GOP1_invColor_Set_AE_Config((int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEDisplayWidth);
        }
    }

    if(genSC_info_gop1.bCropEn)//if crop enable, reset crop information when AE info change
    {
        if(genSC_info_gop1.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP1_invColor_Set_Crop_Config((int)genSC_info_gop1.u32CropXstart,(int)genSC_info_gop1.u32CropYstart,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEDisplayHeight);
        }
        else
        {
            GOP1_invColor_Set_Crop_Config((int)genSC_info_gop1.u32CropXstart,(int)genSC_info_gop1.u32CropYstart,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEDisplayWidth);
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
| FUNCTION    : mdrvinfinityfb1_ioctl
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
mdrvinfinityfb1_ioctl(struct fb_info *pinfo, unsigned int cmd, unsigned long arg)
{
    int err=0, ret=0;
    unsigned int ret_Phy=0;

    if (NULL == pinfo)
    {
        FB1DBGERR("[FB1 Driver] ERROR:in mdrvinfinityfb1_ioctl, pinfo is NULL pointer\r\n");
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
            ret_Phy = sg_G3D_fb2_bus_addr1_gop1;
            FB1DBG( "[FB1 Driver] Phy Addr =%x\r\n",ret_Phy);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;

        case IOCTL_FB_GETVIDEOMEMSIZE:
            ret_Phy = sg_videomemorysize_gop1;
            FB1DBG( "[FB1 Driver] Kernel VideoMemSize =0x%x \n" ,(unsigned int)sg_videomemorysize_gop1);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;

        case IOCTL_FB_GETFBSUPPORTINF:
            FB1DBG( "[FB1 Driver] Get Framebuffer Support Information.\n");
            ret=_MDrv_FBIO_IOC_Get_SUPINFO_GOP1(arg);
            break;

        case IOCTL_FB_GETGWININFO:
            FB1DBG( "[FB1 Driver] Get Framebuffer GWIN Information.\n");
            ret=_MDrv_FBIO_IOC_Get_GWIN_GOP1(arg);
            break;

        case IOCTL_FB_SETGWININFO:
            FB1DBG( "[FB1 Driver] Set Framebuffer GWIN Information.\n");
            ret=_MDrv_FBIO_IOC_Set_GWIN_GOP1(arg);
            break;

        case IOCTL_FB_GETENABLEGOP:
            FB1DBG( "[FB1 Driver] Get GOP Enable/Disable.\n");
            ret=_MDrv_FBIO_IOC_Get_EnGOP_GOP1(arg);
            break;

        case IOCTL_FB_SETENABLEGOP:
            FB1DBG( "[FB1 Driver] Set GOP Enable/Disable.\n");
            ret=_MDrv_FBIO_IOC_Set_EnGOP_GOP1(arg);
            break;

        case IOCTL_FB_GETALPHA:
            FB1DBG( "[FB1 Driver] Get Alpha Information.\n");
            ret=_MDrv_FBIO_IOC_Get_Alpha_GOP1(arg);
            break;

        case IOCTL_FB_SETALPHA:
            FB1DBG( "[FB1 Driver] Set Alpha Information.\n");
            ret=_MDrv_FBIO_IOC_Set_Constant_Alpha_GOP1(arg);
            break;

        case IOCTL_FB_GETCOLORKEY:
            FB1DBG( "[FB1 Driver] Get Colorkey Information.\n");
            ret=_MDrv_FBIO_IOC_Get_Color_Key_GOP1(arg);
            break;

        case IOCTL_FB_SETCOLORKEY:
            FB1DBG( "[FB1 Driver] Set Colorkey Information.\n");
            ret=_MDrv_FBIO_IOC_Set_Color_Key_GOP1(arg);
            break;

        case IOCTL_FB_IMAGEBLIT:
            FB1DBG( "[FB1 Driver] Use Imageblit.\n");
            ret=_MDrv_FBIO_IOC_imageblit_GOP1(pinfo,arg);
            break;

        case IOCTL_FB_SETPALETTE:
            FB1DBG( "[FB1 Driver] Set Palette.\n");
            ret=_MDrv_FBIO_IOC_Set_Palette_GOP1(arg);
            break;

        case IOCTL_FB_GETGENERALCONFIG:
            FB1DBG( "[FB1 Driver] Get General Config.\n");
            ret=_MDrv_FBIO_IOC_Get_General_Config_GOP1(arg);
            break;

        case IOCTL_FB_SETGENERALCONFIG:
            FB1DBG( "[FB1 Driver] Set General Config.\n");
            ret=_MDrv_FBIO_IOC_Set_General_Config_GOP1(pinfo,arg);
            break;

        case IOCTL_FB_SETENABLEINVCOLOR:
            FB1DBG( "[FB1 Driver] Set Enable/Disable inverse color.\n");
            ret=_MDrv_FBIO_IOC_Set_EnInvColor_GOP1(arg);
            break;

        case IOCTL_FB_SETAECONFIG:
            FB1DBG( "[FB1 Driver] Set AE configurations.\n");
            ret=_MDrv_FBIO_IOC_Set_AEinfoConfig_GOP1(arg);
            break;

        case IOCTL_FB_SETYTHRES:
            FB1DBG( "[FB1 Driver] Set Y threshold.\n");
            ret=_MDrv_FBIO_IOC_Set_YThres_GOP1(arg);
            break;

        case IOCTL_FB_SETSCALERCONFIG:
            FB1DBG( "[FB1 Driver] Set Scl configurations.\n");
            ret=_MDrv_FBIO_IOC_Set_ScalerinfoConfig_GOP1(arg);
            break;

        case IOCTL_FB_AUTOUPDATEAE:
            FB1DBG( "[FB1 Driver] auto AE.\n");
            ret=_MDrv_FBIO_IOC_AutoAEConfig_GOP1(arg);
            break;

        default:  /* redundant, as cmd was checked against MAXNR */
            FB1DBGERR("[FB1 Driver] in default ioct\r\n");
            return -ENOTTY;
    }
    return ret;
}

static ssize_t gop1info_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t n)
{
    return 0;
}

static ssize_t gop1info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "[FB1/GOP1 Driver Information]\n");
    str += scnprintf(str, end - str, "[Framebuffer1]\n");
    str += scnprintf(str, end - str, " Width          : %4d pixel\n",sg_mdrvinfinityfb1_default_var.xres);
    str += scnprintf(str, end - str, " Height         : %4d pixel\n",sg_mdrvinfinityfb1_default_var.yres);
    str += scnprintf(str, end - str, " BPP            : %4d bytes per pixel\n",ANDROID_BYTES_PER_PIXEL);
    str += scnprintf(str, end - str, " Memory Size    : %d bytes\n",(unsigned int)sg_videomemorysize_gop1);
    str += scnprintf(str, end - str, " Memory Phy_Addr: 0x%x\n",sg_G3D_fb2_bus_addr1_gop1);
    str += scnprintf(str, end - str, " Memory Vir_Addr: 0x%x\n",(unsigned int)sg_pG3D_fb2_vir_addr1_gop1);
    str += scnprintf(str, end - str, "[GOP1(GWIN)]\n");
    if(genGWIN_gop1==0)
        str += scnprintf(str, end - str, " GWIN Status    : Close\n");
    else if(genGWIN_gop1==1)
        str += scnprintf(str, end - str, " GWIN Status    : Open\n");
    else
        str += scnprintf(str, end - str, " GWIN Status    : Unknown\n");

    str += scnprintf(str, end - str, " H Start        : %4d\n",genGWinInfo_gop1.u18HStart);
    str += scnprintf(str, end - str, " H End          : %4d\n",genGWinInfo_gop1.u18HEnd);
    str += scnprintf(str, end - str, " V Start        : %4d\n",genGWinInfo_gop1.u18VStart);
    str += scnprintf(str, end - str, " V End          : %4d\n",genGWinInfo_gop1.u18VEnd);

    if(genGOPALPHA_gop1.bEn==0)
        str += scnprintf(str, end - str, " Alpha Status   : Close\n");
    else if(genGOPALPHA_gop1.bEn==1)
        str += scnprintf(str, end - str, " Alpha Status   : Open\n");
    else
        str += scnprintf(str, end - str, " Alpha Status   : Unknown\n");


    if(genGOPALPHA_gop1.enAlphaType==PIXEL_ALPHA)
        str += scnprintf(str, end - str, " Alpha Type     : Pixel Alpha\n");
    else if(genGOPALPHA_gop1.enAlphaType==CONST_ALPHA)
    {
        str += scnprintf(str, end - str, " Alpha Type     : Constant Alpha\n");
        str += scnprintf(str, end - str, " Alpha Value    : 0x%2x\n",genGOPALPHA_gop1.u8Alpha);
    }
    else
        str += scnprintf(str, end - str, " Alpha Type     : wrong alpha type\n");

    if(genGOPCOLORKEY_gop1.bEn==0)
        str += scnprintf(str, end - str, " Colorkey Status: Close\n");
    else if(genGOPCOLORKEY_gop1.bEn==1)
    {
        str += scnprintf(str, end - str, " Colorkey Status: Open\n");
        str += scnprintf(str, end - str, " Red            : 0x%2x\n",genGOPCOLORKEY_gop1.u8R);
        str += scnprintf(str, end - str, " Green          : 0x%2x\n",genGOPCOLORKEY_gop1.u8G);
        str += scnprintf(str, end - str, " Blue           : 0x%2x\n",genGOPCOLORKEY_gop1.u8B);
    }
    else
        str += scnprintf(str, end - str, " Colorkey Status: Unknown\n");


    if (str > buf)  str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);

}

DEVICE_ATTR(gop1info, 0644, gop1info_show, gop1info_store);


static int mdrvinfinityfb1_suspend(struct platform_device *pdev,
                 pm_message_t state)
{
    int clockIdxtemp=0;
    FB1DBG("[FB1 Driver] in mdrvinfinityfb1_suspend \r\n");

    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks_gop1 ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            clk_disable_unprepare(gop1_clks[clockIdxtemp]);
        }
    }
    //clk_disable_unprepare(gop_psram_clk);
    return 0;
}

static int mdrvinfinityfb1_resume(struct platform_device *pdev)
{
    //dma_addr_t fb2_PHY_ADD_SHOW=0;
    int ret=-1;
    int clockIdxtemp=0;

    // open all needed clocks
    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks_gop1 ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            if (IS_ERR(gop1_clks[clockIdxtemp]))
            {
                FB1DBGERR( "[FB1 Driver] ERROR: Fail to get gop clk! Clock idx=%d\n",clockIdxtemp);
                kfree(gop1_clks);
                return ret;
            }
            else
            {
                clk_prepare_enable(gop1_clks[clockIdxtemp]);
            }
        }
    }

    FB1DBG("[FB1 Driver] mdrvinfinityfb1_resume \r\n");

    GOP1_Setmode(TRUE, 0,sg_mdrvinfinityfb1_default_var.xres,sg_mdrvinfinityfb1_default_var.yres,sg_G3D_fb2_bus_addr1_gop1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    //set palette
    mdrvinfinityfb1_infinity_SetPalette(stGOPCurrentPalInfo_GOP1,0,255);

    return 0;
}
/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb1_probe
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
static int mdrvinfinityfb1_probe(struct platform_device *dev)
{
    struct fb_info *pinfo = NULL;
    int retval = -ENOMEM;
    int ret=-1;
    int clockIdxtemp=0;

    if (NULL == dev)
    {
        FB1DBGERR("[FB1 Driver] ERROR: in mdrvinfinityfb1_prob: dev is NULL pointer \r\n");
        return -ENOTTY;
    }

    // open all needed clocks
    num_parents_clocks_gop1 = of_clk_get_parent_count(dev->dev.of_node);
    gop1_clks=kzalloc(((sizeof(struct clk *) * num_parents_clocks_gop1)),GFP_KERNEL);

    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks_gop1 ; clockIdxtemp++)
    {
        gop1_clks[clockIdxtemp]= of_clk_get(dev->dev.of_node, clockIdxtemp);
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            if (IS_ERR(gop1_clks[clockIdxtemp]))
            {
                FB1DBGERR( "[FB1 Driver] ERROR: Fail to get gop clk! Clock idx=%d\n",clockIdxtemp);
                kfree(gop1_clks);
                return ret;
            }
            else
            {
                clk_prepare_enable(gop1_clks[clockIdxtemp]);
            }
        }
    }

    // init global variable
    mdrvinfinityfb1_updateinfo();

    pinfo = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
    if (!pinfo)
        goto err;

    // copy data for register framebuffer
    pinfo->screen_base = (char *)CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1_gop1);
    pinfo->fbops = &sg_mdrvinfinityfb1_ops;
    pinfo->var = sg_mdrvinfinityfb1_default_var;
    pinfo->fix = sg_mdrvinfinityfb1_fix;
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
    GOP1_Setmode(FALSE, 0,sg_mdrvinfinityfb1_default_var.xres,sg_mdrvinfinityfb1_default_var.yres,sg_G3D_fb2_bus_addr1_gop1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    // set gop inverse color init settings
    GOP1_invColor_init();

    // set init palette
    mdrvinfinityfb1_infinity_SetPalette(DefaultPaletteEntry,0,255);

    // register sysfs
    ret = device_create_file(&dev->dev, &dev_attr_gop1info);
    if (ret != 0)
    {
      dev_err(&dev->dev,
       "[FB1 Driver] Failed to create ptgen_call sysfs files: %d\n", ret);
    }
    else
    {
        FB1DBGMORE("[FB1 Driver] device attr ok\n");
    }

    return 0;

err2:
    fb_dealloc_cmap(&pinfo->cmap);
err1:
    framebuffer_release(pinfo);
err:

    free_dmem(KEY_DMEM_FB1_BUF,
                      PAGE_ALIGN(sg_videomemorysize_gop1),
                      sg_pG3D_fb2_vir_addr1_gop1,
                      sg_G3D_fb2_bus_addr1_gop1);

    return retval;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb1_remove
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
static int mdrvinfinityfb1_remove(struct platform_device *dev)
{
    struct fb_info *pinfo;
    int clockIdxtemp=0;
    pinfo = 0;

    if (NULL == dev)
    {
        FB1DBGERR("[FB1 Driver] ERROR: mdrvinfinityfb1_remove: dev is NULL pointer \n");
        return -ENOTTY;
    }

    pinfo = platform_get_drvdata(dev);
    if (pinfo)
    {
        unregister_framebuffer(pinfo);

        free_dmem(KEY_DMEM_FB1_BUF,
                          PAGE_ALIGN(sg_videomemorysize_gop1),
                          sg_pG3D_fb2_vir_addr1_gop1,
                          sg_G3D_fb2_bus_addr1_gop1);

        framebuffer_release(pinfo);
    }

    // close all clocks
    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks_gop1 ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            clk_disable_unprepare(gop1_clks[clockIdxtemp]);
        }
    }
    return 0;
}

static int mdrvinfinityfb1_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    dma_addr_t  fb2_pandisplay_PHY_ADD_SHOW=0;

    FB1DBGMORE( "[FB1 Driver]%s: var->yoffset=%x! info->var.height=%x\n ",__func__,var->yoffset,info->var.height);

    if(genGOPGernelSettings_gop1.enBufferNum==FB_DOUBLE_BUFFER)//double buffer
    {
        if (var->yoffset >= info->var.height)
        {
            fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start + (info->var.width * info->var.height * ANDROID_BYTES_PER_PIXEL);

            FB1DBGMORE( "[FB1 Driver]%s: use buffer 1! Addr=%x\n ",__func__,fb2_pandisplay_PHY_ADD_SHOW);
        }
        else
        {
            fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start;

            FB1DBGMORE( "[FB1 Driver]%s: use buffer 0 Addr=%x!\n ",__func__,fb2_pandisplay_PHY_ADD_SHOW);
        }
    }
    else
    {
        fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start;

        FB1DBGMORE( "mdrvinfinityfb_pan_display: use buffer 0 Addr=%x!\n ",fb2_pandisplay_PHY_ADD_SHOW);
    }

    GOP1_Pan_Display(0, fb2_pandisplay_PHY_ADD_SHOW);

    return 0;
}


//check var to see if supported by this device, defined for infinity
static int mdrvinfinityfb1_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    int MiuBusLen=128;//miu bus length in iNfinity
    int DisplayPerPixel=0;
    int max_x_res=2560;
    int max_y_res=2048;

    DisplayPerPixel=MiuBusLen/(ANDROID_BYTES_PER_PIXEL*8);

    //MstarFB_ENTER();

    if((var->yres<1)||(var->xres<1))
    {
        FB1DBGERR( "[FB1 Driver] Error: fb_check_var: buffer x or y size should be > 0 !\n ");
        return -EINVAL;
    }
    if((var->yres * var->xres)>(max_x_res*max_y_res))
    {
        FB1DBGERR( "[FB1 Driver] Error: fb_check_var: total buffer size should be lower than 1920*1088!\n ");
        return -EINVAL;
    }

    if((var->xres % DisplayPerPixel)!=0)
    {
        FB1DBGERR( "[FB1 Driver] Error: fb_check_var: Display Per Pixel for Buffer Width should be %d!\n ",DisplayPerPixel);
        return -EINVAL;
    }

    //MstarFB_LEAVE();

    return 0;
}

module_platform_driver(sg_mdrvinfinityfb1_driver);

MODULE_LICENSE("GPL");
