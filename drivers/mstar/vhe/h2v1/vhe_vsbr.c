
#include <linux/math64.h>
#include <linux/string.h>
#include <vhe.h>

/************************************************************************/
/* VERSION                                                              */
/************************************************************************/

#define RC_MODULE_NAME              "VSBR"
#define RC_MODULE_VER_MAJOR         1
#define RC_MODULE_VER_MINOR         6
#define RC_MODULE_VER_EXT           4

#define VSBR_NAME       RC_MODULE_NAME
#define VSBR_VER_MJR    RC_MODULE_VER_MAJOR
#define VSBR_VER_MNR    RC_MODULE_VER_MINOR
#define VSBR_VER_EXT    RC_MODULE_VER_EXT

/************************************************************************/
/* Configuration                                                        */
/************************************************************************/
// Spec limitation is [1,51]

#define I_FRAME_DELTA_QP                        (0)
#define I_FRAME_MAX_QP                          (38)
#define P_FRAME_MAX_QP                          (44)

#define I_FRAME_MIN_QP                          (12)
#define P_FRAME_MIN_QP                          (12)

#define QP_MAX_STEP                             (1)

#define SENSOR_START_SMOOTH_FRAMES              (44)
#define SENSOR_START_SMOOTH_QP_MAX              (35)

/************************************************************************/

static unsigned short intra_penalty_tu_4[52] = {   /* Intra Penalty for TU 4x4 vs. 8x8 */
    7,    7,    8,   10,   11,   12,   14,   15,   17,   20,   22,   25,   28,
   31,   35,   40,   44,   50,   56,   63,   71,   80,   89,  100,  113,  127,
  142,  160,  179,  201,  226,  254,  285,  320,  359,  403,  452,  508,  570,
  640,  719,  807,  905, 1016, 1141, 1281, 1438, 1614, 1811, 2033, 2282, 2562,
}; // max*3=13bit
static unsigned short intra_penalty_tu_8[52] = {   /* Intra Penalty for TU 8x8 vs. 16x16 */
    7,    7,    8,   10,   11,   12,   14,   15,   17,   20,   22,   25,   28,
   31,   35,   40,   44,   50,   56,   63,   71,   80,   89,  100,  113,  127,
  142,  160,  179,  201,  226,  254,  285,  320,  359,  403,  452,  508,  570,
  640,  719,  807,  905, 1016, 1141, 1281, 1438, 1614, 1811, 2033, 2282, 2562,
}; // max*3=13bit
static unsigned short intra_penalty_tu16[52] = {   /* Intra Penalty for TU 16x16 vs. 32x32 */
    9,   11,   12,   14,   15,   17,   19,   22,   24,   28,   31,   35,   39,
   44,   49,   56,   62,   70,   79,   88,   99,  112,  125,  141,  158,  177,
  199,  224,  251,  282,  317,  355,  399,  448,  503,  564,  634,  711,  799,
  896, 1006, 1129, 1268, 1423, 1598, 1793, 2013, 2259, 2536, 2847, 3196, 3587,
}; // max*3=14bit
static unsigned short intra_penalty_tu32[52] = {   /* Intra Penalty for TU 32x32 vs. 64x64 */
    9,   11,   12,   14,   15,   17,   19,   22,   24,   28,   31,   35,   39,
   44,   49,   56,   62,   70,   79,   88,   99,  112,  125,  141,  158,  177,
  199,  224,  251,  282,  317,  355,  399,  448,  503,  564,  634,  711,  799,
  896, 1006, 1129, 1268, 1423, 1598, 1793, 2013, 2259, 2536, 2847, 3196, 3587,
}; // max*3=14bit

