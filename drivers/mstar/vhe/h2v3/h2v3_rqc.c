
#include <mhve_ops.h>
#include <mhve_ios.h>
#include <rqct_ops.h>

#include <h2v3_def.h>
#include <h2v3_rqc.h>
#include <h2v3_reg.h>
#include <h2v3_ios.h>

#define HRQC_NAME       "HRQC"
#define HRQC_VER_MJR    0
#define HRQC_VER_MNR    0
#define HRQC_VER_EXT    00

void* vmbr_allocate(void);
char* vmbr_describe(void);
void* h2br_allocate(void);
char* h2br_describe(void);

static void* hrqc_allocate(void);
static char* hrqc_describe(void);

#define HRQC_FACTORIES_NR   8

static struct hrqc_factory {
    void* (*rqc_alloc)(void);
    char* (*rqc_descr)(void);
}
factories[HRQC_FACTORIES_NR] =
{
    {hrqc_allocate,hrqc_describe},
    {vmbr_allocate,vmbr_describe},
    {h2br_allocate,h2br_describe},
    {NULL,NULL},
};

int rqctvhe_insert(void* (*allc)(void), char*(*desc)(void))
{
    if (factories[0].rqc_alloc != hrqc_allocate)
        return -1;
    factories[0].rqc_alloc = allc;
    factories[0].rqc_descr = desc;
    return 0;
}
EXPORT_SYMBOL(rqctvhe_insert);

int rqctvhe_remove(void)
{
    if (factories[0].rqc_alloc == hrqc_allocate)
        return -1;
    factories[0].rqc_alloc = hrqc_allocate;
    factories[0].rqc_descr = hrqc_describe;
    return 0;
}
EXPORT_SYMBOL(rqctvhe_remove);

char* rqctvhe_comment(int idx)
{
    if ((unsigned)idx < HRQC_FACTORIES_NR && factories[idx].rqc_descr)
        return (factories[idx].rqc_descr)();
    return NULL;
}

void* rqctvhe_acquire(int idx)
{
    if ((unsigned)idx < HRQC_FACTORIES_NR && factories[idx].rqc_alloc)
        return (factories[idx].rqc_alloc)();
    return (factories[0].rqc_alloc)();
}

static int  _seq_init(rqct_ops*);
static int  _seq_done(rqct_ops*);
static int  _seq_conf(rqct_ops*);
static int  _enc_done(rqct_ops*, mhve_job*);
static int  _dqm_draw(rqct_ops*, mhve_job*);
static void _hrq_free(rqct_ops* rqct) { MEM_FREE(rqct); }

static void* hrqc_allocate(void)
{
    h2v3_rqc* rqcx = MEM_ALLC(sizeof(h2v3_rqc));
    rqct_ops* rqct = NULL;

    if (!rqcx)
    {
        printk(KERN_ERR"%s() alloc fail\n", __func__);
        return rqcx;
    }

    rqct = &rqcx->rqct;
    /* RQCT default setting */
    MEM_COPY(rqct->name, "vrqc", 5);
    rqct->release =  _hrq_free;
    rqct->seq_sync = _seq_init;
    rqct->seq_done = _seq_done;
    rqct->seq_conf = _seq_conf;
    rqct->set_rqcf = hrqc_set_conf;
    rqct->get_rqcf = hrqc_get_conf;
    rqct->enc_buff = hrqc_enc_buff;
    rqct->enc_conf = hrqc_enc_conf;
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
    rqcx->attr.i_iupperq = QP_MAX;
    rqcx->attr.i_ilowerq = QP_MIN;
    rqcx->attr.i_pupperq = QP_MAX;
    rqcx->attr.i_plowerq = QP_MIN;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.b_logoff = 1;
    rqcx->attr.b_dqmstat=-1;
    rqcx->i_config = 0;
    rqcx->i_iperiod = 0;
    rqcx->i_ipcount = 0;
    rqcx->i_ltrperiod = 0;
    rqcx->i_ltrpcount = 0;

    return rqcx;
}

static char* hrqc_describe(void)
{
    static char line[64];
    sprintf(line, "%s@v%d.%d-%02d:constant-q",HRQC_NAME,HRQC_VER_MJR,HRQC_VER_MNR,HRQC_VER_EXT);
    return line;
}

