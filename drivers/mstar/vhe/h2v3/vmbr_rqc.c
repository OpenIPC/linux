
#include <mhve_ops.h>
#include <mhve_ios.h>
#include <rqct_ops.h>

#include <h2v3_def.h>
#include <h2v3_rqc.h>
#include <h2v3_reg.h>
#include <h2v3_ios.h>

#define VMBR_NAME       "VMBR"
#define VMBR_VER_MJR    0
#define VMBR_VER_MNR    0
#define VMBR_VER_EXT    00

#define PICTYPES        2

typedef struct vmbr_rqc
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
    short   i_qpMax, i_qpMin;
    int     i_btrate, i_pixels;
    int     i_btppix, i_btpfrm, i_btpgop;
    int     i_budget, i_ipbias;
    int     i_bucket, i_radius;
    int     i_imbpos, i_errpro;
    int     i_degree, i_margin;
    int     i_rebits, i_imbits;
    int     i_upperb, i_lowerb;

    iir_t   iir_rqprod[PICTYPES];
    acc_t   acc_rqprod;
    acc_t   acc_bitcnt;
    int     i_intrabit;

} vmbr_rqc;

#define COMPLEX_I   (1<<24)
#define COMPLEX_P   (1<<20)

#define QP_UPPER    (48)
#define QP_LOWER    (22)
#define QP_RANGE    (QP_UPPER-QP_LOWER)

static void _vmbr_free(rqct_ops* rqct) { MEM_FREE(rqct); }
static int   _seq_sync(rqct_ops*);
static int   _seq_conf(rqct_ops*);
static int   _seq_done(rqct_ops*);
static int   _enc_conf(rqct_ops* rqct, mhve_job* mjob);
static int   _enc_done(rqct_ops* rqct, mhve_job* mjob);

char* vmbr_describe(void)
{
    static char line[64];
    sprintf(line,"%s@v%d.%d.%02d:pic rq-controller",VMBR_NAME,VMBR_VER_MJR,VMBR_VER_MNR,VMBR_VER_EXT);
    return line;
}

