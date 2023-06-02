
#include <linux/math64.h>

#include <mfe.h>
#include <mfe_util.h>

/************************************************************************/
/* Macros                                                               */
/************************************************************************/
#define MSBR_VER_EXT            00
#define _EXP(expr)              #expr
#define _STR(expr)              _EXP(expr)
#define RC_MODULE_NAME          "MSB3"
#define RC_MODULE_VER_MAJOR     1
#define RC_MODULE_VER_MINOR     0
#define RC_MODULE_VER_TEST      "T"_STR(MSBR_VER_EXT)
#define MSBR_NAME               RC_MODULE_NAME
#define MSBR_VER_MJR            RC_MODULE_VER_MAJOR
#define MSBR_VER_MNR            RC_MODULE_VER_MINOR

/* Debug related configuration */
#define DBG_PRINT_ENABLE                        0

/* General calculations */
#ifndef MSRC_MAX
#define MSRC_MAX(a,b)   (((a)>(b))?(a):(b))
#endif
#ifndef MSRC_MIN
#define MSRC_MIN(a,b)   (((a)<(b))?(a):(b))
#endif
#ifndef MSRC_FIT_IN_RANGE
#define MSRC_FIT_IN_RANGE(a,low,high)   MSRC_MIN(MSRC_MAX((a),(low)),(high))
#endif

#ifndef MSRC_PERCENT
#define MSRC_PERCENT(percent,a)   ( (percent) * (a) / 100 )
#endif

/* Debugging related macro */
#if DBG_PRINT_ENABLE
#define MSBR_DBG                                printk
#else
#define MSBR_DBG(...)
#endif

/* Static assert */
#define MSBR_SASSERT(EXP,STR)                   _Static_assert(EXP,STR)
#define MSBR_ASSERT                             assert
#define MFE_ASSERT(p)

/************************************************************************/
/* Configuration                                                        */
/************************************************************************/

/* Skip the first N frames for VBV calculation  */
#define CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES       50

/* The feature to reset the fullness value if fullness overflow or underflow */
/*   1 - Enable                                                              */
/*   0 - Disable                                                             */
#define CFG_VBV_BUF_OVERFLOW_RESET              1
#define CFG_VBV_BUF_UNDERFLOW_RESET             1

/* The selection of the rqcx of the new Qs                                   */
#define LQS_USE_LAST_SET_QS                     0   /* The last frame encode qs setting */
#define LQS_USE_LAST_AVG_ENC_QS                 1   /* Feed back from HW */
#define LQS_USE_LAST_FRM_TYPE_SET_QS            2   /* The last same type frame encode qs setting  */
#define CFG_LAST_QS_SELECTION                   LQS_USE_LAST_SET_QS

/* The choice of the VBV frame quota selection                               */
#define VFQ_USE_AVERAGE_SIZE                    0   /*   The average size of each frame            */
#define VFQ_USE_WEIGHTED_SIZE                   1   /* The weighted size rqcx on the setting of    */
                                                    /* IFRAME_WEIGHT, PFRAME_WEIGHT, BFRAME_WEIGHT */
                                                    /* and Gop size                                */
#define CFG_VBV_FRAME_QUOTA_METHOD              VFQ_USE_WEIGHTED_SIZE

#define CFG_CBR_IFRAME_CQP                      0   /* 0 - Disable, 1 - Enable */
#define CFG_CBR_PFRAME_CQP                      0   /* 0 - Disable, 1 - Enable */

#define RDC_WEIGHTED_DIVIDED                    0
#define RDC_AVERAGE_DIVIDED                     1
#define CFG_RC_DELTA_CACULATE_METHOD            RDC_WEIGHTED_DIVIDED

#define QS_SHIFT_FACTOR                         5

#define UP_QP_DIFF_LIMIT                        3
#define LF_QP_DIFF_LIMIT                        3
// Spec limitation is [1,51]
#define I_FRAME_INIT_QP                         (30)
#define I_FRAME_MAX_QP                          (44)
#define I_FRAME_MIN_QP                          (15)

#define P_FRAME_INIT_QP                         (35)
#define P_FRAME_MAX_QP                          (48)
#define P_FRAME_MIN_QP                          (15)

#define B_FRAME_INIT_QP                         (P_FRAME_INIT_QP)
#define B_FRAME_MAX_QP                          (P_FRAME_MAX_QP)
#define B_FRAME_MIN_QP                          (P_FRAME_MIN_QP)

#define I_FRAME_MARGIN_QP                       (0)
#define P_FRAME_MARGIN_QP                       (0)
#define B_FRAME_MARGIN_QP                       (0)

// Compute qs
#define SMOOTH_BASE                             10       // 1 sec = 10* 100ms
#define SMOOTH_PERIOD                           10       // in second
#define INIT_QP_OFFSET                          18
#define INIT_QP_FACTOR                          72
#define MIN_INIT_QP                             I_FRAME_MIN_QP
#define MAX_INIT_QP                             I_FRAME_MAX_QP

#define THR_QP_FRAMESKIP                        (40)

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



MSBR_SASSERT( IFRAME_WEIGHT>0 && PFRAME_WEIGHT > 0, "The weighting of I frame or P frame should be bigger than zero" );

/************************************************************************/
/* Constant                                                             */
/************************************************************************/

// Operation Methods
#define CONST_QP                                0
#define CBR                                     1
#define VBR                                     2
#define CONSTRAINED_VBR                         3
#define STATIC_CBR                              4


// For Constrained VBR
#define MAX_GAUGE_SIZE                          64

#define FRAME_TYPE_I                            0   // I, P, B 3 types
#define FRAME_TYPE_P                            1   // I, P, B 3 types
#define FRAME_TYPE_B                            2   // I, P, B 3 types
#define MAX_FRAME_TYPE                          3   // I, P, B 3 types

/************************************************************************/
/* Local structures                                                     */
/************************************************************************/

//#define LOG_MSG

