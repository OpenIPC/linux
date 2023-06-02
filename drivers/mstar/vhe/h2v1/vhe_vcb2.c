
#include <linux/math64.h>
#include <linux/string.h>
#include <vhe.h>

#include <vhe_util.h>

#define VCB2_NAME       "VCB2"
#define VCB2_VER_MJR     1
#define VCB2_VER_MNR     3
#define VCB2_VER_EXT    00

#define PICTYPES         2

typedef struct vhe_vcb2 {
    vhe_rctl    rqcx;
    int     i_method;
    int     i_btrate;
    int     i_levelq;
    int     i_deltaq;
    int     n_fmrate;
    int     d_fmrate;
    int     i_pixels;
    int     i_pixbit;   // average bits per pixel for target bitrate.
    int     i_frmbit;   // average bits per frame for target bitrate.
    int     i_gopbit;
    int     i_budget;
    int     i_ipbias;
    int     i_errpro;
    int     i_smooth;
    int     i_bucket;
    short   i_upperq, i_lowerq;
    int     i_upperb, i_lowerb;
    int     i_margin;
    int     i_radius;
    int     i_degree;
    int     i_imbase;
    int     i_imbits;   // bit-pos:imaginary
    int     i_rebits;   // bit-pos:real

    iir_t   iir_rqprod[PICTYPES];
    acc_t   acc_rqprod;
    acc_t   acc_bitcnt;
    int     i_intrabit;

} vhe_vcb2;

#define COMPLEX_I   (1<<24)
#define COMPLEX_P   (1<<20)
#define QP_UPPER    (48)
#define QP_LOWER    (12)
#define QP_RANGE    (QP_UPPER-QP_LOWER)
#define QP_MAX      (51)
#define QP_MIN      ( 0)

#define LOGOFF_DEFAULT  1

static int   vcb2_seq_sync(rqct_ops*);
static int   vcb2_seq_done(rqct_ops*);
static int   vcb2_seq_conf(rqct_ops*);
static int   vcb2_enc_conf(rqct_ops*, mhve_job*);
static int   vcb2_enc_done(rqct_ops*, mhve_job*);
static void _vcb2_ops_free(rqct_ops* rqct) { MEM_FREE(rqct); }

void* vcb2_allocate(void)
{
    rqct_ops* rqct = NULL;
    vhe_rctl* rqcx;
    vhe_vcb2* vcb2;

    if (!(rqct = MEM_ALLC(sizeof(vhe_vcb2))))
        return rqct;

    MEM_COPY(rqct->name, VCB2_NAME, 5);
    rqct->release = _vcb2_ops_free;
    rqct->seq_sync = vcb2_seq_sync;
    rqct->seq_done = vcb2_seq_done;
    rqct->set_rqcf = vrqc_set_rqcf;
    rqct->get_rqcf = vrqc_get_rqcf;
    rqct->seq_conf = vcb2_seq_conf;
    rqct->enc_buff = vrqc_enc_buff;
    rqct->enc_conf = vcb2_enc_conf;
    rqct->enc_done = vcb2_enc_done;
    rqct->i_enc_nr = 0;
    rqct->i_enc_bs = 0;
    rqcx = (vhe_rctl*)rqct;
    rqcx->attr.i_pict_w = 0;
    rqcx->attr.i_pict_h = 0;
    rqcx->attr.i_method = RQCT_METHOD_CQP;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.i_leadqp =-1;
    rqcx->attr.i_deltaq = 1;
    rqcx->attr.i_upperq = QP_UPPER;
    rqcx->attr.i_lowerq = QP_LOWER;
    rqcx->attr.n_fmrate =30;
    rqcx->attr.d_fmrate = 1;
    rqcx->attr.i_period = 0;
    rqcx->attr.b_logoff = LOGOFF_DEFAULT;
    rqcx->attr.b_dqmstat=-1;
    rqcx->i_config = 0;
    rqcx->i_period = 0;
    rqcx->i_pcount = 0;
    vcb2 = (vhe_vcb2*)rqcx;
    vcb2->i_levelq =36;

    return rqct;
}

