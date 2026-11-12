#ifndef _AK39_ISP2_H
#define _AK39_ISP2_H

#include <mach/anyka_types.h>
/* Lives in include/plat-anyka/ in this tree, not beside the source. */
#include <plat-anyka/ak_isp_drv.h>


//lz0499 9_16
enum ak_isp_3dnr_height_block_size {
    TNR_HEIGHT_BLOCKSIZE_16=0,        
    TNR_HEIGHT_BLOCKSIZE_24,          
};
enum reg_updata_mode
{
    MODE_LEAST,  
    MODE_MOST,   
    MODE_ALL,  
};

enum  envi_flag
{
    ENVI_OUTDOOR=0,
    ENVI_INDOOR,
    ENVI_ISO_2,
    ENVI_ISO_4,
    ENVI_ISO_8,
    ENVI_ISO_16,
    ENVI_ISO_32,
    ENVI_ISO_64,
    ENVI_ISO_128,
    ENVI_MAX
};

enum isp2_colortemp_mode {
    ISP2_COLORTEMP_MODE_A = 0,
    ISP2_COLORTEMP_MODE_TL84,
    ISP2_COLORTEMP_MODE_D50,
    ISP2_COLORTEMP_MODE_D65,
    ISP2_COLORTEMP_MODE_A_1,
    ISP2_COLORTEMP_MODE_TL84_1,
    ISP2_COLORTEMP_MODE_D50_1,
    ISP2_COLORTEMP_MODE_COUNT,
};

typedef struct ak_isp_ccm_info
{
    AK_ISP_CCM  ccm_target;
    AK_ISP_CCM  ccm_current;
    T_U16	strength;	//[0, 256] Q8, derived from total gain
}AK_ISP_CCM_INFO;

typedef struct ak_isp_hue_info
{
	AK_ISP_HUE	hue_target;
	AK_ISP_HUE	hue_current;
}AK_ISP_HUE_INFO;

typedef struct ak_isp_wdr_info
{
	T_U16 step;              //from 16 to 0, change frome last to target_wdr
	AK_ISP_WDR last_wdr;
	AK_ISP_WDR target_wdr; 
	AK_ISP_WDR current_wdr; 
}AK_ISP_WDR_INFO;