static unsigned short intra_penalty_mode_a[52] = { /* Intra Penalty for Mode a */
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    1,    1,
    1,    1,    1,    2,    2,    2,    2,    3,    3,    4,    4,    5,    5,
    6,    7,    8,    9,   10,   11,   13,   15,   16,   19,   21,   23,   26,
   30,   33,   38,   42,   47,   53,   60,   67,   76,   85,   95,  107,  120,
}; // max*3=9bit
static unsigned short intra_penalty_mode_b[52] = { /* Intra Penalty for Mode b */
    0,    0,    0,    0,    0,    0,    1,    1,    1,    1,    1,    1,    2,
    2,    2,    2,    3,    3,    4,    4,    5,    5,    6,    7,    8,    9,
   10,   11,   13,   14,   16,   18,   21,   23,   26,   29,   33,   37,   42,
   47,   53,   59,   66,   75,   84,   94,  106,  119,  133,  150,  168,  189,
}; // max*3=10bit
static unsigned short intra_penalty_mode_c[52] = { /* Intra Penalty for Mode c */
    1,    1,    1,    1,    1,    1,    2,    2,    2,    3,    3,    3,    4,
    4,    5,    6,    6,    7,    8,    9,   10,   12,   13,   15,   17,   19,
   21,   24,   27,   31,   34,   39,   43,   49,   55,   62,   69,   78,   87,
   98,  110,  124,  139,  156,  175,  197,  221,  248,  278,  312,  351,  394,
}; // max*3=11bit


static uint lambda_sse[2][52] = {
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   7,   9,  11,
  14,  18,  23,  29,  36,  46,  58,  73,  93, 117, 147, 186, 234,
 295, 372, 469, 591, 745, 939,1183,1491,1879,2367,2982,3758,4734},
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   1,   1,   2,   2,   3,   4,   5,   6,   8,  10,  12,  16,
  20,  25,  32,  40,  51,  64,  81, 102, 129, 162, 204, 258, 325,
 409, 516, 650, 819,1032,1300,1638,2064,2600,3276,4128,5201,6553},
};
static uint lambda_sad[2][52] = {
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   1,   1,   1,   1,   1,   1,   2,   2,   2,   3,   3,
   3,   4,   4,   5,   6,   6,   7,   8,   9,  10,  12,  13,  15,
  17,  19,  21,  24,  27,  30,  34,  38,  43,  48,  54,  61,  68},
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   3,   3,   4,
   4,   5,   5,   6,   7,   8,   9,  10,  11,  12,  14,  16,  18,
  20,  22,  25,  28,  32,  36,  40,  45,  50,  57,  64,  72,  80},
};


/************************************************************************/

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

typedef struct vhe_vsbr {
    vhe_rctl    base;
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
    int     i_est_qp;
    int     i_pre_qp;
    int     i_rqm_qp;
    /* virtual buffer */
    int     i_realbit;
    int     i_virtbit;
    pidc_t  m_pidctrl[2];
    lmod_t  m_rqmodel[2];
    stat_t  m_ipratio;
#ifdef DBG_BPS
#define MOV_SIZE    (1<<6)
#define MOV_MASK    (MOV_SIZE-1)
    int i_movidx;
    int i_movbit[MOV_SIZE];
#endif
} vhe_vsbr;


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
	{  512000, 250, 1920*1080, 42 },
	{ 1000000, 250, 1920*1080, 38 },
	{ 2000000, 250, 1920*1080, 38 },
	{ 3000000, 250, 1920*1080, 38 }
};

// P frame QP upper bound
static const qp_bound qp_p_upper[]= {
	{  256000, 250, 1920*1080, 51 },
	{  512000, 250, 1920*1080, 44 },
	{ 1000000, 250, 1920*1080, 40 },
	{ 2000000, 250, 1920*1080, 37 },
	{ 3000000, 250, 1920*1080, 36 }
};
// I frame QP lower bound
static const qp_bound qp_i_lower[]= {
	{  256000, 250, 1920*1080, 15 },
	{  512000, 250, 1920*1080, 15 },
	{ 1000000, 250, 1920*1080, 15 },
	{ 2000000, 250, 1920*1080, 15 },
	{ 3000000, 250, 1920*1080, 13 }
};

// P frame QP lower bound
static const qp_bound qp_p_lower[]= {
	{  256000, 250, 1920*1080, 15 },
	{  512000, 250, 1920*1080, 15 },
	{ 1000000, 250, 1920*1080, 15 },
	{ 2000000, 250, 1920*1080, 15 },
	{ 3000000, 250, 1920*1080, 13 }

};