typedef struct mfe_msb3
{
    mfe_rctl    rqcx;
    int     i_method;
    short   i_leadqp, i_deltaq;
    int     i_btrate;
    int     i_pixels;
    int     n_fmrate;
    int     d_fmrate;
    int     i_fmrate;
    int     i_tgt_mb;
    int     i_avg_qs;
    // Derived variables
    int     i_avgbpf;
    int     m_bpf[MAX_FRAME_TYPE];                  // I, P, B
    /* coded frame counter */
    int     i_gbl_frm_nr;
    int     i_frm_nr;
    int     i_frm_count[MAX_FRAME_TYPE];
    int64   i_totbit;
    int64   i_frm_totbits[MAX_FRAME_TYPE];
    /* integral part of frame-rate */
    int     i_fps;
    int     i_maxbps;
    /* this-frame params */
    int     i_this_enc_qs;
    int     i_this_max_qp;
    int     i_this_min_qp;
    int     i_this_max_qs;
    int     i_this_min_qs;
    /* last-frame status */
    int     i_last_avg_qs;
    int     i_last_frm_bs;
    int     i_last_tgt_bs;
    int     i_last_enc_qp;
    int     i_last_enc_qs;
    int     i_last_qp[MAX_FRAME_TYPE];
    int     i_last_qs[MAX_FRAME_TYPE];
    int     i_last_tgt_size[MAX_FRAME_TYPE];
    int     i_last_frm_size[MAX_FRAME_TYPE];

    /* init qp values */
    int     i_init_qp[MAX_FRAME_TYPE];               // Max  qp value for each frame type
    int     i_max_qp[MAX_FRAME_TYPE];               // Max  qp value for each frame type
    int     i_min_qp[MAX_FRAME_TYPE];               // Mini qp value for each frame type
    int     i_margin_qp[MAX_FRAME_TYPE];            // The margin of QP setting between the boundaries.

    /* bitrate usage compensation */
    int     i_vbv_buf_size;                         // Rate control buffer size
    int     i_vbv_fullness_low;                     // Rate control buffer fullness low bound
    int     i_vbv_fullness_high;                    // Rate control buffer fullness high bound
    int     i_vbv_fullness;                         // Rate control buffer fullness
    int     i_vbv_real_fullness;                    // Rate control buffer fullness
    int     i_vbv_target;                           // Rate control buffer target fullness
    int     i_vbv_balance;

    int     i_smooth_cnt;
    int     i_smooth_min;
    /* variable bitrate */
    int     i_lt_qs64;
    // model parameters
    int     i_tgt_bs;                               // target number of bits of current frame
    /* only for Constrained VBR */
    int     i_max_offset;
    int     i_gauge_bgt[MAX_GAUGE_SIZE];
    int     i_gauge_cnt;
    int     i_gauge_idx;
    int     i_gauge_bps;
    int     i_frozen;
    int     i_thr_qp_frameskip;
    int     i_frameskip;                            // original MFE-RC: pic_done's output
    /*start new parameters */
    int     i_levelq;
    int     i_smooth;
    int     i_pixbit;
    int     i_frmbit;
    int     i_budget;
    int     i_ipbias;
    int     i_btbias;
    int     i_gopbit;
    int     i_bucket;
    int     i_missed;
    int     i_bitpos;
    int     i_dqbias;
#define PICTYPES        (2)
#define COMPLEXITY_I    (1<<24)
#define COMPLEXITY_P    (1<<20)
    iir_t   iir_rqprod[PICTYPES];
    acc_t   acc_rqprod;
    acc_t   acc_bitcnt;
    /*endof new parameters */
} mfe_msb3;

typedef struct
{
	int bps;
	int fps_by_10;
	int pixels;
	int qp_bound;
} qp_bound;

#define BOUND_NUMERIC_SHIFT	16

// I frame QP upper bound
static const qp_bound qp_i_upper[]= {
	{  256000, 250, 1920*1080, 51 },
	{  512000, 250, 1920*1080, 44 },
	{ 1000000, 250, 1920*1080, 43 },
	{ 2000000, 250, 1920*1080, 39 },
	{ 3000000, 250, 1920*1080, 39 }
};

// P frame QP upper bound
static const qp_bound qp_p_upper[]= {
	{  256000, 250, 1920*1080, 51 },
	{  512000, 250, 1920*1080, 44 },
	{ 1000000, 250, 1920*1080, 43 },
	{ 2000000, 250, 1920*1080, 40 },
	{ 3000000, 250, 1920*1080, 40 }
};

#define SIZE_OF_I_QP_UPP_BOUND      sizeof( qp_i_upper) / sizeof(qp_bound);
#define SIZE_OF_P_QP_UPP_BOUND      sizeof( qp_p_upper) / sizeof(qp_bound);

#define BOUND_FP_SHIFT              16  // just for calculation

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
static int qs2qp(int qs)
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

/************************************************************************/
/* Local prototypes                                                     */
/************************************************************************/

static int  msb3_seq_sync(rqct_ops*);
static int  msb3_seq_done(rqct_ops*);
static int  msb3_seq_conf(rqct_ops*);
static int  msb3_enc_conf(rqct_ops*, mhve_job*);
static int  msb3_enc_done(rqct_ops*, mhve_job*);
static void msb3_release(rqct_ops*);

static int  _msb3_compute_qs(mfe_msb3* msb3, int frm_type);
static int  _msb3_qp_init(mfe_msb3* msb3);
static int  _msb3_qp_update_max(mfe_msb3* msb3, int frm_type);
static int  _msb3_qp_fit_in_range(mfe_msb3* msb3, int frm_type);
static int  _msb3_get_qp_upper(int frm_type, int bps, int fps_by_10, int pixels );

static int  qp2qs(int QP);
static int  qs2qp(int qsx32);

/************************************************************************/
/* Functions                                                            */
/************************************************************************/


static void
msb3_print_version(void)
{
    printk("RATECTL_VER: [%s] %d.%d %s\n", RC_MODULE_NAME, RC_MODULE_VER_MAJOR,
                                           RC_MODULE_VER_MINOR, RC_MODULE_VER_TEST );
}

char* msb3_describe(void)
{
    static char line[64];
    sprintf(line, "%s@v%d.%d-%02d:enhanced msbr.",MSBR_NAME,MSBR_VER_MJR,MSBR_VER_MNR,MSBR_VER_EXT);
    return line;
}

void* msb3_allocate(void)
{
    rqct_ops* rqct;

    if (NULL != (rqct = MEM_ALLC(sizeof(mfe_msb3))))
    {
        mfe_rctl* rqcx = (mfe_rctl*)rqct;
        MEM_COPY(rqct->name, MSBR_NAME, 5);
        rqct->seq_sync = msb3_seq_sync;
        rqct->seq_done = msb3_seq_done;
        rqct->set_rqcf = mrqc_set_rqcf;
        rqct->get_rqcf = mrqc_get_rqcf;
        rqct->seq_conf = msb3_seq_conf;
        rqct->enc_buff = mrqc_enc_buff;
        rqct->enc_conf = msb3_enc_conf;
        rqct->enc_done = msb3_enc_done;
        rqct->release = msb3_release;

        rqcx->attr.i_method = RQCT_METHOD_CQP;
        rqcx->attr.i_leadqp =-1;
        rqcx->attr.i_deltaq = 3;
        rqcx->attr.i_upperq = 48;
        rqcx->attr.i_lowerq = 12;
        rqcx->attr.i_pict_w = 0;
        rqcx->attr.i_pict_h = 0;
        rqcx->attr.i_btrate = 0;
        rqcx->attr.n_fmrate =30;
        rqcx->attr.d_fmrate = 1;
        rqcx->i_config = 0;
        rqcx->i_pcount = 0;
        rqcx->i_period = 0;
    }

    return rqct;
}


static void msb3_release(rqct_ops* rqct)
{
    MEM_FREE(rqct);
}

static int msb3_seq_done(rqct_ops* rqct)
{

    /* Debug print */
    #if DBG_PRINT_ENABLE
    {
        mfe_msb3* msb3 = (mfe_msb3*)rqct;

        MSBR_DBG( "m_bpf %d %d %d \n", msb3->m_bpf[0], msb3->m_bpf[1], msb3->m_bpf[2] );
    }
    #endif

    /* call seq_done() */
    return 0;
}

