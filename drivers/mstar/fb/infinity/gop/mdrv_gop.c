
#include "mdrv_gop.h"

unsigned long long ms_mem_bus_to_MIU(unsigned long long x)
{
  #define MIU0_BUS_ADDR           0x40000000

	return ((x) - MIU0_BUS_ADDR);
}


U32  OALReadDeviceId(U32 devId)
{
    U16 tmp = 0;
    // need to refine for another SOC
    tmp = 0x50; //C3
    return tmp;
}

void SwitchSubBank(int subbank)
{
    unsigned long GOP_Reg_Base;
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;
    if (subbank==1)
        OUTREG16(GOP_Reg_Base+REG_GOP_7F,0x0001);
    else if (subbank==0)
        OUTREG16(GOP_Reg_Base+REG_GOP_7F,0x0000);
    else
        GOPDBG( "GOP: Wrong Subbank in SwitchSubBank \n");
}



U16 GOP_MIU_BUS(U16 MIU_BUS)
{
    // A3: MIU Bus = 0x5;
    // C2: MIU Bus = 0x3;
    // C3: MIU Bus = 0x4;
    // C4: MIU Bus = 0x3;
    // Need to check with HW team

    U32 devId=0;

    devId=OALReadDeviceId(devId);

    if(DEVICE_AMBER3==devId)
    {
        GOPDBG("*****   devId is AMBER3   *****\r\n");
        MIU_BUS = 5;
    }
    else if(DEVICE_CEDRIC==devId)
    {
        GOPDBG("*****   devId is CEDRIC    *****\r\n");
        MIU_BUS = 0x4;
    }
    else if(DEVICE_CHICAGO==devId)
    {
        GOPDBG("*****   devId is DEVICE_CHICAGO    *****\r\n");
        MIU_BUS = 0x3;
    }
    else
    {
        GOPDBG("*****   devId  UNKNOWN   *****\r\n");
        MIU_BUS = 0x4;

    }

    GOPDBG("GOP_MIU_BUS devId is %d BUS is %d \n",devId,MIU_BUS);


    return MIU_BUS;
}



void GOP_Pan_Display(int Screen, U32  FB_Start_Addr )
{

    U32 FB_add = FB_Start_Addr;

    U32 s32BuffAddr=0;
    U16 MIU_BUS=0x04;

    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    // infinity has only one screen
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G0;
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA ;
    // switch to subbank01
    SwitchSubBank(1);

    //FB_add = (U32)ms_mem_bus_to_MIU(FB_add);//old way
    FB_add = (U32)Chip_Phys_to_MIU(FB_add);//new way

    MIU_BUS= GOP_MIU_BUS(MIU_BUS);

    GOPDBG ("GOP_Pan_Display: in \r\n");

    // 1  GOP fetch framebuffer information
    // 1.1 framebuffer starting address
    s32BuffAddr = FB_add;
    s32BuffAddr=s32BuffAddr&0xFFFFFFFF;
    s32BuffAddr=s32BuffAddr>>MIU_BUS; // 128-bit unit = 16 bytes

    OUTREG16(GOP_Reg_Base+REG_GOP_01,(s32BuffAddr&0xFFFF)); // Ring block start Low address 0x21:16
    s32BuffAddr=s32BuffAddr>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_02,(s32BuffAddr&0xFFFF)); // Ring block start Hi address 0x22:6, page number

    // switch back to subbank00
    SwitchSubBank(0);

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

    GOPDBG ("GOP_Pan_Display: out \r\n");
}

