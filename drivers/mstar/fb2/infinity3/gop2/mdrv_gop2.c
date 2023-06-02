#include "mdrv_gop2.h"

#define PARSING_FB_FORMAT(x)(\
        x == _8Bit_Palette ? "Palette_I8": \
        x == RGB565        ? "RGB565"    : \
        x == ARGB4444      ? "ARGB4444"  : \
        x == ARGB8888      ? "ARGB8888"  : \
        x == ARGB1555      ? "ARGB1555"  : \
        x == UV8Y8         ? "UV8Y8"     : \
                             "Format_Not_Support")

#define PARSING_ALPHA_TYPE(x)(\
        x == 0 ? "PIXEL_ALPHA": \
        x == 1 ? "CONST_ALPHA": \
                 "Wrong_Alpha_Type")


void GOP2_SwitchSubBank(int subbank)
{
    unsigned long GOP_Reg_Base=0;
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    if ((subbank>=0)&&(subbank<=2))
    {
        OUTREG16(GOP_Reg_Base+REG_GOP_7F,subbank);
        GOP2DBG( "[GOP2]%s: Switch to Subbank %d\n",__func__,subbank);
    }
    else
        GOP2DBG( "[GOP2]%s: Wrong Subbank %d\n",__func__,subbank);
}

void GOP2_Pan_Display(int Screen, U32  FB_Start_Addr )
{
    U32 FB_add       = FB_Start_Addr;
    U32 s32BuffAddr  = 0;
    U16 MIU_BUS      = MIU_BUS_ALIGN;
    U32 GOP_Reg_Base = 0;
    U16 GOP_Reg_DB   = 0;

    // 1. Pan display works by changing framebuffer start address


    GOP2DBG ("[GOP2]%s: in, addr=0x%x \n",__func__,FB_Start_Addr);

    /// GOP0_1 settings
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G2;
    GOP_Reg_Base = mdrv_BASE_REG_GOP21_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(1);
    }

    FB_add = (U32)Chip_Phys_to_MIU(FB_add);//map physical address to MIU address

    // GOP fetch framebuffer information
    // set the starting address for showing by GOP
    s32BuffAddr = FB_add;
    s32BuffAddr = s32BuffAddr&0xFFFFFFFF;
    s32BuffAddr = s32BuffAddr>>MIU_BUS; //128-bit unit = 16 bytes

    OUTREG16(GOP_Reg_Base+REG_GOP_01,(s32BuffAddr&0xFFFF)); //Ring block start Low address 0x21:16
    s32BuffAddr = s32BuffAddr>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_02,(s32BuffAddr&0xFFFF)); //Ring block start Hi address 0x22:6, page number

    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(0);
    }

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

    GOP2DBG ("[GOP2]%s: out \n",__func__);
}

