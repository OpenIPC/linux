
#include <linux/math64.h>

#include <mfe.h>

#define CVBR_NAME       "CVBR"
#define CVBR_VER_MJR    0
#define CVBR_VER_MNR    0
#define CVBR_VER_EXT    1

#define MFE_VIDEO_MAX_W     1920
#define MFE_VIDEO_MAX_H     1088

/************************************************************************/
/* Configuration                                                        */
/************************************************************************/
#define QS_SHIFT_FACTOR     5

/************************************************************************/
/* Macros                                                               */
/************************************************************************/
#ifndef MSRC_MAX
#define MSRC_MAX(a,b)   (((a)>(b))?(a):(b))
#endif
#ifndef MSRC_MIN
#define MSRC_MIN(a,b)   (((a)<(b))?(a):(b))
#endif

#define UP_QP_DIFF_LIMIT    3
#define LF_QP_DIFF_LIMIT    3

#define CONST_QP        0
#define CBR             1
#define VBR             2
#define CONSTRAINED_VBR 3
#define STATIC_CBR      4

// For Constrained VBR
#define MAX_GAUGE_SIZE  64

typedef struct mfe_cvbr {
    mfe_rctl    rqcx;
    int     i_method;
    short   i_leadqp, i_deltaq;
    int     i_btrate;
    int     i_pixels;
    int     i_tgt_mb;
    int     i_enc_qs;
    int     i_max_qs, i_min_qs;
    int     i_avg_qs;
    // Derived variables
    int     i_avgbpf;
    int     m_bpf[3];       // I, P, B
    /* coded frame counter */
    int     i_frm_nr;
    int64   i_totbit;
    /* integral part of frame-rate */
    int     n_fmrate, d_fmrate;
    int     i_fmrate;
    int     i_fps;
    int     i_maxbps;
    int     b_fixfps;
    /* last-frame status */
    int     i_last_avg_qs;
    int     i_last_frm_bs;
    int     i_last_tgt_bs;
    /* bitrate usage compensation */
    int     i_buf_fullness; // Rate control buffer
    int     i_tgt_fullness;
    int     i_deputy_cnt, i_deputy_min;
    /* variable bitrate */
    int     i_lt_qs64;
    // model parameters
    int     i_tgt_bs;       // target number of bits of current frame
    /* only for Constrained VBR */
    int     i_max_offset;
    int     i_gauge_bgt[MAX_GAUGE_SIZE];
    int     i_gauge_cnt;
    int     i_gauge_idx;
    int     i_gauge_bps;
    int     i_frozen;
    int     i_thr_qp_frameskip;
    int     i_frameskip;    // original MFE-RC: pic_done's output
} mfe_cvbr;

#define MFE_ASSERT(p)

// Spec limitation is [1,51]
#define MAX_QP              (48)
#define MIN_QP              ( 5)
#define THR_QP_FRAMESKIP    (40)

//! How many seconds of frames are responsible for compensation of bitrate usage.
#define DEPUTY_SECOND_CBR    1
#define DEPUTY_SECOND_CVBR   3
#define DEPUTY_SECOND_VBR   10
#define DEPUTY_FACTOR_MIN   10

//! Default I-frame weighting over inter-frame
#define IFRAME_WEIGHT       8
#define PFRAME_WEIGHT       4
#define BFRAME_WEIGHT       3

static int  cvbr_seq_sync(rqct_ops*);
static int  cvbr_seq_done(rqct_ops*);
static int  cvbr_seq_conf(rqct_ops*);
static int  cvbr_enc_conf(rqct_ops*, mhve_job*);
static int  cvbr_enc_done(rqct_ops*, mhve_job*);

static void _cvbrfree(rqct_ops* rqct) { MEM_FREE(rqct); }

