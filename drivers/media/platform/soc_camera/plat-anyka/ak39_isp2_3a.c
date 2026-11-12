/* See the note at the head of ak39_isp2.c. */
#include <linux/kernel.h>
#include <linux/string.h>
#include <plat-anyka/ak_isp_drv.h>
#include "ak39_isp2.h"
#include "ak39_isp2_3a.h"
#include "ak39_isp2_reg.h"

//#define ISP_DEBUG
#ifdef ISP_DEBUG
#define isp_dbg(stuff...)       printk(" ISP: " stuff)
#else
#define isp_dbg(fmt, args)   do{}while(0)
#endif 

#define HOLD_RANGE		6
#define GAIN_STEP		16
#define ONE_X_GAIN		0x100
#define GAIN_SHIFT		8
#define CONTROL_ZONE1	32
#define CONTROL_ZONE2	96
#define CONTROL_ZONE3	160
#define CONTROL_STEP1	12
#define CONTROL_STEP2	16
#define CONTROL_STEP3	24
#define CONTROL_STEP_MAX	96

#define DROP_FRAMES_MAX 3
#define UNSTABLE_FRAMES_MAX 10
static int unstable_count =0;

/**
 * @brief: set auto focus param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_af:  af  param
 */
int ak_isp_vp_set_af_attr( AK_ISP_AF_ATTR *p_af)//lz0499 9_25
{
	unsigned long cmd;
    isp_dbg("%s enter.\n", __func__);
    
    //yuv_reg_33
    memcpy(&(isp->af_para),p_af,sizeof(AK_ISP_AF_ATTR));
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win0_left, 11) << AF_WIN0_LEFT)
    	| (LOW_BITS(p_af->af_win0_right, 11) << AF_WIN0_RIGHT)
    	| (LOW_BITS(p_af->af_win0_top, 10) << AF_WIN0_TOP);
    isp->reg_blkaddr3[32] = cmd;

    //yuv_reg_34
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win0_bottom, 10) << AF_WIN0_BOTTOM)
    	| (LOW_BITS(p_af->af_win1_left, 11) << AF_WIN1_LEFT)
    	| (LOW_BITS(p_af->af_win1_right, 11) << AF_WIN1_RIGHT);
	isp->reg_blkaddr3[33] = cmd;

	//yuv_reg_35
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win1_top, 10) << AF_WIN1_TOP)
    	| (LOW_BITS(p_af->af_win1_bottom, 10) << AF_WIN1_BOTTOM)
    	| (LOW_BITS(p_af->af_win2_left, 11) << AF_WIN2_LEFT);
	isp->reg_blkaddr3[34] = cmd;

	//yuv_reg_36
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win2_right, 11) << AF_WIN2_RIGHT) 
    	| (LOW_BITS(p_af->af_win2_top, 10) << AF_WIN2_TOP)
    	| (LOW_BITS(p_af->af_win2_bottom, 10) << AF_WIN2_BOTTOM);
	isp->reg_blkaddr3[35] = cmd;

	//yuv_reg_37
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win3_left, 11) << AF_WIN3_LEFT)
    	| (LOW_BITS(p_af->af_win3_right, 11) << AF_WIN3_RIGHT)
    	| (LOW_BITS(p_af->af_win3_top, 10) << AF_WIN3_TOP);
	isp->reg_blkaddr3[36] = cmd;

	//yuv_reg_38
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win3_bottom, 10) << AF_WIN3_BOTTOM)
    	| (LOW_BITS(p_af->af_win4_left, 11) << AF_WIN4_LEFT)
    	| (LOW_BITS(p_af->af_win4_right, 11) << AF_WIN4_RIGHT);
	isp->reg_blkaddr3[37] = cmd;

	//yuv_reg_39
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win4_top, 10) << AF_WIN4_TOP)
    	| (LOW_BITS(p_af->af_win4_bottom, 10) << AF_WIN4_BOTTOM)
    	| (LOW_BITS(p_af->af_th, 8) << AF_TH);
	isp->reg_blkaddr3[38] = cmd;

    return 0;
}


int ak_isp_vp_set_af_win34_attr( AK_ISP_AF_ATTR *p_af)
{
	unsigned long cmd;
    isp_dbg("%s enter.\n", __func__);
    
    
	//yuv_reg_37
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win3_left>>1, 11) << AF_WIN3_LEFT)
    	| (LOW_BITS(p_af->af_win3_right>>1, 11) << AF_WIN3_RIGHT)
    	| (LOW_BITS(p_af->af_win3_top>>1, 10) << AF_WIN3_TOP);
	isp->reg_blkaddr3[36] = cmd;

	//yuv_reg_38
    cmd = 0;
    cmd = (LOW_BITS(p_af->af_win3_bottom>>1, 10) << AF_WIN3_BOTTOM)
    	| (LOW_BITS(p_af->af_win4_left>>1, 11) << AF_WIN4_LEFT)
    	| (LOW_BITS(p_af->af_win4_right>>1, 11) << AF_WIN4_RIGHT);
	isp->reg_blkaddr3[37] = cmd;

	//yuv_reg_39
    cmd = isp->reg_blkaddr3[38];
	CLEAR_BITS(cmd, AF_WIN4_TOP, 10);
	CLEAR_BITS(cmd, AF_WIN4_BOTTOM, 10);
    cmd |= (LOW_BITS(p_af->af_win4_top>>1, 10) << AF_WIN4_TOP)
    	| (LOW_BITS(p_af->af_win4_bottom>>1, 10) << AF_WIN4_BOTTOM);
	isp->reg_blkaddr3[38] = cmd;

    return 0;
}

/**
 * @brief: get auto focus param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_af:  af  param
 */
int ak_isp_vp_get_af_attr( AK_ISP_AF_ATTR *p_af)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_af,&(isp->af_para),sizeof(AK_ISP_AF_ATTR));
    
    return 0;
}

/**
 * @brief: get auto focus statics info param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_af_stat_info:  af_stat  param
 */
int ak_isp_vp_get_af_stat_info( AK_ISP_AF_STAT_INFO *p_af_stat_info)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_af_stat_info,&(isp->af_stat_para),sizeof(AK_ISP_AF_STAT_INFO));
    
    return 0;
}

/**
 * @brief: set white balance  type
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_type:  white balance  type  param
 */
int  ak_isp_vp_set_wb_type( AK_ISP_WB_TYPE_ATTR *p_type)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->wb_type_para),p_type,sizeof(AK_ISP_WB_TYPE_ATTR));
	isp->linkage_ccm_update_flag =1;
	isp->linkage_hue_update_flag=1;
    return 0;
}

/**
 * @brief: get white balance  type
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_type:  white balance  type  param
 */
int  ak_isp_vp_get_wb_type( AK_ISP_WB_TYPE_ATTR *p_type)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_type,&(isp->wb_type_para),sizeof(AK_ISP_WB_TYPE_ATTR));
    
    return 0;
}

/**
 * @brief: set manual white balance
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_mwb:  manual white balance  type  param
 */
int  ak_isp_vp_set_mwb_attr( AK_ISP_MWB_ATTR *p_mwb)
{
   isp_dbg("%s enter.\n", __func__);
   memcpy(&(isp->mwb_para),p_mwb,sizeof(AK_ISP_MWB_ATTR));
   
   return 0;
}

/**
 * @brief: get manual white balance
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_mwb:  manual white balance  type  param
 */
int  ak_isp_vp_get_mwb_attr( AK_ISP_MWB_ATTR *p_mwb)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_mwb,&(isp->mwb_para),sizeof(AK_ISP_MWB_ATTR));
    
    return 0;
}

int ak_isp_updata_wb_gain(AK_ISP_WB_GAIN *wb_gain)
{
    unsigned long cmd;
    isp_dbg("%s enter.\n", __func__);
        
   	//wb_reg_23
    cmd = 0;
    cmd = (LOW_BITS(wb_gain->r_gain, 12) << WBC_WR) 
    	| (LOW_BITS(wb_gain->g_gain, 12) << WBC_WG);
    isp->reg_blkaddr5[22] = cmd;

    //wb_reg_24
    cmd = 0;
    cmd = (LOW_BITS(wb_gain->b_gain, 12) << WBC_WB) 
    	| (LOW_BITS(wb_gain->r_offset, 10) << WBC_WR_OFFSET);
    isp->reg_blkaddr5[23] = cmd;

    //wb_reg_25
    cmd = 0;
    cmd = (LOW_BITS(wb_gain->g_offset, 10) << WBC_WG_OFFSET)
    	| (LOW_BITS(wb_gain->b_offset, 10) << WBC_WB_OFFSET);
    isp->reg_blkaddr5[24] = cmd;
    
    return 0;
}

int _set_awb_attr( AK_ISP_AWB_ATTR *awb)
{ 
    unsigned long cmd;
    isp_dbg("%s enter.\n", __func__);
     
    //wb_reg_1
    cmd = 0;
    cmd = (LOW_BITS(awb->gr_low[0], 10) << WB_GR_LOW_0) 
    	| (LOW_BITS(awb->gr_high[0], 10) << WB_GR_HIGH_0) 
    	| (LOW_BITS(awb->gb_low[0], 10) << WB_GB_LOW_0) 
    	| (LOW_BITS(awb->gb_high[0], 2) << WB_GB_HIGH_0_L2);
    isp->reg_blkaddr5[0] = cmd;
    
    //wb_reg_2
    cmd = 0;
    cmd = (LOW_BITS((awb->gb_high[0] >> 2), 8) << WB_GB_HIGH_0_H8) 
    	| (LOW_BITS(awb->rb_low[0], 10) << WB_RB_LOW_0) 
    	| (LOW_BITS(awb->rb_high[0], 10) << WB_RB_HIGH_0) 
    	| (LOW_BITS(awb->gr_low[1], 4) << WB_GR_LOW_1_L4);
    isp->reg_blkaddr5[1] = cmd;

    //wb_reg_3
    cmd = 0;
    cmd = (LOW_BITS((awb->gr_low[1] >> 4), 6) << WB_GR_LOW_1_H6) 
    	| (LOW_BITS(awb->gr_high[1], 10) << WB_GR_HIGH_1) 
    	| (LOW_BITS(awb->gb_low[1], 10) << WB_GB_LOW_1) 
    	| (LOW_BITS(awb->gb_high[1], 6) << WB_GB_HIGH_1_L6);
       isp->reg_blkaddr5[2] = cmd;
   
    //wb_reg_4
    cmd = 0;
    cmd = (LOW_BITS((awb->gb_high[1] >> 6), 4) << WB_GB_HIGH_1_H4) 
    	| (LOW_BITS(awb->rb_low[1], 10) << WB_RB_LOW_1) 
    	| (LOW_BITS(awb->rb_high[1], 10) << WB_RB_HIGH_1) 
    	| (LOW_BITS(awb->gr_low[2], 8) << WB_GR_LOW_2_L8);
    isp->reg_blkaddr5[3] = cmd;

    //wb_reg_5
    cmd = 0;
    cmd = (LOW_BITS((awb->gr_low[2] >> 8), 2) << WB_GR_LOW_2_H2) 
    	| (LOW_BITS(awb->gr_high[2], 10) << WB_GR_HIGH_2) 
    	| (LOW_BITS(awb->gb_low[2], 10) << WB_GB_LOW_2) 
    	| (LOW_BITS(awb->gb_high[2], 10) << WB_GB_HIGH_2);
    isp->reg_blkaddr5[4] = cmd;

    //wb_reg_6
    cmd = 0;
    cmd = (LOW_BITS(awb->rb_low[2], 10)  << WB_RB_LOW_2) 
    	| (LOW_BITS(awb->rb_high[2], 10) << WB_RB_HIGH_2) 
    	| (LOW_BITS(awb->gr_low[3], 10) << WB_GR_LOW_3) 
    	| (LOW_BITS(awb->gr_high[3], 2) << WB_GR_HIGH_3_L2);
    isp->reg_blkaddr5[5] = cmd;

    //wb_reg_7
    cmd = 0;
    cmd = (LOW_BITS((awb->gr_high[3] >> 2), 8) << WB_GR_HIGH_3_H8) 
    	| (LOW_BITS(awb->gb_low[3], 10) << WB_GB_LOW_3) 
    	| (LOW_BITS(awb->gb_high[3], 10) << WB_GB_HIGH_3) 
    	| (LOW_BITS(awb->rb_low[3], 4) << WB_RB_LOW_3_L4);
    isp->reg_blkaddr5[6] = cmd;

    //wb_reg_8
    cmd = 0;
    cmd = (LOW_BITS((awb->rb_low[3] >> 4), 6) << WB_RB_LOW_3_H6) 
    	| (LOW_BITS(awb->rb_high[3], 10) << WB_RB_HIGH_3) 
    	| (LOW_BITS(awb->gr_low[4], 10) << WB_GR_LOW_4) 
    	| (LOW_BITS(awb->gr_high[4], 6) << WB_GR_HIGH_4_L6);
    isp->reg_blkaddr5[7] = cmd;

    //wb_reg_9
    cmd = 0;
    cmd = (LOW_BITS((awb->gr_high[4] >> 6), 4) << WB_GR_HIGH_4_H4) 
    	| (LOW_BITS(awb->gb_low[4], 10) << WB_GB_LOW_4) 
    	| (LOW_BITS(awb->gb_high[4], 10) << WB_GB_HIGH_4) 
    	| (LOW_BITS(awb->rb_low[4], 8) << WB_RB_LOW_4_L8);
    isp->reg_blkaddr5[8] = cmd;

    //wb_reg_10
    cmd = 0;
    cmd = (LOW_BITS((awb->rb_low[4] >> 8), 2) << WB_RB_LOW_4_H2) 
    	| (LOW_BITS(awb->rb_high[4], 10) << WB_RB_HIGH_4) 
    	| (LOW_BITS(awb->gr_low[5], 10) << WB_GR_LOW_5) 
    	| (LOW_BITS(awb->gr_high[5], 10) << WB_GR_HIGH_5);
    isp->reg_blkaddr5[9] = cmd;
	
    //wb_reg_11
    cmd = 0;
    cmd = (LOW_BITS(awb->gb_low[5], 10) << WB_GB_LOW_5) 
    	| (LOW_BITS(awb->gb_high[5], 10) << WB_GB_HIGH_5) 
    	| (LOW_BITS(awb->rb_low[5], 10) << WB_RB_LOW_5) 
    	| (LOW_BITS(awb->rb_high[5], 2) << WB_RB_HIGH_5_L2);
    isp->reg_blkaddr5[10] = cmd;

    //wb_reg_12
    cmd = 0;
    cmd = (LOW_BITS((awb->rb_high[5] >> 2), 8) << WB_RB_HIGH_5_H8) 
    	| (LOW_BITS(awb->gr_low[6], 10) << WB_GR_LOW_6) 
    	| (LOW_BITS(awb->gr_high[6], 10) << WB_GR_HIGH_6) 
    	| (LOW_BITS(awb->gb_low[6], 4) << WB_GB_LOW_6_L4);
    isp->reg_blkaddr5[11] = cmd;

	//wb_reg_13
    cmd = 0;
    cmd = (LOW_BITS((awb->gb_low[6] >> 4), 6) << WB_GB_LOW_6_H6) 
    	| (LOW_BITS(awb->gb_high[6], 10) << WB_GB_HIGH_6) 
    	| (LOW_BITS(awb->rb_low[6], 10) << WB_RB_LOW_6) 
    	| (LOW_BITS(awb->rb_high[6], 6) << WB_RB_HIGH_6_L6);
    isp->reg_blkaddr5[12] = cmd;

	//wb_reg_14
    cmd = 0;
    cmd = (LOW_BITS((awb->rb_high[6] >> 6), 4) << WB_RB_HIGH_6_H4) 
    	| (LOW_BITS(awb->gr_low[7], 10) << WB_GR_LOW_7) 
    	| (LOW_BITS(awb->gr_high[7], 10) << WB_GR_HIGH_7) 
    	| (LOW_BITS(awb->gb_low[7], 8) << WB_GB_LOW_7_L8);
    isp->reg_blkaddr5[13] = cmd;

	//wb_reg_15
    cmd = 0;
    cmd = (LOW_BITS((awb->gb_low[7] >> 8), 2) << WB_GB_LOW_7_H2) 
    	| (LOW_BITS(awb->gb_high[7], 10) << WB_GB_HIGH_7) 
    	| (LOW_BITS(awb->rb_low[7], 10) << WB_RB_LOW_7) 
    	| (LOW_BITS(awb->rb_high[7], 10) << WB_RB_HIGH_7);
    isp->reg_blkaddr5[14] = cmd;

	//wb_reg_16
    cmd = 0;
    cmd = (LOW_BITS(awb->gr_low[8], 10) << WB_GR_LOW_8) 
    	| (LOW_BITS(awb->gr_high[8], 10) << WB_GR_HIGH_8) 
    	| (LOW_BITS(awb->gb_low[8], 10) << WB_GB_LOW_8) 
    	| (LOW_BITS(awb->gb_high[8], 2) << WB_GB_HIGH_8_L2);
    isp->reg_blkaddr5[15] = cmd;

	//wb_reg_17
    cmd = 0;
    cmd = (LOW_BITS((awb->gb_high[8] >> 2), 8) << WB_GB_HIGH_8_H8) 
    	| (LOW_BITS(awb->rb_low[8], 10) << WB_RB_LOW_8) 
    	| (LOW_BITS(awb->rb_high[8], 10) << WB_RB_HIGH_8) 
    	| (LOW_BITS(awb->gr_low[9], 4) << WB_GR_LOW_9_L4);
    isp->reg_blkaddr5[16] = cmd;

	//wb_reg_18
    cmd = 0;
    cmd = (LOW_BITS((awb->gr_low[9]>> 4), 6) << WB_GR_LOW_9_H6) 
    	| (LOW_BITS(awb->gr_high[9], 10) << WB_GR_HIGH_9) 
    	| (LOW_BITS(awb->gb_low[9], 10) << WB_GB_LOW_9) 
    	| (LOW_BITS(awb->gb_high[9], 6) << WB_GB_HIGH_9_L6);
    isp->reg_blkaddr5[17] = cmd;

	//wb_reg_19
    cmd = 0;
    cmd = (LOW_BITS((awb->gb_high[9] >> 6), 4) << WB_GB_HIGH_9_H4) 
    	| (LOW_BITS(awb->rb_low[9], 10) << WB_RB_LOW_9) 
    	| (LOW_BITS(awb->rb_high[9], 10) << WB_RB_HIGH_9) 
    	| (LOW_BITS(awb->y_low, 8) << WB_Y_LOW);
    isp->reg_blkaddr5[18] = cmd;

	//wb_reg_20
    cmd = 0;
	cmd = isp->reg_blkaddr5[19];
	CLEAR_BITS(cmd, 0, 16);
    cmd |=(LOW_BITS(awb->y_high, 8) << WB_Y_HIGH) 
    	| (LOW_BITS(awb->err_est, 8) << WB_ERR_EST);
    isp->reg_blkaddr5[19] = cmd;

    //wb_reg_21
    cmd = 0;
    cmd = (LOW_BITS(awb->g_weight[0], 4) << WB_G_WEIGHT_0) 
    	| (LOW_BITS(awb->g_weight[1], 4) << WB_G_WEIGHT_1) 
    	| (LOW_BITS(awb->g_weight[2], 4) << WB_G_WEIGHT_2) 
    	| (LOW_BITS(awb->g_weight[3], 4) << WB_G_WEIGHT_3) 
    	| (LOW_BITS(awb->g_weight[4], 4) << WB_G_WEIGHT_4) 
    	| (LOW_BITS(awb->g_weight[5], 4) << WB_G_WEIGHT_5) 
    	| (LOW_BITS(awb->g_weight[6], 4) << WB_G_WEIGHT_6) 
    	| (LOW_BITS(awb->g_weight[7], 4) << WB_G_WEIGHT_7);
    isp->reg_blkaddr5[20] = cmd;

    //wb_reg_22
    cmd = 0;
    cmd = (LOW_BITS(awb->g_weight[8], 4) << WB_G_WEIGHT_8) 
    	| (LOW_BITS(awb->g_weight[9], 4) << WB_G_WEIGHT_9) 
    	| (LOW_BITS(awb->g_weight[10], 4) << WB_G_WEIGHT_10) 
    	| (LOW_BITS(awb->g_weight[11], 4) << WB_G_WEIGHT_11) 
    	| (LOW_BITS(awb->g_weight[12], 4) << WB_G_WEIGHT_12) 
    	| (LOW_BITS(awb->g_weight[13], 4) << WB_G_WEIGHT_13) 
    	| (LOW_BITS(awb->g_weight[14], 4) << WB_G_WEIGHT_14) 
    	| (LOW_BITS(awb->g_weight[15], 4) << WB_G_WEIGHT_15);
    isp->reg_blkaddr5[21] = cmd;
    return 0;
}

