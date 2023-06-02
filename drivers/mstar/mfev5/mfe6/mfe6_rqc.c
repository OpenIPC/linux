#include <mfe6_ops.h>

#define MRQC_NAME       "MRQC"
#define MRQC_VER_MJR    0
#define MRQC_VER_MNR    0
#define MRQC_VER_EXT    1

void* msb2_allocate(void);
char* msb2_describe(void);

static void* mrqc_allocate(void);
static char* mrqc_describe(void);

#define MRQC_FACTORIES_NR   8

static struct mrqc_factory
{
    void* (*rqc_alloc)(void);
    char* (*rqc_descr)(void);
} factories[MRQC_FACTORIES_NR] =
{
    {mrqc_allocate,mrqc_describe},
    {msb2_allocate,msb2_describe},
    {NULL,NULL},
};

int rqctmfe_insert(void* (*allc)(void), char*(*desc)(void))
{
    if (factories[0].rqc_alloc != mrqc_allocate)
        return -1;
    factories[0].rqc_alloc = allc;
    factories[0].rqc_descr = desc;
    return 0;
}
EXPORT_SYMBOL(rqctmfe_insert);

int rqctmfe_remove(void)
{
    if (factories[0].rqc_alloc == mrqc_allocate)
        return -1;
    factories[0].rqc_alloc = mrqc_allocate;
    factories[0].rqc_descr = mrqc_describe;
    return 0;
}
EXPORT_SYMBOL(rqctmfe_remove);

void* rqctmfe_acquire(int idx)
{
    if ((unsigned)idx < MRQC_FACTORIES_NR && factories[idx].rqc_alloc)
        return (factories[idx].rqc_alloc)();
    return (factories[0].rqc_alloc)();
}

char* rqctmfe_comment(int idx)
{
    if ((unsigned)idx < MRQC_FACTORIES_NR && factories[idx].rqc_alloc)
        return (factories[idx].rqc_descr)();
    return NULL;
}

/* default rate controller */

static void _mrqcfree(rqct_ops* rqct) {MEM_FREE(rqct);}
static int  _seq_sync(rqct_ops* rqct);
static int  _seq_done(rqct_ops* rqct);
static int  _seq_conf(rqct_ops* rqct);
static int  _enc_conf(rqct_ops* rqct, mhve_job* mjob);
static int  _enc_done(rqct_ops* rqct, mhve_job* mjob);

static void* mrqc_allocate(void)
{
    rqct_ops* rqct = NULL;
    mfe6_rqc* rqcx;

    if (!(rqct = MEM_ALLC(sizeof(mfe6_rqc))))
        return NULL;

    MEM_COPY(rqct->name, "mrct", 5);
    rqct->release = _mrqcfree;
    rqct->seq_sync = _seq_sync;
    rqct->seq_done = _seq_done;
    rqct->seq_conf = _seq_conf;
    rqct->set_rqcf = mrqc_set_rqcf;
    rqct->get_rqcf = mrqc_get_rqcf;
    rqct->enc_buff = mrqc_enc_buff;
    rqct->enc_conf = _enc_conf;
    rqct->enc_done = _enc_done;
    rqcx = (mfe6_rqc*)rqct;
    rqcx->attr.i_method = RQCT_MODE_CQP;
    rqcx->attr.i_pict_w = 0;
    rqcx->attr.i_pict_h = 0;
    rqcx->attr.n_fmrate =30;
    rqcx->attr.d_fmrate = 1;
    rqcx->attr.i_iperiod= 0;
    rqcx->attr.i_ltrperiod= 0;
    rqcx->attr.i_leadqp =-1;
    rqcx->attr.i_deltaq = QP_IFRAME_DELTA;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.i_iupperq = QP_MAX;
    rqcx->attr.i_ilowerq = QP_MIN;
    rqcx->attr.i_pupperq = QP_MAX;
    rqcx->attr.i_plowerq = QP_MIN;
    /* initiate penalties */
    rqcx->attr.b_i16pln = 1;
    rqcx->attr.i_peni4x = 0;
    rqcx->attr.i_peni16 = 0;
    rqcx->attr.i_penint = 0;
    rqcx->attr.i_penYpl = 0;
    rqcx->attr.i_penCpl = 0;
    rqcx->i_config = 0;
    rqcx->i_ipcount = 0;
    rqcx->i_iperiod = 0;
    rqcx->i_ltrpcount = 0;
    rqcx->i_ltrperiod = 0;

    return rqcx;
}

