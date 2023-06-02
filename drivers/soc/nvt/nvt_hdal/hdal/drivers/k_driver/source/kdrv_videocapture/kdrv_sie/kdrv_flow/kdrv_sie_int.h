#ifndef _KDRV_SIE_INT_H_
#define _KDRV_SIE_INT_H_

#include "kdrv_videocapture/kdrv_sie.h"
#include "sie_lib.h"
#include "sie_limt.h"

#define RAW_SCL_LPF 		50  ///< low-pass valumn, 0~100
#define RAW_SCL_BIN_PWR		0   ///< binning, 0~2
#define RAW_SCL_ADAPT_LPF   1	///< 0: LPF is absolute value; 1: LPF is adaptive value (suggestion: 1)
#define KDRV_SIE_DFT_INT	(KDRV_SIE_INT_VD | KDRV_SIE_INT_CROPEND)

// funtion type in sie driver
typedef void (*KDRV_SIE_DRV_ISR_FP)(UINT32 ui_intp_status);

// funtion type for dal_sie
typedef INT32 (*KDRV_SIE_SET_FP)(KDRV_SIE_PROC_ID, void*);
typedef INT32 (*KDRV_SIE_GET_FP)(KDRV_SIE_PROC_ID, void*);


/**
	SIE KDRV STATE MACHINE

  					      ----- SUSPEND -----
 				         / --->         <--- \
				 resume / /suspend   suspend\ \ resume
                       / /		              \ \
			Open	  < /	     Trig start    \ >
	CLOSE	----->	READY	----------------->  RUN
			<-----	  	 	<-----------------
			Close	   	  	     Trig stop

*/
typedef enum {
	KDRV_SIE_STS_CLOSE = 0,
	KDRV_SIE_STS_READY,
	KDRV_SIE_STS_RUN,
	CTL_SIE_STS_SUSPEND,
	KDRV_SIE_STS_MAX,
	ENUM_DUMMY4WORD(KDRV_SIE_STATUS)
} KDRV_SIE_STATUS;

typedef enum {
	KDRV_SIE_OP_OPEN = 0,
	KDRV_SIE_OP_CLOSE,
	KDRV_SIE_OP_TRIG_START,
	KDRV_SIE_OP_TRIG_STOP,
	KDRV_SIE_OP_SET,
	KDRV_SIE_OP_GET,
	KDRV_SIE_OP_MAX,
	ENUM_DUMMY4WORD(KDRV_SIE_OP)
} KDRV_SIE_OP;

typedef enum {
	KDRV_SIE_PAR_CTL_SET = 0,	//user set parameters
	KDRV_SIE_PAR_CTL_CUR,		//current frame latch parameters
	KDRV_SIE_PAR_CTL_RDY,		//output ready for get
	KDRV_SIE_PAR_CTL_MAX,
	ENUM_DUMMY4WORD(KDRV_SIE_PAR_CTL)
} KDRV_SIE_PAR_CTL;

typedef struct {
	UINT32 ctx_num;
	UINT32 ctx_used;	//current used context number
	UINT32 ctx_idx[KDRV_SIE_MAX_ENG];
	UINT32 ctx_size;;	//each context buf size
	UINT32 start_addr;
	UINT32 req_size;	//total kdrv ctx require size (ctx_num*ctx_size)
} KDRV_SIE_HDL_CONTEXT;