#define SIZE_OF_I_QP_UPP_BOUND      sizeof( qp_i_upper) / sizeof(qp_bound);
#define SIZE_OF_P_QP_UPP_BOUND      sizeof( qp_p_upper) / sizeof(qp_bound);

#define SIZE_OF_I_QP_LOW_BOUND      sizeof( qp_i_lower) / sizeof(qp_bound);
#define SIZE_OF_P_QP_LOW_BOUND      sizeof( qp_p_lower) / sizeof(qp_bound);

#define BOUND_FP_SHIFT              16  // just for calculation

#define FRAME_TYPE_I                0   // I, P, B 3 types
#define FRAME_TYPE_P                1   // I, P, B 3 types
#define FRAME_TYPE_B                2   // I, P, B 3 types
#define MAX_FRAME_TYPE              3   // I, P, B 3 types


static int   vsbr_seq_init(rqct_ops*);
static int   vsbr_seq_done(rqct_ops*);
static int   vsbr_seq_conf(rqct_ops*);
static int   vsbr_enc_conf(rqct_ops*, mhve_job*);
static int   vsbr_enc_regs(rqct_ops*, mhve_job*);
static int   vsbr_enc_done(rqct_ops*, mhve_job*);
static void _vsbr_ops_free(rqct_ops* rqct) { MEM_FREE(rqct); }

static int  _vsbr_get_qp_upper(int frm_type, int bps, int fps_by_10, int pixels );
static int  _vsbr_get_qp_lower(int frm_type, int bps, int fps_by_10, int pixels );