void GOP_Change_Alpha(int bResume, int Screen,int Panel_Width,int  Panel_Height,U32  FB_Start_Addr,U32 Pixel_Format,U16 Bytes_Per_Pixel, int bInterlaceOut)
{
    // 1. GOP Hardware initail (need to check Chip top colok)
    // 2. GOP fetch framebuffer information
    // 3. GOP Display Setting
    // 4. Initial Mux for Select GOP and Sclaer(MApi_XC_ip_sel_for_gop)



    U32 FB_add = FB_Start_Addr;
    //U16 FB_WIDTH=Panel_Width;
    //U16 FB_HEIGHT=Panel_Height;


    //U16 FB_Bytes_Per_Pixel=Bytes_Per_Pixel;

    //U16 u16FrameBuffer_HSize=FB_WIDTH;//LCD_PANEL_WIDTH;
    //U16 u16FrameBuffer_VSize= FB_HEIGHT;//mdrvgop_Get_Panel_Height();//mdrvgop_Get_Panel_HeightYRES;//LCD_PANEL_HEIGHT;
    //U16 u16FrameBuffer_Bytes_Per_Pixel=FB_Bytes_Per_Pixel;

    //U16 u16DispImage_HStart=0;
    //U16 u16DispImage_HSize=FB_WIDTH;
    //U16 u16DispImage_HEnd=u16DispImage_HStart+u16DispImage_HSize;

    //U16 u16DispImage_VStart=000;
    //U16 u16DispImage_VSize=FB_HEIGHT;
    //U16 u16DispImage_VEnd=u16DispImage_VStart+u16DispImage_VSize;

    //U32 s32BuffAddr=0;
    //U32 u32TempVal=0;
    U16 MIU_BUS=0x04;

    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    //U16 DISPLAY_Format=Pixel_Format;

    //FB_add = (U32)ms_mem_bus_to_MIU(FB_add);//old way
    FB_add = (U32)Chip_Phys_to_MIU(FB_add);//new way

    MIU_BUS= GOP_MIU_BUS(MIU_BUS);


    GOPDBG ("GOP_Setmode Screen is %x.\r\n",Screen);


    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G0;

    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    // infinity has only one screen
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV); //
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_ALPHA_INV); // PerPixel alpha 0x3F, RGB8888, enable
    OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_OP|GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
    OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay


    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}