/**
	Collection of sie dal param

	b_encode_en:
		Only control Enable/Disable
		Enable will cause ch5 output sideinfo, and Channel lineoffset can set to 70%
		note that encode can only enable when data_fmt = KDRV_SIE_BAYER_12

		Channel 5 Setting
		lineoffset = align_ceil_32(scl_size.w) * 16/8
		buffer_size = lineoffset * scl_size.h

*/
typedef struct {
	KDRV_SIE_OPENCFG open_cfg;				///< sie open config, no runtime change
	KDRV_SIE_ISRCB isrcb_fp;				///< isr cb function pointer
	KDRV_SIE_INT inte;						///< interrupt enable
	UINT32 func_en;							///< sie function enable
	KDRV_SIE_OUT_DEST out_dest;				///< sie output destination
	KDRV_SIE_DATA_FMT data_fmt;				///< sie data format
	KDRV_SIE_CCIR_INFO ccir_info;			///< ccir information
	KDRV_SIE_SIGNAL signal;					///< signal info
	KDRV_SIE_ACT_CRP_WIN act_window;			///< active window
	KDRV_SIE_ACT_CRP_WIN crp_window;			///< crop window
	USIZE scl_size;							///< raw scale size
	UINT32 out_ch_lof[KDRV_SIE_CH_MAX];		///< line offset of each output channel
	UINT32 out_ch_addr[KDRV_SIE_CH_MAX];	///< address of each output channel
	KDRV_SIE_PATGEN_INFO pat_gen_info;		///< pattern mode info
	KDRV_SIE_FLIP flip;						///< flip type
	KDRV_SIE_OB_PARAM ob_param;				///< ob info
	KDRV_SIE_CA_PARAM ca_param;				///< ca info
	KDRV_SIE_CA_ROI ca_roi;                 ///< ca ROI
	KDRV_SIE_LA_PARAM la_param;				///< la info
	KDRV_SIE_LA_ROI la_roi;				    ///< la ROI
	KDRV_SIE_RAW_ENCODE encode_info;        ///< raw encode info
	KDRV_SIE_DGAIN dgain;					///< digital gain
	KDRV_SIE_CGAIN cgain;					///< color gain
	KDRV_SIE_DPC dpc_info;					///< dpc info
	KDRV_SIE_ECS ecs_info;					///< ecs info

	BOOL b_dma_out;							///< dma output enable
	KDRV_SIE_BP_INFO bp_info;               ///< bp info
	KDRV_SIE_COMPANDING comp_info;          ///< companding info
	KDRV_SIE_SINGLE_OUT_CTRL singleout_info;///< single-out info for all channel
	KDRV_SIE_DRAM_OUT_CTRL ch_output_mode;  ///< channel output mode
	KDRV_SIE_RINGBUF_INFO ring_buf_info;
	KDRV_SIE_MD md_info;					///< md info
	KDRV_SIE_PROC_ID ref_load_id;			///< load reference
	BOOL dma_abort;							///< dma abort
	KDRV_SIE_DVS_CODE dvs_code;				///< dvs code
	BOOL dvs_en;							///< dvs enable
	USIZE dvs_size;							///< dvs window size
	KDRV_SIE_VD_DLY vd_dly;					///< vd delay
	UINT16 ccir_header;						///< ccir header (for debug)
	UINT32 ccir_sts;						///< ccir haeder hit status (for debug)

//internal control parameters
	UINT32 ca_out_addr[KDRV_SIE_PAR_CTL_MAX];
	UINT32 la_out_addr[KDRV_SIE_PAR_CTL_MAX];
	SIE_CA_WIN_INFO ca_win[KDRV_SIE_PAR_CTL_MAX];
	SIE_LA_WIN_INFO la_win[KDRV_SIE_PAR_CTL_MAX];
} KDRV_SIE_INFO;

typedef struct {
	SIE_MCLKSRC_SEL mclk_src;
	SIE_PXCLKSRC	pxclk_src;
	SIE_CLKSRC_SEL	clk_src;
} KDRV_SIE_FB_CLK_INFO;

typedef enum {
	KDRV_SIE_INT_DATA_FMT_RAW		= 0x00000001, // KDRV_SIE_BAYER_xx
	KDRV_SIE_INT_DATA_FMT_CCIR		= 0x00000002, // KDRV_SIE_YUV_xx
	KDRV_SIE_INT_DATA_FMT_DVS		= 0x00000004, // KDRV_SIE_Y_8
} KDRV_SIE_INT_DATA_FMT;

KDRV_SIE_STATUS kdrv_sie_get_state_machine(KDRV_SIE_PROC_ID id);
KDRV_SIE_INFO *kdrv_sie_get_hdl(KDRV_SIE_PROC_ID id);
BOOL kdrv_sie_chk_fmt(UINT32 hdl, KDRV_SIE_DATA_FMT src, KDRV_SIE_INT_DATA_FMT chk);
SIE_ENGINE_ID kdrv_sie_conv2_sie_id(KDRV_SIE_PROC_ID id);
BOOL kdrv_sie_chk_id_valid(KDRV_SIE_PROC_ID id);
INT32 kdrv_sie_get_sie_limit(KDRV_SIE_PROC_ID id, void *data);
KDRV_SIE_HDL_CONTEXT *kdrv_sie_get_ctx(void);
KDRV_SIE_CLK_HDL *kdrv_sie_get_clk_hdl(KDRV_SIE_PROC_ID id);
UINT32 kdrv_sie_get_apb_clk(void);

#endif //_KDRV_SIE_INT_H_
