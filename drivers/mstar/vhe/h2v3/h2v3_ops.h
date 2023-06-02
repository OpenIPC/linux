#ifndef _H2V3_OPS_H_
#define _H2V3_OPS_H_

typedef struct rpb_t rpb_t;

struct rpb_t
{
    short   b_valid;            /* 1: allocated direct memory. 0: use pic+'s memory */
    short   i_index;
    char*   p_virt;
#define RPB_YPIXEL  0
#define RPB_CPIXEL  1
#define RPB_LUMA4N  2
#define RPB_YCOMPR  3
#define RPB_CCOMPR  4
#define RPB_PLANES  5
    uint    u_planes[RPB_PLANES];
    uint    u_buf_size[RPB_PLANES];
};

#define MAX_RPB_NR  MAX_REF_SIZE

typedef struct h2v3_ops h2v3_ops;

struct h2v3_ops
{
    /* mhve_ops MUST be the first element */
    mhve_ops    mops;
    rqct_ops*   rqct;
    h2v3_mir*   mirr;
    h265_enc*   h265;
    /* current resource */
    mhve_pix_e  e_pixf;
    short       i_pixw, i_pixh;
    int         i_rpbn;             // reconstruct buffer numbers
    rpb_t       m_rpbs[MAX_RPB_NR];
    rpb_t       m_rpb_ltr;
    uint        u_conf;
    /* video(input) picture buffer */
    mhve_vpb    m_encp;
    int         i_seqn;             /* seq_cnt: 0->force idr, others: ... */
#define OPS_SEQH_START  (1<<2)
#define OPS_SEQH_RESET  (1<<1)
#define OPS_SEQH_WRITE  (1<<0)
    int         b_seqh;
    /* output bitstream */
    void*       p_otpt;             // current buffer virtual address pointer
    uint        u_otbs;             // current buffer physical start address
    uint        u_otsz;             // current buffer size
    uint        u_otrm;             // seems unused
    uint        u_used;             // buffer used size
    uint        u_size;             // hw encode size
    int         i_bias;             // padding length for real data(sw hack after encode done)
    /* other attributes */
    uint*       p_nalp;
    uint        u_nalb;
    uint        u_coef;             // Base address for compressed coefficients and 0 mean not use compressed buffer
};

#endif/*_H2V3_OPS_H_*/
