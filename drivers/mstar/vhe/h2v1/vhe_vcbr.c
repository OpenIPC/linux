
#include <linux/math64.h>
#include <linux/string.h>
#include <vhe.h>

#define VCBR_NAME       "VCBR"
#define VCBR_VER_MJR    1
#define VCBR_VER_MNR    0
#define VCBR_VER_EXT    02

#define IPR_FRAC_BITS       (16)
#define IPR_FRAC            (1<<IPR_FRAC_BITS)
#define BPP_FRAC_BITS       (13)
#define BPP_FRAC            (1<<BPP_FRAC_BITS)

typedef struct pidc_t {
    int     i_xp, i_xi, i_xd;
    int     i_kp, i_ki, i_kd;
} pidc_t;

#define LMOD_SIZE       16

typedef struct lmod_t {
    int     i_len, i_cnt, i_pos;
    int     i_x[LMOD_SIZE];
    int     i_y[LMOD_SIZE];
    int     i_m, i_b;
} lmod_t;

typedef struct stat_t {
    int     i_len, i_cnt, i_pos;
    int     i_x[LMOD_SIZE];
} stat_t;

typedef struct vhe_vcbr {
    vhe_rctl    rqcx;
    int     i_method;
    int     i_btrate;
    int     i_leadqp;
    int     i_deltaq;
    int     n_fmrate;
    int     d_fmrate;
    int     i_pixels;
    int     b_logoff;
    /* new params */
    int     i_bpf; // BITS: bits per frame.
    int     i_bpp; // Q.12: bits per pixel.
    /* gop layer */
    int     i_bgt[2];
    int     i_keybit;
    int     i_gopbit;
    int     i_gopqps;
    /* pic layser */
    int     i_picbgt;
    int     i_pre_qp;
    /* virtual buffer */
    int     i_realbit;
    int     i_virtbit;
    pidc_t  m_pidctrl[2];
    lmod_t  m_rqmodel[2];
    stat_t  m_ipratio;
} vhe_vcbr;

static int   vcbr_seq_init(rqct_ops*);
static int   vcbr_seq_done(rqct_ops*);
static int   vcbr_seq_conf(rqct_ops*);
static int   vcbr_enc_conf(rqct_ops*, mhve_job*);
static int   vcbr_enc_done(rqct_ops*, mhve_job*);
static void _vcbr_ops_free(rqct_ops* rqct) { MEM_FREE(rqct); }

void* vcbr_allocate(void)
{
    rqct_ops* rqct = NULL;
    vhe_rctl* rqcx;

    if (!(rqct = MEM_ALLC(sizeof(vhe_vcbr))))
        return rqct;

    MEM_COPY(rqct->name, VCBR_NAME, 5);
    rqct->release = _vcbr_ops_free;
    rqct->seq_sync = vcbr_seq_init;
    rqct->seq_done = vcbr_seq_done;
    rqct->set_rqcf = vrqc_set_rqcf;
    rqct->get_rqcf = vrqc_get_rqcf;
    rqct->seq_conf = vcbr_seq_conf;
    rqct->enc_buff = vrqc_enc_buff;
    rqct->enc_conf = vcbr_enc_conf;
    rqct->enc_done = vcbr_enc_done;
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
    rqcx->attr.b_logoff = 1;
    rqcx->attr.b_dqmstat=-1;
    rqcx->i_config = 0;
    rqcx->i_period = 0;
    rqcx->i_pcount = 0;

    return rqct;
}

char* vcbr_describe(void)
{
    static char line[64];
    sprintf(line,"%s@v%d.%d.%02d:vendor's sample rate-controller",VCBR_NAME,VCBR_VER_MJR,VCBR_VER_MNR,VCBR_VER_EXT);
    return line;
}

static int init_qp(int bits, int pels);
static int compute(int a, int b, int c);
/* pid controller */
static int pidc_init(pidc_t* pid, int kp, int ki, int kd);
static int pidc_push(pidc_t* pid, int v , int n);
//static int pidc_calc(pidc_t* pid);
/* linear regression */
static const int q_step[52] = {
  3,  3,  3,  4,  4,  5,  5,  6,  7,  7,  8,  9, 10,
 11, 13, 14, 16, 18, 20, 23, 25, 28, 32, 36, 40, 45,
 51, 57, 64, 72, 80, 90,101,114,128,144,160,180,203,
228,256,288,320,360,405,456,513,577,640,720,810,896,
};
static int lmod_init(lmod_t* lm, int l);
static int lmod_push(lmod_t* lm, int x, int y);
static int lmod_coef(lmod_t* lm);
static int lmod_estm(lmod_t* lm, int x);
/* statistic */
static int stat_init(stat_t* st, int l);
static int stat_push(stat_t* st, int x);
static int stat_mean(stat_t* st);