void GOP2_Setmode(
    int bResume, int Screen,
    int Panel_Width,int  Panel_Height,U32  FB_Start_Addr,
    U32 Pixel_Format,U16 Bytes_Per_Pixel, int bInterlaceOut)
{
    U32 FB_add    = FB_Start_Addr;
    U16 FB_WIDTH  = Panel_Width;
    U16 FB_HEIGHT = Panel_Height;
    U16 FB_Bytes_Per_Pixel   = Bytes_Per_Pixel;
    U16 u16FrameBuffer_HSize = FB_WIDTH;
    U16 u16FrameBuffer_VSize = FB_HEIGHT;
    U16 u16FrameBuffer_Bytes_Per_Pixel = FB_Bytes_Per_Pixel;
    U16 u16DispImage_HStart  = 0;
    U16 u16DispImage_HSize   = FB_WIDTH;
    U16 u16DispImage_HEnd    = u16DispImage_HStart+u16DispImage_HSize;
    U16 u16DispImage_VStart  = 0;
    U16 u16DispImage_VSize   = FB_HEIGHT;
    U16 u16DispImage_VEnd    = u16DispImage_VStart+u16DispImage_VSize;
    U32 s32BuffAddr    = 0;
    U32 u32TempVal     = 0;
    U32 GOP_Reg_Base   = 0;
    U16 GOP_Reg_DB     = 0;
    U16 MIU_BUS        = MIU_BUS_ALIGN;
    U16 DISPLAY_Format = Pixel_Format;

    // 1. GOP Hardware Initial
    // 2. GOP Fetch Framebuffer Information
    // 3. GOP Display Settings


    //GOP2DBG ("GOP_Setmode Screen is %x.\r\n",Screen);
    GOP2DBG ("[GOP2]%s: FB_Width  = [%d]\n",__func__,FB_WIDTH);
    GOP2DBG ("[GOP2]%s: FB_Height = [%d]\n",__func__,FB_HEIGHT);
    GOP2DBG ("[GOP2]%s: FB_addr   = [0x%x]\n",__func__,FB_add);
    GOP2DBG ("[GOP2]%s: FB_Format = [%s]\n",__func__,PARSING_FB_FORMAT(DISPLAY_Format));
    GOP2DBG ("[GOP2]%s: FB_BPP    = [%d]\n",__func__,FB_Bytes_Per_Pixel);

    FB_add = (U32)Chip_Phys_to_MIU(FB_add);//map physical address to MIU address

    /// GOP0_0 settings
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G2;
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    // GOP global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV);//reset
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_ALPHA_INV|GOP_YUVOUT);//set Progress mode; mask Hsync; alpha inverse; YUV output
    OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
    OUTREG16(GOP_Reg_Base+REG_GOP_02,GOP_BLINK_DISABLE);//close blink
    OUTREG16(GOP_Reg_Base+REG_GOP_0A,0x0010);//insert fake rdy between hs & valid rdy
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,(Panel_Width >>1)+1);//miu efficiency = Stretch Window H size (unit:2 pixel) /2 +1
    OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY); //Hsync input pipe delay
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_19, GOP_BURST_LENGTH_MASK, GOP_BURST_LENGTH_MAX);// set gop dma burst length

    // GOP display area global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_30,Panel_Width >>1); //Stretch Window H size (unit:2 pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_31,Panel_Height);    //Stretch window V size
    OUTREG16(GOP_Reg_Base+REG_GOP_32,GOP_STRETCH_HST); //Stretch Window H coordinate
    OUTREG16(GOP_Reg_Base+REG_GOP_34,GOP_STRETCH_VST); //Stretch Window V coordinate
    OUTREG16(GOP_Reg_Base+REG_GOP_35,GOP_STRETCH_HRATIO); //Stretch H ratio
    OUTREG16(GOP_Reg_Base+REG_GOP_36,GOP_STRETCH_VRATIO); //Stretch V ratio
    OUTREG16(GOP_Reg_Base+REG_GOP_38,GOP_STRETCH_HINI); //Stretch H start value
    OUTREG16(GOP_Reg_Base+REG_GOP_39,GOP_STRETCH_VINI); //Stretch V start value


    /// GOP0_1 settings
    GOP_Reg_Base = mdrv_BASE_REG_GOP21_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(1);
    }

    // gwin global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_00,DISPLAY_Format|GOP0_GWin0_Enable|GOP_PIXEL_ALPHA_EN);//set gop format; enable gwin; alpha mode initial


    // framebuffer settings
    // framebuffer starting address
    s32BuffAddr = FB_add;
    s32BuffAddr = s32BuffAddr&0xFFFFFFFF;
    s32BuffAddr = s32BuffAddr>>MIU_BUS;//128-bit unit = 16 bytes
    OUTREG16(GOP_Reg_Base+REG_GOP_01,(s32BuffAddr&0xFFFF));
    s32BuffAddr=s32BuffAddr>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_02,(s32BuffAddr&0xFFFF));

    // framebuffer pitch
    OUTREG16(GOP_Reg_Base+REG_GOP_09,(u16FrameBuffer_HSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS);//bytes per line for gop framebuffer

    // framebuffer size
    u32TempVal=(u16FrameBuffer_HSize*u16FrameBuffer_VSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_10,u32TempVal&0xFFFF);
    u32TempVal=u32TempVal>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_11,u32TempVal);

    // framebuffer V start offset  (line)
    OUTREG16(GOP_Reg_Base+REG_GOP_0C,0);
    OUTREG16(GOP_Reg_Base+REG_GOP_0D,0);

    // framebuffer H start offset  (pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,0);


    // GOP display settings
    // GWIN display area in panel : : H start postion and end information
    u32TempVal=(u16DispImage_HStart*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_04,u32TempVal); //H start
    u32TempVal=(u16DispImage_HEnd*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_05,u32TempVal); //H end

    // GWIN  display area in panel : V start postion and end information
    OUTREG16(GOP_Reg_Base+REG_GOP_06,u16DispImage_VStart);//V start line
    OUTREG16(GOP_Reg_Base+REG_GOP_08,u16DispImage_VEnd);  //V end line


    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(0);
    }

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}



