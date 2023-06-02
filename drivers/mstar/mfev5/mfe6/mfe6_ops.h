#ifndef _MFE6_OPS_H_
#define _MFE6_OPS_H_

#include <mhve_ops.h>

#include <mfe6_def.h>
#include <h264_enc.h>
#include <mfe6_reg.h>
#include <mfe6_ios.h>
#include <mfe6_rqc.h>

typedef struct rpb_t
{
   char    b_valid;                 /* 1: allocated direct memory. 0: use pic+'s memory */
#define RPB_STATE_FREE      0
#define RPB_STATE_BUSY      1
#define RPB_STATE_KEEP      2       // Identify this recn buffer is used to LTR reference picture
#define RPB_STATE_INVL      -1
#define IS_FREE(s)          ((s)==RPB_STATE_FREE)
#define IS_BUSY(s)          ((s)==RPB_STATE_BUSY)
#define IS_KEEP(s)          ((s)==RPB_STATE_KEEP)
#define IS_INVL(s)          ((s)==RPB_STATE_INVL)
    char    i_state;
    short   i_index;                /* only used in case b_valid is 0 that can keep cache buffer index */
#define RPB_YPIX            0
#define RPB_CPIX            1
#define RPB_SIZE            2
    uint    u_phys[RPB_SIZE];
} rpb_t;

typedef enum AVC_REF_LIST
{
    AVC_REF_L0  = 0,
    AVC_REF_L1  = 1,
    AVC_REF_LTR = 2,
    AVC_REF_MAX = 3,
} AVC_REF_LIST;

typedef struct mfe6_ops
{
    mhve_ops    ops;
    /* mfe-registers */
    mfe6_reg*   p_regs;
    /* rate control */
    rqct_ops*   p_rqct;
    /* seq counter */
    mhve_vpb    m_encp;             // video picture buffer
    int         i_seqn;
    int         b_seqh;             // sequence header
    /* current resource */
    mhve_pix_e  e_pixf;
    short       i_pixw, i_pixh;
#define RPB_MAX_NR    4
    int         i_rpbn;             // reconstruct buffer numbers
    rpb_t       m_rpbs[RPB_MAX_NR];
    uint        u_conf;
    short       i_pctw, i_pcth;     // align 16 pixels width/height
    short       i_mbsw, i_mbsh;     // MB level width/height
    int         i_mbsn;             // marco block number
    /* motion search */
    int         i_dmvx;
    int         i_dmvy;
    int         i_subp;
    uint        i_blkp[2];
    /* h264_enc */
    sps_t       m_sps;
    pps_t       m_pps;
    slice_t     m_sh;
    /* multi-slices */
    int         i_rows;
    int         i_bits;
    /* feedback and statistic */
    int         i_total;
    int         i_obits;
    /* output bits */
    char*       p_okptr;
    uint        u_obase;
    uint        u_osize;
    uint        u_obias;
    uint        u_oused;
    uint        u_vacan;
    /*!output bits */
    uint        u_mbp_base;
    /* frame buffers */
    int         i_refn;             // reference frame numbers (1 or 2(multi-ref mode))
    rpb_t*      p_recn;             // reconstruct buffer header
    rpb_t*      m_dpbs[AVC_REF_MAX][RPB_MAX_NR];
    /* fragment of bits */
    /* sequence header */
    int         i_seqh;
    uchar       m_seqh[64];
    /* picture header */
    int         i_pich;
    uchar       m_pich[32];
    /* LTR */
    int         b_enable_pred;      // 0: P ref. I-frame, 1: P ref P-frame
} mfe6_ops;

#endif//_MFE6_OPS_H_
