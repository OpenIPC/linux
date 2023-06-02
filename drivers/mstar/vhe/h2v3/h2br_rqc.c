
#include <mhve_ops.h>
#include <mhve_ios.h>
#include <rqct_ops.h>

#include <h2v3_def.h>
#include <h2v3_rqc.h>
#include <h2v3_reg.h>
#include <h2v3_ios.h>

#define H2BR_NAME       "H2BR"
#define H2BR_VER_MJR    0
#define H2BR_VER_MNR    0
#define H2BR_VER_EXT    00
#define QP_DEGREE   4

/* VSi rate-controller */
#include <h2br/sw_slice.h>
#include <h2br/rate_control_picture.h>

typedef struct h2br_rqc
{
    /* h2v3_rqc MUST be the first element */
    h2v3_rqc    rqcx;

    short   i_method;
    short   i_levelq;
    short   i_deltaq;
    short   i_smooth;
    short   n_fmrate, d_fmrate;
    short   i_iupperq, i_ilowerq;
    short   i_pupperq, i_plowerq;
    int     i_btrate;
    int     i_btpfrm;
/* VSi rate-controller */
    int     blockRcSize;
    int     picComplex;
    int     sumofQp;
    int     sumofQn;
    int     dQpMbGain;
    int     mbComplex;
    int     ctbMinBits;
    int     ctbMaxBits;
    int     totLcuBits;
    int     timeInc;
    hevcRateControl_s rc;
    acc_t   acc_bitcnt;

} h2br_rqc;

#define QP_UPPER        (48)
#define QP_LOWER        (22)

static void _h2br_free(rqct_ops* rqct) { MEM_FREE(rqct); }
static int   _seq_sync(rqct_ops*);
static int   _seq_done(rqct_ops*);
static int   _seq_conf(rqct_ops*);
static int   _enc_conf(rqct_ops* rqct, mhve_job* mjob);
static int   _enc_done(rqct_ops* rqct, mhve_job* mjob);

char* h2br_describe(void)
{
    static char line[64];
    sprintf(line,"%s@v%d.%d.%02d:vsi rq-controller",H2BR_NAME,H2BR_VER_MJR,H2BR_VER_MNR,H2BR_VER_EXT);
    return line;
}

static int _h2br_set_conf(rqct_ops* rqct, rqct_cfg* conf)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    h2br_rqc* priv = (h2br_rqc*)rqcx;
    int err = -1;

    switch (conf->type)
    {
    case RQCT_CFG_DQP:
        rqcx->attr.i_deltaq = MAX(MIN(conf->dqp.i_dqp, DQP_MAX), DQP_MIN);

        priv->i_deltaq = rqcx->attr.i_deltaq;
        priv->rc.intraQpDelta =-priv->i_deltaq << QP_FRACTIONAL_BITS;
        err = 0;
        break;
    case RQCT_CFG_QPR:
        if (conf->qpr.i_iupperq < conf->qpr.i_ilowerq)
        {
            err = -1;
            break;
        }
        if (conf->qpr.i_pupperq < conf->qpr.i_plowerq)
        {
            err = -1;
            break;
        }
        rqcx->attr.i_iupperq = MAX(MIN(conf->qpr.i_iupperq, QP_MAX), QP_MIN);
        rqcx->attr.i_ilowerq = MAX(MIN(conf->qpr.i_ilowerq, QP_MAX), QP_MIN);
        rqcx->attr.i_pupperq = MAX(MIN(conf->qpr.i_pupperq, QP_MAX), QP_MIN);
        rqcx->attr.i_plowerq = MAX(MIN(conf->qpr.i_plowerq, QP_MAX), QP_MIN);

        priv->i_iupperq = MAX(MIN(rqcx->attr.i_iupperq, QP_MAX), QP_MIN);
        priv->i_ilowerq = MAX(MIN(rqcx->attr.i_ilowerq, QP_MAX), QP_MIN);
        priv->i_pupperq = MAX(MIN(rqcx->attr.i_pupperq, QP_MAX), QP_MIN);
        priv->i_plowerq = MAX(MIN(rqcx->attr.i_plowerq, QP_MAX), QP_MIN);
        err = 0;
        break;
    default:
        err = hrqc_set_conf(rqct, conf);
        break;
    }
    return err;
}

