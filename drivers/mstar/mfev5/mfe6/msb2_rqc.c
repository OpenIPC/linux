
#include <mfe6_ops.h>
#include <mfe6_utl.h>

/************************************************************************/
/* Version Declaration                                                  */
/************************************************************************/

#define MSB2_NAME               "MSB2"
#define MSB2_VER_MJR            1
#define MSB2_VER_MNR            1
#define MSB2_VER_EXT            01
#define _EXP(expr)              #expr
#define _STR(expr)              _EXP(expr)

/************************************************************************/
/* Macros                                                               */
/************************************************************************/

#define UP_QP_DIFF_LIMIT        3
#define LF_QP_DIFF_LIMIT        3

#define LOGOFF_DEFAULT          1
#define LOWERQ_DEFAULT          12
#define UPPERQ_DEFAULT          48
#define BOUNDQ_DEFAULT          -1
#define QP_DEGREE       4
#define LQP_HW_LIMIT    16
#define QS_ACCURACY   39    //40

/* 64-bits calculation */
#if defined(LINUX_KERNEL)
#include <linux/math64.h>
#define DIV_S64(n,d)    div_s64(n,d)
#else
#define DIV_S64(n,d)    ((n)/(d))
#endif

#ifndef MSRC_FIT_IN_RANGE
#define MSRC_FIT_IN_RANGE(a,low,high)   MIN(MAX((a),(low)),(high))
#endif

#ifndef MSRC_PERCENT
#define MSRC_PERCENT(percent,a)   ( (percent) * (a) / 100 )
#endif

/* Skip the first N frames for VBV calculation  */
#define CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES       50

/* The feature to reset the fullness value if fullness overflow or underflow */
/*   1 - Enable                                                              */
/*   0 - Disable                                                             */
#define CFG_VBV_BUF_OVERFLOW_RESET              1
#define CFG_VBV_BUF_UNDERFLOW_RESET             1

#define RDC_WEIGHTED_DIVIDED                    0
#define RDC_AVERAGE_DIVIDED                     1
#define CFG_RC_DELTA_CACULATE_METHOD            RDC_WEIGHTED_DIVIDED

/* The choice of the VBV frame quota selection                               */
#define VFQ_USE_AVERAGE_SIZE                    0   /*   The average size of each frame            */
#define VFQ_USE_WEIGHTED_SIZE                   1   /* The weighted size rqcx on the setting of    */
                                                    /* IFRAME_WEIGHT, PFRAME_WEIGHT, BFRAME_WEIGHT */
                                                    /* and Gop size                                */
#define CFG_VBV_FRAME_QUOTA_METHOD              VFQ_USE_WEIGHTED_SIZE

#define SMOOTH_BASE                             10       // 1 sec = 10* 100ms
//! How many seconds of frames are responsible for compensation of bitrate usage.
#define SMOOTH_SECOND_CBR                       10        // unit 100 ms
#define SMOOTH_SECOND_CVBR                      30        // unit 100 ms
#define SMOOTH_SECOND_VBR                       100       // unit 100 ms
#define SMOOTH_FACTOR_MIN                       100       // unit 100 ms

#define VBV_TGT_LVL_CBR                         20        // unit 100 ms
#define VBV_FUL_LOW_CBR                         0         // unit 100 ms
#define VBV_FUL_HIGH_CBR                        40        // unit 100 ms
#define VBV_BUF_SIZE_CBR                        40        // unit 100 ms

#define VBV_TGT_LVL_VBR                         40        // unit 100 ms
#define VBV_FUL_LOW_VBR                         0         // unit 100 ms
#define VBV_FUL_HIGH_VBR                        80        // unit 100 ms
#define VBV_BUF_SIZE_VBR                        80        // unit 100 ms

//! Default I-frame weighting over inter-frame
#define IFRAME_WEIGHT                           (1024*7)
#define PFRAME_WEIGHT                           1024
#define BFRAME_WEIGHT                           768

#define MAX_FRAME_TYPE                          3   // I, P, B 3 types

//enable msbr rate control
#define RC_MSBR      1
/************************************************************************/
/* Configuration                                                        */
/************************************************************************/

/************************************************************************/
/* Constant                                                             */
/************************************************************************/

/************************************************************************/
/* Local structures                                                     */
/************************************************************************/

#define PICTYPES 2

