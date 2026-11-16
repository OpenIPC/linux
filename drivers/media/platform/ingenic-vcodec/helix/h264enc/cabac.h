#ifndef __CABAC_H__
#define __CABAC_H__

#include "common.h"

typedef struct
{
    /* state */
    int i_low;
    int i_range;

    uint8_t *p_start;
    uint8_t *p;
    uint8_t *p_end;

    /* context */
    uint8_t state[1024];

} h264_cabac_t;

extern void h264_cabac_init(void);
extern void h264_cabac_context_init(h264_cabac_t *cb, int i_slice_type, int i_qp, int i_model );





#endif