void* vsbr_allocate(void)
{
    rqct_ops* rqct = NULL;
    vhe_rctl* rqcx;

    if (!(rqct = MEM_ALLC(sizeof(vhe_vsbr))))
        return rqct;

    MEM_COPY(rqct->name, "vsbr", 5);
    rqct->release = _vsbr_ops_free;
    rqct->seq_sync = vsbr_seq_init;
    rqct->seq_done = vsbr_seq_done;
    rqct->set_rqcf = vrqc_set_rqcf;
    rqct->get_rqcf = vrqc_get_rqcf;
    rqct->seq_conf = vsbr_seq_conf;
    rqct->enc_buff = vrqc_enc_buff;
    rqct->enc_conf = vsbr_enc_conf;
    rqct->enc_done = vsbr_enc_done;
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

char* vsbr_describe(void)
{
    static char line[64];
    sprintf(line,"%s@v%d.%d.%02d:enhanced vcbr",VSBR_NAME,VSBR_VER_MJR,VSBR_VER_MNR,VSBR_VER_EXT);
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

static void
vsbr_print_version(void)
{
    printk("RATECTL_VER: [%s] %d.%d-%02d\n", RC_MODULE_NAME, RC_MODULE_VER_MAJOR, RC_MODULE_VER_MINOR, RC_MODULE_VER_EXT );
}

static int vsbr_seq_done(rqct_ops* rqct)
{
    return 0;
}

static int vsbr_seq_init(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
#ifdef DBG_BPS
    vhe_vsbr* vsbr = (vhe_vsbr*)rqct;
    vsbr->i_movidx = 0;
    memset(vsbr->i_movbit, 0, sizeof(vsbr->i_movbit));
#endif
    rqcx->i_pcount = 0;
    return 0;
}

static int vsbr_seq_conf(rqct_ops* rqct)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vsbr* vsbr = (vhe_vsbr*)rqcx;

    vsbr_print_version();

    vsbr->i_method = (int)rqcx->attr.i_method;
    vsbr->i_btrate = (int)rqcx->attr.i_btrate;
    vsbr->i_leadqp = (int)rqcx->attr.i_leadqp;
    vsbr->i_deltaq = (int)rqcx->attr.i_deltaq;
    vsbr->n_fmrate = (int)rqcx->attr.n_fmrate;
    vsbr->d_fmrate = (int)rqcx->attr.d_fmrate;
    vsbr->i_pixels = (int)rqcx->attr.i_pict_w*rqcx->attr.i_pict_h;

    rqcx->i_period = (int)rqcx->attr.i_period;

    vsbr->i_bpf = div_s64((int64)vsbr->i_btrate*vsbr->d_fmrate,vsbr->n_fmrate);
    vsbr->i_bpp = div_s64((int64)vsbr->i_bpf*BPP_FRAC,vsbr->i_pixels);
    vsbr->i_keybit = vsbr->i_gopbit = 0;

    if (vsbr->i_leadqp < 0)
        vsbr->i_leadqp = vsbr->i_est_qp = rqct->i_enc_qp = init_qp(vsbr->i_bpf, vsbr->i_pixels);
    pidc_init(&vsbr->m_pidctrl[0], 4, 8, 0);
    pidc_init(&vsbr->m_pidctrl[1], 4, 8, 0);
    lmod_init(&vsbr->m_rqmodel[0], 10);
    lmod_init(&vsbr->m_rqmodel[1], 10);
    stat_init(&vsbr->m_ipratio, 10);
    stat_push(&vsbr->m_ipratio, 4*IPR_FRAC/(rqcx->i_period+3));
    vsbr->i_gopqps = vsbr->i_leadqp * rqcx->i_period;
    vsbr->i_pre_qp = vsbr->i_leadqp;
    vsbr->i_deltaq = QP_MAX_STEP;

    if (vsbr->i_method == RQCT_METHOD_CQP)
        vsbr->i_deltaq = 0;// Force I, P the same qp

    rqcx->i_pcount = rqcx->i_period;

    return 0;
}

static int
init_qp(
    int     bits,
    int     pels)
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

static int vsbr_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vsbr* vsbr = (vhe_vsbr*)rqcx;

    vsbr->i_virtbit += vsbr->i_bpf;
    /* buffer occupancy */
    if (vsbr->i_virtbit >= 0x400000)
    {
        vsbr->i_virtbit -= 0x400000;
        vsbr->i_realbit -= 0x400000;
        if (vsbr->i_realbit < -0x20000000)
            vsbr->i_realbit = -0x20000000;
    }

    if (vsbr->i_method != RQCT_METHOD_CQP)
    {
        int ratio = stat_mean(&vsbr->m_ipratio);
        int bits = (int)(((int64)vsbr->i_bpf*rqcx->i_period*ratio)>>IPR_FRAC_BITS);
        int diff = vsbr->i_virtbit - vsbr->i_realbit;
        if (IS_IPIC(rqct->i_pictyp))
        {
            vsbr->i_picbgt = bits;
        }
        else
        {
            if (rqcx->i_period == 0)
                vsbr->i_picbgt = vsbr->i_bpf;
            else
            {   /* bit allocation */
                //printk("buffer occupancy: %6d\n",diff);
                bits -= vsbr->i_bpf;
                bits /=(rqcx->i_period-1);
                if (bits < 0)
                    bits = 0;
                diff += bits * (rqcx->i_period - rqcx->i_pcount + 1);
                //printk("dbit=%6d diff=%6d\n",bits,diff);
                vsbr->i_picbgt = vsbr->i_bpf - bits + (diff / rqcx->i_period);
                vsbr->i_picbgt = MAX(vsbr->i_picbgt,vsbr->i_bpf/16);
                //printk("budg:%6d\n",vsbr->i_picbgt);
            }
        }
        if (rqcx->i_period > 0)
        {   /* decide qp to achieve budget bits
             *   use RQ model: R=m/Q+b/Q^2
             */
            int d_qp = 1;
            int i_qp = vsbr->i_leadqp;
          //int i_qp = vsbr->i_pre_qp;
            int targ = (int)div_s64((int64)vsbr->i_picbgt*BPP_FRAC,vsbr->i_pixels);
            int estm = lmod_estm(&vsbr->m_rqmodel[rqct->i_pictyp], q_step[i_qp]);
            int cost = estm - targ;
            int best = cost > 0 ? cost : -cost;
            if (estm > 0)
            {   /* linear model not trained when 'estm' == 0 */
                //printk("<%2d>estm=%8d targ=%8d best=%8d cost=%8d\n",i_qp,estm,targ,best,cost);
                d_qp = cost > 0 ? 1 : -1;
                while ((unsigned)(i_qp + d_qp) < 52)
                {
                    estm = lmod_estm(&vsbr->m_rqmodel[rqct->i_pictyp], q_step[i_qp + d_qp]);
                    cost = estm - targ;
                    cost = cost > 0 ? cost : -cost;
                    //printk("<%2d>estm=%8d targ=%8d cost=%8d\n",i_qp+d_qp,estm,targ,cost);
                    if (cost >= best)
                        break;
                    i_qp += d_qp;
                    best = cost;
                }
            }
            vsbr->i_rqm_qp = i_qp;
            vsbr->i_est_qp = MAX(vsbr->i_pre_qp-vsbr->i_deltaq,MIN(i_qp,vsbr->i_pre_qp+vsbr->i_deltaq));

            // Keep the Qp smaller in the first SENSOR_START_SMOOTH_FRAMES frames
            if( rqct->i_enc_nr < SENSOR_START_SMOOTH_FRAMES )
            {
                vsbr->i_est_qp = MIN( vsbr->i_est_qp, SENSOR_START_SMOOTH_QP_MAX);
            }

            rqct->i_enc_qp = vsbr->i_est_qp;

            // Set the Qp upper boundary.
            if (IS_IPIC(rqct->i_pictyp))
            {
                int max_qp, min_qp;
                rqct->i_enc_qp += I_FRAME_DELTA_QP;
                max_qp = _vsbr_get_qp_upper(FRAME_TYPE_I, vsbr->i_btrate, vsbr->n_fmrate*10/vsbr->d_fmrate, vsbr->i_pixels );
                min_qp = _vsbr_get_qp_lower(FRAME_TYPE_I, vsbr->i_btrate, vsbr->n_fmrate*10/vsbr->d_fmrate, vsbr->i_pixels );
                rqct->i_enc_qp = MIN( rqct->i_enc_qp, max_qp);
                rqct->i_enc_qp = MAX( rqct->i_enc_qp, min_qp);

            }
            else
            {
                int max_qp, min_qp;
                max_qp = _vsbr_get_qp_upper(FRAME_TYPE_P, vsbr->i_btrate, vsbr->n_fmrate*10/vsbr->d_fmrate, vsbr->i_pixels );
                min_qp = _vsbr_get_qp_lower(FRAME_TYPE_P, vsbr->i_btrate, vsbr->n_fmrate*10/vsbr->d_fmrate, vsbr->i_pixels );

                rqct->i_enc_qp = MIN( rqct->i_enc_qp, max_qp);
                rqct->i_enc_qp = MAX( rqct->i_enc_qp, min_qp);
            }
        }
    }
    return vsbr_enc_regs(rqct, mjob);
}

