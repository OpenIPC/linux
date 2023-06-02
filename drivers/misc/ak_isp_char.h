#ifndef __AK_ISP_CHAR_H__
#define __AK_ISP_CHAR_H__

#define AKISP_MAGIC 'I'
#define AK_ISP_VP_GET_BLC			_IOR(AKISP_MAGIC,  1, int)
#define AK_ISP_VP_SET_BLC			_IOW(AKISP_MAGIC,  2, int)
#define AK_ISP_VP_GET_LSC      		_IOR(AKISP_MAGIC,  3, int)
#define AK_ISP_VP_SET_LSC      		_IOW(AKISP_MAGIC,  4, int)
#define AK_ISP_VP_GET_GB			_IOR(AKISP_MAGIC,  5, int)
#define AK_ISP_VP_SET_GB			_IOW(AKISP_MAGIC,  6, int)
#define AK_ISP_VP_GET_GB_LINKAGE	_IOR(AKISP_MAGIC,  7, int)
#define AK_ISP_VP_SET_GB_LINKAGE	_IOW(AKISP_MAGIC,  8, int)
#define AK_ISP_VP_GET_RAW_LUT		_IOR(AKISP_MAGIC,  9, int)
#define AK_ISP_VP_SET_RAW_LUT		_IOW(AKISP_MAGIC,  10, int)
#define AK_ISP_VP_GET_RAW_NR1		_IOR(AKISP_MAGIC,  11, int)
#define AK_ISP_VP_SET_RAW_NR1		_IOW(AKISP_MAGIC,  12, int)
#define AK_ISP_VP_GET_DEMO			_IOR(AKISP_MAGIC,  13, int)
#define AK_ISP_VP_SET_DEMO			_IOW(AKISP_MAGIC,  14, int)
#define AK_ISP_GET_DPC				_IOR(AKISP_MAGIC,  15, int)
#define AK_ISP_SET_DPC				_IOW(AKISP_MAGIC,  16, int)
#define AK_ISP_GET_CCM				_IOR(AKISP_MAGIC,  17, int)
#define AK_ISP_SET_CCM				_IOW(AKISP_MAGIC,  18, int)
#define AK_ISP_GET_CCM_EX			_IOR(AKISP_MAGIC,  19, int)
#define AK_ISP_SET_CCM_EX			_IOW(AKISP_MAGIC,  20, int)
        
#define AK_ISP_SET_WHITE_COLOR_S	_IOW(AKISP_MAGIC,  21, int)
#define AK_ISP_GET_WHITE_COLOR_S	_IOR(AKISP_MAGIC,  22, int)
#define AK_ISP_SET_CCM_FINE			_IOW(AKISP_MAGIC,  23, int)
#define AK_ISP_GET_CCM_FINE			_IOR(AKISP_MAGIC,  24, int)
        
#define AK_ISP_GET_RGB_GAMMA		_IOR(AKISP_MAGIC,  25, int)
#define AK_ISP_SET_RGB_GAMMA		_IOW(AKISP_MAGIC,  26, int)
        
#define AK_ISP_SET_WDR				_IOW(AKISP_MAGIC,  27, int)
#define AK_ISP_GET_WDR				_IOR(AKISP_MAGIC,  28, int)
#define AK_ISP_SET_WDR_EX			_IOW(AKISP_MAGIC,  29, int)
#define AK_ISP_GET_WDR_EX			_IOR(AKISP_MAGIC,  30, int)
#define AK_ISP_SET_EDGE				_IOW(AKISP_MAGIC,  31, int)
#define AK_ISP_GET_EDGE				_IOR(AKISP_MAGIC,  32, int)
#define AK_ISP_SET_EDGE_EX			_IOW(AKISP_MAGIC,  33, int)
#define AK_ISP_GET_EDGE_EX			_IOR(AKISP_MAGIC,  34, int)
#define AK_ISP_SET_EDGE_LINKAGE		_IOW(AKISP_MAGIC,  35, int)
#define AK_ISP_GET_EDGE_LINKAGE		_IOR(AKISP_MAGIC,  36, int)
#define AK_ISP_SET_SHARP			_IOW(AKISP_MAGIC,  37, int)
#define AK_ISP_GET_SHARP			_IOR(AKISP_MAGIC,  38, int)
#define AK_ISP_SET_SHARP_EX			_IOW(AKISP_MAGIC,  39, int)
#define AK_ISP_GET_SHARP_EX			_IOR(AKISP_MAGIC,  40, int)
#define AK_ISP_SET_SHARP_LINKAGE	_IOW(AKISP_MAGIC,  41, int)
#define AK_ISP_GET_SHARP_LINKAGE	_IOR(AKISP_MAGIC,  42, int)
#define AK_ISP_SET_Y_NR2			_IOW(AKISP_MAGIC,  43, int)
#define AK_ISP_GET_Y_NR2			_IOR(AKISP_MAGIC,  44, int)
#define AK_ISP_SET_Y_NR2_LINKAGE	_IOW(AKISP_MAGIC,  45, int)
#define AK_ISP_GET_Y_NR2_LINKAGE	_IOR(AKISP_MAGIC,  46, int)
#define AK_ISP_SET_3D_NR			_IOW(AKISP_MAGIC,  47, int)
#define AK_ISP_GET_3D_NR			_IOR(AKISP_MAGIC,  48, int)
#define AK_ISP_SET_3D_NR_EX			_IOW(AKISP_MAGIC,  49, int)
#define AK_ISP_GET_3D_NR_EX			_IOR(AKISP_MAGIC,  50, int)
#define AK_ISP_SET_3D_NR_LINKAGE	_IOW(AKISP_MAGIC,  51, int)
#define AK_ISP_GET_3D_NR_LINKAGE	_IOR(AKISP_MAGIC,  52, int)
        
