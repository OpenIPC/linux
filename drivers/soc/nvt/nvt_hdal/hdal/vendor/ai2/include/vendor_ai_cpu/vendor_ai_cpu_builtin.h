/**
    Public header file for vendor_ai_cpu

    This file is the header file that defines the API and data types for vendor_ai_cpu.

    @file       vendor_ai_cpu.h
    @ingroup    vendor_ai_cpu

    Copyright   Novatek Microelectronics Corp. 2018.    All rights reserved.
*/
#ifndef _VENDOR_AI_CPU_BUILTIN_H_
#define _VENDOR_AI_CPU_BUILTIN_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"
#include "nvtnn/nvtnn_lib.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/
/**
	Data dimensions.
*/
typedef struct _VENDOR_AIS_SHAPE {
	INT32 num;                          ///< Caffe blob: batch size
	INT32 channels;                     ///< Caffe blob: number of channels
	INT32 height;                       ///< Caffe blob: height
	INT32 width;                        ///< Caffe blob: width
	INT32 lineofs;                      ///< lineoffset: distance between two lines (rows); unit: byte
} VENDOR_AIS_SHAPE;

/**
    Output class.
*/
typedef struct _VENDOR_AIS_OUTPUT_CLASS {
	INT32 no;                           ///< class number
	FLOAT score;                        ///< class score
} VENDOR_AIS_OUTPUT_CLASS;

/**
	Parameters of accuracy calculation
*/
typedef struct _VENDOR_AIS_ACCURACY_PARM {
#if CNN_25_MATLAB
	INT16 *input;                       ///< [in]  address of input data
#else
	UINT32 in_addr;                     ///< [in]  address of input data
#endif
	VENDOR_AIS_OUTPUT_CLASS *classes;   ///< [out] top scoring class list

	VENDOR_AIS_SHAPE shape;             ///< [in]  input/output data dimensions (height/width/lineofs is not used)
	INT32 top_n;                        ///< [in/out] number of top scoring classes

	INT32 *class_idx;                   ///< [in]  address of class index buffer
} VENDOR_AIS_ACCURACY_PARM;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_ais_accuracy_process(VENDOR_AIS_ACCURACY_PARM *p_parm);

extern HD_RESULT vendor_ais_softmax_process(NN_SOFTMAX_PARM  *p_parm);

extern HD_RESULT vendor_ai_bnscale_process(NN_BNSCALE_PARM *p_parm);

extern HD_RESULT vendor_ai_pool_process   (NN_POOL_PARM    *p_parm);

extern HD_RESULT vendor_ai_fc_post_process(NN_FC_POST_PARM *p_parm);

#endif //_VENDOR_AI_CPU_BUILTIN_H_