typedef struct msb2_rqc
{
    mfe6_rqc    rqcx;
    int     i_method;
    int     i_btrate;
    int     i_levelq;
    int     i_deltaq;
    int     n_fmrate;
    int     d_fmrate;
    int     i_fmrate;
    int     i_pixels;
    int     i_blocks;
    short   b_mbkadp, i_limitq;
    short   i_frmdqp, i_blkdqp;
    short   i_iupperq, i_ilowerq;
    short   i_pupperq, i_plowerq;
    int     i_gopbit;
    int     i_frmbit;
    int     i_pixbit;
    int     i_budget;
    int     i_ipbias;
    int     i_smooth;
    int     i_bucket;
    int     i_upperb, i_lowerb;
    int     i_margin;
    int     i_radius;
    int     i_degree;
    int     i_errpro;
    int     i_imbase;
    int     i_imbits;   // bit-pos:imaginary
    int     i_rebits;   // bit-pos:real

    iir_t iir_rqprod[PICTYPES];
    acc_t acc_rqprod;
    acc_t acc_bitcnt;
    int   i_intrabit;

#if RC_MSBR
    /* coded frame counter */
    int i_frm_nr;
    /* bitrate usage compensation */
    int     i_vbv_buf_size;                         // Rate control buffer size
    int     i_vbv_fullness_low;                     // Rate control buffer fullness low bound
    int     i_vbv_fullness_high;                    // Rate control buffer fullness high bound
    int     i_vbv_fullness;                         // Rate control buffer fullness
    int     i_vbv_real_fullness;                    // Rate control buffer fullness
    int     i_vbv_target;                           // Rate control buffer target fullness

    int     i_smooth_cnt;
    int     i_smooth_min;
    /* variable bitrate */
    int     i_lt_qs64;
    /* model parameters*/
    int     i_tgt_bs;                               // target number of bits of current frame
    /* Derived variables*/
    int     m_bpf[MAX_FRAME_TYPE];                  // I, P, B
    int     i_avgbpf;
    /* last-frame status */
    int     i_last_avg_qs;
    int     i_avg_qs;
    /* bitcnt return */
    int     vbr_bit_return_stage;
#endif
} msb2_rqc;

/************************************************************************/
/* Local prototypes                                                     */
/************************************************************************/

#define COMPLEX_I   (1<<24)
#define COMPLEX_P   (1<<20)

static int  msb2_seq_sync(rqct_ops*);
static int  msb2_seq_done(rqct_ops*);
static int  msb2_seq_conf(rqct_ops*);
static int  msb2_enc_conf(rqct_ops*, mhve_job*);
static int  msb2_enc_done(rqct_ops*, mhve_job*);
static void _msb2_ops_free(rqct_ops* rqct) { MEM_FREE(rqct); }

/************************************************************************/
/* Functions                                                            */
/************************************************************************/

char* msb2_describe(void)
{
    static char line[64];
    sprintf(line, "%s@v%d.%d-%02d:r&d analysis.",MSB2_NAME,MSB2_VER_MJR,MSB2_VER_MNR,MSB2_VER_EXT);
    return line;
}

static int msb2_set_rqcf(rqct_ops* rqct, rqct_cfg* rqcf)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    msb2_rqc* msb2 = (msb2_rqc*)rqcx;
    int err = -1;

    switch (rqcf->type)
    {
    case RQCT_CFG_DQP:
        rqcx->attr.i_deltaq = MAX(MIN(rqcf->dqp.i_dqp, DQP_MAX), DQP_MIN);

        msb2->i_deltaq = (int)rqcx->attr.i_deltaq;
        err = 0;
        break;
    case RQCT_CFG_QPR:
        if (rqcf->qpr.i_iupperq < rqcf->qpr.i_ilowerq)
        {
            err = -1;
            break;
        }
        if (rqcf->qpr.i_pupperq < rqcf->qpr.i_plowerq)
        {
            err = -1;
            break;
        }
        rqcx->attr.i_iupperq = MAX(MIN(rqcf->qpr.i_iupperq, QP_MAX), QP_MIN);
        rqcx->attr.i_ilowerq = MAX(MIN(rqcf->qpr.i_ilowerq, QP_MAX), QP_MIN);
        rqcx->attr.i_pupperq = MAX(MIN(rqcf->qpr.i_pupperq, QP_MAX), QP_MIN);
        rqcx->attr.i_plowerq = MAX(MIN(rqcf->qpr.i_plowerq, QP_MAX), QP_MIN);

        msb2->i_iupperq = MAX(MIN(rqcx->attr.i_iupperq, QP_MAX), QP_MIN);
        msb2->i_ilowerq = MAX(MIN(rqcx->attr.i_ilowerq, QP_MAX), QP_MIN);
        msb2->i_pupperq = MAX(MIN(rqcx->attr.i_pupperq, QP_MAX), QP_MIN);
        msb2->i_plowerq = MAX(MIN(rqcx->attr.i_plowerq, QP_MAX), QP_MIN);
        err = 0;
        break;
    default:
        err = mrqc_set_rqcf(rqct, rqcf);
        break;
    }
    return err;
}

void* msb2_allocate(void)
{
    rqct_ops* rqct = NULL;
    mfe6_rqc* rqcx;
    msb2_rqc* msb2;

    if (!(rqct = MEM_ALLC(sizeof(msb2_rqc))))
        return NULL;

    MEM_COPY(rqct->name, MSB2_NAME, 5);
    rqct->release = _msb2_ops_free;
    rqct->seq_sync = msb2_seq_sync;
    rqct->seq_done = msb2_seq_done;
    //rqct->set_rqcf = mrqc_set_rqcf;
    rqct->set_rqcf = msb2_set_rqcf;
    rqct->get_rqcf = mrqc_get_rqcf;
    rqct->seq_conf = msb2_seq_conf;
    rqct->enc_buff = mrqc_enc_buff;
    rqct->enc_conf = msb2_enc_conf;
    rqct->enc_done = msb2_enc_done;
    rqct->i_enc_nr = 0;
    rqct->i_enc_bs = 0;
    rqcx = (mfe6_rqc*)rqct;
    rqcx->attr.i_pict_w = 0;
    rqcx->attr.i_pict_h = 0;
    rqcx->attr.i_method = RQCT_MODE_CQP;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.i_leadqp =-1;
    rqcx->attr.i_deltaq = QP_IFRAME_DELTA;
    rqcx->attr.i_iupperq = QP_UPPER;
    rqcx->attr.i_ilowerq = QP_LOWER;
    rqcx->attr.i_pupperq = QP_UPPER;
    rqcx->attr.i_plowerq = QP_LOWER;
    rqcx->attr.n_fmrate =30;
    rqcx->attr.d_fmrate = 1;
    rqcx->attr.i_iperiod= 0;
    rqcx->attr.i_ltrperiod= 0;
    rqcx->attr.b_logoff = LOGOFF_DEFAULT;
    /* initiate penalties */
    rqcx->attr.b_i16pln = 1;
    rqcx->attr.i_peni4x = 0;
    rqcx->attr.i_peni16 = 0;
    rqcx->attr.i_penint = 0;
    rqcx->attr.i_penYpl = 0;
    rqcx->attr.i_penCpl = 0;
    rqcx->i_config = 0;
    rqcx->i_ipcount = 0;
    rqcx->i_iperiod = 0;
    rqcx->i_ltrpcount = 0;
    rqcx->i_ltrperiod = 0;
    msb2 = (msb2_rqc*)rqcx;
    msb2->i_levelq =36;

    printk("%s\n",msb2_describe());

    return rqct;
}

