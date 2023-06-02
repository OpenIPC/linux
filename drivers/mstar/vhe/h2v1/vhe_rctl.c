
#include <mdrv_mvhe_st.h>

#include <vhe.h>

#define VRQC_NAME       "VRQC"
#define VRQC_VER_MJR    0
#define VRQC_VER_MNR    0
#define VRQC_VER_EXT    02

#define _EXP(expr)      #expr
#define _STR(expr)      _EXP(expr)

void* vcbr_allocate(void);
char* vcbr_describe(void);
void* vcb2_allocate(void);
char* vcb2_describe(void);
void* vsbr_allocate(void);
char* vsbr_describe(void);
void* vsb3_allocate(void);
char* vsb3_describe(void);

static void* vrqc_allocate(void);
static char* vrqc_describe(void);

#define VRQC_FACTORIES_NR   8

static struct vrct_factory {
    void* (*rqc_alloc)(void);
    char* (*rqc_descr)(void);
} factories[VRQC_FACTORIES_NR] =
{
{vrqc_allocate,vrqc_describe},
{vcbr_allocate,vcbr_describe},
{vcb2_allocate,vcb2_describe},
{vsbr_allocate,vsbr_describe},
{vsb3_allocate,vsb3_describe},
{NULL,NULL},
};

int rqctvhe_insert(void* (*allc)(void), char*(*desc)(void))
{
    if (factories[0].rqc_alloc != vrqc_allocate)
        return -1;
    factories[0].rqc_alloc = allc;
    factories[0].rqc_descr = desc;
    return 0;
}
EXPORT_SYMBOL(rqctvhe_insert);

int rqctvhe_remove(void)
{
    if (factories[0].rqc_alloc == vrqc_allocate)
        return -1;
    factories[0].rqc_alloc = vrqc_allocate;
    factories[0].rqc_descr = vrqc_describe;
    return 0;
}
EXPORT_SYMBOL(rqctvhe_remove);

void* rqctvhe_acquire(int i)
{
    if ((unsigned)i < VRQC_FACTORIES_NR && factories[i].rqc_alloc)
        return (factories[i].rqc_alloc)();
    return (factories[0].rqc_alloc)();
}

char* rqctvhe_comment(int i)
{
    if ((unsigned)i < VRQC_FACTORIES_NR && factories[i].rqc_descr)
        return (factories[i].rqc_descr)();
    return NULL;
}

/* defsult rate-controller of VHE */

#define LOGOFF_DEFAULT  1

static int   _seq_init(rqct_ops*);
static int   _seq_done(rqct_ops*);
static int   _seq_conf(rqct_ops*);
static int   _enc_done(rqct_ops* rqct, mhve_job* mjob);
static void _vrqc_free(rqct_ops* rqct) { MEM_FREE(rqct); }

static void* vrqc_allocate(void)
{
    rqct_ops* rqct = NULL;
    vhe_rctl* rqcx;
    if (!(rqct = MEM_ALLC(sizeof(vhe_rctl))))
        return rqcx;
    MEM_COPY(rqct->name, "vrqc", 5);
    rqct->release = _vrqc_free;
    rqct->seq_sync = _seq_init;
    rqct->seq_done = _seq_done;
    rqct->seq_conf = _seq_conf;
    rqct->set_rqcf = vrqc_set_rqcf;
    rqct->get_rqcf = vrqc_get_rqcf;
    rqct->enc_buff = vrqc_enc_buff;
    rqct->enc_conf = vrqc_enc_conf;
    rqct->enc_done = _enc_done;
    rqct->i_enc_nr = 0;
    rqct->i_enc_bs = 0;
    rqcx = (vhe_rctl*)rqct;
    rqcx->attr.i_method = RQCT_METHOD_CQP;
    rqcx->attr.i_pict_w = 0;
    rqcx->attr.i_pict_h = 0;
    rqcx->attr.n_fmrate =30;
    rqcx->attr.d_fmrate = 1;
    rqcx->attr.i_leadqp =-1;
    rqcx->attr.i_deltaq = 3;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.b_logoff = LOGOFF_DEFAULT;
    rqcx->attr.b_dqmstat=-1;
    rqcx->i_config = 0;
    rqcx->i_period = 0;
    rqcx->i_pcount = 0;
    return rqct;
}