static int _seq_init(rqct_ops* rqct)
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
    if (rqcx->attr.i_leadqp < 0)
        rqcx->attr.i_leadqp = QP_DEF;
    rqcx->i_ipcount =
    rqcx->i_iperiod = rqcx->attr.i_iperiod;
    rqcx->i_ltrpcount =
    rqcx->i_ltrperiod = rqcx->attr.i_ltrperiod;
    return 0;
}

static int _enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;
    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line,RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d",\
                 rqct->name,rqct->i_enc_nr,IS_IPIC(rqct->i_pictyp)?'I':'P',rqct->i_enc_qp,rqct->i_bitcnt);
    rqcx->i_refcnt++;
    return 0;
}

int hrqc_set_conf(rqct_ops* rqct, rqct_cfg* conf)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    int idx, err = -1;
    switch (conf->type)
    {
    case RQCT_CFG_SEQ:
        /* TODO: this is workaround for same gop when new seq */
#if 1
        if (rqcx->attr.i_method == conf->seq.i_method &&
            rqcx->attr.i_iperiod == conf->seq.i_period &&
            rqcx->attr.i_btrate == conf->seq.i_btrate &&
            rqcx->attr.i_leadqp == conf->seq.i_leadqp)
        {
            printk(KERN_ERR "%s - skip set RQCT_CFG_SEQ\n", __func__);
            err = 0;
            break;
        }
#endif
        if ((unsigned)conf->seq.i_method > RQCT_MODE_VBR)
            break;
        rqcx->attr.i_method = conf->seq.i_method;
        rqcx->attr.i_iperiod= conf->seq.i_period;
        rqcx->attr.i_btrate = conf->seq.i_btrate;
        rqcx->attr.i_leadqp = conf->seq.i_leadqp;
        rqcx->i_ipcount = 0;
        err = 0;
        break;
    case RQCT_CFG_LTR:
        rqcx->attr.i_ltrperiod= conf->ltr.i_period;
        rqcx->i_ltrperiod = rqcx->attr.i_ltrperiod;
//        rqcx->i_ltrpcount = 0;
        err = 0;
        break;
    case RQCT_CFG_DQP:
        rqcx->attr.i_deltaq = MAX(MIN(conf->dqp.i_dqp, DQP_MAX), DQP_MIN);
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
        err = 0;
        break;
    case RQCT_CFG_RES:
        rqcx->attr.i_pict_w = (short)conf->res.i_picw;
        rqcx->attr.i_pict_h = (short)conf->res.i_pich;
        err = 0;
        break;
    case RQCT_CFG_FPS:
        if (!conf->fps.d_fps ||
            (rqcx->attr.n_fmrate == conf->fps.n_fps && rqcx->attr.d_fmrate == conf->fps.d_fps))
        {
            printk(KERN_ERR "vhe-skip set RQCT_CFG_FPS(%d/%d)\n", rqcx->attr.n_fmrate, rqcx->attr.d_fmrate);
            err = 0;
            break;
        }
        rqcx->attr.n_fmrate = (short)conf->fps.n_fps;
        rqcx->attr.d_fmrate = (short)conf->fps.d_fps;
        err = 0;
        break;
    case RQCT_CFG_ROI:
        if ((idx = conf->roi.i_roiidx) < 0)
        {
            memset(rqcx->attr.i_roidqp, 0, sizeof(rqcx->attr.i_roidqp));
            rqcx->attr.b_dqmstat = -1;
            err = 0;
            break;
        }
#if defined(ROIMAP)
        if (idx >= RQCT_ROI_NR)
#else
        if (idx >= 2)
#endif
            break;
        if ((unsigned)(-conf->roi.i_roidqp) > 15)
            break;
        rqcx->attr.b_dqmstat = -1;
        err = 0;
        if (rqcx->attr.i_dqmw < conf->roi.i_posx || rqcx->attr.i_dqmh < conf->roi.i_posy)
        {
            rqcx->attr.i_roidqp[idx] = 0;
            break;
        }
        if (conf->roi.i_recw > (rqcx->attr.i_dqmw - conf->roi.i_posx))
            conf->roi.i_recw = (rqcx->attr.i_dqmw - conf->roi.i_posx);
        if (conf->roi.i_rech > (rqcx->attr.i_dqmh - conf->roi.i_posy))
            conf->roi.i_rech = (rqcx->attr.i_dqmh - conf->roi.i_posy);
        rqcx->attr.i_roidqp[idx] = -conf->roi.i_roidqp;
        rqcx->attr.m_roirec[idx].i_posx = conf->roi.i_posx;
        rqcx->attr.m_roirec[idx].i_posy = conf->roi.i_posy;
        rqcx->attr.m_roirec[idx].i_recw = conf->roi.i_recw;
        rqcx->attr.m_roirec[idx].i_rech = conf->roi.i_rech;
        break;
    case RQCT_CFG_DQM:
        rqcx->attr.i_dqmw = conf->dqm.i_dqmw;
        rqcx->attr.i_dqmh = conf->dqm.i_dqmh;
#if defined(ROIMAP)
        rqcx->attr.p_dqmkptr = conf->dqm.p_kptr;
        rqcx->attr.u_dqmphys = conf->dqm.u_phys;
        rqcx->attr.i_dqmsize = conf->dqm.i_size;
        rqcx->attr.i_dqmunit = conf->dqm.i_unit;
#else
        rqcx->attr.p_dqmkptr = NULL;;
        rqcx->attr.u_dqmphys = 0;
        rqcx->attr.i_dqmsize = 0;
        rqcx->attr.i_dqmunit = 0;
#endif
        err = 0;
        break;
    case RQCT_CFG_RCM:
        rqcx->p_rcmcur = conf->rcm.p_kptr;
        rqcx->u_rcmcur = conf->rcm.u_phys;
        rqcx->p_rcmpre = conf->rcm.p_kptr + conf->rcm.i_size;
        rqcx->u_rcmpre = conf->rcm.u_phys + conf->rcm.i_size;
        err = 0;
        break;
    default:
        break;
    }
    return err;
}
EXPORT_SYMBOL(hrqc_set_conf);