static char* mrqc_describe(void)
{
    static char line[64];
    sprintf(line, "%s@v%d.%d.%02d:fixed qp only.",MRQC_NAME,MRQC_VER_MJR,MRQC_VER_MNR,MRQC_VER_EXT);
    return line;
}

static int  _seq_conf(rqct_ops* rqct)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    rqcx->i_iperiod = rqcx->attr.i_iperiod;
    rqcx->i_ltrperiod = rqcx->attr.i_ltrperiod;

    if (rqcx->attr.i_leadqp < 8 || rqcx->attr.i_leadqp > 48)
        rqcx->attr.i_leadqp = 36;
    rqcx->i_ipcount = rqcx->i_iperiod;
    rqcx->i_ltrpcount = rqcx->i_ltrperiod;
    return 0;
}

static int  _seq_sync(rqct_ops* rqct)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    /* recount frame */
    rqcx->i_ipcount = 0;
    rqcx->i_ltrpcount = 0;
    return 0;
}

static int  _seq_done(rqct_ops* rqct)
{
    return 0;
}

static int  _enc_conf(rqct_ops* rqct, mhve_job* mjob)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    mfe6_reg* regs = (mfe6_reg*)mjob;
    rqct->i_enc_qp = rqcx->attr.i_leadqp;
    if (rqct->i_pictyp == RQCT_PICTYP_I)
        rqct->i_enc_qp = rqcx->attr.i_leadqp - rqcx->attr.i_deltaq;
    mrqc_roi_draw(rqct, mjob);
#define UP_DQP  3
#define LF_DQP  3
    regs->reg00_g_mbr_en = 0;
    regs->reg26_s_mbr_pqp_dlimit = UP_DQP;
    regs->reg26_s_mbr_uqp_dlimit = LF_DQP;
    regs->reg00_g_qscale = rqct->i_enc_qp;
    regs->reg27_s_mbr_frame_qstep = 0;
    regs->reg26_s_mbr_tmb_bits = 0;
    regs->reg2a_s_mbr_qp_min = 0;
    regs->reg2a_s_mbr_qp_max = 0;
    regs->reg6e_s_mbr_qstep_min = 0;
    regs->reg6f_s_mbr_qstep_max = 0;
    /* penalties of mbtypies */
    regs->regf2_g_i16pln_en = (0!=rqcx->attr.b_i16pln);
    regs->reg87_g_intra4_penalty = rqcx->attr.i_peni4x;
    regs->reg88_g_intra16_penalty = rqcx->attr.i_peni16;
    regs->reg88_g_inter_penalty = rqcx->attr.i_penint;
    regs->reg89_g_planar_penalty_luma = rqcx->attr.i_penYpl;
    regs->reg89_g_planar_penalty_cbcr = rqcx->attr.i_penCpl;
    if (!rqcx->attr.b_logoff)
        snprintf(rqct->print_line, RQCT_LINE_SIZE-1,"<%s#%04d@%c:%2d> bitcnt:%8d", \
                 rqct->name, rqct->i_enc_nr, IS_IPIC(rqct->i_pictyp)?'I':'P' , rqct->i_enc_qp, rqct->i_bitcnt);
    return 0;
}

int mrqc_enc_buff(rqct_ops* rqct, rqct_buf* buff)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    rqcx->i_config = buff->u_config;
    if (rqcx->i_ipcount == 0)
        rqct->seq_conf(rqct);
    /* picture-type decision */
    if (rqcx->i_ipcount == rqcx->i_iperiod)
    {
        /* I-Frame */
        rqcx->i_ipcount =
        rqcx->i_ltrpcount =
        rqcx->i_refcnt = 1;
        rqct->i_pictyp = RQCT_PICTYP_I;
        rqct->b_unrefp = 0;
    }
    else
    {
        rqcx->i_ipcount++;
        if (rqcx->i_ltrperiod != 0 && rqcx->i_ltrpcount >= rqcx->i_ltrperiod)
        {
//            printk("%s-LTR period(%d), count(%d)\n", __func__, rqcx->i_ltrperiod, rqcx->i_ltrpcount);
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
    }
    return 0;
}
EXPORT_SYMBOL(mrqc_enc_buff);

