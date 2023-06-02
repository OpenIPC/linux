/**
    @brief Header file of common definition of vendor net flow sample.

    @file net_common.h

    @ingroup net_flow_sample

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NET_COMMON_H_
#define _NET_COMMON_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#if defined(__LINUX)
#include <linux/kernel.h>   // for snprintf
#elif defined(__FREERTOS)
#include <stdio.h>          // for snprintf
#else
#include "kwrap/type.h"     // for snprintf
#endif

/********************************************************************
    MACRO CONSTANT DEFINITIONS
********************************************************************/
#define DBG_MSG_DUMP            0
#define STR_MAX_LENGTH          256
#define LL_BASE_TEST        	DISABLE // for ll_base test

#define NN_IMEM_NUM             8
#define NN_OMEM_NUM             3

/********************************************************************
    TYPE DEFINITION
********************************************************************/
typedef enum {
	BUF_IN_IDX = 0,
	BUF_PARM_IDX0 = 1,
	BUF_INTERM_IDX,
	BUF_PARM_IDX1,
	BUF_PARM_IDX2,

	BUF_TOTAL_IDX_INMEM,
	ENUM_DUMMY4WORD(BUF_INMEM_IDX_TYPE)
} BUF_INMEM_IDX_TYPE;

typedef enum {
	BUF_OUT_IDX0 = 0,
	BUF_OUT_IDX1 = 1,

	BUF_TOTAL_IDX_OUTMEM,
	ENUM_DUMMY4WORD(BUF_OUTMEM_IDX_TYPE)
} BUF_OUTMEM_IDX_TYPE;

typedef enum {
	CNN_IN_IDX0 = BUF_IN_IDX,
	CNN_IN_IDX1 = 1,
	CNN_IN_IDX2 = 2,
	//RESERVED0,
	//RESERVED1,
	//RESERVED2,
	CNN_WEIGHT_IDX = 3,
	CNN_KMEAN_IDX,
	CNN_BIAS_BNSCL_IDX,
	CNN_INTERM_IDX,
	CNN_LL_IDX,

	CNN_TOTAL_IDX_INMEM,
	ENUM_DUMMY4WORD(CNN_INMEM_IDX_TYPE)
} CNN_INMEM_IDX_TYPE;

typedef enum {
	CNN_OUT_IDX0 = BUF_OUT_IDX0,
	CNN_OUT_IDX1 = 1,

	CNN_TOTAL_IDX_OUTMEM,
	ENUM_DUMMY4WORD(CNN_OUTMEM_IDX_TYPE)
} CNN_OUTMEM_IDX_TYPE;

typedef enum {
	NUE_IN_IDX = BUF_IN_IDX,
	NUE_ELT_IDX = 1,
	NUE_SV_IDX,
	NUE_ALPHA_IDX,
	NUE_RHO_IDX,
	NUE_ROI_IDX,
	NUE_LL_IDX,
	NUE_KMEAN_IDX,

	NUE_TOTAL_IDX_INMEM,
	ENUM_DUMMY4WORD(NUE_INMEM_IDX_TYPE)
} NUE_INMEM_IDX_TYPE;

typedef enum {
	NUE_OUT_IDX = BUF_OUT_IDX0,

	NUE_TOTAL_IDX_OUTMEM,
	ENUM_DUMMY4WORD(NUE_OUTMEM_IDX_TYPE)
} NUE_OUTMEM_IDX_TYPE;

typedef enum {
	NUE2_IN_IDX0 = BUF_IN_IDX,
	NUE2_IN_IDX1 = 1,
	NUE2_IN_IDX2 = 2,

	NUE2_TOTAL_IDX_INMEM,
	ENUM_DUMMY4WORD(NUE2_INMEM_IDX_TYPE)
} NUE2_INMEM_IDX_TYPE;

typedef enum {
	NUE2_OUT_IDX0 = BUF_OUT_IDX0,
	NUE2_OUT_IDX1 = 1,
	NUE2_OUT_IDX2 = 2,

	NUE2_TOTAL_IDX_OUTMEM,
	ENUM_DUMMY4WORD(NUE2_OUTMEM_IDX_TYPE)
} NUE2_OUTMEM_IDX_TYPE;

//========== for first layer linked list mode ==========
typedef struct {
	unsigned : 32;
	unsigned : 32;
} ENG_NULL_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned addr : 32;
	} bit;
} ENG_ADDR_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned ofs : 29;
	} bit;
} ENG_OFS_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned width : 10;
		unsigned : 2;
		unsigned height : 10;
	} bit;
} CNN_SIZE0_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned channel : 12;
		unsigned batch_num : 7;
	} bit;
} CNN_SIZE1_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned area : 14;
		unsigned : 2;
		unsigned channel : 12;
	} bit;
} NUE_SIZE1_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned obj_num : 7;
		unsigned : 1;
		unsigned roi_num : 7;
	} bit;
} NUE_SIZE2_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned width : 11;
		unsigned : 5;
		unsigned height : 11;
	} bit;
} NUE2_SIZE0_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned ofs : 17;
	} bit;
} NUE2_LOFS_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned h_rate : 7;
		unsigned : 1;
		unsigned v_rate : 7;
	} bit;
} NUE2_SCL0_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned h_sfact : 16;
		unsigned v_sfact : 16;
	} bit;
} NUE2_SCL1_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned h_scl_size : 11;
		unsigned : 5;
		unsigned v_scl_size : 11;
	} bit;
} NUE2_SCL_SIZE_REG;

typedef struct _CNN_LL_PARM {
	ENG_ADDR_REG input;
	ENG_ADDR_REG interm_in;
	ENG_ADDR_REG output[2];
	ENG_ADDR_REG weight;
	ENG_ADDR_REG kmean;
	ENG_ADDR_REG bias_bnscal;
	CNN_SIZE0_REG size0;
	CNN_SIZE1_REG size1;
} CNN_LL_PARM;

typedef struct _NUE_LL_PARM {
	ENG_ADDR_REG input;
	ENG_ADDR_REG elt_in;
	ENG_ADDR_REG roi_in;
	ENG_ADDR_REG output;
	ENG_ADDR_REG sv;
	ENG_ADDR_REG alpha;
	ENG_ADDR_REG rho;
	ENG_ADDR_REG kmean;
	ENG_NULL_REG reserved[2];
	NUE_SIZE1_REG size1;
	NUE_SIZE2_REG size2;
} NUE_LL_PARM;

typedef struct _NUE2_LL_PARM {
	ENG_ADDR_REG input[3];
	ENG_ADDR_REG output[3];
	NUE2_SIZE0_REG size0;
	NUE2_LOFS_REG ilofs[3];
	NUE2_LOFS_REG olofs[3];
	NUE2_SCL0_REG scale0;
	NUE2_SCL1_REG scale1;
	NUE2_SCL_SIZE_REG scale_size;
} NUE2_LL_PARM;
/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
//int nn_write_file(char *filename, void *data, size_t count, unsigned long long *offset);
//int nn_read_file(char *filename, void *data, size_t count, unsigned long long *offset);
//UINT32 get_mid_val(UINT32 *p_times, UINT32 length);

#endif  /* _NET_COMMON_H_ */