int hrqc_get_conf(rqct_ops* rqct, rqct_cfg* conf)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    int err = -1;
    switch (conf->type)
    {
    case RQCT_CFG_SEQ:
        conf->seq.i_method = rqcx->attr.i_method;
        conf->seq.i_period = rqcx->attr.i_iperiod;
        conf->seq.i_btrate = rqcx->attr.i_btrate;
        conf->seq.i_leadqp = rqcx->attr.i_leadqp;
        err = 0;
        break;
    case RQCT_CFG_LTR:
        conf->ltr.i_period = rqcx->attr.i_ltrperiod;
        err = 0;
        break;
    case RQCT_CFG_DQP:
        conf->dqp.i_dqp = rqcx->attr.i_deltaq;
        err = 0;
        break;
    case RQCT_CFG_QPR:
        conf->qpr.i_iupperq = rqcx->attr.i_iupperq;
        conf->qpr.i_ilowerq = rqcx->attr.i_ilowerq;
        conf->qpr.i_pupperq = rqcx->attr.i_pupperq;
        conf->qpr.i_plowerq = rqcx->attr.i_plowerq;
        err = 0;
        break;
    case RQCT_CFG_RES:
        conf->res.i_picw = (int)rqcx->attr.i_pict_w;
        conf->res.i_pich = (int)rqcx->attr.i_pict_h;
        err = 0;
        break;
    case RQCT_CFG_FPS:
        conf->fps.n_fps = (int)rqcx->attr.n_fmrate;
        conf->fps.d_fps = (int)rqcx->attr.d_fmrate;
        err = 0;
        break;
    case RQCT_CFG_ROI:
#if defined(ROIMAP)
        if ((unsigned)conf->roi.i_roiidx >= RQCT_ROI_NR)
#else
        if ((unsigned)conf->roi.i_roiidx >= 2)
#endif
            break;
        conf->roi.i_roidqp = -rqcx->attr.i_roidqp[conf->roi.i_roiidx];
        conf->roi.i_posx = rqcx->attr.m_roirec[conf->roi.i_roiidx].i_posx;
        conf->roi.i_posy = rqcx->attr.m_roirec[conf->roi.i_roiidx].i_posy;
        conf->roi.i_recw = rqcx->attr.m_roirec[conf->roi.i_roiidx].i_recw;
        conf->roi.i_rech = rqcx->attr.m_roirec[conf->roi.i_roiidx].i_rech;
        err = 0;
        break;
    case RQCT_CFG_DQM:
        conf->dqm.i_dqmw = rqcx->attr.i_dqmw;
        conf->dqm.i_dqmh = rqcx->attr.i_dqmh;
        conf->dqm.i_unit = rqcx->attr.i_dqmunit;
        conf->dqm.p_kptr = rqcx->attr.p_dqmkptr;
        conf->dqm.u_phys = rqcx->attr.u_dqmphys;
        conf->dqm.i_size = rqcx->attr.i_dqmsize;
        err = 0;
        break;
    default:
        break;
    }
    return err;
}
EXPORT_SYMBOL(hrqc_get_conf);