void GOP2_Set_OutFormat(int screen, int byuvoutput)
{
    U32 GOP_Reg_Base = 0;
    U16 GOP_Reg_DB   = 0;

    // 1. GOP Change Output Color Format (YUV/RGB)


    /// GOP0_0 settings
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G2;
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    GOP2DBG("[GOP2]%s: screen:%d, byuvoutput:%d\n",__func__, screen, byuvoutput);

    if(byuvoutput)
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, GOP_YUVOUT, GOP_YUVOUT);

    }
    else
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, 0, GOP_YUVOUT);
    }

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
}



void GOP2_Set_Constant_Alpha(int screen, int bEn, int alpha)
{
    U32 GOP_Reg_Base = 0;
    U16 GOP_Reg_DB   = 0;

    // 1. GOP Set Alpha Blending
    // 2. bEn= 0:pixel alpha; 1: constant alpha; for register: pixel_alpha=1, constant_alpha=0
    // 3. alpha: constant alpha value


    GOP2DBG("[GOP2]%s: screen:%d, bEn:%d, alpha:%x\n",__func__, screen, bEn, alpha);

    /// GOP0_0 settings
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G2;
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    // alpha inv
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, (bEn ? 0x0000 : GOP_ALPHA_INV), GOP_ALPHA_INV);//set alpha inverse when pixel alpha


    /// GOP0_1 settings
    GOP_Reg_Base = mdrv_BASE_REG_GOP21_PA;
    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(1);
    }

    // constant or pixel alpha
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, (bEn ? GOP_CONSTANT_ALPHA_EN : GOP_PIXEL_ALPHA_EN), GOP_ALPHA_MASK);

    // alpha value
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_0A, (bEn ? alpha : 0x0000), 0x00FF);//available for constant alpha; set 0 when pixel alpha

    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(0);
    }

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);


}


void GOP2_Set_Color_Key(int screen, int bEn, int u8R, int u8G, int u8B)
{
    U32 GOP_Reg_Base = 0;
    U16 GOP_Reg_DB   = 0;

    // 1. GOP Set RGB Color Key
    // 2. bEn= 0: disable; 1: enable
    // 3. u8R: red color value; u8G: green color value; u8B: blue color value ; value range: 0~255 (0x0~0xFF)


    GOP2DBG("[GOP2]%s: screen:%d, bEn:%d, (R=%x, G=%x, B=%x)\n",__func__, screen, bEn, u8R, u8G, u8B);

    /// GOP0_0 settings
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G2;
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    if(bEn)
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, GOP_RGB_TRANSPARENT_COLOR_ENABLE, GOP_RGB_TRANSPARENT_COLOR_MASK);
        OUTREG16(GOP_Reg_Base+REG_GOP_24, (u8B|(u8G<<8)));
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_25,  u8R, 0x00FF);
    }
    else
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, GOP_RGB_TRANSPARENT_COLOR_DISABLE, GOP_RGB_TRANSPARENT_COLOR_MASK);
    }

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}

void GOP2_Set_PipeDelay(int screen, int delay)
{
    U16 pipe_delay   = delay;
    U32 GOP_Reg_Base = 0;
    U16 GOP_Reg_DB   = 0;

    // 1. GOP Set Pipe Delay


    GOP2DBG("[GOP2]%s: screen:%d, delay:%d\n",__func__, screen, delay);

    /// GOP0_0 settings
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G2;
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    OUTREG16(GOP_Reg_Base+REG_GOP_0F, pipe_delay);

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR);
}

void GOP2_Set_Palette_RIU(int screen, int u8Index, int u8A, int u8R, int u8G, int u8B)
{
    U32 GOP_Reg_Base = 0;

    // 1. GOP Set Palette by RIU Mode
    // 2. GOP Palette SRAM Clock should be opened
    // 3. Set Force Write
    // 4. Set RIU Mode
    // 5. Set Palette Index, A, R, G, B Values
    // 6. Trigger Palette Write
    // 7. Close Force Write

    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    //set force write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_BANK_FORCE_WR , GOP_BANK_FORCE_WR);

    //set RIU mode
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_05, GOP_PALETTE_SRAM_CTRL_RIU , GOP_PALETTE_SRAM_CTRL_MASK);

    //set palette value
    OUTREG16(GOP_Reg_Base+REG_GOP_03, u8G<<8|u8B);
    OUTREG16(GOP_Reg_Base+REG_GOP_04, u8A<<8|u8R);

    //set palette index
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_05, u8Index , GOP_PALETTE_TABLE_ADDRESS_MASK);

    //write trigger
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_05, 0x0100 , GOP_PALETTE_WRITE_ENABLE_MASK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_05, 0x0000 , GOP_PALETTE_WRITE_ENABLE_MASK);

    //close force write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0x0000 , GOP_BANK_FORCE_WR);

}