void GOP_Setmode(
    int bResume, int Screen,
    int Panel_Width,int  Panel_Height,U32  FB_Start_Addr,
    U32 Pixel_Format,U16 Bytes_Per_Pixel, int bInterlaceOut)
{



    // 1. GOP Hardware initail (need to check Chip top colok)
    // 2. GOP fetch framebuffer information
    // 3. GOP Display Setting
    // 4. Initial Mux for Select GOP and Sclaer(MApi_XC_ip_sel_for_gop)



    U32 FB_add = FB_Start_Addr;
    U16 FB_WIDTH=Panel_Width;
    U16 FB_HEIGHT=Panel_Height;


    U16 FB_Bytes_Per_Pixel=Bytes_Per_Pixel;

    U16 u16FrameBuffer_HSize=FB_WIDTH;//LCD_PANEL_WIDTH;
    U16 u16FrameBuffer_VSize= FB_HEIGHT;//mdrvgop_Get_Panel_Height();//mdrvgop_Get_Panel_HeightYRES;//LCD_PANEL_HEIGHT;
    U16 u16FrameBuffer_Bytes_Per_Pixel=FB_Bytes_Per_Pixel;

    U16 u16DispImage_HStart=0;
    U16 u16DispImage_HSize=FB_WIDTH;
    U16 u16DispImage_HEnd=u16DispImage_HStart+u16DispImage_HSize;

    U16 u16DispImage_VStart=000;
    U16 u16DispImage_VSize=FB_HEIGHT;
    U16 u16DispImage_VEnd=u16DispImage_VStart+u16DispImage_VSize;

    U32 s32BuffAddr=0;
    U32 u32TempVal=0;

    U32 devId=0x50;

    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    U16 MIU_BUS=0x04;

    U16 GOP_block_0=GOP_BANK_G0_0;
    U16 GOP_block_1=GOP_BANK_G0_1;
    U16 DISPLAY_Format=Pixel_Format;

    //FB_add = (U32)ms_mem_bus_to_MIU(FB_add);//old way
    FB_add = (U32)Chip_Phys_to_MIU(FB_add);//new way

    MIU_BUS= GOP_MIU_BUS(MIU_BUS);
    devId = OALReadDeviceId(devId);

    GOPDBG ("GOP_Setmode Screen is %x.\r\n",Screen);

    // infinity has only one screen
    GOP_block_0=GOP_BANK_G0_0;
    GOP_block_1=GOP_BANK_G0_1;

    GOPDBG ("GOP_Setmode: LCD_PANEL_WIDTH = [%d] \r\n",Panel_Width);
    GOPDBG ("GOP_Setmode: LCD_PANEL_HEIGHT = [%d] \r\n",Panel_Height);
    GOPDBG ("GOP_Setmode: FB_WIDTH = [%d] \r\n",FB_WIDTH);
    GOPDBG ("GOP_Setmode: FB_HEIGHT = [%d] \r\n",FB_HEIGHT);
    GOPDBG ("GOP_Setmode: FB_add = [%x] \r\n",FB_add);
    GOPDBG ("GOP_Setmode: FB_Start_Addr = [%x] \r\n",FB_Start_Addr);

    // infinity has only one screen
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G0;
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    // GOP Global seting ; destination, Hsync
    // infinity has only one screen
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV); //
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_ALPHA_INV|GOP_YUVOUT); // PerPixel alpha 0x3F, RGB8888, enable
    OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
    OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay


    OUTREG16(GOP_Reg_Base+REG_GOP_02,GOP_BLINK_DISABLE);

     //  GOP : Display Area global seting

    OUTREG16(GOP_Reg_Base+REG_GOP_30,Panel_Width >>1); // Stretch Window H size (unit:2 pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_31,Panel_Height); // Stretch window V size
    OUTREG16(GOP_Reg_Base+REG_GOP_32,GOP_STRETCH_HST); // Stretch Window H coordinate
    OUTREG16(GOP_Reg_Base+REG_GOP_34,GOP_STRETCH_VST); // Stretch Window V coordinate
    OUTREG16(GOP_Reg_Base+REG_GOP_35,GOP_STRETCH_HRATIO); // Stretch H ratio
    OUTREG16(GOP_Reg_Base+REG_GOP_36,GOP_STRETCH_VRATIO); // Stretch V ratio
    OUTREG16(GOP_Reg_Base+REG_GOP_38,GOP_STRETCH_HINI); // Stretch H start value
    OUTREG16(GOP_Reg_Base+REG_GOP_39,GOP_STRETCH_VINI); // Stretch V start value

    OUTREG16(GOP_Reg_Base+REG_GOP_0E,(Panel_Width >>1)+1); // miu efficiency = Stretch Window H size (unit:2 pixel) /2 +1
    OUTREG16(GOP_Reg_Base+REG_GOP_0A,0x0010); // insert fake rdy between hs & valid rdy

    // set gop dma burst length
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_19, 0x1F00, 0x1F00);

    //1  GOP fetch framebuffer information
    // 1.0 framebuffer Data format

    // infinity has only one screen
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    // set GOP switch from scaler initially open
    //OUTREGMSK16(GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_60,0x0001,0x0001);

    // switch to subbank01
    SwitchSubBank(1);

    OUTREG16(GOP_Reg_Base+REG_GOP_00,DISPLAY_Format|GOP0_GWin0_Enable|GOP_PIXEL_ALPHA_EN);

    // 1.1 framebuffer starting address
    s32BuffAddr = FB_add;
    s32BuffAddr=s32BuffAddr&0xFFFFFFFF;
    s32BuffAddr=s32BuffAddr>>MIU_BUS; // 128-bit unit = 16 bytes


    OUTREG16(GOP_Reg_Base+REG_GOP_01,(s32BuffAddr&0xFFFF)); // Ring block start Low address 0x21:16
    s32BuffAddr=s32BuffAddr>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_02,(s32BuffAddr&0xFFFF)); // Ring block start Hi address 0x22:6, page number

    // 1.2 framebuffer Pitch
    OUTREG16(GOP_Reg_Base+REG_GOP_09,(u16FrameBuffer_HSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS); // Ring block 0 line size

    // 1.4 framebuffer Size
    u32TempVal=(u16FrameBuffer_HSize*u16FrameBuffer_VSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_10,u32TempVal&0xFFFF);
    u32TempVal=u32TempVal>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_11,u32TempVal);

    // 1.5 frame buffer V start offset  (line)
    OUTREG16(GOP_Reg_Base+REG_GOP_0C,0);
    OUTREG16(GOP_Reg_Base+REG_GOP_0D,0);

    // 1.6 frame buffer H start offset  (pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,0);


    //2  GOP Display Setting
    // 2.1 GWIN display area in panel : : H Start postion and end information
    u32TempVal=(u16DispImage_HStart*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_04,u32TempVal); // H start
    u32TempVal=(u16DispImage_HEnd*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_05,u32TempVal); // H end

    // 2.2 GWIN  display area in panel : V Start postion and end information
    OUTREG16(GOP_Reg_Base+REG_GOP_06,u16DispImage_VStart); // V start line
    OUTREG16(GOP_Reg_Base+REG_GOP_08,u16DispImage_VEnd); // V end line

    // 2.3 Display Area setting in Panel
    CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_GOPG0_MUX_MASK);
    OUTREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer1_Sel_GOP_Null);

    // switch back to subbank00
    SwitchSubBank(0);

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}