#define IPR_BIT     (16)
#define IPR_FAC     (1<<IPR_BIT)
#define BPP_BIT     (16)
#define BPP_FAC     (1<<BPP_BIT)
#define QSC_BIT     (5)
#define QSCALE(v)   (((v)+(512>>(QSC_BIT)))>>(10-(QSC_BIT)))
static int qscale[]={
               QSCALE(   645),QSCALE(   724),QSCALE(   813),QSCALE(   912),QSCALE(  1024),
QSCALE(  1149),QSCALE(  1290),QSCALE(  1448),QSCALE(  1625),QSCALE(  1825),QSCALE(  2048),
QSCALE(  2299),QSCALE(  2580),QSCALE(  2896),QSCALE(  3251),QSCALE(  3649),QSCALE(  4096),
QSCALE(  4598),QSCALE(  5161),QSCALE(  5793),QSCALE(  6502),QSCALE(  7298),QSCALE(  8192),
QSCALE(  9195),QSCALE( 10321),QSCALE( 11585),QSCALE( 13004),QSCALE( 14596),QSCALE( 16384),
QSCALE( 18390),QSCALE( 20643),QSCALE( 23170),QSCALE( 26008),QSCALE( 29193),QSCALE( 32768),
QSCALE( 36781),QSCALE( 41285),QSCALE( 46341),QSCALE( 52016),QSCALE( 58386),QSCALE( 65536),
QSCALE( 73562),QSCALE( 82570),QSCALE( 92682),QSCALE(104032),QSCALE(116772),QSCALE(131072),
QSCALE(147123),QSCALE(165140),QSCALE(185364),QSCALE(208064),QSCALE(233544),QSCALE(262144),
};
static int qp2qs(int qp)
{
    if (qp< 0)  return qscale[ 0];
    if (qp>51)  return 2147483647;
    return qscale[qp];
}
static int qindex[]={
 4,10,16,16,22,22,22,22,28,28,28,28,28,28,28,28,
34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,
40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,
46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,
46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,
46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,
46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,
52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,
52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,
52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,
52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,
52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,
52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,
52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,
52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,
};
static int qs2qp(unsigned long qs)
{
    int idx, cost, best;
    if (qs<qscale[ 0])  return  0;
    if (qs>qscale[51])  return 51;
    idx = qindex[qs>>QSC_BIT];
    best = (qscale[idx]-qs);
    while (0<(cost=(qscale[--idx]-qs)))
        best=cost;
    if (best<-cost)
        return ++idx;
    return idx;
}
#define DQBIAS_BIT  15
#define DQBIAS_FAC  (1<<DQBIAS_BIT)
#define DQBIAS(r)   (((r)+(1<<(15-(DQBIAS_BIT))))>>(16-DQBIAS_BIT))
static const int dqplot[] = {
DQBIAS(    0),//0.0000000000
DQBIAS(13889),//0.2119255505
DQBIAS(22749),//0.3471209568
DQBIAS(28655),//0.4372439481
DQBIAS(33009),//0.5036719007
DQBIAS(36440),//0.5560238879
DQBIAS(39266),//0.5991464061
DQBIAS(41666),//0.6357766516
DQBIAS(43752),//0.6676004651
DQBIAS(45595),//0.6957262149
DQBIAS(47246),//0.7209202303
DQBIAS(48741),//0.7437338663
DQBIAS(50107),//0.7645767803
DQBIAS(51365),//0.7837615271
DQBIAS(52529),//0.8015319306
DQBIAS(53614),//0.8180818208
DQBIAS(54629),//0.8335678038
DQBIAS(55582),//0.8481182052
DQBIAS(56482),//0.8618394843
DQBIAS(57332),//0.8748209289
DQBIAS(58139),//0.8871381559
DQBIAS(58907),//0.8988557638
DQBIAS(59640),//0.9100293726
DQBIAS(60339),//0.9207072127
DQBIAS(61010),//0.9309313772
DQBIAS(61652),//0.9407388203
DQBIAS(62270),//0.9501621606
DQBIAS(62864),//0.9592303332
DQBIAS(63437),//0.9679691242
DQBIAS(63989),//0.9764016112
DQBIAS(64523),//0.9845485296
DQBIAS(65040),//0.9924285792
};
static int qdelta(int r)
{
    int d = 0, s = r<0;
    r = s?-r:r;
    while (r > dqplot[++d]) ;
    --d;
    return s?-d:d;
}
/*
static int dqbias(int d)
{
    int i=((unsigned)(d+32)>64)?32:(d<0?-d:d);
    int p=dqplot[i];
    return d<0?-p:p;
}
*/
#define MAX_SUMQS   ((2UL<<31)-1)
#define ALIGN2MB(l) (((l)+15)>>4)
static int calc_limitq(int mbs)
{
    unsigned long qs = MAX_SUMQS/(mbs-1);
    int qp = qs2qp(qs);
    return qs<qp2qs(qp) ? qp-1 : qp;
}

