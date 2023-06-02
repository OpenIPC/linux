
#include <linux/math64.h>

#include <mfe.h>

/************************************************************************/
/* Macros                                                               */
/************************************************************************/
#define MSBR_VER_EXT            04
#define _EXP(expr)              #expr
#define _STR(expr)              _EXP(expr)
#define RC_MODULE_NAME          "MSBR"
#define RC_MODULE_VER_MAJOR     1
#define RC_MODULE_VER_MINOR     2
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

typedef struct mfe_msbr
{
    mfe_rctl    rqcx;
    int     i_method;
    short   i_leadqp, i_deltaq;
    int     i_btrate;
    int     i_pixels;
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
    int     n_fmrate;
    int     d_fmrate;
    int     i_fmrate;
    int     i_fps;
    int     i_maxbps;
    int     b_fixfps;
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
} mfe_msbr;

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

/************************************************************************/
/* Local prototypes                                                     */
/************************************************************************/

static int  msbr_seq_sync(rqct_ops*);
static int  msbr_seq_done(rqct_ops*);
static int  msbr_seq_conf(rqct_ops*);
static int  msbr_enc_conf(rqct_ops*, mhve_job*);
static int  msbr_enc_done(rqct_ops*, mhve_job*);
static void msbr_release(rqct_ops*);

static int  _msbr_compute_qs(mfe_msbr* msbr, int frm_type);
static int  _msbr_qp_init(mfe_msbr* msbr);
static int  _msbr_qp_update_max(mfe_msbr* msbr, int frm_type);
static int  _msbr_qp_fit_in_range(mfe_msbr* msbr, int frm_type);
static int  _msbr_get_qp_upper(int frm_type, int bps, int fps_by_10, int pixels );

static int  qp2qs(int QP);
static int  qs2qp(int qsx32);

/************************************************************************/
/* Functions                                                            */
/************************************************************************/


static void
msbr_print_version(void)
{
    printk("RATECTL_VER: [%s] %d.%d %s\n", RC_MODULE_NAME, RC_MODULE_VER_MAJOR,
                                           RC_MODULE_VER_MINOR, RC_MODULE_VER_TEST );
}

char* msbr_describe(void)
{
    static char line[64];
    sprintf(line, "%s@v%d.%d-%02d:alternative rc.",MSBR_NAME,MSBR_VER_MJR,MSBR_VER_MNR,MSBR_VER_EXT);
    return line;
}