void* h2br_allocate(void)
{
    h2br_rqc* priv = MEM_ALLC(sizeof(h2br_rqc));
    h2v3_rqc* rqcx = NULL;
    rqct_ops* rqct = NULL;
    int bpfrm;

    if (!priv)
    {
        printk(KERN_ERR"%s() alloc fail\n", __func__);
        return priv;
    }

    rqcx = &priv->rqcx;
    rqct = &rqcx->rqct;
    /* RQCT default setting */
    MEM_COPY(rqct->name, H2BR_NAME, 5);
    rqct->release = _h2br_free;
    rqct->seq_sync = _seq_sync;
    rqct->seq_done = _seq_done;
    rqct->seq_conf = _seq_conf;
    rqct->set_rqcf = _h2br_set_conf;
//    rqct->set_rqcf = hrqc_set_conf;
    rqct->get_rqcf = hrqc_get_conf;
    rqct->enc_buff = hrqc_enc_buff;
    rqct->enc_conf = _enc_conf;
    rqct->enc_done = _enc_done;
    rqct->i_enc_qp = 26;
    rqct->i_enc_nr = 0;
    rqct->i_enc_bs = 0;
    /* H2v3 default setting */
    rqcx->attr.i_method = RQCT_MODE_CQP;
    rqcx->attr.i_pict_w = 0;
    rqcx->attr.i_pict_h = 0;
    rqcx->attr.n_fmrate =30;
    rqcx->attr.d_fmrate = 1;
    rqcx->attr.i_leadqp =-1;
    rqcx->attr.i_deltaq = QP_IFRAME_DELTA;
//    rqcx->attr.i_upperq = QP_MAX;
//    rqcx->attr.i_lowerq = QP_MIN;
    rqcx->attr.i_iupperq = QP_UPPER;
    rqcx->attr.i_ilowerq = QP_LOWER;
    rqcx->attr.i_pupperq = QP_UPPER;
    rqcx->attr.i_plowerq = QP_LOWER;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.b_logoff = 1;
    rqcx->attr.b_dqmstat=-1;
    rqcx->i_config = 0;
    rqcx->i_iperiod = 0;
    rqcx->i_ipcount = 0;
    rqcx->i_ltrperiod = 0;
    rqcx->i_ltrpcount = 0;
    /* h2br default setting */
    /* SetParameters() */
    priv->rc.outRateDenom = rqcx->attr.n_fmrate;
    priv->rc.outRateNum = rqcx->attr.d_fmrate;
    priv->rc.picArea = 0;
    priv->rc.ctbPerPic = 0;
    priv->rc.ctbRows =  0;
    priv->rc.ctbSize = 64;
    {
        hevcVirtualBuffer_s *vb = &priv->rc.virtualBuffer;
        vb->bitRate = rqcx->attr.i_btrate;
        vb->unitsInTic = rqcx->attr.d_fmrate;
        vb->timeScale = rqcx->attr.n_fmrate;
        vb->bufferSize = 1000000;// FIXME: HEVCMaxCPBS[inst->levelIdx];
    }
    priv->rc.hrd = ENCHW_NO;
    priv->rc.picRc = ENCHW_NO;
    priv->rc.ctbRc = ENCHW_NO;
    priv->rc.picSkip = ENCHW_NO;
#define HEVCENC_DEFAULT_QP  26
    priv->rc.qpHdr = HEVCENC_DEFAULT_QP << QP_FRACTIONAL_BITS;
    priv->rc.qpMin =  0 << QP_FRACTIONAL_BITS;
    priv->rc.qpMax = 51 << QP_FRACTIONAL_BITS;
    priv->rc.frameCoded = ENCHW_YES;
    priv->rc.sliceTypeCur = I_SLICE;
    priv->rc.sliceTypePrev = P_SLICE;
    priv->rc.gopLen = 150;
    priv->rc.intraQpDelta =-5 << QP_FRACTIONAL_BITS;
    priv->rc.fixedIntraQp = 0 << QP_FRACTIONAL_BITS;
    priv->rc.frameQpDelta = 0 << QP_FRACTIONAL_BITS;
    priv->rc.gopPoc = 0;
    /* HEVCEncInit() */
    priv->rc.monitorFrames = rqcx->attr.n_fmrate/rqcx->attr.d_fmrate;
    priv->rc.picRc = ENCHW_NO;
    priv->rc.picSkip = ENCHW_NO;
    priv->rc.qpHdr = -1<<QP_FRACTIONAL_BITS;
    priv->rc.qpMin =  0<<QP_FRACTIONAL_BITS;
    priv->rc.qpMax = 51<<QP_FRACTIONAL_BITS;
    priv->rc.virtualBuffer.bitRate = 1000000;
    priv->rc.virtualBuffer.bufferSize = 1000000;
    priv->rc.gopLen = 150;
    priv->rc.intraQpDelta =-5 << QP_FRACTIONAL_BITS;
    priv->rc.fixedIntraQp = 0 << QP_FRACTIONAL_BITS;
    priv->rc.tolMovingBitRate = 2000;
    priv->rc.f_tolMovingBitRate = 2000.0;
    bpfrm = HevcCalculate(priv->rc.virtualBuffer.bitRate, priv->rc.outRateDenom, priv->rc.outRateNum);
    priv->rc.maxPicSizeI = bpfrm*(1+20);
    priv->rc.maxPicSizeP = bpfrm*(1+20);
    priv->rc.maxPicSizeB = bpfrm*(1+20);
    priv->rc.minPicSizeI = bpfrm/(1+20);
    priv->rc.minPicSizeP = bpfrm/(1+20);
    priv->rc.minPicSizeB = bpfrm/(1+20);
/*
    if (HevcInitRc(&priv->rc, 1) != ENCHW_OK)
    {
        rqct->release(rqct);
        return NULL;
    }
*/
    priv->rc.sei.enabled = ENCHW_NO;
    priv->rc.sei.insertRecoveryPointMessage = 0;
    priv->rc.sei.recoveryFrameCnt = 0;
    priv->timeInc = 0;

    printk("%s\n",h2br_describe());

    return priv;
}

