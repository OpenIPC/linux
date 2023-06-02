
#include "mdrv_gop.h"

#if defined(CONFIG_ARCH_CEDRIC)

#include <hal/cedric_r2.h>
static volatile PREG_MBX_st sg_pMbxRegs = NULL;
extern DEVINFO_RTK_FLAG ms_devinfo_rtk_flag(void);
#endif
U32  OALReadDeviceId(U32 devId)
{
    U16 tmp = 0;
    // need to refine for another SOC
#if defined(CONFIG_ARCH_CHICAGO)
    tmp = 0x70; //C4
#elif defined(CONFIG_ARCH_CEDRIC)
    tmp = 0x50; //C3
#else
    tmp = 0x50; //C3
#endif
    return tmp;
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

    GOP_Reg_DB = Screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;
#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = Screen == 0 ? mdrv_BASE_REG_GOP01_PA : mdrv_BASE_REG_GOP11_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREG16(GOP_Reg_Base+REG_GOP_7F, (Screen == 0 ? GOP_BANK_G0_1 : GOP_BANK_G1_1));
#endif


    FB_add = (U32)ms_mem_bus_to_MIU(FB_add);
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

    FB_add = (U32)ms_mem_bus_to_MIU(FB_add);
    MIU_BUS= GOP_MIU_BUS(MIU_BUS);


    GOPDBG ("GOP_Setmode Screen is %x.\r\n",Screen);


    GOP_Reg_DB = Screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;

#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = Screen == 0 ? mdrv_BASE_REG_GOP00_PA : mdrv_BASE_REG_GOP10_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREG16(GOP_Reg_Base+REG_GOP_7F, (Screen == 0 ?  GOP_BANK_G0_0 : GOP_BANK_G1_0)); // Change to bank GOP2G_0
#endif

       if (Screen == 0)
       {
           OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV); //
           OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_ALPHA_INV); // PerPixel alpha 0x3F, RGB8888, enable
           OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_OP|GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
           OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay
       }
       else if (Screen == 1)
       {
           OUTREG16(mdrv_BASE_REG_CHIPTOP_PA+BK_REG(0x40), 0x1000);
           OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV|GOP_ALPHA); // constant alpha 0x3F, RGB565, enable
           if(bInterlaceOut)
           {
               OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_interlace|GOP_HS_MASK|GOP_YUVOUT|GOP_FIELD_INV|GOP_ALPHA_INV);
           }
           else
           {
               OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_YUVOUT|GOP_FIELD_INV|GOP_ALPHA_INV);
           }

           OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_SC1_OP|GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
           OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY_FOR_SC1_OP);//Hsync input pipe delay
       }



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

    FB_add = (U32)ms_mem_bus_to_MIU(FB_add);
    MIU_BUS= GOP_MIU_BUS(MIU_BUS);
    devId = OALReadDeviceId(devId);

#if defined(CONFIG_ARCH_CEDRIC)
    sg_pMbxRegs = (PREG_MBX_st)IO_ADDRESS(REG_MBX_BASE);
#endif

    GOPDBG ("GOP_Setmode Screen is %x.\r\n",Screen);

    if (Screen == 0)
    {
        GOP_block_0=GOP_BANK_G0_0;
        GOP_block_1=GOP_BANK_G0_1;
    }
    else if (Screen == 1)
    {
        GOP_block_0=GOP_BANK_G1_0;
        GOP_block_1=GOP_BANK_G1_1;
    }

    GOPDBG ("GOP_Setmode: LCD_PANEL_WIDTH = [%d] \r\n",Panel_Width);
    GOPDBG ("GOP_Setmode: LCD_PANEL_HEIGHT = [%d] \r\n",Panel_Height);
    GOPDBG ("GOP_Setmode: FB_WIDTH = [%d] \r\n",FB_WIDTH);
    GOPDBG ("GOP_Setmode: FB_HEIGHT = [%d] \r\n",FB_HEIGHT);
    GOPDBG ("GOP_Setmode: FB_add = [%x] \r\n",FB_add);
    GOPDBG ("GOP_Setmode: FB_Start_Addr = [%x] \r\n",FB_Start_Addr);



    GOP_Reg_DB = Screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;

