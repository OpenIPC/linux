
#ifndef _H2V3_RQC_H_
#define _H2V3_RQC_H_

typedef struct h2v3_rqc h2v3_rqc;

struct h2v3_rqc
{
    /* rqct_ops MUST be the first element */
    rqct_ops rqct;
    rqct_att attr;

    int i_iperiod;      //I-frame/P-frame period
    int i_ltrperiod;    //LTR P-frame / P-frame period
    int i_ipcount;      //I-frame/P-frame count up to period
    int i_ltrpcount;    //LTR-frame/P-frame count up to period
    int i_refcnt;       // ref count from I-frame
    int i_config;
    int i_rcsize;
    unsigned int u_rcmcur;
    unsigned int u_rcmpre;
    char* p_rcmcur;
    char* p_rcmpre;
#define RQC_ROIA_ENABLE (1<<0)
#define RQC_ROIM_ENABLE (1<<1)
#define RQC_DQRC_ENABLE (1<<2)
#define RQC_ROIX_BIT    (1<<4)
    int b_rqcbit;
};

//#define QP_FRAC_BITS        8
#define QP_DEF              32
#define QP_MAX              51
#define QP_MIN              10
#define QP_DRANGE           10
#define QP_IFRAME_DELTA     2
#define DQP_MAX             12
#define DQP_MIN             -12

void* rqctvhe_acquire(int);
char* rqctvhe_comment(int);

int hrqc_set_conf(rqct_ops* rqct, rqct_cfg* conf);
int hrqc_get_conf(rqct_ops* rqct, rqct_cfg* conf);
int hrqc_enc_buff(rqct_ops* rqct, rqct_buf* buff);
int hrqc_enc_conf(rqct_ops* rqct, mhve_job* mjob);

/* dynamical kernel module support */
int rqctvhe_insert(void* (*allc)(void), char*(*desc)(void));
int rqctvhe_remove(void);

#endif/*_H2V3_RQC_H_*/
