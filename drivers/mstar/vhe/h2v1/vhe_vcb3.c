
#include <linux/math64.h>
#include <linux/string.h>
#include <vhe.h>

#include <vhe_util.h>

#define VCB3_NAME       "VCB3"
#define VCB3_VER_MJR     0
#define VCB3_VER_MNR     0
#define VCB3_VER_EXT    00

#define PICTYPES         2

typedef struct vhe_vcb3 {
    vhe_rctl    rqcx;
    int     i_method;
    int     i_btrate;
    int     i_levelq;
    int     i_deltaq;
    int     n_fmrate;
    int     d_fmrate;
    int     i_pixels;
    int     i_pixbit;   // average bits per pixel for target bitrate. (unit:fixed point with 16-bit fraction)
    int     i_frmbit;   // average bits per frame for target bitrate. (unit:bits)
    int     i_gopbit;
    int     i_budget;
    int     i_radius;
    int     i_ipbias;
    int     i_errpro;
    int     i_smooth;
    int     i_bucket;
    int     i_rebits;   // bit-pos:real
    int     i_imbits;   // bit-pos:imaginary
    int     i_imbase;
    int     i_qpbase;

    iir_t   iir_rqprod[PICTYPES];
    acc_t   acc_rqprod;
    acc_t   acc_bitcnt;
    int     i_intrabit;

} vhe_vcb3;

#define LOGOFF_DEFAULT  1

static int   vcb3_seq_init(rqct_ops*);
static int   vcb3_seq_done(rqct_ops*);
static int   vcb3_seq_conf(rqct_ops*);
static int   vcb3_enc_conf(rqct_ops*, mhve_job*);
static int   vcb3_enc_done(rqct_ops*, mhve_job*);
static void _vcb3_ops_free(rqct_ops* rqct) { MEM_FREE(rqct); }

void* vcb3_allocate(void)
{
    rqct_ops* rqct = NULL;
    vhe_rctl* rqcx;

    if (!(rqct = MEM_ALLC(sizeof(vhe_vcb3))))
        return rqct;

    MEM_COPY(rqct->name, VCB3_NAME, 5);
    rqct->release = _vcb3_ops_free;
    rqct->seq_sync = vcb3_seq_init;
    rqct->seq_done = vcb3_seq_done;
    rqct->set_rqcf = vrqc_set_rqcf;
    rqct->get_rqcf = vrqc_get_rqcf;
    rqct->seq_conf = vcb3_seq_conf;
    rqct->enc_buff = vrqc_enc_buff;
    rqct->enc_conf = vcb3_enc_conf;
    rqct->enc_done = vcb3_enc_done;
    rqct->i_enc_nr = 0;
    rqct->i_enc_bs = 0;
    rqcx = (vhe_rctl*)rqct;
    rqcx->attr.i_pict_w = 0;
    rqcx->attr.i_pict_h = 0;
    rqcx->attr.i_method = RQCT_METHOD_CQP;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.i_leadqp =-1;
    rqcx->attr.i_deltaq = 1;
    rqcx->attr.n_fmrate =30;
    rqcx->attr.d_fmrate = 1;
    rqcx->attr.i_period = 0;
    rqcx->attr.b_logoff = LOGOFF_DEFAULT;
    rqcx->i_config = 0;
    rqcx->i_period = 0;
    rqcx->i_pcount = 0;

    return rqct;
}

char* vcb3_describe(void)
{
    static char line[64];
    sprintf(line,"%s@v%d.%d.%02d:r&d analysis",VCB3_NAME,VCB3_VER_MJR,VCB3_VER_MNR,VCB3_VER_EXT);
    return line;
}

static int init_qp(int bits, int pels);
static int compute(int a, int b, int c);

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
static int dqbias(int d)
{
    int s = d<0,bias;
    d = s?-d:d;
    bias = dqplot[d];
    return s?-bias:bias;
}

static int vcb3_seq_done(rqct_ops* rqct)
{
    return 0;
}

static int vcb3_seq_init(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    rqcx->i_pcount = 0;
    return 0;
}

#define X_I         (1<<24)
#define X_P         (1<<20)
#define QP_LOWER    (12)
#define QP_UPPER    (48)
#define QP_RANGE    (QP_UPPER-QP_LOWER)