static int msb3_seq_sync(rqct_ops* rqct)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    rqcx->i_pcount = 0;
    return 0;
}

static int msb3_seq_conf(rqct_ops* rqct)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_msb3* msb3 = (mfe_msb3*)rqcx;
    int i;

    msb3_print_version();

    msb3->i_method = (int)rqcx->attr.i_method;
    msb3->i_leadqp = rqcx->attr.i_leadqp;
    msb3->i_deltaq = rqcx->attr.i_deltaq;
    msb3->i_btrate = rqcx->attr.i_btrate;
    msb3->i_pixels = (int)rqcx->attr.i_pict_w * (int)rqcx->attr.i_pict_h;
    msb3->n_fmrate = (int)rqcx->attr.n_fmrate;
    msb3->d_fmrate = (int)rqcx->attr.d_fmrate;
    msb3->i_fmrate = msb3->n_fmrate / msb3->d_fmrate;
    msb3->i_maxbps = 0;

    rqcx->i_period = rqcx->attr.i_period;

    if (msb3->i_method == RQCT_METHOD_CBR)
    {
        msb3->i_levelq = rqcx->attr.i_leadqp;
        msb3->i_smooth = rqcx->i_period;
        msb3->i_frmbit = (int)div_s64((int64)msb3->i_btrate*msb3->d_fmrate,msb3->n_fmrate);
        msb3->i_pixbit = (int)div_s64((int64)msb3->i_frmbit*BPP_FAC,msb3->i_pixels);
        msb3->i_gopbit = rqcx->i_period*msb3->i_frmbit;
        msb3->i_bucket = msb3->i_frmbit*msb3->i_smooth;
        msb3->i_missed = msb3->i_bucket/4;
        msb3->i_bitpos = msb3->i_bucket/4;
        msb3->i_dqbias = msb3->i_levelq-42;
        iir_init(&msb3->iir_rqprod[RQCT_PICTYP_I], COMPLEXITY_I, KAPA_ONE*3/4);
        iir_init(&msb3->iir_rqprod[RQCT_PICTYP_P], COMPLEXITY_P, KAPA_ONE*2/4);
        acc_init(&msb3->acc_rqprod, COMPLEXITY_P);
        acc_init(&msb3->acc_bitcnt, msb3->i_frmbit*3/4);
        if (msb3->i_levelq < 0)
            msb3->i_levelq = 30;
        rqcx->i_pcount = rqcx->i_period;
        return 0;
    }

    // more bitrate checking
    if (msb3->i_method == CONSTRAINED_VBR)
    {
        if (msb3->i_maxbps == 0)
            msb3->i_maxbps = (int)((msb3->i_btrate*14)/10);

        msb3->i_max_offset = (int)div_s64((int64)(msb3->i_maxbps-msb3->i_btrate)*msb3->d_fmrate,msb3->n_fmrate);

        if (!(rqcx->i_config&RCTL_CFG_MB_DQP))
            msb3->i_max_offset = (msb3->i_max_offset) >> 2;
    }
    else
        msb3->i_maxbps = 0;  // Don't care

    msb3->i_avgbpf = (int)div_s64((int64)msb3->i_btrate*msb3->d_fmrate,msb3->n_fmrate);

    if (rqcx->i_period > 0)
    {
        int length = rqcx->i_period;
        int weight = IFRAME_WEIGHT + PFRAME_WEIGHT*(rqcx->i_period-1);
        msb3->m_bpf[0] = (int)div64_s64((int64)msb3->i_btrate*msb3->d_fmrate*IFRAME_WEIGHT*length, (int64)weight*msb3->n_fmrate);
        msb3->m_bpf[1] = (int)div64_s64((int64)msb3->i_btrate*msb3->d_fmrate*PFRAME_WEIGHT*length, (int64)weight*msb3->n_fmrate);
        msb3->m_bpf[2] = (int)div64_s64((int64)msb3->i_btrate*msb3->d_fmrate*BFRAME_WEIGHT*length, (int64)weight*msb3->n_fmrate);

    }
    else
    {
        msb3->m_bpf[0] = (int)div64_s64((int64)msb3->i_btrate*msb3->d_fmrate*IFRAME_WEIGHT, (int64)PFRAME_WEIGHT*msb3->n_fmrate);
        msb3->m_bpf[1] = (int)div64_s64((int64)msb3->i_btrate*msb3->d_fmrate,msb3->n_fmrate);
        msb3->m_bpf[2] = msb3->m_bpf[1];
    }

    _msb3_qp_init(msb3);

    // Init frame number in Gop
    rqcx->i_pcount = rqcx->i_period;
    msb3->i_totbit = 0;

    // Bitrate usage monitoring
    msb3->i_smooth_min        = (msb3->n_fmrate*SMOOTH_SECOND_CBR)/msb3->d_fmrate/SMOOTH_BASE;

    switch (msb3->i_method)
    {
    case RQCT_METHOD_VBR:
        msb3->i_smooth_cnt = (msb3->n_fmrate*SMOOTH_SECOND_VBR)/msb3->d_fmrate/SMOOTH_BASE;

        msb3->i_vbv_target        = msb3->i_btrate * VBV_TGT_LVL_VBR  / SMOOTH_BASE;
        msb3->i_vbv_fullness_low  = msb3->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msb3->i_vbv_fullness_high = msb3->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msb3->i_vbv_buf_size      = msb3->i_btrate * VBV_BUF_SIZE_VBR / SMOOTH_BASE;

        break;
    case CONSTRAINED_VBR:
        msb3->i_smooth_cnt = (msb3->n_fmrate*SMOOTH_SECOND_CVBR)/msb3->d_fmrate/SMOOTH_BASE;
        msb3->i_gauge_bgt[0] = msb3->m_bpf[0];
        for (i = 1; i < MAX_GAUGE_SIZE; i++)
            msb3->i_gauge_bgt[i] = msb3->m_bpf[1];
        msb3->i_gauge_cnt = msb3->i_fmrate;
        msb3->i_gauge_idx = 0;
        msb3->i_gauge_bps = (int)div_s64((int64)msb3->i_btrate*msb3->d_fmrate*msb3->i_gauge_cnt,msb3->n_fmrate);

        msb3->i_vbv_target        = msb3->i_btrate * VBV_TGT_LVL_VBR  / SMOOTH_BASE;
        msb3->i_vbv_fullness_low  = msb3->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msb3->i_vbv_fullness_high = msb3->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msb3->i_vbv_buf_size      = msb3->i_btrate * VBV_BUF_SIZE_VBR / SMOOTH_BASE;

        break;
    case RQCT_METHOD_CBR:
    default:

        msb3->i_smooth_cnt = (int)div_s64((int64)msb3->n_fmrate*SMOOTH_SECOND_CBR,msb3->d_fmrate*SMOOTH_BASE);
        msb3->i_vbv_target        = msb3->i_btrate * VBV_TGT_LVL_CBR  / SMOOTH_BASE;
        msb3->i_vbv_fullness_low  = msb3->i_btrate * VBV_FUL_LOW_CBR  / SMOOTH_BASE;
        msb3->i_vbv_fullness_high = msb3->i_btrate * VBV_FUL_LOW_CBR  / SMOOTH_BASE;
        msb3->i_vbv_buf_size      = msb3->i_btrate * VBV_BUF_SIZE_CBR / SMOOTH_BASE;

        break;
    }

    msb3->i_vbv_fullness      = msb3->i_vbv_target;
    msb3->i_vbv_real_fullness = msb3->i_vbv_target;

    MSBR_DBG( "conf i_vbv_fullness = %d\n", msb3->i_vbv_fullness);

    msb3->i_thr_qp_frameskip = THR_QP_FRAMESKIP;
    msb3->i_frm_nr = 0;
    msb3->i_last_avg_qs = msb3->i_last_frm_bs = 0;
    msb3->i_lt_qs64 = 0;

    // Init var
    for( i=0; i<MAX_FRAME_TYPE; i++ )
    {

        msb3->i_last_enc_qs      = 0;
        msb3->i_last_enc_qp      = 0;

        msb3->i_last_qp[i]       = 0;
        msb3->i_last_qs[i]       = 0;
        msb3->i_last_tgt_size[i] = 0;
        msb3->i_last_frm_size[i] = 0;
        /*
        msb3->i_last_qp[i]       = msb3->i_init_qp[i];
        msb3->i_last_qs[i]       = qp2qs( msb3->i_init_qp[i]);
        msb3->i_last_tgt_size[i] = msb3->m_bpf[i];
        msb3->i_last_frm_size[i] = msb3->m_bpf[i];
        */
    }

    return 0;
}