#define AK_ISP_GET_FCS				_IOR(AKISP_MAGIC,  53, int)
#define AK_ISP_SET_FCS				_IOW(AKISP_MAGIC,  54, int)
        
#define AK_ISP_SET_FCS_LINKAGE		_IOW(AKISP_MAGIC,  55, int)
#define AK_ISP_GET_FCS_LINKAGE		_IOR(AKISP_MAGIC,  56, int)
#define AK_ISP_SET_CONTRAST			_IOW(AKISP_MAGIC,  57, int)
#define AK_ISP_GET_CONTRAST			_IOR(AKISP_MAGIC,  58, int)
#define AK_ISP_SET_SAT				_IOW(AKISP_MAGIC,  59, int)
#define AK_ISP_GET_SAT				_IOR(AKISP_MAGIC,  60, int)
#define AK_ISP_SET_SAT_LINKAGE		_IOW(AKISP_MAGIC,  61, int)
#define AK_ISP_GET_SAT_LINKAGE		_IOR(AKISP_MAGIC,  62, int)
#define AK_ISP_SET_RGB2YUV			_IOW(AKISP_MAGIC,  63, int)
#define AK_ISP_GET_RGB2YUV			_IOR(AKISP_MAGIC,  64, int)
#define AK_ISP_SET_YUV_EFFECT		_IOW(AKISP_MAGIC,  65, int)
#define AK_ISP_GET_YUV_EFFECT		_IOR(AKISP_MAGIC,  66, int)
#define AK_ISP_SET_RAW_HIST			_IOW(AKISP_MAGIC,  67, int)
#define AK_ISP_GET_RAW_HIST			_IOR(AKISP_MAGIC,  68, int)
        
#define AK_ISP_GET_RAW_HIST_STAT	_IOR(AKISP_MAGIC,  69, int)
#define AK_ISP_SET_RGB_HIST			_IOW(AKISP_MAGIC,  70, int)
#define AK_ISP_GET_RGB_HIST			_IOR(AKISP_MAGIC,  71, int)
#define AK_ISP_GET_RGB_HIST_STAT	_IOR(AKISP_MAGIC,  72, int)
        
#define AK_ISP_SET_Y_HIST			_IOW(AKISP_MAGIC,  73, int)
#define AK_ISP_GET_Y_HIST			_IOR(AKISP_MAGIC,  74, int)
#define AK_ISP_GET_Y_HIST_STAT		_IOR(AKISP_MAGIC,  75, int)
#define AK_ISP_SET_EXP_TYPE			_IOW(AKISP_MAGIC,  76, int)
#define AK_ISP_GET_EXP_TYPE			_IOR(AKISP_MAGIC,  77, int)
#define AK_ISP_SET_FRAME_RATE		_IOW(AKISP_MAGIC,  78, int)
#define AK_ISP_GET_FRAME_RATE		_IOR(AKISP_MAGIC,  79, int)
#define AK_ISP_SET_AE				_IOW(AKISP_MAGIC,  80, int)
#define AK_ISP_GET_AE				_IOR(AKISP_MAGIC,  81, int)
#define AK_ISP_GET_AE_RUN_INFO		_IOR(AKISP_MAGIC,  82, int)
#define AK_ISP_SET_WB_TYPE			_IOW(AKISP_MAGIC,  83, int)
#define AK_ISP_GET_WB_TYPE			_IOR(AKISP_MAGIC,  84, int)
#define AK_ISP_SET_AWB				_IOW(AKISP_MAGIC,  85, int)
#define AK_ISP_GET_AWB				_IOR(AKISP_MAGIC,  86, int)
#define AK_ISP_SET_AWB_DEFAULT		_IOW(AKISP_MAGIC,  87, int)
#define AK_ISP_GET_AWB_DEFAULT		_IOR(AKISP_MAGIC,  88, int)
#define AK_ISP_GET_AWB_STAT_INFO	_IOR(AKISP_MAGIC,  89, int)
        