#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = GOP_block_0 == GOP_BANK_G0_0 ? mdrv_BASE_REG_GOP00_PA : mdrv_BASE_REG_GOP10_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREG16(GOP_Reg_Base+REG_GOP_7F,GOP_block_0);
#endif

    // GOP Global seting ; destination, Hsync
    if (Screen == 0)
    {
#if defined(CONFIG_ARCH_CEDRIC)

        OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV); //
        OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_ALPHA_INV); // PerPixel alpha 0x3F, RGB8888, enable
        OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_OP|GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
        OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay

#elif defined(CONFIG_ARCH_CHICAGO)

        OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_DISPLAY_MODE| GOP_GENSHOT_FAST|GOP_HS_MASK);
        OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_DISPLAY_MODE| GWIN_display_mode_progress| GOP_GENSHOT_FAST|GOP_HS_MASK);
        OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_OP|GOP_DMA_LEN_16| GOP_MIU_TSH_1|   GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
        OUTREG16(GOP_Reg_Base+REG_GOP_02,GOP_BLINK_DISABLE);
        OUTREG16(GOP_Reg_Base+REG_GOP_20,GOP_GWIN_HIGH_PRI_0);
        OUTREG16(GOP_Reg_Base+REG_GOP_0E,(Panel_Width+0x10)>>1); // RDMA H  total  (unit: 2 pixels)
        OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay
#endif

    }
    else if (Screen == 1)
    {

#if defined(CONFIG_ARCH_CEDRIC)
        OUTREG16(mdrv_BASE_REG_CHIPTOP_PA+BK_REG(0x40), 0x1000);
        OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV|GOP_ALPHA); // constant alpha 0x3F, RGB565, enable
        if(bInterlaceOut)
        {
            OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_interlace|GOP_HS_MASK|GOP_YUVOUT|GOP_FIELD_INV|GOP_ALPHA_INV);
        }
        else
        {
            OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_YUVOUT|GOP_FIELD_INV|GOP_ALPHA_INV);
        }

        OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_SC1_OP|GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
        OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY_FOR_SC1_OP);//Hsync input pipe delay
#endif
    }

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

    //1  GOP fetch framebuffer information
    // 1.0 framebuffer Data format
#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = GOP_block_1 == GOP_BANK_G0_1 ? mdrv_BASE_REG_GOP01_PA : mdrv_BASE_REG_GOP11_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    // GOP Global seting ; destination, Hsync
    OUTREG16(GOP_Reg_Base+REG_GOP_7F,GOP_block_1); // Change to bank GOP2G_1
#endif

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

    if (Screen == 0)
    {

#if defined(CONFIG_ARCH_CEDRIC)

        if (bResume || ((sg_pMbxRegs->carbacking_status || DEVINFO_RTK_FLAG_0 == ms_devinfo_rtk_flag()) && (!sg_pMbxRegs->mmi_task_running || sg_pMbxRegs->Os_flag)))
        {
            //normal
            CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_GOPG0_MUX_MASK);
            OUTREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer1_Sel_GOP_Null|GOP_Layer0_Sel_GOP0);
        }
        else
        {
            OUTREG16(mdrv_REG_GOP01_WIN, INREG16(mdrv_REG_GOP01_WIN)  & mdrv_GWin0_Disable) ;
            OUTREG16(mdrv_REG_GOP31_WIN, INREG16(mdrv_REG_GOP31_WIN) | mdrv_GWIN0_Enable) ;
            OUTREG16(mdrv_REG_GOP31_WIN, INREG16(mdrv_REG_GOP31_WIN)  & mdrv_ENABLE_CONST_ALPHA );
            OUTREG16(mdrv_REG_GOP31_ALPHA, (INREG16(mdrv_REG_GOP31_ALPHA) & (0xFF00)) |(0x80));

            CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_GOPG0_MUX_MASK);
            OUTREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer1_Sel_GOP_Null|GOP_Layer0_Sel_GOP3);
        }