/*
    Get smooth count
*/
static int _msb3_get_smooth_count(mfe_msb3* msb3)
{
    int smooth_cnt = 0;
    if (msb3->i_frm_nr > msb3->i_smooth_cnt*SMOOTH_FACTOR_MIN/SMOOTH_BASE)
        smooth_cnt = msb3->i_smooth_cnt;
    else if (msb3->i_frm_nr < msb3->i_smooth_min)
        smooth_cnt = msb3->i_smooth_min;
    else
    {
        int fact_n = msb3->i_smooth_cnt - msb3->i_smooth_min;
        int fact_d = msb3->i_smooth_cnt * SMOOTH_FACTOR_MIN/SMOOTH_BASE - msb3->i_smooth_min;
        smooth_cnt = msb3->i_smooth_min + (msb3->i_frm_nr - msb3->i_smooth_min) * fact_n / fact_d;
    }

    return smooth_cnt;
}

static int _msb3_calculate_delta(mfe_msb3* msb3, int smooth_cnt, int frm_type)
{
    mfe_rctl* rqcx = (mfe_rctl*)msb3;

    int delta = 0;


    #if ( CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES != 0 )
    if(msb3->i_frm_nr  < CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES )
    {
        msb3->i_vbv_fullness = msb3->i_vbv_target;

        return 0;
    }
    #endif //CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES

    #if CFG_VBV_BUF_UNDERFLOW_RESET

    /* reset if under flow  */
    if( msb3->i_vbv_fullness < 0 )
    {
        MSBR_DBG( "reset i_vbv_fullness %d= 0\n", msb3->i_vbv_fullness );
        msb3->i_vbv_fullness = 0;
    }
    #endif //CFG_VBV_BUF_UNDERFLOW_RESET

    #if CFG_VBV_BUF_OVERFLOW_RESET

    if( msb3->i_vbv_fullness > msb3->i_vbv_buf_size  )
    {
        MSBR_DBG( "reset i_vbv_fullness %d= %d\n", msb3->i_vbv_fullness, msb3->i_vbv_buf_size );
        msb3->i_vbv_fullness = msb3->i_vbv_buf_size;
    }

    #endif //CFG_VBV_BUF_OVERFLOW_RESET

    if (msb3->i_method == RQCT_METHOD_CBR)
    {
        delta = (int)div_s64((int64)(msb3->i_vbv_fullness-msb3->i_vbv_target)*msb3->d_fmrate,msb3->n_fmrate*SMOOTH_SECOND_CBR/SMOOTH_BASE);
        MSBR_DBG( "i_vbv_fullness %d i_vbv_target %d delta = %d\n", msb3->i_vbv_fullness, msb3->i_vbv_target,delta );

        #if( CFG_RC_DELTA_CACULATE_METHOD == RDC_WEIGHTED_DIVIDED )
        {
            int   wp[MAX_FRAME_TYPE] = {IFRAME_WEIGHT,PFRAME_WEIGHT,BFRAME_WEIGHT};
            int   w                  = IFRAME_WEIGHT + PFRAME_WEIGHT*(rqcx->i_period-1);
            int   n                  = rqcx->i_period;

            // Multiply the ratio of the frame type
            delta = delta* n * wp[frm_type] / w;
            MSBR_DBG( "Adjust delta = %d\n", delta );
        }
        #endif //CFG_RC_DELTA_CACULATE_METHOD

    }
    else if (msb3->i_method == CONSTRAINED_VBR)
    {
        delta = (msb3->i_vbv_fullness - msb3->i_vbv_target) / smooth_cnt;
        if (delta < -msb3->i_max_offset)
            delta = -msb3->i_max_offset;
    }
    else if (msb3->i_method == RQCT_METHOD_VBR)
    {
        delta = (msb3->i_vbv_fullness - msb3->i_vbv_target) / smooth_cnt;
        if (delta > 0 && (msb3->i_last_avg_qs*64) > msb3->i_lt_qs64)
            delta = delta>>1;  // Make it more variable bitrate to allow better quality
    }

    return delta;
}