static int _enc_done(rqct_ops* rqct, mhve_job* mjob)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    mfe6_reg* regs = (mfe6_reg*)mjob;

    regs->reg28 = regs->reg29 = 0;
    regs->regf5 = regs->regf6 = 0;
    regs->reg42 = regs->reg43 = 0;

    rqct->i_bitcnt = mjob->i_bits;
    rqct->i_enc_bs+= mjob->i_bits/8;
    rqct->i_enc_nr++;
    rqcx->i_refcnt++;
    return 0;
}

int mrqc_set_rqcf(rqct_ops* rqct, rqct_cfg* rqcf)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    int err = -1;
    switch (rqcf->type)
    {
    case RQCT_CFG_SEQ:
        /* TODO: this is workaround for same gop when new seq */
#if 1
        if (rqcx->attr.i_method == rqcf->seq.i_method &&
            rqcx->attr.i_iperiod == rqcf->seq.i_period &&
            rqcx->attr.i_btrate == rqcf->seq.i_btrate &&
            rqcx->attr.i_leadqp == rqcf->seq.i_leadqp)
        {
            printk(KERN_ERR "%s - skip set RQCT_CFG_SEQ\n", __func__);
            err = 0;
            break;
        }
#endif
        if ((unsigned)rqcf->seq.i_method > RQCT_MODE_VBR)
            break;
        rqcx->attr.i_method = rqcf->seq.i_method;
        rqcx->attr.i_iperiod = rqcf->seq.i_period;
        rqcx->attr.i_btrate = rqcf->seq.i_btrate;
        rqcx->attr.i_leadqp = rqcf->seq.i_leadqp;
        rqcx->i_ipcount = 0;
        err = 0;
        break;
    case RQCT_CFG_LTR:
        rqcx->attr.i_ltrperiod = rqcf->ltr.i_period;
        rqcx->i_ltrperiod = rqcx->attr.i_ltrperiod;
//        rqcx->i_ltrpcount = 0;
        err = 0;
        break;
    case RQCT_CFG_DQP:
        rqcx->attr.i_deltaq = MAX(MIN(rqcf->dqp.i_dqp, DQP_MAX), DQP_MIN);
        err = 0;
        break;
    case RQCT_CFG_QPR:
        if (rqcf->qpr.i_iupperq < rqcf->qpr.i_ilowerq)
        {
            err = -1;
            break;
        }
        if (rqcf->qpr.i_pupperq < rqcf->qpr.i_plowerq)
        {
            err = -1;
            break;
        }
        rqcx->attr.i_iupperq = MAX(MIN(rqcf->qpr.i_iupperq, QP_MAX), QP_MIN);
        rqcx->attr.i_ilowerq = MAX(MIN(rqcf->qpr.i_ilowerq, QP_MAX), QP_MIN);
        rqcx->attr.i_pupperq = MAX(MIN(rqcf->qpr.i_pupperq, QP_MAX), QP_MIN);
        rqcx->attr.i_plowerq = MAX(MIN(rqcf->qpr.i_plowerq, QP_MAX), QP_MIN);
        err = 0;
        break;
    case RQCT_CFG_LOG:
        rqcx->attr.b_logoff = !rqcf->log.b_logm;
        err = 0;
        break;
    /* private config */
    case RQCT_CFG_RES:
        rqcx->attr.i_pict_w = rqcf->res.i_picw;
        rqcx->attr.i_pict_h = rqcf->res.i_pich;
        err = 0;
        break;
    case RQCT_CFG_FPS:
        rqcx->attr.n_fmrate = rqcf->fps.n_fps;
        rqcx->attr.d_fmrate = rqcf->fps.d_fps;
        err = 0;
        break;
    case RQCT_CFG_ROI:
        err = 0;
        rqcx->attr.b_dqmstat = -1;
        if (-1 == rqcf->roi.i_roiidx)
        {
            memset(rqcx->attr.i_roidqp, 0, sizeof(rqcx->attr.i_roidqp));
            break;
        }
        if (rqcf->roi.i_roidqp == 0 ||
            (unsigned)rqcf->roi.i_posx > (unsigned)rqcx->attr.i_dqmw ||
            (unsigned)rqcf->roi.i_posy > (unsigned)rqcx->attr.i_dqmh ||
            rqcf->roi.i_recw <= 0 ||
            rqcf->roi.i_rech <= 0)
        {
            rqcx->attr.i_roidqp[rqcf->roi.i_roiidx] = 0;
            break;
        }
        if (rqcf->roi.i_recw > (rqcx->attr.i_dqmw - rqcf->roi.i_posx))
            rqcf->roi.i_recw = (rqcx->attr.i_dqmw - rqcf->roi.i_posx);
        if (rqcf->roi.i_rech > (rqcx->attr.i_dqmh - rqcf->roi.i_posy))
            rqcf->roi.i_rech = (rqcx->attr.i_dqmh - rqcf->roi.i_posy);
        rqcx->attr.i_roidqp[rqcf->roi.i_roiidx] = rqcf->roi.i_roidqp;
        rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_posx = rqcf->roi.i_posx;
        rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_posy = rqcf->roi.i_posy;
        rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_recw = rqcf->roi.i_recw;
        rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_rech = rqcf->roi.i_rech;
        break;
    case RQCT_CFG_DQM:
        rqcx->attr.u_dqmphys = rqcf->dqm.u_phys;
        rqcx->attr.p_dqmkptr = rqcf->dqm.p_kptr;
        rqcx->attr.i_dqmsize = rqcf->dqm.i_size;
        rqcx->attr.i_dqmw = rqcf->dqm.i_dqmw;
        rqcx->attr.i_dqmh = rqcf->dqm.i_dqmh;
        err = 0;
        break;
    case RQCT_CFG_PEN:
        rqcx->attr.b_i16pln = rqcf->pen.b_i16pln;
        rqcx->attr.i_peni4x = rqcf->pen.i_peni4x;
        rqcx->attr.i_peni16 = rqcf->pen.i_peni16;
        rqcx->attr.i_penint = rqcf->pen.i_penint;
        rqcx->attr.i_penYpl = rqcf->pen.i_penYpl;
        rqcx->attr.i_penCpl = rqcf->pen.i_penCpl;
        err = 0;
        break;
    default:
        break;
    }
    return err;
}
EXPORT_SYMBOL(mrqc_set_rqcf);