#define AK_ISP_SET_MASK_COLOR		_IOW(AKISP_MAGIC,  92, int)
#define AK_ISP_GET_MASK_COLOR	    _IOR(AKISP_MAGIC,  93, int)
#define AK_ISP_SET_WEIGHT			_IOW(AKISP_MAGIC,  94, int)
#define AK_ISP_GET_WEIGHT	   		 _IOR(AKISP_MAGIC, 95, int)
#define AK_ISP_SET_AF				_IOW(AKISP_MAGIC,  96, int)
#define AK_ISP_GET_AF	    		_IOR(AKISP_MAGIC,  97, int)
#define AK_ISP_GET_AF_STAT	    	_IOR(AKISP_MAGIC,  98, int)
#define AK_ISP_SET_MWB				_IOW(AKISP_MAGIC,  99, int)
#define AK_ISP_GET_MWB				_IOR(AKISP_MAGIC,  100,int)
        
#define AK_ISP_SET_MAIN_CHAN_MASK_AREA	_IOW(AKISP_MAGIC,  90, int)
#define AK_ISP_GET_MAIN_CHAN_MASK_AREA	_IOR(AKISP_MAGIC,  91, int)
#define AK_ISP_SET_SUB_CHAN_MASK_AREA	_IOW(AKISP_MAGIC,  101, int)
#define AK_ISP_GET_SUB_CHAN_MASK_AREA	_IOR(AKISP_MAGIC,  102, int)
        
#define AK_ISP_SET_3D_NR_REF		_IOW(AKISP_MAGIC,  103, int)
#define AK_ISP_GET_3D_NR_REF	    _IOR(AKISP_MAGIC,  104, int)
        
#define AK_ISP_INIT_SENSOR_DEV		_IOW(AKISP_MAGIC,  105, int)
#define AK_ISP_SET_3D_NR_PHYADDR	_IOW(AKISP_MAGIC,  106, int)
#define AK_ISP_SET_SENSOR_REG		_IOW(AKISP_MAGIC,  107, int)
#define AK_ISP_GET_SENSOR_REG		_IOR(AKISP_MAGIC,  108, int)
#define AK_ISP_SET_USER_PARAMS		_IOW(AKISP_MAGIC,  109, int)
#define AK_ISP_SET_MISC_ATTR		_IOW(AKISP_MAGIC,  110, int)
#define AK_ISP_GET_MISC_ATTR		_IOR(AKISP_MAGIC,  111, int)
        
#define AK_ISP_GET_3D_NR_STAT_INFO	_IOR(AKISP_MAGIC,  112, int)
#define AK_ISP_GET_SENSOR_ID		_IOR(AKISP_MAGIC,  113, int)
        
#define AK_ISP_SET_ISP_CAPTURING	_IOW(AKISP_MAGIC,  114, int)
        
#define AK_ISP_SET_AWB_EX			_IOW(AKISP_MAGIC,  115, int)
#define AK_ISP_GET_AWB_EX			_IOR(AKISP_MAGIC,  116, int)
        
#define AK_ISP_SET_Y_GAMMA			_IOW(AKISP_MAGIC,  117, int)
#define AK_ISP_GET_Y_GAMMA			_IOR(AKISP_MAGIC,  118, int)
        
#define AK_ISP_SET_HUE				_IOW(AKISP_MAGIC,  119, int)
#define AK_ISP_GET_HUE				_IOR(AKISP_MAGIC,  120, int)
        
#define AK_ISP_SET_FLIP_MIRROR		_IOW(AKISP_MAGIC,  121, int)
#define AK_ISP_SET_SENSOR_FPS		_IOW(AKISP_MAGIC,  122, int)
#define AK_ISP_GET_SENSOR_FPS		_IOR(AKISP_MAGIC,  123, int)
         
#define  AK_ISP_GET_WORK_SCENE		_IOR(AKISP_MAGIC,  124, int)
#define	 AK_ISP_GET_ISO				_IOR(AKISP_MAGIC,  125, int)


