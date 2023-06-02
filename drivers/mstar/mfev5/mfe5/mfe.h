
#ifndef _MFE_H_
#define _MFE_H_

#include <mdrv_mmfe_st.h>
#include <mhve_ops.h>

#include <mfe_defs.h>
#include <mfe_h264.h>
#include <mfe_regs.h>
#include <mfe_asic.h>
#include <mfe_rctl.h>

typedef struct rpb_t {
    char    b_valid;        /* 1: allocated direct memory. 0: use pic+'s memory */
  #define RPB_STATE_FREE     0
  #define RPB_STATE_BUSY     1
  #define RPB_STATE_INVL    -1
  #define IS_FREE(s)        ((s)==RPB_STATE_FREE)
  #define IS_INVL(s)        ((s)==RPB_STATE_INVL)
    char    i_state;
    short   i_index;
  #define RPB_YPIX           0
  #define RPB_CPIX           1
  #define RPB_SIZE           2
    uint    u_phys[RPB_SIZE];
} rpb_t;

typedef struct mfe_hndl {
    mhve_ops       ops;
    /* mfe-registers */
    mfe_regs*   p_regs;
    /* rate control */
    rqct_ops*   p_rqct;
    /* seq counter */
    mhve_vpb    m_encp;
    int         i_seqn;
    int         b_seqh;
    /* current resource */
    mhve_pix_e  e_pixf;
    short       i_pixw, i_pixh;
    int         i_rpbn;
    uint        u_conf;
    short       i_pctw, i_pcth;
    short       i_mbsw, i_mbsh;
    int         i_mbsn;
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
    /* freeback and statistic */
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
  #define RPB_MAX_NR    4
    int         i_refn;
    rpb_t*      p_recn;
    rpb_t*      m_dpbs[RPB_MAX_NR];
    rpb_t       m_rpbs[RPB_MAX_NR];
    /* fragment of bits */
    /* sequence header */
    int         i_seqh;
    uchar       m_seqh[32];
    /* picture header */
    int         i_pich;
    uchar       m_pich[32];
} mfe_hndl;

#endif//_MFE_H_