void GOP_Setmode_Stretch_H(
    int bResume, int Screen,
    int Panel_Width,int  Panel_Height,U32  FB_Start_Addr,
    U32 Pixel_Format,U16 Bytes_Per_Pixel, int bInterlaceOut, int StretchRatio)
{

    // 1. GOP Hardware initial
    // 2. GOP fetch framebuffer information
    // 3. GOP Display Setting
    // 4. Set Stretch Ratio for GOP Display

    U32 FB_add = FB_Start_Addr;
    U16 FB_WIDTH=Panel_Width;
    U16 FB_HEIGHT=Panel_Height;


    U16 FB_Bytes_Per_Pixel=Bytes_Per_Pixel;

    U16 u16FrameBuffer_HSize=FB_WIDTH;
    U16 u16FrameBuffer_VSize= FB_HEIGHT;
    U16 u16FrameBuffer_Bytes_Per_Pixel=FB_Bytes_Per_Pixel;

    U16 u16DispImage_HStart=0;
    U16 u16DispImage_HSize=FB_WIDTH;
    U16 u16DispImage_HEnd=u16DispImage_HStart+u16DispImage_HSize;

    U16 u16DispImage_VStart=000;
    U16 u16DispImage_VSize=FB_HEIGHT;
    U16 u16DispImage_VEnd=u16DispImage_VStart+u16DispImage_VSize;

    U32 s32BuffAddr=0;
    U32 u32TempVal=0;

    U32 devId=0x50;

    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    U16 MIU_BUS=0x04;

    U16 GOP_block_0=GOP_BANK_G0_0;
    U16 GOP_block_1=GOP_BANK_G0_1;
    U16 DISPLAY_Format=Pixel_Format;

    //FB_add = (U32)ms_mem_bus_to_MIU(FB_add);//old way
    FB_add = (U32)Chip_Phys_to_MIU(FB_add);//new way

    MIU_BUS= GOP_MIU_BUS(MIU_BUS);
    devId = OALReadDeviceId(devId);

    GOPDBG ("GOP_Setmode Screen is %x.\r\n",Screen);

    // infinity has only one screen
    GOP_block_0=GOP_BANK_G0_0;
    GOP_block_1=GOP_BANK_G0_1;

    GOPDBG ("GOP_Setmode_Stretch: LCD_PANEL_WIDTH = [%d] \r\n",Panel_Width);
    GOPDBG ("GOP_Setmode_Stretch: LCD_PANEL_HEIGHT = [%d] \r\n",Panel_Height);
    GOPDBG ("GOP_Setmode_Stretch: FB_WIDTH = [%d] \r\n",FB_WIDTH);
    GOPDBG ("GOP_Setmode_Stretch: FB_HEIGHT = [%d] \r\n",FB_HEIGHT);
    GOPDBG ("GOP_Setmode_Stretch: FB_add = [%x] \r\n",FB_add);
    GOPDBG ("GOP_Setmode_Stretch: FB_Start_Addr = [%x] \r\n",FB_Start_Addr);
    GOPDBG ("GOP_Setmode_Stretch: FB_StretchRatio = [%x] \r\n",StretchRatio);

    // infinity has only one screen
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G0;
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    // GOP Global seting ; destination, Hsync
    // infinity has only one screen
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV); //
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_ALPHA_INV|GOP_YUVOUT); // PerPixel alpha 0x3F, RGB8888, enable
    OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
    OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay


    OUTREG16(GOP_Reg_Base+REG_GOP_02,GOP_BLINK_DISABLE);

     //  GOP : Display Area global seting

    OUTREG16(GOP_Reg_Base+REG_GOP_30,(Panel_Width >>1)); // Stretch Window H size (unit:2 pixel), for Stretch Ratio: Window/2/Ratio
    OUTREG16(GOP_Reg_Base+REG_GOP_31,Panel_Height); // Stretch window V size
    OUTREG16(GOP_Reg_Base+REG_GOP_32,GOP_STRETCH_HST); // Stretch Window H coordinate
    OUTREG16(GOP_Reg_Base+REG_GOP_34,GOP_STRETCH_VST); // Stretch Window V coordinate
    OUTREG16(GOP_Reg_Base+REG_GOP_35,GOP_STRETCH_HRATIO/StretchRatio); // Stretch H ratio, val=0x1000/StretchRatio
    OUTREG16(GOP_Reg_Base+REG_GOP_36,GOP_STRETCH_VRATIO); // Stretch V ratio
    OUTREG16(GOP_Reg_Base+REG_GOP_38,GOP_STRETCH_HINI); // Stretch H start value
    OUTREG16(GOP_Reg_Base+REG_GOP_39,GOP_STRETCH_VINI); // Stretch V start value

    OUTREG16(GOP_Reg_Base+REG_GOP_0E,(Panel_Width >>1)+1); // miu efficiency = Stretch Window H size (unit:2 pixel) /2 +1
    OUTREG16(GOP_Reg_Base+REG_GOP_0A,0x0010); // insert fake rdy between hs & valid rdy

    // set gop dma burst length
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_19, 0x1F00, 0x1F00);

    //1  GOP fetch framebuffer information
    // 1.0 framebuffer Data format

    // infinity has only one screen
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;

    // set GOP switch from scaler initially open
    //OUTREGMSK16(GET_REG16_ADDR(COLUMBUS_BASE_REG_RIU_PA, (0x121A00/2))+REG_GOP_60,0x0001,0x0001);

    // switch to subbank01
    SwitchSubBank(1);

    OUTREG16(GOP_Reg_Base+REG_GOP_00,DISPLAY_Format|GOP0_GWin0_Enable|GOP_PIXEL_ALPHA_EN);

    // 1.1 framebuffer starting address
    s32BuffAddr = FB_add;
    s32BuffAddr=s32BuffAddr&0xFFFFFFFF;
    s32BuffAddr=s32BuffAddr>>MIU_BUS; // 128-bit unit = 16 bytes


    OUTREG16(GOP_Reg_Base+REG_GOP_01,(s32BuffAddr&0xFFFF)); // Ring block start Low address 0x21:16
    s32BuffAddr=s32BuffAddr>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_02,(s32BuffAddr&0xFFFF)); // Ring block start Hi address 0x22:6, page number

    // 1.2 framebuffer Pitch
    OUTREG16(GOP_Reg_Base+REG_GOP_09,(u16FrameBuffer_HSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS); // Ring block 0 line size

    // 1.4 framebuffer Size
    u32TempVal=(u16FrameBuffer_HSize*u16FrameBuffer_VSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_10,u32TempVal&0xFFFF);
    u32TempVal=u32TempVal>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_11,u32TempVal);

    // 1.5 frame buffer V start offset  (line)
    OUTREG16(GOP_Reg_Base+REG_GOP_0C,0);
    OUTREG16(GOP_Reg_Base+REG_GOP_0D,0);

    // 1.6 frame buffer H start offset  (pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,0);


    //2  GOP Display Setting
    // 2.1 GWIN display area in panel : : H Start postion and end information
    u32TempVal=(u16DispImage_HStart*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_04,u32TempVal); // H start
    u32TempVal=(u16DispImage_HEnd*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_05,u32TempVal); // H end

    // 2.2 GWIN  display area in panel : V Start postion and end information
    OUTREG16(GOP_Reg_Base+REG_GOP_06,u16DispImage_VStart); // V start line
    OUTREG16(GOP_Reg_Base+REG_GOP_08,u16DispImage_VEnd); // V end line

    // 2.3 Display Area setting in Panel
    CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_GOPG0_MUX_MASK);
    OUTREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer1_Sel_GOP_Null);

    // switch back to subbank00
    SwitchSubBank(0);

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}