int hrqc_enc_buff(rqct_ops* rqct, rqct_buf* buff)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    if (rqcx->i_ipcount == 0)
        rqct->seq_conf(rqct);
    /* picture-type decision */
    if (rqcx->i_ipcount == rqcx->i_iperiod)
    {
        /* I-Frame */
        short uq = rqcx->attr.i_iupperq;
        short lq = rqcx->attr.i_ilowerq;
        rqcx->i_ipcount =
        rqcx->i_ltrpcount =
        rqcx->i_refcnt = 1;
        rqct->i_pictyp = RQCT_PICTYP_I;
        rqct->b_unrefp = 0;
        rqct->i_enc_qp = rqcx->attr.i_leadqp - rqcx->attr.i_deltaq;
        rqct->i_enc_qp = MAX(lq,MIN(uq,rqct->i_enc_qp));
    }
    else
    {
        short uq = rqcx->attr.i_pupperq;
        short lq = rqcx->attr.i_plowerq;
        rqcx->i_ipcount++;
        if (rqcx->i_ltrperiod != 0 && rqcx->i_ltrpcount >= rqcx->i_ltrperiod)
        {
            /* LTR P-frame */
            rqct->i_pictyp = RQCT_PICTYP_LTRP;
            rqcx->i_ltrpcount = 1;
        }
        else
        {
            /* P-frame */
            rqct->i_pictyp = RQCT_PICTYP_P;
            if (rqcx->i_ltrperiod)
                rqcx->i_ltrpcount++;
            if (rqct->b_unrefp)
                rqct->b_unrefp = !(1&rqcx->i_refcnt);
        }
        rqct->i_enc_qp = rqcx->attr.i_leadqp;
        rqct->i_enc_qp = MAX(lq,MIN(uq,rqct->i_enc_qp));
    }
    return 0;
}
EXPORT_SYMBOL(hrqc_enc_buff);

static short lambda_satd_intra[]=
{
  14,  16,  18,  20,  23,  25,  29,  32,  36,  40,  45,  51,  57,
  64,  72,  81,  91, 102, 114, 129, 144, 162, 182, 204, 229, 257,
 288, 324, 363, 408, 458, 514, 577, 648, 727, 816, 916,1028,1154,
1295,1454,1632,1832,2056,2308,2591,2908,3264,3664,4112,4616,5181,
};
static short lambda_sse_motion[]=
{
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   1,   1,   2,   2,   3,   4,   5,   6,   8,  10,  12,  16,
  20,  25,  32,  40,  51,  64,  81, 102, 129, 162, 204, 258, 325,
 409, 516, 650, 819,1032,1300,1638,2064,2600,3276,4128,5201,6553,
};
static short lambda_satd_inter[]=
{
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   3,   3,   4,
   4,   5,   5,   6,   7,   8,   9,  10,  11,  12,  14,  16,  18,
  20,  22,  25,  28,  32,  36,  40,  45,  50,  57,  64,  72,  80,
};