/**
 * @brief: set auto white balance
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_awb:  auto white balance  param
 */
int  ak_isp_vp_set_awb_attr( AK_ISP_AWB_ATTR *p_awb)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->awb_para),p_awb,sizeof(AK_ISP_AWB_ATTR));

    return 0;
}

/**
 * @brief: get auto white balance
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_awb:  auto white balance  param
 */
int  ak_isp_vp_get_awb_attr( AK_ISP_AWB_ATTR *p_awb)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_awb,&(isp->awb_para),sizeof(AK_ISP_AWB_ATTR));
    
    return 0;
}

////////////////////////awb new struct///////////////////////////////////
int  ak_isp_vp_set_awb_ex_attr( AK_ISP_AWB_EX_ATTR *p_awb)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->awb_ex_ctrl_para),p_awb,sizeof(AK_ISP_AWB_EX_ATTR));
    
    return 0;
}

int  ak_isp_vp_get_awb_ex_attr( AK_ISP_AWB_EX_ATTR *p_awb)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_awb,&(isp->awb_ex_ctrl_para),sizeof(AK_ISP_AWB_EX_ATTR));
    
    return 0;
}

/**
 * @brief: get awb statics info
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_awb_stat_info:  awb statics info  param
 */
int ak_isp_vp_get_awb_stat_info(AK_ISP_AWB_STAT_INFO *p_awb_stat_info)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_awb_stat_info,&(isp->awb_stat_info_para),sizeof(AK_ISP_AWB_STAT_INFO));
    
    return 0;
}

int ak_isp_calc_avg(void)
{
    return 0;  
}

int  _ae_compensation(void)
{
    return 0;  
}

/**
 * @brief: set exp  type
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_exp_type:  exp type  param
 */
int  ak_isp_vp_set_exp_type(AK_ISP_EXP_TYPE* p_exp_type)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->exp_type_para),p_exp_type,sizeof(AK_ISP_EXP_TYPE));

    return 0;
}

/**
 * @brief: get exp  type
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_exp_type:  exp type  param
 */
int  ak_isp_vp_get_exp_type( AK_ISP_EXP_TYPE* p_exp_type)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_exp_type,&(isp->exp_type_para),sizeof(AK_ISP_EXP_TYPE));
    
    return 0;
}

/**
 * @brief: set frame  rate
 * @author: lz
 * @date: 2016-9-21
 * @param [in] *p_frame_rate:  frame rate  param
 */
int  ak_isp_vp_set_frame_rate(AK_ISP_FRAME_RATE_ATTR* p_frame_rate)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->frame_rate_para),p_frame_rate,sizeof(AK_ISP_FRAME_RATE_ATTR));
    
    return 0;
}

/**
 * @brief: get frame  rate
 * @author: lz
 * @date: 2016-9-21
 * @param [in] *p_frame_rate:  frame rate  param
 */
int  ak_isp_vp_get_frame_rate( AK_ISP_FRAME_RATE_ATTR* p_frame_rate)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_frame_rate,&(isp->frame_rate_para),sizeof(AK_ISP_FRAME_RATE_ATTR));

    return 0;
}

/**
 * @brief: set manual exposure  param
 * @author: lizhi
 * @date: 2020-06-01
 * @param [in] *p_mae:  manual exposure  param
 */
int ak_isp_vp_set_mae_attr( AK_ISP_MAE_ATTR *p_mae)
{
	isp_dbg("%s enter.\n", __func__);
	if(p_mae->a_gain != 0 
	   || p_mae->d_gain != 0
	   || p_mae->isp_d_gain != 0
	   || p_mae->exp_time != 0
		)
	memcpy(&(isp->mae_para),p_mae,sizeof(AK_ISP_MAE_ATTR));	
	
	if(isp->mae_para.a_gain <256)
		isp->mae_para.a_gain= 256;

	if(isp->mae_para.d_gain <256)
		isp->mae_para.d_gain= 256;

	if(isp->mae_para.isp_d_gain <256)
		isp->mae_para.isp_d_gain= 256;

	if(isp->mae_para.exp_time<1)
		isp->mae_para.exp_time= 1;
	
	isp->mae_para_update_flag = 1;

	return 0;
}

/**
 * @brief: get manual exposure  param
 * @author: lizhi
 * @date: 2020-06-01
 * @param [in] *p_mae:  manual exposure  param
 */
int  ak_isp_vp_get_mae_attr(AK_ISP_MAE_ATTR *p_mae)
{
	isp_dbg("%s enter.\n", __func__);
	memcpy(p_mae,&(isp->mae_para),sizeof(AK_ISP_MAE_ATTR));
	return 0;
}



/**
 * @brief: set auto exposure  param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_ae:  auto exposure  param
 */
int  ak_isp_vp_set_ae_attr( AK_ISP_AE_ATTR *p_ae)
{
	isp_dbg("%s enter.\n", __func__);

	if(p_ae->exp_time_max!=isp->ae_para.exp_time_max
		||p_ae->exp_time_min!=isp->ae_para.exp_time_min
		||p_ae->a_gain_max!=isp->ae_para.a_gain_max
		||p_ae->a_gain_min!=isp->ae_para.a_gain_min
		||p_ae->d_gain_max!=isp->ae_para.d_gain_max
		||p_ae->d_gain_min!=isp->ae_para.d_gain_min
		||p_ae->isp_d_gain_max!=isp->ae_para.isp_d_gain_max
		||p_ae->isp_d_gain_min!=isp->ae_para.isp_d_gain_min
		||p_ae->exp_step!=isp->ae_para.exp_step)
		isp->ae_para_update_flag = 1;
	isp->ae_last_lumi = isp->ae_run_info_para.current_calc_avg_lumi;
		
	memcpy(&(isp->ae_para),p_ae,sizeof(AK_ISP_AE_ATTR));
	if(isp->ae_para.exp_time_min<1)
		isp->ae_para.exp_time_min=1;

	if(isp->ae_para.d_gain_min<256)
		isp->ae_para.d_gain_min=256;

	if(isp->ae_para.a_gain_min<256)
		isp->ae_para.a_gain_min=256;

	if(isp->ae_para.exp_step<1)
		isp->ae_para.exp_step=1;

	return 0;   
}

/**
 * @brief: get auto exposure  param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_ae:  auto exposure  param
 */
int  ak_isp_vp_get_ae_attr(AK_ISP_AE_ATTR *p_ae)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_ae,&(isp->ae_para),sizeof(AK_ISP_AE_ATTR));

    return 0;
}

/**
 * @brief: get auto  exposure  running info 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_ae_stat:  auto  exposure  running info  param
 */
int  ak_isp_vp_get_ae_run_info(AK_ISP_AE_RUN_INFO*p_ae_stat)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_ae_stat,&(isp->ae_run_info_para),sizeof(AK_ISP_AE_RUN_INFO));

    return 0;
}

/**
 * @brief: get raw hist   running info 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_raw_hist_stat:  raw hist info  param
 */
int ak_isp_vp_get_raw_hist_stat_info( AK_ISP_RAW_HIST_STAT_INFO *p_raw_hist_stat)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_raw_hist_stat,&(isp->raw_hist_stat_para),sizeof(AK_ISP_RAW_HIST_STAT_INFO));

    return 0;
}


int ak_isp_vp_set_raw_hist_attr( AK_ISP_RAW_HIST_ATTR *p_raw_hist)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->raw_hist_para),p_raw_hist,sizeof(AK_ISP_RAW_HIST_ATTR));

    return 0;
}

int ak_isp_vp_get_raw_hist_attr( AK_ISP_RAW_HIST_ATTR *p_raw_hist)
{
    
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_raw_hist,&(isp->raw_hist_para),sizeof(AK_ISP_RAW_HIST_ATTR));

    return 0;
}

/**
 * @brief: get rgb hist   running info 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_rgb_hist_stat:  rgb hist info  param
 */

int ak_isp_vp_get_rgb_hist_stat_info( AK_ISP_RGB_HIST_STAT_INFO *p_rgb_hist_stat)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_rgb_hist_stat,&(isp->rgb_hist_stat_para),sizeof(AK_ISP_RGB_HIST_STAT_INFO));

    return 0;
}

int ak_isp_vp_set_rgb_hist_attr( AK_ISP_RGB_HIST_ATTR *p_rgb_hist)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->rgb_hist_para),p_rgb_hist,sizeof(AK_ISP_RGB_HIST_ATTR));

    return 0;
}

int ak_isp_vp_get_rgb_hist_attr( AK_ISP_RGB_HIST_ATTR *p_rgb_hist)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_rgb_hist,&(isp->rgb_hist_para),sizeof(AK_ISP_RGB_HIST_ATTR));

    return 0;
}

/**
 * @brief: get rgb hist   running info 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_yuv_hist_stat:  yuv hist info  param
 */
int ak_isp_vp_get_yuv_hist_stat_info( AK_ISP_YUV_HIST_STAT_INFO *p_yuv_hist_stat)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_yuv_hist_stat,&(isp->yuv_hist_stat_para),sizeof(AK_ISP_YUV_HIST_STAT_INFO));

    return 0;
}

int ak_isp_vp_set_yuv_hist_attr(AK_ISP_YUV_HIST_ATTR *p_yuv_hist)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->yuv_hist_para),p_yuv_hist,sizeof(AK_ISP_YUV_HIST_ATTR));

    return 0;
}

int ak_isp_vp_get_yuv_hist_attr(AK_ISP_YUV_HIST_ATTR *p_yuv_hist)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_yuv_hist,&(isp->yuv_hist_para),sizeof(AK_ISP_YUV_HIST_ATTR));

    return 0;
}

/**
 * @brief: set zone weight param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_weight:  weight  param
 */
int  ak_isp_vp_set_zone_weight(AK_ISP_WEIGHT_ATTR *p_weight)
{
    int i;
    unsigned long cmd;
    
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->weight_para),p_weight,sizeof(AK_ISP_WEIGHT_ATTR));

    //raw_reg_4
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][0], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C0);
    isp->reg_blkaddr2[3]= cmd;

    //raw_reg_5
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][1], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C1);
    isp->reg_blkaddr2[4]= cmd;

    //raw_reg_6
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][2], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C2);
    isp->reg_blkaddr2[5]= cmd;

    //raw_reg_7
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][3], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C3);
    isp->reg_blkaddr2[6]= cmd;

    //raw_reg_8
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][4], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C4);
    isp->reg_blkaddr2[7]= cmd;

    //raw_reg_9
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][5], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C5);
    isp->reg_blkaddr2[8]= cmd;

    //raw_reg_10
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][6], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C6);
    isp->reg_blkaddr2[9]= cmd;

    //raw_reg_11
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][7], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C7);
    isp->reg_blkaddr2[10]= cmd;

    //raw_reg_12
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][8], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C8);
    isp->reg_blkaddr2[11]= cmd;

    //raw_reg_13
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][9], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C9);
    isp->reg_blkaddr2[12]= cmd;

    //raw_reg_14
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][10], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C10);
    isp->reg_blkaddr2[13]= cmd;

    //raw_reg_15
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][11], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C11);
    isp->reg_blkaddr2[14]= cmd;

    //raw_reg_16
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][12], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C12);
    isp->reg_blkaddr2[15]= cmd;

    //raw_reg_17
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][13], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C13);
    isp->reg_blkaddr2[16]= cmd;

    //raw_reg_18
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][14], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C14);
    isp->reg_blkaddr2[17]= cmd;

    //raw_reg_19
    cmd = 0;
    for (i=0; i<8; i++)
    {
        cmd |= (LOW_BITS(p_weight->zone_weight[i][15], 4) << (i*4));
    }
    cmd = (cmd << ZONE_WEIGHT_C15);
    isp->reg_blkaddr2[18]= cmd;
    
    return 0;
}