static int msb2_seq_done(rqct_ops* rqct)
{
    /* Debug print */
    #if (DBG_PRINT_ENABLE && RC_MSBR)
    {
        mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
        msb2_rqc* msb2 = (msb2_rqc*)rqcx;
        MFE_DBG( "m_bpf %d %d %d \n", msb2->m_bpf[0], msb2->m_bpf[1], msb2->m_bpf[2] );
    }
    #endif

    return 0;
}

static int msb2_seq_sync(rqct_ops* rqct)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    rqcx->i_ipcount = 0;
    rqcx->i_ltrpcount = 0;
    return 0;
}

#define SMOOTH_X    (16)

static int msb2_seq_conf(rqct_ops* rqct)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    msb2_rqc* msb2 = (msb2_rqc*)rqcx;
    short upperq = rqcx->attr.i_iupperq>rqcx->attr.i_pupperq?rqcx->attr.i_iupperq:rqcx->attr.i_pupperq;
    short lowerq = rqcx->attr.i_ilowerq<rqcx->attr.i_plowerq?rqcx->attr.i_ilowerq:rqcx->attr.i_plowerq;
    msb2->i_method = (int)rqcx->attr.i_method;
    msb2->i_btrate = (int)rqcx->attr.i_btrate;
    if (rqcx->attr.i_leadqp > 8 && rqcx->attr.i_leadqp < 48)
        msb2->i_levelq = (int)rqcx->attr.i_leadqp;
    msb2->i_deltaq = (int)rqcx->attr.i_deltaq;
    msb2->n_fmrate = (int)rqcx->attr.n_fmrate;
    msb2->d_fmrate = (int)rqcx->attr.d_fmrate;
    msb2->i_pixels = (int)rqcx->attr.i_pict_w*rqcx->attr.i_pict_h;
    msb2->i_fmrate = (msb2->n_fmrate+msb2->n_fmrate-1)/msb2->d_fmrate;
    msb2->i_blocks = ALIGN2MB(rqcx->attr.i_pict_w)*ALIGN2MB(rqcx->attr.i_pict_h);
    msb2->i_limitq = calc_limitq(msb2->i_blocks);
    msb2->i_iupperq = MAX(MIN(rqcx->attr.i_iupperq, QP_MAX), QP_MIN);
    msb2->i_ilowerq = MAX(MIN(rqcx->attr.i_ilowerq, QP_MAX), QP_MIN);
    msb2->i_pupperq = MAX(MIN(rqcx->attr.i_pupperq, QP_MAX), QP_MIN);
    msb2->i_plowerq = MAX(MIN(rqcx->attr.i_plowerq, QP_MAX), QP_MIN);
    if (msb2->i_btrate == 0)
        msb2->i_method = RQCT_MODE_CQP;
    rqcx->i_ipcount =
    rqcx->i_iperiod = (int)rqcx->attr.i_iperiod;
    rqcx->i_ltrpcount =
    rqcx->i_ltrperiod = (int)rqcx->attr.i_ltrperiod;

//    msb2->i_smooth = rqcx->i_iperiod*2;
    msb2->i_smooth = rqcx->i_iperiod;
    msb2->i_frmbit = (int)div_s64((int64)msb2->i_btrate*msb2->d_fmrate,msb2->n_fmrate);
    msb2->i_pixbit = (int)div_s64((int64)msb2->i_frmbit*BPP_FAC,msb2->i_pixels);
    msb2->i_gopbit = rqcx->i_iperiod*msb2->i_frmbit;
    msb2->b_mbkadp = 0;
    msb2->i_blkdqp = 4;
    msb2->i_frmdqp = 1;

    msb2->i_degree = MAX(msb2->i_btrate/(QP_RANGE*2), 1);  //avoid division by zero in kernel
    msb2->i_margin = msb2->i_degree*(QP_RANGE);
    msb2->i_bucket = msb2->i_margin<<1;
    msb2->i_radius = msb2->i_margin>>1;
    msb2->i_rebits =
    msb2->i_imbits =
    msb2->i_imbase = msb2->i_margin+(msb2->i_levelq-QP_LOWER)*msb2->i_degree;
    msb2->i_lowerb = msb2->i_margin+(lowerq-QP_LOWER)*msb2->i_degree;
    msb2->i_upperb = msb2->i_margin+(upperq-QP_LOWER)*msb2->i_degree;

    iir_init(&msb2->iir_rqprod[RQCT_PICTYP_I], COMPLEX_I, KAPA_ONE*3/4);
    iir_init(&msb2->iir_rqprod[RQCT_PICTYP_P], COMPLEX_P, KAPA_ONE*2/4);
    acc_init(&msb2->acc_rqprod, COMPLEX_P);
    acc_init(&msb2->acc_bitcnt, msb2->i_frmbit*3/4);