void GOP_Set_OutFormat(int screen, int byuvoutput)
{
    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    // infinity has only one screen
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G0;
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    GOPDBG("GOP_Set_OutFormat: screen:%d, byuvoutput:%d\n", screen, byuvoutput);

    if(byuvoutput)
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, GOP_YUVOUT, GOP_YUVOUT);

    }
    else
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, 0, GOP_YUVOUT);
    }

    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
}



void GOP_Set_Constant_Alpha(int screen, int bEn, int alpha)
{
    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    GOP_DBG(GOP_DBG_LV_0, "SetConstatnAlpha: screen:%d, bEn:%d, alpha:%x\n", screen, bEn, alpha);

    // infinity has only one screen
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G0;
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    // alpha inv
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, (bEn ? 0x0000 : 0x8000), 0x8000);

    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;
    // switch to subbank01
    SwitchSubBank(1);

    // constant or pixel alpha
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, (bEn ? 0x0000 : 0x4000), 0x4000);

    // alhap value
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_0A, (bEn ? alpha : 0x0000), 0x00FF);

    // switch back to subbank00
    SwitchSubBank(0);


    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);


}


void GOP_Set_Color_Key(int screen, int bEn, int u8R, int u8G, int u8B)
{
    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    GOP_DBG(GOP_DBG_LV_0, "SetColorKey: screen:%d, bEn:%d, (%x %x %x)\n", screen, bEn, u8R, u8G, u8B);

    // infinity has only one screen
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G0;
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    if(bEn)
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, 0x0800, 0x0800);
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_24, (u8B|(u8G<<8)), 0xFFFF);
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_25,  u8R, 0x00FF);
    }
    else
    {
        OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, 0x0000, 0x0800);
    }

    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}