typedef struct isp2_struct {
    T_U8    *base;          /* reg base address */
    T_U8    *area;          /* virtual pointer */
    T_U32   handle;         /* physical pointer */
    T_U32   bytes;          /* buffer size in bytes */

    AK_ISP_OSD_CONTEXT_ATTR main_osd_buffer[OSD_CHN_NUM];          /* buffer for main channel osd*/
    T_U8    *area_main_osd[OSD_CHN_NUM];          /* virtual pointer for main channel osd*/
    T_U32   handle_main_osd[OSD_CHN_NUM];         /* physical pointer for main channel osd*/
    T_U32   bytes_main_osd[OSD_CHN_NUM];          /* buffer size in bytes for main channel osd*/

    AK_ISP_OSD_CONTEXT_ATTR sub_osd_buffer[OSD_CHN_NUM];          /* buffer for sub channel osd*/
    T_U8    *area_sub_osd[OSD_CHN_NUM];          /* virtual pointer for sub channel osd*/
    T_U32   handle_sub_osd[OSD_CHN_NUM];         /* physical pointer for sub channel osd*/
    T_U32   bytes_sub_osd[OSD_CHN_NUM];          /* buffer size in bytes for sub channel osd*/

    enum reg_updata_mode reg_update_flag;
    int	linkage_para_update_flag;
    int	linkage_ccm_update_flag;
	int linkage_hue_update_flag;
	int awb_needs_seed;
    enum  envi_flag curr_envi_flag;
    enum isp2_colortemp_mode last_colortemp;
	int main_osd_chn_update_flag[OSD_CHN_NUM];
	int sub_osd_chn_update_flag[OSD_CHN_NUM];
	int main_osd_update_flag;
	int sub_osd_update_flag;

   T_U32 *sdpc_area;
   T_U32 sdpc_handle;
   T_U8 *stat_area;
   T_U32 stat_handle;
   
    enum isp_working_mode cur_mode;
    
    unsigned long   *reg_blkaddr1;
    unsigned long   *reg_blkaddr2;
    unsigned long   *reg_blkaddr3;
    unsigned long   *reg_blkaddr4;
    unsigned long   *reg_blkaddr5;
    
    /*the input size from sensor*/
    int cut_width;
    int cut_height;

    /*
     * Rows of 3D-NR motion statistics the ISP writes, 16 or 24, as
     * set_crop programmed it. The AF and AWB statistics follow the
     * motion block in the DMA statistics area, so their offsets move
     * with it.
     */
    int tnr_height_block_num;

    /* master channel output size*/
    int ch1_width;
    int ch1_height;
    int chl1_xrate;
    int chl1_yrate;

    /* minor channel */
    int ch2_width;  
    int ch2_height;
    int chl2_xrate;
    int chl2_yrate;
    


    T_SUBSMP_RTO                sub_sample;             //0:1x,1:2x,2:4x,3:8x
    
    AK_ISP_BLC_ATTR             blc_para;
    AK_ISP_LSC_ATTR             lsc_para;
    AK_ISP_RAW_LUT_ATTR         raw_lut_para;
    AK_ISP_RGB_GAMMA_ATTR       rgb_gamma_para;
    AK_ISP_Y_GAMMA_ATTR         y_gamma_para;
    AK_ISP_NR1_ATTR             nr1_para;      
    AK_ISP_NR2_ATTR             nr2_para;
   
    AK_ISP_3D_NR_ATTR           _3d_nr_para;
    //int _3d_nr_enable;
    int _3d_nr_status;
    AK_ISP_NR1 * cur_env_nr1;
    AK_ISP_NR2* cur_env_nr2;
    AK_ISP_SHARP *cur_env_sharp;	
    AK_ISP_NR1 * cur_3dnr_nr1;
    AK_ISP_NR2* cur_3dnr_nr2;
    AK_ISP_SHARP *cur_3dnr_sharp;
    AK_ISP_3D_NR_REF_ATTR       _3d_nr_ref_para;
    AK_ISP_3D_NR_STAT_INFO	    _3d_nr_stat_para;
    AK_ISP_GB_ATTR              gb_para;
    AK_ISP_DEMO_ATTR            demo_para;
    AK_ISP_CCM_ATTR             ccm_para;
    AK_ISP_CCM_INFO			    ccm_info;
    
    AK_ISP_WDR_ATTR             wdr_para;
    AK_ISP_WDR_EX_ATTR          wdr_ex_para;
    AK_ISP_WDR_INFO				wdr_info;
   
    AK_ISP_SHARP_ATTR           sharp_para;
    AK_ISP_SHARP_EX_ATTR        sharp_ex_para;
    AK_ISP_FCS_ATTR             fcs_para;
	AK_ISP_HUE_ATTR             hue_para;
	AK_ISP_HUE_INFO				hue_info;
    AK_ISP_CONTRAST	        	contrast_setting;
    AK_ISP_CONTRAST_ATTR		contrast_para;
    AK_ISP_SATURATION_ATTR      saturation_para;
    AK_ISP_RGB2YUV_ATTR         rgb2yuv_para;
    AK_ISP_EFFECT_ATTR          effect_para;
    AK_ISP_DDPC_ATTR            dpc_para;
    AK_ISP_SDPC_ATTR            sdpc_para;
    
    AK_ISP_AF_ATTR              af_para;
    AK_ISP_AF_STAT_INFO         af_stat_para;
    
    AK_ISP_WEIGHT_ATTR          weight_para;
    
    AK_ISP_WB_TYPE_ATTR         wb_type_para;
    AK_ISP_MWB_ATTR             mwb_para;
    AK_ISP_AWB_ATTR             awb_para;
    AK_ISP_AWB_EX_ATTR          awb_ex_ctrl_para;	
    AK_ISP_AWB_STAT_INFO        awb_stat_info_para;
    AK_ISP_WB_GAIN              wb_gain_para;
    AK_ISP_AWB_ALGO             awb_algo;
    
    AK_ISP_RAW_HIST_ATTR        raw_hist_para;
    AK_ISP_RAW_HIST_STAT_INFO   raw_hist_stat_para;
    AK_ISP_RGB_HIST_ATTR        rgb_hist_para;
    AK_ISP_RGB_HIST_STAT_INFO   rgb_hist_stat_para;
    AK_ISP_YUV_HIST_ATTR        yuv_hist_para;
    AK_ISP_YUV_HIST_STAT_INFO   yuv_hist_stat_para;
    
    AK_ISP_EXP_TYPE             exp_type_para;
	AK_ISP_MAE_ATTR  			mae_para;

    AK_ISP_AE_ATTR              ae_para;
    T_U8					ae_para_update_flag;
	T_U8					mae_para_update_flag;
    int					ae_drop_count;
    int					ae_drop_flag;
    int					ae_last_lumi;
    int					ae_isp_d_gain;
    int					ae_exp_time_effect_frame;
    int					current_y_gain;
    AK_ISP_AE_RUN_INFO          ae_run_info_para;
    //AK_ISP_WEIGHT_ATTR        weight_para;
    AK_ISP_AEC_ALGO             ae_algo;
	AK_ISP_FRAME_RATE_ATTR		frame_rate_para;
    AK_ISP_MISC_ATTR            misc_para;
    AK_ISP_MASK_AREA_ATTR       mask_area_para;
    AK_ISP_MASK_COLOR_ATTR      mask_color_para;
    
    AK_ISP_MAIN_CHAN_MASK_AREA_ATTR  main_chan_mask_area_para;
    AK_ISP_SUB_CHAN_MASK_AREA_ATTR   sub_chan_mask_area_para;

    AK_ISP_FUNC_CB      cb;         //kernel dependent callbacks
    AK_ISP_SENSOR_CB    sensor_cb;  //sensor dependent callbacks
    
    //register block addresses
    unsigned long   *cfg_mem_blkaddr1;
    unsigned long   *cfg_mem_blkaddr2;
    unsigned long   *cfg_mem_blkaddr3;
    unsigned long   *cfg_mem_blkaddr4;
    unsigned long   *cfg_mem_blkaddr5;
    unsigned long   *cfg_mem_blkaddr6;

    //statistics buffer addresses
    unsigned long       *stat_addr[4];
    int              current_buffer;
    int		next_buffer;
    int		enable_buffer_list;
    
    int		frame_cnt;
	volatile int pause_flag;	//require irq to stop isp

	int _3dnr_reset;
}AK_ISP_STRUCT;
/***************************************************************************
        
***************************************************************************/