void* cvbr_allocate(void)
{
    rqct_ops* rqct;

    if (NULL != (rqct = MEM_ALLC(sizeof(mfe_cvbr))))
    {
        mfe_rctl* rqcx = (mfe_rctl*)rqcx;
        MEM_COPY(rqct->name, CVBR_NAME, 5);
        rqct->seq_sync = cvbr_seq_sync;
        rqct->seq_done = cvbr_seq_done;
        rqct->set_rqcf = mrqc_set_rqcf;
        rqct->get_rqcf = mrqc_get_rqcf;
        rqct->seq_conf = cvbr_seq_conf;
        rqct->enc_buff = mrqc_enc_buff;
        rqct->enc_conf = cvbr_enc_conf;
        rqct->enc_done = cvbr_enc_done;
        rqct->release = _cvbrfree;

        rqcx->attr.i_method = RQCT_METHOD_CQP;
        rqcx->attr.i_leadqp =-1;
        rqcx->attr.i_deltaq = 3;
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

char* cvbr_describe(void)
{
    static char line[64];
    sprintf(line, "%s@v%d.%d-%02d:utopian version.",CVBR_NAME,CVBR_VER_MJR,CVBR_VER_MNR,CVBR_VER_EXT);
    return line;
}

static int compute_qs(mfe_cvbr* cvbr);
static int qp2qs(mfe_cvbr* cvbr, int QP);
static int qs2qp(mfe_cvbr* cvbr, int qsx32);

static int cvbr_seq_done(rqct_ops* rqct)
{
    return 0;
}

static int cvbr_seq_sync(rqct_ops* rqct)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    rqcx->i_pcount = 0;
    return 0;
}

/**
 * fps=14.985 = 15000/1001
 * fps=23.976 = 24000/1001
 * fps=29.970 = 30000/1001
 */
static int cvbr_seq_conf(rqct_ops* rqct)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_cvbr* cvbr = (mfe_cvbr*)rqcx;
    int i;

    printk("%s\n",cvbr_describe());

    cvbr->i_method = (int)rqcx->attr.i_method;
    cvbr->i_leadqp = rqcx->attr.i_leadqp;
    cvbr->i_deltaq = rqcx->attr.i_deltaq;
    cvbr->i_btrate = rqcx->attr.i_btrate;
    cvbr->i_pixels = (int)rqcx->attr.i_pict_w * (int)rqcx->attr.i_pict_h;
    cvbr->n_fmrate = rqcx->attr.n_fmrate;
    cvbr->d_fmrate = rqcx->attr.d_fmrate;
    cvbr->i_fmrate = (int)rqcx->attr.n_fmrate / (int)rqcx->attr.d_fmrate;
    cvbr->i_maxbps = 0;
    cvbr->b_fixfps = 1;

    rqcx->i_period = rqcx->attr.i_period;

    // more bitrate checking
    if (cvbr->i_method == CONSTRAINED_VBR)
    {
        if (cvbr->i_maxbps == 0)
            cvbr->i_maxbps = (int)((cvbr->i_btrate*14)/10);

        cvbr->i_max_offset = (int)div_s64((int64)(cvbr->i_maxbps-cvbr->i_btrate)*cvbr->d_fmrate,cvbr->n_fmrate);

        if (!(rqcx->i_config&RCTL_CFG_MB_DQP))
            cvbr->i_max_offset = (cvbr->i_max_offset) >> 2;
    }
    else
        cvbr->i_maxbps = 0;  // Don't care

    cvbr->i_avgbpf = (int)div_s64((int64)cvbr->i_btrate*cvbr->d_fmrate,cvbr->n_fmrate);

    if (rqcx->i_period > 0)
    {
        int length = rqcx->i_period;
        int weight = IFRAME_WEIGHT + PFRAME_WEIGHT*(rqcx->i_period-1);
        cvbr->m_bpf[0] = (int)div_s64((int64)cvbr->i_btrate*cvbr->d_fmrate*IFRAME_WEIGHT*length,weight*cvbr->n_fmrate);
        cvbr->m_bpf[1] = (int)div_s64((int64)cvbr->i_btrate*cvbr->d_fmrate*PFRAME_WEIGHT*length,weight*cvbr->n_fmrate);
        cvbr->m_bpf[2] = (int)div_s64((int64)cvbr->i_btrate*cvbr->d_fmrate*BFRAME_WEIGHT*length,weight*cvbr->n_fmrate);
    }
    else
    {
        cvbr->m_bpf[0] = (int)div_s64((int64)cvbr->i_btrate*cvbr->d_fmrate*IFRAME_WEIGHT,PFRAME_WEIGHT*cvbr->n_fmrate);
        cvbr->m_bpf[1] = (int)div_s64((int64)cvbr->i_btrate*cvbr->d_fmrate,cvbr->n_fmrate);
    }

    // QP, QStep: min, max
    cvbr->i_min_qs = qp2qs(cvbr,MIN_QP)-1;
    cvbr->i_max_qs = qp2qs(cvbr,MAX_QP);
    cvbr->i_totbit = 0;
    rqcx->i_pcount = rqcx->i_period;
    // Bitrate usage monitoring
    cvbr->i_deputy_min = (cvbr->n_fmrate*DEPUTY_SECOND_CBR)/cvbr->d_fmrate;
    cvbr->i_tgt_fullness = cvbr->i_btrate >> 1;
    cvbr->i_buf_fullness = cvbr->i_tgt_fullness;
    switch (cvbr->i_method)
    {
    case RQCT_METHOD_VBR:
        cvbr->i_deputy_cnt = (cvbr->n_fmrate*DEPUTY_SECOND_VBR)/cvbr->d_fmrate;
        break;
    case CONSTRAINED_VBR:
        cvbr->i_deputy_cnt = (cvbr->n_fmrate*DEPUTY_SECOND_CVBR)/cvbr->d_fmrate;
        cvbr->i_gauge_bgt[0] = cvbr->m_bpf[0];
        for (i = 1; i < MAX_GAUGE_SIZE; i++)
            cvbr->i_gauge_bgt[i] = cvbr->m_bpf[1];
        cvbr->i_gauge_cnt = cvbr->i_fmrate;
        cvbr->i_gauge_idx = 0;
        cvbr->i_gauge_bps = (int)div_s64((int64)cvbr->i_btrate*cvbr->d_fmrate*cvbr->i_gauge_cnt,cvbr->n_fmrate);
        break;
    case RQCT_METHOD_CBR:
    default:
        cvbr->i_deputy_cnt = (int)div_s64((int64)cvbr->n_fmrate*DEPUTY_SECOND_CBR,cvbr->d_fmrate);
        break;
    }
    cvbr->i_thr_qp_frameskip = THR_QP_FRAMESKIP;
    cvbr->i_frm_nr = 0;
    cvbr->i_last_avg_qs = cvbr->i_last_frm_bs = 0;
    cvbr->i_lt_qs64 = 0;

    return 0;
}