static int _seq_sync(rqct_ops* rqct)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    rqcx->i_ipcount = 0;
    rqcx->i_ltrpcount = 0;
    return 0;
}

static int _seq_done(rqct_ops* rqct)
{
    return 0;
}

#define CTBBITS     (6)
#define CTBSIZE     (1<<CTBBITS)
#define CTBNUMB(x)  (((x)+(CTBSIZE-1))>>CTBBITS)

const int HEVCMaxCPBS[13] =
{
  350000,  1500000,  3000000,  6000000,
10000000, 12000000, 20000000, 25000000,
40000000, 60000000, 60000000,120000000,240000000,
};

static int _seq_conf(rqct_ops* rqct)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    h2br_rqc* priv = (h2br_rqc*)rqcx;
    int bpf, rng = 2000;
    int cpb, bps, tmp, i;
    rqcx->b_rqcbit&=~RQC_DQRC_ENABLE;
    priv->i_btrate = rqcx->attr.i_btrate;
    priv->i_method = rqcx->attr.i_method;
    if (priv->i_btrate == 0)
        priv->i_method = RQCT_MODE_CQP;
    priv->i_deltaq = rqcx->attr.i_deltaq;
    priv->i_levelq = rqcx->attr.i_leadqp;
    if (rqcx->attr.i_leadqp < 0)
        priv->i_levelq = QP_DEF;
    priv->n_fmrate = rqcx->attr.n_fmrate;
    priv->d_fmrate = rqcx->attr.d_fmrate;
    priv->i_iupperq = MAX(MIN(rqcx->attr.i_iupperq, QP_MAX), QP_MIN);
    priv->i_ilowerq = MAX(MIN(rqcx->attr.i_ilowerq, QP_MAX), QP_MIN);
    priv->i_pupperq = MAX(MIN(rqcx->attr.i_pupperq, QP_MAX), QP_MIN);
    priv->i_plowerq = MAX(MIN(rqcx->attr.i_plowerq, QP_MAX), QP_MIN);
    priv->i_btpfrm = (int)div_s64((int64)priv->i_btrate*priv->d_fmrate,priv->n_fmrate);
    rqcx->i_ipcount =
    rqcx->i_iperiod = rqcx->attr.i_iperiod;
    rqcx->i_ltrpcount =
    rqcx->i_ltrperiod = rqcx->attr.i_ltrperiod;
    /* VSi rate-controller */
    priv->rc.outRateDenom = rqcx->attr.d_fmrate;
    priv->rc.outRateNum = rqcx->attr.n_fmrate;
    priv->rc.picArea = rqcx->attr.i_pict_w*rqcx->attr.i_pict_h;
    priv->rc.ctbPerPic = CTBNUMB(rqcx->attr.i_pict_w)*CTBNUMB(rqcx->attr.i_pict_h);
    priv->rc.ctbSize = CTBSIZE;
    priv->rc.ctbRows = CTBNUMB(rqcx->attr.i_pict_h);
    bpf = HevcCalculate(priv->i_btrate, priv->d_fmrate, priv->n_fmrate);
    cpb = 1000000;//HEVCMaxCPBS[10];
    bps = rqcx->attr.i_btrate;
    tmp = 3*8*priv->rc.ctbPerPic*priv->rc.ctbSize*priv->rc.ctbSize/2;
    tmp = HevcCalculate(tmp, priv->rc.outRateNum, priv->rc.outRateDenom);
    tmp = (int)div_s64((int64)tmp*5,3);
    if (bps > tmp)
        bps = tmp;
    tmp = HevcCalculate(bps, priv->rc.outRateDenom, priv->rc.outRateNum);
    cpb = MAX(cpb,tmp);
    i = 0;
    tmp = cpb;
    while (4095 < (tmp >> (4 + i++))) ;
    cpb = (tmp >> (4+i))<<(4+i);
    priv->rc.hrd = ENCHW_NO;
    priv->rc.picSkip = ENCHW_NO;
    priv->rc.picRc = ENCHW_NO;
    priv->rc.ctbRc = ENCHW_NO;
    if (priv->i_method != RQCT_MODE_CQP)
    {
        rqcx->b_rqcbit|= RQC_DQRC_ENABLE;
        priv->rc.picRc = ENCHW_YES;
        priv->rc.ctbRc = ENCHW_YES;
    }
    priv->rc.qpHdr = priv->i_levelq << QP_FRACTIONAL_BITS;
