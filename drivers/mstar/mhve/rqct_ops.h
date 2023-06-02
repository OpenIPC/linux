
// mstar hardware video encoder I/F (mhveif)

#ifndef _RQCT_OPS_H_
#define _RQCT_OPS_H_

#include <rqct_cfg.h>

typedef struct rqct_ops rqct_ops;

struct rqct_ops
{
    char name[32];
    void (*release)(rqct_ops* rqct);
    int (*seq_sync)(rqct_ops* rqct);
    int (*seq_done)(rqct_ops* rqct);
    int (*set_rqcf)(rqct_ops* rqct, rqct_cfg* rqcf);
    int (*get_rqcf)(rqct_ops* rqct, rqct_cfg* rqcf);
    int (*seq_conf)(rqct_ops* rqct);
    int (*enc_buff)(rqct_ops* rqct, rqct_buf* buff);
    int (*enc_conf)(rqct_ops* rqct, mhve_job* mjob);
    int (*enc_done)(rqct_ops* rqct, mhve_job* mjob);
#define RQCT_LINE_SIZE    128
    char print_line[RQCT_LINE_SIZE];
    enum
    {
        RQCT_PICTYP_I=0,
        RQCT_PICTYP_P,
        RQCT_PICTYP_B,
        RQCT_PICTYP_LTRP,           // This frame is LTR P-frame
    } i_pictyp;
#define IS_IPIC(t)    (RQCT_PICTYP_I==(t))
#define IS_PPIC(t)    (RQCT_PICTYP_P==(t))
#define IS_BPIC(t)    (RQCT_PICTYP_B==(t))
#define IS_LTRPPIC(t) (RQCT_PICTYP_LTRP==(t))
    int i_enc_qp;
    int b_unrefp;                   // used for disposable unreferenced frame
    int i_bitcnt;
    int i_enc_nr;
    int i_enc_bs;
};

typedef struct rqct_att
{
    short   i_pict_w, i_pict_h;
    short   n_fmrate, d_fmrate;
    short   i_method, i_iperiod;    // i_method:CBR/VBR; i_iperiod:GoP
    short   i_ltrperiod;            // LTR P-frame period
    short   i_leadqp, i_deltaq;
    short   i_iupperq, i_ilowerq;
    short   i_pupperq, i_plowerq;
    int     i_btrate;
    int     b_logoff;
    short   b_i16pln, i_peni16;
    short   i_peni4x, i_penint;
    short   i_penYpl, i_penCpl;
#define RQCT_ROI_NR     8
    signed char     i_roidqp[RQCT_ROI_NR];
    struct roirec
    {
        short   i_posx, i_posy;
        short   i_recw, i_rech;
    }               m_roirec[RQCT_ROI_NR];
    short           i_dqmw, i_dqmh;
    int             b_dqmstat;
    int             i_dqmunit;
    void*           p_dqmkptr;
    unsigned int    u_dqmphys;
    unsigned int    i_dqmsize;
} rqct_att;

#endif//_RQCT_OPS_H_