#if RC_MSBR
    msb2->i_avgbpf = (int)div_s64((int64)msb2->i_btrate*msb2->d_fmrate,msb2->n_fmrate);

    if (rqcx->i_iperiod > 0)
    {
        int length = rqcx->i_iperiod;
        int weight = IFRAME_WEIGHT + PFRAME_WEIGHT*(rqcx->i_iperiod-1);
        msb2->m_bpf[0] = (int)div_s64((int64)msb2->i_btrate*msb2->d_fmrate*IFRAME_WEIGHT*length, (int64)weight*msb2->n_fmrate);
        msb2->m_bpf[1] = (int)div_s64((int64)msb2->i_btrate*msb2->d_fmrate*PFRAME_WEIGHT*length, (int64)weight*msb2->n_fmrate);
        msb2->m_bpf[2] = (int)div_s64((int64)msb2->i_btrate*msb2->d_fmrate*BFRAME_WEIGHT*length, (int64)weight*msb2->n_fmrate);
    }
    else
    {
        msb2->m_bpf[0] = (int)div_s64((int64)msb2->i_btrate*msb2->d_fmrate*IFRAME_WEIGHT, (int64)PFRAME_WEIGHT*msb2->n_fmrate);
        msb2->m_bpf[1] = (int)div_s64((int64)msb2->i_btrate*msb2->d_fmrate,msb2->n_fmrate);
        msb2->m_bpf[2] = msb2->m_bpf[1];
    }

    // Bitrate usage monitoring
    msb2->i_smooth_min        = (msb2->n_fmrate*SMOOTH_SECOND_CBR)/msb2->d_fmrate/SMOOTH_BASE;

    switch (msb2->i_method)
    {
    case RQCT_MODE_VBR:
        msb2->i_smooth_cnt = (msb2->n_fmrate*SMOOTH_SECOND_VBR)/msb2->d_fmrate/SMOOTH_BASE;
        msb2->i_vbv_target        = msb2->i_btrate * VBV_TGT_LVL_VBR  / SMOOTH_BASE;
        msb2->i_vbv_fullness_low  = msb2->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msb2->i_vbv_fullness_high = msb2->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msb2->i_vbv_buf_size      = msb2->i_btrate * VBV_BUF_SIZE_VBR / SMOOTH_BASE;
        break;
    case RQCT_MODE_CBR:
    default:
        msb2->i_smooth_cnt = (int)div_s64((int64)msb2->n_fmrate*SMOOTH_SECOND_CBR,msb2->d_fmrate*SMOOTH_BASE);
        msb2->i_vbv_target        = msb2->i_btrate * VBV_TGT_LVL_CBR  / SMOOTH_BASE;
        msb2->i_vbv_fullness_low  = msb2->i_btrate * VBV_FUL_LOW_CBR  / SMOOTH_BASE;
        msb2->i_vbv_fullness_high = msb2->i_btrate * VBV_FUL_LOW_CBR  / SMOOTH_BASE;
        msb2->i_vbv_buf_size      = msb2->i_btrate * VBV_BUF_SIZE_CBR / SMOOTH_BASE;
        break;
    }

    msb2->i_vbv_fullness      = msb2->i_vbv_target;
    msb2->i_vbv_real_fullness = msb2->i_vbv_target;

    MFE_DBG( "conf i_vbv_fullness = %d\n", msb2->i_vbv_fullness);

    msb2->i_last_avg_qs = 0;
    msb2->i_frm_nr = 0;
    msb2->i_lt_qs64 = 0;
    msb2->vbr_bit_return_stage = 0;
#endif

    return 0;
}

#if RC_MSBR
static int _msbr_get_smooth_count(msb2_rqc* msb2)
{
    int smooth_cnt = 0;
    if (msb2->i_frm_nr > msb2->i_smooth_cnt*SMOOTH_FACTOR_MIN/SMOOTH_BASE)
        smooth_cnt = msb2->i_smooth_cnt;
    else if (msb2->i_frm_nr < msb2->i_smooth_min)
        smooth_cnt = msb2->i_smooth_min;
    else
    {
        int fact_n = MAX((msb2->i_smooth_cnt - msb2->i_smooth_min), 1);
        int fact_d = MAX((msb2->i_smooth_cnt * SMOOTH_FACTOR_MIN/SMOOTH_BASE - msb2->i_smooth_min), 1);
        smooth_cnt = msb2->i_smooth_min + (msb2->i_frm_nr - msb2->i_smooth_min) * fact_n / fact_d;
    }

    return smooth_cnt;
}

