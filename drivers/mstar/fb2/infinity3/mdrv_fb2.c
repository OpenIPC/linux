

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

#include "mdrv_fb2.h"

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
static void __init mdrvinfinityfb2_updateinfo(void)
{
    static int main_videomemory_size=0;

    //default settings for global structure of fb_var_screeninfo
    sg_mdrvinfinityfb2_default_var.xres         = 0;
    sg_mdrvinfinityfb2_default_var.yres         = 0;
    sg_mdrvinfinityfb2_default_var.xres_virtual = sg_mdrvinfinityfb2_default_var.xres;
    sg_mdrvinfinityfb2_default_var.yres_virtual = sg_mdrvinfinityfb2_default_var.yres * (ANDROID_NUMBER_OF_BUFFERS+1);//double buffer for pan display
    sg_mdrvinfinityfb2_default_var.width        = sg_mdrvinfinityfb2_default_var.xres;
    sg_mdrvinfinityfb2_default_var.height       = sg_mdrvinfinityfb2_default_var.yres;

    //GWin init, same size as screen
    genGWinInfo_gop2.u18HStart = 0;
    genGWinInfo_gop2.u18HEnd   = genGWinInfo_gop2.u18HStart + sg_mdrvinfinityfb2_default_var.xres;
    genGWinInfo_gop2.u18VStart = 0;
    genGWinInfo_gop2.u18VEnd   = genGWinInfo_gop2.u18VStart + sg_mdrvinfinityfb2_default_var.yres;
    main_videomemory_size = (sg_mdrvinfinityfb2_default_var.xres * sg_mdrvinfinityfb2_default_var.yres * ANDROID_BYTES_PER_PIXEL * (ANDROID_NUMBER_OF_BUFFERS+1));


    sg_videomemorysize_gop2 = main_videomemory_size;

    //default settings for global structure of fb_fix_screeninfo
    sg_mdrvinfinityfb2_fix.smem_start  = sg_G3D_fb2_bus_addr1_gop2;
    sg_mdrvinfinityfb2_fix.smem_len    = sg_videomemorysize_gop2;
    sg_mdrvinfinityfb2_fix.line_length = sg_mdrvinfinityfb2_default_var.xres * ANDROID_BYTES_PER_PIXEL;
    sg_mdrvinfinityfb2_fix.ypanstep    = 1;

    FB2DBGMORE("[FB2 Driver] updateinfo: GOP_WIDTH = [%d], GOP_HEIGHT = [%d], GOPmemorysize = 0x%x \r\n",sg_mdrvinfinityfb2_default_var.xres,sg_mdrvinfinityfb2_default_var.yres,(unsigned int)sg_videomemorysize_gop2);

}

// update global variables while setting new resolution by ioctl FBIOPUT_VSCREENINFO
static int mdrvinfinityfb2_infinity_updateinfo_par(unsigned int setWidth,unsigned int setHeight)
{

    static int main_videomemory_size=0;

    //update settings for global structure of fb_var_screeninfo
    sg_mdrvinfinityfb2_default_var.xres         = setWidth;
    sg_mdrvinfinityfb2_default_var.yres         = setHeight;
    sg_mdrvinfinityfb2_default_var.xres_virtual = sg_mdrvinfinityfb2_default_var.xres;
    sg_mdrvinfinityfb2_default_var.yres_virtual = sg_mdrvinfinityfb2_default_var.yres * (ANDROID_NUMBER_OF_BUFFERS+1);//double buffer for pan display
    sg_mdrvinfinityfb2_default_var.width        = sg_mdrvinfinityfb2_default_var.xres;
    sg_mdrvinfinityfb2_default_var.height       = sg_mdrvinfinityfb2_default_var.yres;

    //GWin init, same size as screen
    genGWinInfo_gop2.u18HStart = 0;
    genGWinInfo_gop2.u18HEnd   = genGWinInfo_gop2.u18HStart + sg_mdrvinfinityfb2_default_var.xres;
    genGWinInfo_gop2.u18VStart = 0;
    genGWinInfo_gop2.u18VEnd   = genGWinInfo_gop2.u18VStart + sg_mdrvinfinityfb2_default_var.yres;
    main_videomemory_size = (sg_mdrvinfinityfb2_default_var.xres * sg_mdrvinfinityfb2_default_var.yres * ANDROID_BYTES_PER_PIXEL * (ANDROID_NUMBER_OF_BUFFERS+1));

    // free memory if it has
    if(sg_videomemorysize_gop2 > 0)
    {
        free_dmem(KEY_DMEM_FB2_BUF,
                  PAGE_ALIGN(sg_videomemorysize_gop2),
                  sg_pG3D_fb2_vir_addr1_gop2,
                  sg_G3D_fb2_bus_addr1_gop2);
    }

    sg_videomemorysize_gop2 = main_videomemory_size;

    FB2DBG ("[FB2 Driver] updateinfo_par: GOP_WIDTH = [%d], GOP_HEIGHT = [%d], GOPmemorysize = 0x%x \r\n",sg_mdrvinfinityfb2_default_var.xres,sg_mdrvinfinityfb2_default_var.yres,(unsigned int)sg_videomemorysize_gop2);

    // allocate new memory
    if (!(sg_pG3D_fb2_vir_addr1_gop2 = alloc_dmem(KEY_DMEM_FB2_BUF,
                                             PAGE_ALIGN(sg_videomemorysize_gop2),
                                             &sg_G3D_fb2_bus_addr1_gop2)))
    {
        FB2DBGERR("[FB2 Driver] Error: updateinfo_par: unable to allocate screen memory\n");
        return -ENOMEM;
    }

    // prevent initial garbage on screen ; to clean memory
    memset(CAST_2_IOMEM_CHAR_P sg_pG3D_fb2_vir_addr1_gop2,0, sg_videomemorysize_gop2);

    //update settings for global structure of fb_fix_screeninfo
    sg_mdrvinfinityfb2_fix.smem_start  = sg_G3D_fb2_bus_addr1_gop2;
    sg_mdrvinfinityfb2_fix.smem_len    = sg_videomemorysize_gop2;
    sg_mdrvinfinityfb2_fix.line_length = sg_mdrvinfinityfb2_default_var.xres * ANDROID_BYTES_PER_PIXEL;

    FB2DBG("[FB2 Driver] vir=0x%x, phy=0x%x\n", (unsigned int)sg_pG3D_fb2_vir_addr1_gop2, sg_G3D_fb2_bus_addr1_gop2);


    return 0;
}

static int mdrvinfinityfb2_infinity_SetPalette(FB_GOP_PaletteEntry *pPalEntry, int intPalStart,int intPalEnd)
{
    int i   = 0;
    int ret = -1;

    //open scl0 fclk to load palette
    if (IS_ERR(gop2_clks[1]))
    {
        FB2DBGERR( "[FB2 Driver] Error: SetPalette Fail to get gop fclk of scl!\n" );
        return ret;
    }
    else
    {
        clk_prepare_enable(gop2_clks[1]);
    }

    FB2DBGMORE("Palette value:(B,G,R,A)\n");

    //write palette
    for(i=intPalStart;i<=intPalEnd;i++)
    {
        GOP2_Set_Palette_RIU(0, i, pPalEntry[i].BGRA.u8A, pPalEntry[i].BGRA.u8R, pPalEntry[i].BGRA.u8G, pPalEntry[i].BGRA.u8B);

        //record in global variable
        stGOPCurrentPalInfo_GOP2[i].BGRA.u8B=pPalEntry[i].BGRA.u8B;
        stGOPCurrentPalInfo_GOP2[i].BGRA.u8G=pPalEntry[i].BGRA.u8G;
        stGOPCurrentPalInfo_GOP2[i].BGRA.u8R=pPalEntry[i].BGRA.u8R;
        stGOPCurrentPalInfo_GOP2[i].BGRA.u8A=pPalEntry[i].BGRA.u8A;
        FB2DBGMORE("(%d,%d,%d,%d)\n",stGOPCurrentPalInfo_GOP2[i].BGRA.u8B,stGOPCurrentPalInfo_GOP2[i].BGRA.u8G,stGOPCurrentPalInfo_GOP2[i].BGRA.u8R,stGOPCurrentPalInfo_GOP2[i].BGRA.u8A);
    }

    //close clk
    clk_disable_unprepare(gop2_clks[1]);

    return 0;
}

