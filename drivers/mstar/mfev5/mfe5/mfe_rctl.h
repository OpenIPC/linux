
#ifndef _MFE_RCTL_H_
#define _MFE_RCTL_H_

#include <rqct_ops.h>

typedef struct mfe_rctl mfe_rctl;

struct mfe_rctl {
    rqct_ops    rqct;
    rqct_att    attr;
    int         i_period;
    int         i_pcount;
  #define RCTL_CFG_MB_DQP      (1<<0)
  #define RCTL_CFG_NIGHT       (1<<2)
    int         i_config;
};

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