static int cvbr_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_regs* regs = (mfe_regs*)mjob;
    mfe_cvbr* cvbr = (mfe_cvbr*)rqcx;
    int deputy_cnt = 0;
    int delta = 0;

    // Target frame bitcount
    if (cvbr->i_frm_nr > 0)
    {
        // 1. Determine the number of future frame to compensate for current bitrate mismatch.
        if (cvbr->i_frm_nr > cvbr->i_deputy_cnt*DEPUTY_FACTOR_MIN)
            deputy_cnt = cvbr->i_deputy_cnt;
        else if (cvbr->i_frm_nr < cvbr->i_deputy_min)
            deputy_cnt = cvbr->i_deputy_min;
        else
        {
            int fact_n = cvbr->i_deputy_cnt - cvbr->i_deputy_min;
            int fact_d = cvbr->i_deputy_cnt * DEPUTY_FACTOR_MIN - cvbr->i_deputy_min;
            deputy_cnt = cvbr->i_deputy_min + (cvbr->i_frm_nr - cvbr->i_deputy_min) * fact_n / fact_d;
        }
        // 2. Calculate the bitcount that this frame should be compensate for.
        if (cvbr->i_method == RQCT_METHOD_CBR)
        {
            delta = (int)div_s64((int64)(cvbr->i_buf_fullness-cvbr->i_tgt_fullness)*cvbr->d_fmrate,cvbr->n_fmrate);
        }
        else if (cvbr->i_method == CONSTRAINED_VBR)
        {
            delta = (cvbr->i_buf_fullness - cvbr->i_tgt_fullness) / deputy_cnt;
            if (delta < -cvbr->i_max_offset)
                delta = -cvbr->i_max_offset;
        }
        else if (cvbr->i_method == RQCT_METHOD_VBR)
        {
            delta = (cvbr->i_buf_fullness - cvbr->i_tgt_fullness) / deputy_cnt;
            if (delta > 0 && (cvbr->i_last_avg_qs*64) > cvbr->i_lt_qs64)
                delta = delta>>1;  // Make it more variable bitrate to allow better quality
        }
        // 3. Finally, calculate the target bitcount.
        cvbr->i_tgt_bs = cvbr->m_bpf[rqct->i_pictyp] - delta;

        if (cvbr->i_tgt_bs <=(cvbr->i_avgbpf >> 3))
            cvbr->i_tgt_bs = (cvbr->i_avgbpf >> 3);  // Target bitcount must>0 for ComputeFrameQP()
    }
    else
    {
        cvbr->i_tgt_bs = cvbr->m_bpf[0]; // Must be I-frame
    }
    /* Return initial frame QP */
    cvbr->i_enc_qs = compute_qs(cvbr);

    cvbr->i_last_avg_qs = 0;
    cvbr->i_last_frm_bs = 0;

    if (cvbr->i_method != RQCT_METHOD_CQP)
        rqcx->i_config |= RCTL_CFG_MB_DQP;

    cvbr->i_tgt_mb = (int)div_s64((int64)cvbr->i_tgt_bs*256,cvbr->i_pixels);

    if (cvbr->i_tgt_mb < 1)
        cvbr->i_tgt_mb = 1;

    rqct->i_enc_qp = qs2qp(cvbr, cvbr->i_enc_qs);

    mrqc_roi_draw(rqct, mjob);

    regs->reg00_g_mbr_en = !rqcx->attr.b_dqmstat && (cvbr->i_method!=RQCT_METHOD_CQP);
    regs->reg26_s_mbr_pqp_dlimit = LF_QP_DIFF_LIMIT;
    regs->reg26_s_mbr_uqp_dlimit = UP_QP_DIFF_LIMIT;
    regs->reg00_g_qscale = rqct->i_enc_qp;
    regs->reg27_s_mbr_frame_qstep = cvbr->i_enc_qs;
    regs->reg26_s_mbr_tmb_bits = cvbr->i_tgt_mb;
    regs->reg2a_s_mbr_qp_min = MIN_QP;
    regs->reg2a_s_mbr_qp_max = MAX_QP;
    regs->reg6e_s_mbr_qstep_min = cvbr->i_min_qs;
    regs->reg6f_s_mbr_qstep_max = cvbr->i_max_qs;

    return 0;
}