static int vcbr_seq_done(rqct_ops* rqct)
{
    return 0;
}

static int vcbr_seq_init(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    rqcx->i_pcount = 0;
    return 0;
}

static int vcbr_seq_conf(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcbr* vcbr = (vhe_vcbr*)rqcx;

    printk("%s\n",vcbr_describe());

    vcbr->i_method = (int)rqcx->attr.i_method;
    vcbr->i_leadqp = (int)rqcx->attr.i_leadqp;
    vcbr->i_btrate = (int)rqcx->attr.i_btrate;
    vcbr->i_deltaq = (int)rqcx->attr.i_deltaq;
    vcbr->n_fmrate = (int)rqcx->attr.n_fmrate;
    vcbr->d_fmrate = (int)rqcx->attr.d_fmrate;
    vcbr->i_pixels = (int)rqcx->attr.i_pict_w*rqcx->attr.i_pict_h;

    rqcx->i_period = (int)rqcx->attr.i_period;
    rqcx->i_pcount = rqcx->i_period;

    vcbr->i_bpf = div_s64((int64)vcbr->i_btrate*vcbr->d_fmrate,vcbr->n_fmrate);
    vcbr->i_bpp = div_s64((int64)vcbr->i_bpf*BPP_FRAC,vcbr->i_pixels);
    vcbr->i_keybit = vcbr->i_gopbit = 0;

    if (vcbr->i_leadqp < 0)
        vcbr->i_leadqp = rqct->i_enc_qp = init_qp(vcbr->i_bpf, vcbr->i_pixels);
    pidc_init(&vcbr->m_pidctrl[0], 4, 8, 0);
    pidc_init(&vcbr->m_pidctrl[1], 4, 8, 0);
    lmod_init(&vcbr->m_rqmodel[0], 10);
    lmod_init(&vcbr->m_rqmodel[1], 10);
    stat_init(&vcbr->m_ipratio, 10);
    stat_push(&vcbr->m_ipratio, 4*IPR_FRAC/(rqcx->i_period+3));
    vcbr->i_gopqps = vcbr->i_leadqp * rqcx->i_period;
    vcbr->i_pre_qp = vcbr->i_leadqp;

    return 0;
}