int  ak_isp_vp_get_zone_weight( AK_ISP_WEIGHT_ATTR *p_weight)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_weight,&(isp->weight_para),sizeof(AK_ISP_WEIGHT_ATTR));

    return 0;
}

/*
 * The DMA statistics area, in 32-bit words: raw, RGB and Y histograms
 * (257 each), then the 3D-NR motion block of 16 words per row, then 5
 * AF words, then 10 AWB zones of 4 words. The motion block has 16 or 24
 * rows depending on how set_crop split the frame (EV300/EV330 pick 24
 * whenever the height divides by 24: 1080, 720, 360), so everything
 * after it moves. With 24 rows the whole area is 1200 words, exactly
 * DMA_STAT_DATA_SIZE.
 */
#define STAT_MD_OFFSET		(257 + 257 + 257)
#define STAT_MD_ROW_WORDS	16

static int _md_rows(void)
{
	return isp->tnr_height_block_num == 24 ? 24 : 16;
}

static int _af_stat_offset(void)
{
	return STAT_MD_OFFSET + _md_rows() * STAT_MD_ROW_WORDS;
}

/**************************************AWB************************************************/
//get wb stat
//calc target awb gain
//

int _get_awb_stat_result(void)
{
    int i;
    unsigned long *stat_addr;

    switch(isp->current_buffer)
    {
    case BUFFER_ONE:
        stat_addr = isp->stat_addr[BUFFER_ONE];
        break;
    case BUFFER_TWO:
        stat_addr = isp->stat_addr[BUFFER_TWO];
        break;
    case BUFFER_THREE:
        stat_addr = isp->stat_addr[BUFFER_THREE];
        break;
    case BUFFER_FOUR:
        stat_addr = isp->stat_addr[BUFFER_FOUR];
        break;
    default:
        break;
    }

	stat_addr = stat_addr + _af_stat_offset() + 5;

	for(i=0;i<10;i++)
    {
        isp->awb_stat_info_para.total_R[i]  = *stat_addr++ ;
        isp->awb_stat_info_para.total_G[i]  = *stat_addr++;
        isp->awb_stat_info_para.total_B[i]  = *stat_addr++ ;
        isp->awb_stat_info_para.total_cnt[i]= *stat_addr++;
    }
  
    return 0;   
}

int _get_normal_color_temp_index(void)
{
    unsigned long max_index;
    unsigned long max_cnt;

    max_index = 0;
    max_cnt =isp->awb_stat_info_para.total_cnt[0];
    if(max_cnt<=isp->awb_stat_info_para.total_cnt[1])
    {
        max_index = 1;
        max_cnt =isp->awb_stat_info_para.total_cnt[1];
    }
    if(max_cnt<=isp->awb_stat_info_para.total_cnt[2])
    {
        max_index = 2;
        max_cnt =isp->awb_stat_info_para.total_cnt[2];
    }
   if(max_cnt<=isp->awb_stat_info_para.total_cnt[3])
   {
        max_index = 3;
        max_cnt =isp->awb_stat_info_para.total_cnt[3];
   }

   if(max_cnt<=isp->awb_stat_info_para.total_cnt[4])
   {
        max_index = 4;
        max_cnt =isp->awb_stat_info_para.total_cnt[4];
   }
   
   return max_index;
}

int _get_abnormal_color_temp_index(void)
{
    unsigned long max_index;
    unsigned long max_cnt;
    int i;

    max_index = 0;
    max_cnt =isp->awb_stat_info_para.total_cnt[0];
    for(i=1; i<10; i++)
    {
	    if(max_cnt<=isp->awb_stat_info_para.total_cnt[i])
	    {
	        max_index = i;
	        max_cnt =isp->awb_stat_info_para.total_cnt[i];
	    }
    }
    
   return max_index;
}

#if 0
int _calc_awb_gain(unsigned long temp_index)
{
    unsigned long max_channel_value = 0;
    int  max_channel_index = 0;
    max_channel_value = isp->awb_stat_info_para.total_R[temp_index];
    max_channel_index = 0;
    if(max_channel_value<=isp->awb_stat_info_para.total_G[temp_index])
    {
        max_channel_index  =1;
        max_channel_value  = isp->awb_stat_info_para.total_G[temp_index];
    }
    if(max_channel_value<=isp->awb_stat_info_para.total_B[temp_index])
    {
        max_channel_index  =2;
        max_channel_value  = isp->awb_stat_info_para.total_B[temp_index];

    }
    if((isp->awb_stat_info_para.total_R[temp_index]>>8)==0||
    	(isp->awb_stat_info_para.total_G[temp_index]>>8)==0||
    	(isp->awb_stat_info_para.total_B[temp_index]>>8)==0)
    	return -1;
    
    switch(max_channel_index)
    {
    case 0:
        isp->awb_algo.calc_r_gain = 256;
        isp->awb_algo.calc_g_gain = max_channel_value/(isp->awb_stat_info_para.total_G[temp_index]>>8);
        isp->awb_algo.calc_b_gain = max_channel_value/(isp->awb_stat_info_para.total_B[temp_index]>>8);
        break;
    case 1:
        isp->awb_algo.calc_r_gain = max_channel_value/(isp->awb_stat_info_para.total_R[temp_index]>>8);
        isp->awb_algo.calc_g_gain = 256;
        isp->awb_algo.calc_b_gain = max_channel_value/(isp->awb_stat_info_para.total_B[temp_index]>>8);
        break;
    case 2:
        isp->awb_algo.calc_r_gain = max_channel_value/(isp->awb_stat_info_para.total_R[temp_index]>>8);
        isp->awb_algo.calc_g_gain = max_channel_value/(isp->awb_stat_info_para.total_G[temp_index]>>8);
        isp->awb_algo.calc_b_gain = 256;
        break;
    default:
        break;
    }

   
    return 0;        
}
#endif

const int awb_weight[17]=
{
	8, 8, 7,6,4,3,2,1,
	0,0,0,0,0,0,0,0,0
};

int _calc_awb_gain(unsigned long temp_index)
{
    unsigned long max_channel_value = 0;
    unsigned long total_R, total_G, total_B;
    int  max_channel_index = 0;
    int i;

    total_R = (isp->awb_stat_info_para.total_R[temp_index] >> 8);
    total_G = (isp->awb_stat_info_para.total_G[temp_index] >> 8);
    total_B = (isp->awb_stat_info_para.total_B[temp_index] >> 8);
    for(i=0; i<10; i++)
    {
		if(i==temp_index)
			continue;
    		
		if(isp->awb_stat_info_para.total_cnt[i]>isp->awb_para.total_cnt_thresh)
		{
			int weight_index, weight;
			
			weight_index = (isp->awb_stat_info_para.total_cnt[temp_index] - isp->awb_stat_info_para.total_cnt[i])*16/isp->awb_stat_info_para.total_cnt[temp_index];
			if(weight_index<0)
				weight_index = 0;
			if(weight_index>16)
				weight_index = 16;
			weight = awb_weight[weight_index];
			total_R += (isp->awb_stat_info_para.total_R[i]>>8)*weight/8;
			total_G += (isp->awb_stat_info_para.total_G[i]>>8)*weight/8;
			total_B += (isp->awb_stat_info_para.total_B[i]>>8)*weight/8;
		}
    }
    
    max_channel_value = total_R;
    max_channel_index = 0;
    if(max_channel_value<=total_G)
    {
        max_channel_index  =1;
        max_channel_value  = total_G;
    }
    if(max_channel_value<=total_B)
    {
        max_channel_index  =2;
        max_channel_value  = total_B;

    }
    if((total_R>>8)==0 || (total_G>>8)==0 || (total_B>>8)==0)
    	return -1;
    
    switch(max_channel_index)
    {
    case 0:
        isp->awb_algo.calc_r_gain = 256;
        isp->awb_algo.calc_g_gain = max_channel_value/(total_G>>8);
        isp->awb_algo.calc_b_gain = max_channel_value/(total_B>>8);
        break;
    case 1:
        isp->awb_algo.calc_r_gain = max_channel_value/(total_R>>8);
        isp->awb_algo.calc_g_gain = 256;
        isp->awb_algo.calc_b_gain = max_channel_value/(total_B>>8);
        break;
    case 2:
        isp->awb_algo.calc_r_gain = max_channel_value/(total_R>>8);
        isp->awb_algo.calc_g_gain = max_channel_value/(total_G>>8);
        isp->awb_algo.calc_b_gain = 256;
        break;
    default:
        break;
    }
   
    return 0;        
}

static int _calc_wb_step(void)
{
    int  deltar,deltab,deltag;
    int  step = 0;

    step = isp->awb_para.auto_wb_step;
    if(step == 0)
    {
        step = 1;
    }
    
	/*
	isp->awb_stat_info_para.current_colortemp_index=ISP2_COLORTEMP_MODE_D65;
	isp->awb_algo.current_r_gain = isp->awb_stat_info_para.r_gain =	isp->awb_algo.target_r_gain = isp->mwb_para.r_gain;
	isp->awb_algo.current_g_gain = isp->awb_stat_info_para.g_gain = isp->awb_algo.target_g_gain = isp->mwb_para.g_gain;
	isp->awb_algo.current_b_gain = isp->awb_stat_info_para.b_gain = isp->awb_algo.target_b_gain = isp->mwb_para.b_gain;
	*/
	
    deltar = isp->awb_algo.target_r_gain - isp->awb_algo.current_r_gain;
    if(deltar>0)
    {
        deltar= (deltar+step-1)/step;
    }
    if(deltar<0)
    {
       deltar= (deltar-step+1)/step;
    }
    isp->awb_algo.current_r_gain += deltar;

    deltab =isp->awb_algo.target_b_gain - isp->awb_algo.current_b_gain;
    if(deltab>0)
    {
       deltab = (deltab+step-1)/step;
    }
    if(deltab<0)
    {
        deltab = (deltab-step+1)/step;
    }

    isp->awb_algo.current_b_gain += deltab;
    deltag =isp->awb_algo.target_g_gain - isp->awb_algo.current_g_gain;
    if(deltag>0)
    {
       deltag= (deltag+step-1)/step;
    }
    if(deltag<0)
    {
        deltag= (deltag-step+1)/step;
    }
    isp->awb_algo.current_g_gain += deltag;
    
    return 0;
}

static int _calc_ccm_step(void)
{
	int  step = 0;
	int i,  j;
	int changed = 0;
	int identity_ccm[3][3] = {{256, 0, 0},
				{0, 256, 0},
				{0, 0, 256},};
	int strength = isp->ccm_info.strength;
	T_S16 target_ccm[3][3];
	T_U16 cc_enable;

	step = isp->awb_para.auto_wb_step;
	if(step == 0)
	{
		step = 1;
	}

	cc_enable = isp->ccm_info.ccm_target.cc_enable;

	for(i=0; i<3; i++)
		for(j=0; j<3; j++)
		{
			target_ccm[i][j] = (isp->ccm_info.ccm_target.ccm[i][j]*strength
				+ identity_ccm[i][j]*(256-strength))/256;
		}

	for(i=0; i<3; i++)
		for(j=0; j<3; j++)
		{
			int delta = target_ccm[i][j] - isp->ccm_info.ccm_current.ccm[i][j];

			if(delta != 0)
				changed = 1;
			if(delta > 0)
				delta = (delta+step-1)/step;
			if(delta < 0)
				delta = (delta-step+1)/step;
			isp->ccm_info.ccm_current.ccm[i][j] += delta;
		}

	if(isp->ccm_info.ccm_current.cc_enable != cc_enable)
	{
		changed = 1;
		isp->ccm_info.ccm_current.cc_enable = cc_enable;
	}

	return changed;
}

static int _calc_hue_step(void)
{
	int  step = 0;
	int i;
	int changed = 0;
	T_U16 hue_sat_en;

	step = isp->awb_para.auto_wb_step;
	if(step == 0)
	{
		step = 1;
	}

	hue_sat_en = isp->hue_info.hue_target.hue_sat_en;

	for(i = 0; i < 64;i++)
	{
		int delta_a = isp->hue_info.hue_target.hue_lut_a[i] - isp->hue_info.hue_current.hue_lut_a[i];
		int delta_b = isp->hue_info.hue_target.hue_lut_b[i] - isp->hue_info.hue_current.hue_lut_b[i];
		int delta_s = isp->hue_info.hue_target.hue_lut_s[i] - isp->hue_info.hue_current.hue_lut_s[i];

		if(delta_a > 0)
			delta_a = (delta_a+step-1)/step;
		if(delta_a < 0)
			delta_a = (delta_a-step+1)/step;
		isp->hue_info.hue_current.hue_lut_a[i] += delta_a;

		if(delta_b > 0)
			delta_b = (delta_b+step-1)/step;
		if(delta_b < 0)
			delta_b = (delta_b-step+1)/step;
		isp->hue_info.hue_current.hue_lut_b[i] += delta_b;

		if(delta_s > 0)
			delta_s = (delta_s+step-1)/step;
		if(delta_s < 0)
			delta_s = (delta_s-step+1)/step;
		isp->hue_info.hue_current.hue_lut_s[i] += delta_s;

		if(delta_a != 0 || delta_b != 0 || delta_s != 0)
			changed = 1;
	}

	if(isp->hue_info.hue_current.hue_sat_en != hue_sat_en)
	{
		changed = 1;
		isp->hue_info.hue_current.hue_sat_en = hue_sat_en;
	}

	return changed;
}

int _awb_ex_ctrl(unsigned long temp_index, AK_ISP_WB_GAIN *wb_gain)
{
	AK_ISP_AWB_CTRL *awb_ctrl;

	wb_gain->r_gain = isp->awb_stat_info_para.r_gain;
	wb_gain->g_gain = isp->awb_stat_info_para.g_gain;
	wb_gain->b_gain = isp->awb_stat_info_para.b_gain;
	if(isp->awb_ex_ctrl_para.awb_ex_ctrl_enable)
	{
		awb_ctrl = &isp->awb_ex_ctrl_para.awb_ctrl[temp_index];
		if(wb_gain->r_gain>awb_ctrl->rgain_max)
			wb_gain->r_gain = awb_ctrl->rgain_max;
		if(wb_gain->r_gain<awb_ctrl->rgain_min)
			wb_gain->r_gain = awb_ctrl->rgain_min;
		if(wb_gain->g_gain>awb_ctrl->ggain_max)
			wb_gain->g_gain = awb_ctrl->ggain_max;
		if(wb_gain->g_gain<awb_ctrl->ggain_min)
			wb_gain->g_gain = awb_ctrl->ggain_min;
		if(wb_gain->b_gain>awb_ctrl->bgain_max)
			wb_gain->b_gain = awb_ctrl->bgain_max;
		if(wb_gain->b_gain<awb_ctrl->bgain_min)
			wb_gain->b_gain = awb_ctrl->bgain_min;

		wb_gain->r_gain = (wb_gain->r_gain*awb_ctrl->rgain_ex)>>8;
		wb_gain->b_gain = (wb_gain->b_gain*awb_ctrl->bgain_ex)>>8;
	}

	return 0;
}