void* msbr_allocate(void)
{
    rqct_ops* rqct;

    if (NULL != (rqct = MEM_ALLC(sizeof(mfe_msbr))))
    {
        mfe_rctl* rqcx = (mfe_rctl*)rqct;
        MEM_COPY(rqct->name, MSBR_NAME, 5);
        rqct->seq_sync = msbr_seq_sync;
        rqct->seq_done = msbr_seq_done;
        rqct->set_rqcf = mrqc_set_rqcf;
        rqct->get_rqcf = mrqc_get_rqcf;
        rqct->seq_conf = msbr_seq_conf;
        rqct->enc_buff = mrqc_enc_buff;
        rqct->enc_conf = msbr_enc_conf;
        rqct->enc_done = msbr_enc_done;
        rqct->release = msbr_release;

        rqcx->attr.i_method = RQCT_METHOD_CQP;
        rqcx->attr.i_leadqp =-1;
        rqcx->attr.i_deltaq = 3;
        rqcx->attr.i_upperq =48;
        rqcx->attr.i_lowerq =12;
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


static void msbr_release(rqct_ops* rqct)
{
    MEM_FREE(rqct);
}

static int msbr_seq_done(rqct_ops* rqct)
{

    /* Debug print */
    #if DBG_PRINT_ENABLE
    {
        mfe_msbr* msbr = (mfe_msbr*)rqct;

        MSBR_DBG( "m_bpf %d %d %d \n", msbr->m_bpf[0], msbr->m_bpf[1], msbr->m_bpf[2] );
    }
    #endif

    /* call seq_done() */
    return 0;
}

static int msbr_seq_sync(rqct_ops* rqct)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    rqcx->i_pcount = 0;
    return 0;
}

static int msbr_seq_conf(rqct_ops* rqct)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_msbr* msbr = (mfe_msbr*)rqcx;
    int i;

    msbr_print_version();

    msbr->i_method = (int)rqcx->attr.i_method;
    msbr->i_leadqp = rqcx->attr.i_leadqp;
    msbr->i_deltaq = rqcx->attr.i_deltaq;
    msbr->i_btrate = rqcx->attr.i_btrate;
    msbr->i_pixels = (int)rqcx->attr.i_pict_w * (int)rqcx->attr.i_pict_h;
    msbr->n_fmrate = (int)rqcx->attr.n_fmrate;
    msbr->d_fmrate = (int)rqcx->attr.d_fmrate;
    msbr->i_fmrate = msbr->n_fmrate / msbr->d_fmrate;
    msbr->i_maxbps = 0;
    msbr->b_fixfps = 1;

    rqcx->i_period = rqcx->attr.i_period;

    // more bitrate checking
    if (msbr->i_method == CONSTRAINED_VBR)
    {
        if (msbr->i_maxbps == 0)
            msbr->i_maxbps = (int)((msbr->i_btrate*14)/10);

        msbr->i_max_offset = (int)div_s64((int64)(msbr->i_maxbps-msbr->i_btrate)*msbr->d_fmrate,msbr->n_fmrate);

        if (!(rqcx->i_config&RCTL_CFG_MB_DQP))
            msbr->i_max_offset = (msbr->i_max_offset) >> 2;
    }
    else
        msbr->i_maxbps = 0;  // Don't care

    msbr->i_avgbpf = (int)div_s64((int64)msbr->i_btrate*msbr->d_fmrate,msbr->n_fmrate);

    if (rqcx->i_period > 0)
    {
        int length = rqcx->i_period;
        int weight = IFRAME_WEIGHT + PFRAME_WEIGHT*(rqcx->i_period-1);
        msbr->m_bpf[0] = (int)div64_s64((int64)msbr->i_btrate*msbr->d_fmrate*IFRAME_WEIGHT*length, (int64)weight*msbr->n_fmrate);
        msbr->m_bpf[1] = (int)div64_s64((int64)msbr->i_btrate*msbr->d_fmrate*PFRAME_WEIGHT*length, (int64)weight*msbr->n_fmrate);
        msbr->m_bpf[2] = (int)div64_s64((int64)msbr->i_btrate*msbr->d_fmrate*BFRAME_WEIGHT*length, (int64)weight*msbr->n_fmrate);

    }
    else
    {
        msbr->m_bpf[0] = (int)div64_s64((int64)msbr->i_btrate*msbr->d_fmrate*IFRAME_WEIGHT, (int64)PFRAME_WEIGHT*msbr->n_fmrate);
        msbr->m_bpf[1] = (int)div64_s64((int64)msbr->i_btrate*msbr->d_fmrate,msbr->n_fmrate);
        msbr->m_bpf[2] = msbr->m_bpf[1];
    }

    _msbr_qp_init(msbr);

    // Init frame number in Gop
    rqcx->i_pcount = rqcx->i_period;
    msbr->i_totbit = 0;

    // Bitrate usage monitoring
    msbr->i_smooth_min        = (msbr->n_fmrate*SMOOTH_SECOND_CBR)/msbr->d_fmrate/SMOOTH_BASE;

    switch (msbr->i_method)
    {
    case RQCT_METHOD_VBR:
        msbr->i_smooth_cnt = (msbr->n_fmrate*SMOOTH_SECOND_VBR)/msbr->d_fmrate/SMOOTH_BASE;

        msbr->i_vbv_target        = msbr->i_btrate * VBV_TGT_LVL_VBR  / SMOOTH_BASE;
        msbr->i_vbv_fullness_low  = msbr->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msbr->i_vbv_fullness_high = msbr->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msbr->i_vbv_buf_size      = msbr->i_btrate * VBV_BUF_SIZE_VBR / SMOOTH_BASE;

        break;
    case CONSTRAINED_VBR:
        msbr->i_smooth_cnt = (msbr->n_fmrate*SMOOTH_SECOND_CVBR)/msbr->d_fmrate/SMOOTH_BASE;
        msbr->i_gauge_bgt[0] = msbr->m_bpf[0];
        for (i = 1; i < MAX_GAUGE_SIZE; i++)
            msbr->i_gauge_bgt[i] = msbr->m_bpf[1];
        msbr->i_gauge_cnt = msbr->i_fmrate;
        msbr->i_gauge_idx = 0;
        msbr->i_gauge_bps = (int)div_s64((int64)msbr->i_btrate*msbr->d_fmrate*msbr->i_gauge_cnt,msbr->n_fmrate);

        msbr->i_vbv_target        = msbr->i_btrate * VBV_TGT_LVL_VBR  / SMOOTH_BASE;
        msbr->i_vbv_fullness_low  = msbr->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msbr->i_vbv_fullness_high = msbr->i_btrate * VBV_FUL_LOW_VBR  / SMOOTH_BASE;
        msbr->i_vbv_buf_size      = msbr->i_btrate * VBV_BUF_SIZE_VBR / SMOOTH_BASE;

        break;
    case RQCT_METHOD_CBR:
    default:

        msbr->i_smooth_cnt = (int)div_s64((int64)msbr->n_fmrate*SMOOTH_SECOND_CBR,msbr->d_fmrate*SMOOTH_BASE);
        msbr->i_vbv_target        = msbr->i_btrate * VBV_TGT_LVL_CBR  / SMOOTH_BASE;
        msbr->i_vbv_fullness_low  = msbr->i_btrate * VBV_FUL_LOW_CBR  / SMOOTH_BASE;
        msbr->i_vbv_fullness_high = msbr->i_btrate * VBV_FUL_LOW_CBR  / SMOOTH_BASE;
        msbr->i_vbv_buf_size      = msbr->i_btrate * VBV_BUF_SIZE_CBR / SMOOTH_BASE;

        break;
    }

    msbr->i_vbv_fullness      = msbr->i_vbv_target;
    msbr->i_vbv_real_fullness = msbr->i_vbv_target;

    MSBR_DBG( "conf i_vbv_fullness = %d\n", msbr->i_vbv_fullness);

    msbr->i_thr_qp_frameskip = THR_QP_FRAMESKIP;
    msbr->i_frm_nr = 0;
    msbr->i_last_avg_qs = msbr->i_last_frm_bs = 0;
    msbr->i_lt_qs64 = 0;

    // Init var
    for( i=0; i<MAX_FRAME_TYPE; i++ )
    {

        msbr->i_last_enc_qs      = 0;
        msbr->i_last_enc_qp      = 0;

        msbr->i_last_qp[i]       = 0;
        msbr->i_last_qs[i]       = 0;
        msbr->i_last_tgt_size[i] = 0;
        msbr->i_last_frm_size[i] = 0;
        /*
        msbr->i_last_qp[i]       = msbr->i_init_qp[i];
        msbr->i_last_qs[i]       = qp2qs( msbr->i_init_qp[i]);
        msbr->i_last_tgt_size[i] = msbr->m_bpf[i];
        msbr->i_last_frm_size[i] = msbr->m_bpf[i];
        */
    }

    return 0;
}