//    priv->rc.qpMin = priv->i_lowerq << QP_FRACTIONAL_BITS;
//    priv->rc.qpMax = priv->i_upperq << QP_FRACTIONAL_BITS;
    priv->rc.virtualBuffer.bitRate = bps;
    priv->rc.virtualBuffer.bufferSize = cpb;
    priv->rc.virtualBuffer.unitsInTic = rqcx->attr.d_fmrate;
    priv->rc.virtualBuffer.timeScale = rqcx->attr.n_fmrate;
    priv->rc.gopLen = rqcx->i_iperiod;
    priv->rc.maxPicSizeI = bpf/100*(100+rng);
    priv->rc.maxPicSizeP = bpf/100*(100+rng);
    priv->rc.maxPicSizeB = bpf/100*(100+rng);
    priv->rc.minPicSizeI = bpf*100/(100+rng);
    priv->rc.minPicSizeP = bpf*100/(100+rng);
    priv->rc.minPicSizeB = bpf*100/(100+rng);
    //priv->rc.tolMovingBitRate = 20;
    priv->rc.tolMovingBitRate = 0;
    priv->rc.monitorFrames = priv->rc.outRateNum/priv->rc.outRateDenom;
    priv->rc.intraQpDelta =-priv->i_deltaq << QP_FRACTIONAL_BITS;
    priv->rc.fixedIntraQp = 0;
    priv->rc.frameQpDelta = 0 << QP_FRACTIONAL_BITS;