static inline int _get_total_gain(void)
{
	unsigned int a_gain = isp->ae_run_info_para.current_a_gain;
	unsigned int d_gain = isp->ae_run_info_para.current_d_gain;
	unsigned int isp_d_gain = isp->ae_run_info_para.current_isp_d_gain;
	unsigned int all_gain;
	all_gain=(((a_gain*d_gain)>>GAIN_SHIFT)*isp_d_gain)>>GAIN_SHIFT;

	return all_gain;
}

#define AWB_WORK_PERIOD	2
int ak_isp_awb_work(void)
{
	unsigned long  color_index = 0;
	AK_ISP_WB_GAIN wb_gain;
	int i;

	if(isp==0)
		return 0;
	/* Only run AWB processing every AWB_WORK_PERIOD frames. */
	if(isp->frame_cnt%AWB_WORK_PERIOD!=0)
    	return 0;
	_get_awb_stat_result();
	_set_awb_attr(&(isp->awb_para));
	  
	if(WB_OPS_TYPE_MANU==isp->wb_type_para.wb_type )
	{
		unsigned int isp_d_gain;

		isp_d_gain= isp->ae_run_info_para.current_isp_d_gain;
		wb_gain.r_gain = isp->mwb_para.r_gain;
		wb_gain.g_gain = isp->mwb_para.g_gain;
		wb_gain.b_gain = isp->mwb_para.b_gain;
		wb_gain.r_offset = (isp->mwb_para.r_offset*isp_d_gain)/256;
		wb_gain.g_offset = (isp->mwb_para.g_offset*isp_d_gain)/256;
		wb_gain.b_offset = (isp->mwb_para.b_offset*isp_d_gain)/256;
		ak_isp_updata_wb_gain(&wb_gain);
		_set_ccm_attr(&(isp->ccm_para.manual_ccm));
		_set_hue_attr(&(isp->hue_para.manual_hue));
		
		return 0;
	}
	else
	{
		unsigned int total_gain;
		unsigned int isp_d_gain;

		isp_d_gain= isp->ae_run_info_para.current_isp_d_gain;		
		_calc_wb_step();
		wb_gain.r_gain = isp->awb_algo.current_r_gain;
		wb_gain.g_gain = isp->awb_algo.current_g_gain;
		wb_gain.b_gain = isp->awb_algo.current_b_gain;
		wb_gain.r_offset = (isp->mwb_para.r_offset*isp_d_gain)/256;
		wb_gain.g_offset = (isp->mwb_para.g_offset*isp_d_gain)/256;
		wb_gain.b_offset = (isp->mwb_para.b_offset*isp_d_gain)/256;
		ak_isp_updata_wb_gain(&wb_gain);

		total_gain = _get_total_gain();
		if(total_gain <= 0x2000)
			isp->ccm_info.strength = 256;
		else if(total_gain >= 0x4000)
			isp->ccm_info.strength = 0;
		else
			isp->ccm_info.strength = 256-((total_gain-0x2000)>>5);

		if(_calc_ccm_step())
		{
			_set_ccm_attr(&isp->ccm_info.ccm_current);
		}

		if(_calc_hue_step())
		{
			_set_hue_attr(&isp->hue_info.hue_current);
		}
		
	}
	
	
	//determine color temp

    color_index = _get_abnormal_color_temp_index();
	if(isp->awb_stat_info_para.total_cnt[color_index]>=isp->awb_para.total_cnt_thresh)
	{
		int change_cnt_thresh;
		//if(isp->awb_stat_info_para.current_colortemp_index<10)
			change_cnt_thresh = isp->awb_stat_info_para.total_cnt[isp->awb_stat_info_para.current_colortemp_index]*(128+isp->awb_para.colortemp_stable_cnt_thresh+13)/128;
		//else
		//	change_cnt_thresh = 0;

		if(isp->awb_stat_info_para.current_colortemp_index != color_index
			&&isp->awb_stat_info_para.total_cnt[color_index] >change_cnt_thresh)
		{
			isp->awb_stat_info_para.colortemp_stable_cnt[color_index]++;
			for(i=0;i<10;i++)
			{
				if(i!=color_index)
					isp->awb_stat_info_para.colortemp_stable_cnt[i] = 0;   
			} 
		}
		else
		{           	
			for(i=0;i<10;i++)
			{
				isp->awb_stat_info_para.colortemp_stable_cnt[i] = 0;
				
			}
		}
	}
	else
	{
		for(i=0;i<10;i++)
		{
			isp->awb_stat_info_para.colortemp_stable_cnt[i] = 0;   
		} 
	}
	   
	if(isp->awb_stat_info_para.colortemp_stable_cnt[color_index]>=isp->awb_para.colortemp_stable_cnt_thresh)
	{
		isp->awb_stat_info_para.current_colortemp_index = color_index;
		isp->awb_stat_info_para.colortemp_stable_cnt[color_index] = 0;
	}

	//if(isp->awb_stat_info_para.current_colortemp_index==color_index)
	if(isp->awb_stat_info_para.total_cnt[isp->awb_stat_info_para.current_colortemp_index]>=isp->awb_para.total_cnt_thresh)
	{
		//calc wb gain
		if(_calc_awb_gain(isp->awb_stat_info_para.current_colortemp_index)>=0)
		{
			if(isp->awb_needs_seed)
			{
				isp->awb_stat_info_para.r_gain = isp->awb_algo.calc_r_gain;
				isp->awb_stat_info_para.g_gain = isp->awb_algo.calc_g_gain;
				isp->awb_stat_info_para.b_gain = isp->awb_algo.calc_b_gain;
			}
			else
			{
				isp->awb_stat_info_para.r_gain = (isp->awb_stat_info_para.r_gain+isp->awb_algo.calc_r_gain)/2;
				isp->awb_stat_info_para.g_gain = (isp->awb_stat_info_para.g_gain+isp->awb_algo.calc_g_gain)/2;
				isp->awb_stat_info_para.b_gain = (isp->awb_stat_info_para.b_gain+isp->awb_algo.calc_b_gain)/2;
			}

			_awb_ex_ctrl(isp->awb_stat_info_para.current_colortemp_index, &wb_gain);
			if(abs(wb_gain.r_gain-isp->awb_algo.target_r_gain)>5)
				isp->awb_algo.target_r_gain = wb_gain.r_gain;
			if(abs(wb_gain.g_gain-isp->awb_algo.target_g_gain)>5)
				isp->awb_algo.target_g_gain = wb_gain.g_gain;
			if(abs(wb_gain.b_gain-isp->awb_algo.target_b_gain)>5)
				isp->awb_algo.target_b_gain = wb_gain.b_gain;

			if(isp->awb_needs_seed)
			{
				isp->awb_algo.current_r_gain = isp->awb_algo.target_r_gain;
				isp->awb_algo.current_g_gain = isp->awb_algo.target_g_gain;
				isp->awb_algo.current_b_gain = isp->awb_algo.target_b_gain;
				isp->awb_needs_seed = 0;
			}
		}
	}

	/*if(isp->awb_stat_info_para.current_colortemp_index!=isp->last_colortemp)
	{
		//update demosacing para
		AK_ISP_DEMO_ATTR demo_para;

		memcpy(&demo_para, &isp->demo_para, sizeof(AK_ISP_DEMO_ATTR));
		demo_para.dm_rg_gain = isp->awb_algo.target_r_gain*64/isp->awb_algo.target_g_gain;
		demo_para.dm_bg_gain = isp->awb_algo.target_b_gain*64/isp->awb_algo.target_g_gain;
		demo_para.dm_gr_gain = isp->awb_algo.target_g_gain*64/isp->awb_algo.target_r_gain;
		demo_para.dm_gb_gain = isp->awb_algo.target_g_gain*64/isp->awb_algo.target_b_gain;		
		ak_isp_vp_set_demo_attr(&demo_para);
	}*/

	if(isp->awb_para.colortemp_envi[isp->awb_stat_info_para.current_colortemp_index]!=
		isp->awb_para.colortemp_envi[isp->last_colortemp]
		||isp->linkage_ccm_update_flag!=0
		||isp->linkage_hue_update_flag!=0)
	{
		_isp_ccm_change_with_envi(isp->awb_stat_info_para.current_colortemp_index);
		isp->linkage_ccm_update_flag=0;
		_isp_hue_change_with_envi(isp->awb_stat_info_para.current_colortemp_index);
		isp->linkage_hue_update_flag=0;
	}

	isp->last_colortemp = isp->awb_stat_info_para.current_colortemp_index;

	return 0;
}	

/*************************************AF*************************************************/
int _get_af_stat(void)
{
    unsigned long *stat_addr;
    int i = 0;
	
    switch(isp->current_buffer){
    case BUFFER_ONE:
        stat_addr = isp->stat_addr[BUFFER_ONE];
        break;
    case BUFFER_TWO:
        stat_addr = isp->stat_addr[BUFFER_TWO];
        break;
    case BUFFER_THREE:
        stat_addr = isp->stat_addr[BUFFER_THREE];
        break;
    case BUFFER_FOUR:
        stat_addr = isp->stat_addr[BUFFER_FOUR];
        break;
    default:
        break;
        }

	stat_addr = stat_addr + _af_stat_offset();

    for(i = 0; i < 5;i++)
		isp->af_stat_para.af_statics[i]= stat_addr[i];
    return 0;
}

/*************************************3DNR*************************************************/
int _get_3dnr_stat(void)
{
    int i, j;
    unsigned long *stat_addr;
	int height_block_num;


    switch(isp->current_buffer)
    {
    case BUFFER_ONE:
        stat_addr = isp->stat_addr[BUFFER_ONE];
        break;
    case BUFFER_TWO:
        stat_addr = isp->stat_addr[BUFFER_TWO];
        break;
    case BUFFER_THREE:
        stat_addr = isp->stat_addr[BUFFER_THREE];
        break;
    case BUFFER_FOUR:
        stat_addr = isp->stat_addr[BUFFER_FOUR];
        break;
    default:
        break;
  	}
  	
    stat_addr = stat_addr + STAT_MD_OFFSET;

	height_block_num = _md_rows();

    for(i=0; i<height_block_num; i++)
    {
    	for(j=0; j<32; j+=2)
    	{
			isp->_3d_nr_stat_para.MD_stat[i][j] = stat_addr[0]&0xffff;
			isp->_3d_nr_stat_para.MD_stat[i][j+1] = (stat_addr[0]>>16)&0xffff;
			stat_addr++;
    	}
    }

    return 0;
}

/**************************************AE************************************************/
int _get_raw_hist(void)
{
    int i = 0;
    unsigned long *stat_addr;
    unsigned long pixel_total = 0;
    switch(isp->current_buffer)
    {
    case BUFFER_ONE:
        stat_addr = isp->stat_addr[BUFFER_ONE];
        break;
    case BUFFER_TWO:
        stat_addr = isp->stat_addr[BUFFER_TWO];
        break;
    case BUFFER_THREE:
        stat_addr = isp->stat_addr[BUFFER_THREE];
        break;
    case BUFFER_FOUR:
        stat_addr = isp->stat_addr[BUFFER_FOUR];
        break;
    default:
        break;
    }

    stat_addr = stat_addr;
    for(i=0;i<256;i++)
    {
        isp->raw_hist_stat_para.raw_g_hist[i] = stat_addr[i];
        pixel_total += stat_addr[i];
    }

    if (pixel_total <= 0) {
        pixel_total = 1;
    }

    isp->raw_hist_stat_para.raw_g_total = stat_addr[256];
    isp->ae_run_info_para.current_calc_avg_lumi = stat_addr[256]/pixel_total;
    
    return 0;
}

int _get_rgb_hist(void)
{
    int i;
    unsigned long *stat_addr;
    unsigned long pixel_total = 0;
    
    switch(isp->current_buffer)
    {
    case BUFFER_ONE:
        stat_addr = isp->stat_addr[BUFFER_ONE];
        break;
    case BUFFER_TWO:
        stat_addr = isp->stat_addr[BUFFER_TWO];
        break;
    case BUFFER_THREE:
        stat_addr = isp->stat_addr[BUFFER_THREE];
        break;
    case BUFFER_FOUR:
        stat_addr = isp->stat_addr[BUFFER_FOUR];
        break;
    default:
        break;
    }
    
    stat_addr = stat_addr + 257;
    for(i=0;i<256;i++)
    {
        isp->rgb_hist_stat_para.rgb_hist[i] = stat_addr[i];
        pixel_total += stat_addr[i];
    }
    isp->rgb_hist_stat_para.rgb_total= stat_addr[256];
    //isp->ae_run_info_para.current_calc_avg_lumi = stat_addr[256]/pixel_total;

    return 0;
}

int _get_yuv_hist(void)
{
    int i;
    unsigned long *stat_addr;
    unsigned long pixel_total = 0;
    switch(isp->current_buffer){
    case BUFFER_ONE:
        stat_addr = isp->stat_addr[BUFFER_ONE];
        break;
    case BUFFER_TWO:
        stat_addr = isp->stat_addr[BUFFER_TWO];
        break;
    case BUFFER_THREE:
        stat_addr = isp->stat_addr[BUFFER_THREE];
        break;
    case BUFFER_FOUR:
        stat_addr = isp->stat_addr[BUFFER_FOUR];
        break;
    default:
        break;
    }
    stat_addr = stat_addr + 257 + 257;
    for(i=0;i<256;i++)
    {
        isp->yuv_hist_stat_para.y_hist[i] = stat_addr[i];
        pixel_total += stat_addr[i];
    }
    isp->yuv_hist_stat_para.y_total= stat_addr[256];
    //isp->ae_run_info_para.current_calc_avg_lumi = stat_addr[256]/pixel_total;

    return 0;
}

const int grey_thre[17]=
{
	0,   16, 32,  48, 64, 80,96,112,
	128,144,160,176,192,208,224,240, 256
};	

int _isp_compensation_all_weight_with_ae(void)
{   
	int i=0;	
	int j=0;   
	int histo_sum[16];
	int pixel_sum[16];
	int new_avg = 0,sum_num=0,value = 0;	
	int pix_num = 0;
	T_U32 *histo_arr;
	int hist_weight[256];

	for(i=0; i<8; i++)
	{
		int step;
		
		step = isp->ae_para.hist_weight[i+1]-isp->ae_para.hist_weight[i];
		for(j=0; j<16; j++)
		{
			hist_weight[i*16+j] = isp->ae_para.hist_weight[i]+(step*j)/16;
		}
	}

	for(i=8; i<16; i++)
	{
		int step;
		
		step = isp->ae_para.hist_weight[i]-isp->ae_para.hist_weight[i-1];
		for(j=0; j<16; j++)
		{
			hist_weight[i*16+j] = isp->ae_para.hist_weight[i-1]+(step*(j+1))/16;
		}
	}

	if(isp->ae_para.OE_suppress_en)
		histo_arr = isp->rgb_hist_stat_para.rgb_hist;
	else
		histo_arr = isp->raw_hist_stat_para.raw_g_hist;
	memset(histo_sum,0,sizeof(histo_sum));    
	memset(pixel_sum,0,sizeof(pixel_sum));    	
	
	for(i=0; i<16;i++)   
	{        
		for(j=grey_thre[i];j<grey_thre[i+1];j++)        
		{	        
			histo_sum[i] += ((j*histo_arr[j])*hist_weight[j]);
			pixel_sum[i] += (histo_arr[j]*hist_weight[j]);
		}   
	}	 
	for(i=0;i<256;i++)	 
	{	 	
		pix_num += histo_arr[i];	 
	}	 
	      
	for(i=0;i<16;i++)     
	{               	
		value += histo_sum[i];
		sum_num += pixel_sum[i];
	}   
	
	if (sum_num == 0)       
		sum_num = 1; 
	
	new_avg = value/sum_num; 
	if(new_avg>255)       
	  new_avg =255;  
	return new_avg;
}

