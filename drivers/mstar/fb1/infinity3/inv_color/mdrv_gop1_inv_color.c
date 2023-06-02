#include "mdrv_gop1_inv_color.h"

void GOP1_invColor_init(void)
{
    unsigned long GOP1_invColor_Reg_Base = 0;
    int output_YUV_mode=0x0002;
    int complementary_color_mode=0x0008;

    // 1. GOP Set inverse color init settings

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;

    // set gop output mode YUV
    OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BASE_SETTINGS, output_YUV_mode, GOP1_invColor_gop_output_mode);

    // set inverse color mode to complementary color mode
    OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BASE_SETTINGS, complementary_color_mode, GOP1_invColor_color_mode);

}

void GOP1_invColor_Enable(unsigned char bEn)
{
    unsigned long GOP1_invColor_Reg_Base = 0;

    // 1. GOP Set inverse color Enable/Disable

    GOP1_invColor_DBG( "[GOP1_invColor]%s: Set enable_GOP = [%d]\n",__func__,bEn);

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;

    // enable/disable inverse color
    OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BASE_SETTINGS, bEn, GOP1_invColor_enable_mask);

}

void GOP1_invColor_Set_UpdateMode(unsigned char bMode)
{
    unsigned long GOP1_invColor_Reg_Base = 0;

    // 1. GOP Set inverse color update inverse table mode

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;

    // set inverse color update mode, 1:by cpu, 0:by engine
    if(bMode)
    {
        OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BASE_SETTINGS, GOP1_invColor_update_mode, GOP1_invColor_update_mode);
    }
    else
    {
        OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BASE_SETTINGS, 0, GOP1_invColor_update_mode);
    }

}

void GOP1_invColor_Set_Y_Threshold(int AE_Y_Thres)
{
    unsigned long GOP1_invColor_Reg_Base = 0;

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;

    // 1. GOP Set Y threshold to do inverse color or not

    GOP1_invColor_DBG ("[GOP1_invColor]%s: Set Y Threshold  = [%d]\n",__func__,AE_Y_Thres);

    // set AE_Y_Thres
    OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BASE_SETTINGS, (AE_Y_Thres<<8),GOP1_invColor_AE_Y_Threshold_mask);

}

void GOP1_invColor_Set_AE_Config(int AE_Blk_Width, int AE_Blk_Height, int AE_x_res)
{
    unsigned long GOP1_invColor_Reg_Base = 0;
    int real_blk_width = AE_Blk_Width; // register setting is real_size-1
    int AE_Blk_x_num   = 0;

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;
    AE_Blk_x_num = AE_x_res / real_blk_width; //real x blk num

    // 1. GOP Set AE config

    GOP1_invColor_DBG ("[GOP1_invColor]%s: reg AE_Blk_Width  = [%d]\n",__func__,AE_Blk_Width-1);
    GOP1_invColor_DBG ("[GOP1_invColor]%s: reg AE_Blk_Height = [%d]\n",__func__,AE_Blk_Height-1);
    GOP1_invColor_DBG ("[GOP1_invColor]%s: reg AE_Blk_x_num  = [%d]\n",__func__,AE_Blk_x_num-1);

    // set AE_Blk_Width
    OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BLK_WIDTH, AE_Blk_Width-1); // reg_val=real_size-1

    // set AE_Blk_Height
    OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BLK_HEIGHT, AE_Blk_Height-1); // reg_val=real_size-1

    // set AE_Blk_x_num
    OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_AE_X_BLK_NUM, (AE_Blk_x_num-1)<<8,GOP1_invColor_AE_X_BLK_NUM_mask); // reg_val=real_size-1

}

void GOP1_invColor_Set_AE_Config_Scaling(int AE_Blk_Width, int AE_Blk_Height, int AE_x_blk_num)
{
    unsigned long GOP1_invColor_Reg_Base = 0;

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;

    // 1. GOP Set AE config

    GOP1_invColor_DBG ("[GOP1_invColor]%s: reg AE_Blk_Width  = [%d]\n",__func__,AE_Blk_Width-1);
    GOP1_invColor_DBG ("[GOP1_invColor]%s: reg AE_Blk_Height = [%d]\n",__func__,AE_Blk_Height-1);
    GOP1_invColor_DBG ("[GOP1_invColor]%s: reg AE_x_blk_num  = [%d]\n",__func__,AE_x_blk_num-1);

    // set AE_Blk_Width
    OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BLK_WIDTH, AE_Blk_Width-1); // reg_val=real_size-1

    // set AE_Blk_Height
    OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BLK_HEIGHT, AE_Blk_Height-1); // reg_val=real_size-1

    // set AE_Blk_x_num
    OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_AE_X_BLK_NUM, (AE_x_blk_num-1)<<8,GOP1_invColor_AE_X_BLK_NUM_mask); // reg_val=real_size-1

}


