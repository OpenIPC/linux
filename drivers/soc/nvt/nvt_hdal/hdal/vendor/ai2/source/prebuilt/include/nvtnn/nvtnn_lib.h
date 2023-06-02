/**
    @brief Header file of NvtNN library definitions.

    @file nvtnn_lib.h

    @ingroup nvtnn

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/
#ifndef _NVTNN_LIB_H_
#define _NVTNN_LIB_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_ai_net/nn_parm.h"

/********************************************************************
    TYPE DEFINITIONS
********************************************************************/
typedef NN_SOFTMAX_PARM     NVTNN_SOFTMAX_PRMS;
typedef NN_PRELU_PARM       NVTNN_PRELU_PRMS;
typedef NN_PRIORBOX_PARM    NVTNN_PRIORBOX_PRMS;
typedef NN_DETOUT_PARM      NVTNN_DETOUT_PRMS;
typedef NN_LRN_PARM         NVTNN_LRN_PRMS;
typedef NN_POOL_PARM        NVTNN_POOL_PRMS;
typedef NN_FC_POST_PARM     NVTNN_FC_POST_PRMS;
typedef NN_LSTM_PARM        NVTNN_LSTM_PRMS;
typedef NN_PERMUTE_PARM     NVTNN_PERMUTE_PRMS;
typedef NN_REVERSE_PARM     NVTNN_REVERSE_PRMS;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


extern const CHAR *prebuilt_ai_get_lib_version(VOID);

/* Float to fix */
extern VOID floattoint8_c(FLOAT *input_data, INT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattouint8_c(FLOAT *input_data, UINT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattoint16_c(FLOAT *input_data, INT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattouint16_c(FLOAT *input_data, UINT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattouint16_neon(FLOAT *input_data, UINT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattoint16_neon(FLOAT *input_data, INT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattouint8_neon(FLOAT *input_data, UINT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattoint8_neon(FLOAT *input_data, INT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);

/* Fix to float */
extern VOID int8tofloat_c(INT8 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID uint8tofloat_c(UINT8 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID int16tofloat_c(INT16 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID uint16tofloat_c(UINT16 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID uint16tofloat_neon(UINT16 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID int16tofloat_neon(INT16 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID int8tofloat_neon(INT8 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID uint8tofloat_neon(UINT8 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);

/* Permute layer */
extern void permute_data_out(INT8* p_in, INT8* p_out, UINT32  stride, UINT32 num);
extern void permute_data_in(INT8* p_in, INT8* p_out, UINT32  stride, UINT32 num);

/* Vision layers */
extern HD_RESULT nvtnn_pool_process(NVTNN_POOL_PRMS *p_prms);

/* Activation layers */
extern HD_RESULT nvtnn_prelu_process(NVTNN_PRELU_PRMS *p_prms);

/* Utility layers */
extern HD_RESULT nvtnn_softmax_process(NVTNN_SOFTMAX_PRMS *p_prms);
extern HD_RESULT nvtnn_lrn_process(NVTNN_LRN_PRMS *p_prms);
extern HD_RESULT nvtnn_lstm_process(NVTNN_LSTM_PRMS *p_prms);
extern HD_RESULT nvtnn_permute_process(NVTNN_PERMUTE_PRMS *p_prms);
extern HD_RESULT nvtnn_reverse_process(NVTNN_REVERSE_PRMS *p_prms);

/* Post-processing layers */
extern HD_RESULT nvtnn_priorbox_process(NVTNN_PRIORBOX_PRMS *p_prms);
extern HD_RESULT nvtnn_detout_process(NVTNN_DETOUT_PRMS *p_prms);
extern HD_RESULT nvtnn_fc_post_process(NVTNN_FC_POST_PRMS *p_prms);


#ifdef __cplusplus
}
#endif

#endif  /* _NVTNN_LIB_H_ */