int _cmos_updata_isp_d_gain(T_U32 d_gain)
{
    return 0;
}

int _cmos_updata_d_gain(T_U32 d_gain)
{
	//isp->sensor_cb.sensor_update_d_gain_func(d_gain);
    
    return isp->sensor_cb.sensor_update_d_gain_func(d_gain); 
}

int _cmos_updata_a_gain(T_U32 a_gain)
{ 
  //isp->sensor_cb.sensor_update_a_gain_func(a_gain);
  
  return isp->sensor_cb.sensor_update_a_gain_func(a_gain);
}

int _cmos_updata_exp_time(T_U32 exp_time)
{
	//isp->sensor_cb.sensor_updata_exp_time_func(exp_time);
    
    return isp->sensor_cb.sensor_updata_exp_time_func(exp_time);
}

/**
 * @brief: update isp digtal gain
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] isp_d_gain:  isp digtal gain
 */
int _isp_update_isp_d_gain(T_U32 isp_d_gain)
{
	AK_ISP_BLC         *p_blc;
	int envi_flag =  isp->curr_envi_flag;

	isp->ae_isp_d_gain = isp_d_gain;
	switch(envi_flag){
	case ENVI_OUTDOOR:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_OUTDOOR]);
		break;
	case ENVI_INDOOR:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_INDOOR]);
		break;
	case ENVI_ISO_2:    
		p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_2]);
		break;
	case ENVI_ISO_4:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_4]);
		break;
	case ENVI_ISO_8:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_8]);
		break;
	case ENVI_ISO_16:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_16]);
		break;
	case ENVI_ISO_32:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_32]);
		break;
	case ENVI_ISO_64:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_64]);
		break;
	case ENVI_ISO_128:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_128]);
		break;
	default:
		break;
	}
	
	if(MODE_MANUAL == isp->blc_para.blc_mode)
		p_blc = &(isp->blc_para.m_blc);

	_set_blc_attr(p_blc);
	return 0;
}

/**
 * @brief: update isp param with envimnet changing
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] envi_flag:  enviment flag
 */

int _isp_para_change_with_envi(enum  envi_flag envi_flag)
{
	//get all gain
	//int envi_flag   =  0;//_cmos_get_all_gain();
	AK_ISP_BLC         *p_blc;
	AK_ISP_DDPC	      *p_dpc;
	AK_ISP_GB          *p_gb;
	AK_ISP_NR1         *p_nr1;
	AK_ISP_NR2         *p_nr2;
	AK_ISP_3D_NR       *p_3d;
	AK_ISP_FCS         *p_fcs;

	AK_ISP_SHARP       *p_sharp;
	AK_ISP_SATURATION  *p_satu;
	AK_ISP_WDR		  *p_wdr;
	AK_ISP_LSC_ATTR		lsc;
	int				lsc_strength;
	
	isp->curr_envi_flag = envi_flag;
	isp->cb.cb_memcpy(&lsc, &(isp->lsc_para),sizeof(AK_ISP_LSC_ATTR));
	lsc_strength = isp->af_para.af_win3_left;	//env1
	switch(envi_flag){
	case ENVI_OUTDOOR:
		p_blc = &(isp->blc_para.linkage_blc[ENVI_OUTDOOR]);
		p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_OUTDOOR]); 
		p_gb  =&(isp->gb_para.linkage_gb[ENVI_OUTDOOR]);
		p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_OUTDOOR]);
		p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_OUTDOOR]);
		p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_OUTDOOR]);

		p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_OUTDOOR]);
		p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_OUTDOOR]);
		p_satu =&(isp->saturation_para.linkage_sat[ENVI_OUTDOOR]);
		p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_OUTDOOR]);
		lsc_strength = isp->af_para.af_win3_left;//env1
		break;
	case ENVI_INDOOR:
        p_blc = &(isp->blc_para.linkage_blc[ENVI_INDOOR]);
        p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_INDOOR]); 
        p_gb  =&(isp->gb_para.linkage_gb[ENVI_INDOOR]);
        p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_INDOOR]);
        p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_INDOOR]);
        p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_INDOOR]);
      
        p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_INDOOR]);
        p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_INDOOR]);
        p_satu =&(isp->saturation_para.linkage_sat[ENVI_INDOOR]);
        p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_INDOOR]);
        lsc_strength = isp->af_para.af_win3_right;//env2
        break;
    case ENVI_ISO_2:    
        p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_2]);
        p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_ISO_2]); 
        p_gb  =&(isp->gb_para.linkage_gb[ENVI_ISO_2]);
        p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_ISO_2]);
        p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_ISO_2]);
        p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_ISO_2]);
       
        p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_ISO_2]);
        p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_ISO_2]);
        p_satu =&(isp->saturation_para.linkage_sat[ENVI_ISO_2]);
        p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_ISO_2]);
        lsc_strength = isp->af_para.af_win3_top;//env3
        break;
    case ENVI_ISO_4:
        p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_4]);
        p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_ISO_4]); 
        p_gb  =&(isp->gb_para.linkage_gb[ENVI_ISO_4]);
        p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_ISO_4]);
        p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_ISO_4]);
        p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_ISO_4]);
      
        p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_ISO_4]);
        p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_ISO_4]);
        p_satu =&(isp->saturation_para.linkage_sat[ENVI_ISO_4]);
        p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_ISO_4]);
        lsc_strength = isp->af_para.af_win3_bottom;//env4
        break;
    case ENVI_ISO_8:
        p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_8]);
        p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_ISO_8]); 
        p_gb  =&(isp->gb_para.linkage_gb[ENVI_ISO_8]);
        p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_ISO_8]);
        p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_ISO_8]);
        p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_ISO_8]);
    
        p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_ISO_8]);
        p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_ISO_8]);
        p_satu =&(isp->saturation_para.linkage_sat[ENVI_ISO_8]);
        p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_ISO_8]);
        lsc_strength = isp->af_para.af_win4_left;//env5
        break;
    case ENVI_ISO_16:
        p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_16]);
        p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_ISO_16]); 
        p_gb  =&(isp->gb_para.linkage_gb[ENVI_ISO_16]);
        p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_ISO_16]);
        p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_ISO_16]);
        p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_ISO_16]);
      
        p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_ISO_16]);
        p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_ISO_16]);
        p_satu =&(isp->saturation_para.linkage_sat[ENVI_ISO_16]);
        p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_ISO_16]);
        lsc_strength = isp->af_para.af_win4_right;//env6
        break;
    case ENVI_ISO_32:
        p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_32]);
        p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_ISO_32]); 
        p_gb  =&(isp->gb_para.linkage_gb[ENVI_ISO_32]);
        p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_ISO_32]);
        p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_ISO_32]);
        p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_ISO_32]);
       
        p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_ISO_32]);
        p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_ISO_32]);
        p_satu =&(isp->saturation_para.linkage_sat[ENVI_ISO_32]);
        p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_ISO_32]);
        lsc_strength = isp->af_para.af_win4_top;//env7
        break;
    case ENVI_ISO_64:
        p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_64]);
        p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_ISO_64]); 
        p_gb  =&(isp->gb_para.linkage_gb[ENVI_ISO_64]);
        p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_ISO_64]);
        p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_ISO_64]);
        p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_ISO_64]);
       
        p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_ISO_64]);
        p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_ISO_64]);
        p_satu =&(isp->saturation_para.linkage_sat[ENVI_ISO_64]);
        p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_ISO_64]);
        lsc_strength = isp->af_para.af_win4_bottom;//env8
        break;
    case ENVI_ISO_128:
        p_blc = &(isp->blc_para.linkage_blc[ENVI_ISO_128]);
        p_dpc =&(isp->dpc_para.linkage_ddpc[ENVI_ISO_128]); 
        p_gb  =&(isp->gb_para.linkage_gb[ENVI_ISO_128]);
        p_nr1 = &(isp->nr1_para.linkage_nr1[ENVI_ISO_128]);
        p_nr2 = &(isp->nr2_para.linkage_nr2[ENVI_ISO_128]);
        p_3d = &(isp->_3d_nr_para.linkage_3d_nr[ENVI_ISO_128]);
    
        p_sharp = &(isp->sharp_para.linkage_sharp_attr[ENVI_ISO_128]);
        p_fcs = &(isp->fcs_para.linkage_fcs[ENVI_ISO_128]);
        p_satu =&(isp->saturation_para.linkage_sat[ENVI_ISO_128]);
        p_wdr =&(isp->wdr_para.linkage_wdr[ENVI_ISO_128]);
        lsc_strength = isp->af_para.af_win4_bottom;//env8
        break;
    default:
        break;
    }
    
    if(MODE_MANUAL == isp->blc_para.blc_mode)
        p_blc = &(isp->blc_para.m_blc);

    if(MODE_MANUAL == isp->dpc_para.ddpc_mode)
        p_dpc = &(isp->dpc_para.manual_ddpc);
    
    if(MODE_MANUAL == isp->gb_para.gb_mode)
        p_gb = &(isp->gb_para.manual_gb);
    
    if(MODE_MANUAL == isp->nr1_para.nr1_mode)
        p_nr1 = &(isp->nr1_para.manual_nr1);
    
    if(MODE_MANUAL == isp->nr2_para.nr2_mode)
        p_nr2 = &(isp->nr2_para.manual_nr2);
    
    if(MODE_MANUAL == isp->_3d_nr_para._3d_nr_mode)
        p_3d = &(isp->_3d_nr_para.manual_3d_nr);
    
    if(MODE_MANUAL == isp->fcs_para.fcs_mode)
        p_fcs = &(isp->fcs_para.manual_fcs);
    
    if(MODE_MANUAL == isp->saturation_para.SE_mode)
        p_satu = &(isp->saturation_para.manual_sat);

    if(MODE_MANUAL == isp->wdr_para.wdr_mode)
        p_wdr = &(isp->wdr_para.manual_wdr);
    
    if(MODE_MANUAL == isp->sharp_para.ysharp_mode)
        p_sharp = &(isp->sharp_para.manual_sharp_attr);

	_set_blc_attr(p_blc);
	_set_dpc_attr(p_dpc);
	_set_gb_attr(p_gb);
	//_set_nr1_attr(p_nr1);
	//_set_nr2_attr(p_nr2);
	//_set_sharp_attr(p_sharp);
	isp->cur_env_nr1 = p_nr1;
	isp->cur_env_nr2 = p_nr2;
	isp->cur_env_sharp = p_sharp;
	isp->cur_3dnr_nr1 = 0;
	isp->cur_3dnr_nr2 = 0;
	isp->cur_3dnr_sharp = 0;
	_set_3d_nr_attr(p_3d);
	_set_fcs_attr(p_fcs);
	_set_saturation_attr(p_satu);
	//_set_wdr_attr(p_wdr);
	memcpy(&isp->wdr_info.last_wdr, &isp->wdr_info.current_wdr, sizeof(AK_ISP_WDR));
	memcpy(&isp->wdr_info.target_wdr, p_wdr, sizeof(AK_ISP_WDR));
	memcpy(&isp->wdr_info.current_wdr, p_wdr, sizeof(AK_ISP_WDR));
	if(isp->wdr_info.last_wdr.wdr_enable==0 || isp->wdr_info.target_wdr.wdr_enable==0)
	{
		_set_wdr_attr(p_wdr);
		isp->wdr_info.step = 0;
	}
	else
		isp->wdr_info.step = 16;

	{//adjust lsc by envi
		int i;
		int CC_R, CC_G, CC_B;
		AK_ISP_LSC_ATTR		*lsc_para;

		lsc_para = &lsc;

		if(lsc_para->enable)
		{
			lsc_strength = (100-lsc_strength)*128/100;	//translter #100# to #128#
			for (i = 1; i<10; i++)
			{
				CC_R = ((lsc_para->lsc_r_coef.coef_c[i]-256)*lsc_strength)/128 + 256;
				CC_G = ((lsc_para->lsc_gr_coef.coef_c[i]-256)*lsc_strength)/128 + 256;
				CC_B = ((lsc_para->lsc_b_coef.coef_c[i]-256)*lsc_strength)/128 + 256;
				if(lsc_para->range[i] > lsc_para->range[i-1])
				{
					lsc_para->lsc_r_coef.coef_b[i-1] = ((CC_R - lsc_para->lsc_r_coef.coef_c[i-1]) * 64 + (lsc_para->range[i] - lsc_para->range[i-1]) / 2) / (lsc_para->range[i] - lsc_para->range[i-1]);
					lsc_para->lsc_gr_coef.coef_b[i-1] = lsc_para->lsc_gb_coef.coef_b[i-1] = ((CC_G - lsc_para->lsc_gr_coef.coef_c[i-1]) * 64 + (lsc_para->range[i] - lsc_para->range[i-1]) / 2) / (lsc_para->range[i] - lsc_para->range[i-1]);
					lsc_para->lsc_b_coef.coef_b[i-1] = ((CC_B - lsc_para->lsc_b_coef.coef_c[i-1]) * 64 + (lsc_para->range[i] - lsc_para->range[i-1]) / 2) / (lsc_para->range[i] - lsc_para->range[i-1]);

					lsc_para->lsc_r_coef.coef_c[i] = lsc_para->lsc_r_coef.coef_c[i - 1] + (((lsc_para->range[i] - lsc_para->range[i - 1])*lsc_para->lsc_r_coef.coef_b[i - 1]) >> 6);
					lsc_para->lsc_gb_coef.coef_c[i] = lsc_para->lsc_gr_coef.coef_c[i] = lsc_para->lsc_gr_coef.coef_c[i - 1] + (((lsc_para->range[i] - lsc_para->range[i - 1])*lsc_para->lsc_gr_coef.coef_b[i - 1]) >> 6);
					lsc_para->lsc_b_coef.coef_c[i] = lsc_para->lsc_b_coef.coef_c[i - 1] + (((lsc_para->range[i] - lsc_para->range[i - 1])*lsc_para->lsc_b_coef.coef_b[i - 1]) >> 6);
				}
				else
				{
					lsc_para->lsc_r_coef.coef_c[i] = lsc_para->lsc_r_coef.coef_c[i - 1] ;
					lsc_para->lsc_gb_coef.coef_c[i] = lsc_para->lsc_gr_coef.coef_c[i] = lsc_para->lsc_gr_coef.coef_c[i - 1];
					lsc_para->lsc_b_coef.coef_c[i] = lsc_para->lsc_b_coef.coef_c[i - 1];
				}
			}
			lsc_para->lsc_r_coef.coef_b[9] = lsc_para->lsc_r_coef.coef_b[8];
			lsc_para->lsc_gr_coef.coef_b[9] = lsc_para->lsc_gb_coef.coef_b[9] = lsc_para->lsc_gb_coef.coef_b[8];
			lsc_para->lsc_b_coef.coef_b[9] = lsc_para->lsc_b_coef.coef_b[8];
		}
		_set_lsc_attr(lsc_para);
	}
	return 0;
}

