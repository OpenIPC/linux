
#ifndef _MFE_RCTL_H_
#define _MFE_RCTL_H_

#include <rqct_ops.h>

typedef struct mfe6_rqc
{
    rqct_ops    rqct;
    rqct_att    attr;
    int         i_iperiod;      //I-frame/P-frame period
    int         i_ltrperiod;    //LTR P-frame / P-frame period
    int         i_ipcount;      //I-frame/P-frame count up to period
    int         i_ltrpcount;    //LTR-frame/P-frame count up to period
#define RCTL_CFG_MB_DQP      (1<<0)
#define RCTL_CFG_NIGHT       (1<<2)
    int         i_refcnt;
    int         i_config;
} mfe6_rqc;

#define QP_UPPER        (48)
#define QP_LOWER        (12)
#define QP_RANGE        (QP_UPPER-QP_LOWER)
#define QP_MAX          (51)
#define QP_MIN          (10)
#define QP_IFRAME_DELTA (-2)     //I-frame QP delta from P-frame
#define DQP_MAX         12
#define DQP_MIN         -12

void* rqctmfe_acquire(int);
char* rqctmfe_comment(int);

/* basic functions of mfe rate-controller */
int mrqc_set_rqcf(rqct_ops* rqct, rqct_cfg* rqcf);
int mrqc_get_rqcf(rqct_ops* rqct, rqct_cfg* rqcf);
int mrqc_enc_buff(rqct_ops* rqct, rqct_buf* buff);
int mrqc_roi_draw(rqct_ops* rqct, mhve_job* mjob);
/* functions to insert/remove additional rate-controller */
int rqctmfe_insert(void*(*allc)(void), char*(*desc)(void));
int rqctmfe_remove(void);

#endif/*_MFE_RCTL_H_*/