void GOP1_invColor_Set_Crop_Config(int crop_x_cor, int crop_y_cor, int AE_Blk_Width, int AE_Blk_Height, int AE_x_res)
{
    unsigned long GOP1_invColor_Reg_Base = 0;
    int real_blk_width  = AE_Blk_Width+1; // register setting is real_size-1
    int real_blk_height = AE_Blk_Height+1;// register setting is real_size-1
    int x_offset=0;
    int y_offset=0;
    int invTable_idx_offset=0;

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;
    // x_offset = crop_x_offset % ae_blk_w
    x_offset = crop_x_cor % real_blk_width;
    // y_offset = crop_y_offset % ae_blk_h
    y_offset = crop_y_cor % real_blk_height;
    // index offset = floor(crop_y_offset / ae_blk_h) x (reg_blk_num_x_m1+1) + floor (crop_x_offset / ae_blk_w)
    invTable_idx_offset= (crop_y_cor / real_blk_height)*(AE_x_res / real_blk_width)+(crop_x_cor / real_blk_width);

    // 1. GOP Set crop config: x_offset, y_offset & index_offset

    GOP1_invColor_DBG ("[GOP1_invColor]%s: x_offset   = [%d]\n",__func__,x_offset);
    GOP1_invColor_DBG ("[GOP1_invColor]%s: y_offset   = [%d]\n",__func__,y_offset);
    GOP1_invColor_DBG ("[GOP1_invColor]%s: idx_offset = [%d]\n",__func__,invTable_idx_offset);

    // set x_offset
    OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_X_PIX_OFFSET, x_offset);

    // set y_offset
    OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_Y_PIX_OFFSET, y_offset);

    // set invTable_idx_offset
    OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_BLK_INDEX_OFFSET, invTable_idx_offset);

}

int GOP1_invColor_Engine_Update_Done(void)
{
    unsigned long GOP1_invColor_Reg_Update = 0;
    unsigned char bDone=0;

    GOP1_invColor_Reg_Update=mdrv_BASE_REG_GOP1_invColor_AE_Update_Done;

    if(INREGMSK16(GOP1_invColor_Reg_Update+GOP1_invColor_REG_AE_Update_Done, GOP1_invColor_AE_Update_Done_mask)!=0)
    {
        bDone=1;
        //if get ready, clear it after read
        OUTREGMSK16(GOP1_invColor_Reg_Update+GOP1_invColor_REG_AE_Update_Done, 0 , GOP1_invColor_AE_Update_Done_mask);
    }
    else
    {
        bDone=0;
    }

    return bDone;
}