/*
    Get smooth count
*/
static int _msbr_get_smooth_count(mfe_msbr* msbr)
{
    int smooth_cnt = 0;
    if (msbr->i_frm_nr > msbr->i_smooth_cnt*SMOOTH_FACTOR_MIN/SMOOTH_BASE)
        smooth_cnt = msbr->i_smooth_cnt;
    else if (msbr->i_frm_nr < msbr->i_smooth_min)
        smooth_cnt = msbr->i_smooth_min;
    else
    {
        int fact_n = msbr->i_smooth_cnt - msbr->i_smooth_min;
        int fact_d = msbr->i_smooth_cnt * SMOOTH_FACTOR_MIN/SMOOTH_BASE - msbr->i_smooth_min;
        smooth_cnt = msbr->i_smooth_min + (msbr->i_frm_nr - msbr->i_smooth_min) * fact_n / fact_d;
    }

    return smooth_cnt;
}

static int _msbr_calculate_delta(mfe_msbr* msbr, int smooth_cnt, int frm_type)
{
    mfe_rctl* rqcx = (mfe_rctl*)msbr;

    int delta = 0;


    #if ( CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES != 0 )
    if(msbr->i_frm_nr  < CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES )
    {
        msbr->i_vbv_fullness = msbr->i_vbv_target;

        return 0;
    }
    #endif //CFG_VBV_SKIP_THE_FIRST_NUM_FRAMES

    #if CFG_VBV_BUF_UNDERFLOW_RESET

    /* reset if under flow  */
    if( msbr->i_vbv_fullness < 0 )
    {
        MSBR_DBG( "reset i_vbv_fullness %d= 0\n", msbr->i_vbv_fullness );
        msbr->i_vbv_fullness = 0;
    }
    #endif //CFG_VBV_BUF_UNDERFLOW_RESET

    #if CFG_VBV_BUF_OVERFLOW_RESET

    if( msbr->i_vbv_fullness > msbr->i_vbv_buf_size  )
    {
        MSBR_DBG( "reset i_vbv_fullness %d= %d\n", msbr->i_vbv_fullness, msbr->i_vbv_buf_size );
        msbr->i_vbv_fullness = msbr->i_vbv_buf_size;
    }

    #endif //CFG_VBV_BUF_OVERFLOW_RESET

    if (msbr->i_method == RQCT_METHOD_CBR)
    {
        delta = (int)div_s64((int64)(msbr->i_vbv_fullness-msbr->i_vbv_target)*msbr->d_fmrate,msbr->n_fmrate*SMOOTH_SECOND_CBR/SMOOTH_BASE);
        MSBR_DBG( "i_vbv_fullness %d i_vbv_target %d delta = %d\n", msbr->i_vbv_fullness, msbr->i_vbv_target,delta );

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
    else if (msbr->i_method == CONSTRAINED_VBR)
    {
        delta = (msbr->i_vbv_fullness - msbr->i_vbv_target) / smooth_cnt;
        if (delta < -msbr->i_max_offset)
            delta = -msbr->i_max_offset;
    }
    else if (msbr->i_method == RQCT_METHOD_VBR)
    {
        delta = (msbr->i_vbv_fullness - msbr->i_vbv_target) / smooth_cnt;
        if (delta > 0 && (msbr->i_last_avg_qs*64) > msbr->i_lt_qs64)
            delta = delta>>1;  // Make it more variable bitrate to allow better quality
    }

    return delta;
}


/* Return target QPStep */
static int _msbr_compute_qs(mfe_msbr* msbr, int frm_type)
{
    mfe_rctl* rqcx = &msbr->rqcx;
    int newqs = 0;
    int adjust;
    int bitrate = msbr->i_btrate;
    int fps_den = msbr->d_fmrate;
    int fps_num = msbr->n_fmrate;
    int mb_numb = msbr->i_pixels>>8;


    // For the very first frame, guess one qp!
    if (msbr->i_frm_nr  == 0           ||
        msbr->i_last_qp[frm_type] == 0    )
    {
        int bpMBK, newqp;
        if (msbr->i_method == RQCT_METHOD_CQP)
        {
            newqp = msbr->i_leadqp;
            newqs = qp2qs(newqp);  // So that frame qp will be exactly msbr->i_qp
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

        msbr->i_frozen = newqp >> 1;

        if (msbr->i_frozen > 15)
            msbr->i_frozen = 15;

        return newqs;
    }

    if (msbr->i_method == RQCT_METHOD_CQP)
        return qp2qs(rqcx->rqct.i_enc_qp);

    if (msbr->i_method  == RQCT_METHOD_CBR)
    {

#if 1
        int dqs;
        int last_size,tgt_size;

        last_size = msbr->i_last_frm_size[frm_type];

        #if   (CFG_LAST_QS_SELECTION == LQS_USE_LAST_SET_QS          )
        newqs     = msbr->i_last_enc_qs;
        #elif (CFG_LAST_QS_SELECTION == LQS_USE_LAST_FRM_TYPE_SET_QS )
        newqs     = msbr->i_last_qs[frm_type];
        #elif (CFG_LAST_QS_SELECTION == LQS_USE_LAST_AVG_ENC_QS      )
        newqs = msbr->i_last_avg_qs;
        #else
        newqs     = msbr->i_last_enc_qs;
        #endif

        dqs       = newqs / 8;
        tgt_size  = msbr->i_tgt_bs;

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
        int frm_cnt = msbr->i_frm_nr;
        int frames_left;
        int buf_rest_pic;
        int64 buf_rest;

        int dqp;
        buf_rest = div_s64((int64)frm_cnt*fps_den*bitrate,fps_num);
        buf_rest+= (SMOOTH_PERIOD*msbr->i_btrate) - msbr->i_totbit;
        newqs = msbr->i_last_avg_qs;
        frames_left = (SMOOTH_PERIOD * msbr->n_fmrate) / msbr->d_fmrate;
        buf_rest_pic = (int)div_s64(buf_rest,frames_left);
        dqp = msbr->i_last_avg_qs/8;

        if (msbr->i_last_frm_bs > (buf_rest_pic*9)>>3)
            newqs = msbr->i_last_avg_qs + dqp;
        else if (msbr->i_last_frm_bs < (buf_rest_pic*7)>>3)
            newqs = msbr->i_last_avg_qs - dqp;
#endif

    }
    else if (msbr->i_method == CONSTRAINED_VBR)
    {
        int lo_qs, hi_qs;
        adjust = msbr->i_lt_qs64>>2;
        lo_qs = (msbr->i_lt_qs64 - adjust) >> 6;
        hi_qs = (msbr->i_lt_qs64 + adjust) >> 6;
        if (rqcx->i_period > 1 && IS_IPIC(rqcx->rqct.i_pictyp))
        {
            newqs = msbr->i_last_avg_qs;
            if (msbr->i_gauge_bps < msbr->i_btrate)
                newqs = newqs-(1<<QS_SHIFT_FACTOR);
            newqs = MSRC_MAX(( 1<<QS_SHIFT_FACTOR),newqs);
            newqs = MSRC_MIN((12<<QS_SHIFT_FACTOR),newqs);
        }
        else
        {
            //int64 tmp = div_s64((int64)msbr->i_lt_qs64 * div_s64(msbr->i_totbit,msbr->i_frm_nr), msbr->i_tgt_bs) ;
            //MFE_ASSERT((tmp >> 6) < (1<<31));
            newqs = (int)(div_s64((int64)msbr->i_lt_qs64 * div_s64(msbr->i_totbit,msbr->i_frm_nr),msbr->i_tgt_bs)>>6) ;
            if (msbr->i_last_frm_bs>msbr->i_last_tgt_bs)
            {
                adjust = ((msbr->i_last_frm_bs-msbr->i_last_tgt_bs)/msbr->i_max_offset) + (1<<QS_SHIFT_FACTOR);
                if (adjust > (3<<QS_SHIFT_FACTOR))
                    adjust = (3<<QS_SHIFT_FACTOR);
                if ((msbr->i_last_avg_qs*64) > msbr->i_lt_qs64)
                {
                    // Danger! Make it more aggressive
                    hi_qs = msbr->i_last_avg_qs + adjust;
                    newqs = msbr->i_last_avg_qs + adjust;
                }
                else
                {
                    hi_qs += adjust;
                    newqs += adjust;
                }
            }
            else if (msbr->i_gauge_bps > msbr->i_btrate)
            {
                if (msbr->i_last_avg_qs > newqs)
                    newqs = msbr->i_last_avg_qs;
                if (msbr->i_lt_qs64 > (newqs*64))
                    newqs = msbr->i_lt_qs64/64;
            }
            else
            {
                if ((newqs<<6) >= msbr->i_lt_qs64)
                    newqs = (msbr->i_lt_qs64/64)-1;
                if (msbr->i_vbv_target>msbr->i_vbv_fullness)
                {
                    adjust = (msbr->i_vbv_target - msbr->i_vbv_fullness) / (int)msbr->i_btrate;
                    newqs -= adjust;
                }
            }
        }
        newqs = MSRC_MIN(hi_qs, newqs);
        newqs = MSRC_MAX(lo_qs, newqs);
    }
    else if (msbr->i_method == RQCT_METHOD_VBR)
    {
        int lo_qs, hi_qs;
        if (rqcx->i_period > 1 && IS_IPIC(rqcx->rqct.i_pictyp))
        {
            newqs = msbr->i_last_avg_qs;
            if ((msbr->i_last_avg_qs*64) > msbr->i_lt_qs64)
                newqs = newqs - (1<<QS_SHIFT_FACTOR);
        }
        else
        {
            int adj_ltq;
            if (msbr->i_frm_nr >= msbr->i_fmrate || rqcx->i_period == 1)
            {
                //int64 tmp = div_s64((int64)msbr->i_lt_qs64*div_s64(msbr->i_totbit,msbr->i_frm_nr),msbr->i_avgbpf);
                //MFE_ASSERT((tmp >> 6) < (1<<31));
                adj_ltq = (int)(div_s64((int64)msbr->i_lt_qs64*div_s64(msbr->i_totbit,msbr->i_frm_nr),msbr->i_avgbpf)>>6);
            }
            else
            {
                adj_ltq = msbr->i_lt_qs64/64;   // Wait for stabilization
            }
            MFE_ASSERT(msbr->i_tgt_bs>0);
            newqs = (adj_ltq * msbr->i_avgbpf) / msbr->i_tgt_bs;

            adjust = MSRC_MAX((2<<QS_SHIFT_FACTOR),adj_ltq>>2);
            lo_qs = adj_ltq - adjust;
            hi_qs = adj_ltq + adjust;

            if (msbr->i_last_frm_bs > msbr->i_last_tgt_bs)
            {
                adjust = (int)(msbr->i_last_frm_bs/msbr->i_last_tgt_bs);
                if (adjust > 2)
                    adjust = 2;
                hi_qs += adjust;
            }

            if (msbr->i_avgbpf > msbr->i_tgt_bs)
                newqs = MSRC_MIN(hi_qs,newqs);
            else
                newqs = MSRC_MAX(lo_qs,newqs);
        }
    }
    return newqs;
}


static int msbr_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_regs* regs = (mfe_regs*)mjob;
    mfe_msbr* msbr = (mfe_msbr*)rqcx;
    int frm_type;
    int delta = 0;
    int init_frames = 0;

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
    if (msbr->i_frm_nr > init_frames)
    {
        int smooth_cnt = 0;
        // 1. Determine the number of future frame to compensate for current bitrate mismatch.
        smooth_cnt = _msbr_get_smooth_count(msbr);

        // 2. Calculate the bitcount that this frame should be compensate for.
        delta = _msbr_calculate_delta( msbr, smooth_cnt, frm_type);

        // 3. Finally, calculate the target bitcount.
        msbr->i_tgt_bs = msbr->m_bpf[frm_type] - delta;


        // 4. Clip the target size to 70% ~ 130%
        msbr->i_tgt_bs = MSRC_FIT_IN_RANGE( msbr->i_tgt_bs
                                            ,MSRC_PERCENT ( 70, msbr->m_bpf[frm_type])
                                            ,MSRC_PERCENT (130, msbr->m_bpf[frm_type]));

    }
    else
    {
        // The first I frame and the first P frame use the default value.
        msbr->i_tgt_bs = msbr->m_bpf[frm_type];
    }

    /* Update max min qp value */
    _msbr_qp_update_max(msbr, frm_type);
    /* Return initial frame QP */
    msbr->i_this_enc_qs = _msbr_compute_qs(msbr, frm_type);
    rqct->i_enc_qp = qs2qp(msbr->i_this_enc_qs);

    MSBR_DBG( "Qp = %d qs = %d ls %d tg %d <%d:%d> dt %d\n", rqcx->rqct.i_enc_qp , msbr->i_this_enc_qs, msbr->i_last_frm_size[frm_type], msbr->i_tgt_bs
                                                   , msbr->i_frm_nr,  frm_type,delta);

    /* Clip qs and qp value in range */
    _msbr_qp_fit_in_range(msbr, frm_type);

    msbr->i_last_avg_qs = 0;
    msbr->i_last_frm_bs = 0;

    msbr->i_tgt_mb = (int)div_s64((int64)msbr->i_tgt_bs*256,msbr->i_pixels);

    if (msbr->i_tgt_mb < 1)
        msbr->i_tgt_mb = 1;

    switch (msbr->i_method)
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

    regs->reg00_g_mbr_en = !rqcx->attr.b_dqmstat && (msbr->i_method!=RQCT_METHOD_CQP);
    regs->reg26_s_mbr_pqp_dlimit = LF_QP_DIFF_LIMIT;
    regs->reg26_s_mbr_uqp_dlimit = UP_QP_DIFF_LIMIT;
    regs->reg00_g_qscale = rqct->i_enc_qp;
    regs->reg27_s_mbr_frame_qstep = msbr->i_this_enc_qs;
    regs->reg26_s_mbr_tmb_bits = msbr->i_tgt_mb;
    regs->reg2a_s_mbr_qp_min = msbr->i_this_min_qp;
    regs->reg2a_s_mbr_qp_max = msbr->i_this_max_qp;
    regs->reg6e_s_mbr_qstep_min = msbr->i_this_min_qs;
    regs->reg6f_s_mbr_qstep_max = msbr->i_this_max_qs;

    return 0;
}

static int msbr_enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_msbr* msbr = (mfe_msbr*)rqcx;
    mfe_regs* regs = (mfe_regs*)mjob;
    int frameskip  = 0;
    int frm_type;
    int mbs = (msbr->i_pixels>>8);

    regs->reg28 = regs->reg29 = 0;
    regs->reg42 = regs->reg43 = 0;

    rqct->i_bitcnt = mjob->i_bits;
    msbr->i_avg_qs = regs->sumofq / (mbs-1);

    rqct->i_enc_bs += rqct->i_bitcnt/8;
    rqct->i_enc_nr++;

    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d",\
                 rqct->name,rqct->i_enc_nr,IS_IPIC(rqct->i_pictyp)?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt);

    frm_type = rqct->i_pictyp;

    // update counter
    msbr->i_totbit += rqct->i_bitcnt;
    msbr->i_frm_nr++;

    MSBR_DBG( "conf i_vbv_fullness %d + bitcnt %d= ", msbr->i_vbv_fullness, rqct->i_bitcnt );

    msbr->i_vbv_fullness      += rqct->i_bitcnt;
    msbr->i_vbv_real_fullness += rqct->i_bitcnt;

    MSBR_DBG( "%d\n", msbr->i_vbv_fullness );


    msbr->i_last_frm_bs   = rqct->i_bitcnt;
    msbr->i_last_tgt_bs   = msbr->i_tgt_bs;

    msbr->i_last_enc_qs             = msbr->i_this_enc_qs;
    msbr->i_last_enc_qp             = rqct->i_enc_qp;

    msbr->i_last_qs[frm_type]       = msbr->i_this_enc_qs;
    msbr->i_last_qp[frm_type]       = rqct->i_enc_qp;
    msbr->i_last_tgt_size[frm_type] = msbr->i_tgt_bs;
    msbr->i_last_frm_size[frm_type] = rqct->i_bitcnt;

#if defined(MMFE_RCTL_PATCH)
    if (msbr->i_avg_qs <(msbr->i_this_enc_qs/2))
        msbr->i_avg_qs = msbr->i_this_enc_qs;
    if (msbr->i_avg_qs > msbr->i_this_max_qs)
        msbr->i_avg_qs = msbr->i_this_max_qs;
#endif

    msbr->i_last_avg_qs = msbr->i_avg_qs;
    // Variable bitrate
    if (msbr->i_method == RQCT_METHOD_VBR)
        msbr->i_lt_qs64 += ((msbr->i_last_avg_qs*64)-msbr->i_lt_qs64) / msbr->i_frm_nr;
    else if (msbr->i_method == CONSTRAINED_VBR)
    {
        if (msbr->i_frm_nr == 1)
            msbr->i_lt_qs64 = msbr->i_last_avg_qs*64;
        else
            msbr->i_lt_qs64 = (msbr->i_lt_qs64*(msbr->i_smooth_cnt-1) + (msbr->i_last_avg_qs*64)) / msbr->i_smooth_cnt;
    }

    if (msbr->i_method == CONSTRAINED_VBR)
    {
        msbr->i_gauge_bps -= msbr->i_gauge_bgt[msbr->i_gauge_idx];
        msbr->i_gauge_bps += rqct->i_bitcnt;
        msbr->i_gauge_bgt[msbr->i_gauge_idx] = rqct->i_bitcnt;
        msbr->i_gauge_idx++;
        if (msbr->i_gauge_idx == msbr->i_gauge_cnt)
            msbr->i_gauge_idx = 0;
    }

    /* Update VBV buffer status  */

    #if   ( CFG_VBV_FRAME_QUOTA_METHOD == VFQ_USE_AVERAGE_SIZE )

    MSBR_DBG( "conf i_vbv_fullness %d - i_avgbpf %d= ", msbr->i_vbv_fullness, msbr->i_avgbpf );
    msbr->i_vbv_fullness      -= msbr->i_avgbpf;
    msbr->i_vbv_real_fullness -= msbr->i_avgbpf;

    #elif ( CFG_VBV_FRAME_QUOTA_METHOD == VFQ_USE_WEIGHTED_SIZE )

    MSBR_DBG( "conf i_vbv_fullness %d - i_avgbpf %d= ", msbr->i_vbv_fullness, msbr->m_bpf[frm_type] );
    msbr->i_vbv_fullness      -= msbr->m_bpf[frm_type];
    msbr->i_vbv_real_fullness -= msbr->m_bpf[frm_type];

    #else  //

    msbr->i_vbv_fullness      -= msbr->i_avgbpf;
    msbr->i_vbv_real_fullness -= msbr->i_avgbpf;

    #endif //VFQ_USE_WEIGHTED_SIZE

    MSBR_DBG( "%d\n", msbr->i_vbv_fullness);

    // check if next skipped frame(s) needed
    if (!msbr->b_fixfps)
    {
        if (msbr->i_method == CONSTRAINED_VBR || msbr->i_method == RQCT_METHOD_VBR)
        {
            if (msbr->i_lt_qs64 > (qp2qs(msbr->i_thr_qp_frameskip)*64) &&
                    msbr->i_last_frm_bs >= (msbr->i_last_tgt_bs<<1))
            {
                frameskip = (int)((msbr->i_last_frm_bs - msbr->i_last_tgt_bs) / msbr->i_avgbpf - 1);
                if (frameskip < 0)
                    frameskip = 0;
                else if (frameskip > msbr->i_frozen)
                    frameskip = msbr->i_frozen;
            }
        }
        else if (msbr->i_method == RQCT_METHOD_CBR)
        {
            if (msbr->i_last_avg_qs > qp2qs(msbr->i_thr_qp_frameskip))
            {
                // Actual fullness is updated after encoding dummy-P frame
                int fullness = msbr->i_vbv_fullness;
                while (fullness > msbr->i_vbv_target)
                {
                    fullness = (int)(fullness - msbr->i_avgbpf);
                    frameskip++;
                }
            }
        }
        msbr->i_frameskip = frameskip;
    }
    return 0;
}

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