#elif defined(CONFIG_ARCH_CHICAGO)

            OUTREG16(GOP_Reg_Base+REG_GOP_7E,(GOP_GOPG0_MUX_2G<<2)|GOP_GOPG0_MUX_4G);

#endif
    }
    else if (Screen == 1)
    {
#if defined(CONFIG_ARCH_CEDRIC)

        if (bResume || ((sg_pMbxRegs->carbacking_status || DEVINFO_RTK_FLAG_0 == ms_devinfo_rtk_flag()) && (!sg_pMbxRegs->mmi_task_running || sg_pMbxRegs->Os_flag)))
        {
            CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer2_MUX_MASK|GOP_Layer3_MUX_MASK);
            SETREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer2_Sel_GOP1);
        }
        else
        {
           // OUTREG16(mdrv_REG_GOP31_WIN, INREG16(mdrv_REG_GOP31_WIN) | mdrv_GWIN0_Enable) ;
           // OUTREG16(mdrv_REG_GOP31_WIN, INREG16(mdrv_REG_GOP31_WIN)  & mdrv_ENABLE_CONST_ALPHA );
           // OUTREG16(mdrv_REG_GOP31_ALPHA, (INREG16(mdrv_REG_GOP31_ALPHA) & (0xFF00)) |(0x80));
            CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer2_MUX_MASK);
            SETREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer2_Sel_GOP1);
        }
#endif
    }

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);

}



void GOP_Set_OutFormat(int screen, int byuvoutput)
{
    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    GOP_Reg_DB = screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;
#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = screen == 0 ? mdrv_BASE_REG_GOP00_PA : mdrv_BASE_REG_GOP10_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, (screen == 0 ? GOP_BANK_G0_0 : GOP_BANK_G1_0), 0x000F); // Change to bank GOP2G_0
#endif

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

    GOP_Reg_DB = screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;

#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = screen == 0 ? mdrv_BASE_REG_GOP00_PA : mdrv_BASE_REG_GOP10_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, (screen == 0 ? GOP_BANK_G0_0 : GOP_BANK_G1_0), 0x000F); // Change to bank GOP2G_0
#endif

    // alpha inv
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, (bEn ? 0x0000 : 0x8000), 0x8000);


#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = screen == 0 ? mdrv_BASE_REG_GOP01_PA : mdrv_BASE_REG_GOP11_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, (screen == 0 ? GOP_BANK_G0_1 : GOP_BANK_G1_1), 0x000F); // Change to bank GOP2G_0
#endif


    // constant or pixel alpha
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_00, (bEn ? 0x0000 : 0x4000), 0x4000);

    // alhap value
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_0A, (bEn ? alpha : 0x0000), 0x00FF);


    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR_MSK);


}