static int _msbr_calculate_delta(msb2_rqc* msb2, int smooth_cnt, int frm_type)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)msb2;
    rqct_ops* rqct = (rqct_ops*)rqcx;
    int delta = 0;

    #if ( CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES != 0 )
    if(msb2->i_frm_nr  < CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES )
    {
        msb2->i_vbv_fullness = msb2->i_vbv_target;

        return 0;
    }
    #endif //CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES

    #if CFG_VBV_BUF_UNDERFLOW_RESET

    /* reset if under flow  */
    if( msb2->i_vbv_fullness < 0 )
    {
        MFE_DBG( "reset i_vbv_fullness %d= 0\n", msb2->i_vbv_fullness );
        msb2->i_vbv_fullness = 0;
    }
    #endif //CFG_VBV_BUF_UNDERFLOW_RESET

    #if CFG_VBV_BUF_OVERFLOW_RESET

    if( msb2->i_vbv_fullness > msb2->i_vbv_buf_size  )
    {
        MFE_DBG( "reset i_vbv_fullness %d= %d\n", msb2->i_vbv_fullness, msb2->i_vbv_buf_size );
        msb2->i_vbv_fullness = msb2->i_vbv_buf_size;
    }

    #endif //CFG_VBV_BUF_OVERFLOW_RESET

    if (msb2->i_method == RQCT_MODE_CBR)
    {
        delta = (int)div_s64((int64)(msb2->i_vbv_fullness-msb2->i_vbv_target)*msb2->d_fmrate,msb2->n_fmrate*SMOOTH_SECOND_CBR/SMOOTH_BASE);
        MFE_DBG( "i_vbv_fullness %d i_vbv_target %d delta = %d\n", msb2->i_vbv_fullness, msb2->i_vbv_target,delta );

        #if( CFG_RC_DELTA_CACULATE_METHOD == RDC_WEIGHTED_DIVIDED )
        {
            int   wp[MAX_FRAME_TYPE] = {IFRAME_WEIGHT,PFRAME_WEIGHT,BFRAME_WEIGHT};
            int   w                  = IFRAME_WEIGHT + PFRAME_WEIGHT*(rqcx->i_iperiod-1);
            int   n                  = rqcx->i_iperiod;

            // Multiply the ratio of the frame type
            delta = delta* n * wp[frm_type] / w;
            MFE_DBG( "Adjust delta = %d\n", delta );
        }
        #endif //CFG_RC_DELTA_CACULATE_METHOD

    }
    else if (msb2->i_method == RQCT_MODE_VBR)
    {
        delta = (msb2->i_vbv_fullness - msb2->i_vbv_target) / smooth_cnt;
        if (delta > 0 && (msb2->i_last_avg_qs*64) > msb2->i_lt_qs64)
            delta = delta>>1;  // Make it more variable bitrate to allow better quality

//when previous frame's average QP<minQP (VbrMinQP), reduce compensation bits and return to vbv buffer.
//if average QP<VbrMinQP Continuously, speed up of returning the compensation bits.
#define VBR_BIT_RETURN_MAX_STAGE (8)
        if (delta < 0)
        {
            int VbrMinQP;
            if (IS_IPIC(rqct->i_pictyp))
                VbrMinQP = msb2->i_ilowerq;
            else
                VbrMinQP = msb2->i_plowerq;

            if (qs2qp(msb2->i_avg_qs) < VbrMinQP + QP_DEGREE)
            {
                int return_bits;
                if (msb2->vbr_bit_return_stage<VBR_BIT_RETURN_MAX_STAGE)
                {
                    msb2->vbr_bit_return_stage++;   //when meet MAX_STAGE, return all the compensation bits.
                }
                return_bits = -delta * msb2->vbr_bit_return_stage / VBR_BIT_RETURN_MAX_STAGE;
                delta = return_bits;
                msb2->i_vbv_fullness += return_bits;
            }
        }
        else
        {
            msb2->vbr_bit_return_stage = 0;
        }
    }

    return delta;
}
#endif