int  _calc_aec_para(void)
{
	unsigned int calc_lumi_average =isp->ae_run_info_para.current_calc_avg_compensation_lumi; ///isp->ae_run_info_para.current_calc_avg_lumi;

	//int target_lumiance = isp->ae_para.target_lumiance;  //comment by xc
	unsigned int high_lock = 0;
	unsigned int low_lock =  0; 
	unsigned int high_hold = 0;   
	unsigned int low_hold  = 0; 
	int hold_range = HOLD_RANGE; 
	unsigned int gain_step;
	int exp_step = isp->ae_para.exp_step;
	//int current_exposure_time = 0; //comment by xc
	int tempvalue = 0;
	int tmp_exp_time = isp->ae_run_info_para.current_exp_time;
	int tmp_a_gain = isp->ae_run_info_para.current_a_gain;
	int tmp_d_gain = isp->ae_run_info_para.current_d_gain;
	int tmp_isp_d_gain = isp->ae_run_info_para.current_isp_d_gain;
	//int i, oe_count=0, pixel_total=0;
	int increase_flag = 0;
	
	int tmp_rang = 0;//isp->ae_para.envi_gain_range[9][0];
	if(tmp_rang>UNSTABLE_FRAMES_MAX)
	{
		tmp_rang = UNSTABLE_FRAMES_MAX;
	}

	if(isp->ae_para.envi_gain_range[9][0]!=0)
		hold_range = isp->ae_para.envi_gain_range[9][0];
	
	//what is the ae_drop_count for?
	/*if(isp->ae_drop_count >=0)
	{
		isp->ae_drop_count--;
		//printk("%d :exp_time=%d isp_d_gain=%d calc_com_average=%d buf id=%d\n",
    	//			isp->ae_drop_count+1, isp->ae_run_info_para.current_exp_time,    isp->ae_run_info_para.current_isp_d_gain,    calc_lumi_average,    isp->current_buffer);
    	//printk("AE_DPC:%d \r\n",isp->ae_drop_count);
		return 0;
	}*/
	
	if(isp->ae_para_update_flag )
	{
		int total_gain;
		unsigned int target_exp_time;
		
		isp->ae_run_info_para.current_a_gain = ONE_X_GAIN;
		isp->ae_run_info_para.current_d_gain = ONE_X_GAIN;
		isp->ae_run_info_para.current_isp_d_gain = ONE_X_GAIN;
		
		total_gain = (tmp_a_gain*tmp_d_gain)/ONE_X_GAIN*tmp_isp_d_gain/ONE_X_GAIN;
		target_exp_time = tmp_exp_time*total_gain;
		if(target_exp_time/ONE_X_GAIN>isp->ae_para.exp_time_max)
		{
			isp->ae_run_info_para.current_exp_time = isp->ae_para.exp_time_max/exp_step*exp_step;
			total_gain = target_exp_time/isp->ae_run_info_para.current_exp_time;
			if(total_gain>isp->ae_para.a_gain_max)
			{
				isp->ae_run_info_para.current_a_gain = isp->ae_para.a_gain_max;
				total_gain = total_gain*ONE_X_GAIN/isp->ae_run_info_para.current_a_gain;
				if(total_gain>isp->ae_para.d_gain_max)
				{
					isp->ae_run_info_para.current_d_gain = isp->ae_para.d_gain_max;
					total_gain = total_gain*ONE_X_GAIN/isp->ae_run_info_para.current_d_gain;
					isp->ae_run_info_para.current_isp_d_gain = total_gain;
					if(isp->ae_run_info_para.current_isp_d_gain> isp->ae_para.isp_d_gain_max)
						isp->ae_run_info_para.current_isp_d_gain= isp->ae_para.isp_d_gain_max;
				}
				else
					isp->ae_run_info_para.current_d_gain = total_gain;
			}
			else
				isp->ae_run_info_para.current_a_gain = total_gain;
		}
		else
		{
			if(target_exp_time/ONE_X_GAIN>exp_step)
			{
				isp->ae_run_info_para.current_exp_time = target_exp_time/ONE_X_GAIN/exp_step*exp_step;
				isp->ae_run_info_para.current_isp_d_gain = target_exp_time/isp->ae_run_info_para.current_exp_time;
			}
			else
			{
				isp->ae_run_info_para.current_exp_time = target_exp_time/ONE_X_GAIN;
			}
			if(isp->ae_run_info_para.current_exp_time<isp->ae_para.exp_time_min)
				isp->ae_run_info_para.current_exp_time = isp->ae_para.exp_time_min;
			if(isp->ae_run_info_para.current_exp_time>isp->ae_para.exp_time_max)
				isp->ae_run_info_para.current_exp_time = isp->ae_para.exp_time_max;
		}
		
		isp->ae_algo.exp_time_need_updata  = 1;
		isp->ae_algo.a_gain_need_updata  = 1;
		isp->ae_algo.d_gain_need_updata  = 1;
		isp->ae_algo.isp_d_gain_need_updata  = 1;
		
		isp->ae_para_update_flag = 0;
		//printk("update_AE:EXP %d Again %d Dgain %d ispGain %d \r\n",isp->ae_run_info_para.current_exp_time,isp->ae_run_info_para.current_a_gain,
		//	isp->ae_run_info_para.current_d_gain,isp->ae_run_info_para.current_isp_d_gain);
		isp->ae_drop_count=DROP_FRAMES_MAX;
		return 0;
	}
	isp->ae_last_lumi = isp->ae_run_info_para.current_calc_avg_lumi;
	
    //printk("tmp_isp_d_gain tmp_isp_d_gain=%d\n",tmp_isp_d_gain);
	tempvalue = isp->ae_para.target_lumiance + isp->ae_para.exp_stable_range;
	high_lock = (tempvalue > 256) ? 256 : tempvalue;

	tempvalue = isp->ae_para.target_lumiance -isp->ae_para.exp_stable_range;
	low_lock = (tempvalue < 0) ? 0 : tempvalue;

	//tempvalue = isp->ae_para.target_lumiance + isp->ae_para.exp_stable_range+6;
	tempvalue = isp->ae_para.target_lumiance + isp->ae_para.exp_stable_range+hold_range;
	high_hold = (tempvalue > 256) ? 256 : tempvalue;

	//tempvalue = isp->aec_param.target_lumiance - isp->aec_param.stable_range - 6;
	tempvalue = isp->ae_para.target_lumiance -isp->ae_para.exp_stable_range-hold_range;
	low_hold = (tempvalue < 0) ? 0 : tempvalue;
	//printk("low_lock =%d\n",low_lock);
	//printk("high_lock = %d\n",high_lock);
	//printk("avg_compensation_lumi=%d\n",isp->ae_run_info_para.current_calc_avg_compensation_lumi);
	#if 0
    printk("exp_time=%d isp_d_gain=%d a_gain =%d calc_com_average=%d calc_avg=%d\n",
    isp->ae_run_info_para.current_exp_time,
	isp->ae_run_info_para.current_isp_d_gain,
	isp->ae_run_info_para.current_a_gain,
	calc_lumi_average,
	isp->ae_run_info_para.current_calc_avg_lumi);		
	#endif
	
	/*if(isp->ae_para.OE_suppress_en)
	{
		for(i=0; i<isp->ae_para.OE_detect_scope; i++)
			oe_count+=isp->raw_hist_stat_para.raw_g_hist[255-i];

		for(i=0; i<256; i++)
			pixel_total+=isp->raw_hist_stat_para.raw_g_hist[i];
	}
	if(isp->ae_para.OE_suppress_en&&
		oe_count>isp->ae_para.OE_rate_max*pixel_total/256)
	{
		increase_flag = 0;
		gain_step = GAIN_STEP;
	}
	//else if(isp->ae_para.OE_suppress_en&&oe_count>isp->ae_para.OE_rate_min*pixel_total/256)
	//{
	//	return 0;	//do nothing
	//}
	else*/
	{
		//controll with target lumi
		if((calc_lumi_average<=high_lock)&&(calc_lumi_average>=low_lock))
		{
			isp->ae_algo.aec_locked  = 1;                              //no work to do
			unstable_count = tmp_rang;
			return 0;
		}

	    	if((isp->ae_algo.aec_locked == 1) &&
			(((calc_lumi_average<high_hold)&&(calc_lumi_average>high_lock)) ||
			((calc_lumi_average < low_lock)&&(calc_lumi_average>low_hold) )))
		{
			unstable_count = tmp_rang;
			return 0;		//stay in stable area, no work to do
		}

		if(isp->ae_algo.aec_locked == 1)
		{
			if(unstable_count>0)
			{
				unstable_count--;
				return 0;
			}
		}
		
		//start to adjust exp&gain
		isp->ae_algo.aec_locked  = 0;
		
		if(high_lock+((high_lock*CONTROL_ZONE1)>>8)>calc_lumi_average
			&& calc_lumi_average+((calc_lumi_average*CONTROL_ZONE1)>>8)>low_lock)
		{
			gain_step = CONTROL_STEP1;
		}
		else if(high_lock+((high_lock*CONTROL_ZONE2)>>8)>calc_lumi_average
			&& calc_lumi_average+((calc_lumi_average*CONTROL_ZONE2)>>8)>low_lock)
		{
			gain_step = CONTROL_STEP2;
		}
		else if(high_lock+((high_lock*CONTROL_ZONE3)>>8)>calc_lumi_average
			&& calc_lumi_average+((calc_lumi_average*CONTROL_ZONE3)>>8)>low_lock)
		{
			gain_step = CONTROL_STEP3;
		}
		else
		{
			if(calc_lumi_average>=high_lock)
			{
				gain_step = ((calc_lumi_average-high_lock)*ONE_X_GAIN/calc_lumi_average)>>1;
			}
			else if(calc_lumi_average<=low_lock)
			{
				gain_step = ((low_lock - calc_lumi_average)*ONE_X_GAIN/low_lock)>>1;
			}
		}

		{
			tmp_rang = isp->ae_para.envi_gain_range[9][1];
			if(tmp_rang>0)
			{
				//16:1x
				if(tmp_rang>16)
					tmp_rang=16;
				gain_step = gain_step*tmp_rang/16;
			}
			if(gain_step>CONTROL_STEP_MAX)
				gain_step=CONTROL_STEP_MAX;
		}
		/*else if(calc_lumi_average<low_lock/2 || calc_lumi_average>high_lock*2)
		{
			gain_step = ONE_X_GAIN/2;		//very fast
		}
		else if(((calc_lumi_average<high_hold)&&(calc_lumi_average>high_lock)) ||
			((calc_lumi_average < low_lock)&&(calc_lumi_average>low_hold) ))
		{
			gain_step = GAIN_STEP;		//normal
		}
		else
		{
			gain_step = GAIN_STEP*2;		//fast
		}*/
		
		if(calc_lumi_average > high_lock)
			increase_flag = 0;
		//else if(isp->ae_para.OE_suppress_en&&oe_count>isp->ae_para.OE_rate_min*pixel_total/256)
		//	return 0;
		else
			increase_flag = 1;
	}

	//printk("aec_locked = %d\n",isp->ae_algo.aec_locked);
	//printk("aec_status =%d\n",isp->ae_algo.aec_status);
	//control AGC/AEC speed, by increase/decrease step
	//printk("exp_time=%d isp_d_gain=%d calc_com_average=%d step=%d\n",
    	//			isp->ae_run_info_para.current_exp_time,    isp->ae_run_info_para.current_isp_d_gain,    calc_lumi_average,    gain_step);
   		
	if(increase_flag ==0) 
	{    
        //printk("light envi \n");
		//reduce isp d gain
		if(isp->ae_run_info_para.current_isp_d_gain > isp->ae_para.isp_d_gain_min)
		{
			isp->ae_run_info_para.current_isp_d_gain = isp->ae_run_info_para.current_isp_d_gain*(ONE_X_GAIN-gain_step)/ONE_X_GAIN;
			if(isp->ae_run_info_para.current_isp_d_gain< isp->ae_para.isp_d_gain_min)
				isp->ae_run_info_para.current_isp_d_gain = isp->ae_para.isp_d_gain_min;
			if(tmp_isp_d_gain != isp->ae_run_info_para.current_isp_d_gain)
				isp->ae_algo.isp_d_gain_need_updata =1;
			return 0;
    	}
    	
		if(isp->ae_run_info_para.current_d_gain>isp->ae_para.d_gain_min)
		{
			isp->ae_run_info_para.current_d_gain = isp->ae_run_info_para.current_d_gain*(ONE_X_GAIN-gain_step)/ONE_X_GAIN;
			if(isp->ae_run_info_para.current_d_gain < isp->ae_para.d_gain_min)
				isp->ae_run_info_para.current_d_gain = isp->ae_para.d_gain_min;
			if(tmp_d_gain != isp->ae_run_info_para.current_d_gain)
				isp->ae_algo.d_gain_need_updata = 1;
			return 0;   
		}
		
		if(isp->ae_run_info_para.current_a_gain>isp->ae_para.a_gain_min)
		{
			isp->ae_run_info_para.current_a_gain = isp->ae_run_info_para.current_a_gain*(ONE_X_GAIN-gain_step)/ONE_X_GAIN;
			if(isp->ae_run_info_para.current_a_gain<isp->ae_para.a_gain_min)
				isp->ae_run_info_para.current_a_gain = isp->ae_para.a_gain_min;
			if(tmp_a_gain != isp->ae_run_info_para.current_a_gain)
				isp->ae_algo.a_gain_need_updata  = 1;
			return 0;
		}
            
		if(isp->ae_run_info_para.current_exp_time>isp->ae_para.exp_time_min)
		{
			unsigned int target_exp_time;
			unsigned int min_step;

			target_exp_time = isp->ae_run_info_para.current_exp_time*isp->ae_run_info_para.current_isp_d_gain;
			if(target_exp_time>0xFFFFF)
				target_exp_time = target_exp_time/ONE_X_GAIN*(ONE_X_GAIN-gain_step);
			else
				target_exp_time = target_exp_time*(ONE_X_GAIN-gain_step)/ONE_X_GAIN;
			if(target_exp_time<isp->ae_para.exp_time_min*isp->ae_para.isp_d_gain_min)
				target_exp_time = isp->ae_para.exp_time_min*isp->ae_para.isp_d_gain_min;
			isp->ae_run_info_para.current_exp_time = target_exp_time/isp->ae_para.isp_d_gain_min;
			isp->ae_run_info_para.current_isp_d_gain = isp->ae_para.isp_d_gain_min;
			if(isp->ae_run_info_para.current_exp_time>exp_step)
				isp->ae_run_info_para.current_exp_time = isp->ae_run_info_para.current_exp_time/exp_step*exp_step;

			if(isp->ae_run_info_para.current_exp_time<isp->ae_para.exp_time_min)
				isp->ae_run_info_para.current_exp_time = isp->ae_para.exp_time_min;
			if(isp->ae_run_info_para.current_exp_time>isp->ae_para.exp_time_max)
				isp->ae_run_info_para.current_exp_time = isp->ae_para.exp_time_max;

			min_step = isp->ae_run_info_para.current_exp_time*isp->ae_para.isp_d_gain_min;
			if(min_step>0xFFFFF)
				min_step = min_step/ONE_X_GAIN*gain_step;
			else
				min_step = min_step*gain_step/ONE_X_GAIN;
			if(target_exp_time-isp->ae_run_info_para.current_exp_time*isp->ae_para.isp_d_gain_min>=min_step)
			{
				//need to sooth with isp d gain
				isp->ae_run_info_para.current_isp_d_gain =  target_exp_time/isp->ae_run_info_para.current_exp_time;
				if(isp->ae_run_info_para.current_isp_d_gain< isp->ae_para.isp_d_gain_min)
					isp->ae_run_info_para.current_isp_d_gain = isp->ae_para.isp_d_gain_min;
			}

			if(tmp_isp_d_gain != isp->ae_run_info_para.current_isp_d_gain)
				isp->ae_algo.isp_d_gain_need_updata =1;
			
			if(tmp_exp_time != isp->ae_run_info_para.current_exp_time)
				isp->ae_algo.exp_time_need_updata  = 1;

			return 0;
		}
	}
	else if(increase_flag ==1)
	{  
		//printk("dark envi \n");
		//printk("exp_time =%d\n",isp->ae_run_info_para.current_exp_time);    
		if(isp->ae_run_info_para.current_exp_time<isp->ae_para.exp_time_max/exp_step*exp_step)
		{
			unsigned int target_exp_time;
			unsigned int min_step;

			target_exp_time = isp->ae_run_info_para.current_exp_time*isp->ae_run_info_para.current_isp_d_gain;
			if(target_exp_time>0xFFFFF)
				target_exp_time = target_exp_time/(ONE_X_GAIN-gain_step)*ONE_X_GAIN;
			else
				target_exp_time = target_exp_time*ONE_X_GAIN/(ONE_X_GAIN-gain_step);
			if(target_exp_time>isp->ae_para.exp_time_max*isp->ae_para.isp_d_gain_min)
				target_exp_time=isp->ae_para.exp_time_max*isp->ae_para.isp_d_gain_min;
			isp->ae_run_info_para.current_exp_time = target_exp_time/isp->ae_para.isp_d_gain_min;
			isp->ae_run_info_para.current_isp_d_gain = isp->ae_para.isp_d_gain_min;
			if(isp->ae_run_info_para.current_exp_time>exp_step)
				isp->ae_run_info_para.current_exp_time = isp->ae_run_info_para.current_exp_time/exp_step*exp_step;

			if(isp->ae_run_info_para.current_exp_time<isp->ae_para.exp_time_min)
				isp->ae_run_info_para.current_exp_time = isp->ae_para.exp_time_min;
			if(isp->ae_run_info_para.current_exp_time>isp->ae_para.exp_time_max)
				isp->ae_run_info_para.current_exp_time = isp->ae_para.exp_time_max;

			min_step = isp->ae_run_info_para.current_exp_time*isp->ae_para.isp_d_gain_min;
			if(min_step>0xFFFFF)
				min_step = min_step/ONE_X_GAIN*gain_step;
			else
				min_step = min_step*gain_step/ONE_X_GAIN;
			if(target_exp_time-isp->ae_run_info_para.current_exp_time*isp->ae_para.isp_d_gain_min>=min_step)
			{
				//need to sooth with isp d gain
				isp->ae_run_info_para.current_isp_d_gain =  target_exp_time/isp->ae_run_info_para.current_exp_time;
				if(isp->ae_run_info_para.current_isp_d_gain< isp->ae_para.isp_d_gain_min)
					isp->ae_run_info_para.current_isp_d_gain = isp->ae_para.isp_d_gain_min;
			}

			if(tmp_isp_d_gain != isp->ae_run_info_para.current_isp_d_gain)
				isp->ae_algo.isp_d_gain_need_updata =1;
			
			if(tmp_exp_time != isp->ae_run_info_para.current_exp_time)
				isp->ae_algo.exp_time_need_updata  = 1;
			
			return 0;
		}
           
		if(isp->ae_run_info_para.current_a_gain< isp->ae_para.a_gain_max)
		{
			isp->ae_run_info_para.current_a_gain = isp->ae_run_info_para.current_a_gain*ONE_X_GAIN/(ONE_X_GAIN-gain_step);
			if(isp->ae_run_info_para.current_a_gain>isp->ae_para.a_gain_max)
				isp->ae_run_info_para.current_a_gain = isp->ae_para.a_gain_max;
			if(tmp_a_gain != isp->ae_run_info_para.current_a_gain)
				isp->ae_algo.a_gain_need_updata  = 1;

			return 0;
		}
		if(isp->ae_run_info_para.current_d_gain< isp->ae_para.d_gain_max)
		{
			isp->ae_run_info_para.current_d_gain = isp->ae_run_info_para.current_d_gain*ONE_X_GAIN/(ONE_X_GAIN-gain_step);
			if(isp->ae_run_info_para.current_d_gain>isp->ae_para.d_gain_max)
				isp->ae_run_info_para.current_d_gain = isp->ae_para.d_gain_max;
			if(tmp_d_gain != isp->ae_run_info_para.current_d_gain)
				isp->ae_algo.d_gain_need_updata = 1;

			return 0;
		}
		if(isp->ae_run_info_para.current_isp_d_gain< isp->ae_para.isp_d_gain_max)
		{
			isp->ae_run_info_para.current_isp_d_gain = isp->ae_run_info_para.current_isp_d_gain*ONE_X_GAIN/(ONE_X_GAIN-gain_step);
			if(isp->ae_run_info_para.current_isp_d_gain>isp->ae_para.isp_d_gain_max)
				isp->ae_run_info_para.current_isp_d_gain = isp->ae_para.isp_d_gain_max;
			if(tmp_isp_d_gain != isp->ae_run_info_para.current_isp_d_gain)
				isp->ae_algo.isp_d_gain_need_updata =1;

			return 0;
		}
         
        // printk("int the dark envi, the exposure_time%d\n",isp->aec_param.current_exposure_time);
        // printk("in  the dark envi,the a_gain =%d\n",isp->aec_param.current_a_gain);
	}
 
	return 0;       
}