static int vcb3_seq_conf(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcb3* vcb3 = (vhe_vcb3*)rqcx;
    int bits;

    vcb3->i_method = (int)rqcx->attr.i_method;
    vcb3->i_btrate = (int)rqcx->attr.i_btrate;
    vcb3->i_levelq = (int)rqcx->attr.i_leadqp;
    vcb3->i_deltaq = (int)rqcx->attr.i_deltaq;
    vcb3->n_fmrate = (int)rqcx->attr.n_fmrate;
    vcb3->d_fmrate = (int)rqcx->attr.d_fmrate;
    vcb3->i_pixels = (int)rqcx->attr.i_pict_w*rqcx->attr.i_pict_h;

    rqcx->i_period = (int)rqcx->attr.i_period;

    vcb3->i_smooth = rqcx->i_period;
    vcb3->i_frmbit = (int)div_s64((int64)vcb3->i_btrate*vcb3->d_fmrate,vcb3->n_fmrate);
    vcb3->i_pixbit = (int)div_s64((int64)vcb3->i_frmbit*BPP_FAC,vcb3->i_pixels);
    bits = vcb3->i_frmbit * rqcx->i_period;
    vcb3->i_gopbit = bits;
    bits = bits/4;
    vcb3->i_radius = bits;
    vcb3->i_bucket = vcb3->i_gopbit - bits;
    vcb3->i_rebits = vcb3->i_imbits = vcb3->i_imbase = bits;

    iir_init(&vcb3->iir_rqprod[RQCT_PICTYP_I], X_I, KAPA_ONE*3/4);
    iir_init(&vcb3->iir_rqprod[RQCT_PICTYP_P], X_P, KAPA_ONE*2/4);
    acc_init(&vcb3->acc_rqprod, X_P);
    acc_init(&vcb3->acc_bitcnt, vcb3->i_frmbit*3/4);

    if (vcb3->i_levelq < 0)
        vcb3->i_levelq = init_qp(vcb3->i_frmbit, vcb3->i_pixels);
    rqcx->i_pcount = rqcx->i_period;

    return 0;
}

static int init_qp(int bits, int pels)
{
    const int qp_tbl[2][36] = {
    { 16, 19, 23, 27, 32, 36, 40, 44, 51, 58, 65, 72, 84, 96,108,119,138,156,174,192,223,253,285,314,349,384,419,453,503,553,603,653,719,784,864,0x7FFFFFFF},
    { 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16},
    };
    const int upscale = 16000/2;
    int i;

    if (bits == 0)
        return 24;

    pels = pels >> 8;
    bits = bits >> 5;
    if (!bits)
        return 51;
    bits *= pels + 250;
    bits /= 350 + (pels * 3) / 4;
    bits = compute(bits, upscale, pels<<6);

    for (i = 0; qp_tbl[0][i] < bits; i++) ;

    return qp_tbl[1][i];
}

static int compute(int a, int b, int c)
{
    long long l = (long long)a * b;
    return div_s64(l, c);
}

static int vcb3_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcb3* vcb3 = (vhe_vcb3*)rqcx;
    int qstep, bppix, btpos, encqp, delta;
    switch (vcb3->i_method)
    {
    case RQCT_METHOD_CQP:
        rqct->i_enc_qp = vcb3->i_levelq;
        if (IS_IPIC(rqct->i_pictyp))
            rqct->i_enc_qp -= vcb3->i_deltaq;
        break;
    case RQCT_METHOD_CBR:
    case RQCT_METHOD_VBR:
    default:
        if (IS_IPIC(rqct->i_pictyp))
        {
            int Xi = iir_data(&vcb3->iir_rqprod[RQCT_PICTYP_I]);
            int Xp = acc_calc(&vcb3->acc_rqprod, rqcx->i_period) / rqcx->i_period;
            int Ri = (int)div_s64((int64)qp2qs(40+vcb3->i_deltaq)*IPR_FAC,qp2qs(40));
            int Rp = (int)div_s64((int64)Xp*IPR_FAC,Xi);
            vcb3->i_budget = (int)div_s64((int64)vcb3->i_gopbit*Ri,(rqcx->i_period*Rp+Ri-Rp));
            bppix = (int)div_s64((int64)vcb3->i_budget*BPP_FAC,vcb3->i_pixels);
            qstep = Xi/bppix;
            vcb3->i_ipbias = 0;
            rqct->i_enc_qp = qs2qp(qstep);
#if 0
            btpos = vcb3->i_imbase-vcb3->i_radius-(vcb3->i_radius/2);
            if (btpos > vcb3->i_frmbit)
                vcb3->i_levelq++;
            if (btpos <-vcb3->i_frmbit)
                vcb3->i_levelq--;
            vcb3->i_levelq = _MIN(QP_UPPER,_MAX(QP_LOWER,vcb3->i_levelq));
#endif
            break;
        }
        btpos = vcb3->i_imbits-vcb3->i_imbase;
        delta = qdelta((int)div_s64((int64)btpos*DQBIAS_FAC,vcb3->i_radius));
#if 0
        btpos = vcb3->i_imbase-vcb3->i_radius-(vcb3->i_radius/2);
        if (btpos > vcb3->i_frmbit)
            vcb3->i_levelq++;
        if (btpos <-vcb3->i_frmbit)
            vcb3->i_levelq--;
        vcb3->i_levelq = _MIN(QP_UPPER,_MAX(QP_LOWER,vcb3->i_levelq));
#endif
        encqp = vcb3->i_levelq+delta;
        encqp = _MIN(encqp,QP_UPPER);
        vcb3->i_budget = vcb3->i_frmbit-vcb3->i_ipbias;
        vcb3->i_budget = _MAX(vcb3->i_budget,vcb3->i_frmbit/16);
        rqct->i_enc_qp = encqp;
        break;
    }
    return vrqc_enc_conf(rqct, mjob);
}