#if defined(RCTRACE)
printk("[H2V3E]HevcInitRc():\n");
printk("[H2V3E]outRateNum       =%8d\n",priv->rc.outRateNum);
printk("[H2V3E]outRateDenom     =%8d\n",priv->rc.outRateDenom);
printk("[H2V3E]picArea          =%8d\n",priv->rc.picArea);
printk("[H2V3E]ctbPerPic        =%8d\n",priv->rc.ctbPerPic);
printk("[H2V3E]ctbRows          =%8d\n",priv->rc.ctbRows);
printk("[H2V3E]ctbSize          =%8d\n",priv->rc.ctbSize);
printk("[H2V3E]vb.bufferSize    =%8d\n",priv->rc.virtualBuffer.bufferSize);
printk("[H2V3E]picRc            =%8d\n",priv->rc.picRc);
printk("[H2V3E]ctbRc            =%8d\n",priv->rc.ctbRc);
printk("[H2V3E]picSkip          =%8d\n",priv->rc.picSkip);
printk("[H2V3E]qpHdr            =%8d\n",priv->rc.qpHdr);
printk("[H2V3E]qpMin            =%8d\n",priv->rc.qpMin);
printk("[H2V3E]qpMax            =%8d\n",priv->rc.qpMax);
printk("[H2V3E]vb.bitRate       =%8d\n",priv->rc.virtualBuffer.bitRate);
printk("[H2V3E]gopLen           =%8d\n",priv->rc.gopLen);
printk("[H2V3E]maxPicSizeI      =%8d\n",priv->rc.maxPicSizeI);
printk("[H2V3E]maxPicSizeP      =%8d\n",priv->rc.maxPicSizeP);
printk("[H2V3E]maxPicSizeB      =%8d\n",priv->rc.maxPicSizeB);
printk("[H2V3E]minPicSizeI      =%8d\n",priv->rc.minPicSizeI);
printk("[H2V3E]minPicSizeP      =%8d\n",priv->rc.minPicSizeP);
printk("[H2V3E]minPicSizeB      =%8d\n",priv->rc.minPicSizeB);
printk("[H2V3E]tolMovingBitRate =%8d\n",priv->rc.tolMovingBitRate);
printk("[H2V3E]monitorFrames    =%8d\n",priv->rc.monitorFrames);
printk("[H2V3E]intraQpDelta     =%8d\n",priv->rc.intraQpDelta);
printk("[H2V3E]fixedIntraQp     =%8d\n",priv->rc.fixedIntraQp);
printk("[H2V3E]frameQpDelta     =%8d\n",priv->rc.frameQpDelta);
#endif
    HevcInitRc(&priv->rc, 1);
    rqct->i_enc_qp = priv->rc.qpHdr >> QP_FRACTIONAL_BITS;
    acc_init(&priv->acc_bitcnt, priv->i_btpfrm*3/4);
    return 0;
}

#define SWAP(a,b) {typeof(a) _c=(a);(a)=(b);(b)=_c;}
#define QP_FRACTIONAL_PART  ((1<<QP_FRACTIONAL_BITS)-1)