/**
 * @brief: get enviment flag
 * @author: xiepenghe
 * @date: 2016-5-06
 *
 */
int _get_iso(void)
{
	unsigned int exp_time = isp->ae_run_info_para.current_exp_time;
	unsigned int a_gain = isp->ae_run_info_para.current_a_gain;
	unsigned int d_gain = isp->ae_run_info_para.current_d_gain;
	unsigned int isp_d_gain = isp->ae_run_info_para.current_isp_d_gain;
	unsigned int exp_time_max = isp->ae_para.exp_time_max;
	unsigned int iso=0;
	unsigned int all_gain;
	all_gain=(((a_gain*d_gain)>>GAIN_SHIFT)*isp_d_gain)>>GAIN_SHIFT;

	if(isp->exp_type_para.exp_type != 0)
	{
		exp_time_max = exp_time_max/isp->ae_para.exp_step*isp->ae_para.exp_step;
		exp_time = (exp_time*all_gain)>>GAIN_SHIFT;
		if(exp_time<exp_time_max)
			all_gain = 0;
	}

	if(isp->ae_para.envi_gain_range[0][0] !=0&&exp_time<isp->ae_para.envi_gain_range[0][0])
		iso = 0;
	else if(isp->ae_para.envi_gain_range[0][0] ==0&& all_gain<isp->ae_para.envi_gain_range[0][1]*ONE_X_GAIN)
		iso = 0;
	else if(all_gain<isp->ae_para.envi_gain_range[1][1]*ONE_X_GAIN)
		iso = 1;
	else if((isp->ae_para.envi_gain_range[2][0]*ONE_X_GAIN<=all_gain)&&(all_gain<isp->ae_para.envi_gain_range[2][1]*ONE_X_GAIN))
		iso = 2;
	else if((isp->ae_para.envi_gain_range[3][0]*ONE_X_GAIN<=all_gain)&&(all_gain<isp->ae_para.envi_gain_range[3][1]*ONE_X_GAIN))
		iso = 3;
	else if((isp->ae_para.envi_gain_range[4][0]*ONE_X_GAIN<=all_gain)&&(all_gain<isp->ae_para.envi_gain_range[4][1]*ONE_X_GAIN))
		iso = 4;
	else if((isp->ae_para.envi_gain_range[5][0]*ONE_X_GAIN<=all_gain)&&(all_gain<isp->ae_para.envi_gain_range[5][1]*ONE_X_GAIN))
		iso=5;
	else if((isp->ae_para.envi_gain_range[6][0]*ONE_X_GAIN<=all_gain)&&(all_gain<isp->ae_para.envi_gain_range[6][1]*ONE_X_GAIN))
		iso = 6;
	else if((isp->ae_para.envi_gain_range[7][0]*ONE_X_GAIN<=all_gain)&&(all_gain<isp->ae_para.envi_gain_range[7][1]*ONE_X_GAIN))
		iso= 7; 
	else 
		iso = 8;
	return iso;
}

int _set_blc_attr(AK_ISP_BLC *blc)
{
    unsigned int cmd;
    int bl_r_a, bl_gr_a, bl_gb_a,bl_b_a;
    int bl_r_offset, bl_gr_offset, bl_gb_offset,bl_b_offset;
    
    isp_dbg("%s enter.\n", __func__);
    bl_r_a = (isp->ae_isp_d_gain*blc->bl_r_a)>>GAIN_SHIFT;
    if(bl_r_a>1023)
    	bl_r_a=1023;

    bl_gr_a = (isp->ae_isp_d_gain*blc->bl_gr_a)>>GAIN_SHIFT;
    if(bl_gr_a>1023)
    	bl_gr_a=1023;

    bl_gb_a = (isp->ae_isp_d_gain*blc->bl_gb_a)>>GAIN_SHIFT;
    if(bl_gb_a>1023)
    	bl_gb_a=1023;

    bl_b_a = (isp->ae_isp_d_gain*blc->bl_b_a)>>GAIN_SHIFT;
    if(bl_b_a>1023)
    	bl_b_a=1023;

    bl_r_offset = (isp->ae_isp_d_gain*blc->bl_r_offset)>>GAIN_SHIFT;
    bl_gr_offset = (isp->ae_isp_d_gain*blc->bl_gr_offset)>>GAIN_SHIFT;
    bl_gb_offset = (isp->ae_isp_d_gain*blc->bl_gb_offset)>>GAIN_SHIFT;
    bl_b_offset = (isp->ae_isp_d_gain*blc->bl_b_offset)>>GAIN_SHIFT;
    
    //BUG_ON(blc == NULL);
    //raw_reg_1
    cmd = 0;
    cmd = (LOW_BITS(bl_r_a, 10) << BL_A_R)
    	| (LOW_BITS(bl_gr_a, 10) << BL_A_GR)
    	| (LOW_BITS(bl_gb_a, 10) << BL_A_GB)
    	| (LOW_BITS(bl_b_a, 2) << BL_A_B_2);
    isp->reg_blkaddr2[0]= cmd;

    //raw_reg_2
    cmd = 0;
    cmd = (LOW_BITS((bl_b_a >> 2), 8) << BL_A_B_8)
    	| (LOW_BITS(bl_r_offset, 12) << BL_B_R)
    	| (LOW_BITS(bl_gr_offset, 12) << BL_B_GR);
    isp->reg_blkaddr2[1] = cmd;

    //raw_reg_3
    cmd = 0;
    cmd = (LOW_BITS(bl_gb_offset, 12) << BL_B_GB)
    	| (LOW_BITS(bl_b_offset, 12) << BL_B_B);
    isp->reg_blkaddr2[2] = cmd;
    
    //enable
	ak_isp_vo_update_setting();
	
    return 0;
}

static int _isp_contrast_work(enum  envi_flag envi_flag)
{
	int dark_pixel_th = 0;
	int i;
	int dark_pixel;
	int area;
	AK_ISP_CONTRAST contrast;
	AK_ISP_AUTO_CONTRAST *p_ac;

	if(isp->contrast_para.cc_mode==MODE_MANUAL)
	{
		contrast.y_contrast = isp->contrast_para.manual_contrast.y_contrast;
		contrast.y_shift= isp->contrast_para.manual_contrast.y_shift;
		//contrast.y_contrast = 256*64/(256-(contrast.y_shift>>2));			
	}
	else
	{
		p_ac = &isp->contrast_para.linkage_contrast[envi_flag];
		contrast.y_shift = isp->contrast_setting.y_shift;
		dark_pixel = 0;
		for(i=0; i<256; i++)
			dark_pixel_th += isp->yuv_hist_stat_para.y_hist[i];
		dark_pixel_th = dark_pixel_th*p_ac->dark_pixel_rate/256;
		area = p_ac->dark_pixel_area;
		if(area<1)
			area = 1;
		for(i=0; i<area; i++)
		{
			dark_pixel += isp->yuv_hist_stat_para.y_hist[i];
		}
		if(dark_pixel<dark_pixel_th)
			contrast.y_shift+=4;
		else if(dark_pixel>dark_pixel_th*75/64 && dark_pixel-isp->yuv_hist_stat_para.y_hist[i-1]>=dark_pixel_th)
			contrast.y_shift-=4;

		if(contrast.y_shift>p_ac->shift_max*4)
			contrast.y_shift=p_ac->shift_max*4;
		if(contrast.y_shift<0)
			contrast.y_shift=0;
		contrast.y_contrast = 256*64/(256-(contrast.y_shift>>2));
	}

	if(contrast.y_shift!=isp->contrast_setting.y_shift ||
	   contrast.y_contrast!=isp->contrast_setting.y_contrast)
		_set_contrast_attr(&contrast);

	return 0;
}

int _isp_3dnr_work(enum  envi_flag envi_flag)
{
	AK_ISP_3D_NR     *_3d_nr_move;
	AK_ISP_3D_NR  *_3d_nr_still;
	AK_ISP_NR1  nr1;
	AK_ISP_NR2  nr2;
	AK_ISP_NR1  *nr1_still;
	AK_ISP_NR2  *nr2_still;
	//AK_ISP_SHARP *sharp_move;
	AK_ISP_SHARP sharp;
	int MD_level;
	AK_ISP_3D_NR_STAT_INFO	    *tnr_stat;
	int height_block_num;

	height_block_num = _md_rows();
		
	
	nr1_still = &isp->nr1_para.linkage_nr1[8];
	nr2_still = &isp->nr2_para.linkage_nr2[8];
	//sharp_move = &isp->sharp_para.linkage_sharp_attr[8];
	_3d_nr_still = &isp->_3d_nr_para.linkage_3d_nr[8];
	if(isp->_3d_nr_para._3d_nr_mode==MODE_MANUAL)
		_3d_nr_move = &isp->_3d_nr_para.manual_3d_nr;
	else
		_3d_nr_move = &isp->_3d_nr_para.linkage_3d_nr[envi_flag];
	
	tnr_stat = &isp->_3d_nr_stat_para;
	MD_level = tnr_stat->MD_level;
	
	if(_3d_nr_move->md_th>0)
	{
		int md_cnt_th1,md_cnt_th2;
		int i, j;
		int md_cnt = 0;

		for(i=0; i<height_block_num; i++) //lz0499  9_6
			for(j=0; j<32; j++)
			{
				if(tnr_stat->MD_stat[i][j]>_3d_nr_move->t_y_mf_th2)
					md_cnt++;
			}
		/*
		 * md_th in the tunings counts moving cells out of 16 rows of
		 * 32. Scale a 24-row count to that, so the same share of the
		 * frame in motion gives the same MD_level in either mode.
		 */
		md_cnt = md_cnt * 16 / height_block_num;
		md_cnt_th1 = _3d_nr_move->md_th;
		md_cnt_th2 = (_3d_nr_move->md_th+_3d_nr_still->md_th);
		if(md_cnt>md_cnt_th1)
		{
			MD_level = (md_cnt-md_cnt_th1)*16/(md_cnt_th2-md_cnt_th1);
			if(MD_level >16)
				MD_level = 16;
		}
		else
		{
			MD_level = 0;
			//isp->_3d_nr_md_flag = 0;
		}

		if(tnr_stat->MD_level!=MD_level
			||isp->cur_3dnr_nr1!=isp->cur_env_nr1
			||isp->cur_3dnr_nr2!=isp->cur_env_nr2
			||isp->cur_3dnr_sharp!=isp->cur_env_sharp)
		{
			isp->cur_3dnr_nr1 = isp->cur_env_nr1;
			isp->cur_3dnr_nr2 = isp->cur_env_nr2;
			isp->cur_3dnr_sharp = isp->cur_env_sharp;
			if(tnr_stat->MD_level!=MD_level)
			{
				if(tnr_stat->MD_level >MD_level)
					//tnr_stat->MD_level --;
					tnr_stat->MD_level = (tnr_stat->MD_level*10+MD_level*6)/16;
				else if(tnr_stat->MD_level <MD_level)
					//tnr_stat->MD_level ++;
					tnr_stat->MD_level = (tnr_stat->MD_level*10+MD_level*6+15)/16;
				_set_3d_nr_attr(_3d_nr_move);
			}

			memcpy(&nr1, isp->cur_3dnr_nr1,sizeof(AK_ISP_NR1));
			memcpy(&nr2, isp->cur_3dnr_nr2,sizeof(AK_ISP_NR2));
			memcpy(&sharp, isp->cur_3dnr_sharp,sizeof(AK_ISP_SHARP));
			if(nr1.nr1_enable)
			{
				for(i=0; i<17; i++)
				{
					if(nr1.nr1_weight_btbl[i]>nr1_still->nr1_weight_btbl[i])
						nr1.nr1_weight_btbl[i] = (nr1.nr1_weight_btbl[i]*tnr_stat->MD_level +nr1_still->nr1_weight_btbl[i]*(16-tnr_stat->MD_level ))/16;
					if(nr1.nr1_weight_gtbl[i]>nr1_still->nr1_weight_gtbl[i])
						nr1.nr1_weight_gtbl[i] = (nr1.nr1_weight_gtbl[i]*tnr_stat->MD_level +nr1_still->nr1_weight_gtbl[i]*(16-tnr_stat->MD_level ))/16;
					if(nr1.nr1_weight_rtbl[i]>nr1_still->nr1_weight_rtbl[i])
						nr1.nr1_weight_rtbl[i] = (nr1.nr1_weight_rtbl[i]*tnr_stat->MD_level +nr1_still->nr1_weight_rtbl[i]*(16-tnr_stat->MD_level ))/16;
				}
			}
			if(nr2.nr2_enable)
			{
				for(i=0; i<17; i++)
				{
					if(nr2.nr2_weight_tbl[i]>nr2_still->nr2_weight_tbl[i])
						nr2.nr2_weight_tbl[i] = (nr2.nr2_weight_tbl[i]*tnr_stat->MD_level +nr2_still->nr2_weight_tbl[i]*(16-tnr_stat->MD_level ))/16;
				}
			}

			if(sharp.ysharp_enable)
			{
				int weak_gain;
				weak_gain = sharp.sharp_skin_gain_weaken*tnr_stat->MD_level/16;
				for(i=1; i<sharp.sharp_skin_gain_th; i++)
				{
					sharp.MF_HPF_LUT[128+i] = (sharp.MF_HPF_LUT[128+i] *(256-weak_gain ))>>8;
					sharp.HF_HPF_LUT[128+i] = (sharp.HF_HPF_LUT[128+i] *(256-weak_gain ))>>8;
					sharp.MF_HPF_LUT[128-i] = (sharp.MF_HPF_LUT[128-i] *(256-weak_gain ))>>8;
					sharp.HF_HPF_LUT[128-i] = (sharp.HF_HPF_LUT[128-i] *(256-weak_gain ))>>8;
				}
			}
			_set_nr1_attr(&nr1);
			_set_nr2_attr(&nr2);
			_set_sharp_attr(&sharp);
				
		}
	}
	else
	{
		if(tnr_stat->MD_level!= 16)
		{
			tnr_stat->MD_level = 16;
			_set_3d_nr_attr(_3d_nr_move);
		}
		
		if(isp->cur_3dnr_nr1!=isp->cur_env_nr1)
		{
			isp->cur_3dnr_nr1 = isp->cur_env_nr1;
			_set_nr1_attr(isp->cur_3dnr_nr1);
		}
		if(isp->cur_3dnr_nr2!=isp->cur_env_nr2)
		{
			isp->cur_3dnr_nr2 = isp->cur_env_nr2;
			_set_nr2_attr(isp->cur_3dnr_nr2);
		}
		if(isp->cur_3dnr_sharp!=isp->cur_env_sharp)
		{
			isp->cur_3dnr_sharp = isp->cur_env_sharp;
			_set_sharp_attr(isp->cur_3dnr_sharp);
		}
	}
		
	return 0;
}