#define BPS_DUR (32)

static int vcb3_enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcb3* vcb3 = (vhe_vcb3*)rqcx;
    int bps, bpp, cpx, dur, bcn = mjob->i_bits;

    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;

    bpp = (int)div_s64((int64)bcn*BPP_FAC,vcb3->i_pixels);
    cpx = bpp*qp2qs(rqct->i_enc_qp);
    dur = rqcx->i_period;

    if (IS_PPIC(rqct->i_pictyp))
    {
        acc_push(&vcb3->acc_bitcnt, bcn);
        acc_push(&vcb3->acc_rqprod, cpx);
        iir_push(&vcb3->iir_rqprod[RQCT_PICTYP_P], acc_calc(&vcb3->acc_rqprod,8)/8);
        vcb3->i_errpro = (vcb3->i_ipbias-vcb3->i_frmbit+bcn);
        vcb3->i_imbits += vcb3->i_errpro;
    }
    else
    {
        vcb3->i_intrabit = bcn;
        iir_push(&vcb3->iir_rqprod[RQCT_PICTYP_I], cpx);
        vcb3->i_errpro = (vcb3->i_rebits-vcb3->i_imbits);
        vcb3->i_imbits += vcb3->i_errpro;
        if (dur > 1)
            vcb3->i_ipbias = (bcn - vcb3->i_frmbit) / (dur-1);
        if (!dur)
            acc_push(&vcb3->acc_bitcnt, bcn);
    }
    vcb3->i_rebits += (bcn - vcb3->i_frmbit);

    if (dur > 1)
        bps = acc_calc(&vcb3->acc_bitcnt, dur-1) + vcb3->i_intrabit;
    else
        bps = acc_calc(&vcb3->acc_bitcnt, dur=BPS_DUR);
    bps = (int)div_s64((int64)bps*vcb3->n_fmrate,vcb3->d_fmrate*dur);

    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bits:%8d bps:%8d",\
        rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp?'P':'I',rqct->i_enc_qp,rqct->i_bitcnt,bps);
#if 1
//  printk("%s\n",rqct->print_line);
    printk("<%s#%04d@%c:%2d/%2d> bits:%8d avbr:%8d i:%8d p:%8d r:%8d\n",\
    rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp?'P':'I',rqct->i_enc_qp,vcb3->i_levelq,rqct->i_bitcnt,bps,vcb3->i_imbits,vcb3->i_imbase,vcb3->i_rebits);
#endif
#if 1
    bcn = vcb3->i_imbits-vcb3->i_imbase;
    bpp = vcb3->i_frmbit/32;
    bcn = _MIN(bpp,_MAX(bcn,-bpp));
    vcb3->i_imbase += bcn;
    vcb3->i_imbase = _MAX(vcb3->i_imbase,vcb3->i_radius);
    vcb3->i_imbase = _MIN(vcb3->i_imbase,vcb3->i_bucket);
#endif
    return 0;
}