static int cvbr_enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    mfe_rctl* rqcx = (mfe_rctl*)rqct;
    mfe_cvbr* cvbr = (mfe_cvbr*)rqcx;
    mfe_regs* regs = (mfe_regs*)mjob;
    int frameskip = 0;
    int mbs = (cvbr->i_pixels>>8);

    regs->reg28 = regs->reg29 = 0;
    regs->reg42 = regs->reg43 = 0;

    rqct->i_bitcnt = mjob->i_bits;
    cvbr->i_avg_qs = regs->sumofq / (mbs-1);

    rqct->i_enc_bs += rqct->i_bitcnt/8;
    rqct->i_enc_nr++;

    // update counter
    cvbr->i_totbit += rqct->i_bitcnt;
    cvbr->i_frm_nr++;
    cvbr->i_buf_fullness += rqct->i_bitcnt;
    cvbr->i_last_frm_bs = rqct->i_bitcnt;
    cvbr->i_last_tgt_bs = cvbr->i_tgt_bs;
#if defined(MMFE_RCTL_PATCH)
    if (cvbr->i_avg_qs <(cvbr->i_enc_qs/2))
        cvbr->i_avg_qs = cvbr->i_enc_qs;
    if (cvbr->i_avg_qs > cvbr->i_max_qs)
        cvbr->i_avg_qs = cvbr->i_max_qs;