int mrqc_get_rqcf(rqct_ops* rqct, rqct_cfg* rqcf)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    int err = -1;
    switch (rqcf->type)
    {
    case RQCT_CFG_SEQ:
        rqcf->seq.i_method = rqcx->attr.i_method;
        rqcf->seq.i_period = rqcx->attr.i_iperiod;
        rqcf->seq.i_btrate = rqcx->attr.i_btrate;
        rqcf->seq.i_leadqp = rqcx->attr.i_leadqp;
        err = 0;
        break;
    case RQCT_CFG_LTR:
        rqcf->ltr.i_period = rqcx->attr.i_ltrperiod;
        err = 0;
        break;
    case RQCT_CFG_DQP:
        rqcf->dqp.i_dqp = rqcx->attr.i_deltaq;
        err = 0;
        break;
    case RQCT_CFG_QPR:
        rqcf->qpr.i_iupperq = rqcx->attr.i_iupperq;
        rqcf->qpr.i_ilowerq = rqcx->attr.i_ilowerq;
        rqcf->qpr.i_pupperq = rqcx->attr.i_pupperq;
        rqcf->qpr.i_plowerq = rqcx->attr.i_plowerq;
        err = 0;
        break;
    case RQCT_CFG_LOG:
        rqcf->log.b_logm = !rqcx->attr.b_logoff;
        err = 0;
        break;
    /* private config */
    case RQCT_CFG_RES:
        rqcf->res.i_picw = rqcx->attr.i_pict_w;
        rqcf->res.i_pich = rqcx->attr.i_pict_h;
        err = 0;
        break;
    case RQCT_CFG_FPS:
        rqcf->fps.n_fps = rqcx->attr.n_fmrate;
        rqcf->fps.d_fps = rqcx->attr.d_fmrate;
        err = 0;
        break;
    case RQCT_CFG_ROI:
         rqcf->roi.i_roidqp = rqcx->attr.i_roidqp[rqcf->roi.i_roiidx];
         rqcf->roi.i_posx = rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_posx;
         rqcf->roi.i_posy = rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_posy;
         rqcf->roi.i_recw = rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_recw;
         rqcf->roi.i_rech = rqcx->attr.m_roirec[rqcf->roi.i_roiidx].i_rech;
         err = 0;
        break;
     case RQCT_CFG_DQM:
        rqcf->dqm.u_phys = rqcx->attr.u_dqmphys;
        rqcf->dqm.p_kptr = rqcx->attr.p_dqmkptr;
        rqcf->dqm.i_dqmw = rqcx->attr.i_dqmw;
        rqcf->dqm.i_dqmh = rqcx->attr.i_dqmh;
        rqcf->dqm.i_size = rqcx->attr.i_dqmsize;
        err = 0;
        break;
     case RQCT_CFG_PEN:
        rqcf->pen.b_i16pln = rqcx->attr.b_i16pln;
        rqcf->pen.i_peni4x = rqcx->attr.i_peni4x;
        rqcf->pen.i_peni16 = rqcx->attr.i_peni16;
        rqcf->pen.i_penint = rqcx->attr.i_penint;
        rqcf->pen.i_penYpl = rqcx->attr.i_penYpl;
        rqcf->pen.i_penCpl = rqcx->attr.i_penCpl;
        err = 0;
        break;
    default:
        break;
    }
    return err;
}
EXPORT_SYMBOL(mrqc_get_rqcf);