static char* vrqc_describe(void)
{
    static char line[64];
    sprintf(line, "%s@v%d.%d-%02d:basic rq-controller.",VRQC_NAME,VRQC_VER_MJR,VRQC_VER_MNR,VRQC_VER_EXT);
    return line;
}

static int  _seq_conf(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    if (rqcx->attr.i_leadqp < 0)
        rqcx->attr.i_leadqp =36;
    rqcx->i_pcount =
    rqcx->i_period = rqcx->attr.i_period;
    return 0;
}

static int  _seq_init(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    rqcx->i_pcount = 0;
    return 0;
}

static int  _seq_done(rqct_ops* rqct)
{
    MVHE_PRINT("[%s]%8d frames encoded\n",rqct->name,rqct->i_enc_nr);
    MVHE_PRINT("[%s]%8d  bytes encoded\n",rqct->name,rqct->i_enc_bs);
    return 0;
}

static int  _enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;
    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d",rqct->name,rqct->i_enc_nr,IS_IPIC(rqct->i_pictyp)?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt);
    return 0;
}

static unsigned short intra_penalty_tu_4[52] = {   /* Intra Penalty for TU 4x4 vs. 8x8 */
   7,   7,   8,  10,  11,  12,  14,  15,  17,  20,  22,  25,  28,
  31,  35,  40,  44,  50,  56,  63,  71,  80,  89, 100, 113, 127,
 142, 160, 179, 201, 226, 254, 285, 320, 359, 403, 452, 508, 570,
 640, 719, 807, 905,1016,1141,1281,1438,1614,1811,2033,2282,2562,
}; // max*3=13bit
static unsigned short intra_penalty_tu_8[52] = {   /* Intra Penalty for TU 8x8 vs. 16x16 */
   7,   7,   8,  10,  11,  12,  14,  15,  17,  20,  22,  25,  28,
  31,  35,  40,  44,  50,  56,  63,  71,  80,  89, 100, 113, 127,
 142, 160, 179, 201, 226, 254, 285, 320, 359, 403, 452, 508, 570,
 640, 719, 807, 905,1016,1141,1281,1438,1614,1811,2033,2282,2562,
}; // max*3=13bit
static unsigned short intra_penalty_tu16[52] = {   /* Intra Penalty for TU 16x16 vs. 32x32 */
   9,  11,  12,  14,  15,  17,  19,  22,  24,  28,  31,  35,  39,
  44,  49,  56,  62,  70,  79,  88,  99, 112, 125, 141, 158, 177,
 199, 224, 251, 282, 317, 355, 399, 448, 503, 564, 634, 711, 799,
 896,1006,1129,1268,1423,1598,1793,2013,2259,2536,2847,3196,3587,
}; // max*3=14bit
static unsigned short intra_penalty_tu32[52] = {   /* Intra Penalty for TU 32x32 vs. 64x64 */
   9,  11,  12,  14,  15,  17,  19,  22,  24,  28,  31,  35,  39,
  44,  49,  56,  62,  70,  79,  88,  99, 112, 125, 141, 158, 177,
 199, 224, 251, 282, 317, 355, 399, 448, 503, 564, 634, 711, 799,
 896,1006,1129,1268,1423,1598,1793,2013,2259,2536,2847,3196,3587,
}; // max*3=14bit
static unsigned short intra_penalty_mode_a[52] = { /* Intra Penalty for Mode a */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,
   1,   1,   1,   2,   2,   2,   2,   3,   3,   4,   4,   5,   5,
   6,   7,   8,   9,  10,  11,  13,  15,  16,  19,  21,  23,  26,
  30,  33,  38,  42,  47,  53,  60,  67,  76,  85,  95, 107, 120,
}; // max*3=9bit
static unsigned short intra_penalty_mode_b[52] = { /* Intra Penalty for Mode b */
   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   2,
   2,   2,   2,   3,   3,   4,   4,   5,   5,   6,   7,   8,   9,
  10,  11,  13,  14,  16,  18,  21,  23,  26,  29,  33,  37,  42,
  47,  53,  59,  66,  75,  84,  94, 106, 119, 133, 150, 168, 189,
}; // max*3=10bit
static unsigned short intra_penalty_mode_c[52] = { /* Intra Penalty for Mode c */
   1,   1,   1,   1,   1,   1,   2,   2,   2,   3,   3,   3,   4,
   4,   5,   6,   6,   7,   8,   9,  10,  12,  13,  15,  17,  19,
  21,  24,  27,  31,  34,  39,  43,  49,  55,  62,  69,  78,  87,
  98, 110, 124, 139, 156, 175, 197, 221, 248, 278, 312, 351, 394,
}; // max*3=11bit
static unsigned short lambda_sse[2][52] = {
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   7,   9,  11,
  14,  18,  23,  29,  36,  46,  58,  73,  93, 117, 147, 186, 234,
 295, 372, 469, 591, 745, 939,1183,1491,1879,2367,2982,3758,4734},
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   1,   1,   2,   2,   3,   4,   5,   6,   8,  10,  12,  16,
  20,  25,  32,  40,  51,  64,  81, 102, 129, 162, 204, 258, 325,
 409, 516, 650, 819,1032,1300,1638,2064,2600,3276,4128,5201,6553},
};
static unsigned short lambda_sad[2][52] = {
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   1,   1,   1,   1,   1,   1,   2,   2,   2,   3,   3,
   3,   4,   4,   5,   6,   6,   7,   8,   9,  10,  12,  13,  15,
  17,  19,  21,  24,  27,  30,  34,  38,  43,  48,  54,  61,  68},
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   3,   3,   4,
   4,   5,   5,   6,   7,   8,   9,  10,  11,  12,  14,  16,  18,
  20,  22,  25,  28,  32,  36,  40,  45,  50,  57,  64,  72,  80},
};

