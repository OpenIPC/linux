#ifndef _NVT_DIS_API_H_
#define _NVT_DIS_API_H_

#include "nvt_dis.h"

/**
    Get the working buffer size that nvt_dis need
    @return UINT32: the working buffer size that nvt_dis need.
*/
extern UINT32	 nvt_dis_cal_buffer_size(void);
/**
    init buff size need by nvt_dis, and enable dis_eth.
    @param p_buf: the address for buff.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
extern HD_RESULT nvt_dis_init(DIS_IPC_INIT *p_buf);
/**
    uninit nvt_dis.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
extern HD_RESULT nvt_dis_uninit(void);

/**
    process the nvt_dis without compensation using edgemap by eth from kflow.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
extern HD_RESULT nvt_dis_only_calc_vector(void);

#endif