static int msb2_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    mfe6_reg* regs = (mfe6_reg*)mjob;
    msb2_rqc* msb2 = (msb2_rqc*)rqcx;
    int qsc, bpp, pqp, dqp, lim, tgt=0;
    int uqp = 0;
    int lqp = 0;
    switch (msb2->i_method)
    {
    case RQCT_MODE_CQP:
        rqct->i_enc_qp = msb2->i_levelq;
        if (IS_IPIC(rqct->i_pictyp))
            rqct->i_enc_qp -= msb2->i_deltaq;
        break;
    case RQCT_MODE_CBR:
    case RQCT_MODE_VBR:
    default:
        lim = msb2->i_limitq;
        if (IS_IPIC(rqct->i_pictyp))
        {
            int xi = iir_data(&msb2->iir_rqprod[RQCT_PICTYP_I]);
            int xp = acc_calc(&msb2->acc_rqprod,SMOOTH_X)/SMOOTH_X;
            int ri = (int)div_s64((int64)qp2qs(QS_ACCURACY+msb2->i_deltaq)*IPR_FAC,qp2qs(QS_ACCURACY));
            int rp = (int)div_s64((int64)xp*IPR_FAC,xi);
            uqp = msb2->i_iupperq;
            lqp = msb2->i_ilowerq;
            msb2->i_budget = (int)div_s64((int64)msb2->i_gopbit*ri,(rqcx->i_iperiod*rp+ri-rp));
            bpp = _MAX((int)div_s64((int64)msb2->i_budget*BPP_FAC,msb2->i_pixels), 1);  //avoid division by zero in kernel
            qsc = xi/bpp;
            pqp = _MAX(qs2qp(qsc),msb2->i_levelq-msb2->i_deltaq);
            pqp = _MAX(lqp,_MIN(pqp,uqp));
            if( lqp>(pqp-QP_DEGREE) && lqp>LQP_HW_LIMIT)
                pqp = lqp + QP_DEGREE;
            rqct->i_enc_qp = pqp;
            tgt = msb2->i_budget/msb2->i_blocks;
            uqp = _MIN(pqp+msb2->i_blkdqp,_MIN(lim,uqp));
            lqp = _MIN(_MAX(pqp-msb2->i_blkdqp,lqp), LQP_HW_LIMIT);
            msb2->b_mbkadp = msb2->i_blkdqp>0 && pqp<lim;
            msb2->i_ipbias = 0;
        }
        else
        {
            uqp = msb2->i_pupperq;
            lqp = msb2->i_plowerq;
            pqp = QP_LOWER+(msb2->i_imbase-msb2->i_margin)/msb2->i_degree;
            dqp = qdelta((int)div_s64((int64)(msb2->i_imbits-msb2->i_imbase)*DQBIAS_FAC,msb2->i_radius));
            msb2->i_levelq = pqp;
            msb2->i_budget = msb2->i_frmbit-msb2->i_ipbias;
            msb2->i_budget = _MAX(msb2->i_budget,msb2->i_frmbit/16);
            pqp = _MAX(_MIN(uqp,pqp+dqp),lqp);
            if( lqp>(pqp-QP_DEGREE) && lqp>LQP_HW_LIMIT)
                pqp = lqp + QP_DEGREE;
            rqct->i_enc_qp = pqp;
            tgt = msb2->i_budget/msb2->i_blocks;
            uqp = _MIN(pqp+msb2->i_blkdqp,_MIN(lim,uqp));
            lqp = _MIN(_MAX(pqp-msb2->i_blkdqp,lqp), LQP_HW_LIMIT);
            msb2->b_mbkadp = msb2->i_blkdqp>0 && pqp<lim;
        }
#if RC_MSBR
        do
        {
            int frm_type = rqct->i_pictyp;
            int delta = 0;
            int init_frames = 0;

            if( rqcx->i_iperiod > 1 )
            {
                // The number of P frames in GOP is not zero.
                init_frames = 1;
            }

            #if (CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES != 0 )
            init_frames = CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES;
            #endif //CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES

            // Target frame bitcount
            if (msb2->i_frm_nr > init_frames)
            {
                int smooth_cnt = 0;
                // 1. Determine the number of future frame to compensate for current bitrate mismatch.
                smooth_cnt = _MAX(_msbr_get_smooth_count(msb2), 1);

                // 2. Calculate the bitcount that this frame should be compensate for.
                delta = _msbr_calculate_delta( msb2, smooth_cnt, frm_type);

                // 3. Finally, calculate the target bitcount.
                msb2->i_tgt_bs = msb2->m_bpf[frm_type] - delta;

                // 4. Clip the target size to 10% ~ 130%
                msb2->i_tgt_bs = MSRC_FIT_IN_RANGE( msb2->i_tgt_bs
                                                ,MSRC_PERCENT ( 10, msb2->m_bpf[frm_type])
                                                ,MSRC_PERCENT (130, msb2->m_bpf[frm_type]));
            }
            else
            {
                // The first I frame and the first P frame use the default value.
                msb2->i_tgt_bs = msb2->m_bpf[frm_type];
            }

            MFE_DBG( "Qp = %d, tg %d <%d:%d> dt %d\n", rqcx->rqct.i_enc_qp , msb2->i_tgt_bs
                                                   , msb2->i_frm_nr,  frm_type,delta);
            msb2->i_last_avg_qs = 0;
            tgt = (int)div_s64((int64)msb2->i_tgt_bs*256,msb2->i_pixels);
        }
        while(0);
#endif
        break;
    }
    mrqc_roi_draw(rqct, mjob);
    msb2->b_mbkadp = msb2->b_mbkadp && !rqcx->attr.b_dqmstat;
    regs->reg00_g_mbr_en = msb2->b_mbkadp;
    regs->reg26_s_mbr_pqp_dlimit = LF_QP_DIFF_LIMIT;
    regs->reg26_s_mbr_uqp_dlimit = UP_QP_DIFF_LIMIT;
    regs->reg00_g_qscale = rqct->i_enc_qp;
    regs->reg27_s_mbr_frame_qstep = qp2qs(rqct->i_enc_qp);
    regs->reg26_s_mbr_tmb_bits = tgt>1 ? tgt : 1;
    regs->reg2a_s_mbr_qp_min = lqp;
    regs->reg2a_s_mbr_qp_max = uqp;
    regs->reg6e_s_mbr_qstep_min = qp2qs(lqp);
    regs->reg6f_s_mbr_qstep_max = qp2qs(uqp);
    /* penalties of mbtypies */
    regs->regf2_g_i16pln_en = (0!=rqcx->attr.b_i16pln);
    regs->reg87_g_intra4_penalty = rqcx->attr.i_peni4x;
    regs->reg88_g_intra16_penalty = rqcx->attr.i_peni16;
    regs->reg88_g_inter_penalty = rqcx->attr.i_penint;
    regs->reg89_g_planar_penalty_luma = rqcx->attr.i_penYpl;
    regs->reg89_g_planar_penalty_cbcr = rqcx->attr.i_penCpl;
    return 0;
}

#define BSP_DUR (32)