static int vsbr_pic_cal_penalties(vhe_rctl* rqcx, vhe_mirr* mirr)
{

    if (mirr)
    {
        uint* regs = mirr->swregs;
        int   type = rqcx->rqct.i_pictyp;
        int   i_qp = rqcx->rqct.i_enc_qp;

        if ( i_qp > 42 )
        {
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC4X4,               intra_penalty_tu_4[i_qp]/5);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC8X8,               intra_penalty_tu_8[i_qp]/5);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC16X16,             intra_penalty_tu16[i_qp]/5);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC32X32,             intra_penalty_tu32[i_qp]/5);
        }
        else if( i_qp > 35 )
        {
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC4X4,               intra_penalty_tu_4[i_qp]*5/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC8X8,               intra_penalty_tu_8[i_qp]*5/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC16X16,             intra_penalty_tu16[i_qp]*5/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC32X32,             intra_penalty_tu32[i_qp]*5/10);
        }
        else if( i_qp > 30 )
        {
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC4X4,               intra_penalty_tu_4[i_qp]*6/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC8X8,               intra_penalty_tu_8[i_qp]*6/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC16X16,             intra_penalty_tu16[i_qp]*6/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC32X32,             intra_penalty_tu32[i_qp]*6/10);
        }
        else if ( i_qp > 25 )
        {
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC4X4,               intra_penalty_tu_4[i_qp]*9/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC8X8,               intra_penalty_tu_8[i_qp]*9/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC16X16,             intra_penalty_tu16[i_qp]*9/10);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC32X32,             intra_penalty_tu32[i_qp]*9/10);
        }
        else
        {
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC4X4,               intra_penalty_tu_4[i_qp]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC8X8,               intra_penalty_tu_8[i_qp]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC16X16,             intra_penalty_tu16[i_qp]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_PIC32X32,             intra_penalty_tu32[i_qp]);
        }
        /* TODO: need to fix */
        vhe_put_regs(regs, HWIF_ENC_LAMDA_MOTION_SSE,           lambda_sse[type][i_qp]);
        vhe_put_regs(regs, HWIF_ENC_LAMBDA_MOTIONSAD,           lambda_sad[type][i_qp]);
        vhe_put_regs(regs, HWIF_ENC_LAMDA_SAO_LUMA,             lambda_sse[type][i_qp]);
        vhe_put_regs(regs, HWIF_ENC_LAMDA_SAO_CHROMA,           lambda_sse[type][i_qp]*3/4);
        vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_PIC1,     intra_penalty_mode_a[i_qp]);
        vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_PIC2,     intra_penalty_mode_b[i_qp]);
        vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_PIC3,     intra_penalty_mode_c[i_qp]);
        if ((unsigned)rqcx->attr.i_roidqp[0] < 16)
        {
            int r = MAX(0,i_qp-rqcx->attr.i_roidqp[0]);
            vhe_put_regs(regs, HWIF_ENC_LAMDA_MOTION_SSE_ROI1,  lambda_sse[type][r]);
            vhe_put_regs(regs, HWIF_ENC_LAMBDA_MOTIONSAD_ROI1,  lambda_sad[type][r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI14X4,  intra_penalty_tu_4[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI18X8,  intra_penalty_tu_8[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI116X16,intra_penalty_tu16[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI132X32,intra_penalty_tu32[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI11,intra_penalty_mode_a[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI12,intra_penalty_mode_b[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI13,intra_penalty_mode_c[r]);
        }
        if ((unsigned)rqcx->attr.i_roidqp[1] < 16)
        {
            int r = MAX(0,i_qp-rqcx->attr.i_roidqp[1]);
            vhe_put_regs(regs, HWIF_ENC_LAMDA_MOTION_SSE_ROI2,  lambda_sse[type][r]);
            vhe_put_regs(regs, HWIF_ENC_LAMBDA_MOTIONSAD_ROI2,  lambda_sad[type][r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI24X4,  intra_penalty_tu_4[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI28X8,  intra_penalty_tu_8[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI216X16,intra_penalty_tu16[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_PENALTY_ROI232X32,intra_penalty_tu32[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI21,intra_penalty_mode_a[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI22,intra_penalty_mode_b[r]);
            vhe_put_regs(regs, HWIF_ENC_INTRA_MPM_PENALTY_ROI23,intra_penalty_mode_c[r]);
        }
    }
    return 0;
}

static int vsbr_enc_regs(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vrqc_enc_conf(rqct, mjob);
    return vsbr_pic_cal_penalties(rqcx, (vhe_mirr*)mjob);
}

static int vsbr_enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    vhe_rctl* rqcx = (vhe_rctl*)rqct;
    vhe_vsbr* vsbr = (vhe_vsbr*)rqcx;

    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;

    if (IS_PPIC(rqct->i_pictyp))
        vsbr->i_gopbit += rqct->i_bitcnt;
    else
    {
        if (vsbr->i_gopbit > 0)
            stat_push(&vsbr->m_ipratio, (int)div_s64((int64)vsbr->i_keybit*IPR_FRAC,vsbr->i_gopbit));
        vsbr->i_gopbit = vsbr->i_keybit = rqct->i_bitcnt;
    }

    vsbr->i_realbit += rqct->i_bitcnt;
    if (vsbr->i_realbit > 0x20000000)
        vsbr->i_realbit = 0x20000000;

    if (vsbr->i_method != RQCT_METHOD_CQP)
    {
        int x, y;
        /* update pid controller */
        pidc_push(&vsbr->m_pidctrl[rqct->i_pictyp], rqct->i_bitcnt-vsbr->i_picbgt, 0);
        /* update rq-model */
        x = q_step[vsbr->i_est_qp];
      //x = q_step[rctl->i_enc_qp];
        y = (int)div_s64((int64)x*x*rqct->i_bitcnt*BPP_FRAC,vsbr->i_pixels);
        lmod_push(&vsbr->m_rqmodel[rqct->i_pictyp], x, y);
        lmod_coef(&vsbr->m_rqmodel[rqct->i_pictyp]);

        if (IS_PPIC(rqct->i_pictyp))
            vsbr->i_gopqps += vsbr->i_est_qp;
        else
        {
            if (rqcx->i_period > 0)
                vsbr->i_leadqp = (vsbr->i_gopqps+rqcx->i_period-1)/rqcx->i_period;
            vsbr->i_gopqps = vsbr->i_est_qp;
        }
#ifdef DBG_BPS
    {
    int curr = vsbr->i_movidx;
    int last = (curr-1)&MOV_MASK;
    int next = (curr+1)&MOV_MASK;
    lmod_t* lm = &vsbr->m_rqmodel[rqct->i_pictyp];
    int estm = lmod_estm(lm, x);
    int bits;
    estm = (int)div_s64((int64)rqcx->i_pixels*estm,BPP_FRAC);
    bits =-vsbr->i_movbit[curr];
    vsbr->i_movbit[curr] = vsbr->i_movbit[last] + rqct->i_bitcnt;
    bits+= vsbr->i_movbit[curr];
    vsbr->i_movidx = next;
    bits = div_s64((int64)bits*rqcx->n_fmrate,MOV_SIZE*rqcx->d_fmrate)/1024;
    printk("<%4d>(%c) qp:%2d rq:%2d bcnt:%8d bdgt:%8d kbps:%8d est:%7d (%7d)\n",\
           rqct->i_enc_nr%10000,rqct->i_pictyp?'P':'I',rqct->i_enc_qp,vsbr->i_rqm_qp,rqct->i_bitcnt,vsbr->i_picbgt,bits,estm,vsbr->i_realbit-vsbr->i_virtbit);
    }
#endif
    }
    vsbr->i_pre_qp = vsbr->i_est_qp;
    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d",rqct->name,rqct->i_enc_nr,IS_IPIC(rqct->i_pictyp)?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt);
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
#if 0
        m -= m/8;
        b -= b/8;
        lm->i_m = m + lm->i_m/8;
        lm->i_b = b + lm->i_b/8;
#else
        lm->i_m = m;
        lm->i_b = b;
#endif
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


static int _vsbr_get_qp_upper(int frm_type, int bps, int fps_by_10, int pixels )
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


static int _vsbr_get_qp_lower(int frm_type, int bps, int fps_by_10, int pixels )
{
	int i;
	int bpm;
	int num = 0;
	int qp, qp_min;
	const qp_bound* qp_table;

    bpm = (int)div64_s64( (int64)(bps*10)<<BOUND_FP_SHIFT , (int64)fps_by_10 * (int64)pixels);

	switch( frm_type )
	{
	case FRAME_TYPE_I:
        qp_min   = I_FRAME_MIN_QP;
		qp_table = qp_i_lower;
		num		 = SIZE_OF_I_QP_LOW_BOUND;
		break;
	case FRAME_TYPE_P:
    case FRAME_TYPE_B:
    default:
        qp_min   = P_FRAME_MIN_QP;
		qp_table = qp_p_lower;
		num		 = SIZE_OF_P_QP_LOW_BOUND;
		break;
	}

    qp = qp_min;

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