int vrqc_enc_buff(rqct_ops* rqct, rqct_buf* buff)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    if (rqcx->i_pcount == 0)
        rqct->seq_conf(rqct);
    /* picture-type decision */
    if (rqcx->i_pcount == rqcx->i_period)
    {
        rqcx->i_pcount = 1;
        rqct->i_pictyp = RQCT_PICTYP_I;
        rqct->i_enc_qp = rqcx->attr.i_leadqp - rqcx->attr.i_deltaq;
        rqct->b_unrefp = 0;
    }
    else
    {
        rqcx->i_pcount++;
        rqct->i_pictyp = RQCT_PICTYP_P;
        rqct->i_enc_qp = rqcx->attr.i_leadqp;
        if (rqct->b_unrefp)
            rqct->b_unrefp = !(rqcx->i_pcount%2);
    }
    return 0;
}
EXPORT_SYMBOL(vrqc_enc_buff);

int vrqc_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_mirr* mirr = (vhe_mirr*)mjob;
    int encq = rqct->i_enc_qp, roiq;
    int type = rqct->i_pictyp;
    uint* regs = mirr->swregs;

    vhe_put_regs(regs, HWIF_ENC_LAMDA_MOTION_SSE,       lambda_sse[type][encq]);
    vhe_put_regs(regs, HWIF_ENC_LAMBDA_MOTIONSAD,       lambda_sad[type][encq]);
    vhe_put_regs(regs, HWIF_ENC_LAMDA_SAO_LUMA,         lambda_sse[type][encq]);
    vhe_put_regs(regs, HWIF_ENC_LAMDA_SAO_CHROMA,       lambda_sse[type][encq]*3/4);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC4X4,   intra_penalty_tu_4[encq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC8X8,   intra_penalty_tu_8[encq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC16X16, intra_penalty_tu16[encq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC32X32, intra_penalty_tu32[encq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_PIC1, intra_penalty_mode_a[encq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_PIC2, intra_penalty_mode_b[encq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_PIC3, intra_penalty_mode_c[encq]);
    roiq = MAX(0,encq-rqcx->attr.i_roidqp[0]);
    vhe_put_regs(regs, HWIF_ENC_LAMDA_MOTION_SSE_ROI1,  lambda_sse[type][roiq]);
    vhe_put_regs(regs, HWIF_ENC_LAMBDA_MOTIONSAD_ROI1,  lambda_sad[type][roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI14X4,  intra_penalty_tu_4[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI18X8,  intra_penalty_tu_8[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI116X16,intra_penalty_tu16[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI132X32,intra_penalty_tu32[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI11,intra_penalty_mode_a[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI12,intra_penalty_mode_b[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI13,intra_penalty_mode_c[roiq]);
    roiq = MAX(0,encq-rqcx->attr.i_roidqp[1]);
    vhe_put_regs(regs, HWIF_ENC_LAMDA_MOTION_SSE_ROI2,  lambda_sse[type][roiq]);
    vhe_put_regs(regs, HWIF_ENC_LAMBDA_MOTIONSAD_ROI2,  lambda_sad[type][roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI24X4,  intra_penalty_tu_4[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI28X8,  intra_penalty_tu_8[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI216X16,intra_penalty_tu16[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI232X32,intra_penalty_tu32[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI21,intra_penalty_mode_a[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI22,intra_penalty_mode_b[roiq]);
    vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI23,intra_penalty_mode_c[roiq]);
    /* ROIs' region and their dqps */
    if (0 == rqcx->attr.b_dqmstat)
        return 0;
    if (rqcx->attr.b_dqmstat < 0)
    {
        rqcx->attr.b_dqmstat = 0;
#if defined(H2V1_ROI_PATCH)
        vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_LEFT,    0xFF);
        vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_RIGHT,   0xFF);
        vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_TOP,     0xFF);
        vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_BOTTOM,  0xFF);
        vhe_put_regs(regs, HWIF_ENC_CIR_START,          0);
        vhe_put_regs(regs, HWIF_ENC_CIR_INTERVAL,       0);
#endif
        if (0 < (roiq = rqcx->attr.i_roidqp[0]))
        {
            short lf = rqcx->attr.m_roirec[0].i_posx;
            short tp = rqcx->attr.m_roirec[0].i_posy;
            short rt = rqcx->attr.m_roirec[0].i_recw+lf-1;
            short bt = rqcx->attr.m_roirec[0].i_rech+tp-1;
            vhe_put_regs(regs, HWIF_ENC_ROI1_LEFT,      lf);
            vhe_put_regs(regs, HWIF_ENC_ROI1_RIGHT,     rt);
            vhe_put_regs(regs, HWIF_ENC_ROI1_TOP,       tp);
            vhe_put_regs(regs, HWIF_ENC_ROI1_BOTTOM,    bt);
            vhe_put_regs(regs, HWIF_ENC_ROI1_DELTA_QP,  MIN(encq,roiq));
            rqcx->attr.b_dqmstat|= 1;
#if defined(H2V1_ROI_PATCH)
            rqcx->i_edge[0].cir = rt+tp*rqcx->attr.i_dqmw+1;
            rqcx->i_edge[0].lft = lf;
            rqcx->i_edge[0].top = tp;
            rqcx->i_edge[0].bot = bt;
#endif
        }
        else
        {
            vhe_put_regs(regs, HWIF_ENC_ROI1_LEFT,      0xFF);
            vhe_put_regs(regs, HWIF_ENC_ROI1_RIGHT,     0xFF);
            vhe_put_regs(regs, HWIF_ENC_ROI1_TOP,       0xFF);
            vhe_put_regs(regs, HWIF_ENC_ROI1_BOTTOM,    0xFF);
            vhe_put_regs(regs, HWIF_ENC_ROI1_DELTA_QP,  0);
#if defined(H2V1_ROI_PATCH)
            rqcx->i_edge[0].cir = rqcx->i_edge[0].lft =
            rqcx->i_edge[0].top = rqcx->i_edge[0].bot = 0;
#endif
        }
        if (0 < (roiq = rqcx->attr.i_roidqp[1]))
        {
            short lf = rqcx->attr.m_roirec[1].i_posx;
            short tp = rqcx->attr.m_roirec[1].i_posy;
            short rt = rqcx->attr.m_roirec[1].i_recw+lf-1;
            short bt = rqcx->attr.m_roirec[1].i_rech+tp-1;
            vhe_put_regs(regs, HWIF_ENC_ROI2_LEFT,      lf);
            vhe_put_regs(regs, HWIF_ENC_ROI2_RIGHT,     rt);
            vhe_put_regs(regs, HWIF_ENC_ROI2_TOP,       tp);
            vhe_put_regs(regs, HWIF_ENC_ROI2_BOTTOM,    bt);
            vhe_put_regs(regs, HWIF_ENC_ROI2_DELTA_QP,  MIN(encq,roiq));
            rqcx->attr.b_dqmstat|= 1;
#if defined(H2V1_ROI_PATCH)
            rqcx->i_edge[1].cir = rt+tp*rqcx->attr.i_dqmw+1;
            rqcx->i_edge[1].lft = lf;
            rqcx->i_edge[1].top = tp;
            rqcx->i_edge[1].bot = bt;
#endif
        }
        else
        {
            vhe_put_regs(regs, HWIF_ENC_ROI2_LEFT,      0xFF);
            vhe_put_regs(regs, HWIF_ENC_ROI2_RIGHT,     0xFF);
            vhe_put_regs(regs, HWIF_ENC_ROI2_TOP,       0xFF);
            vhe_put_regs(regs, HWIF_ENC_ROI2_BOTTOM,    0xFF);
            vhe_put_regs(regs, HWIF_ENC_ROI2_DELTA_QP,  0);
#if defined(H2V1_ROI_PATCH)
            rqcx->i_edge[1].cir = rqcx->i_edge[1].lft =
            rqcx->i_edge[1].top = rqcx->i_edge[1].bot = 0;
#endif
        }
#if defined(H2V1_ROI_PATCH)
        if (rqcx->attr.i_roidqp[0] == 0 && rqcx->attr.i_roidqp[1] > 0)
            rqcx->i_edge[0] = rqcx->i_edge[1];
        if (rqcx->attr.i_roidqp[1] == 0 && rqcx->attr.i_roidqp[0] > 0)
            rqcx->i_edge[1] = rqcx->i_edge[0];
#endif
    }
#if defined(H2V1_ROI_PATCH)
    /* Intra area */
    vhe_put_regs(regs, HWIF_ENC_SKIP_CHROMA_DC_THREADHOLD,      2);
    if (rqcx->attr.b_dqmstat > 0)
    {
        int i = 1&rqcx->i_coin++;
        vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_LEFT,    rqcx->i_edge[i].lft);
        vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_RIGHT,   rqcx->i_edge[i].lft);
        vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_TOP,     rqcx->i_edge[i].top);
        vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_BOTTOM,  rqcx->i_edge[i].bot);
        vhe_put_regs(regs, HWIF_ENC_CIR_START,          rqcx->i_edge[i].cir);
        vhe_put_regs(regs, HWIF_ENC_CIR_INTERVAL,       rqcx->attr.i_dqmw);
        if (type == RQCT_PICTYP_P)
            vhe_put_regs(regs, HWIF_ENC_SKIP_CHROMA_DC_THREADHOLD, 0);
    }
#endif
    return 0;
}
EXPORT_SYMBOL(vrqc_enc_conf);

int vrqc_set_rqcf(rqct_ops* rqct, rqct_cfg* rqcf)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    int err = -1, idx;
    switch (rqcf->type)
    {
    case RQCT_CFG_SEQ:
        if ((unsigned)rqcf->seq.i_method > RQCT_METHOD_VBR)
            break;
        rqcx->attr.i_method = rqcf->seq.i_method;
        rqcx->attr.i_period = rqcf->seq.i_period;
        rqcx->attr.i_btrate = rqcf->seq.i_btrate;
        rqcx->attr.i_leadqp = rqcf->seq.i_leadqp;
        rqcx->i_pcount = 0;
        err = 0;
        break;
    case RQCT_CFG_DQP:
        if ((unsigned)rqcf->dqp.i_dqp > 8)
            break;
        rqcx->attr.i_deltaq = rqcf->dqp.i_dqp;
        err = 0;
        break;
    case RQCT_CFG_QPR:
        if ((unsigned)rqcf->qpr.i_upperq > 51 ||
            (unsigned)rqcf->qpr.i_lowerq > 51 ||
            rqcf->qpr.i_lowerq > rqcf->qpr.i_upperq)
            break;
        rqcx->attr.i_upperq = rqcf->qpr.i_upperq;
        rqcx->attr.i_lowerq = rqcf->qpr.i_lowerq;
        err = 0;
        break;
    case RQCT_CFG_LOG:
        rqcx->attr.b_logoff = !rqcf->log.b_logm;
        err = 0;
        break;
    /* private config */
    case RQCT_CFG_RES:
        rqcx->attr.i_pict_w = (short)rqcf->res.i_picw;
        rqcx->attr.i_pict_h = (short)rqcf->res.i_pich;
        err = 0;
        break;
    case RQCT_CFG_FPS:
        rqcx->attr.n_fmrate = (short)rqcf->fps.n_fps;
        rqcx->attr.d_fmrate = (short)rqcf->fps.d_fps;
        err = 0;
        break;
    case RQCT_CFG_ROI:
        if (1 < (idx = rqcf->roi.i_roiidx))
            break;
        if ((unsigned)(-rqcf->roi.i_roidqp) > 15)
            break;
        rqcx->attr.b_dqmstat = -1;
        err = 0;
        if (idx < 0)
        {
            rqcx->attr.i_roidqp[0] =
            rqcx->attr.i_roidqp[1] = 0;
            break;
        }
        if (rqcx->attr.i_dqmw < rqcf->roi.i_posx || rqcx->attr.i_dqmh < rqcf->roi.i_posy)
        {
            rqcx->attr.i_roidqp[idx] = 0;
            break;
        }
        if (rqcf->roi.i_recw > (rqcx->attr.i_dqmw - rqcf->roi.i_posx))
            rqcf->roi.i_recw = (rqcx->attr.i_dqmw - rqcf->roi.i_posx);
        if (rqcf->roi.i_rech > (rqcx->attr.i_dqmh - rqcf->roi.i_posy))
            rqcf->roi.i_rech = (rqcx->attr.i_dqmh - rqcf->roi.i_posy);
        rqcx->attr.i_roidqp[idx] = -rqcf->roi.i_roidqp;
        rqcx->attr.m_roirec[idx].i_posx = rqcf->roi.i_posx;
        rqcx->attr.m_roirec[idx].i_posy = rqcf->roi.i_posy;
        rqcx->attr.m_roirec[idx].i_recw = rqcf->roi.i_recw;
        rqcx->attr.m_roirec[idx].i_rech = rqcf->roi.i_rech;
        break;
    case RQCT_CFG_DQM:
        rqcx->attr.i_dqmw = rqcf->dqm.i_dqmw;
        rqcx->attr.i_dqmh = rqcf->dqm.i_dqmh;
        err = 0;
        break;
    default:
        break;
    }
    return err;
}
EXPORT_SYMBOL(vrqc_set_rqcf);

int vrqc_get_rqcf(rqct_ops* rqct, rqct_cfg* rqcf)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    int err = -1;
    switch (rqcf->type)
    {
    case RQCT_CFG_SEQ:
        rqcf->seq.i_method = rqcx->attr.i_method;
        rqcf->seq.i_period = rqcx->attr.i_period;
        rqcf->seq.i_btrate = rqcx->attr.i_btrate;
        rqcf->seq.i_leadqp = rqcx->attr.i_leadqp;
        err = 0;
        break;
    case RQCT_CFG_DQP:
        rqcf->dqp.i_dqp = rqcx->attr.i_deltaq;
        err = 0;
        break;
    case RQCT_CFG_QPR:
        rqcf->qpr.i_upperq = rqcx->attr.i_upperq;
        rqcf->qpr.i_lowerq = rqcx->attr.i_lowerq;
        err = 0;
        break;
    case RQCT_CFG_LOG:
        rqcf->log.b_logm = !rqcx->attr.b_logoff;
        err = 0;
        break;
    /* private config */
    case RQCT_CFG_RES:
        rqcf->res.i_picw = (int)rqcx->attr.i_pict_w;
        rqcf->res.i_pich = (int)rqcx->attr.i_pict_h;
        err = 0;
        break;
    case RQCT_CFG_FPS:
        rqcf->fps.n_fps = (int)rqcx->attr.n_fmrate;
        rqcf->fps.d_fps = (int)rqcx->attr.d_fmrate;
        err = 0;
        break;
    case RQCT_CFG_ROI:
        if ((unsigned)rqcf->roi.i_roiidx > 1)
            break;
        rqcf->roi.i_roidqp = -rqcx->attr.i_roidqp[rqcf->roi.i_roiidx];
        rqcf->roi.i_posx = rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_posx;
        rqcf->roi.i_posy = rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_posy;
        rqcf->roi.i_recw = rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_recw;
        rqcf->roi.i_rech = rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_rech;
        err = 0;
        break;
    case RQCT_CFG_DQM:
        rqcf->dqm.i_dqmw = rqcx->attr.i_dqmw;
        rqcf->dqm.i_dqmh = rqcx->attr.i_dqmh;
        err = 0;
        break;
    default:
        break;
    }
    return err;
}
EXPORT_SYMBOL(vrqc_get_rqcf);