static int _draw(rqct_att* attr)
{
    int i = 0, enable = 0;
    char*  tbl = (char*)attr->p_dqmkptr + 1;
    uchar* idc = (char*)attr->p_dqmkptr +16;
    int mbw = attr->i_dqmw;
    struct roirec* roi = attr->m_roirec;
    while (i < RQCT_ROI_NR)
    {
        struct roirec* rec = roi+i;
        uchar  h, l;
        int w, j, k, m, n;
        tbl[i] = attr->i_roidqp[i];
        if (tbl[i++] == 0)
            continue;
        enable++;
        n = rec->i_rech;
        j = rec->i_posy * mbw + rec->i_posx;
        k = rec->i_recw + j;
        l = (uchar)i;
        h = l << 4;
        for (m = 0; m < n; m++)
        {
            uchar* p = idc + (j>>1), q;
            q = *p;
            w = j;
            do
            switch (w&1)
            {
            case 0:
                if (!(q&0x0F))  q += l;
                break;
            case 1:
                if (!(q&0xF0))  q += h;
                *p++ = q;
                q = *p;
                break;
            }
            while (++w < k);
            *p++ = q;
            j += mbw;
            k += mbw;
        }
    }
    idc[0] &=0xF0;
    return enable;
}

int mrqc_roi_draw(rqct_ops* rqct, mhve_job* mjob)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    mfe6_reg* regs = (mfe6_reg*)mjob;

    if (0 > rqcx->attr.b_dqmstat)
    {   /* edit qmap and enable it */
        MEM_SETV((char*)rqcx->attr.p_dqmkptr +16, 0, rqcx->attr.i_dqmsize-16);
        if (_draw(&rqcx->attr))
        {
            regs->regf2_g_roi_en = 1;
            regs->regfb_g_roi_qmap_adr_lo = (ushort)(rqcx->attr.u_dqmphys>> 8);
            regs->regfc_g_roi_qmap_adr_hi = (ushort)(rqcx->attr.u_dqmphys>>24);
            rqcx->attr.b_dqmstat = 1;
        }
        else
        {
            regs->regf2_g_roi_en = 0;
            regs->regfb_g_roi_qmap_adr_lo = 0;
            regs->regfc_g_roi_qmap_adr_hi = 0;
            rqcx->attr.b_dqmstat = 0;
        }
#if 0
        /* draw qmap on console */
        int i, j, w = rqcx->attr.i_dqmw/2;
        uchar* pl = (char*)rqcx->attr.p_dqmkptr +16;
        for(i = 0; i < RQCT_ROI_NR; i++)
            printk("%3d", rqcx->attr.i_roidqp[i]);
        printk("\n");
        for (i = 0; i < rqcx->attr.i_dqmh; i++)
        {
            for (j = 0; j < rqcx->attr.i_dqmw/2; j++)
            {
                uchar qm = pl[w*i+j];
                qm = (qm>>4) + (qm<<4);
                printk("%02X",qm);
            }
            printk("\n");
        }
#endif
    }
    return (rqcx->attr.b_dqmstat);
}
EXPORT_SYMBOL(mrqc_roi_draw);
