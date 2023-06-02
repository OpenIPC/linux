/**
@brief Header file of NN engine definition of vendor net flow sample.

@file nn_diff.h

@ingroup net_flow_sample

@note Nothing.

Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/


#ifndef _NN_DIFF_H_
#define _NN_DIFF_H_



//#include "type.h"
#include "nn_net.h"
#include "kflow_ai_net/kflow_ai_net.h"

#define NN_DIFF_IMEM_NUM                     8
#define NN_DIFF_OMEM_NUM                     3

typedef struct _NN_DIFF_IOMEM {
  NN_DATA SAI[NN_DIFF_IMEM_NUM];
  NN_DATA SAO[NN_DIFF_OMEM_NUM];
} NN_DIFF_IOMEM;


typedef struct _NN_DIFF_CONFIG {
	UINT32 scale_w;
	UINT32 scale_h;
	char NN_SCALE_CONFIG[16];      //< different scale model name
}NN_DIFF_CONFIG;


typedef struct _NN_DIFF_HEAD {
	NN_DIFF_CONFIG diff_id;         ///< configuration of scale model
	UINT32 diff_parm_size;          // diff parameters size
	UINT32 io_buff_size;            // diff IO buffer size
	
} NN_DIFF_HEAD;


typedef struct _NN_DIFF_MODEL_HEAD {
	UINT32 stripe_model_num;         ///< configuration of nvt_diff_model.bin
	UINT32 ker_va;
	UINT32 pa;
	UINT32 total_size;
} NN_DIFF_MODEL_HEAD;      


typedef struct _NN_DIFF_MODE_CTRL { ///< mode control of nvt_diff_model.bin
	UINT32 offset;                    

} NN_DIFF_MODE_CTRL;


typedef struct _NN_DIFF_OFS {
	UINT32 line_ofs;                ///< line offset for multi-stripe mode or image mode
	UINT32 channel_ofs;             ///< channel offst for multi-stripe mode
	UINT32 batch_ofs;
} NN_DIFF_OFS;

typedef struct _NN_DIFF_PAD_PARM {
	UINT8 top_pad_num;              ///< number of padding top pixels
	UINT8 bot_pad_num;              ///< number of padding bottom pixels
	UINT8 left_pad_num;             ///< number of padding left pixels
	UINT8 right_pad_num;            ///< number of padding right pixels
} NN_DIFF_PAD_PARM;

typedef struct _NN_DIFF_PARM {

	UINT32 stripe_inaddr[3];
	UINT32 stripe_outaddr[3];
	UINT32 in_width;      
	UINT32 in_height;
	UINT32 batch; 
    UINT32 channel; 
	NN_DIFF_OFS in_ofs[3];
	NN_DIFF_OFS out_ofs[3];
	NN_DIFF_PAD_PARM conv_pad;
	NN_DIFF_PAD_PARM deconv_pad; 
	NN_DIFF_PAD_PARM pool_pad;
	BOOL h_stripe_en;
	BOOL v_stripe_en;
	BOOL skip_en;
	BOOL is_top_pad;
	BOOL is_bot_pad;
	BOOL is_left_pad;
	BOOL is_right_pad;
	UINT32 tc_width;
	UINT32 tc_height;
	UINT32 sca_width;
	UINT32 sca_height;
	UINT32 mean_width;
	UINT32 mean_height;
	UINT32 pad_out_width; 
    UINT32 pad_out_height;
#if !CNN_25_MATLAB
	UINT32 process_cycle;
#endif
}NN_DIFF_PARM;

/*
nvt_diff_model

////////////////////// /////////////////////
//NN_DIFF_MODEL_HEAD// //NN_DIFF_MODE_CTRL//
////////////////////// /////////////////////

//diff model 1
//////////////////  ////////////////  ////////////////// 
// NN_DIFF_HEAD //  //  NN_IOMEM  //  // NN_DIFF_PARM // 
//////////////////  ////////////////  ////////////////// 

//diff model 2
......



*/

ER nvt_ai_update_net_online(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, NN_DIFF_MODEL_HEAD *p_model_head, UINT32 model_id, UINT32 net_id);
ER nvt_ai_restore_net_online(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, NN_DIFF_MODEL_HEAD *p_model_head, UINT32 model_id, UINT32 net_id);
#endif 