int _set_lsc_attr(AK_ISP_LSC_ATTR *p_lsc);

/**
 * @brief: set blc param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *blc:  blc param
 */
int _set_blc_attr(AK_ISP_BLC *blc);

/**
 * @brief: set dpc param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_dpc:  dpc param
 */
int _set_dpc_attr(AK_ISP_DDPC* p_dpc);


/**
 * @brief: set gb param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *gb:  gb param
 */
int _set_gb_attr( AK_ISP_GB *gb);

/**
 * @brief: set raw noise remove param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *nr1:  raw noise remove param 
 */
int _set_nr1_attr( AK_ISP_NR1 *nr1);


/**
 * @brief: set yuv noise remove param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *nr2:  yuv noise remove param 
 */
int _set_nr2_attr( AK_ISP_NR2 *nr2);


/**
 * @brief: set 3d noise remove param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *_3d_nr:  3d noise remove param 
 */
int _set_3d_nr_attr(AK_ISP_3D_NR *_3d_nr);




/**
 * @brief: set sharp param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *sharp: sharp param 
 */
int _set_sharp_attr( AK_ISP_SHARP *sharp);

/**
 * @brief: set wdr param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_wdr: wdr param 
 */
int _set_wdr_attr(AK_ISP_WDR*p_wdr);

/**
 * @brief: set saturation param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *sat: saturation param 
 */
int _set_saturation_attr( AK_ISP_SATURATION *sat);

/**
 * @brief: set ccm param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *cc: cc param 
 */
int _set_ccm_attr( AK_ISP_CCM *cc);

/**
 * @brief: set fcs param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *fcs: fcs param 
 */
int _set_fcs_attr( AK_ISP_FCS *fcs);

/**
 * @brief: set hue param 
 * @author: lz
 * @date: 2016-8-26
 * @param [in] *hue: hue param 
 */
int _set_hue_attr( AK_ISP_HUE*hue);




/**
 * @brief: set awb param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *awb: awb param 
 */
int  _set_awb_attr( AK_ISP_AWB_ATTR *awb);

/**
 * @brief: set contrast param 
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_contrast: contrast param 
 */
int _set_contrast_attr(AK_ISP_CONTRAST  *p_contrast);

int _isp_para_change_with_envi(enum  envi_flag flag);
int _isp_ccm_change_with_envi(enum isp2_colortemp_mode color_index);
int _isp_hue_change_with_envi(enum isp2_colortemp_mode color_index);


extern AK_ISP_STRUCT *isp;

#define printk      isp->cb.cb_printk
#define memcpy      isp->cb.cb_memcpy
#define memset      isp->cb.cb_memset
#define malloc      isp->cb.cb_malloc
#define free        isp->cb.cb_free
#define dma_malloc  isp->cb.cb_dmamalloc
#define dma_free    isp->cb.cb_dmafree

//////////////////////////////////////////////////////////////////////////////////////////////
#endif //#define _AK39_ISP2_H