int hrqc_roi_draw(rqct_ops* rqct, mhve_job* mjob)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    h2v3_mir* mirr = (h2v3_mir*)mjob;
    uint* regs = mirr->regs;
    short lf, rt, tp, bt;
    int roiq, picq = rqct->i_enc_qp;
    /* update roi-dqp for picq changing */
    if (!rqcx->attr.i_dqmsize)
    {
        roiq = rqcx->attr.i_roidqp[0];
        H2PutRegVal(regs, HWIF_ENC_ROI1_DQP_RC, MIN(picq,roiq));
        roiq = rqcx->attr.i_roidqp[1];
        H2PutRegVal(regs, HWIF_ENC_ROI2_DQP_RC, MIN(picq,roiq));
    }
    /* update roi area */
    if (0 <= rqcx->attr.b_dqmstat)
        return 0;
    if (rqcx->attr.i_dqmsize > 0)
        return _dqm_draw(rqct, mjob);
    rqcx->attr.b_dqmstat = 0;
    rqcx->b_rqcbit &=~RQC_ROIA_ENABLE;
    lf = rt = tp = bt = -1;
    if (0 < rqcx->attr.i_roidqp[0])
    {
        lf = rqcx->attr.m_roirec[0].i_posx;
        tp = rqcx->attr.m_roirec[0].i_posy;
        rt = rqcx->attr.m_roirec[0].i_recw+lf-1;
        bt = rqcx->attr.m_roirec[0].i_rech+tp-1;
        rqcx->b_rqcbit |= RQC_ROIA_ENABLE;
    }
    H2PutRegVal(regs, HWIF_ENC_ROI1_LFT,    lf);
    H2PutRegVal(regs, HWIF_ENC_ROI1_RHT,    rt);
    H2PutRegVal(regs, HWIF_ENC_ROI1_TOP,    tp);
    H2PutRegVal(regs, HWIF_ENC_ROI1_BOT,    bt);
    lf = rt = tp = bt = -1;
    if (0 < rqcx->attr.i_roidqp[1])
    {
        lf = rqcx->attr.m_roirec[1].i_posx;
        tp = rqcx->attr.m_roirec[1].i_posy;
        rt = rqcx->attr.m_roirec[1].i_recw+lf-1;
        bt = rqcx->attr.m_roirec[1].i_rech+tp-1;
        rqcx->b_rqcbit |= RQC_ROIA_ENABLE;
    }
    H2PutRegVal(regs, HWIF_ENC_ROI2_LFT,    lf);
    H2PutRegVal(regs, HWIF_ENC_ROI2_RHT,    rt);
    H2PutRegVal(regs, HWIF_ENC_ROI2_TOP,    tp);
    H2PutRegVal(regs, HWIF_ENC_ROI2_BOT,    bt);
    return 0;
}
EXPORT_SYMBOL(hrqc_roi_draw);

int hrqc_enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    int pqp = rqct->i_enc_qp, idx;
    int qpo = MAX(15,MIN(35,pqp));
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    h2v3_mir* mirr = (h2v3_mir*)mjob;
    uint* regs = mirr->regs;
    /* set lambdas */
    H2PutRegVal(regs, HWIF_ENC_RC_CTBRC_SLICEQPOFFSET, (qpo-pqp)&0x3F);
    for (idx = -16; idx < 16; idx++)
    {
        H2PutRegVal(regs, HWIF_ENC_LAMBDA_SATD_INTRA_0+(idx&31), lambda_satd_intra[qpo-idx]);
        H2PutRegVal(regs, HWIF_ENC_LAMBDA_SSE_MOTION_0+(idx&31), lambda_sse_motion[qpo-idx]);
        H2PutRegVal(regs, HWIF_ENC_LAMBDA_SATD_INTER_0+(idx&31), lambda_satd_inter[qpo-idx]);
    }
    H2PutRegVal(regs, HWIF_ENC_LAMBDA_SAO_Y,    lambda_sse_motion[qpo]);
    H2PutRegVal(regs, HWIF_ENC_LAMBDA_SAO_C,   (lambda_sse_motion[qpo]*3)/4);
    /* draw roi */
    hrqc_roi_draw(rqct, mjob);
    /* rc bitmap */
    H2PutRegVal(regs, HWIF_ENC_BASE_CTBRC_CURR, rqcx->u_rcmcur);
    H2PutRegVal(regs, HWIF_ENC_BASE_CTBRC_PREV, rqcx->u_rcmpre);
    /* quantization parameters */
    H2PutRegVal(regs, HWIF_ENC_RCROI_ENABLE,    rqcx->b_rqcbit&(~(-RQC_ROIX_BIT)));
    H2PutRegVal(regs, HWIF_ENC_QP_MAX,          QP_MAX);
    H2PutRegVal(regs, HWIF_ENC_QP_MIN,          QP_MIN);
    H2PutRegVal(regs, HWIF_ENC_PIC_QP,          rqct->i_enc_qp);
    return 0;
}
EXPORT_SYMBOL(hrqc_enc_conf);