/* Return target QPStep */
static int _msb3_compute_qs(mfe_msb3* msb3, int frm_type)
{
    mfe_rctl* rqcx = &msb3->rqcx;
    int newqs = 0;
    int adjust;
    int bitrate = msb3->i_btrate;
    int fps_den = msb3->d_fmrate;
    int fps_num = msb3->n_fmrate;
    int mb_numb = msb3->i_pixels>>8;

    // For the very first frame, guess one qp!
    if (msb3->i_frm_nr  == 0           ||
        msb3->i_last_qp[frm_type] == 0    )
    {
        int bpMBK, newqp;
        if (msb3->i_method == RQCT_METHOD_CQP)
        {
            newqp = msb3->i_leadqp;
            newqs = qp2qs(newqp);  // So that frame qp will be exactly msb3->i_qp
        }
        else
        {
            // bits per MB
            bpMBK = (int)div_s64((int64)bitrate*fps_den*100,mb_numb*fps_num)+1;
            newqp = INIT_QP_OFFSET + ( ( INIT_QP_FACTOR*100) / (bpMBK) );

            if (newqp < MIN_INIT_QP)
                newqp = MIN_INIT_QP;
            if (newqp > MAX_INIT_QP)
                newqp = MAX_INIT_QP;
            newqs = newqp << QS_SHIFT_FACTOR;
        }

        msb3->i_frozen = newqp >> 1;

        if (msb3->i_frozen > 15)
            msb3->i_frozen = 15;

        return newqs;
    }

    if (msb3->i_method == RQCT_METHOD_CQP)
        return qp2qs(rqcx->rqct.i_enc_qp);

    if (msb3->i_method  == RQCT_METHOD_CBR)
    {

#if 1
        int dqs;
        int last_size,tgt_size;

        last_size = msb3->i_last_frm_size[frm_type];

        #if   (CFG_LAST_QS_SELECTION == LQS_USE_LAST_SET_QS          )
        newqs     = msb3->i_last_enc_qs;
        #elif (CFG_LAST_QS_SELECTION == LQS_USE_LAST_FRM_TYPE_SET_QS )
        newqs     = msb3->i_last_qs[frm_type];
        #elif (CFG_LAST_QS_SELECTION == LQS_USE_LAST_AVG_ENC_QS      )
        newqs = msb3->i_last_avg_qs;
        #else
        newqs     = msb3->i_last_enc_qs;
        #endif

        dqs       = newqs / 8;
        tgt_size  = msb3->i_tgt_bs;

        MSBR_DBG( "newqs = %d dqs = %d last_size %d tgt_size %d\n", newqs, dqs, last_size, tgt_size);

        if( tgt_size > MSRC_PERCENT( 160, last_size) )
        {
            newqs = newqs - dqs*2;
            MSBR_DBG( "newqs-- = %d \n", newqs );
        }
        else if( tgt_size > MSRC_PERCENT( 113, last_size) )
        {
            newqs = newqs - dqs*1;
            MSBR_DBG( "newqs- = %d \n", newqs );
        }
        else if( tgt_size < MSRC_PERCENT( 60, last_size) )
        {
            newqs = newqs + dqs*2;
            MSBR_DBG( "newqs++ = %d \n", newqs );
        }
        else if( tgt_size < MSRC_PERCENT( 87, last_size) )
        {
            newqs = newqs + dqs*1;
            MSBR_DBG( "newqs+ = %d \n", newqs );
        }
        else
        {
            MSBR_DBG( "newqs~ = %d \n", newqs );
        }

#else
        int frm_cnt = msb3->i_frm_nr;
        int frames_left;
        int buf_rest_pic;
        int64 buf_rest;

        int dqp;
        buf_rest = div_s64((int64)frm_cnt*fps_den*bitrate,fps_num);
        buf_rest+= (SMOOTH_PERIOD*msb3->i_btrate) - msb3->i_totbit;
        newqs = msb3->i_last_avg_qs;
        frames_left = (SMOOTH_PERIOD * msb3->n_fmrate) / msb3->d_fmrate;
        buf_rest_pic = (int)div_s64(buf_rest,frames_left);
        dqp = msb3->i_last_avg_qs/8;

        if (msb3->i_last_frm_bs > (buf_rest_pic*9)>>3)
            newqs = msb3->i_last_avg_qs + dqp;
        else if (msb3->i_last_frm_bs < (buf_rest_pic*7)>>3)
            newqs = msb3->i_last_avg_qs - dqp;
#endif

    }
    else if (msb3->i_method == CONSTRAINED_VBR)
    {
        int lo_qs, hi_qs;
        adjust = msb3->i_lt_qs64>>2;
        lo_qs = (msb3->i_lt_qs64 - adjust) >> 6;
        hi_qs = (msb3->i_lt_qs64 + adjust) >> 6;
        if (rqcx->i_period > 1 && IS_IPIC(rqcx->rqct.i_pictyp))
        {
            newqs = msb3->i_last_avg_qs;
            if (msb3->i_gauge_bps < msb3->i_btrate)
                newqs = newqs-(1<<QS_SHIFT_FACTOR);
            newqs = MSRC_MAX(( 1<<QS_SHIFT_FACTOR),newqs);
            newqs = MSRC_MIN((12<<QS_SHIFT_FACTOR),newqs);
        }
        else
        {
            //int64 tmp = div_s64((int64)msb3->i_lt_qs64 * div_s64(msb3->i_totbit,msb3->i_frm_nr), msb3->i_tgt_bs) ;
            //MFE_ASSERT((tmp >> 6) < (1<<31));
            newqs = (int)(div_s64((int64)msb3->i_lt_qs64 * div_s64(msb3->i_totbit,msb3->i_frm_nr),msb3->i_tgt_bs)>>6) ;
            if (msb3->i_last_frm_bs>msb3->i_last_tgt_bs)
            {
                adjust = ((msb3->i_last_frm_bs-msb3->i_last_tgt_bs)/msb3->i_max_offset) + (1<<QS_SHIFT_FACTOR);
                if (adjust > (3<<QS_SHIFT_FACTOR))
                    adjust = (3<<QS_SHIFT_FACTOR);
                if ((msb3->i_last_avg_qs*64) > msb3->i_lt_qs64)
                {
                    // Danger! Make it more aggressive
                    hi_qs = msb3->i_last_avg_qs + adjust;
                    newqs = msb3->i_last_avg_qs + adjust;
                }
                else
                {
                    hi_qs += adjust;
                    newqs += adjust;
                }
            }
            else if (msb3->i_gauge_bps > msb3->i_btrate)
            {
                if (msb3->i_last_avg_qs > newqs)
                    newqs = msb3->i_last_avg_qs;
                if (msb3->i_lt_qs64 > (newqs*64))
                    newqs = msb3->i_lt_qs64/64;
            }
            else
            {
                if ((newqs<<6) >= msb3->i_lt_qs64)
                    newqs = (msb3->i_lt_qs64/64)-1;
                if (msb3->i_vbv_target>msb3->i_vbv_fullness)
                {
                    adjust = (msb3->i_vbv_target - msb3->i_vbv_fullness) / (int)msb3->i_btrate;
                    newqs -= adjust;
                }
            }
        }
        newqs = MSRC_MIN(hi_qs, newqs);
        newqs = MSRC_MAX(lo_qs, newqs);
    }
    else if (msb3->i_method == RQCT_METHOD_VBR)
    {
        int lo_qs, hi_qs;
        if (rqcx->i_period > 1 && IS_IPIC(rqcx->rqct.i_pictyp))
        {
            newqs = msb3->i_last_avg_qs;
            if ((msb3->i_last_avg_qs*64) > msb3->i_lt_qs64)
                newqs = newqs - (1<<QS_SHIFT_FACTOR);
        }
        else
        {
            int adj_ltq;
            if (msb3->i_frm_nr >= msb3->i_fmrate || rqcx->i_period == 1)
            {
                //int64 tmp = div_s64((int64)msb3->i_lt_qs64*div_s64(msb3->i_totbit,msb3->i_frm_nr),msb3->i_avgbpf);
                //MFE_ASSERT((tmp >> 6) < (1<<31));
                adj_ltq = (int)(div_s64((int64)msb3->i_lt_qs64*div_s64(msb3->i_totbit,msb3->i_frm_nr),msb3->i_avgbpf)>>6);
            }
            else
            {
                adj_ltq = msb3->i_lt_qs64/64;   // Wait for stabilization
            }
            MFE_ASSERT(msb3->i_tgt_bs>0);
            newqs = (adj_ltq * msb3->i_avgbpf) / msb3->i_tgt_bs;

            adjust = MSRC_MAX((2<<QS_SHIFT_FACTOR),adj_ltq>>2);
            lo_qs = adj_ltq - adjust;
            hi_qs = adj_ltq + adjust;

            if (msb3->i_last_frm_bs > msb3->i_last_tgt_bs)
            {
                adjust = (int)(msb3->i_last_frm_bs/msb3->i_last_tgt_bs);
                if (adjust > 2)
                    adjust = 2;
                hi_qs += adjust;
            }

            if (msb3->i_avgbpf > msb3->i_tgt_bs)
                newqs = MSRC_MIN(hi_qs,newqs);
            else
                newqs = MSRC_MAX(lo_qs,newqs);
        }
    }

    return newqs;
}