static int _enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    h2br_rqc* priv = (h2br_rqc*)rqcx;
    h2v3_mir* mirr = (h2v3_mir*)mjob;
    uint* regs = mirr->regs;
    int err = 0;
    do
    {   /* Rate control */
        int64 dqpgain;
        priv->rc.hierarchial_bit_allocation_GOP_size = 1;//pEncIn->gopSize;
        if (IS_IPIC(rqct->i_pictyp))
        {
            priv->rc.gopPoc = 0;
            priv->rc.encoded_frame_number = 0;
            priv->rc.qpMin = priv->i_ilowerq << QP_FRACTIONAL_BITS;
            priv->rc.qpMax = priv->i_iupperq << QP_FRACTIONAL_BITS;
        }
        else
        {
            priv->rc.frameQpDelta = 0;
            priv->rc.gopPoc = 0;
            priv->rc.encoded_frame_number = 0;
            if (priv->rc.gopPoc > 0)
                priv->rc.gopPoc-= 1;
            priv->rc.qpMin = priv->i_plowerq << QP_FRACTIONAL_BITS;
            priv->rc.qpMax = priv->i_pupperq << QP_FRACTIONAL_BITS;
/*
            if (pEncIn->gopSize > 8)
            {
                priv->rc.hierarchial_bit_allocation_GOP_size = 1;
                priv->rc.gopPoc = 0;
                priv->rc.encoded_frame_number = 0;
            }
*/
        }
#if defined(RCTRACE)
printk("[H2V3E]HevcBeforePicRc():\n");
printk("[H2V3E]outRateNum       =%8d\n",priv->rc.outRateNum);
printk("[H2V3E]outRateDenom     =%8d\n",priv->rc.outRateDenom);
printk("[H2V3E]picArea          =%8d\n",priv->rc.picArea);
printk("[H2V3E]ctbPerPic        =%8d\n",priv->rc.ctbPerPic);
printk("[H2V3E]ctbRows          =%8d\n",priv->rc.ctbRows);
printk("[H2V3E]ctbSize          =%8d\n",priv->rc.ctbSize);
printk("[H2V3E]vb.bufferSize    =%8d\n",priv->rc.virtualBuffer.bufferSize);
printk("[H2V3E]picRc            =%8d\n",priv->rc.picRc);
printk("[H2V3E]ctbRc            =%8d\n",priv->rc.ctbRc);
printk("[H2V3E]picSkip          =%8d\n",priv->rc.picSkip);
printk("[H2V3E]qpHdr            =%8d\n",priv->rc.qpHdr);
printk("[H2V3E]qpMin            =%8d\n",priv->rc.qpMin);
printk("[H2V3E]qpMax            =%8d\n",priv->rc.qpMax);
printk("[H2V3E]vb.bitRate       =%8d\n",priv->rc.virtualBuffer.bitRate);
printk("[H2V3E]gopLen           =%8d\n",priv->rc.gopLen);
printk("[H2V3E]maxPicSizeI      =%8d\n",priv->rc.maxPicSizeI);
printk("[H2V3E]maxPicSizeP      =%8d\n",priv->rc.maxPicSizeP);
printk("[H2V3E]maxPicSizeB      =%8d\n",priv->rc.maxPicSizeB);
printk("[H2V3E]minPicSizeI      =%8d\n",priv->rc.minPicSizeI);
printk("[H2V3E]minPicSizeP      =%8d\n",priv->rc.minPicSizeP);
printk("[H2V3E]minPicSizeB      =%8d\n",priv->rc.minPicSizeB);
printk("[H2V3E]tolMovingBitRate =%8d\n",priv->rc.tolMovingBitRate);
printk("[H2V3E]monitorFrames    =%8d\n",priv->rc.monitorFrames);
printk("[H2V3E]intraQpDelta     =%8d\n",priv->rc.intraQpDelta);
printk("[H2V3E]fixedIntraQp     =%8d\n",priv->rc.fixedIntraQp);
printk("[H2V3E]frameQpDelta     =%8d\n",priv->rc.frameQpDelta);
#endif
        priv->rc.ctbMemCurAddr = rqcx->u_rcmcur;
        priv->rc.ctbMemPreAddr = rqcx->u_rcmpre;
        priv->rc.ctbMemCurVirtualAddr = (u32*)rqcx->p_rcmcur;
        priv->rc.ctbMemPreVirtualAddr = (u32*)rqcx->p_rcmpre;
        HevcBeforePicRc(&priv->rc, priv->timeInc, IS_IPIC(rqct->i_pictyp)?I_SLICE:P_SLICE);
        rqct->i_enc_qp = priv->rc.qpHdr >> QP_FRACTIONAL_BITS;
        if(rqcx->attr.i_roidqp[0] > 0 || rqcx->attr.i_roidqp[1] > 0)
        {
            if(IS_IPIC(rqct->i_pictyp))
            {
                if(rqcx->attr.i_ilowerq >= rqct->i_enc_qp)
                    rqct->i_enc_qp = rqcx->attr.i_ilowerq + QP_DEGREE;
            }
            else
            {
                if(rqcx->attr.i_plowerq >= rqct->i_enc_qp)
                    rqct->i_enc_qp = rqcx->attr.i_plowerq + QP_DEGREE;
            }
        }
        H2PutRegVal(regs, HWIF_ENC_BITSRATIO,       priv->rc.bitsRatio);
        H2PutRegVal(regs, HWIF_ENC_CTBRCTHRDMIN,    priv->rc.ctbRcThrdMin);
        H2PutRegVal(regs, HWIF_ENC_CTBRCTHRDMAX,    priv->rc.ctbRcThrdMax);
        H2PutRegVal(regs, HWIF_ENC_TARGETPICSIZE,   priv->rc.targetPicSize);
#if defined(RCTRACE)
printk("[H2BR]bitsRatio    =%8d\n",priv->rc.bitsRatio);
printk("[H2BR]ctbRcThrdMin =%8d\n",priv->rc.ctbRcThrdMin);
printk("[H2BR]ctbRcThrdMax =%8d\n",priv->rc.ctbRcThrdMax);
printk("[H2BR]targetPicSize=%8d\n",priv->rc.targetPicSize);
#endif
        if (IS_IPIC(rqct->i_pictyp))
        {
            H2PutRegVal(regs, HWIF_ENC_MINPICSIZE,  priv->rc.minPicSizeI);
            H2PutRegVal(regs, HWIF_ENC_MAXPICSIZE,  priv->rc.maxPicSizeI);
#if defined(RCTRACE)
printk("[H2BR]minPicSize   =%8d\n",priv->rc.minPicSizeI);
printk("[H2BR]maxPicSize   =%8d\n",priv->rc.maxPicSizeI);
#endif
        }
        else
        {
            H2PutRegVal(regs, HWIF_ENC_MINPICSIZE,  priv->rc.minPicSizeP);
            H2PutRegVal(regs, HWIF_ENC_MAXPICSIZE,  priv->rc.maxPicSizeP);
#if defined(RCTRACE)
printk("[H2BR]minPicSize   =%8d\n",priv->rc.minPicSizeP);
printk("[H2BR]maxPicSize   =%8d\n",priv->rc.maxPicSizeP);
#endif
        }
#define COMPLEXITY_BITS (16)
        dqpgain = div_s64((int64)priv->rc.rcPicComplexity<<COMPLEXITY_BITS,priv->rc.ctbPerPic*8*8);
        if (dqpgain < (14<<COMPLEXITY_BITS))
            dqpgain = (14<<COMPLEXITY_BITS);
        dqpgain = (dqpgain*dqpgain)>>COMPLEXITY_BITS;
        dqpgain = div_s64(dqpgain*16,10<<COMPLEXITY_BITS);
        priv->dQpMbGain = (int)dqpgain;
        priv->mbComplex = 15;
        H2PutRegVal(regs, HWIF_ENC_QP_DELTA_GAIN,       priv->dQpMbGain);
        H2PutRegVal(regs, HWIF_ENC_COMPLEXITY_OFFSET,   priv->mbComplex);
        H2PutRegVal(regs, HWIF_ENC_RCDQP_RANGE,         10);
#if defined(RCTRACE)
printk("[H2BR]dQpMbGain    =%8d\n",priv->dQpMbGain);
printk("[H2BR]mbComplex    =%8d\n",priv->mbComplex);
printk("[H2BR]rcDqpRange   =%8d\n",10);
#endif
        SWAP(rqcx->u_rcmcur,rqcx->u_rcmpre);
        SWAP(rqcx->p_rcmcur,rqcx->p_rcmpre);
        priv->timeInc = priv->rc.outRateDenom;
    }
    while (0);

    if (!(err = hrqc_enc_conf(rqct, mjob)))
    {
        H2PutRegVal(regs, HWIF_ENC_QP_FRAC, priv->rc.qpHdr &QP_FRACTIONAL_PART);
        H2PutRegVal(regs, HWIF_ENC_QP_MAX,  priv->rc.qpMax>>QP_FRACTIONAL_BITS);
        H2PutRegVal(regs, HWIF_ENC_QP_MIN,  priv->rc.qpMin>>QP_FRACTIONAL_BITS);
#if defined(RCTRACE)
printk("[H2BR]qpHdr        =%8d\n",priv->rc.qpHdr>>QP_FRACTIONAL_BITS);
printk("[H2BR]qpMin        =%8d\n",priv->rc.qpMin>>QP_FRACTIONAL_BITS);
printk("[H2BR]qpMax        =%8d\n",priv->rc.qpMax>>QP_FRACTIONAL_BITS);
#endif
    }
    return err;
}