#endif
    cvbr->i_last_avg_qs = cvbr->i_avg_qs;
    // Variable bitrate
    if (cvbr->i_method == RQCT_METHOD_VBR)
        cvbr->i_lt_qs64 += ((cvbr->i_last_avg_qs*64)-cvbr->i_lt_qs64) / cvbr->i_frm_nr;
    else if (cvbr->i_method == CONSTRAINED_VBR)
    {
        if (cvbr->i_frm_nr == 1)
            cvbr->i_lt_qs64 = cvbr->i_last_avg_qs*64;
        else
            cvbr->i_lt_qs64 = (cvbr->i_lt_qs64*(cvbr->i_deputy_cnt-1) + (cvbr->i_last_avg_qs*64)) / cvbr->i_deputy_cnt;
    }

    if (cvbr->i_method == CONSTRAINED_VBR)
    {
        cvbr->i_gauge_bps -= cvbr->i_gauge_bgt[cvbr->i_gauge_idx];
        cvbr->i_gauge_bps += rqct->i_bitcnt;
        cvbr->i_gauge_bgt[cvbr->i_gauge_idx] = rqct->i_bitcnt;
        cvbr->i_gauge_idx++;
        if (cvbr->i_gauge_idx == cvbr->i_gauge_cnt)
            cvbr->i_gauge_idx = 0;
    }
    // update buffer status
    cvbr->i_buf_fullness -= cvbr->i_avgbpf;
    // check if next skipped frame(s) needed
    if (!cvbr->b_fixfps)
    {
        if (cvbr->i_method == CONSTRAINED_VBR || cvbr->i_method == RQCT_METHOD_VBR)
        {
            if (cvbr->i_lt_qs64 > (qp2qs(cvbr,cvbr->i_thr_qp_frameskip)*64) &&
                cvbr->i_last_frm_bs >= (cvbr->i_last_tgt_bs<<1))
            {
                    frameskip = (int)((cvbr->i_last_frm_bs - cvbr->i_last_tgt_bs) / cvbr->i_avgbpf - 1);
                    if (frameskip < 0)
                        frameskip = 0;
                    else if (frameskip > cvbr->i_frozen)
                        frameskip = cvbr->i_frozen;
            }
        }
        else if (cvbr->i_method == RQCT_METHOD_CBR)
        {
            if (cvbr->i_last_avg_qs > qp2qs(cvbr, cvbr->i_thr_qp_frameskip))
            {   // Actual fullness is updated after encoding dummy-P frame
                int fullness = cvbr->i_buf_fullness;
                while (fullness > cvbr->i_tgt_fullness)
                {
                    fullness = (int)(fullness - cvbr->i_avgbpf);
                    frameskip++;
                }
            }
        }
        cvbr->i_frameskip = frameskip;
    }
    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d",\
                 rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp==RQCT_PICTYP_I?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt);
    return 0;
}

#define SMOOTH_PERIOD   1
#define INIT_QP_FACTOR  720
#define MIN_INIT_QP     1
#define MAX_INIT_QP     96