void GOP2_Set_Outputmode(int Screen, int Panel_Width, int Panel_Height,U32 FB_Start_Addr, U32 Pixel_Format, U16 Bytes_Per_Pixel)
{
    U32 FB_add    = FB_Start_Addr;
    U16 FB_WIDTH  = Panel_Width;
    U16 FB_HEIGHT = Panel_Height;
    U16 FB_Bytes_Per_Pixel   = Bytes_Per_Pixel;
    U16 u16FrameBuffer_HSize = FB_WIDTH;
    U16 u16FrameBuffer_VSize = FB_HEIGHT;
    U16 u16FrameBuffer_Bytes_Per_Pixel = FB_Bytes_Per_Pixel;
    U16 u16DispImage_HStart  = 0;
    U16 u16DispImage_HSize   = FB_WIDTH;
    U16 u16DispImage_HEnd    = u16DispImage_HStart+u16DispImage_HSize;
    U16 u16DispImage_VStart  = 0;
    U16 u16DispImage_VSize   = FB_HEIGHT;
    U16 u16DispImage_VEnd    = u16DispImage_VStart+u16DispImage_VSize;
    U32 s32BuffAddr    = 0;
    U32 u32TempVal     = 0;
    U32 GOP_Reg_Base   = 0;
    U16 GOP_Reg_DB     = 0;
    U16 MIU_BUS        = MIU_BUS_ALIGN;
    U16 DISPLAY_Format = Pixel_Format;

    // 1. GOP Set Output Mode
    // 2. GOP Fetch Framebuffer Information
    // 3. GOP Display Settings

    GOP2DBG ("[GOP2]%s\n",__func__);
    GOP2DBG ("[GOP2]%s: FB_Width  = [%d]\n",__func__,FB_WIDTH);
    GOP2DBG ("[GOP2]%s: FB_Height = [%d]\n",__func__,FB_HEIGHT);
    GOP2DBG ("[GOP2]%s: FB_addr   = [0x%x]\n",__func__,FB_add);
    GOP2DBG ("[GOP2]%s: FB_Format = [%s]\n",__func__,PARSING_FB_FORMAT(DISPLAY_Format));
    GOP2DBG ("[GOP2]%s: FB_BPP    = [%d]\n",__func__,FB_Bytes_Per_Pixel);

    FB_add = (U32)Chip_Phys_to_MIU(FB_add);//map physical address to MIU address

    /// GOP0_0 settings
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G2;
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    // GOP global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,(Panel_Width >>1)+1);//miu efficiency = Stretch Window H size (unit:2 pixel) /2 +1

    // GOP display area global settings
    OUTREG16(GOP_Reg_Base+REG_GOP_30,Panel_Width >>1); //Stretch Window H size (unit:2 pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_31,Panel_Height);    //Stretch window V size


    /// GOP0_1 settings
    GOP_Reg_Base = mdrv_BASE_REG_GOP21_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(1);
    }

    // gwin global settings
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, DISPLAY_Format, GWIN_FMT_MASK);

    // framebuffer settings
    // framebuffer starting address
    s32BuffAddr = FB_add;
    s32BuffAddr = s32BuffAddr&0xFFFFFFFF;
    s32BuffAddr = s32BuffAddr>>MIU_BUS;//128-bit unit = 16 bytes
    OUTREG16(GOP_Reg_Base+REG_GOP_01,(s32BuffAddr&0xFFFF));
    s32BuffAddr=s32BuffAddr>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_02,(s32BuffAddr&0xFFFF));

    // framebuffer pitch
    OUTREG16(GOP_Reg_Base+REG_GOP_09,(u16FrameBuffer_HSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS);//bytes per line for gop framebuffer

    // framebuffer size, use for scroll; useless in I1, I3
    u32TempVal=(u16FrameBuffer_HSize*u16FrameBuffer_VSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_10,u32TempVal&0xFFFF);
    u32TempVal=u32TempVal>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_11,u32TempVal);

    // framebuffer V start offset  (line)
    OUTREG16(GOP_Reg_Base+REG_GOP_0C,0);
    OUTREG16(GOP_Reg_Base+REG_GOP_0D,0);

    // framebuffer H start offset  (pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,0);


    // GOP display settings
    // GWIN display area in panel : : H start postion and end information
    u32TempVal=(u16DispImage_HStart*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_04,u32TempVal); //H start
    u32TempVal=(u16DispImage_HEnd*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_05,u32TempVal); //H end

    // GWIN  display area in panel : V start postion and end information
    OUTREG16(GOP_Reg_Base+REG_GOP_06,u16DispImage_VStart);//V start line
    OUTREG16(GOP_Reg_Base+REG_GOP_08,u16DispImage_VEnd);  //V end line


    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(0);
    }

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}