static int _msbr_qp_init(mfe_msbr* msbr)
{
    int       i;
    // Init with a default value
    for( i=0; i< MAX_FRAME_TYPE; i++ )
    {
        msbr->i_init_qp[i]        = P_FRAME_INIT_QP;
        msbr->i_min_qp[i]         = P_FRAME_MIN_QP;
        msbr->i_max_qp[i]         = P_FRAME_MAX_QP;
        msbr->i_margin_qp[i]      = P_FRAME_MARGIN_QP;
    }

    msbr->i_init_qp[RQCT_PICTYP_I]   = I_FRAME_INIT_QP;
    msbr->i_min_qp[RQCT_PICTYP_I]    = I_FRAME_MIN_QP;
    msbr->i_max_qp[RQCT_PICTYP_I]    = I_FRAME_MAX_QP;
    msbr->i_margin_qp[RQCT_PICTYP_I] = I_FRAME_MARGIN_QP;

    msbr->i_init_qp[RQCT_PICTYP_P]   = P_FRAME_INIT_QP;
    msbr->i_min_qp[RQCT_PICTYP_P]    = P_FRAME_MIN_QP;
    msbr->i_max_qp[RQCT_PICTYP_P]    = P_FRAME_MAX_QP;
    msbr->i_margin_qp[RQCT_PICTYP_P] = P_FRAME_MARGIN_QP;

    msbr->i_init_qp[RQCT_PICTYP_B]   = B_FRAME_INIT_QP;
    msbr->i_min_qp[RQCT_PICTYP_B]    = B_FRAME_MIN_QP;
    msbr->i_max_qp[RQCT_PICTYP_B]    = B_FRAME_MAX_QP;
    msbr->i_margin_qp[RQCT_PICTYP_B] = B_FRAME_MARGIN_QP;

    if (msbr->i_method == RQCT_METHOD_VBR)
    {
        msbr->i_init_qp[RQCT_PICTYP_I] = msbr->i_leadqp - msbr->i_deltaq;
        msbr->i_init_qp[RQCT_PICTYP_P] = msbr->i_leadqp;
        msbr->i_min_qp[RQCT_PICTYP_I] = msbr->rqcx.attr.i_lowerq - msbr->i_deltaq;
        msbr->i_max_qp[RQCT_PICTYP_I] = msbr->rqcx.attr.i_upperq - msbr->i_deltaq;
        msbr->i_min_qp[RQCT_PICTYP_P] = msbr->rqcx.attr.i_lowerq;
        msbr->i_max_qp[RQCT_PICTYP_P] = msbr->rqcx.attr.i_upperq;
    }

    return 0;
}