#define AK_ISP_SET_AF_WIN34			_IOW(AKISP_MAGIC,  126, int)

#define AK_ISP_SET_MAE              _IOW(AKISP_MAGIC,  127, int)
#define AK_ISP_GET_MAE			    _IOR(AKISP_MAGIC,  128, int)

/* Blow is the params that user can adjust in real time */
typedef struct {
	int id;
	unsigned char data[128];
} AK_ISP_USER_PARAM;

struct isp_zoom_info {
	int channel;
	int cut_xpos;
	int cut_ypos;
	int cut_width;
	int cut_height;
	int out_width;
	int out_height;
};

struct isp_channel2_info {
	int width;
	int height;
};

struct isp_mask_area {
	unsigned short start_xpos;
	unsigned short end_xpos;
	unsigned short start_ypos;
	unsigned short end_ypos;
	unsigned char enable;
};

struct isp_mask_area_info {
	struct isp_mask_area mask[4];
};

struct isp_mask_color_info {
	unsigned char color_type;
	unsigned char mk_alpha;
	unsigned char y_mk_color;
	unsigned char u_mk_color;
	unsigned char v_mk_color;
};

struct isp_gamma_info {
	int value;
};

struct isp_saturation_info {
	int value;
};

struct isp_brightness_info {
	int value;
};

struct isp_contrast_info {
	int value;
};

struct isp_sharp_info {
	int value;
};

struct isp_power_line_freq_info {
	int value;
};

struct isp_flip_mirror_info {
	int flip_en;
	int mirror_en;
};

struct isp_osd_color_table_attr {
	unsigned int color_table[16];
};

typedef enum isp_osd_channel {
	ISP_OSD_CHN0 = 0,
	ISP_OSD_CHN1,
	ISP_OSD_CHN2,
	ISP_OSD_CHN_NUM
} ISP_OSD_CHANNEL;

struct isp_osd_context_attr {
	ISP_OSD_CHANNEL chn;
	unsigned char	*osd_context_addr;
	unsigned int	osd_width;
	unsigned int	osd_height;
	unsigned short	start_xpos;
	unsigned short	start_ypos;
	unsigned short	alpha;
	unsigned short	enable;
};

struct isp_osd_mem_attr {
	ISP_OSD_CHANNEL chn;
	unsigned char	*dma_paddr;	
	unsigned int	size;
};

#define AK_ISP_USER_CID_SET_ZOOM			_IOW(AKISP_MAGIC,  0x00010000, int)
#define AK_ISP_USER_CID_SET_SUB_CHANNEL		_IOW(AKISP_MAGIC,  0x00010001, int)
#define AK_ISP_USER_CID_SET_OCCLUSION		_IOW(AKISP_MAGIC,  0x00010002, int)
#define AK_ISP_USER_CID_SET_OCCLUSION_COLOR	_IOW(AKISP_MAGIC,  0x00010003, int)
#define AK_ISP_USER_CID_SET_GAMMA			_IOW(AKISP_MAGIC,  0x00010004, int)
#define AK_ISP_USER_CID_SET_SATURATION		_IOW(AKISP_MAGIC,  0x00010005, int)
#define AK_ISP_USER_CID_SET_BRIGHTNESS		_IOW(AKISP_MAGIC,  0x00010006, int)
#define AK_ISP_USER_CID_SET_CONTRAST		_IOW(AKISP_MAGIC,  0x00010007, int)
#define AK_ISP_USER_CID_SET_SHARPNESS		_IOW(AKISP_MAGIC,  0x00010008, int)

#define AK_ISP_USER_CID_SET_POWER_LINE_FREQUENCY \
			_IOW(AKISP_MAGIC,  0x00010009, int)
			
#define AK_ISP_USER_CID_SET_OSD_COLOR_TABLE_ATTR \
			_IOW(AKISP_MAGIC,  0x0001000a, int)
			
#define AK_ISP_USER_CID_SET_MAIN_CHANNEL_OSD_CONTEXT_ATTR \
			_IOW(AKISP_MAGIC,  0x0001000b, int)
			
#define AK_ISP_USER_CID_SET_SUB_CHANNEL_OSD_CONTEXT_ATTR \
			_IOW(AKISP_MAGIC,  0x0001000c, int)
			
#define AK_ISP_USER_CID_SET_MAIN_CHANNEL_OSD_MEM_ATTR \
			_IOW(AKISP_MAGIC,  0x0001000d, int)
			
#define AK_ISP_USER_CID_SET_SUB_CHANNEL_OSD_MEM_ATTR \
			_IOW(AKISP_MAGIC,  0x0001000e, int)

#endif