void GOP2_Set_Enable_GWIN(int screen, unsigned char bEn)
{
    unsigned long GOP_Reg_Base = 0;
    unsigned char enGOP        = bEn;
    unsigned long GOP_Reg_DB   = 0x0400;

    // 1. GOP Set GWIN Enable/Disable

    GOP2DBG( "[GOP2]%s: Set enable_GOP = [%d]\n",__func__,enGOP);

    // enable gwin is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP21_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(1);
    }

    // enable/disable gwin
    if(enGOP)
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, enGOP, GOP0_GWin0_Enable);
    else
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, 0, GOP0_GWin0_Enable);

    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(0);
    }

    //write gop register
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}

void GOP2_Get_Enable_GWIN(int screen, unsigned char *bEn)
{
    unsigned long GOP_Reg_Base = 0;

    // 1. GOP Get GWIN Enable/Disable

    // enable gwin is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP21_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(1);
    }

    if(INREGMSK16(GOP_Reg_Base+REG_GOP_00, GOP0_GWin0_Enable))
    {
        *bEn  = 1;
    }
    else
    {
        *bEn  = 0;
    }

    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(0);
    }

    GOP2DBG( "[GOP2]%s: Get enable_GOP = [%d]\n",__func__,enGOP);

}

void GOP2_Get_Alpha(int screen, int *alpha_type, int *alpha_val)
{
    unsigned long GOP_Reg_Base = 0;

    // 1. GOP Get Alpha Blending
    // 2. alpha_type= 0:pixel_alpha; 1: constant_alpha; for register: pixel_alpha=1, constant_alpha=0
    // 3. alpha_val: constant alpha value

    // alpha info is in GOP01
    GOP_Reg_Base = mdrv_BASE_REG_GOP21_PA;

    // switch to subbank01 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(1);
    }

    // constant or pixel alpha
    if(INREGMSK16(GOP_Reg_Base+REG_GOP_00, GOP_PIXEL_ALPHA_EN)==GOP_PIXEL_ALPHA_EN)
        *alpha_type = 0; //pixel_alpha
    else
        *alpha_type = 1; //constant_alpha

    // alpha value
    *alpha_val = (unsigned char)INREGMSK16(GOP_Reg_Base+REG_GOP_0A, 0x00FF);

    // switch back to subbank00 if there's no direct bank
    if(mdrv_BASE_REG_GOP21_PA==mdrv_BASE_REG_GOP20_PA)
    {
        GOP2_SwitchSubBank(0);
    }

    GOP2DBG( "[GOP2]%s: Alpha_type = [%s], Alpha_value = [%d]\n",__func__,PARSING_ALPHA_TYPE(*alpha_type),*alpha_val);

}

void GOP2_Get_Color_Key(int screen, int *bEn, int *u8R, int *u8G, int *u8B)
{
    unsigned long GOP_Reg_Base;

    // 1. GOP Get RGB Color Key
    // 2. bEn= 0: disable; 1: enable
    // 3. u8R: red color value; u8G: green color value; u8B: blue color value ; value range: 0~255 (0x0~0xFF)

    // colorkey setting is in GOP00
    GOP_Reg_Base = mdrv_BASE_REG_GOP20_PA;

    // enable/disable
    *bEn = (INREGMSK16(GOP_Reg_Base+REG_GOP_00,0x0800)>>11);

    // colorkey value
    *u8R = (unsigned char)INREGMSK16(GOP_Reg_Base+REG_GOP_25,0x00FF);
    *u8G = (unsigned char)(INREGMSK16(GOP_Reg_Base+REG_GOP_24,0xFF00)>>8);
    *u8B = (unsigned char)INREGMSK16(GOP_Reg_Base+REG_GOP_24,0x00FF);

    GOP2DBG("[GOP2]%s: screen:%d, bEn:%d, (R=%x, G=%x, B=%x)\n",__func__, screen, *bEn, *u8R, *u8G, *u8B);

}