char* vcb2_describe(void)
{
    static char line[64];
    sprintf(line,"%s@v%d.%d.%02d:r&d analysis",VCB2_NAME,VCB2_VER_MJR,VCB2_VER_MNR,VCB2_VER_EXT);
    return line;
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

static int vcb2_seq_done(rqct_ops* rqct)
{
    return 0;
}

static int vcb2_seq_sync(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    rqcx->i_pcount = 0;
    return 0;
}

static int vcb2_seq_conf(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcb2* vcb2 = (vhe_vcb2*)rqcx;

    printk("%s\n",vcb2_describe());

    vcb2->i_method = (int)rqcx->attr.i_method;
    vcb2->i_btrate = (int)rqcx->attr.i_btrate;
    if (rqcx->attr.i_leadqp > 8 && rqcx->attr.i_leadqp < 48)
        vcb2->i_levelq = (int)rqcx->attr.i_leadqp;
    vcb2->i_deltaq = (int)rqcx->attr.i_deltaq;
    vcb2->n_fmrate = (int)rqcx->attr.n_fmrate;
    vcb2->d_fmrate = (int)rqcx->attr.d_fmrate;
    vcb2->i_pixels = (int)rqcx->attr.i_pict_w*rqcx->attr.i_pict_h;
    vcb2->i_upperq = (unsigned)rqcx->attr.i_upperq<52?rqcx->attr.i_upperq:QP_UPPER;
    vcb2->i_lowerq = (unsigned)rqcx->attr.i_lowerq<52?rqcx->attr.i_lowerq:QP_LOWER;
    if (vcb2->i_btrate == 0)
        vcb2->i_method = RQCT_METHOD_CQP;
    rqcx->i_pcount =
    rqcx->i_period = (int)rqcx->attr.i_period;

    vcb2->i_smooth = rqcx->i_period;
    vcb2->i_frmbit = (int)div_s64((int64)vcb2->i_btrate*vcb2->d_fmrate,vcb2->n_fmrate);
    vcb2->i_pixbit = (int)div_s64((int64)vcb2->i_frmbit*BPP_FAC,vcb2->i_pixels);
    vcb2->i_gopbit = vcb2->i_frmbit*rqcx->i_period;
    vcb2->i_degree = vcb2->i_btrate/(QP_RANGE*2);
    vcb2->i_margin = vcb2->i_degree*(QP_RANGE);
    vcb2->i_bucket = vcb2->i_margin<<1;
    vcb2->i_radius = vcb2->i_margin>>1;
    vcb2->i_rebits =
    vcb2->i_imbits =
    vcb2->i_imbase = vcb2->i_margin+(vcb2->i_levelq-QP_LOWER)*vcb2->i_degree;
    vcb2->i_lowerb = vcb2->i_margin+(vcb2->i_lowerq-QP_LOWER)*vcb2->i_degree;
    vcb2->i_upperb = vcb2->i_margin+(vcb2->i_upperq-QP_LOWER)*vcb2->i_degree;

    iir_init(&vcb2->iir_rqprod[RQCT_PICTYP_I], COMPLEX_I, KAPA_ONE*3/4);
    iir_init(&vcb2->iir_rqprod[RQCT_PICTYP_P], COMPLEX_P, KAPA_ONE*2/4);
    acc_init(&vcb2->acc_rqprod, COMPLEX_P);
    acc_init(&vcb2->acc_bitcnt, vcb2->i_frmbit*3/4);

    return 0;
}

static int vcb2_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcb2* vcb2 = (vhe_vcb2*)rqcx;
    int qsc, bpp, pqp, dqp;
    switch (vcb2->i_method)
    {
    case RQCT_METHOD_CQP:
        rqct->i_enc_qp = vcb2->i_levelq;
        if (IS_IPIC(rqct->i_pictyp))
            rqct->i_enc_qp -= vcb2->i_deltaq;
        break;
    case RQCT_METHOD_CBR:
    case RQCT_METHOD_VBR:
    default:
        if (IS_IPIC(rqct->i_pictyp))
        {
            int xi = iir_data(&vcb2->iir_rqprod[RQCT_PICTYP_I]);
            int xp = acc_calc(&vcb2->acc_rqprod, rqcx->i_period)/rqcx->i_period;
            int ri = (int)div_s64((int64)qp2qs(40+vcb2->i_deltaq)*IPR_FAC,qp2qs(40));
            int rp = (int)div_s64((int64)xp*IPR_FAC,xi);
            vcb2->i_budget = (int)div_s64((int64)vcb2->i_gopbit*ri,(rqcx->i_period*rp+ri-rp));
            bpp = (int)div_s64((int64)vcb2->i_budget*BPP_FAC,vcb2->i_pixels);
            qsc = xi/bpp;
            pqp = _MAX(qs2qp(qsc),vcb2->i_levelq-vcb2->i_deltaq);
            vcb2->i_ipbias = 0;
            rqct->i_enc_qp = pqp;
            break;
        }
        pqp = QP_LOWER+(vcb2->i_imbase-vcb2->i_margin)/vcb2->i_degree;
        dqp = qdelta((int)div_s64((int64)(vcb2->i_imbits-vcb2->i_imbase)*DQBIAS_FAC,vcb2->i_radius));
        vcb2->i_levelq = pqp;
        vcb2->i_budget = vcb2->i_frmbit-vcb2->i_ipbias;
        vcb2->i_budget = _MAX(vcb2->i_budget,vcb2->i_frmbit/16);
        rqct->i_enc_qp = _MAX(_MIN(vcb2->i_upperq,pqp+dqp),vcb2->i_lowerq);
        break;
    }
    return vrqc_enc_conf(rqct, mjob);
}