static int msb2_enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    msb2_rqc* msb2 = (msb2_rqc*)rqcx;
    mfe6_reg* regs = (mfe6_reg*)mjob;
    int bpp, cpx, bps, inc, dec, dur, avq = qp2qs(rqct->i_enc_qp), bcn = mjob->i_bits;

    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= rqct->i_bitcnt/8;
    rqct->i_enc_nr++;

    if (msb2->b_mbkadp)
        avq = (avq + regs->enc_sumpqs)/msb2->i_blocks;

    regs->reg28 = regs->reg29 = 0;
    regs->regf5 = regs->regf6 = 0;
    regs->reg42 = regs->reg43 = 0;

    bpp = (int)div_s64((int64)rqct->i_bitcnt*BPP_FAC,msb2->i_pixels);
    dur = rqcx->i_iperiod;
    cpx = bpp * avq;

    if (IS_PPIC(rqct->i_pictyp))
    {
        acc_push(&msb2->acc_bitcnt, bcn);
        acc_push(&msb2->acc_rqprod, cpx);
        iir_push(&msb2->iir_rqprod[RQCT_PICTYP_P], acc_calc(&msb2->acc_rqprod,8)/8);
        msb2->i_errpro = (msb2->i_ipbias-msb2->i_frmbit+bcn);
        msb2->i_imbits += msb2->i_errpro;
        msb2->i_imbits = _MIN(msb2->i_imbits,msb2->i_upperb+msb2->i_radius);
        msb2->i_imbits = _MAX(msb2->i_imbits,msb2->i_lowerb);
        msb2->i_rebits = _MAX(msb2->i_rebits,msb2->i_imbits);
    }
    else
    {
        msb2->i_intrabit = bcn;
        iir_push(&msb2->iir_rqprod[RQCT_PICTYP_I], cpx);
        if (msb2->i_imbits > msb2->i_upperb)
            msb2->i_rebits = msb2->i_imbits;
        msb2->i_errpro = (msb2->i_rebits-msb2->i_imbits);
        msb2->i_imbits += msb2->i_errpro;
        if (dur >= 2)
            msb2->i_ipbias = (bcn - msb2->i_frmbit) / (dur-1);
        if (dur == 0)
            acc_push(&msb2->acc_bitcnt, bcn);
    }
    msb2->i_rebits += (bcn - msb2->i_frmbit);

    if (dur > 0)
        bps = acc_calc(&msb2->acc_bitcnt, dur-1) + msb2->i_intrabit;
    else
        bps = acc_calc(&msb2->acc_bitcnt, dur=BSP_DUR);
    bps = (int)div_s64((int64)bps*msb2->n_fmrate,msb2->d_fmrate*dur);

    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d bps:%8d",\
        rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp==RQCT_PICTYP_I?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt,bps);
#if 0
   //  printk("%s\n",rqct->print_line);
    printk("<%s#%04d@%c:%2d> bitcnt:%8d bps:%8d im:%8d df:%8d re:%8d\n",\
    rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp==RQCT_PICTYP_I?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt,bps,msb2->i_imbits,msb2->i_imbase,msb2->i_rebits);
#endif
    bcn = msb2->i_imbits-msb2->i_imbase;
    inc = msb2->i_frmbit/64;
    dec = -3*inc;
    bcn = _MIN(inc,_MAX(bcn,dec));
    msb2->i_imbase += bcn;
    msb2->i_imbase = _MIN(_MAX(msb2->i_imbase,msb2->i_lowerb),msb2->i_upperb);
    rqcx->i_refcnt++;

#if RC_MSBR
    do
    {
        int frm_type;
        frm_type = rqct->i_pictyp;
        // update counter
        msb2->i_frm_nr++;

        MFE_DBG( "conf i_vbv_fullness %d + bitcnt %d= ", msb2->i_vbv_fullness, rqct->i_bitcnt );

        msb2->i_vbv_fullness      += rqct->i_bitcnt;
        msb2->i_vbv_real_fullness += rqct->i_bitcnt;

        MFE_DBG( "%d\n", msbr->i_vbv_fullness );

        msb2->i_avg_qs = avq;
        msb2->i_last_avg_qs = msb2->i_avg_qs;
        // Variable bitrate
        if (msb2->i_method == RQCT_MODE_VBR)
            msb2->i_lt_qs64 += ((msb2->i_last_avg_qs*64)-msb2->i_lt_qs64) / msb2->i_frm_nr;

        /* Update VBV buffer status  */

        #if   ( CFG_VBV_FRAME_QUOTA_METHOD == VFQ_USE_AVERAGE_SIZE )

        MFE_DBG( "conf i_vbv_fullness %d - i_avgbpf %d= ", msb2->i_vbv_fullness, msb2->i_avgbpf );
        msb2->i_vbv_fullness      -= msb2->i_avgbpf;
        msb2->i_vbv_real_fullness -= msb2->i_avgbpf;

        #elif ( CFG_VBV_FRAME_QUOTA_METHOD == VFQ_USE_WEIGHTED_SIZE )

        MFE_DBG( "conf i_vbv_fullness %d - i_avgbpf %d= ", msb2->i_vbv_fullness, msb2->m_bpf[frm_type] );
        msb2->i_vbv_fullness      -= msb2->m_bpf[frm_type];
        msb2->i_vbv_real_fullness -= msb2->m_bpf[frm_type];

        #else

        msb2->i_vbv_fullness      -= msb2->i_avgbpf;
        msb2->i_vbv_real_fullness -= msb2->i_avgbpf;

        #endif //VFQ_USE_WEIGHTED_SIZE

        MFE_DBG( "%d\n", msb2->i_vbv_fullness);
    }
    while(0);
#endif

    return 0;
}