static int paintv(unsigned int* d, int pitch, unsigned int m, unsigned int v, int s, int e)
{
    int i, j;
    if (m)
    {
        unsigned int t;
        for (j = s/8, i = s%8; s < e; j++, i=0)
        for (       ; s < e && i < 8; s++, i++)
        {
            t =~m&d[pitch*j+i];
            d[pitch*j+i] = t|v;
        }
        return 0;
    }
    for (j = s/8, i = s%8; s < e; j++, i=0)
    for (       ; s < e && i < 8; s++, i++)
        d[pitch*j+i] = v;
    return 0;
}

static int draw(unsigned int* p, int pitch, int v, short l, short r, short t, short b)
{
    short j;
    unsigned int u = 0x11111111, w;
    unsigned int m = -1, n = -1;

    n = n>>((-r*4)&31);
    m = m<<(( l*4)&31);
    u*= v;
    w = u&m;
    j = l-(l%8);
    l = j+8;
    if (l < r)
    {
        paintv(p+j, pitch, m, w, t, b);
        j = l;
        l+= 8;
        while (l < r)
        {
            paintv(p+j, pitch, 0, u, t, b);
            j = l;
            l+= 8;
        }
        w = u;
        m =-1;
    }
    m = m&n;
    w = w&m;
    paintv(p+j, pitch, m, w, t, b);
    return 0;
}

#define ALIGNB(x,b) (((x)      )&~((b)-1))
#define ALIGNF(x,b) (((x)+(b)-1)&~((b)-1))

static int _dqm_draw(rqct_ops* rqct, mhve_job* mjob)
{
    h2v3_rqc* rqcx = (h2v3_rqc*)rqct;
    rqct_att* attr = &rqcx->attr;
    h2v3_mir* mirr = (h2v3_mir*)mjob;
    uint* regs = mirr->regs;
    int i, u = attr->i_dqmunit;
    rqcx->b_rqcbit &=~RQC_ROIM_ENABLE;
    MEM_SETV(attr->p_dqmkptr, 0, attr->i_dqmsize);
    for (i = 0; i < RQCT_ROI_NR; i++)
    {
        short l, r, t, b;
        int v = attr->i_roidqp[i];
        if (v == 0)
            continue;
        l = attr->m_roirec[i].i_posx;
        t = attr->m_roirec[i].i_posy;
        r = attr->m_roirec[i].i_recw + l;
        b = attr->m_roirec[i].i_rech + t;
        if (u > 1)
        {
            l = ALIGNB(l,u);
            t = ALIGNB(t,u);
            r = ALIGNF(r,u);
            b = ALIGNF(b,u);
        }
        draw(attr->p_dqmkptr, attr->i_dqmw, v, l, r, t, b);

        rqcx->b_rqcbit |= RQC_ROIM_ENABLE;
    }
#if 0
    // TRACE DQMAP
    {
        unsigned int* _p = attr->p_dqmkptr;
        int _i,_j;
        for (_i=0;_i<attr->i_dqmh;_i++)
        {
            if (!(_i%8))
                printk("\n");
            for (_j=0;_j<attr->i_dqmw;_j+=8)
                printk(" %08X ",_p[(_i/8)*attr->i_dqmw+(_j/8)*8+(_i%8)]);
            printk("\n");
        }
    }
#endif
    /* roim addr */
    H2PutRegVal(regs, HWIF_ENC_BASE_ROIDQ_ADDR, attr->u_dqmphys);
    rqcx->attr.b_dqmstat = 0;
    return 0;
}