static int _msbr_qp_update_max(mfe_msbr* msbr, int frm_type)
{
    int max_qp, min_qp;

    if( frm_type > MAX_FRAME_TYPE-1 ) frm_type = RQCT_PICTYP_P;

    min_qp = msbr->i_min_qp[frm_type];

/*
    // Fixed boundaries
    max_qp = msbr->i_max_qp[frm_type];
*/
    max_qp = _msbr_get_qp_upper(frm_type, msbr->i_btrate, msbr->i_fmrate*10, msbr->i_pixels );

    // Apply into max/min setting
    msbr->i_this_min_qp = min_qp;
    msbr->i_this_max_qp = max_qp;
    msbr->i_this_min_qs = qp2qs(min_qp)-1;
    msbr->i_this_max_qs = qp2qs(max_qp);

    return 0;
}

static int _msbr_qp_fit_in_range(mfe_msbr* msbr, int frm_type)
{
    mfe_rctl* rqcx = &msbr->rqcx;
    rqct_ops* rqct = &rqcx->rqct;
    int       max_qp, min_qp;
    int       max_qs, min_qs;

    if( frm_type > MAX_FRAME_TYPE-1 ) frm_type = RQCT_PICTYP_P;

    max_qp = msbr->i_max_qp[frm_type] - msbr->i_margin_qp[frm_type];
    min_qp = msbr->i_min_qp[frm_type] - msbr->i_margin_qp[frm_type];

    min_qs = qp2qs(min_qp )-1;
    max_qs = qp2qs(max_qp );

    msbr->i_this_enc_qs = MSRC_FIT_IN_RANGE(msbr->i_this_enc_qs, min_qs, max_qs );
    rqct->i_enc_qp = MSRC_FIT_IN_RANGE(rqct->i_enc_qp, min_qp, max_qp );

    return 0;
}


static int _msbr_get_qp_upper(int frm_type, int bps, int fps_by_10, int pixels )
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
