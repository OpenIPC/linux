

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
static u_long sg_videomemorysize=0;

/* physical address, used in probe(), ioctl(), remove(), CreateGWin() */
/* physical address of frame buffer 1 2  */
dma_addr_t  sg_G3D_fb2_bus_addr1 = 0;

//extern
extern void SwitchSubBank(int);

extern void GOP_Setmode(int,int,int,int,U32,U32,U16,int);
extern void GOP_Change_Alpha(int,int,int,int,U32,U32,U16,int);
extern void GOP_Pan_Display(int,U32);
extern void GOP_Set_OutFormat(int,int);
extern void GOP_Set_Constant_Alpha(int,int,int);
extern void GOP_Set_Color_Key(int,int,int,int,int);
extern void GOP_Set_PipeDelay(int,int);
extern void GOP_Setmode_and_win(int,int,int,int,int,int,int,int,U32,U32,U16,int);


//known function, close temperally
//extern int mstar_sca_drv_get_pnl_refresh_rate(void);

//clock define, one for palette sram clock, one for scaler fclk1
struct clk **gop_clks;//index=1 is scl_fclk

//struct clk* gop_psram_clk;
//struct clk* gop_scl_fclk1;
int num_parents_clocks=0;



static struct platform_device *psg_mdrv_fbdev0;

FB_GOP_GWIN_CONFIG genGWinInfo={0};
unsigned char genGWIN=1; //record gwin is open or close
FB_GOP_ALPHA_CONFIG genGOPALPHA={0};
FB_GOP_COLORKEY_CONFIG genGOPCOLORKEY={0};
FB_GOP_RESOLUTION_STRETCH_H_CONFIG genGOPGernelSettings={0};

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

FB_GOP_PaletteEntry stGOPCurrentPalInfo[GOP_PALETTE_ENTRY_NUM]={};


static const char* KEY_DMEM_FB_BUF="FB_BUFFER";


//module_param(sg_videomemorysize, ulong, 0);

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

// update global variables while init
static void __init mdrvinfinityfb_updateinfo(void)
{

    static int main_videomemory_size=0;

    sg_mdrvinfinityfb_default_var.xres = 0;
    sg_mdrvinfinityfb_default_var.yres = 0;
    genGOPGernelSettings.u32Width=sg_mdrvinfinityfb_default_var.xres;
    genGOPGernelSettings.u32Height=sg_mdrvinfinityfb_default_var.yres;
    genGOPGernelSettings.enBufferNum=FB_DOUBLE_BUFFER;
    genGOPGernelSettings.enStretchH_Ratio=FB_STRETCH_H_RATIO_1;
    genGOPGernelSettings.u32DisplayWidth=genGOPGernelSettings.u32Width*genGOPGernelSettings.enStretchH_Ratio;
    genGOPGernelSettings.u32DisplayHeight=genGOPGernelSettings.u32Height;
    //GWin init, same size as screen
    genGWinInfo.u18HStart = 0;
    genGWinInfo.u18HEnd   = genGWinInfo.u18HStart+sg_mdrvinfinityfb_default_var.xres;
    genGWinInfo.u18VStart = 0;
    genGWinInfo.u18VEnd   = genGWinInfo.u18VStart+sg_mdrvinfinityfb_default_var.yres;
    main_videomemory_size = (sg_mdrvinfinityfb_default_var.xres*sg_mdrvinfinityfb_default_var.yres * ANDROID_BYTES_PER_PIXEL * (genGOPGernelSettings.enBufferNum));

    //known function, close temperally
    //sg_mdrvinfinityfb_default_fbmode.refresh=mstar_sca_drv_get_pnl_refresh_rate();

    sg_videomemorysize = main_videomemory_size;

    FBDBGMORE("[FB Driver] updateinfo: GOP_WIDTH = [%d], GOP_HEIGHT = [%d], sg_videomemorysize = 0x%x \r\n",sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,(unsigned int)sg_videomemorysize);

}