static int _enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    h2br_rqc* priv = (h2br_rqc*)rqcx;
    h2v3_mir* mirr = (h2v3_mir*)mjob;
    uint* regs = mirr->regs;
    int bps, dur = rqcx->i_iperiod;
    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;
#if defined(RCTRACE)
printk("[H2BR]byteCnt      =%8d\n",rqct->i_bitcnt/8);
#endif
    H2GetRegVal(regs, HWIF_ENC_CTBBITSMIN,      &priv->ctbMinBits);
    H2GetRegVal(regs, HWIF_ENC_CTBBITSMAX,      &priv->ctbMaxBits);
    H2GetRegVal(regs, HWIF_ENC_TOTALLCUBITS,    &priv->totLcuBits);
    H2GetRegVal(regs, HWIF_ENC_PIC_COMPLEXITY,  &priv->picComplex);
    H2GetRegVal(regs, HWIF_ENC_QP_SUM,          &priv->sumofQp);
    H2GetRegVal(regs, HWIF_ENC_QP_NUM,          &priv->sumofQn);
    H2PutRegVal(regs, HWIF_ENC_PIC_COMPLEXITY,  0);
    H2PutRegVal(regs, HWIF_ENC_QP_SUM,          0);
    H2PutRegVal(regs, HWIF_ENC_QP_NUM,          0);
    H2PutRegVal(regs, HWIF_ENC_INTRACU8NUM,     0);
    H2PutRegVal(regs, HWIF_ENC_SKIPCU8NUM,      0);
    H2PutRegVal(regs, HWIF_ENC_PBFRAME4NRDCOST, 0);
    acc_push(&priv->acc_bitcnt, mjob->i_bits);
    if ((unsigned)dur > ACC_MASK)
        dur = ACC_MASK;
    bps = acc_calc(&priv->acc_bitcnt, dur);
    bps = (int)div_s64((int64)bps*priv->n_fmrate,priv->d_fmrate*dur);
#if 0
    printk("<%s#%04d@%c:%2d> bits:%8d bps:%8d\n",rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp?'P':'I',rqct->i_enc_qp,rqct->i_bitcnt,bps);
#endif
    priv->rc.ctbRcBitsMin = priv->ctbMaxBits;
    priv->rc.ctbRcBitsMax = priv->ctbMinBits;
    priv->rc.ctbRctotalLcuBit = priv->totLcuBits;
    priv->rc.rcPicComplexity = priv->picComplex;
    HevcAfterPicRc(&priv->rc, 0, rqct->i_bitcnt/8, priv->sumofQp, priv->sumofQn);
    rqcx->i_refcnt++;
    return 0;
}