static int msb3_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_regs* regs = (mfe_regs*)mjob;
    mfe_msb3* msb3 = (mfe_msb3*)rqcx;
    int frm_type;
    int delta = 0;
    int init_frames = 0;

    if (msb3->i_method == RQCT_METHOD_CBR)
    {
        do
        {
            int qstep, bppix, btpos, dqoff;
            if (IS_IPIC(rqct->i_pictyp))
            {
                int Xi = iir_data(&msb3->iir_rqprod[RQCT_PICTYP_I]);
                int Xp = acc_calc(&msb3->acc_rqprod, rqcx->i_period) / rqcx->i_period;
                int Ri = (int)div_s64((int64)qp2qs(40+msb3->i_deltaq)*IPR_FAC,qp2qs(40));
                int Rp = (int)div_s64((int64)Xp*IPR_FAC,Xi);
                msb3->i_budget = (int)div_s64((int64)msb3->i_gopbit*Ri,(rqcx->i_period*Rp+Ri-Rp));
                bppix = (int)div_s64((int64)msb3->i_budget*BPP_FAC,msb3->i_pixels);
                qstep = Xi/bppix;
                msb3->i_ipbias = 0;
                msb3->i_btbias = 0;
                if (rqcx->i_period > 1)
                    msb3->i_ipbias = (msb3->i_budget-msb3->i_frmbit)/(rqcx->i_period-1);
                rqct->i_enc_qp = qs2qp(qstep);
                break;
            }
            btpos = msb3->i_missed;
            dqoff = qs2qp(((int)div_s64((int64)btpos*qp2qs(46),msb3->i_bucket)))-34;
            btpos-= msb3->i_bucket/4;
            msb3->i_budget = msb3->i_frmbit-msb3->i_ipbias-(btpos/msb3->i_smooth);
            msb3->i_budget = _MAX(msb3->i_budget,msb3->i_frmbit/16);
            msb3->i_levelq+= dqoff-msb3->i_dqbias;
            msb3->i_dqbias = dqoff;
            rqct->i_enc_qp = _MIN(51,msb3->i_levelq);
            msb3->i_dqbias+= rqct->i_enc_qp-msb3->i_levelq;
            msb3->i_levelq = rqct->i_enc_qp;
        }
        while (0);
        mrqc_roi_draw(rqct, mjob);
        regs->reg00_g_mbr_en = 0;
        regs->reg26_s_mbr_pqp_dlimit = LF_QP_DIFF_LIMIT;
        regs->reg26_s_mbr_uqp_dlimit = UP_QP_DIFF_LIMIT;
        regs->reg00_g_qscale = rqct->i_enc_qp;
        regs->reg27_s_mbr_frame_qstep = 0;
        regs->reg26_s_mbr_tmb_bits = 0;
        regs->reg2a_s_mbr_qp_min = 0;
        regs->reg2a_s_mbr_qp_max = 0;
        regs->reg6e_s_mbr_qstep_min = 0;
        regs->reg6f_s_mbr_qstep_max = 0;
        return 0;
    }

    frm_type = rqct->i_pictyp;

    if( rqcx->i_period > 1 )
    {
        // The number of P frames in GOP is not zero.
        init_frames = 1;
    }

    #if (CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES != 0 )
    init_frames = CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES;
    #endif //CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES

    // Target frame bitcount
    if (msb3->i_frm_nr > init_frames)
    {
        int smooth_cnt = 0;
        // 1. Determine the number of future frame to compensate for current bitrate mismatch.
        smooth_cnt = _msb3_get_smooth_count(msb3);

        // 2. Calculate the bitcount that this frame should be compensate for.
        delta = _msb3_calculate_delta( msb3, smooth_cnt, frm_type);

        // 3. Finally, calculate the target bitcount.
        msb3->i_tgt_bs = msb3->m_bpf[frm_type] - delta;


        // 4. Clip the target size to 70% ~ 130%
        msb3->i_tgt_bs = MSRC_FIT_IN_RANGE( msb3->i_tgt_bs
                                            ,MSRC_PERCENT ( 70, msb3->m_bpf[frm_type])
                                            ,MSRC_PERCENT (130, msb3->m_bpf[frm_type]));

    }
    else
    {
        // The first I frame and the first P frame use the default value.
        msb3->i_tgt_bs = msb3->m_bpf[frm_type];
    }

    /* Update max min qp value */
    _msb3_qp_update_max(msb3, frm_type);
    /* Return initial frame QP */
    msb3->i_this_enc_qs = _msb3_compute_qs(msb3, frm_type);
    rqct->i_enc_qp = qs2qp(msb3->i_this_enc_qs);

    MSBR_DBG( "Qp = %d qs = %d ls %d tg %d <%d:%d> dt %d\n", rqcx->rqct.i_enc_qp , msb3->i_this_enc_qs, msb3->i_last_frm_size[frm_type], msb3->i_tgt_bs
                                                   , msb3->i_frm_nr,  frm_type,delta);

    /* Clip qs and qp value in range */
    _msb3_qp_fit_in_range(msb3, frm_type);

    msb3->i_last_avg_qs = 0;
    msb3->i_last_frm_bs = 0;

    msb3->i_tgt_mb = (int)div_s64((int64)msb3->i_tgt_bs*256,msb3->i_pixels);

    if (msb3->i_tgt_mb < 1)
        msb3->i_tgt_mb = 1;

    switch (msb3->i_method)
    {
    case RQCT_METHOD_CBR:
        rqcx->i_config |= RCTL_CFG_MB_DQP;

        #if CFG_CBR_IFRAME_CQP
        if( frm_type == 0 )
            rqcx->i_config&=~RCTL_CFG_MB_DQP;
        #endif

        #if CFG_CBR_PFRAME_CQP
        if( frm_type != 0 )
            rqcx->i_config&=~RCTL_CFG_MB_DQP;
        #endif

        break;
    case CONSTRAINED_VBR:
    case RQCT_METHOD_VBR:
        rqcx->i_config|= RCTL_CFG_MB_DQP;
        break;
    case RQCT_METHOD_CQP:
        rqcx->i_config&=~RCTL_CFG_MB_DQP;
        break;
    default:
        rqcx->i_config|= RCTL_CFG_MB_DQP;
        break;
    }

    mrqc_roi_draw(rqct, mjob);

    regs->reg00_g_mbr_en = !rqcx->attr.b_dqmstat && (msb3->i_method!=RQCT_METHOD_CQP);
    regs->reg26_s_mbr_pqp_dlimit = LF_QP_DIFF_LIMIT;
    regs->reg26_s_mbr_uqp_dlimit = UP_QP_DIFF_LIMIT;
    regs->reg00_g_qscale = rqct->i_enc_qp;
    regs->reg27_s_mbr_frame_qstep = msb3->i_this_enc_qs;
    regs->reg26_s_mbr_tmb_bits = msb3->i_tgt_mb;
    regs->reg2a_s_mbr_qp_min = msb3->i_this_min_qp;
    regs->reg2a_s_mbr_qp_max = msb3->i_this_max_qp;
    regs->reg6e_s_mbr_qstep_min = msb3->i_this_min_qs;
    regs->reg6f_s_mbr_qstep_max = msb3->i_this_max_qs;

    return 0;
}