#define BPS_DUR (32)

static int vcb2_enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcb2* vcb2 = (vhe_vcb2*)rqcx;
    int bps, bpp, cpx, dur, inc, dec, bcn = mjob->i_bits;

    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;

    bpp = (int)div_s64((int64)bcn*BPP_FAC,vcb2->i_pixels);
    cpx = bpp*qp2qs(rqct->i_enc_qp);
    dur = rqcx->i_period;

    if (IS_PPIC(rqct->i_pictyp))
    {
        acc_push(&vcb2->acc_bitcnt, bcn);
        acc_push(&vcb2->acc_rqprod, cpx);
        iir_push(&vcb2->iir_rqprod[RQCT_PICTYP_P], acc_calc(&vcb2->acc_rqprod,8)/8);
        vcb2->i_errpro = (vcb2->i_ipbias-vcb2->i_frmbit+bcn);
        vcb2->i_imbits += vcb2->i_errpro;
        vcb2->i_imbits = _MIN(vcb2->i_imbits,vcb2->i_upperb+vcb2->i_radius);
        vcb2->i_imbits = _MAX(vcb2->i_imbits,vcb2->i_lowerb);
        vcb2->i_rebits = _MAX(vcb2->i_rebits,vcb2->i_lowerb);
    }
    else
    {
        vcb2->i_intrabit = bcn;
        iir_push(&vcb2->iir_rqprod[RQCT_PICTYP_I], cpx);
        if (vcb2->i_imbits > vcb2->i_upperb)
            vcb2->i_rebits = vcb2->i_imbits;
        vcb2->i_errpro = (vcb2->i_rebits-vcb2->i_imbits);
        vcb2->i_imbits += vcb2->i_errpro;
        vcb2->i_imbits = _MIN(_MAX(vcb2->i_imbits,vcb2->i_lowerb),vcb2->i_upperb);
        vcb2->i_rebits = _MAX(vcb2->i_rebits,vcb2->i_lowerb);
        if (dur >= 2)
            vcb2->i_ipbias = (bcn - vcb2->i_frmbit) / (dur-1);
        if (dur == 0)
            acc_push(&vcb2->acc_bitcnt, bcn);
    }
    vcb2->i_rebits += (bcn - vcb2->i_frmbit);

    if (dur > 1)
        bps = acc_calc(&vcb2->acc_bitcnt, dur-1) + vcb2->i_intrabit;
    else
        bps = acc_calc(&vcb2->acc_bitcnt, dur=BPS_DUR);
    bps = (int)div_s64((int64)bps*vcb2->n_fmrate,vcb2->d_fmrate*dur);

    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bits:%8d bps:%8d",\
        rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp?'P':'I',rqct->i_enc_qp,rqct->i_bitcnt,bps);
#if 0
//  printk("%s\n",rqct->print_line);
    printk("<%s#%04d@%c:%2d/%2d> bits:%8d avbr:%8d i:%8d p:%8d r:%8d\n",\
    rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp?'P':'I',rqct->i_enc_qp,vcb2->i_levelq,rqct->i_bitcnt,bps,vcb2->i_imbits,vcb2->i_imbase,vcb2->i_rebits);
#endif
    bcn = vcb2->i_imbits-vcb2->i_imbase;
    inc = vcb2->i_frmbit/64;
    dec = -3*inc;
    bcn = _MIN(inc,_MAX(bcn,dec));
    vcb2->i_imbase += bcn;
    vcb2->i_imbase = _MIN(_MAX(vcb2->i_imbase,vcb2->i_lowerb),vcb2->i_upperb);

    return 0;
}
