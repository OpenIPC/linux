
#ifndef _VHE_RCTL_H_
#define _VHE_RCTL_H_

#include <rqct_ops.h>

#define H2V1_ROI_PATCH 

typedef struct vhe_rctl vhe_rctl;

struct vhe_rctl {
    rqct_ops    rqct;
    rqct_att    attr;
#if defined(H2V1_ROI_PATCH)
    int         i_coin;
    struct {
    short   cir, lft;
    short   top, bot;
    }           i_edge[2];
#endif
    int         i_period;
    int         i_pcount;
  #define RCTL_CFG_CTB_DQP     (1<<0)
  #define RCTL_CFG_NIGHT       (1<<2)
    int         i_config;
};

void* rqctvhe_acquire(int);
char* rqctvhe_comment(int);

/* basic functions of vhe rate-controller */
int vrqc_set_rqcf(rqct_ops* rqct, rqct_cfg* rqcf);
int vrqc_get_rqcf(rqct_ops* rqct, rqct_cfg* rqcf);
int vrqc_enc_buff(rqct_ops* rqct, rqct_buf* buff);
int vrqc_enc_conf(rqct_ops* rqct, mhve_job* mjob);
/* functions to insert/remove additional rate-controller */
int rqctvhe_insert(void*(*allc)(void), char*(*desc)(void));
int rqctvhe_remove(void);

#endif/*_VHE_RCTL_H_*/
