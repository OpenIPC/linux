
#ifndef _VHE_HDLR_H_
#define _VHE_HDLR_H_

#include <mdrv_mvhe_st.h>
#include <mhve_ops.h>

#include <vhe_defs.h>
#include <vhe_hevc.h>
#include <vhe_regs.h>
#include <vhe_asic.h>
#include <vhe_rctl.h>

typedef struct {
    short   b_valid;
    short   i_index;
  #define VHE_PIXELY    0
  #define VHE_PIXELC    1
  #define VHE_LUMA4N    2
  #define VHE_COMPRY    3
  #define VHE_PLANES    4
    uint    u_planes[VHE_PLANES];
} rpb_t;

#define VHE_ALIGN(i,b)  (((i)+((1<<(b))-1))&(~((1<<(b))-1)))

#define MAX_DPB_NR      4

typedef struct vhe_hndl vhe_hndl;

struct vhe_hndl {
    mhve_ops       ops;
    rqct_ops*   p_rqct;
    hevc_enc*   p_hevc;
    vhe_mirr*   p_mirr;
    /* current resource */
    mhve_pix_e  e_pixf;
    short       i_pixw, i_pixh;
    int         i_rpbn;
    uint        u_conf;
    /* video-picture-buffer */
    mhve_vpb    m_encp;
    int         i_seqn;     /* seq_cnt: 0->force idr, others: ... */
    int         b_seqh;
    /* output bitstream */
    void*       p_otpt;
    uint        u_otbs;
    uint        u_otsz;
    uint        u_otrm;
    uint        u_used;
    uint        u_size;
    /*!output bitstream */
    uint*       p_nalp;
    uint        u_nalb;
    uint        u_coef;
    rpb_t       m_rpbs[MAX_DPB_NR];
};

void* mvheops_acquire(int);

#endif/*_VHE_HDLR_H_*/