static int mdrvinfinityfb2_set_par(struct fb_info *info)
{
    int ret = 0;
    FB2DBG( "[FB2 Driver] Test I3!\n" );

    ret=mdrvinfinityfb2_infinity_updateinfo_par(info->var.xres,info->var.yres);

    info->fix.smem_start  = sg_mdrvinfinityfb2_fix.smem_start;
    info->fix.smem_len    = sg_mdrvinfinityfb2_fix.smem_len;

    info->var.width       = info->var.xres;
    info->var.height      = info->var.yres;

    info->screen_base     = (char *)CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1_gop2);
    info->fix.line_length = info->var.xres * ANDROID_BYTES_PER_PIXEL;

    //GOP_Setmode(FALSE, 0,sg_mdrvinfinityfb2_default_var.xres,sg_mdrvinfinityfb2_default_var.yres,sg_G3D_fb2_bus_addr1_gop2,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
    GOP2_Set_Outputmode(0, sg_mdrvinfinityfb2_default_var.xres,sg_mdrvinfinityfb2_default_var.yres,sg_G3D_fb2_bus_addr1_gop2,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL);

    return ret;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb2_mmap
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
static int mdrvinfinityfb2_mmap(struct fb_info *pinfo, struct vm_area_struct *vma)
{
    size_t size;
    size = 0;

    if (sg_videomemorysize_gop2<1)
    {
        FB2DBGERR("[FB2 Driver] ERROR: mmap, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if (NULL == pinfo)
    {
        FB2DBGERR("[FB2 Driver] ERROR: mmap, pinfo is NULL pointer !\n");
        return -ENOTTY;
    }
    if (NULL == vma)
    {
        FB2DBGERR("[FB2 Driver] ERROR: mmap, vma is NULL pointer !\n");
        return -ENOTTY;
    }

    size = vma->vm_end - vma->vm_start;

    vma->vm_pgoff += ((sg_G3D_fb2_bus_addr1_gop2 ) >> PAGE_SHIFT);

    FB2DBG(
    "[FB2 Driver] mmap vma->vm_start=%x\n vma->vm_end=%x\n vma->vm_pgoff =%x\n",
    (unsigned int) vma->vm_start, (unsigned int)vma->vm_end ,
    (unsigned int)vma->vm_pgoff);

    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

//sw update inverse color's inverse table thread when rotate case happens
static int mdrvinfinityfb2_SWupdate_InvTable(void *arg)
{
    int ret=0;
    while(1)
    {
        if (kthread_should_stop()) break;

        ret=GOP2_invColor_CPU_Upate_InvTable(genAE_info_gop2.u32AEBlkWidth,
                                             genAE_info_gop2.u32AEBlkHeight,
                                             genAE_info_gop2.u32AEDisplayWidth,
                                             genAE_info_gop2.u32AEDisplayHeight);
        if(!ret)//if update wrong, break
        {
            FB2DBGERR("[FB2 Driver] ERROR: mdrvinfinityfb2_SWupdate_InvTable wrong!\n");
            genTreadisError_gop2=1;
            break;
        }


    }

   return 0;

}

static int mdrvinfinityfb2_SWupdate_InvTable_debugmode(void *arg)
{

    while(1)
    {
        if (kthread_should_stop()) break;

        GOP2_invColor_DebugMode_UpdateInvTable();
        msleep(1000);
    }

   return 0;

}


//check whether need doing sw update inverse color's inverse table or not
static void mdrvinfinityfb2_checkInvTable_UpdateMode(void)
{
    int idx=0;
    int ret;

    if((genInv_Color_gop2==1) && (genSC_info_gop2.bRotateEn==1))
    {
        idx=1;
    }
    else
    {
        idx=0;
    }

    if(genTreadisError_gop2==1)
    {
        pSWupdateInvThread_gop2=NULL;
        genTreadisError_gop2=0;
    }

    if(idx==1)//enable thread
    {
        if(pSWupdateInvThread_gop2==NULL)
        {
            pSWupdateInvThread_gop2 = kthread_create(mdrvinfinityfb2_SWupdate_InvTable,(void *)&pSWupdateInvThread_gop2,"GOP2_SWupdate_InvColor");
            if (IS_ERR(pSWupdateInvThread_gop2))
            {
                ret = PTR_ERR(pSWupdateInvThread_gop2);
                pSWupdateInvThread_gop2 = NULL;
                FB2DBGERR("[FB2 Driver] ERROR: mdrvinfinityfb2_checkInvTable_UpdateMode wrong! Create thread fail! ret=%d\n",ret);
            }
            else
            {
                wake_up_process(pSWupdateInvThread_gop2);
            }
        }
    }
    else if (idx==0 && (pSWupdateInvThread_gop2!=NULL))//disable thread
    {
        kthread_stop(pSWupdateInvThread_gop2);
        pSWupdateInvThread_gop2 = NULL;
    }

}



// FB Ioctl Functions

int _MDrv_FBIO_IOC_Get_SUPINFO_GOP2(unsigned long arg)
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


int _MDrv_FBIO_IOC_Get_GWIN_GOP2(unsigned long arg)
{
    FB_GOP_GWIN_CONFIG stCfg;

    stCfg.u18HStart = genGWinInfo_gop2.u18HStart;
    stCfg.u18HEnd   = genGWinInfo_gop2.u18HEnd;
    stCfg.u18VStart = genGWinInfo_gop2.u18VStart;
    stCfg.u18VEnd   = genGWinInfo_gop2.u18VEnd;

    if(copy_to_user((FB_GOP_GWIN_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_GWIN_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_GWIN_GOP2(unsigned long arg)
{
    unsigned long GOP_Reg_Base = 0;
    FB_GOP_GWIN_CONFIG stCfg;
    unsigned short MIU_BUS     = 0x04;
    unsigned short u16FrameBuffer_Bytes_Per_Pixel = ANDROID_BYTES_PER_PIXEL;
    unsigned long u32TempVal   = 0;
    unsigned long GOP_Reg_DB   = GOP_BANK_DOUBLE_WR_G2;
    int Panel_Width            = 0;
    int Panel_Height           = 0;

    if (sg_videomemorysize_gop2<1)
    {
        FB2DBGERR("[FB2 Driver] ERROR: _MDrv_FBIO_IOC_Set_GWIN, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&stCfg, (FB_GOP_GWIN_CONFIG __user *)arg, sizeof(FB_GOP_GWIN_CONFIG)))
    {
        return -EFAULT;
    }

    Panel_Width  = (int)(stCfg.u18HEnd-stCfg.u18HStart+1);
    Panel_Height = (int)(stCfg.u18VEnd-stCfg.u18VStart+1);

    /// GOP0_0 settings
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    // GOP global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,(Panel_Width >>1)+1);//miu efficiency = Stretch Window H size (unit:2 pixel) /2 +1

    // GOP display area global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_30,Panel_Width >>1); //Stretch Window H size (unit:2 pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_31,Panel_Height);    //Stretch window V size

    // gwin info is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP21_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(1);
    }

    genGWinInfo_gop2.u18HStart = stCfg.u18HStart;
    genGWinInfo_gop2.u18HEnd   = stCfg.u18HEnd;
    genGWinInfo_gop2.u18VStart = stCfg.u18VStart;
    genGWinInfo_gop2.u18VEnd   = stCfg.u18VEnd;

    // GOP Display Setting, GWIN
    // 2.1 GWIN display area in panel : : H Start postion and end information
    u32TempVal=(genGWinInfo_gop2.u18HStart*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_04,u32TempVal); // H start
    u32TempVal=(genGWinInfo_gop2.u18HEnd*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_05,u32TempVal); // H end

    // 2.2 GWIN  display area in panel : V Start postion and end information
    OUTREG16(GOP_Reg_Base+REG_GOP_06,genGWinInfo_gop2.u18VStart); // V start line
    OUTREG16(GOP_Reg_Base+REG_GOP_08,genGWinInfo_gop2.u18VEnd); // V end line

    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(0);
    }

    //write gop register
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

    return 0;
}


int _MDrv_FBIO_IOC_Get_EnGOP_GOP2(unsigned long arg)
{
    unsigned char enGOP;

    GOP2_Get_Enable_GWIN(0, &enGOP);

    //remove check scaler's switch later in I3
    if(enGOP && INREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_68,0x0001))
        enGOP=1;
    else
        enGOP=0;

    if(copy_to_user((unsigned char __user *)arg, &enGOP, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_EnGOP_GOP2(unsigned long arg)
{
    unsigned char enGOP;

    if (sg_videomemorysize_gop2<1)
    {
        FB2DBGERR("[FB2 Driver] ERROR: _MDrv_FBIO_IOC_Set_EnGOP, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&enGOP, (unsigned char __user *)arg, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    FB2DBG( "[FB2 Driver] enable GOP=%d \n",enGOP);

    // record in global variable
    genGWIN_gop2=enGOP;

    // this switch will let scaler handle, remove later in I3
    // enable/disable gop switch at scaler
    if(enGOP)
        OUTREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_68,0x0001,0x0001);
    else
        OUTREGMSK16(GET_REG16_ADDR(BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_68,0x0000,0x0001);

    GOP2_Set_Enable_GWIN(0,enGOP);

    return 0;
}


int _MDrv_FBIO_IOC_Get_Alpha_GOP2(unsigned long arg)
{
    FB_GOP_ALPHA_CONFIG stCfg;
    int alpha_type  = 0;
    int alpha_value = 0;

    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    // get alpha blending settings from register
    GOP2_Get_Alpha(0,&alpha_type,&alpha_value);

    // copy result from register
    stCfg.bEn         = genGOPALPHA_gop2.bEn;
    stCfg.enAlphaType = (FB_GOP_ALPHA_TYPE)alpha_type;
    stCfg.u8Alpha     = (unsigned char)alpha_value;

    // record settings in global variables
    genGOPALPHA_gop2.enAlphaType=stCfg.enAlphaType;
    genGOPALPHA_gop2.u8Alpha=stCfg.u8Alpha;

    if(copy_to_user((FB_GOP_ALPHA_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_Constant_Alpha_GOP2(unsigned long arg)
{
    FB_GOP_ALPHA_CONFIG stCfg;

    if (sg_videomemorysize_gop2<1)
    {
        FB2DBGERR("[FB2 Driver] ERROR: _MDrv_FBIO_IOC_Set_Constant_Alpha, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }
    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    FB2DBG( "[FB2 Driver] Set alpha enable=%d, type=%d, constant alpha value=0x%x \n",(int)stCfg.bEn, (int)stCfg.enAlphaType, stCfg.u8Alpha);

    if(stCfg.bEn)
    {
        GOP2_Set_Constant_Alpha(0, (int)stCfg.enAlphaType, stCfg.u8Alpha);
    }
    else
    {
        GOP2_Set_Constant_Alpha(0, CONST_ALPHA, 0);//close alpha blending, let gop always shows
    }

    genGOPALPHA_gop2.bEn=stCfg.bEn;
    genGOPALPHA_gop2.enAlphaType=stCfg.enAlphaType;
    genGOPALPHA_gop2.u8Alpha=stCfg.u8Alpha;

    return 0;
}

int _MDrv_FBIO_IOC_Get_Color_Key_GOP2(unsigned long arg)
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

    GOP2_Get_Color_Key(0,&Enable,&Red,&Green,&Blue);

    // enable/disable
    stCfg.bEn = (unsigned char)Enable;

    // colorkey value
    stCfg.u8R = (unsigned char)Red;
    stCfg.u8G = (unsigned char)Green;
    stCfg.u8B = (unsigned char)Blue;

    genGOPCOLORKEY_gop2.bEn=stCfg.bEn;
    genGOPCOLORKEY_gop2.u8R=stCfg.u8R;
    genGOPCOLORKEY_gop2.u8G=stCfg.u8G;
    genGOPCOLORKEY_gop2.u8B=stCfg.u8B;

    if(copy_to_user((FB_GOP_COLORKEY_CONFIG __user *)arg, &stCfg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}


int _MDrv_FBIO_IOC_Set_Color_Key_GOP2(unsigned long arg)
{
    FB_GOP_COLORKEY_CONFIG stCfg;

    if (sg_videomemorysize_gop2<1)
    {
        FB2DBGERR("[FB2 Driver] ERROR: _MDrv_FBIO_IOC_Set_Color_Key, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }
    if(copy_from_user(&stCfg, (FB_GOP_COLORKEY_CONFIG __user *)arg, sizeof(FB_GOP_COLORKEY_CONFIG)))
    {
        return -EFAULT;
    }

    FB2DBG( "[FB2 Driver] Set ColorKey enable=%d, R=0x%x, G=0x%x, B=0x%x\n", (int)stCfg.bEn, stCfg.u8R, stCfg.u8G, stCfg.u8B);

    GOP2_Set_Color_Key(0, stCfg.bEn, stCfg.u8R, stCfg.u8G, stCfg.u8B);
    genGOPCOLORKEY_gop2.bEn=stCfg.bEn;
    genGOPCOLORKEY_gop2.u8R=stCfg.u8R;
    genGOPCOLORKEY_gop2.u8G=stCfg.u8G;
    genGOPCOLORKEY_gop2.u8B=stCfg.u8B;

    return 0;
}

int _MDrv_FBIO_IOC_imageblit_GOP2(struct fb_info *info,unsigned long arg)
{
    int ret = 0;

// this ioctl is unused, the same fuction can be accessed by fb_ops -> fb_imageblit
#if 0
    struct fb_image stCfg;
    int i = 0;


    if (sg_videomemorysize_gop2<1)
    {
        FB2DBGERR("[FB2 Driver] ERROR: _MDrv_FBIO_IOC_imageblit, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&stCfg, (struct fb_image __user *)arg, sizeof(const struct fb_image)))
    {
        return -EFAULT;
    }

    if (NULL == stCfg.data)
    {
        FB2DBGERR("[FB2 Driver] ERROR:in mdrvinfinityfb2_ioctl, fb_image.data is NULL pointer\r\n");
        return -EFAULT;
    }

    if (stCfg.fg_color>255)
    {
        FB2DBGERR("[FB2 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }

    if (stCfg.bg_color>255)
    {
        FB2DBGERR("[FB2 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }

    if (stCfg.width>1920)
    {
        FB2DBGERR("[FB2 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
        return -EFAULT;
    }
    if (stCfg.height>1920)
    {
        FB2DBGERR("[FB2 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
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
            FB2DBGERR("[FB2 Driver] ERROR:in mdrvinfinityfb_ioctl, parameter wrong\r\n");
            return -EFAULT;
        }
    }

    FB2DBGMORE( "[FB2 Driver] Imgblit test, dst (x,y)=(%d,%d), width=%d, height=%d, colordepth=%d \n" ,stCfg.dx,stCfg.dy,stCfg.width,stCfg.height,stCfg.depth);

    sys_imageblit(info,&stCfg);
#endif

    return ret;
}

int _MDrv_FBIO_IOC_Set_Palette_GOP2(unsigned long arg)
{
    FB_GOP_PaletteEntry stCfg[256];

    if(copy_from_user(&stCfg, (FB_GOP_PaletteEntry __user *)arg, sizeof(FB_GOP_PaletteEntry)*256))
    {
        return -EFAULT;
    }

    mdrvinfinityfb2_infinity_SetPalette(stCfg,0,255);

    return 0;
}

int _MDrv_FBIO_IOC_Set_EnInvColor_GOP2(unsigned long arg)
{
    unsigned char enInvColor;
    int ret=0;

    if (sg_videomemorysize_gop2<1)
    {
        FB2DBGERR("[FB2 Driver] ERROR: _MDrv_FBIO_IOC_Set_EnInvColor, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&enInvColor, (unsigned char __user *)arg, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    FB2DBG( "[FB2 Driver] enable inverse color=%d \n",enInvColor);

    // record in global variable
    genInv_Color_gop2=enInvColor;

    GOP2_invColor_Enable(enInvColor);

    //check whether do sw update inverse color
    mdrvinfinityfb2_checkInvTable_UpdateMode();

    if(enInvColor==1)
    {
        GOP2_invColor_Debug_Mode();

        if(pSWupdateInvDebugThread_gop2==NULL)
        {
            pSWupdateInvDebugThread_gop2 = kthread_create(mdrvinfinityfb2_SWupdate_InvTable_debugmode,(void *)&pSWupdateInvDebugThread_gop2,"GOP2_debug_SWupdate_InvColor");
            if (IS_ERR(pSWupdateInvDebugThread_gop2))
            {
                ret=PTR_ERR(pSWupdateInvDebugThread_gop2);
                pSWupdateInvDebugThread_gop2 = NULL;
                FB2DBGERR("[FB2 Driver] ERROR: debug mode! Create thread fail!%d\n",ret);
            }
            else
            {
                wake_up_process(pSWupdateInvDebugThread_gop2);
            }
        }
    }
    else if((enInvColor==0)&&(pSWupdateInvDebugThread_gop2!=NULL))
    {
        kthread_stop(pSWupdateInvDebugThread_gop2);
        pSWupdateInvDebugThread_gop2 = NULL;
    }

    return 0;
}

int _MDrv_FBIO_IOC_Set_AEinfoConfig_GOP2(unsigned long arg)
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

    FB2DBG( "[FB2 Driver] set AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d\n",
    (int)stAECfg.u32AEDisplayWidth,(int)stAECfg.u32AEDisplayHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight);

    // record in global variable
    genAE_info_gop2.u32AEDisplayWidth  = stAECfg.u32AEDisplayWidth;
    genAE_info_gop2.u32AEDisplayHeight = stAECfg.u32AEDisplayHeight;
    genAE_info_gop2.u32AEBlkWidth      = stAECfg.u32AEBlkWidth;
    genAE_info_gop2.u32AEBlkHeight     = stAECfg.u32AEBlkHeight;

    // when scaling happens, change AE block size with this scale
    if(genSC_info_gop2.bScalingEn)
    {
        if((genAE_info_gop2.u32AEBlkWidth!=0)&&(genAE_info_gop2.u32AEBlkHeight!=0)&&(genAE_info_gop2.u32AEDisplayWidth!=0)&&(genAE_info_gop2.u32AEDisplayHeight!=0))
        {
            if(genSC_info_gop2.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info_gop2.u32CropYend-genSC_info_gop2.u32CropYstart;
                    tempcorpAEheight=genSC_info_gop2.u32CropXend-genSC_info_gop2.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info_gop2.u32CropXend-genSC_info_gop2.u32CropXstart;
                    tempcorpAEheight=genSC_info_gop2.u32CropYend-genSC_info_gop2.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerHeight/(int)genAE_info_gop2.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerWidth/(int)genAE_info_gop2.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerWidth/(int)genAE_info_gop2.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerHeight/(int)genAE_info_gop2.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FB2DBGERR( "[FB2 Driver] Error: _MDrv_FBIO_IOC_Set_AEinfoConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP2_invColor_Set_AE_Config((int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEDisplayHeight);
        }
        else
        {
            GOP2_invColor_Set_AE_Config((int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEDisplayWidth);
        }
    }

    if(genSC_info_gop2.bCropEn)//if crop enable, reset crop information when AE info change
    {
        if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP2_invColor_Set_Crop_Config((int)genSC_info_gop2.u32CropXstart,(int)genSC_info_gop2.u32CropYstart,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEDisplayHeight);
        }
        else
        {
            GOP2_invColor_Set_Crop_Config((int)genSC_info_gop2.u32CropXstart,(int)genSC_info_gop2.u32CropYstart,(int)stAECfg.u32AEBlkWidth,(int)stAECfg.u32AEBlkHeight,(int)stAECfg.u32AEDisplayWidth);
        }
    }

    return 0;
}

int _MDrv_FBIO_IOC_Set_YThres_GOP2(unsigned long arg)
{
    unsigned long YThres;

    if(copy_from_user(&YThres, (unsigned long __user *)arg, sizeof(unsigned long)))
    {
        return -EFAULT;
    }

    FB2DBG( "[FB2 Driver] set Y threshold=%d \n",(int)YThres);

    // record in global variable
    genY_Thres_gop2=YThres;

    GOP2_invColor_Set_Y_Threshold((int)YThres);

    return 0;
}

int _MDrv_FBIO_IOC_Set_ScalerinfoConfig_GOP2(unsigned long arg)
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

    FB2DBG( "[FB2 Driver] set Scl config, CropEn=%d, ScalingEn=%d, RotateEn=%d\n",
    (int)stSclCfg.bCropEn,(int)stSclCfg.bScalingEn,(int)stSclCfg.bRotateEn);

    if((stSclCfg.u32ScalerWidth<1)||(stSclCfg.u32ScalerWidth>1920))
    {
        FB2DBGERR( "[FB2 Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! parameter wrong\n");
        return -EFAULT;
    }
    if((stSclCfg.u32ScalerHeight<1)||(stSclCfg.u32ScalerHeight>1920))
    {
        FB2DBGERR( "[FB2 Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! parameter wrong\n");
        return -EFAULT;
    }

    // record in global variable
    genSC_info_gop2.bCropEn        = stSclCfg.bCropEn;
    genSC_info_gop2.bScalingEn     = stSclCfg.bScalingEn;
    genSC_info_gop2.u32ScalerWidth = stSclCfg.u32ScalerWidth;
    genSC_info_gop2.u32ScalerHeight= stSclCfg.u32ScalerHeight;
    genSC_info_gop2.u32CropXstart  = stSclCfg.u32CropXstart;
    genSC_info_gop2.u32CropXend    = stSclCfg.u32CropXend;
    genSC_info_gop2.u32CropYstart  = stSclCfg.u32CropYstart;
    genSC_info_gop2.u32CropYend    = stSclCfg.u32CropYend;
    // rotate will be get from ISP driver
    //genSC_info_gop2.bRotateEn      = stSclCfg.bRotateEn;
    //genSC_info_gop2.enRotateA      = stSclCfg.enRotateA;

    if(stSclCfg.bCropEn)
    {
        if((genAE_info_gop2.u32AEBlkWidth!=0)&&(genAE_info_gop2.u32AEBlkHeight!=0)&&(genAE_info_gop2.u32AEDisplayWidth!=0)&&(genAE_info_gop2.u32AEDisplayHeight!=0))
        {
            if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
            {
                GOP2_invColor_Set_Crop_Config((int)stSclCfg.u32CropXstart,(int)stSclCfg.u32CropYstart,(int)genAE_info_gop2.u32AEBlkHeight,(int)genAE_info_gop2.u32AEBlkWidth,(int)genAE_info_gop2.u32AEDisplayHeight);
            }
            else
            {
                GOP2_invColor_Set_Crop_Config((int)stSclCfg.u32CropXstart,(int)stSclCfg.u32CropYstart,(int)genAE_info_gop2.u32AEBlkWidth,(int)genAE_info_gop2.u32AEBlkHeight,(int)genAE_info_gop2.u32AEDisplayWidth);
            }
        }
        else
        {
            FB2DBGERR( "[FB2 Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail!!! No AE info\n");
            return -EFAULT;
        }
    }

    // when scaling happens, change AE block size with this scale
    if(stSclCfg.bScalingEn)
    {
        if((genAE_info_gop2.u32AEBlkWidth!=0)&&(genAE_info_gop2.u32AEBlkHeight!=0)&&(genAE_info_gop2.u32AEDisplayWidth!=0)&&(genAE_info_gop2.u32AEDisplayHeight!=0))
        {
            if(stSclCfg.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info_gop2.u32CropYend-genSC_info_gop2.u32CropYstart;
                    tempcorpAEheight=genSC_info_gop2.u32CropXend-genSC_info_gop2.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info_gop2.u32AEDisplayWidth/(int)genAE_info_gop2.u32AEBlkWidth);
                    if(tempAEblkheight > 0)
                    {
                        while(((genSC_info_gop2.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkheight++;
                            if(tempAEblkheight>genSC_info_gop2.u32ScalerWidth)
                            {
                                tempAEblkheight=genSC_info_gop2.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info_gop2.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkwidth++;
                        }
                    }
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info_gop2.u32CropXend-genSC_info_gop2.u32CropXstart;
                    tempcorpAEheight=genSC_info_gop2.u32CropYend-genSC_info_gop2.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayWidth/(int)genAE_info_gop2.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    FB2DBGMORE("[FB2 Driver] crop:x_start=%d, x_end=%d\n",(int)genSC_info_gop2.u32CropXstart,(int)genSC_info_gop2.u32CropXend);
                    FB2DBGMORE("[FB2 Driver] AE:D_width=%d, Blk_width=%d\n",(int)genAE_info_gop2.u32AEDisplayWidth,(int)genAE_info_gop2.u32AEBlkWidth);
                    FB2DBGMORE("[FB2 Driver] SCL:width=%d, height=%d\n",(int)genSC_info_gop2.u32ScalerWidth,(int)genSC_info_gop2.u32ScalerHeight);
                    FB2DBGMORE("[FB2 Driver] tempcorpAEwidth=%d, tempcorpAEheight=%d\n",(int)tempcorpAEwidth,(int)tempcorpAEheight);
                    FB2DBGMORE("[FB2 Driver] before tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    if(tempAEblkwidth > 0)
                    {
                        while(((genSC_info_gop2.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkwidth++;
                            if(tempAEblkwidth>genSC_info_gop2.u32ScalerWidth)
                            {
                                tempAEblkwidth=genSC_info_gop2.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info_gop2.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkheight++;
                        }
                    }
                    FB2DBGMORE("[FB2 Driver] after tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerHeight/(int)genAE_info_gop2.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerWidth/(int)genAE_info_gop2.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info_gop2.u32AEDisplayWidth/(int)genAE_info_gop2.u32AEBlkWidth);
                    if(tempAEblkheight > 0)
                    {
                        while(((genSC_info_gop2.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkheight++;
                            if(tempAEblkheight>genSC_info_gop2.u32ScalerWidth)
                            {
                                tempAEblkheight=genSC_info_gop2.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info_gop2.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkwidth++;
                        }
                    }
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerWidth/genAE_info_gop2.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerHeight/genAE_info_gop2.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayWidth/(int)genAE_info_gop2.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    FB2DBGMORE("[FB2 Driver] crop:x_start=%d, x_end=%d\n",(int)genSC_info_gop2.u32CropXstart,(int)genSC_info_gop2.u32CropXend);
                    FB2DBGMORE("[FB2 Driver] AE:D_width=%d, Blk_width=%d\n",(int)genAE_info_gop2.u32AEDisplayWidth,(int)genAE_info_gop2.u32AEBlkWidth);
                    FB2DBGMORE("[FB2 Driver] AE:D_height=%d, Blk_height=%d\n",(int)genAE_info_gop2.u32AEDisplayHeight,(int)genAE_info_gop2.u32AEBlkHeight);
                    FB2DBGMORE("[FB2 Driver] SCL:width=%d, height=%d\n",(int)genSC_info_gop2.u32ScalerWidth,(int)genSC_info_gop2.u32ScalerHeight);
                    FB2DBGMORE("[FB2 Driver] before tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    if(tempAEblkwidth > 0)
                    {
                        while(((genSC_info_gop2.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                        {
                            tempAEblkwidth++;
                            if(tempAEblkwidth>genSC_info_gop2.u32ScalerWidth)
                            {
                                tempAEblkwidth=genSC_info_gop2.u32ScalerWidth;
                                break;
                            }
                        }
                    }
                    if(tempAEblkynum > 0)
                    {
                        while(genSC_info_gop2.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                        {
                            tempAEblkheight++;
                        }
                    }
                    FB2DBGMORE("[FB2 Driver] after tempAEblkwidth=%d, tempAEblkheight=%d\n",tempAEblkwidth,tempAEblkheight);
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FB2DBGERR( "[FB2 Driver] Error: _MDrv_FBIO_IOC_Set_ScalerinfoConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP2_invColor_Set_AE_Config((int)genAE_info_gop2.u32AEBlkHeight,(int)genAE_info_gop2.u32AEBlkWidth,(int)genAE_info_gop2.u32AEDisplayHeight);
        }
        else
        {
            GOP2_invColor_Set_AE_Config((int)genAE_info_gop2.u32AEBlkWidth,(int)genAE_info_gop2.u32AEBlkHeight,(int)genAE_info_gop2.u32AEDisplayWidth);
        }
    }

    return 0;
}

int _MDrv_FBIO_IOC_AutoAEConfig_GOP2(unsigned long arg)
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
        FB2DBGERR( "[FB2 Driver] Error: auto AE config fail!!! No proper AE value\n");
        FB2DBGERR( "[FB2 Driver] auto AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d, rotate en=%d\n",
        (int)stGOPAEConfig.u32AEDisplayWidth,(int)stGOPAEConfig.u32AEDisplayHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,genSC_info_gop2.bRotateEn);
        return -EFAULT;
    }

    stGOPAEConfig.u32AEBlkWidth= (unsigned long)stISPAEconfig.blk_w;
    stGOPAEConfig.u32AEBlkHeight= (unsigned long)stISPAEconfig.blk_h;
    stGOPAEConfig.u32AEDisplayWidth= (unsigned long)stISPAEconfig.img_w;
    stGOPAEConfig.u32AEDisplayHeight= (unsigned long)stISPAEconfig.img_h;

    // record in global variable
    genAE_info_gop2.u32AEDisplayWidth  = stGOPAEConfig.u32AEDisplayWidth;
    genAE_info_gop2.u32AEDisplayHeight = stGOPAEConfig.u32AEDisplayHeight;
    genAE_info_gop2.u32AEBlkWidth      = stGOPAEConfig.u32AEBlkWidth;
    genAE_info_gop2.u32AEBlkHeight     = stGOPAEConfig.u32AEBlkHeight;
    if(stISPAEconfig.rot==1)
    {
        genSC_info_gop2.bRotateEn=1;
        InvTblupdateMode=1;//by cpu
    }
    else
    {
        genSC_info_gop2.bRotateEn=0;
        InvTblupdateMode=0;//by hw engine
    }

    GOP2_invColor_Set_UpdateMode(InvTblupdateMode);

    FB2DBG( "[FB2 Driver] auto AE config, AExres=%d, AEyres=%d, AEblkW=%d, AEblkH=%d, rotate en=%d\n",
    (int)stGOPAEConfig.u32AEDisplayWidth,(int)stGOPAEConfig.u32AEDisplayHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,genSC_info_gop2.bRotateEn);

    // when scaling happens, change AE block size with this scale
    if(genSC_info_gop2.bScalingEn)
    {
        if((genAE_info_gop2.u32AEBlkWidth!=0)&&(genAE_info_gop2.u32AEBlkHeight!=0)&&(genAE_info_gop2.u32AEDisplayWidth!=0)&&(genAE_info_gop2.u32AEDisplayHeight!=0))
        {
            if(genSC_info_gop2.bCropEn)//if do crop, should add crop information to calculate scaling ratio
            {
                if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempcorpAEwidth=genSC_info_gop2.u32CropYend-genSC_info_gop2.u32CropYstart;
                    tempcorpAEheight=genSC_info_gop2.u32CropXend-genSC_info_gop2.u32CropXstart;
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerHeight/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerWidth/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info_gop2.u32AEDisplayWidth/(int)genAE_info_gop2.u32AEBlkWidth);
                    while(((genSC_info_gop2.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkheight++;
                        if(tempAEblkheight>genSC_info_gop2.u32ScalerWidth)
                        {
                            tempAEblkheight=genSC_info_gop2.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info_gop2.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkwidth++;
                    }
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempcorpAEwidth=genSC_info_gop2.u32CropXend-genSC_info_gop2.u32CropXstart;
                    tempcorpAEheight=genSC_info_gop2.u32CropYend-genSC_info_gop2.u32CropYstart;
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerWidth/(int)tempcorpAEwidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerHeight/(int)tempcorpAEheight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayWidth/(int)genAE_info_gop2.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    while(((genSC_info_gop2.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkwidth++;
                        if(tempAEblkwidth>genSC_info_gop2.u32ScalerWidth)
                        {
                            tempAEblkwidth=genSC_info_gop2.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info_gop2.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkheight++;
                    }
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
            else// if no crop, calculate the scaling ratio directly
            {
                if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
                {
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerHeight/(int)genAE_info_gop2.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerWidth/(int)genAE_info_gop2.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    tempAEblkynum=((int)genAE_info_gop2.u32AEDisplayWidth/(int)genAE_info_gop2.u32AEBlkWidth);
                    while(((genSC_info_gop2.u32ScalerWidth%tempAEblkheight)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkheight++;
                        if(tempAEblkheight>genSC_info_gop2.u32ScalerWidth)
                        {
                            tempAEblkheight=genSC_info_gop2.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info_gop2.u32ScalerHeight>(tempAEblkwidth*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkwidth++;
                    }
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkheight,tempAEblkwidth,tempAEblkxnum);
                }
                else
                {
                    tempAEblkwidth=((int)genAE_info_gop2.u32AEBlkWidth*(int)genSC_info_gop2.u32ScalerWidth/genAE_info_gop2.u32AEDisplayWidth);
                    tempAEblkheight=((int)genAE_info_gop2.u32AEBlkHeight*(int)genSC_info_gop2.u32ScalerHeight/genAE_info_gop2.u32AEDisplayHeight);
                    tempAEblkxnum=((int)genAE_info_gop2.u32AEDisplayWidth/(int)genAE_info_gop2.u32AEBlkWidth);
                    tempAEblkynum=((int)genAE_info_gop2.u32AEDisplayHeight/(int)genAE_info_gop2.u32AEBlkHeight);
                    while(((genSC_info_gop2.u32ScalerWidth%tempAEblkwidth)!=0))//find proper blk_width, scaler's width mod blk_width should be 0
                    {
                        tempAEblkwidth++;
                        if(tempAEblkwidth>genSC_info_gop2.u32ScalerWidth)
                        {
                            tempAEblkwidth=genSC_info_gop2.u32ScalerWidth;
                            break;
                        }
                    }
                    while(genSC_info_gop2.u32ScalerHeight>(tempAEblkheight*tempAEblkynum))//find proper blk_height, add 1 if blk_H * blk_y_num <Scl_res
                    {
                        tempAEblkheight++;
                    }
                    GOP2_invColor_Set_AE_Config_Scaling(tempAEblkwidth,tempAEblkheight,tempAEblkxnum);
                }
            }
        }
        else
        {
            FB2DBGERR( "[FB2 Driver] Error: _MDrv_FBIO_IOC_AutoAEConfig fail2!!! No AE info\n");
            return -EFAULT;
        }
    }
    else//if disable scaling, set original settings
    {
        if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP2_invColor_Set_AE_Config((int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEDisplayHeight);
        }
        else
        {
            GOP2_invColor_Set_AE_Config((int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEDisplayWidth);
        }
    }

    if(genSC_info_gop2.bCropEn)//if crop enable, reset crop information when AE info change
    {
        if(genSC_info_gop2.bRotateEn)//if do rotate, switch AE's W and H
        {
            GOP2_invColor_Set_Crop_Config((int)genSC_info_gop2.u32CropXstart,(int)genSC_info_gop2.u32CropYstart,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEDisplayHeight);
        }
        else
        {
            GOP2_invColor_Set_Crop_Config((int)genSC_info_gop2.u32CropXstart,(int)genSC_info_gop2.u32CropYstart,(int)stGOPAEConfig.u32AEBlkWidth,(int)stGOPAEConfig.u32AEBlkHeight,(int)stGOPAEConfig.u32AEDisplayWidth);
        }
    }

    //return ae settings to user
    if(copy_to_user((FB_GOP_INVCOLOR_AE_CONFIG __user *)arg, &stGOPAEConfig, sizeof(FB_GOP_INVCOLOR_AE_CONFIG)))
    {
        return -EFAULT;
    }

    return 0;
}

int _MDrv_FBIO_IOC_Set_SW_InvTable(unsigned long arg)
{
    FB_GOP_SW_INV_TABLE swinvTable;
    int i=0;
    unsigned char *invColTable;
    int invTableSize=0;

    if(copy_from_user(&swinvTable, (FB_GOP_SW_INV_TABLE __user *)arg, sizeof(FB_GOP_SW_INV_TABLE)))
    {
        return -EFAULT;
    }

    FB2DBG( "[FB2 Driver] set sw inverse table \n");

    if(swinvTable.size>(128*90*4))
    {
        FB2DBG( "[FB2 Driver] Error: _MDrv_FBIO_IOC_Set_SW_InvTable fail, max size is 128*90 \n");
        return -EFAULT;
    }

    if(swinvTable.size<1)
    {
        FB2DBG( "[FB2 Driver] Error: _MDrv_FBIO_IOC_Set_SW_InvTable fail, min size is 1 \n");
        return -EFAULT;
    }

    invTableSize=(swinvTable.size/4);
    invColTable=kmalloc(invTableSize*sizeof(unsigned char),GFP_KERNEL);
    if (!invColTable)
    {
        FB2DBGERR("[FB2 Driver] Error: _MDrv_FBIO_IOC_Set_SW_InvTable: unable to kmalloc\n");
        return -ENOMEM;
    }
    memset(invColTable,0, invTableSize*sizeof(unsigned char));


    for(i=0;i<invTableSize;i++)
    {
        if(i<10)
        {
            FB2DBGMORE( "[FB2 Driver] %d: Y=%d\n",i,swinvTable.invTable[4*i+3]);
        }

        if(swinvTable.invTable[4*i+3]<genY_Thres_gop2)
            invColTable[i]=1;
        else
            invColTable[i]=0;
    }

    GOP2_invColor_Debug_Mode_SWupdate_InvTable(invTableSize,invColTable);

    kfree(invColTable);

    return 0;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb2_ioctl
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
mdrvinfinityfb2_ioctl(struct fb_info *pinfo, unsigned int cmd, unsigned long arg)
{
    int err=0, ret=0;
    unsigned int ret_Phy=0;

    if (NULL == pinfo)
    {
        FB2DBGERR("[FB2 Driver] ERROR:in mdrvinfinityfb2_ioctl, pinfo is NULL pointer\r\n");
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
            ret_Phy = sg_G3D_fb2_bus_addr1_gop2;
            FB2DBG( "[FB2 Driver] Phy Addr =%x\r\n",ret_Phy);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;

        case IOCTL_FB_GETVIDEOMEMSIZE:
            ret_Phy = sg_videomemorysize_gop2;
            FB2DBG( "[FB2 Driver] Kernel VideoMemSize =0x%x \n" ,(unsigned int)sg_videomemorysize_gop2);
            ret = __put_user(ret_Phy, CAST_2_USER_UINT_P arg);
            break;

        case IOCTL_FB_GETFBSUPPORTINF:
            FB2DBG( "[FB2 Driver] Get Framebuffer Support Information.\n");
            ret=_MDrv_FBIO_IOC_Get_SUPINFO_GOP2(arg);
            break;

        case IOCTL_FB_GETGWININFO:
            FB2DBG( "[FB2 Driver] Get Framebuffer GWIN Information.\n");
            ret=_MDrv_FBIO_IOC_Get_GWIN_GOP2(arg);
            break;

        case IOCTL_FB_SETGWININFO:
            FB2DBG( "[FB2 Driver] Set Framebuffer GWIN Information.\n");
            ret=_MDrv_FBIO_IOC_Set_GWIN_GOP2(arg);
            break;

        case IOCTL_FB_GETENABLEGOP:
            FB2DBG( "[FB2 Driver] Get GOP Enable/Disable.\n");
            ret=_MDrv_FBIO_IOC_Get_EnGOP_GOP2(arg);
            break;

        case IOCTL_FB_SETENABLEGOP:
            FB2DBG( "[FB2 Driver] Set GOP Enable/Disable.\n");
            ret=_MDrv_FBIO_IOC_Set_EnGOP_GOP2(arg);
            break;

        case IOCTL_FB_GETALPHA:
            FB2DBG( "[FB2 Driver] Get Alpha Information.\n");
            ret=_MDrv_FBIO_IOC_Get_Alpha_GOP2(arg);
            break;

        case IOCTL_FB_SETALPHA:
            FB2DBG( "[FB2 Driver] Set Alpha Information.\n");
            ret=_MDrv_FBIO_IOC_Set_Constant_Alpha_GOP2(arg);
            break;

        case IOCTL_FB_GETCOLORKEY:
            FB2DBG( "[FB2 Driver] Get Colorkey Information.\n");
            ret=_MDrv_FBIO_IOC_Get_Color_Key_GOP2(arg);
            break;

        case IOCTL_FB_SETCOLORKEY:
            FB2DBG( "[FB2 Driver] Set Colorkey Information.\n");
            ret=_MDrv_FBIO_IOC_Set_Color_Key_GOP2(arg);
            break;

        case IOCTL_FB_IMAGEBLIT:
            FB2DBG( "[FB2 Driver] Use Imageblit.\n");
            ret=_MDrv_FBIO_IOC_imageblit_GOP2(pinfo,arg);
            break;
        case IOCTL_FB_SETPALETTE:
            FB2DBG( "[FB2 Driver] Set Palette.\n");
            ret=_MDrv_FBIO_IOC_Set_Palette_GOP2(arg);
            break;

        case IOCTL_FB_SETENABLEINVCOLOR:
            FB2DBG( "[FB2 Driver] Set Enable/Disable inverse color.\n");
            ret=_MDrv_FBIO_IOC_Set_EnInvColor_GOP2(arg);
            break;

        case IOCTL_FB_SETAECONFIG:
            FB2DBG( "[FB2 Driver] Set AE configurations.\n");
            ret=_MDrv_FBIO_IOC_Set_AEinfoConfig_GOP2(arg);
            break;

        case IOCTL_FB_SETYTHRES:
            FB2DBG( "[FB2 Driver] Set Y threshold.\n");
            ret=_MDrv_FBIO_IOC_Set_YThres_GOP2(arg);
            break;

        case IOCTL_FB_SETSCALERCONFIG:
            FB2DBG( "[FB2 Driver] Set Scl configurations.\n");
            ret=_MDrv_FBIO_IOC_Set_ScalerinfoConfig_GOP2(arg);
            break;

        case IOCTL_FB_AUTOUPDATEAE:
            FB2DBG( "[FB2 Driver] auto AE.\n");
            ret=_MDrv_FBIO_IOC_AutoAEConfig_GOP2(arg);
            break;

        case IOCTL_FB_SWINVTABLE:
            FB2DBG( "[FB2 Driver] set sw inverse table.\n");
            ret=_MDrv_FBIO_IOC_Set_SW_InvTable(arg);
            break;


        default:  /* redundant, as cmd was checked against MAXNR */
            FB2DBGERR("[FB2 Driver] in default ioct\r\n");
            return -ENOTTY;
    }
    return ret;
}

static ssize_t gop2info_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t n)
{
    return 0;
}

static ssize_t gop2info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "[FB1/GOP1 Driver Information]\n");
    str += scnprintf(str, end - str, "[Framebuffer1]\n");
    str += scnprintf(str, end - str, " Width          : %4d pixel\n",sg_mdrvinfinityfb2_default_var.xres);
    str += scnprintf(str, end - str, " Height         : %4d pixel\n",sg_mdrvinfinityfb2_default_var.yres);
    str += scnprintf(str, end - str, " BPP            : %4d bytes per pixel\n",ANDROID_BYTES_PER_PIXEL);
    str += scnprintf(str, end - str, " Memory Size    : %d bytes\n",(unsigned int)sg_videomemorysize_gop2);
    str += scnprintf(str, end - str, " Memory Phy_Addr: 0x%x\n",sg_G3D_fb2_bus_addr1_gop2);
    str += scnprintf(str, end - str, " Memory Vir_Addr: 0x%x\n",(unsigned int)sg_pG3D_fb2_vir_addr1_gop2);
    str += scnprintf(str, end - str, "[GOP1(GWIN)]\n");
    if(genGWIN_gop2==0)
        str += scnprintf(str, end - str, " GWIN Status    : Close\n");
    else if(genGWIN_gop2==1)
        str += scnprintf(str, end - str, " GWIN Status    : Open\n");
    else
        str += scnprintf(str, end - str, " GWIN Status    : Unknown\n");

    str += scnprintf(str, end - str, " H Start        : %4d\n",genGWinInfo_gop2.u18HStart);
    str += scnprintf(str, end - str, " H End          : %4d\n",genGWinInfo_gop2.u18HEnd);
    str += scnprintf(str, end - str, " V Start        : %4d\n",genGWinInfo_gop2.u18VStart);
    str += scnprintf(str, end - str, " V End          : %4d\n",genGWinInfo_gop2.u18VEnd);

    if(genGOPALPHA_gop2.bEn==0)
        str += scnprintf(str, end - str, " Alpha Status   : Close\n");
    else if(genGOPALPHA_gop2.bEn==1)
        str += scnprintf(str, end - str, " Alpha Status   : Open\n");
    else
        str += scnprintf(str, end - str, " Alpha Status   : Unknown\n");


    if(genGOPALPHA_gop2.enAlphaType==PIXEL_ALPHA)
        str += scnprintf(str, end - str, " Alpha Type     : Pixel Alpha\n");
    else if(genGOPALPHA_gop2.enAlphaType==CONST_ALPHA)
    {
        str += scnprintf(str, end - str, " Alpha Type     : Constant Alpha\n");
        str += scnprintf(str, end - str, " Alpha Value    : 0x%2x\n",genGOPALPHA_gop2.u8Alpha);
    }
    else
        str += scnprintf(str, end - str, " Alpha Type     : wrong alpha type\n");

    if(genGOPCOLORKEY_gop2.bEn==0)
        str += scnprintf(str, end - str, " Colorkey Status: Close\n");
    else if(genGOPCOLORKEY_gop2.bEn==1)
    {
        str += scnprintf(str, end - str, " Colorkey Status: Open\n");
        str += scnprintf(str, end - str, " Red            : 0x%2x\n",genGOPCOLORKEY_gop2.u8R);
        str += scnprintf(str, end - str, " Green          : 0x%2x\n",genGOPCOLORKEY_gop2.u8G);
        str += scnprintf(str, end - str, " Blue           : 0x%2x\n",genGOPCOLORKEY_gop2.u8B);
    }
    else
        str += scnprintf(str, end - str, " Colorkey Status: Unknown\n");


    if (str > buf)  str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);

}

DEVICE_ATTR(gop2info, 0644, gop2info_show, gop2info_store);


static int mdrvinfinityfb2_suspend(struct platform_device *pdev,
                 pm_message_t state)
{
    int clockIdxtemp=0;
    FB2DBG("[FB2 Driver] in mdrvinfinityfb2_suspend \r\n");

    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks_gop2 ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            clk_disable_unprepare(gop2_clks[clockIdxtemp]);
        }
    }
    //clk_disable_unprepare(gop_psram_clk);
    return 0;
}

static int mdrvinfinityfb2_resume(struct platform_device *pdev)
{
    //dma_addr_t fb2_PHY_ADD_SHOW=0;
    int ret=-1;
    int clockIdxtemp=0;

    // open all needed clocks
    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks_gop2 ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            if (IS_ERR(gop2_clks[clockIdxtemp]))
            {
                FB2DBGERR( "[FB2 Driver] ERROR: Fail to get gop clk! Clock idx=%d\n",clockIdxtemp);
                kfree(gop2_clks);
                return ret;
            }
            else
            {
                clk_prepare_enable(gop2_clks[clockIdxtemp]);
            }
        }
    }

    FB2DBG("[FB2 Driver] mdrvinfinityfb2_resume \r\n");

    GOP2_Setmode(TRUE, 0,sg_mdrvinfinityfb2_default_var.xres,sg_mdrvinfinityfb2_default_var.yres,sg_G3D_fb2_bus_addr1_gop2,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    //set palette
    mdrvinfinityfb2_infinity_SetPalette(stGOPCurrentPalInfo_GOP2,0,255);

    return 0;
}
/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb2_probe
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
static int mdrvinfinityfb2_probe(struct platform_device *dev)
{
    struct fb_info *pinfo = NULL;
    int retval = -ENOMEM;
    int ret=-1;
    int clockIdxtemp=0;

    if (NULL == dev)
    {
        FB2DBGERR("[FB2 Driver] ERROR: in mdrvinfinityfb2_prob: dev is NULL pointer \r\n");
        return -ENOTTY;
    }

    // open all needed clocks
    num_parents_clocks_gop2 = of_clk_get_parent_count(dev->dev.of_node);
    gop2_clks=kzalloc(((sizeof(struct clk *) * num_parents_clocks_gop2)),GFP_KERNEL);

    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks_gop2 ; clockIdxtemp++)
    {
        gop2_clks[clockIdxtemp]= of_clk_get(dev->dev.of_node, clockIdxtemp);
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            if (IS_ERR(gop2_clks[clockIdxtemp]))
            {
                FB2DBGERR( "[FB2 Driver] ERROR: Fail to get gop clk! Clock idx=%d\n",clockIdxtemp);
                kfree(gop2_clks);
                return ret;
            }
            else
            {
                clk_prepare_enable(gop2_clks[clockIdxtemp]);
            }
        }
    }

    // init global variable
    mdrvinfinityfb2_updateinfo();

    pinfo = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
    if (!pinfo)
        goto err;

    // copy data for register framebuffer
    pinfo->screen_base = (char *)CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1_gop2);
    pinfo->fbops = &sg_mdrvinfinityfb2_ops;
    pinfo->var = sg_mdrvinfinityfb2_default_var;
    pinfo->fix = sg_mdrvinfinityfb2_fix;
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
    GOP2_Setmode(FALSE, 0,sg_mdrvinfinityfb2_default_var.xres,sg_mdrvinfinityfb2_default_var.yres,sg_G3D_fb2_bus_addr1_gop2,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    // set gop inverse color init settings
    GOP2_invColor_init();

    // set init palette
    mdrvinfinityfb2_infinity_SetPalette(DefaultPaletteEntry,0,255);

    // register sysfs
    ret = device_create_file(&dev->dev, &dev_attr_gop2info);
    if (ret != 0)
    {
      dev_err(&dev->dev,
       "[FB2 Driver] Failed to create ptgen_call sysfs files: %d\n", ret);
    }
    else
    {
        FB2DBGMORE("[FB2 Driver] device attr ok\n");
    }

    return 0;

err2:
    fb_dealloc_cmap(&pinfo->cmap);
err1:
    framebuffer_release(pinfo);
err:

    free_dmem(KEY_DMEM_FB2_BUF,
                      PAGE_ALIGN(sg_videomemorysize_gop2),
                      sg_pG3D_fb2_vir_addr1_gop2,
                      sg_G3D_fb2_bus_addr1_gop2);

    return retval;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb2_remove
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
static int mdrvinfinityfb2_remove(struct platform_device *dev)
{
    struct fb_info *pinfo;
    int clockIdxtemp=0;
    pinfo = 0;

    if (NULL == dev)
    {
        FB2DBGERR("[FB2 Driver] ERROR: mdrvinfinityfb2_remove: dev is NULL pointer \n");
        return -ENOTTY;
    }

    pinfo = platform_get_drvdata(dev);
    if (pinfo)
    {
        unregister_framebuffer(pinfo);

        free_dmem(KEY_DMEM_FB2_BUF,
                          PAGE_ALIGN(sg_videomemorysize_gop2),
                          sg_pG3D_fb2_vir_addr1_gop2,
                          sg_G3D_fb2_bus_addr1_gop2);

        framebuffer_release(pinfo);
    }

    // close all clocks
    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks_gop2 ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            clk_disable_unprepare(gop2_clks[clockIdxtemp]);
        }
    }
    return 0;
}

static int mdrvinfinityfb2_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    dma_addr_t  fb2_pandisplay_PHY_ADD_SHOW=0;
    int fb_size = 0;
    int fb_index = 0;
    FB2DBGMORE( "[FB2 Driver]%s: var->yoffset=%x! info->var.height=%x\n ",__func__,var->yoffset,info->var.height);

    fb_size = (info->var.width * info->var.height * ANDROID_BYTES_PER_PIXEL);
    fb_index = (var->yoffset/info->var.height);
    FB2DBGMORE( "[FB2 Driver]%s: fb_size = 0x%x, fb_index = %d\n", __func__, fb_size, fb_index);

    if(fb_index > ANDROID_NUMBER_OF_BUFFERS) {
        FB2DBGMORE( "[FB2 Driver]%s: fb_index = %d invalid!! reset as %d\n", __func__, fb_index, ANDROID_NUMBER_OF_BUFFERS);
        fb_index = ANDROID_NUMBER_OF_BUFFERS;
    }

    fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start + fb_size * fb_index;
    FB2DBGMORE( "[FB2 Driver]%s: use buffer %d! Addr = %x\n ", __func__, fb_index, fb2_pandisplay_PHY_ADD_SHOW);

    GOP2_Pan_Display(0, fb2_pandisplay_PHY_ADD_SHOW);

    return 0;
}


//check var to see if supported by this device, defined for infinity
static int mdrvinfinityfb2_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    int MiuBusLen=128;//miu bus length in iNfinity
    int DisplayPerPixel=0;
    int max_x_res=2560;
    int max_y_res=2048;

    DisplayPerPixel=MiuBusLen/(ANDROID_BYTES_PER_PIXEL*8);

    //MstarFB_ENTER();

    if((var->yres<1)||(var->xres<1))
    {
        FB2DBGERR( "[FB2 Driver] Error: fb_check_var: buffer x or y size should be > 0 !\n ");
        return -EINVAL;
    }
    if((var->yres * var->xres)>(max_x_res*max_y_res))
    {
        FB2DBGERR( "[FB2 Driver] Error: fb_check_var: total buffer size should be lower than 1920*1088!\n ");
        return -EINVAL;
    }

    if((var->xres % DisplayPerPixel)!=0)
    {
        FB2DBGERR( "[FB2 Driver] Error: fb_check_var: Display Per Pixel for Buffer Width should be %d!\n ",DisplayPerPixel);
        return -EINVAL;
    }

    //MstarFB_LEAVE();

    return 0;
}

module_platform_driver(sg_mdrvinfinityfb2_driver);

MODULE_LICENSE("GPL");