static int init_qp(int bits, int pels)
{
    const int qp_tbl[2][36] = {
    { 16, 19, 23, 27, 32, 36, 40, 44, 51, 58, 65, 72, 84, 96,108,119,138,156,174,192,223,253,285,314,349,384,419,453,503,553,603,653,719,784,864,0x7FFFFFFF},
  //{ 27, 32, 36, 40, 44, 51, 58, 65, 72, 84, 96,108,119,138,156,174,192,223,253,285,314,349,384,419,453,503,553,603,653,719,784,864,0x7FFFFFFF},
  //{ 26, 38, 59, 96,173,305,545,0xFFFFFFF},
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

static int
vcbr_enc_conf(
    rqct_ops*   rqct,
    mhve_job*   mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcbr* vcbr = (vhe_vcbr*)rqcx;

    vcbr->i_virtbit += vcbr->i_bpf;
    /* buffer occupancy */
    if (vcbr->i_virtbit >= 0x400000)
    {
        vcbr->i_virtbit -= 0x400000;
        vcbr->i_realbit -= 0x400000;
        if (vcbr->i_realbit < -0x20000000)
            vcbr->i_realbit = -0x20000000;
    }

    if (vcbr->i_method != RQCT_METHOD_CQP)
    {
        int ratio = stat_mean(&vcbr->m_ipratio);
        int bits = (int)(((int64)vcbr->i_bpf*rqcx->i_period*ratio)>>IPR_FRAC_BITS);
        int diff = vcbr->i_virtbit - vcbr->i_realbit;
        if (IS_IPIC(rqct->i_pictyp))
        {
            vcbr->i_picbgt = bits;
        }
        else
        {
            if (rqcx->i_period == 0)
                vcbr->i_picbgt = vcbr->i_bpf;
            else
            {   /* bit allocation */
                //printk("buffer occupancy: %6d\n",diff);
                bits -= vcbr->i_bpf;
                bits /=(rqcx->i_period-1);
                if (bits < 0)
                    bits = 0;
                diff += bits * (rqcx->i_period - rqcx->i_pcount + 1);
                //printk("dbit=%6d diff=%6d\n",bits,diff);
                vcbr->i_picbgt = vcbr->i_bpf - bits + (diff / rqcx->i_period);
                vcbr->i_picbgt = MAX(vcbr->i_picbgt,vcbr->i_bpf/16);
                //printk("budg:%6d\n",vcbr->i_picbgt);
            }
        }
        if (rqcx->i_period > 0)
        {   /* decide qp to achieve budget bits
             *   use RQ model: R=m/Q+b/Q^2
             */
            int d_qp = 1;
            int i_qp = vcbr->i_leadqp;
            int targ = (int)div_s64((int64)vcbr->i_picbgt*BPP_FRAC,vcbr->i_pixels);
            int estm = lmod_estm(&vcbr->m_rqmodel[rqct->i_pictyp], q_step[i_qp]);
            int cost = estm - targ;
            int best = cost > 0 ? cost : -cost;
            if (estm > 0)
            {   /* linear model not trained when 'estm' == 0 */
                //printk("<%2d>estm=%8d targ=%8d best=%8d cost=%8d\n",i_qp,estm,targ,best,cost);
                d_qp = cost > 0 ? 1 : -1;
                while ((unsigned)(i_qp + d_qp) < 52)
                {
                    estm = lmod_estm(&vcbr->m_rqmodel[rqct->i_pictyp], q_step[i_qp + d_qp]);
                    cost = estm - targ;
                    cost = cost > 0 ? cost : -cost;
                    //printk("<%2d>estm=%8d targ=%8d cost=%8d\n",i_qp+d_qp,estm,targ,cost);
                    if (cost >= best)
                        break;
                    i_qp += d_qp;
                    best = cost;
                }
            }
            rqct->i_enc_qp = MAX(vcbr->i_pre_qp-vcbr->i_deltaq,MIN(i_qp,vcbr->i_pre_qp+vcbr->i_deltaq));
          //rqcx->i_enc_qp = i_qp;
        }
    }

    return vrqc_enc_conf(rqct, mjob);;
}

static int
vcbr_enc_done(
    rqct_ops*   rqct,
    mhve_job*   mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vcbr* vcbr = (vhe_vcbr*)rqcx;

    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;
    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d\n",rqct->name,rqct->i_enc_nr%10000,rqct->i_pictyp?'P':'I',rqct->i_enc_qp,rqct->i_bitcnt);

    if (IS_PPIC(rqct->i_pictyp))
        vcbr->i_gopbit += rqct->i_bitcnt;
    else
    {
        if (vcbr->i_gopbit > 0)
            stat_push(&vcbr->m_ipratio, (int)div_s64((int64)vcbr->i_keybit*IPR_FRAC,vcbr->i_gopbit));
        vcbr->i_gopbit = vcbr->i_keybit = rqct->i_bitcnt;
    }

    vcbr->i_realbit += rqct->i_bitcnt;
    if (vcbr->i_realbit > 0x20000000)
        vcbr->i_realbit = 0x20000000;

    if (vcbr->i_method != RQCT_METHOD_CQP)
    {
        int x, y;
        /* update pid controller */
        pidc_push(&vcbr->m_pidctrl[rqct->i_pictyp], rqct->i_bitcnt-vcbr->i_picbgt, 0);
        /* update rq-model */
        x = q_step[rqct->i_enc_qp];
        y = (int)div_s64((int64)x*x*rqct->i_bitcnt*BPP_FRAC,vcbr->i_pixels);
        lmod_push(&vcbr->m_rqmodel[rqct->i_pictyp], x, y);
        lmod_coef(&vcbr->m_rqmodel[rqct->i_pictyp]);
        if (IS_PPIC(rqct->i_pictyp))
            vcbr->i_gopqps += rqct->i_enc_qp;
        else
        {
            if (rqcx->i_period > 0)
                vcbr->i_leadqp = (vcbr->i_gopqps+rqcx->i_period-1)/rqcx->i_period;
            vcbr->i_gopqps = rqct->i_enc_qp;
        }
    }
    vcbr->i_pre_qp = rqct->i_enc_qp;

    return 0;
}

static int
pidc_init(
    pidc_t* pid,
    int     kp,
    int     ki,
    int     kd)
{
    pid->i_kp = kp;
    pid->i_ki = ki;
    pid->i_kd = kd;
    pid->i_xp = pid->i_xi = pid->i_xd = 0;
    return 0;
}

static int
pidc_push(
    pidc_t* pid,
    int     v,
    int     n)
{
    if (n == 0)
    {
        pid->i_xd = v - pid->i_xp;
        if (v > 0 && (v + pid->i_xi) > pid->i_xi)
            pid->i_xi = v + pid->i_xi;
        if (v < 0 && (v + pid->i_xi) < pid->i_xi)
            pid->i_xi = v + pid->i_xi;
        pid->i_xp = v;
        return 0;
    }
    pid->i_xd = 0;
    pid->i_xi = pid->i_xi / n;
    pid->i_xp = 0;
    return 0;
}
/*
static int
pidc_calc(
    pidc_t* pid)
{
    long long s = (long long)pid->i_kp * pid->i_xp;
    s += (long long)pid->i_ki * pid->i_xi;
    s += (long long)pid->i_kd * pid->i_xd;
    return div_s64(s,100);
}
*/
static int
calc_cov(
    int     n,
    int     i,
    int*    x,
    int     u,
    int*    y,
    int     v)
{
    long long s = 0;
    int m = n;
    while (m > 0)
    {
        --i;
        --m;
        s += (long long)(x[i&(LMOD_SIZE-1)] - u) * (y[i&(LMOD_SIZE-1)] - v);
    }
    return div_s64(s,n);
}

static int
calc_var(
    int     n,
    int     i,
    int*    x,
    int     u)
{
    long long s = 0;
    int m = n;
    while (m > 0)
    {
        --i;
        --m;
        s += (long long)(x[i&(LMOD_SIZE-1)] - u) * (x[i&(LMOD_SIZE-1)] - u);
    }
    return div_s64(s,n);
}

static int
calc_sx(
    int     n,
    int     i,
    int*    x)
{
    long long s = 0;
    int m = n;
    while (m > 0)
    {
        --i;
        --m;
        s += (long long)x[i&(LMOD_SIZE-1)];
    }
    return div_s64(s,n);
}

static int
calc_sy(
    int     n,
    int     i,
    int*    y)
{
    long long s = 0;
    int m = n;
    while (m > 0)
    {
        --i;
        --m;
        s += (long long)y[i&(LMOD_SIZE-1)];
    }
    return div_s64(s,n);
}

static int
lmod_init(
    lmod_t* lm,
    int     l)
{
    lm->i_m = lm->i_b = 0;
    memset(lm->i_x, 0, sizeof(int) * LMOD_SIZE);
    memset(lm->i_y, 0, sizeof(int) * LMOD_SIZE);
    lm->i_pos = lm->i_cnt = 0;
    lm->i_len = l;
    return 0;
}

static int
lmod_push(
    lmod_t* lm,
    int     x,
    int     y)
{
    lm->i_x[lm->i_pos] = x;
    lm->i_y[lm->i_pos] = y;
    lm->i_pos++;
    lm->i_pos &= (LMOD_SIZE-1);
    lm->i_cnt++;
    if (lm->i_cnt > lm->i_len)
        lm->i_cnt = lm->i_len;
    return 0;
}

static int
lmod_coef(
    lmod_t* lm)
{
    int n = lm->i_cnt;
    if (lm->i_cnt > 0)
    {
        int i = lm->i_pos;
        int sx = calc_sx(n, i, lm->i_x);
        int sy = calc_sy(n, i, lm->i_y);
        int m = calc_cov(n, i, lm->i_x, sx, lm->i_y, sy);
        int b = calc_var(n, i, lm->i_x, sx);
        if (b != 0)
            m = m / b;
        b = sy - m * sx;
#if 0
        {
            int j, m = lm->i_cnt;
            printk("[lmod]sx=%12d\n[lmod]sy=%12d\n",sx,sy);
            for (j = lm->i_pos-1; m > 0; m--, j--)
            {
                printk("%4d %12d\n",lm->i_x[j&(LMOD_SIZE-1)],lm->i_y[j&(LMOD_SIZE-1)]);
            }
            printk("[lmod]m=%12d\n[lmod]b=%12d\n",lm->i_m,lm->i_b);
        }
#endif
        if (b < 0)
        {
            m = sy / sx;
            b = 0;
        }
        if (m < 0)
            m = 0;
        lm->i_m = m;
        lm->i_b = b;
    }
    return 0;
}

static int
lmod_estm(
    lmod_t* lm,
    int     x)
{
    int t = lm->i_m + lm->i_b / x;
    return (t / x);
}

static int
stat_init(
    stat_t* st,
    int     l)
{
    memset(st->i_x, 0, sizeof(int) * LMOD_SIZE);
    st->i_pos = st->i_cnt = 0;
    st->i_len = l;
    return 0;
}

static int
stat_push(
    stat_t* st,
    int     x)
{
    st->i_x[st->i_pos++] = x;
    if (++st->i_cnt > st->i_len)
        st->i_cnt = st->i_len;
    st->i_pos &= (LMOD_SIZE-1);
    return x;
}

static int
stat_mean(
    stat_t* st)
{
    return calc_sx(st->i_cnt, st->i_pos, st->i_x);
}