static int msb3_enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_msb3* msb3 = (mfe_msb3*)rqcx;
    mfe_regs* regs = (mfe_regs*)mjob;
    int frm_type;
    int mbs = (msb3->i_pixels>>8), bps;

    regs->reg28 = regs->reg29 = 0;
    regs->reg42 = regs->reg43 = 0;

    rqct->i_bitcnt = mjob->i_bits;
    msb3->i_avg_qs = regs->sumofq / (mbs-1);

    rqct->i_enc_bs += rqct->i_bitcnt/8;
    rqct->i_enc_nr++;

    acc_push(&msb3->acc_bitcnt, rqct->i_bitcnt);
    bps = acc_calc(&msb3->acc_bitcnt, rqcx->i_period);
    bps = (int)div_s64((int64)bps*msb3->n_fmrate,msb3->d_fmrate*rqcx->i_period);

    if (msb3->i_method == RQCT_METHOD_CBR)
    {
        int bpp = (int)div_s64((int64)rqct->i_bitcnt*BPP_FAC,msb3->i_pixels);
        int cpx = bpp * qp2qs(rqct->i_enc_qp);

        if (IS_PPIC(rqct->i_pictyp))
        {
            acc_push(&msb3->acc_rqprod, cpx);
            iir_push(&msb3->iir_rqprod[RQCT_PICTYP_P], acc_calc(&msb3->acc_rqprod, 8) / 8);
            msb3->i_missed += (rqct->i_bitcnt - msb3->i_frmbit + msb3->i_btbias);
        }
        else
        {
            iir_push(&msb3->iir_rqprod[RQCT_PICTYP_I], cpx);
            msb3->i_missed = msb3->i_bitpos;
            if (rqcx->i_period > 1)
                msb3->i_btbias = (rqct->i_bitcnt - msb3->i_frmbit) / (rqcx->i_period - 1);
        }
        msb3->i_bitpos += (rqct->i_bitcnt - msb3->i_frmbit);
        if (!rqcx->attr.b_logoff)
            snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d bps:%8d",\
            rqct->name,rqct->i_enc_nr,IS_IPIC(rqct->i_pictyp)?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt,bps);
#if defined(LOG_MSG)
        printk("%s\n",rqct->print_line);
#endif
        return 0;
    }

    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d bps:%8d",\
        rqct->name,rqct->i_enc_nr,IS_IPIC(rqct->i_pictyp)?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt,bps);
#if defined(LOG_MSG)
    printk("%s\n",rqct->print_line);
#endif
    frm_type = rqct->i_pictyp;

    // update counter
    msb3->i_totbit += rqct->i_bitcnt;
    msb3->i_frm_nr++;

    MSBR_DBG( "conf i_vbv_fullness %d + bitcnt %d= ", msb3->i_vbv_fullness, rqct->i_bitcnt );

    msb3->i_vbv_fullness      += rqct->i_bitcnt;
    msb3->i_vbv_real_fullness += rqct->i_bitcnt;

    MSBR_DBG( "%d\n", msb3->i_vbv_fullness );


    msb3->i_last_frm_bs   = rqct->i_bitcnt;
    msb3->i_last_tgt_bs   = msb3->i_tgt_bs;

    msb3->i_last_enc_qs             = msb3->i_this_enc_qs;
    msb3->i_last_enc_qp             = rqct->i_enc_qp;

    msb3->i_last_qs[frm_type]       = msb3->i_this_enc_qs;
    msb3->i_last_qp[frm_type]       = rqct->i_enc_qp;
    msb3->i_last_tgt_size[frm_type] = msb3->i_tgt_bs;
    msb3->i_last_frm_size[frm_type] = rqct->i_bitcnt;

#if defined(MMFE_RCTL_PATCH)
    if (msb3->i_avg_qs <(msb3->i_this_enc_qs/2))
        msb3->i_avg_qs = msb3->i_this_enc_qs;
    if (msb3->i_avg_qs > msb3->i_this_max_qs)
        msb3->i_avg_qs = msb3->i_this_max_qs;