int _isp_set_yuveffect(AK_ISP_EFFECT_ATTR *p_effect_attr)
{
    unsigned long cmd = 0;

    //yuv_reg_41
    cmd = 0;
	cmd = isp->reg_blkaddr3[40];
	CLEAR_BITS(cmd, 0, 25);
    cmd |= ((LOW_BITS(p_effect_attr->y_a, 8) << ENHANCE_Y_A) 
    	|  (LOW_BITS(p_effect_attr->y_b, 8) << ENHANCE_Y_B) 
    	|  (LOW_BITS(p_effect_attr->uv_a, 9) << ENHANCE_UV_A));
    isp->reg_blkaddr3[40] = cmd;

	//yuv_reg_42
    cmd = 0;
	cmd = isp->reg_blkaddr3[41];
	CLEAR_BITS(cmd, 0, 9);
	cmd |= (LOW_BITS(p_effect_attr->uv_b, 9) << ENHANCE_UV_B);
	isp->reg_blkaddr3[41] = cmd;

    //dark margen enable
    cmd = 0;                                            //reg5
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, DARK_MARGIN_EN, 1);
    cmd |= (LOW_BITS(p_effect_attr->dark_margin_en, 1) << DARK_MARGIN_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

    ak_isp_vo_update_setting();

    return 0;
}

static int _calc_wdr_step(void)
{
	int i;
	isp->wdr_info.step = isp->wdr_info.step-1;
	if(isp->wdr_info.last_wdr.wdr_enable==0||isp->wdr_info.target_wdr.wdr_enable==0)
		return 0;

	for(i = 0; i<65; i++)
	{
		isp->wdr_info.current_wdr.area_tb1[i] = (isp->wdr_info.last_wdr.area_tb1[i]*isp->wdr_info.step+isp->wdr_info.target_wdr.area_tb1[i]*(16-isp->wdr_info.step))/16;
		isp->wdr_info.current_wdr.area_tb2[i] = (isp->wdr_info.last_wdr.area_tb2[i]*isp->wdr_info.step+isp->wdr_info.target_wdr.area_tb2[i]*(16-isp->wdr_info.step))/16;
		isp->wdr_info.current_wdr.area_tb3[i] = (isp->wdr_info.last_wdr.area_tb3[i]*isp->wdr_info.step+isp->wdr_info.target_wdr.area_tb3[i]*(16-isp->wdr_info.step))/16;
		isp->wdr_info.current_wdr.area_tb4[i] = (isp->wdr_info.last_wdr.area_tb4[i]*isp->wdr_info.step+isp->wdr_info.target_wdr.area_tb4[i]*(16-isp->wdr_info.step))/16;
		isp->wdr_info.current_wdr.area_tb5[i] = (isp->wdr_info.last_wdr.area_tb5[i]*isp->wdr_info.step+isp->wdr_info.target_wdr.area_tb5[i]*(16-isp->wdr_info.step))/16;
		isp->wdr_info.current_wdr.area_tb6[i] = (isp->wdr_info.last_wdr.area_tb6[i]*isp->wdr_info.step+isp->wdr_info.target_wdr.area_tb6[i]*(16-isp->wdr_info.step))/16;
	}

	_set_wdr_attr(&isp->wdr_info.current_wdr);
	return 1;
}


/**
 * @brief: auto exp work queue
 * @author: xiepenghe
 * @date: 2016-5-06
 
 */
#define AE_WORK_PERIOD		2
int ak_isp_ae_work(void)
{
	int curr_envi_flag = 0;

	if(isp==0)
		return 0;

	if(isp->ae_exp_time_effect_frame>0)
	{
		isp->ae_exp_time_effect_frame--;
		if(isp->ae_exp_time_effect_frame <= 1&&isp->ae_algo.isp_d_gain_need_updata==AK_TRUE)
		{
			_isp_update_isp_d_gain(isp->ae_run_info_para.current_isp_d_gain);
			isp->ae_exp_time_effect_frame = 1;
			isp->ae_algo.isp_d_gain_need_updata=AK_FALSE;
		}

		if(isp->ae_exp_time_effect_frame>0&&isp->ae_drop_count <0)
		{
			isp->ae_drop_count=0;//dont calc ae this frame
		}
	}

    
	_get_raw_hist();
	_get_rgb_hist();

	//  isp->ae_run_info_para.current_calc_avg_compensation_lumi = _isp_compensation_with_ae();
	// isp->ae_run_info_para.current_calc_avg_lumi;//_isp_compensation_with_ae();
	isp->ae_run_info_para.current_calc_avg_compensation_lumi = _isp_compensation_all_weight_with_ae();    
	//printk("isp->ae_run_info_para.current_calc_avg_compensation_lumi=%d\n",
	//isp->ae_run_info_para.current_calc_avg_compensation_lumi);
  	//_cmos_calc_step();
	if(isp->ae_drop_count >=0)
	{
		isp->ae_drop_count--;		
		return 0;
	}

    	_calc_aec_para();
    	
	//_isp_change_ae_target();
     //printk("isp->ae_algo.exp_time_need_updata=%d\n",isp->ae_algo.exp_time_need_updata);
    // printk("isp->ae_algo.a_gain_need_updata=%d\n",isp->ae_algo.a_gain_need_updata); 

	//printk("target_lumiance = %d\n",isp->ae_para.target_lumiance);
   	//printk("current_exp_time=%d\n",isp->ae_run_info_para.current_exp_time);
   	//printk("current_a_gain=%d\n",isp->ae_run_info_para.current_a_gain);
	//printk("isp_d_gain=%d\n",isp->ae_run_info_para.current_isp_d_gain);
    //printk("isp_d_gain_min=%d\n",isp->ae_para.isp_d_gain_min);
	//printk("isp_d_gain_max=%d\n",isp->ae_para.isp_d_gain_max);
	/*	 
     printk("exp_step = %d\n",isp->ae_run_info_para.current_exp_time_step);
     printk("a_step = %d\n",isp->ae_run_info_para.current_a_gain_step);
     printk("d_step = %d\n",isp->ae_run_info_para.current_a_gain_step);
     printk("isp_d_step = %d\n",isp->ae_run_info_para.current_isp_d_gain_step);
   	*/  
	
	if(isp->ae_algo.exp_time_need_updata==AK_TRUE)
	{
		isp->ae_exp_time_effect_frame = _cmos_updata_exp_time(isp->ae_run_info_para.current_exp_time)+1;
		isp->ae_algo.exp_time_need_updata=AK_FALSE;
	}

	if(isp->ae_algo.isp_d_gain_need_updata==AK_TRUE)
	{
		//delay one frame to update isp_d_gain if exp_time need to update
		if(isp->ae_exp_time_effect_frame <= 1)
		{
		//printk("ispd2:%d\n",isp->ae_run_info_para.current_isp_d_gain);
			_isp_update_isp_d_gain(isp->ae_run_info_para.current_isp_d_gain);
			isp->ae_exp_time_effect_frame = 1;
			isp->ae_algo.isp_d_gain_need_updata=AK_FALSE;
		}		
	}
	
	if(isp->ae_algo.a_gain_need_updata ==AK_TRUE)
	{
		int skip_frame;
		skip_frame = 	_cmos_updata_a_gain(isp->ae_run_info_para.current_a_gain)-1;
		if(skip_frame>=0 && skip_frame>isp->ae_drop_count)
			isp->ae_drop_count = skip_frame;
		isp->ae_algo.a_gain_need_updata= AK_FALSE;
	}
	
	if(isp->ae_algo.d_gain_need_updata==AK_TRUE)
	{
		int skip_frame;
		skip_frame = _cmos_updata_d_gain(isp->ae_run_info_para.current_d_gain)-1;
		if(skip_frame>=0 && skip_frame>isp->ae_drop_count)
			isp->ae_drop_count = skip_frame;
		isp->ae_algo.d_gain_need_updata=AK_FALSE;
	}

	if(isp->frame_cnt%AE_WORK_PERIOD!=0)
		return 0;

	if(isp->_3d_nr_status!=0)
	{
		isp->_3d_nr_status = 0;
		isp->linkage_para_update_flag = 1;
	}

	_get_af_stat();
	_get_3dnr_stat();
	_get_yuv_hist();

	if(isp->wdr_info.step>0)
	{
		_calc_wdr_step();
	}
	
	curr_envi_flag  = _get_iso();

	//contrast work
	_isp_contrast_work(curr_envi_flag);

	// 3dnr work
	_isp_3dnr_work(curr_envi_flag);

	//if(isp->_3d_nr_status==10)
	//	isp->linkage_para_update_flag=1;
	
	//printk("curr_envi_flag=%d\n",curr_envi_flag);
	if(isp->curr_envi_flag!=curr_envi_flag||
		isp->linkage_para_update_flag!=0)
	{
		_isp_para_change_with_envi(curr_envi_flag); 
		isp->linkage_para_update_flag=0;
	}

	//if(isp->_3d_nr_status<10)
	//	_isp_3dnr_work(curr_envi_flag);
	
	if (isp->sensor_cb.sensor_timer_func) {
		isp->sensor_cb.sensor_timer_func();
	}

	return 0;
}

int _isp_ccm_change_with_envi(enum isp2_colortemp_mode color_index)
{
	if(isp->ccm_para.cc_mode!=MODE_MANUAL)
	{
		switch(isp->awb_para.colortemp_envi[color_index])
		{
		case ISP2_COLORTEMP_MODE_A:
			isp->cb.cb_memcpy(&(isp->ccm_info.ccm_target), &(isp->ccm_para.ccm[ISP2_COLORTEMP_MODE_A]), sizeof(AK_ISP_CCM));
			break;
		case ISP2_COLORTEMP_MODE_TL84:
			isp->cb.cb_memcpy(&(isp->ccm_info.ccm_target), &(isp->ccm_para.ccm[ISP2_COLORTEMP_MODE_TL84]), sizeof(AK_ISP_CCM));
			break;
		case ISP2_COLORTEMP_MODE_D50:
			isp->cb.cb_memcpy(&(isp->ccm_info.ccm_target), &(isp->ccm_para.ccm[ISP2_COLORTEMP_MODE_D50]), sizeof(AK_ISP_CCM));
			break;
		case ISP2_COLORTEMP_MODE_D65:
			isp->cb.cb_memcpy(&(isp->ccm_info.ccm_target), &(isp->ccm_para.ccm[ISP2_COLORTEMP_MODE_D65]), sizeof(AK_ISP_CCM));
			break;
		default:
			break;
		}
	}
	else
	{
		isp->cb.cb_memcpy(&(isp->ccm_info.ccm_target), &(isp->ccm_para.manual_ccm), sizeof(AK_ISP_CCM));
	}

   	return 0;
}

int _isp_hue_change_with_envi(enum isp2_colortemp_mode color_index)
{
	if(isp->hue_para.hue_mode!=MODE_MANUAL)
	{
		switch(isp->awb_para.colortemp_envi[color_index])
		{
		case ISP2_COLORTEMP_MODE_A:
			isp->cb.cb_memcpy(&(isp->hue_info.hue_target), &(isp->hue_para.hue[ISP2_COLORTEMP_MODE_A]), sizeof(AK_ISP_HUE));
			break;
		case ISP2_COLORTEMP_MODE_TL84:
			isp->cb.cb_memcpy(&(isp->hue_info.hue_target), &(isp->hue_para.hue[ISP2_COLORTEMP_MODE_TL84]), sizeof(AK_ISP_HUE));
			break;
		case ISP2_COLORTEMP_MODE_D50:
			isp->cb.cb_memcpy(&(isp->hue_info.hue_target), &(isp->hue_para.hue[ISP2_COLORTEMP_MODE_D50]), sizeof(AK_ISP_HUE));
			break;
		case ISP2_COLORTEMP_MODE_D65:
			isp->cb.cb_memcpy(&(isp->hue_info.hue_target), &(isp->hue_para.hue[ISP2_COLORTEMP_MODE_D65]), sizeof(AK_ISP_HUE));
			break;
		default:
			break;
		}
	}
	else
	{
		isp->cb.cb_memcpy(&(isp->hue_info.hue_target), &(isp->hue_para.manual_hue), sizeof(AK_ISP_HUE));
	}

   	return 0;
}

//para change with ae
int ak_isp_updata_param_with_envi_work(void)
{
  	// _isp_para_change_with_envi(); 
   	//_ccm_change_with_envi();

	return 0;
}