void GOP_Set_Color_Key(int screen, int bEn, int u8R, int u8G, int u8B)
{
    U32 GOP_Reg_Base;
    U16 GOP_Reg_DB;

    GOP_DBG(GOP_DBG_LV_0, "SetColorKey: screen:%d, bEn:%d, (%x %x %x)\n", screen, bEn, u8R, u8G, u8B);

    GOP_Reg_DB = screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;


#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = screen == 0 ? mdrv_BASE_REG_GOP00_PA : mdrv_BASE_REG_GOP01_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, (screen == 0 ? GOP_BANK_G0_0 : GOP_BANK_G0_1), 0x000F); // Change to bank GOP2G_0
#endif

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


    GOP_Reg_DB = screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;

#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = screen == 0 ? mdrv_BASE_REG_GOP00_PA : mdrv_BASE_REG_GOP10_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F,(screen == 0 ? GOP_BANK_G0_0 : GOP_BANK_G1_0), 0x000F); // Change to bank GOP2G_0
#endif

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

    FB_add = (U32)ms_mem_bus_to_MIU(FB_add);
    MIU_BUS= GOP_MIU_BUS(MIU_BUS);
    devId = OALReadDeviceId(devId);

#if defined(CONFIG_ARCH_CEDRIC)
    sg_pMbxRegs = (PREG_MBX_st)IO_ADDRESS(REG_MBX_BASE);
#endif

    GOPDBG("screen:%d, pixel_format:%d, binter:%d\n", Screen, Pixel_Format, bInterlaceOut);

    GOP_Reg_DB = Screen == 0 ? GOP_BANK_DOUBLE_WR_G0 : GOP_BANK_DOUBLE_WR_G1;

    if (Screen == 0)
    {
        GOP_block_0=GOP_BANK_G0_0;
        GOP_block_1=GOP_BANK_G0_1;
    }
    else if (Screen == 1)
    {
        GOP_block_0=GOP_BANK_G1_0;
        GOP_block_1=GOP_BANK_G1_1;
    }

    GOPDBG ("GOP_Setmode: LCD_PANEL_WIDTH = [%d] \r\n",Panel_Width);
    GOPDBG ("GOP_Setmode: LCD_PANEL_HEIGHT = [%d] \r\n",Panel_Height);
    GOPDBG ("GOP_Setmode: FB_WIDTH = [%d] \r\n",FB_WIDTH);
    GOPDBG ("GOP_Setmode: FB_HEIGHT = [%d] \r\n",FB_HEIGHT);
    GOPDBG ("GOP_Setmode: FB_add = [%x] \r\n",FB_add);
    GOPDBG ("GOP_Setmode: FB_Start_Addr = [%x] \r\n",FB_Start_Addr);

#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = GOP_block_0 == GOP_BANK_G0_0 ? mdrv_BASE_REG_GOP00_PA : mdrv_BASE_REG_GOP10_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F,GOP_block_0, 0x000F);
#endif

    if (Screen == 0)
    {
#if defined(CONFIG_ARCH_CEDRIC)

        OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV); //
        OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_ALPHA_INV); // PerPixel alpha 0x3F, RGB8888, enable
        OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_OP|GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
        OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay

#elif defined(CONFIG_ARCH_CHICAGO)

        OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_DISPLAY_MODE| GOP_GENSHOT_FAST|GOP_HS_MASK);
        OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_DISPLAY_MODE| GWIN_display_mode_progress| GOP_GENSHOT_FAST|GOP_HS_MASK);
        OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_OP|GOP_DMA_LEN_16| GOP_MIU_TSH_1|   GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
        OUTREG16(GOP_Reg_Base+REG_GOP_02,GOP_BLINK_DISABLE);
        OUTREG16(GOP_Reg_Base+REG_GOP_20,GOP_GWIN_HIGH_PRI_0);
        OUTREG16(GOP_Reg_Base+REG_GOP_0E,(Panel_Width+0x10)>>1); // RDMA H  total  (unit: 2 pixels)
        OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY);//Hsync input pipe delay
#endif

    }
    else if (Screen == 1)
    {

#if defined(CONFIG_ARCH_CEDRIC)
        OUTREG16(mdrv_BASE_REG_CHIPTOP_PA+BK_REG(0x40), 0x1000);
        OUTREG16(GOP_Reg_Base+REG_GOP_00,GOP_SOFT_RESET|GOP_FIELD_INV|GOP_ALPHA); // constant alpha 0x3F, RGB565, enable
        if(bInterlaceOut)
        {
            OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_interlace|GOP_HS_MASK|GOP_YUVOUT|GOP_FIELD_INV|GOP_ALPHA_INV);
        }
        else
        {
            OUTREG16(GOP_Reg_Base+REG_GOP_00,GWIN_display_mode_progress|GOP_HS_MASK|GOP_YUVOUT|GOP_FIELD_INV|GOP_ALPHA_INV);
        }

        OUTREG16(GOP_Reg_Base+REG_GOP_01,GOP_DST_SC1_OP|GOP_REGDMA_INTERVAL_START|GOP_REGDMA_INTERVAL_END);
        OUTREG16(GOP_Reg_Base+REG_GOP_0F,GOP_HSYNC_PIPE_DLY_FOR_SC1_OP);//Hsync input pipe delay