int GOP1_invColor_CPU_Upate_InvTable(int AE_Blk_Width, int AE_Blk_Height, int AE_x_res, int AE_y_res)
{
    unsigned long GOP1_invColor_Reg_Base = 0;
    unsigned char *invTable;
    unsigned char *invTable_rot;
    int retry_count=0;
    int blk_origin_num_x=0;
    int blk_origin_num_y=0;
    int total_index_used=0;
    int reg_index_used=0;
    int num_invTbl_per_regidx=32;
    int temp_idx=0;
    int temp_count=0;
    int temp_data1=0;
    int temp_data2=0;
    int temp_x_idx=0;
    int temp_y_idx=0;


    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;

    // 1. Count Needed Settings

    blk_origin_num_x = AE_x_res / AE_Blk_Width;
    blk_origin_num_y = AE_y_res / AE_Blk_Height;
    if((blk_origin_num_x>128)||(blk_origin_num_y>90))
    {
        GOP1_invColor_DBG ("[GOP1_invColor]%s: Error! blk_num too big!!! x_num=%d, y_num=%d\n",__func__,blk_origin_num_x,blk_origin_num_y);
        return FALSE;
    }
    total_index_used = blk_origin_num_x * blk_origin_num_y;
    reg_index_used = total_index_used / num_invTbl_per_regidx;
    if((total_index_used % num_invTbl_per_regidx)!=0)// if total_index in use is not 32 align, then reg_idx +1
    {
        reg_index_used++;
    }

    //assign memory
    invTable=kmalloc((reg_index_used*num_invTbl_per_regidx) * sizeof(unsigned char),GFP_KERNEL);
    if (!invTable)
    {
        GOP1_invColor_DBG ("[GOP1_invColor]%s: Error! kmalloc error\n",__func__);
        return FALSE;
    }
    invTable_rot=kmalloc((reg_index_used*num_invTbl_per_regidx) * sizeof(unsigned char),GFP_KERNEL);
    if (!invTable_rot)
    {
        kfree(invTable);
        GOP1_invColor_DBG ("[GOP1_invColor]%s: Error! kmalloc error\n",__func__);
        return FALSE;
    }

    // 2. Wait Inverse HW Engine write AE Inverse Table Done with Interrupt Trigger and then clean Interrupt

    while(!GOP1_invColor_Engine_Update_Done())
    {
        mdelay(30);
        if(retry_count>10)
        {
            GOP1_invColor_DBG ("[GOP1_invColor]%s: Error! wait no ack\n",__func__);
            return FALSE;
        }
        retry_count++;
    }


    // 3. Read Inverse Table

    for(temp_idx=0;temp_idx<reg_index_used;temp_idx++)
    {
        //set index
        OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, temp_idx,GOP1_invColor_SRAM_ADDR_mask);

        //set read enable
        OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, GOP1_invColor_SRAM_READ_enable ,GOP1_invColor_SRAM_READ_enable);

        //read inverse table
        temp_data1 = INREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_READ_DATA1);
        temp_data2 = INREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_READ_DATA2);

        //passing data
        for(temp_count=0;temp_count<16;temp_count++)
        {
            invTable[(temp_idx * num_invTbl_per_regidx)+temp_count]= ((temp_data1>>temp_count)& 0x1);
        }
        for(temp_count=0;temp_count<16;temp_count++)
        {
            invTable[(temp_idx * num_invTbl_per_regidx)+temp_count+16]= ((temp_data2>>temp_count)& 0x1);
        }
    }

    // 4. Do Transpose for Rotate Case

    for(temp_y_idx=0;temp_y_idx<blk_origin_num_y;temp_y_idx++)
    {
        for(temp_x_idx=0;temp_x_idx<blk_origin_num_x;temp_x_idx++)
        {
            invTable_rot[temp_x_idx*blk_origin_num_y+temp_y_idx]=invTable[temp_y_idx*blk_origin_num_x+temp_x_idx];
        }
    }

    // 5. Write Inverse Table

    for(temp_idx=0;temp_idx<reg_index_used;temp_idx++)
    {
        //re-assemble data
        temp_data1 = 0;
        temp_data2 = 0;

        for(temp_count=0;temp_count<16;temp_count++)
        {
            temp_data1=temp_data1 | (invTable[(temp_idx * num_invTbl_per_regidx)+temp_count]<<temp_count);
        }
        for(temp_count=0;temp_count<16;temp_count++)
        {
            temp_data2=temp_data2 | (invTable[(temp_idx * num_invTbl_per_regidx)+temp_count+16]<<temp_count);
        }

        //set index
        OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, temp_idx,GOP1_invColor_SRAM_ADDR_mask);

        //write data
        OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_WRITE_DATA1, temp_data1);
        OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_WRITE_DATA2, temp_data2);

        //set write enable
        OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, GOP1_invColor_SRAM_WRITE_enable ,GOP1_invColor_SRAM_WRITE_enable);
    }

    kfree(invTable);
    kfree(invTable_rot);

    // 6. Trigger CPU Update Inverse Table Done

    OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, GOP1_invColor_SRAM_CPU_UPDATE_DONE ,GOP1_invColor_SRAM_CPU_UPDATE_DONE);

    return TRUE;

}

void GOP1_invColor_Debug_Mode(void)
{
    unsigned long GOP1_invColor_Reg_Base = 0;
    unsigned long GOP1_invColor_DebugMode_Reg_Base = 0;
    int temp_idx=0;
    int reg_index_used=0;
    int temp_data1=0;
    int temp_data2=0;
    int i=0;

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;
    GOP1_invColor_DebugMode_Reg_Base = GET_REG16_ADDR_GOP1_invColor(BASE_REG_RIU_PA_GOP1_invColor, (0x122500/2));

    //open debug mode
    OUTREG16(GOP1_invColor_DebugMode_Reg_Base+BK_REG_GOP1_invColor(0x10), 0x7);

    //open CPU update mode
    GOP1_invColor_Set_UpdateMode(1);

    reg_index_used=128*90/32;//maxmum

    for(i=0;i<4;i++)
    {
        for(temp_idx=0;temp_idx<reg_index_used;temp_idx++)
        {
            //re-assemble data
            temp_data1 = 0xAAAA;
            temp_data2 = 0xAAAA;

            //set index
            OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, temp_idx,GOP1_invColor_SRAM_ADDR_mask);

            //write data
            OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_WRITE_DATA1, temp_data1);
            OUTREG16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_WRITE_DATA2, temp_data2);

            //set write enable
            OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, GOP1_invColor_SRAM_WRITE_enable ,GOP1_invColor_SRAM_WRITE_enable);
        }

        OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, GOP1_invColor_SRAM_CPU_UPDATE_DONE ,GOP1_invColor_SRAM_CPU_UPDATE_DONE);

        mdelay(50);

    }
}

void GOP1_invColor_DebugMode_UpdateInvTable(void)
{
    unsigned long GOP1_invColor_Reg_Base = 0;

    GOP1_invColor_Reg_Base = mdrv_BASE_REG_GOP1_invColor;
    OUTREGMSK16(GOP1_invColor_Reg_Base+GOP1_invColor_REG_SRAM_SETTINGS, GOP1_invColor_SRAM_CPU_UPDATE_DONE ,GOP1_invColor_SRAM_CPU_UPDATE_DONE);

}