// update global variables while setting new resolution by ioctl FBIOPUT_VSCREENINFO
static int mdrvinfinityfb_infinity_updateinfo_par(unsigned int setWidth,unsigned int setHeight)
{

    static int main_videomemory_size=0;

    sg_mdrvinfinityfb_default_var.xres = setWidth;
    sg_mdrvinfinityfb_default_var.yres = setHeight;
    genGOPGernelSettings.u32Width=sg_mdrvinfinityfb_default_var.xres;
    genGOPGernelSettings.u32Height=sg_mdrvinfinityfb_default_var.yres;
    genGOPGernelSettings.u32DisplayWidth=genGOPGernelSettings.u32Width*genGOPGernelSettings.enStretchH_Ratio;
    genGOPGernelSettings.u32DisplayHeight=genGOPGernelSettings.u32Height;
    //GWin init, same size as screen
    genGWinInfo.u18HStart = 0;
    genGWinInfo.u18HEnd   = genGWinInfo.u18HStart+sg_mdrvinfinityfb_default_var.xres;
    genGWinInfo.u18VStart = 0;
    genGWinInfo.u18VEnd   = genGWinInfo.u18VStart+sg_mdrvinfinityfb_default_var.yres;
    main_videomemory_size = (sg_mdrvinfinityfb_default_var.xres*sg_mdrvinfinityfb_default_var.yres * ANDROID_BYTES_PER_PIXEL * (genGOPGernelSettings.enBufferNum));

    // free memory if it has
    if(sg_videomemorysize > 0)
    {
        free_dmem(KEY_DMEM_FB_BUF,
                  PAGE_ALIGN(sg_videomemorysize),
                  sg_pG3D_fb2_vir_addr1,
                  sg_G3D_fb2_bus_addr1);
    }

    sg_videomemorysize = main_videomemory_size;

    FBDBG ("[FB Driver] updateinfo_par: GOP_WIDTH = [%d], GOP_HEIGHT = [%d], sg_videomemorysize = 0x%x \r\n",sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,(unsigned int)sg_videomemorysize);

    // allocate new memory
    if (!(sg_pG3D_fb2_vir_addr1 = alloc_dmem(KEY_DMEM_FB_BUF,
                                             PAGE_ALIGN(sg_videomemorysize),
                                             &sg_G3D_fb2_bus_addr1)))
    {
        FBDBG("[FB Driver] Error: updateinfo_par: unable to allocate screen memory\n");
        return -ENOMEM;
    }

    // prevent initial garbage on screen ; to clean memory
    memset(CAST_2_IOMEM_CHAR_P sg_pG3D_fb2_vir_addr1,OTHER_COLOR, sg_videomemorysize);

    sg_mdrvinfinityfb_fix.smem_start = sg_G3D_fb2_bus_addr1;
    sg_mdrvinfinityfb_fix.smem_len = sg_videomemorysize;

    FBDBG("[FB Driver] vir=0x%x, phy=0x%x\n", (unsigned int)sg_pG3D_fb2_vir_addr1, sg_G3D_fb2_bus_addr1);


    return 0;
}

static int mdrvinfinityfb_infinity_SetPalette(FB_GOP_PaletteEntry *pPalEntry, int intPalStart,int intPalEnd)
{
    unsigned long GOP_Reg_Base;
    int i=0;
    int ret=-1;
    //unsigned long tempval;

    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    //open scl0 fclk to load palette
    if (IS_ERR(gop_clks[1]))
    {
        FBDBG( "[FB Driver] Error: SetPalette Fail to get gop fclk of scl!\n" );
        return ret;
    }
    else
    {
        clk_prepare_enable(gop_clks[1]);
    }

    //set force write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0x0200 , GOP_BANK_FORCE_WR);

    /*tempval=INREGMSK16(GOP_Reg_Base+REG_GOP_3B, 0x0003);
    if(tempval & 0x03)
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_3B, 0 , 0x0003);
    }*/

    //set RIU mode
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_05, 0x0000 , 0x1000);

    FBDBGMORE("Palette value:(B,G,R,A)\n");

    //write palette
    for(i=intPalStart;i<=intPalEnd;i++)
    {
        //set palette value
        OUTREG16(GOP_Reg_Base+REG_GOP_03, pPalEntry[i].BGRA.u8G<<8|pPalEntry[i].BGRA.u8B);
        OUTREG16(GOP_Reg_Base+REG_GOP_04, pPalEntry[i].BGRA.u8A<<8|pPalEntry[i].BGRA.u8R);
        //set palette index
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_05, i , 0x00FF);
        //write trigger
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_05, 0x0100 , 0x0100);
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_05, 0x0000 , 0x0100);

        //record in global variable
        stGOPCurrentPalInfo[i].BGRA.u8B=pPalEntry[i].BGRA.u8B;
        stGOPCurrentPalInfo[i].BGRA.u8G=pPalEntry[i].BGRA.u8G;
        stGOPCurrentPalInfo[i].BGRA.u8R=pPalEntry[i].BGRA.u8R;
        stGOPCurrentPalInfo[i].BGRA.u8A=pPalEntry[i].BGRA.u8A;
        FBDBGMORE("(%d,%d,%d,%d)\n",stGOPCurrentPalInfo[i].BGRA.u8B,stGOPCurrentPalInfo[i].BGRA.u8G,stGOPCurrentPalInfo[i].BGRA.u8R,stGOPCurrentPalInfo[i].BGRA.u8A);
    }

    //close force write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0x0000 , GOP_BANK_FORCE_WR);

    //close clk
    clk_disable_unprepare(gop_clks[1]);

    return 0;
}