#endif
    }

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
#if defined(CONFIG_ARCH_CEDRIC)
    GOP_Reg_Base = GOP_block_1 == GOP_BANK_G0_1 ? mdrv_BASE_REG_GOP01_PA : mdrv_BASE_REG_GOP11_PA;
#elif defined(CONFIG_ARCH_CHICAGO)
    GOP_Reg_Base = mdrv_BASE_REG_GOP_PA;
    // GOP Global seting ; destination, Hsync
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_block_1, 0x000F); // Change to bank GOP2G_1
#endif


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

    if (Screen == 0)
    {

#if defined(CONFIG_ARCH_CEDRIC)

        if (bResume || ((sg_pMbxRegs->carbacking_status || DEVINFO_RTK_FLAG_0 == ms_devinfo_rtk_flag()) && (!sg_pMbxRegs->mmi_task_running || sg_pMbxRegs->Os_flag)))
        {
            //normal
            CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_GOPG0_MUX_MASK);
            OUTREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer1_Sel_GOP_Null|GOP_Layer0_Sel_GOP0);
        }
        else
        {
            OUTREG16(mdrv_REG_GOP01_WIN, INREG16(mdrv_REG_GOP01_WIN)  & mdrv_GWin0_Disable) ;
            OUTREG16(mdrv_REG_GOP31_WIN, INREG16(mdrv_REG_GOP31_WIN) | mdrv_GWIN0_Enable) ;
            OUTREG16(mdrv_REG_GOP31_WIN, INREG16(mdrv_REG_GOP31_WIN)  & mdrv_ENABLE_CONST_ALPHA );
            OUTREG16(mdrv_REG_GOP31_ALPHA, (INREG16(mdrv_REG_GOP31_ALPHA) & (0xFF00)) |(0x80));

            CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_GOPG0_MUX_MASK);
            OUTREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer1_Sel_GOP_Null|GOP_Layer0_Sel_GOP3);
        }

#elif defined(CONFIG_ARCH_CHICAGO)

            OUTREG16(GOP_Reg_Base+REG_GOP_7E,(GOP_GOPG0_MUX_2G<<2)|GOP_GOPG0_MUX_4G);

#endif
    }
    else if (Screen == 1)
    {
#if defined(CONFIG_ARCH_CEDRIC)

        if (bResume || ((sg_pMbxRegs->carbacking_status || DEVINFO_RTK_FLAG_0 == ms_devinfo_rtk_flag()) && (!sg_pMbxRegs->mmi_task_running || sg_pMbxRegs->Os_flag)))
        {
            CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer2_MUX_MASK|GOP_Layer3_MUX_MASK);
            SETREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer2_Sel_GOP1);
        }
        else
        {
           // OUTREG16(mdrv_REG_GOP31_WIN, INREG16(mdrv_REG_GOP31_WIN) | mdrv_GWIN0_Enable) ;
           // OUTREG16(mdrv_REG_GOP31_WIN, INREG16(mdrv_REG_GOP31_WIN)  & mdrv_ENABLE_CONST_ALPHA );
           // OUTREG16(mdrv_REG_GOP31_ALPHA, (INREG16(mdrv_REG_GOP31_ALPHA) & (0xFF00)) |(0x80));
            CLRREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer2_MUX_MASK);
            SETREG16(GOP_Reg_Base+REG_GOP_7E,GOP_Layer2_Sel_GOP1);
        }
#endif
    }

    // Double Buffer Write
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR);
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, GOP_Reg_DB, (GOP_BANK_DOUBLE_WR_MSK|GOP_BANK_SEL_MSK));
    OUTREGMSK16(GOP_Reg_Base+REG_GOP_7F, 0, GOP_BANK_DOUBLE_WR);

}
//MODULE_LICENSE("GPL")