void GOP_Set_PipeDelay(int screen, int delay)
{
    U16 pipe_delay = 0x00;

    U32 GOP_Reg_Base;

    U16 GOP_Reg_DB;

    // infinity has only one screen
    GOP_Reg_DB = GOP_BANK_DOUBLE_WR_G0;
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    if (screen == 0)
    {
        pipe_delay = 0x50 + delay;
    }
    else if (screen == 1)
    {
        pipe_delay = 0x2E  + delay;
    }

    OUTREG16(GOP_Reg_Base+REG_GOP_0F, pipe_delay);

    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR);
}


void GOP_Setmode_and_win(
    int bResume, int Screen,
    int H_Start, int V_Start,
    int H_End, int V_End,
    int Panel_Width, int Panel_Height,
    U32 FB_Start_Addr,U32 Pixel_Format,U16 Bytes_Per_Pixel,
    int bInterlaceOut)
{



    // 1. GOP Hardware initail (need to check Chip top colok)
    // 2. GOP fetch framebuffer information
    // 3. GOP Display Setting
    // 4. Initial Mux for Select GOP and Sclaer(MApi_XC_ip_sel_for_gop)



    U32 FB_add = FB_Start_Addr;
    U16 FB_WIDTH=Panel_Width;
    U16 FB_HEIGHT=Panel_Height;


    U16 FB_Bytes_Per_Pixel=Bytes_Per_Pixel;

    U16 u16FrameBuffer_HSize=FB_WIDTH;//LCD_PANEL_WIDTH;
    U16 u16FrameBuffer_VSize= FB_HEIGHT;//mdrvgop_Get_Panel_Height();//mdrvgop_Get_Panel_HeightYRES;//LCD_PANEL_HEIGHT;
    U16 u16FrameBuffer_Bytes_Per_Pixel=FB_Bytes_Per_Pixel;

    U16 u16DispImage_HStart= H_Start;
    //U16 u16DispImage_HSize=FB_WIDTH;
    U16 u16DispImage_HEnd= H_End; //u16DispImage_HStart+u16DispImage_HSize;

    U16 u16DispImage_VStart= bInterlaceOut ? V_Start/2 : V_Start;
    //U16 u16DispImage_VSize=FB_HEIGHT;
    U16 u16DispImage_VEnd= bInterlaceOut ?  V_End/2 : V_End; //u16DispImage_VStart+u16DispImage_VSize;

    U32 s32BuffAddr=0;
    U32 u32TempVal=0;

    U32 devId=0x50;


    U16 MIU_BUS=0x04;

    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    U16 GOP_block_0=GOP_BANK_G0_0;
    U16 GOP_block_1=GOP_BANK_G0_1;
    U16 DISPLAY_Format=Pixel_Format;

    //FB_add = (U32)ms_mem_bus_to_MIU(FB_add);//old way
    FB_add = (U32)Chip_Phys_to_MIU(FB_add);//new way

    MIU_BUS= GOP_MIU_BUS(MIU_BUS);
    devId = OALReadDeviceId(devId);

    GOPDBG("screen:%d, pixel_format:%d, binter:%d\n", Screen, Pixel_Format, bInterlaceOut);

    GOP_Reg_DB = Screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;

    // infinity has only one screen
    GOP_block_0=GOP_BANK_G0_0;
    GOP_block_1=GOP_BANK_G0_1;


    GOPDBG ("GOP_Setmode: LCD_PANEL_WIDTH = [%d] \r\n",Panel_Width);
    GOPDBG ("GOP_Setmode: LCD_PANEL_HEIGHT = [%d] \r\n",Panel_Height);
    GOPDBG ("GOP_Setmode: FB_WIDTH = [%d] \r\n",FB_WIDTH);
    GOPDBG ("GOP_Setmode: FB_HEIGHT = [%d] \r\n",FB_HEIGHT);
    GOPDBG ("GOP_Setmode: FB_add = [%x] \r\n",FB_add);
    GOPDBG ("GOP_Setmode: FB_Start_Addr = [%x] \r\n",FB_Start_Addr);

    // infinity has only one screen
    GOP_Reg_Base = mdrv_BASE_REG_GOP00_PA;

    // infinity has only one screen
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV); //
    OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_ALPHA_INV); // PerPixel alpha 0x3F, RGB8888, enable
    OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_OP|GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
    OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay


    OUTREG16(GOP_Reg_Base+REG_GOP_02,GOP_BLINK_DISABLE);

     //  GOP : Display Area global seting

    OUTREG16(GOP_Reg_Base+REG_GOP_30,Panel_Width >>1); // Stretch Window H size (unit:2 pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_31,Panel_Height); // Stretch window V size
    OUTREG16(GOP_Reg_Base+REG_GOP_32,GOP_STRETCH_HST); // Stretch Window H coordinate
    OUTREG16(GOP_Reg_Base+REG_GOP_34,GOP_STRETCH_VST);// Stretch Window V coordinate
    OUTREG16(GOP_Reg_Base+REG_GOP_35,GOP_STRETCH_HRATIO); // Stretch H ratio
    OUTREG16(GOP_Reg_Base+REG_GOP_36,GOP_STRETCH_VRATIO); // Stretch V ratio
    OUTREG16(GOP_Reg_Base+REG_GOP_38,GOP_STRETCH_HINI); // Stretch H start value
    OUTREG16(GOP_Reg_Base+REG_GOP_39,GOP_STRETCH_VINI); // Stretch V start value

    //1  GOP fetch framebuffer information
    // 1.0 framebuffer Data format

    // infinity has only one screen
    GOP_Reg_Base = mdrv_BASE_REG_GOP01_PA;
    // switch to subbank01
    SwitchSubBank(1);

    OUTREG16(GOP_Reg_Base+REG_GOP_00,DISPLAY_Format|GOP0_GWin0_Enable|GOP_PIXEL_ALPHA_EN);


    // 1.1 framebuffer starting address
    s32BuffAddr = FB_add;
    s32BuffAddr=s32BuffAddr&0xFFFFFFFF;
    s32BuffAddr=s32BuffAddr>>MIU_BUS; // 128-bit unit = 16 bytes


    OUTREG16(GOP_Reg_Base+REG_GOP_01,(s32BuffAddr&0xFFFF)); // Ring block start Low address 0x21:16
    s32BuffAddr=s32BuffAddr>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_02,(s32BuffAddr&0xFFFF)); // Ring block start Hi address 0x22:6, page number

    // 1.2 framebuffer Pitch
    OUTREG16(GOP_Reg_Base+REG_GOP_09,(u16FrameBuffer_HSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS); // Ring block 0 line size

    // 1.4 framebuffer Size
    u32TempVal=(u16FrameBuffer_HSize*u16FrameBuffer_VSize*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_10,u32TempVal&0xFFFF);
    u32TempVal=u32TempVal>>0x10;
    OUTREG16(GOP_Reg_Base+REG_GOP_11,u32TempVal);

    // 1.5 frame buffer V start offset  (line)
    OUTREG16(GOP_Reg_Base+REG_GOP_0C,0);
    OUTREG16(GOP_Reg_Base+REG_GOP_0D,0);

    // 1.6 frame buffer H start offset  (pixel)
    OUTREG16(GOP_Reg_Base+REG_GOP_0E,0);


    //2  GOP Display Setting
    // 2.1 GWIN display area in panel : : H Start postion and end information
    u32TempVal=(u16DispImage_HStart*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_04,u32TempVal); // H start
    u32TempVal=(u16DispImage_HEnd*u16FrameBuffer_Bytes_Per_Pixel)>>MIU_BUS;
    OUTREG16(GOP_Reg_Base+REG_GOP_05,u32TempVal); // H end

    // 2.2 GWIN  display area in panel : V Start postion and end information
    OUTREG16(GOP_Reg_Base+REG_GOP_06,u16DispImage_VStart); // V start line
    OUTREG16(GOP_Reg_Base+REG_GOP_08,u16DispImage_VEnd); // V end line

    // 2.3 Display Area setting in Panel
    CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_GOPG0_MUX_MASK);
    OUTREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer1_Sel_GOP_Null|GOP_Layer0_Sel_GOP3);

    // switch back to subbank00
    SwitchSubBank(0);


    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR);

}
//MODULE_LICENSE("GPL")