/* Return target QPStep */
static int compute_qs(mfe_cvbr* cvbr)
{
    mfe_rctl* rqcx = &cvbr->rqcx;
    rqct_ops* rqct = &rqcx->rqct;
    int newqs = 0;
    int64 buf_rest;
    int buf_rest_pic;
    int frames_left;
    int adjust;
    int bitrate = cvbr->i_btrate;
    int fps_den = cvbr->d_fmrate;
    int fps_num = cvbr->n_fmrate;
    int mb_numb = cvbr->i_pixels>>8;
    int frm_cnt = cvbr->i_frm_nr;

    // For the very first frame, guess one qp!
    if (cvbr->i_frm_nr == 0)
    {
        int bpMBK, newqp;
        if (cvbr->i_method == RQCT_METHOD_CQP)
        {
            newqp = cvbr->i_leadqp;
            newqs = qp2qs(cvbr, newqp);  // So that frame qp will be exactly cvbr->i_qp
        }
        else
        {
            bpMBK = (int)div_s64((int64)bitrate*fps_den,mb_numb*fps_num)+1;
            newqp = INIT_QP_FACTOR / bpMBK;

            if (newqp < MIN_INIT_QP)
                newqp = MIN_INIT_QP;
            if (newqp > MAX_INIT_QP)
                newqp = MAX_INIT_QP;
            newqs = newqp << QS_SHIFT_FACTOR;
        }

        cvbr->i_frozen = newqp >> 1;

        if (cvbr->i_frozen > 15)
            cvbr->i_frozen = 15;

        return newqs;
    }

    if (cvbr->i_method == RQCT_METHOD_CQP)
        return qp2qs(cvbr, rqct->i_enc_qp);

    if (cvbr->i_method  == RQCT_METHOD_CBR)
    {
        int dqp;
        buf_rest = div_s64((int64)frm_cnt*fps_den*bitrate,fps_num);
        buf_rest+= (SMOOTH_PERIOD*cvbr->i_btrate) - cvbr->i_totbit;
        newqs = cvbr->i_last_avg_qs;
        frames_left = (SMOOTH_PERIOD * cvbr->n_fmrate) / cvbr->d_fmrate;
        buf_rest_pic = (int)div_s64(buf_rest,frames_left);
        dqp = cvbr->i_last_avg_qs/8;

        if (cvbr->i_last_frm_bs > (buf_rest_pic*9)>>3)
            newqs = cvbr->i_last_avg_qs + dqp;
        else if (cvbr->i_last_frm_bs < (buf_rest_pic*7)>>3)
            newqs = cvbr->i_last_avg_qs - dqp;
    }
    else if (cvbr->i_method == CONSTRAINED_VBR)
    {
        int lo_qs, hi_qs;
        adjust = cvbr->i_lt_qs64>>2;
        lo_qs = (cvbr->i_lt_qs64 - adjust) >> 6;
        hi_qs = (cvbr->i_lt_qs64 + adjust) >> 6;
        if (rqcx->i_period > 1 && IS_IPIC(rqct->i_pictyp))
        {
            newqs = cvbr->i_last_avg_qs;
            if (cvbr->i_gauge_bps < cvbr->i_btrate)
                newqs = newqs-(1<<QS_SHIFT_FACTOR);
            newqs = MSRC_MAX(( 1<<QS_SHIFT_FACTOR),newqs);
            newqs = MSRC_MIN((12<<QS_SHIFT_FACTOR),newqs);
        }
        else
        {
            //int64 tmp = div_s64((int64)cvbr->i_lt_qs64 * div_s64(cvbr->i_totbit,cvbr->i_frm_nr), cvbr->i_tgt_bs) ;
            //MFE_ASSERT((tmp >> 6) < (1<<31));
            newqs = (int)(div_s64((int64)cvbr->i_lt_qs64 * div_s64(cvbr->i_totbit,cvbr->i_frm_nr),cvbr->i_tgt_bs)>>6) ;
            if (cvbr->i_last_frm_bs>cvbr->i_last_tgt_bs)
            {
                adjust = ((cvbr->i_last_frm_bs-cvbr->i_last_tgt_bs)/cvbr->i_max_offset) + (1<<QS_SHIFT_FACTOR);
                if (adjust > (3<<QS_SHIFT_FACTOR))
                    adjust = (3<<QS_SHIFT_FACTOR);
                if ((cvbr->i_last_avg_qs*64) > cvbr->i_lt_qs64)
                {  // Danger! Make it more aggressive
                    hi_qs = cvbr->i_last_avg_qs + adjust;
                    newqs = cvbr->i_last_avg_qs + adjust;
                }
                else
                {
                    hi_qs += adjust;
                    newqs += adjust;
                }
            }
            else if (cvbr->i_gauge_bps > cvbr->i_btrate)
            {
                if (cvbr->i_last_avg_qs > newqs)
                    newqs = cvbr->i_last_avg_qs;
                if (cvbr->i_lt_qs64 > (newqs*64))
                    newqs = cvbr->i_lt_qs64/64;
            }
            else
            {
                if ((newqs<<6) >= cvbr->i_lt_qs64)
                    newqs = (cvbr->i_lt_qs64/64)-1;
                if (cvbr->i_tgt_fullness>cvbr->i_buf_fullness)
                {
                    adjust = (cvbr->i_tgt_fullness - cvbr->i_buf_fullness) / (int)cvbr->i_btrate;
                    newqs -= adjust;
                }
            }
        }
        newqs = MSRC_MIN(hi_qs, newqs);
        newqs = MSRC_MAX(lo_qs, newqs);
    }
    else if (cvbr->i_method == RQCT_METHOD_VBR)
    {
        int lo_qs, hi_qs;
        if (rqcx->i_period > 1 && IS_IPIC(rqct->i_pictyp))
        {
            newqs = cvbr->i_last_avg_qs;
            if ((cvbr->i_last_avg_qs*64) > cvbr->i_lt_qs64)
                newqs = newqs - (1<<QS_SHIFT_FACTOR);
        }
        else
        {
            int adj_ltq;
            if (cvbr->i_frm_nr >= cvbr->i_fmrate || rqcx->i_period == 1)
            {
                //int64 tmp = div_s64((int64)cvbr->i_lt_qs64*div_s64(cvbr->i_totbit,cvbr->i_frm_nr),cvbr->i_avgbpf);
                //MFE_ASSERT((tmp >> 6) < (1<<31));
                adj_ltq = (int)(div_s64((int64)cvbr->i_lt_qs64*div_s64(cvbr->i_totbit,cvbr->i_frm_nr),cvbr->i_avgbpf)>>6);
            }
            else
            {
                adj_ltq = cvbr->i_lt_qs64/64;   // Wait for stabilization
            }
            MFE_ASSERT(cvbr->i_tgt_bs>0);
            newqs = (adj_ltq * cvbr->i_avgbpf) / cvbr->i_tgt_bs;

            adjust = MSRC_MAX((2<<QS_SHIFT_FACTOR),adj_ltq>>2);
            lo_qs = adj_ltq - adjust;
            hi_qs = adj_ltq + adjust;

            if (cvbr->i_last_frm_bs > cvbr->i_last_tgt_bs)
            {
                adjust = (int)(cvbr->i_last_frm_bs/cvbr->i_last_tgt_bs);
                if (adjust > 2)
                    adjust = 2;
                hi_qs += adjust;
            }

            if (cvbr->i_avgbpf > cvbr->i_tgt_bs)
                newqs = MSRC_MIN(hi_qs,newqs);
            else
                newqs = MSRC_MAX(lo_qs,newqs);
        }
    }
    return newqs;
}

static int qp2qs(mfe_cvbr* cvbr,  int qp)
{
    int i;
    int qs;
    static const int QP2QSTEP[6] = { 20, 22, 26, 28, 32, 36 };

    qs = QP2QSTEP[qp%6];

    for (i = 0; i < (qp/6); i++)
        qs *= 2;

    return qs;
}

static int qs2qp(mfe_cvbr* cvbr, int qsx32)
{
    int q_per = 0, q_rem = 0;

    if (qsx32 <= cvbr->i_min_qs)
        return MIN_QP;
    else if (qsx32 > cvbr->i_max_qs)
        return MAX_QP;

    while (qsx32 > qp2qs(cvbr, 5))
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