void* vmbr_allocate(void)
{
    vmbr_rqc* priv = MEM_ALLC(sizeof(vmbr_rqc));
    h2v3_rqc* rqcx = NULL;
    rqct_ops* rqct = NULL;

    if (!priv)
    {
        printk(KERN_ERR"%s() alloc fail\n", __func__);
        return priv;
    }

    rqcx = &priv->rqcx;
    rqct = &rqcx->rqct;
    /* RQCT default setting */
    MEM_COPY(rqct->name, VMBR_NAME, 5);
    rqct->release = _vmbr_free;
    rqct->seq_sync = _seq_sync;
    rqct->seq_done = _seq_done;
    rqct->seq_conf = _seq_conf;
    rqct->set_rqcf = hrqc_set_conf;
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
    rqcx->attr.i_iupperq = QP_UPPER;
    rqcx->attr.i_ilowerq = QP_LOWER;
    rqcx->attr.i_pupperq = QP_UPPER;
    rqcx->attr.i_plowerq = QP_LOWER;
    rqcx->attr.i_iperiod = 0;
    rqcx->attr.i_ltrperiod = 0;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.b_logoff = 0;
    rqcx->attr.b_dqmstat=-1;
    rqcx->i_config = 0;
    rqcx->i_iperiod = 0;
    rqcx->i_ipcount = 0;
    rqcx->i_ltrperiod = 0;
    rqcx->i_ltrpcount = 0;
    /* h2br default setting */
    priv->i_levelq = QP_DEF;

    printk("%s\n",vmbr_describe());

    return priv;
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

static int _seq_conf(rqct_ops* rqct)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    vmbr_rqc* priv = (vmbr_rqc*)rqcx;
    int Xi = COMPLEX_I;
    int Xp = COMPLEX_P;
    short upperq = rqcx->attr.i_iupperq>rqcx->attr.i_pupperq?rqcx->attr.i_iupperq:rqcx->attr.i_pupperq;
    short lowerq = rqcx->attr.i_ilowerq<rqcx->attr.i_plowerq?rqcx->attr.i_ilowerq:rqcx->attr.i_plowerq;

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
    priv->i_pixels = (int)rqcx->attr.i_pict_w*rqcx->attr.i_pict_h;
    rqcx->i_ipcount =
    rqcx->i_iperiod = rqcx->attr.i_iperiod;
    rqcx->i_ltrpcount =
    rqcx->i_ltrperiod = rqcx->attr.i_ltrperiod;
    priv->i_smooth = rqcx->i_iperiod;
    priv->i_btpfrm = (int)div_s64((int64)priv->i_btrate*priv->d_fmrate,priv->n_fmrate);
    priv->i_btppix = (int)div_s64((int64)priv->i_btpfrm*BPP_FAC,priv->i_pixels);
    priv->i_btpgop = priv->i_btpfrm*rqcx->i_iperiod;
    priv->i_degree = priv->i_btrate/(QP_RANGE*2);
    priv->i_degree = priv->i_degree ? priv->i_degree : 1;
    priv->i_margin = priv->i_degree*(QP_RANGE);
    priv->i_bucket = priv->i_margin*2;
    priv->i_radius = priv->i_margin/2;
#define RATIO_MIN   ( 16)
#define RATIO_MAX   (256)
#define RATIO_FAC   (1<<12)
    if (rqcx->i_iperiod > 1 && priv->i_method != RQCT_MODE_CQP)
    {
        int rt = (int)div_s64((int64)(rqcx->i_iperiod+RATIO_MIN-1)*RATIO_FAC,RATIO_MIN);
        int mt = (int)div_s64((int64)(rqcx->i_iperiod+RATIO_MAX-1)*RATIO_FAC,RATIO_MAX);
        if ((unsigned)(priv->i_levelq-16) <= 24 )
            priv->i_levelq = _MAX(mt,(int)div_s64((int64)priv->i_btpgop*qp2qs(priv->i_levelq)*RATIO_FAC,Xi));
        priv->i_levelq = qs2qp((int)div_s64((int64)Xi*rt,priv->i_btppix*rqcx->i_iperiod*RATIO_FAC));
        priv->i_levelq = _MAX(16,_MIN(40,priv->i_levelq));
        rt = rt - RATIO_FAC;
        Xp = (int)div_s64((int64)Xi*rt,(rqcx->i_iperiod-1)*RATIO_FAC);
    }
    priv->i_rebits =
    priv->i_imbits =
    priv->i_imbpos = priv->i_margin+(priv->i_levelq-QP_LOWER)*priv->i_degree;
    priv->i_lowerb = priv->i_margin+(lowerq-QP_LOWER)*priv->i_degree;
    priv->i_upperb = priv->i_margin+(upperq-QP_LOWER)*priv->i_degree;
    iir_init(&priv->iir_rqprod[RQCT_PICTYP_I], Xi, KAPA_ONE*3/4);
    iir_init(&priv->iir_rqprod[RQCT_PICTYP_P], Xp, KAPA_ONE*2/4);
    acc_init(&priv->acc_rqprod, Xp);
    acc_init(&priv->acc_bitcnt, priv->i_btpfrm*3/4);

    return 0;
}

static int _enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    vmbr_rqc* priv = (vmbr_rqc*)rqcx;
    int qsc, bpp, pqp, dqp;
    int err = 0;
    switch (priv->i_method)
    {
    case RQCT_MODE_CQP:
        rqct->i_enc_qp = priv->i_levelq;
        if (IS_IPIC(rqct->i_pictyp))
            rqct->i_enc_qp -= priv->i_deltaq;
        break;
    case RQCT_MODE_CBR:
    case RQCT_MODE_VBR:
    default:
        if (IS_IPIC(rqct->i_pictyp))
        {
            int Xi = iir_data(&priv->iir_rqprod[RQCT_PICTYP_I]);
            int Xp = acc_calc(&priv->acc_rqprod, rqcx->i_iperiod)/rqcx->i_iperiod;
            int Ri = (int)div_s64((int64)qp2qs(40+priv->i_deltaq)*IPR_FAC,qp2qs(40));
            int Rp = (int)div_s64((int64)Xp*IPR_FAC,Xi);
            priv->i_budget = (int)div_s64((int64)priv->i_btpgop*Ri,(rqcx->i_iperiod*Rp+Ri-Rp));
            bpp = (int)div_s64((int64)priv->i_budget*BPP_FAC,priv->i_pixels);
            bpp = bpp ? bpp : 1;
            qsc = Xi/bpp;
            pqp = _MAX(qs2qp(qsc),priv->i_levelq-priv->i_deltaq);
            priv->i_ipbias = 0;
            rqct->i_enc_qp = pqp;
            priv->i_qpMin = priv->i_ilowerq;
            priv->i_qpMax = priv->i_iupperq;
            break;
        }
        pqp = QP_LOWER+(priv->i_imbpos-priv->i_margin)/priv->i_degree;
        dqp = qdelta((int)div_s64((int64)(priv->i_imbits-priv->i_imbpos)*DQBIAS_FAC,priv->i_radius));
        priv->i_levelq = pqp;
        priv->i_budget = priv->i_btpfrm-priv->i_ipbias;
        priv->i_budget = _MAX(priv->i_budget,priv->i_btpfrm/16);
        rqct->i_enc_qp = _MAX(_MIN(priv->i_pupperq,pqp+dqp),priv->i_plowerq);
        priv->i_qpMin = priv->i_plowerq;
        priv->i_qpMax = priv->i_pupperq;
        break;
    }
    if (!(err = hrqc_enc_conf(rqct, mjob)))
    {
        h2v3_mir* mirr = (h2v3_mir*)mjob;
        uint* regs = mirr->regs;
        H2PutRegVal(regs, HWIF_ENC_QP_MAX,  priv->i_qpMax);
        H2PutRegVal(regs, HWIF_ENC_QP_MIN,  priv->i_qpMin);
    }
    return err;
}