#endif

    msb3->i_last_avg_qs = msb3->i_avg_qs;
    // Variable bitrate
    if (msb3->i_method == RQCT_METHOD_VBR)
        msb3->i_lt_qs64 += ((msb3->i_last_avg_qs*64)-msb3->i_lt_qs64) / msb3->i_frm_nr;
    else if (msb3->i_method == CONSTRAINED_VBR)
    {
        if (msb3->i_frm_nr == 1)
            msb3->i_lt_qs64 = msb3->i_last_avg_qs*64;
        else
            msb3->i_lt_qs64 = (msb3->i_lt_qs64*(msb3->i_smooth_cnt-1) + (msb3->i_last_avg_qs*64)) / msb3->i_smooth_cnt;
    }

    if (msb3->i_method == CONSTRAINED_VBR)
    {
        msb3->i_gauge_bps -= msb3->i_gauge_bgt[msb3->i_gauge_idx];
        msb3->i_gauge_bps += rqct->i_bitcnt;
        msb3->i_gauge_bgt[msb3->i_gauge_idx] = rqct->i_bitcnt;
        msb3->i_gauge_idx++;
        if (msb3->i_gauge_idx == msb3->i_gauge_cnt)
            msb3->i_gauge_idx = 0;
    }

    /* Update VBV buffer status  */

    #if   ( CFG_VBV_FRAME_QUOTA_METHOD == VFQ_USE_AVERAGE_SIZE )

    MSBR_DBG( "conf i_vbv_fullness %d - i_avgbpf %d= ", msb3->i_vbv_fullness, msb3->i_avgbpf );
    msb3->i_vbv_fullness      -= msb3->i_avgbpf;
    msb3->i_vbv_real_fullness -= msb3->i_avgbpf;

    #elif ( CFG_VBV_FRAME_QUOTA_METHOD == VFQ_USE_WEIGHTED_SIZE )

    MSBR_DBG( "conf i_vbv_fullness %d - i_avgbpf %d= ", msb3->i_vbv_fullness, msb3->m_bpf[frm_type] );
    msb3->i_vbv_fullness      -= msb3->m_bpf[frm_type];
    msb3->i_vbv_real_fullness -= msb3->m_bpf[frm_type];

    #else  //

    msb3->i_vbv_fullness      -= msb3->i_avgbpf;
    msb3->i_vbv_real_fullness -= msb3->i_avgbpf;

    #endif //VFQ_USE_WEIGHTED_SIZE

    MSBR_DBG( "%d\n", msb3->i_vbv_fullness);

    return 0;
}
/*
static int qp2qs(int qp)
{
    int i;
    int qs;
    static const int QP2QSTEP[6] = { 20, 22, 26, 28, 32, 36 };

    qs = QP2QSTEP[qp%6];

    for (i = 0; i < (qp/6); i++)
        qs*=2;

    return qs;
}

static int qs2qp(int qsx32)
{
    int q_per = 0, q_rem = 0;

    while (qsx32 > qp2qs(5))
    {
        qsx32 >>= 1;
        q_per += 1;
    }

    if (qsx32 <= 21)
    {
        qsx32 = 20;
        q_rem = 0;
    }
    else if (qsx32 <= 24)
    {
        qsx32 = 22;
        q_rem = 1;
    }
    else if (qsx32 <= 27)
    {
        qsx32 = 26;
        q_rem = 2;
    }
    else if (qsx32 <= 30)
    {
        qsx32 = 28;
        q_rem = 3;
    }
    else if (qsx32 <= 34)
    {
        qsx32 = 32;
        q_rem = 4;
    }
    else
    {
        qsx32 = 36;
        q_rem = 5;
    }

    return (q_per*6 + q_rem);
}
*/
static int _msb3_qp_init(mfe_msb3* msb3)
{
    int       i;
    // Init with a default value
    for( i=0; i< MAX_FRAME_TYPE; i++ )
    {
        msb3->i_init_qp[i]        = P_FRAME_INIT_QP;
        msb3->i_min_qp[i]         = P_FRAME_MIN_QP;
        msb3->i_max_qp[i]         = P_FRAME_MAX_QP;
        msb3->i_margin_qp[i]      = P_FRAME_MARGIN_QP;
    }

    msb3->i_init_qp[RQCT_PICTYP_I]   = I_FRAME_INIT_QP;
    msb3->i_min_qp[RQCT_PICTYP_I]    = I_FRAME_MIN_QP;
    msb3->i_max_qp[RQCT_PICTYP_I]    = I_FRAME_MAX_QP;
    msb3->i_margin_qp[RQCT_PICTYP_I] = I_FRAME_MARGIN_QP;

    msb3->i_init_qp[RQCT_PICTYP_P]   = P_FRAME_INIT_QP;
    msb3->i_min_qp[RQCT_PICTYP_P]    = P_FRAME_MIN_QP;
    msb3->i_max_qp[RQCT_PICTYP_P]    = P_FRAME_MAX_QP;
    msb3->i_margin_qp[RQCT_PICTYP_P] = P_FRAME_MARGIN_QP;

    msb3->i_init_qp[RQCT_PICTYP_B]   = B_FRAME_INIT_QP;
    msb3->i_min_qp[RQCT_PICTYP_B]    = B_FRAME_MIN_QP;
    msb3->i_max_qp[RQCT_PICTYP_B]    = B_FRAME_MAX_QP;
    msb3->i_margin_qp[RQCT_PICTYP_B] = B_FRAME_MARGIN_QP;

    if (msb3->i_method == RQCT_METHOD_VBR)
    {
        msb3->i_init_qp[RQCT_PICTYP_I] = msb3->i_leadqp - msb3->i_deltaq;
        msb3->i_init_qp[RQCT_PICTYP_P] = msb3->i_leadqp;
        msb3->i_min_qp[RQCT_PICTYP_I] = msb3->rqcx.attr.i_lowerq - msb3->i_deltaq;
        msb3->i_max_qp[RQCT_PICTYP_I] = msb3->rqcx.attr.i_upperq - msb3->i_deltaq;
        msb3->i_min_qp[RQCT_PICTYP_P] = msb3->rqcx.attr.i_lowerq;
        msb3->i_max_qp[RQCT_PICTYP_P] = msb3->rqcx.attr.i_upperq;
    }

    return 0;
}

static int _msb3_qp_update_max(mfe_msb3* msb3, int frm_type)
{
    int max_qp, min_qp;

    if( frm_type > MAX_FRAME_TYPE-1 ) frm_type = RQCT_PICTYP_P;

    min_qp = msb3->i_min_qp[frm_type];

/*
    // Fixed boundaries
    max_qp = msb3->i_max_qp[frm_type];
*/
    max_qp = _msb3_get_qp_upper(frm_type, msb3->i_btrate, msb3->i_fmrate*10, msb3->i_pixels );

    // Apply into max/min setting
    msb3->i_this_min_qp = min_qp;
    msb3->i_this_max_qp = max_qp;
    msb3->i_this_min_qs = qp2qs(min_qp)-1;
    msb3->i_this_max_qs = qp2qs(max_qp);

    return 0;
}

static int _msb3_qp_fit_in_range(mfe_msb3* msb3, int frm_type)
{
    mfe_rctl* rqcx = &msb3->rqcx;
    rqct_ops* rqct = &rqcx->rqct;
    int       max_qp, min_qp;
    int       max_qs, min_qs;

    if( frm_type > MAX_FRAME_TYPE-1 ) frm_type = RQCT_PICTYP_P;

    max_qp = msb3->i_max_qp[frm_type] - msb3->i_margin_qp[frm_type];
    min_qp = msb3->i_min_qp[frm_type] - msb3->i_margin_qp[frm_type];

    min_qs = qp2qs(min_qp )-1;
    max_qs = qp2qs(max_qp );

    msb3->i_this_enc_qs = MSRC_FIT_IN_RANGE(msb3->i_this_enc_qs, min_qs, max_qs );
    rqct->i_enc_qp = MSRC_FIT_IN_RANGE(rqct->i_enc_qp, min_qp, max_qp );

    return 0;
}


static int _msb3_get_qp_upper(int frm_type, int bps, int fps_by_10, int pixels )
{
	int i;
	int bpm;
	int num = 0;
	int qp;
	const qp_bound* qp_table;

    bpm = (int)div64_s64( (int64)(bps*10)<<BOUND_FP_SHIFT , (int64)fps_by_10 * (int64)pixels);

	switch( frm_type )
	{
	case FRAME_TYPE_I:
        qp       = I_FRAME_MAX_QP;
		qp_table = qp_i_upper;
		num		 = SIZE_OF_I_QP_UPP_BOUND;
		break;
	case FRAME_TYPE_P:
    case FRAME_TYPE_B:
    default:
        qp       = P_FRAME_MAX_QP;
		qp_table = qp_p_upper;
		num		 = SIZE_OF_P_QP_UPP_BOUND;
		break;
	}

	for( i=0; i< num; i++ )
	{
		int64 tmp_bpm;
        tmp_bpm = (int)div64_s64( (int64)(qp_table[i].bps*10)<<BOUND_FP_SHIFT , (int64)qp_table[i].fps_by_10 * (int64)qp_table[i].pixels);
		qp = qp_table[i].qp_bound;
		if( bpm <= tmp_bpm )
			break;
	}

	return qp;
}