static int mdrvinfinityfb_set_par(struct fb_info *info)
{
    int ret = 0;
    //unsigned long GOP_Reg_Base;
    //MstarFB_ENTER();

    //if use back to this to set resolution, it will set to default
    genGOPGernelSettings.enStretchH_Ratio=FB_STRETCH_H_RATIO_1;
    genGOPGernelSettings.enBufferNum=FB_DOUBLE_BUFFER;

    ret=mdrvinfinityfb_infinity_updateinfo_par(info->var.xres,info->var.yres);

    info->fix.smem_start=sg_mdrvinfinityfb_fix.smem_start;
    info->fix.smem_len=sg_mdrvinfinityfb_fix.smem_len;

    info->var.width=info->var.xres;
    info->var.height=info->var.yres;

    info->screen_base = CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1);
    info->fix.line_length      = info->var.xres * ANDROID_BYTES_PER_PIXEL;

    GOP_Setmode(FALSE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    //MstarFB_LEAVE();
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

    //MstarFB_ENTER();

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    if (sg_videomemorysize<1)
    {
        FBDBG("[FB Driver] ERROR: mmap, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if (NULL == pinfo)
    {
        FBDBG("[FB Driver] ERROR: mmap, pinfo is NULL pointer !\n");
        return -ENOTTY;
    }
    if (NULL == vma)
    {
        FBDBG("[FB Driver] ERROR: mmap, vma is NULL pointer !\n");
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


    //MstarFB_LEAVE();
    return 0;
}



// FB Ioctl Functions

int _MDrv_FBIO_IOC_Get_SUPINFO(unsigned long arg)
{
    FB_GOP_SUPINF_CONFIG stCfg;

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    // enable gwin is in GOP01

    stCfg.bKeyAlpha   = 1;
    stCfg.bConstAlpha = 1;
    stCfg.bPixelAlpha = 1;
    stCfg.bColFmt[COLORFMT_ARGB8888] = 0;
    stCfg.bColFmt[COLORFMT_RGB565]   = 0;
    stCfg.bColFmt[COLORFMT_YUV422]   = 0;
    stCfg.bColFmt[COLORFMT_I8PALETTE]= 1;
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
    FB_GOP_GWIN_CONFIG stCfg;

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

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

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    if (sg_videomemorysize<1)
    {
        FBDBG("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_GWIN, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    // gwin info is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;
    // switch to subbank01
    SwitchSubBank(1);

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

    // switch back to subbank00
    SwitchSubBank(0);

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

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    // enable gwin is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    // switch to subbank01
    SwitchSubBank(1);

    // get enable/disable gwin
    if(INREGMSK16(GOP_Reg_Base+REG_GOP_00, 0x0001) && INREGMSK16(GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_60,0x0001))
        enGOP=1;
    else
        enGOP=0;

    // switch back to subbank00
    SwitchSubBank(0);

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

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    if (sg_videomemorysize<1)
    {
        FBDBG("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_EnGOP, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    // enable gwin is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    if(copy_from_user(&enGOP, (unsigned char __user *)arg, sizeof(unsigned char)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] enable GOP=%d \n",enGOP);

    genGWIN=enGOP;

    // enable/disable gop switch at scaler
    if(enGOP)
        OUTREGMSK16(GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_60,0x0001,0x0001);
    else
        OUTREGMSK16(GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_60,0x0000,0x0001);

    // switch to subbank01
    SwitchSubBank(1);

    // enable/disable gwin
    if(enGOP)
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00,0x0001,0x0001);
    else
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00,0x0000,0x0001);

    // switch back to subbank00
    SwitchSubBank(0);

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

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    // alpha info is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;
    // switch to subbank01
    SwitchSubBank(1);

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

    // switch back to subbank00
    SwitchSubBank(0);

    genGOPALPHA.bEn=stCfg.bEn;
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

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    if (sg_videomemorysize<1)
    {
        FBDBG("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_Constant_Alpha, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }
    if(copy_from_user(&stCfg, (FB_GOP_ALPHA_CONFIG __user *)arg, sizeof(FB_GOP_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    FBDBG( "[FB Driver] Set alpha type=%d, constant alpha value=0x%x \n", (int)stCfg.enAlphaType, stCfg.u8Alpha);

    GOP_Set_Constant_Alpha(0, (int)stCfg.enAlphaType, stCfg.u8Alpha);

    genGOPALPHA.bEn=stCfg.bEn;
    genGOPALPHA.enAlphaType=stCfg.enAlphaType;
    genGOPALPHA.u8Alpha=stCfg.u8Alpha;

    return 0;
}

int _MDrv_FBIO_IOC_Get_Color_Key(unsigned long arg)
{
    unsigned long GOP_Reg_Base;
    FB_GOP_COLORKEY_CONFIG stCfg;

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

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

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    if (sg_videomemorysize<1)
    {
        FBDBG("[FB Driver] ERROR: _MDrv_FBIO_IOC_Set_Color_Key, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
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
    struct fb_image stCfg;
    //struct fb_image image;

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    if (sg_videomemorysize<1)
    {
        FBDBG("[FB Driver] ERROR: _MDrv_FBIO_IOC_imageblit, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }

    if(copy_from_user(&stCfg, (struct fb_image __user *)arg, sizeof(const struct fb_image)))
    {
        return -EFAULT;
    }

    if (NULL == stCfg.data)
    {
        FBDBG("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, fb_image.data is NULL pointer\r\n");
        return -EFAULT;
    }

    FBDBGMORE( "[FB Driver] Imgblit test, dst (x,y)=(%d,%d), width=%d, height=%d, colordepth=%d \n" ,stCfg.dx,stCfg.dy,stCfg.width,stCfg.height,stCfg.depth);

    sys_imageblit(info,&stCfg);

    return 0;
}

int _MDrv_FBIO_IOC_Set_Palette(unsigned long arg)
{
    FB_GOP_PaletteEntry stCfg[256];

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

    //int i=0;
    //struct fb_image image;

    /*if (sg_videomemorysize<1)
    {
        FBDBG("ERROR: _MDrv_FBIO_IOC_imageblit, memory is not set! Please use ioctl FBIOPUT_VSCREENINFO first!\n");
        return -ENOMEM;
    }*/

    if(copy_from_user(&stCfg, (FB_GOP_PaletteEntry __user *)arg, sizeof(FB_GOP_PaletteEntry)*256))
    {
        return -EFAULT;
    }

    //test palette
    /*for(i=0;i<=255;i++)
    {
        FBDBG("(%d,%d,%d,%d)\n",stCfg[i].BGRA.u8B,stCfg[i].BGRA.u8G,stCfg[i].BGRA.u8R,stCfg[i].BGRA.u8A);
    }*/

    mdrvinfinityfb_infinity_SetPalette(stCfg,0,255);

    return 0;
}

int _MDrv_FBIO_IOC_Get_General_Config(unsigned long arg)
{
    FB_GOP_RESOLUTION_STRETCH_H_CONFIG stCfg;

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

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
    int ret = 0;

    FBDBG("[FB Driver] %s, sg_videomemorysize=0x%x\n",__func__,(unsigned int)sg_videomemorysize);

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
    //max size for display is 1920*1088
    if(((stCfg.u32Height * stCfg.u32Width)>(1920*1088))||((stCfg.u32DisplayHeight* stCfg.u32DisplayWidth)>(1920*1088)))
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

    //after checking valid parameter
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

    info->screen_base = CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1);
    info->fix.line_length      = info->var.xres * ANDROID_BYTES_PER_PIXEL;

    GOP_Setmode_Stretch_H(FALSE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0,(int)genGOPGernelSettings.enStretchH_Ratio);

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
        FBDBG("[FB Driver] ERROR:in mdrvinfinityfb_ioctl, pinfo is NULL pointer\r\n");
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
        case IOCTL_FB_SETPALETTE:
            FBDBG( "[FB Driver] Set Palette.\n");
            ret=_MDrv_FBIO_IOC_Set_Palette(arg);
            break;
        case IOCTL_FB_GETGENERALCONFIG:
            FBDBG( "[FB Driver] Set Palette.\n");
            ret=_MDrv_FBIO_IOC_Get_General_Config(arg);
            break;
        case IOCTL_FB_SETGENERALCONFIG:
            FBDBG( "[FB Driver] Set Palette.\n");
            ret=_MDrv_FBIO_IOC_Set_General_Config(pinfo,arg);
            break;

        default:  /* redundant, as cmd was checked against MAXNR */
            FBDBG("[FB Driver] in default ioct\r\n");
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


//close for device tree structure

/*
+------------------------------------------------------------------------------
| FUNCTION    : mdrvinfinityfb_platform_release
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
/*
static void mdrvinfinityfb_platform_release(struct device *device)
{

    MstarFB_ENTER();

    if (NULL == device)
    {
        FBDBG("ERROR: in mdrvinfinityfb_platform_release, \
                device is NULL pointer !\r\n");
    }
    else
    {
        FBDBG("in mdrvinfinityfb_platform_release, module unload!\n");
    }

    MstarFB_LEAVE();
}
*/
#if 0
void    mdrvgop_DispCreateGWin2(struct fb_info *WG)
{

    int xx=0;
    xx++;

    FBDBG("fb%d: mdrv frame buffer device, of video memory\n", WG->node);
}
#endif
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

    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks ; clockIdxtemp++)
    {
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            if (IS_ERR(gop_clks[clockIdxtemp]))
            {
                FBDBG( "[FB Driver] ERROR: Fail to get gop clk! Clock idx=%d\n",clockIdxtemp);
                kfree(gop_clks);
                return ret;
            }
            else
            {
                clk_prepare_enable(gop_clks[clockIdxtemp]);
            }
        }
    }
/*
    if (IS_ERR(gop_psram_clk))
    {
        FBDBG( "[FB Driver] Error: Fail to get gop palette sram (psram) clk!\n" );
        return ret;
    }
    else
    {
        clk_prepare_enable(gop_psram_clk);
    }
*/
    FBDBG("[FB Driver] mdrvinfinityfb_resume \r\n");

    GOP_Setmode(TRUE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    //set palette
    mdrvinfinityfb_infinity_SetPalette(stGOPCurrentPalInfo,0,255);
/*
    if(unlikely(useCalibrationFrameBuffer))
    {
        fb2_PHY_ADD_SHOW = sg_G3D_fb2_bus_addr1 + ((sg_mdrvinfinityfb_default_var.xres * sg_mdrvinfinityfb_default_var.yres * ANDROID_BYTES_PER_PIXEL)*ANDROID_NUMBER_OF_BUFFERS);
        GOP_Setmode(TRUE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,fb2_PHY_ADD_SHOW,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
    }
    else
    {
        GOP_Setmode(TRUE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
    }
*/
    //printk("fb2 resume sleep ! \n");
     //msleep(5000);
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
    int fb_format=0;
    int clockIdxtemp=0;

    num_parents_clocks = of_clk_get_parent_count(dev->dev.of_node);
    gop_clks=kzalloc(((sizeof(struct clk *) * num_parents_clocks)),GFP_KERNEL);

    for(clockIdxtemp = 0 ; clockIdxtemp < num_parents_clocks ; clockIdxtemp++)
    {
        gop_clks[clockIdxtemp]= of_clk_get(dev->dev.of_node, clockIdxtemp);
        if(clockIdxtemp!=1)//index=1 is scl_fclk, only open when load palette
        {
            if (IS_ERR(gop_clks[clockIdxtemp]))
            {
                FBDBG( "[FB Driver] ERROR: Fail to get gop clk! Clock idx=%d\n",clockIdxtemp);
                kfree(gop_clks);
                return ret;
            }
            else
            {
                clk_prepare_enable(gop_clks[clockIdxtemp]);
            }
        }
    }

    //gop_psram_clk = gop_clks[0];
    //gop_scl_fclk1 = gop_clks[1];

/*
    if (IS_ERR(gop_psram_clk))
    {
        FBDBG( "[FB Driver] ERROR: Fail to get gop palette sram (psram) clk!\n" );
        return ret;
    }
    else
    {
        clk_prepare_enable(gop_psram_clk);
    }
*/
    //MstarFB_ENTER();

    mdrvinfinityfb_updateinfo();

    if (NULL == dev)
    {
        FBDBG("[FB Driver] ERROR: in mdrvinfinityfb_prob: dev is NULL pointer \r\n");
        return -ENOTTY;
    }

    fb_format=ANDROID_FB_FORMAT;

    // static global variable, pass at probe(), used by dma alloc and dma free in ioctl(),
    psg_mdrv_fbdev0 = dev;

    //FBDBG("mdrvinfinityfb: GOPmemorysize is 0x%X\n",(unsigned int)sg_videomemorysize);

    /* For G3D frame buffer2 */
    /*
    if (!(sg_pG3D_fb2_vir_addr1 = alloc_dmem(KEY_DMEM_FB_BUF,
                                                    PAGE_ALIGN(sg_videomemorysize),
                                                    &sg_G3D_fb2_bus_addr1)))
    {
        FBDBG("mdrvinfinityfb: unable to allocate screen memory\n");
        return retval;
    }
    */

    //printk("---------1 \n");
    //msleep(6000);
    // prevent initial garbage on screen ; to clean memory
    //memset(CAST_2_IOMEM_CHAR_P sg_pG3D_fb2_vir_addr1,OTHER_COLOR, sg_videomemorysize);

    //FBDBG("[FB Driver] mdrvinfinityfb vir=0x%x, phy=0x%x\n", (unsigned int)sg_pG3D_fb2_vir_addr1, sg_G3D_fb2_bus_addr1);

    sg_mdrvinfinityfb_fix.smem_start = sg_G3D_fb2_bus_addr1;
    sg_mdrvinfinityfb_fix.smem_len = sg_videomemorysize;

    pinfo = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
    if (!pinfo)
        goto err;


    pinfo->screen_base = CAST_2_IOMEM_CHAR_P( sg_pG3D_fb2_vir_addr1);
    //FBDBG( "[FB Driver] sg_mdrvinfinityfb_fix.smem_start=0x%x ",(unsigned int) sg_mdrvinfinityfb_fix.smem_start);

    //printk("---------2 \n");
    //msleep(6000);

    pinfo->fbops = &sg_mdrvinfinityfb_ops;
    pinfo->var = sg_mdrvinfinityfb_default_var;
    pinfo->fix = sg_mdrvinfinityfb_fix;
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
    pinfo->fix.line_length      = sg_mdrvinfinityfb_default_var.xres * ANDROID_BYTES_PER_PIXEL;
    pinfo->fix.accel            = FB_ACCEL_NONE;
    pinfo->fix.ypanstep         = 1;

    pinfo->var.xres             = sg_mdrvinfinityfb_default_var.xres;
    pinfo->var.yres             = sg_mdrvinfinityfb_default_var.yres;
    pinfo->var.xres_virtual     = sg_mdrvinfinityfb_default_var.xres;
    pinfo->var.yres_virtual     = sg_mdrvinfinityfb_default_var.yres * ANDROID_NUMBER_OF_BUFFERS;
    pinfo->var.bits_per_pixel   = BPX;
    pinfo->var.activate         = FB_ACTIVATE_NOW;
    pinfo->var.height           = sg_mdrvinfinityfb_default_var.yres;
    pinfo->var.width            = sg_mdrvinfinityfb_default_var.xres;
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
    else if(_8Bit_Palette==fb_format)
    {
        pinfo->var.grayscale = 0;

        pinfo->var.transp.length    = 0;
        pinfo->var.red.length       = 0;
        pinfo->var.green.length     = 0;
        pinfo->var.blue.length      = 0;

        pinfo->var.transp.offset    = 0;
        pinfo->var.red.offset       = 0;
        pinfo->var.green.offset     = 0;
        pinfo->var.blue.offset      = 0;
    }
    else
    {
        FBDBG( "[FB Driver] ERROR: mdrvinfinityfb_probe fb_format =%d Not Support!!\n", fb_format);
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
        modelist->mode.refresh=sg_mdrvinfinityfb_default_fbmode.refresh;
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


    GOP_Setmode(FALSE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr1,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);

    //set palette
    mdrvinfinityfb_infinity_SetPalette(DefaultPaletteEntry,0,255);

    //register sysfs
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

    //set to sca2 for test Infinity in cedric
    /*
    #define mdrv_BASE_REG_GOP00_PA        GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x123000/2))
    #define mdrv_BASE_REG_DIP             GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x133400/2))
    #define mdrv_BASE_REG_CLKGEN          GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x100B00/2))
    OUTREG16(mdrv_BASE_REG_GOP00_PA + (0x7E<<2),0x924);
    OUTREG16(mdrv_BASE_REG_GOP00_PA + (0x7B<<2),0x04);
    OUTREG16(mdrv_BASE_REG_GOP00_PA + (0x01<<2),((INREG16(mdrv_BASE_REG_GOP00_PA + (0x01<<2))&0xFFF0)|0x07));
    OUTREG16(mdrv_BASE_REG_DIP+ (0x10<<2),((INREG16(mdrv_BASE_REG_DIP + (0x10<<2)))&0xFFEE)|0x11);
    OUTREG16(mdrv_BASE_REG_CLKGEN+ (0x40<<2),((INREG16(mdrv_BASE_REG_CLKGEN + (0x40<<2)))&0xFFF0)|0x04);
    GOP_Set_OutFormat(0,1);
    */


   //#else
   //     GOP_Change_Alpha(FALSE, 0,sg_mdrvinfinityfb_default_var.xres,sg_mdrvinfinityfb_default_var.yres,sg_G3D_fb2_bus_addr2,ANDROID_FB_FORMAT,ANDROID_BYTES_PER_PIXEL, 0);
//   #endif


//    FBDBG("fb%d: mdrv frame buffer device, using %ldK of video memory\n", pinfo->node, sg_videomemorysize >> 10);

    //printk("---------5 \n");
    //msleep(6000);
    //MstarFB_LEAVE();
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


    //MstarFB_ENTER();


    if (NULL == dev)
    {
        FBDBG("[FB Driver] ERROR: mdrvinfinityfb_remove: dev is NULL pointer \n");
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

    //MstarFB_LEAVE();

    //clk_disable_unprepare(gop_psram_clk);


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

    //MstarFB_ENTER();

    //FBDBG( "mdrvinfinityfb_pan_display: var->yoffset=%x! info->var.height=%x\n ",var->yoffset,info->var.height);
    if(genGOPGernelSettings.enBufferNum==FB_DOUBLE_BUFFER)//double buffer
    {
        if (var->yoffset >= info->var.height)
        {
            fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start + (info->var.width * info->var.height * ANDROID_BYTES_PER_PIXEL);

            FBDBGMORE( "mdrvinfinityfb_pan_display: use buffer 1! Addr=%x\n ",fb2_pandisplay_PHY_ADD_SHOW);
        }
        else
        {
            fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start;

            FBDBGMORE( "mdrvinfinityfb_pan_display: use buffer 0 Addr=%x!\n ",fb2_pandisplay_PHY_ADD_SHOW);
        }
    }
    else//single buffer, always show first buffer
    {
        fb2_pandisplay_PHY_ADD_SHOW = info->fix.smem_start;

        FBDBGMORE( "mdrvinfinityfb_pan_display: use buffer 0 Addr=%x!\n ",fb2_pandisplay_PHY_ADD_SHOW);
    }

    GOP_Pan_Display(0, fb2_pandisplay_PHY_ADD_SHOW);

    //MstarFB_LEAVE();

    return 0;
}


//check var to see if supported by this device, defined for infinity
static int mdrvinfinityfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    int MiuBusLen=128;//miu bus length in iNfinity
    int DisplayPerPixel=0;

    DisplayPerPixel=MiuBusLen/(ANDROID_BYTES_PER_PIXEL*8);

    //MstarFB_ENTER();

    if((var->yres<1)||(var->xres<1))
    {
        FBDBG( "[FB Driver] Error: fb_check_var: buffer x or y size should be > 0 !\n ");
        return -EINVAL;
    }
    if((var->yres * var->xres)>(1920*1088))
    {
        FBDBG( "[FB Driver] Error: fb_check_var: total buffer size should be lower than 1920*1088!\n ");
        return -EINVAL;
    }

    if((var->xres % DisplayPerPixel)!=0)
    {
        FBDBG( "[FB Driver] Error: fb_check_var: Display Per Pixel for Buffer Width should be %d!\n ",DisplayPerPixel);
        return -EINVAL;
    }

    //MstarFB_LEAVE();

    return 0;
}

//close for device tree structure
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
/*
int _mdrvinfinityfb_init(void)
{
    int ret = 0;

    MstarFB_ENTER();

    //register the driver into kernel

    // how to get the Panel type and what kind of output display device


    //MApi_PNL_Init(tPanelNANOVISON_45);

    mdrvinfinityfb_updateinfo();

    ret = platform_driver_register(&sg_mdrvinfinityfb_driver);

    if (!ret)
    {   // register driver sucess
        // register device
        ret = platform_device_register(&sg_mdrvinfinityfb_device);
        if (ret)    // if register device fail, then unregister the driver.
        {
            platform_driver_unregister(&sg_mdrvinfinityfb_driver);
            printk(KERN_INFO"mdrvinfinityfb_device register failed...\n");
        }
        else
        {
            printk(KERN_INFO"mdrvinfinityfb_device register success...\n");
        }
    }

    MstarFB_LEAVE();

    return ret;
}
module_init(_mdrvinfinityfb_init);   //tell kernel where the init function
*/

module_platform_driver(sg_mdrvinfinityfb_driver);


#ifdef MODULE
/*
static void mdrvinfinityfb_platform_release(struct device *device)
{

    //MstarFB_ENTER();

    if (NULL == device)
    {
        FBDBG("ERROR: in mdrvinfinityfb_platform_release, \
                device is NULL pointer !\r\n");
    }
    else
    {
        FBDBG("in mdrvinfinityfb_platform_release, module unload!\n");
    }

    //MstarFB_LEAVE();
}

static int __init _mdrvinfinityfb_init(void)
{
    int ret = 0;

    //MstarFB_ENTER();

    //register the driver into kernel

    // how to get the Panel type and what kind of output display device


    //MApi_PNL_Init(tPanelNANOVISON_45);

    mdrvinfinityfb_updateinfo();

    ret = platform_driver_register(&sg_mdrvinfinityfb_driver);

    if (!ret)
    {   // register driver sucess
        // register device
        ret = platform_device_register(&sg_mdrvinfinityfb_device);
        if (ret)    // if register device fail, then unregister the driver.
        {
            platform_driver_unregister(&sg_mdrvinfinityfb_driver);
            printk(KERN_INFO"mdrvinfinityfb_device register failed...\n");
        }
        else
        {
            printk(KERN_INFO"mdrvinfinityfb_device register success...\n");
        }
    }

    //MstarFB_LEAVE();

    return ret;
}
module_init(_mdrvinfinityfb_init);   //tell kernel where the init function

static void __exit mdrvinfinityfb_exit(void)
{

       //MstarFB_ENTER();

       platform_device_unregister(&sg_mdrvinfinityfb_device);

       platform_driver_unregister(&sg_mdrvinfinityfb_driver);

       //MstarFB_LEAVE();

}

module_exit(mdrvinfinityfb_exit); // tel kernel where is the clean up function
*/

MODULE_LICENSE("GPL");
/* use GPL license to avoid Kernel to complain this driver */
#endif      /* MODULE */



//MODULE_LICENSE("GPL")