#define BTRATE_DUR  32

static int _enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    vmbr_rqc* priv = (vmbr_rqc*)rqcx;
    int bcn = mjob->i_bits;
    int dur, bps, bpp, cpx, inc, dec;

    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;

    bpp = (int)div_s64((int64)bcn*BPP_FAC,priv->i_pixels);
    cpx = bpp*qp2qs(rqct->i_enc_qp);
    dur = rqcx->i_iperiod;

    if (IS_PPIC(rqct->i_pictyp))
    {
        acc_push(&priv->acc_bitcnt, bcn);
        acc_push(&priv->acc_rqprod, cpx);
        iir_push(&priv->iir_rqprod[RQCT_PICTYP_P], acc_calc(&priv->acc_rqprod,8)/8);
        priv->i_errpro = (priv->i_ipbias-priv->i_btpfrm+bcn);
        priv->i_imbits += priv->i_errpro;
        priv->i_imbits = _MIN(priv->i_imbits,priv->i_upperb+priv->i_radius);
        priv->i_imbits = _MAX(priv->i_imbits,priv->i_lowerb);
        priv->i_rebits = _MAX(priv->i_rebits,priv->i_lowerb);
    }
    else
    {
        priv->i_intrabit = bcn;
        iir_push(&priv->iir_rqprod[RQCT_PICTYP_I], cpx);
        if (priv->i_imbits > priv->i_upperb)
            priv->i_rebits = priv->i_imbits;
        priv->i_errpro = (priv->i_rebits-priv->i_imbits);
        priv->i_imbits += priv->i_errpro;
        priv->i_imbits = _MIN(_MAX(priv->i_imbits,priv->i_lowerb),priv->i_upperb);
        priv->i_rebits = _MAX(priv->i_rebits,priv->i_lowerb);
        if (dur >= 2)
            priv->i_ipbias = (bcn-priv->i_btpfrm)/(dur-1);
        if (dur == 0)
            acc_push(&priv->acc_bitcnt, bcn);
    }
    priv->i_rebits += (bcn-priv->i_btpfrm);

    if (dur > 1)
        bps = acc_calc(&priv->acc_bitcnt, dur-1) + priv->i_intrabit;
    else
        bps = acc_calc(&priv->acc_bitcnt, dur=BTRATE_DUR);
    bps = (int)div_s64((int64)bps*priv->n_fmrate,priv->d_fmrate*dur);
#if 0
    printk("<%s#%04d@%c:%2d> bits:%8d avbr:%8d\n",\
    rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp?'P':'I',rqct->i_enc_qp,rqct->i_bitcnt,bps);
#endif
    bcn = priv->i_imbits-priv->i_imbpos;
    inc = priv->i_btpfrm/64;
    dec = -3*inc;
    bcn = _MIN(inc,_MAX(bcn,dec));
    priv->i_imbpos += bcn;
    priv->i_imbpos = _MIN(_MAX(priv->i_imbpos,priv->i_lowerb),priv->i_upperb);
    rqcx->i_refcnt++;
    return 0;
}
