
#include <vhe.h>

#define USER_DATA_SIZE  4096

static int   _seq_sync(mhve_ops*);
static int   _seq_conf(mhve_ops*);
static int   _seq_done(mhve_ops*);
static int   _enc_buff(mhve_ops*,mhve_vpb*);
static int   _deq_buff(mhve_ops*,mhve_vpb*);
static int   _enc_conf(mhve_ops*);
static int   _put_data(mhve_ops*,void*,int);
static int   _enc_done(mhve_ops*);
static int   _out_buff(mhve_ops*,mhve_cpb*);
static int   _set_conf(mhve_ops*,mhve_cfg*);
static int   _get_conf(mhve_ops*,mhve_cfg*);
static void* _rqct_ops(mhve_ops* mops)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    return hndl->p_rqct;
}
static void* _mvhe_job(mhve_ops* mops)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    return hndl->p_mirr;
}
static void __ops_free(mhve_ops* mops)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    rqct_ops* rqct = hndl->p_rqct;
    vhe_mirr* mirr = hndl->p_mirr;
    hevc_enc* hevc = hndl->p_hevc;
    if (rqct)
        rqct->release(rqct);
    if (mirr)
        MEM_FREE(mirr);
    if (hevc)
        hevc->release(hevc);
    MEM_FREE(hndl);
}

void*
mvheops_acquire(int id)
{
    mhve_ops* mops = NULL;
    while (NULL != (mops = MEM_ALLC(sizeof(vhe_hndl))))
    {
        vhe_hndl* hndl;
        mops->release = __ops_free;
        mops->rqct_ops = _rqct_ops;
        mops->mhve_job = _mvhe_job;
        mops->seq_sync = _seq_sync;
        mops->seq_done = _seq_done;
        mops->seq_conf = _seq_conf;
        mops->enc_buff = _enc_buff;
        mops->deq_buff = _deq_buff;
        mops->enc_conf = _enc_conf;
        mops->put_data = _put_data;
        mops->enc_done = _enc_done;
        mops->out_buff = _out_buff;
        mops->set_conf = _set_conf;
        mops->get_conf = _get_conf;
        MEM_COPY(mops->mhvename,"h2v1@0.0-00",12);
        hndl = (vhe_hndl*)mops;
        /* default setting of hevc */
        hndl->p_rqct = rqctvhe_acquire(id);
        hndl->p_hevc = h265enc_acquire();
        hndl->p_mirr = MEM_ALLC(sizeof(vhe_mirr));
        if (!hndl->p_rqct || !hndl->p_hevc || !hndl->p_mirr)
        {
            mops->release(mops);
            break;
        }
        /* default setting of vhe handler */
        hndl->i_rpbn = 2;
        return mops;
    }
    return mops;
}

static int _seq_sync(mhve_ops* mops)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    rqct_ops* rqct = hndl->p_rqct;
    hevc_enc* hevc = hndl->p_hevc;
    int err = 0;

    if (0 != (err = hevc_seq_init(hevc, hndl->i_rpbn)))
    {
        mops->seq_done(mops);
        return err;
    }
    hndl->i_seqn = 0;
    hndl->b_seqh = 1;
    rqct->seq_sync(rqct);

    return err;
}

static int _seq_done(mhve_ops* mops)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    hevc_enc* hevc = hndl->p_hevc;
    rqct_ops* rqct = hndl->p_rqct;
    rqct->seq_done(rqct);
    hevc_seq_done(hevc);
    return 0;
}

static int _put_data(mhve_ops* mops, void* data, int size)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    void* dst;
    int len = 0;
    /* copy sequence-header bits */
    if (hndl->b_seqh)
    {
        hevc_enc* hevc = hndl->p_hevc;
        dst = (char*)hndl->p_otpt + hndl->u_used;
        MEM_COPY(dst, hevc->m_seqh, hevc->i_seqh);
        hndl->u_used += hevc->i_seqh;
        len += hevc->i_seqh;
    }
    /* insert user data */
    if (size > 0)
    {
        dst = (char*)hndl->p_otpt + hndl->u_used;
        MEM_COPY(dst, data, size);
        hndl->u_used += size;
        len += (int)size;
    }
    return len;
}

static int _deq_buff(mhve_ops* mops, mhve_vpb* mvpb)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    *mvpb = hndl->m_encp;
    return 0;
}

static int _enc_buff(mhve_ops* mops, mhve_vpb* mvpb)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    rqct_ops* rqct = hndl->p_rqct;
    mhve_job* mjob = (mhve_job*)hndl->p_mirr;
    hevc_enc* hevc = hndl->p_hevc;
    rqct_buf  rqcb;
    int type, rpsi = 0;
    int err = 0, idx;

    hndl->m_encp = *mvpb;
    mvpb = &hndl->m_encp;

    if (hndl->b_seqh)
    {
        hevc_seq_conf(hevc);
        rqct->seq_sync(rqct);
    }

    rqcb.u_config = 0;
    if (mvpb->u_flags&MVHE_FLAGS_IDR)
        rqct->seq_sync(rqct);
    if (mvpb->u_flags&MVHE_FLAGS_DISPOSABLE)
        rqct->b_unrefp = 1;

    rqct->enc_buff(rqct,&rqcb);

    rqct->enc_conf(rqct, mjob);

    if (IS_IPIC(rqct->i_pictyp))
    {
        type = HEVC_ISLICE;
        mvpb->u_flags = MVHE_FLAGS_IDR;
        hndl->b_seqh = 1;
    }
    else
    {
        type = HEVC_PSLICE;
        if (mvpb->u_flags&MVHE_FLAGS_DISPOSABLE)
            rpsi = !rqct->b_unrefp;
    }
    hevc->i_picq = rqct->i_enc_qp;

    if (rqct->b_unrefp == 0)
        mvpb->u_flags &=~MVHE_FLAGS_DISPOSABLE;

    if (0 <= (idx = hevc_enc_buff(hevc, type, !rqct->b_unrefp)))
    {
        rpb_t* rpb = hndl->m_rpbs + idx;
        rpb->i_index = mvpb->i_index;
        if (!rpb->b_valid)
        {
            rpb->u_planes[VHE_PIXELY] = mvpb->planes[0].u_phys;
            rpb->u_planes[VHE_PIXELC] = mvpb->planes[1].u_phys;
            mvpb->i_index = -1;
        }
    }

    return err;
}

static int _enc_done(mhve_ops* mops)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    hevc_enc* hevc = hndl->p_hevc;
    rqct_ops* rqct = hndl->p_rqct;
    vhe_mirr* mirr = hndl->p_mirr;
    int idx;

    /* feedback to rate-controller */
    rqct->enc_done(rqct, &mirr->mjob);

    hndl->u_size = rqct->i_bitcnt/8;
    hndl->u_used+= hndl->u_size;
    hndl->u_otrm = hndl->u_used;

    if (0 <= (idx = hevc_enc_done(hevc)))
    {
        mhve_vpb* vpb = &hndl->m_encp;
        rpb_t* rpb = hndl->m_rpbs+idx;
        if (!rpb->b_valid)
            vpb->i_index = rpb->i_index;
        rpb->i_index = -1;
    }

    hndl->i_seqn++;
    hndl->b_seqh=0;

    return 0;
}

#define MVHE_FLAGS_CTRL (MVHE_FLAGS_IDR|MVHE_FLAGS_DISPOSABLE)

static int _out_buff(mhve_ops* mops, mhve_cpb* mcpb)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    int err = 0;
    if (mcpb->i_index >= 0)
    {
        hndl->u_used = hndl->u_otrm = 0;
        mcpb->planes[0].u_phys = 0;
        mcpb->planes[0].i_size = 0;
        return err;
    }
    err = hndl->u_used;
    mcpb->i_index = 0;
    mcpb->planes[0].u_phys = hndl->u_otbs;
    mcpb->planes[0].i_size = err;
    mcpb->i_stamp = hndl->m_encp.i_stamp;
    mcpb->i_flags = (MVHE_FLAGS_SOP|MVHE_FLAGS_EOP)|(hndl->m_encp.u_flags&MVHE_FLAGS_CTRL);
    return err;
}

static int _seq_conf(mhve_ops* mops)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    vhe_mirr* mirr = hndl->p_mirr;
    uint* regs = mirr->swregs;

    vhe_put_regs(regs, HWIF_ENC_IRQ_DIS,            ENCH2_IRQ_DISABLE);
    vhe_put_regs(regs, HWIF_ENC_INPUT_READ_CHUNK,   ENCH2_INPUT_READ_CHUNK);
    vhe_put_regs(regs, HWIF_ENC_AXI_READ_ID,        ENCH2_AXI_READ_ID);
    vhe_put_regs(regs, HWIF_ENC_AXI_WRITE_ID,       ENCH2_AXI_WRITE_ID);
    vhe_put_regs(regs, HWIF_ENC_BURST_DISABLE,      ENCH2_BURST_SCMD_DISABLE&(1));
    vhe_put_regs(regs, HWIF_ENC_BURST_INCR,         ENCH2_BURST_INCR_TYPE_ENABLED&(1));
    vhe_put_regs(regs, HWIF_ENC_DATA_DISCARD,       ENCH2_BURST_DATA_DISCARD_ENABLED&(1));
    vhe_put_regs(regs, HWIF_ENC_CLOCK_GATING,       ENCH2_ASIC_CLOCK_GATING_ENABLED&(1));
    vhe_put_regs(regs, HWIF_ENC_AXI_DUAL_CH,        ENCH2_AXI_2CH_DISABLE);
    vhe_put_regs(regs, HWIF_ENC_STRM_SWAP,         (ENCH2_OUTPUT_SWAP_8|(ENCH2_OUTPUT_SWAP_16<<1)|(ENCH2_OUTPUT_SWAP_32<<2)|(ENCH2_OUTPUT_SWAP_64<<3)));
    vhe_put_regs(regs, HWIF_ENC_PIC_SWAP,          (ENCH2_INPUT_SWAP_8_YUV|(ENCH2_INPUT_SWAP_16_YUV<<1)|(ENCH2_INPUT_SWAP_32_YUV<<2)|(ENCH2_INPUT_SWAP_64_YUV<<3)));
    vhe_put_regs(regs, HWIF_ENC_MODE,               ENCH2_ASIC_HEVC);
    vhe_put_regs(regs, HWIF_ENC_BITS_EST_BIAS_INTRA_CU_8,       22);    // 25
    vhe_put_regs(regs, HWIF_ENC_BITS_EST_BIAS_INTRA_CU_16,      40);    // 48
    vhe_put_regs(regs, HWIF_ENC_BITS_EST_BIAS_INTRA_CU_32,      86);    // 108
    vhe_put_regs(regs, HWIF_ENC_BITS_EST_BIAS_INTRA_CU_64,      38*8);  // 48*8
    vhe_put_regs(regs, HWIF_ENC_INTER_SKIP_BIAS,                124);
    vhe_put_regs(regs, HWIF_ENC_SKIP_CHROMA_DC_THREADHOLD,      2);
    vhe_put_regs(regs, HWIF_ENC_TIMEOUT_INT,        ENCH2_TIMEOUT_INTERRUPT&(1));
    vhe_put_regs(regs, HWIF_ENC_MAX_BURST,          ENCH2_AXI40_BURST_LENGTH);
    vhe_put_regs(regs, HWIF_TIMEOUT_OVERRIDE_E,     ENCH2_ASIC_TIMEOUT_OVERRIDE_ENABLE);
    vhe_put_regs(regs, HWIF_TIMEOUT_CYCLES,         ENCH2_ASIC_TIMEOUT_CYCLES);
    vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_LEFT,    0xFF);
    vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_RIGHT,   0xFF);
    vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_TOP,     0xFF);
    vhe_put_regs(regs, HWIF_ENC_INTRA_AREA_BOTTOM,  0xFF);
    vhe_put_regs(regs, HWIF_ENC_CIR_START,          0);
    vhe_put_regs(regs, HWIF_ENC_CIR_INTERVAL,       0);

    return 0;
}

static int _enc_conf(mhve_ops* mops)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    hevc_enc* hevc = hndl->p_hevc;
    mhve_vpb* vpb = &hndl->m_encp;
    pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
    sps_t* sps = pps->p_sps;
    pic_t* ref,* rec = hevc->p_recn;
    rpb_t* rpb;
    vhe_mirr* mirr = hndl->p_mirr;
    uint* regs = mirr->swregs;

    vhe_put_regs(regs, HWIF_ENC_HW_PERFORMANCE,                     0);
    /* output stream buffer/limits */
    vhe_put_regs(regs, HWIF_ENC_OUTPUT_STRM_BASE,                   hndl->u_otbs+hndl->u_used);
    vhe_put_regs(regs, HWIF_ENC_OUTPUT_STRM_BUFFER_LIMIT,           hndl->u_otsz-hndl->u_used);
    if (hevc->i_type == HEVC_ISLICE)
        vhe_put_regs(regs, HWIF_ENC_FRAME_CODING_TYPE,              1);
    else
        vhe_put_regs(regs, HWIF_ENC_FRAME_CODING_TYPE,              0);
    vhe_put_regs(regs, HWIF_ENC_OUTPUT_STRM_MODE,                   0); // 0:stream, 1:nalu
    vhe_put_regs(regs, HWIF_ENC_SLICE_SIZE,                         hevc->i_rows);
    vhe_put_regs(regs, HWIF_ENC_STRONG_INTRA_SMOOTHING_ENABLED_FLAG,sps->b_strong_intra_smoothing_enabled);
    vhe_put_regs(regs, HWIF_ENC_CONSTRAINED_INTRA_PRED_FLAG,        pps->b_constrained_intra_pred);
    vhe_put_regs(regs, HWIF_ENC_POC,                                hevc->i_poc);
    vhe_put_regs(regs, HWIF_ENC_INPUT_Y_BASE,                       vpb->planes[0].u_phys+vpb->planes[0].i_bias);
    vhe_put_regs(regs, HWIF_ENC_INPUT_CB_BASE,                      vpb->planes[1].u_phys+vpb->planes[1].i_bias);
    rpb = &hndl->m_rpbs[hevc->p_recn->i_id];
    vhe_put_regs(regs, HWIF_ENC_RECON_Y_BASE,                       rpb->u_planes[VHE_PIXELY]);
    vhe_put_regs(regs, HWIF_ENC_RECON_CHROMA_BASE,                  rpb->u_planes[VHE_PIXELC]);
    vhe_put_regs(regs, HWIF_ENC_RECON_LUMA_4N_BASE,                 rpb->u_planes[VHE_LUMA4N]);
    vhe_put_regs(regs, HWIF_ENC_MIN_CB_SIZE,                        sps->i_log2_min_cb_size-3);
    vhe_put_regs(regs, HWIF_ENC_MAX_CB_SIZE,                        sps->i_log2_max_cb_size-3);
    vhe_put_regs(regs, HWIF_ENC_MIN_TRB_SIZE,                       sps->i_log2_min_tr_size-2);
    vhe_put_regs(regs, HWIF_ENC_MAX_TRB_SIZE,                       sps->i_log2_max_tr_size-2);
    vhe_put_regs(regs, HWIF_ENC_PIC_WIDTH,                          sps->i_pic_w_min_cbs / 8);
    vhe_put_regs(regs, HWIF_ENC_PIC_HEIGHT,                         sps->i_pic_h_min_cbs / 8);
    vhe_put_regs(regs, HWIF_ENC_PPS_DEBLOCKING_FILTER_OVERRIDE_ENABLED_FLAG,    pps->b_deblocking_filter_override_enabled);
    vhe_put_regs(regs, HWIF_ENC_SLICE_DEBLOCKING_FILTER_OVERRIDE_FLAG,          hevc->b_deblocking_override);
    vhe_put_regs(regs, HWIF_ENC_PIC_INIT_QP,                        pps->i_init_qp);
    vhe_put_regs(regs, HWIF_ENC_CHROMA_QP_OFFSET,                   pps->i_cb_qp_offset&0x1F);
    vhe_put_regs(regs, HWIF_ENC_SAO_ENABLE,                         sps->b_sao_enabled);
    vhe_put_regs(regs, HWIF_ENC_MAX_TRANS_HIERARCHY_DEPTH_INTER,    sps->i_max_tr_hierarchy_depth_inter);
    vhe_put_regs(regs, HWIF_ENC_MAX_TRANS_HIERARCHY_DEPTH_INTRA,    sps->i_max_tr_hierarchy_depth_intra);
    vhe_put_regs(regs, HWIF_ENC_CU_QP_DELTA_ENABLED,                pps->b_cu_qp_delta_enabled);
    vhe_put_regs(regs, HWIF_ENC_NUM_SHORT_TERM_REF_PIC_SETS,        sps->i_num_short_term_ref_pic_sets);
    if (rec->p_rps)
    {
        rps_t* rps = rec->p_rps;
        vhe_put_regs(regs, HWIF_ENC_RPS_ID,                         rps->i_id);
        vhe_put_regs(regs, HWIF_ENC_NUM_NEGATIVE_PICS,              rps->i_num_neg_pics);
        vhe_put_regs(regs, HWIF_ENC_NUM_POSITIVE_PICS,              rps->i_num_pos_pics);
        vhe_put_regs(regs, HWIF_ENC_DELTA_POC0,                     rps->i_dpoc[0]&0x3FF);
        vhe_put_regs(regs, HWIF_ENC_USED_BY_CURR_PIC0,              rps->b_used[0]);
        if (rps->i_num_neg_pics > 1)
        {
            vhe_put_regs(regs, HWIF_ENC_DELTA_POC1,                 rps->i_dpoc[1]&0x3FF);
            vhe_put_regs(regs, HWIF_ENC_USED_BY_CURR_PIC1,          rps->b_used[1]);
        }
    }
    vhe_put_regs(regs, HWIF_ENC_ACTIVE_L0_CNT,                      hevc->i_active_l0_count);
    if (hevc->i_active_l0_count > 0)
    {
        ref = hevc_find_rpl(hevc, 0, 0);
        rpb = &hndl->m_rpbs[ref->i_id];
        vhe_put_regs(regs, HWIF_ENC_REFPIC_RECON_L0_Y0,             rpb->u_planes[VHE_PIXELY]);
        vhe_put_regs(regs, HWIF_ENC_REFPIC_RECON_L0_CHROMA0,        rpb->u_planes[VHE_PIXELC]);
        vhe_put_regs(regs, HWIF_ENC_REFPIC_RECON_L0_4N0_BASE,       rpb->u_planes[VHE_LUMA4N]);
    }
    if (hevc->i_active_l0_count > 1)
    {
        ref = hevc_find_rpl(hevc, 0, 1);
        rpb = &hndl->m_rpbs[ref->i_id];
        vhe_put_regs(regs, HWIF_ENC_REFPIC_RECON_L0_Y1,             rpb->u_planes[VHE_PIXELY]);
        vhe_put_regs(regs, HWIF_ENC_REFPIC_RECON_L0_CHROMA1,        rpb->u_planes[VHE_PIXELC]);
        vhe_put_regs(regs, HWIF_ENC_REFPIC_RECON_L0_4N1_BASE,       rpb->u_planes[VHE_LUMA4N]);
    }
    vhe_put_regs(regs, HWIF_ENC_ACTIVE_OVERRIDE_FLAG,               hevc->b_active_override);
    vhe_put_regs(regs, HWIF_ENC_SCALING_LIST_ENABLED_FLAG,          sps->b_scaling_list_enable);
    /* Pic Qp */
    vhe_put_regs(regs, HWIF_ENC_PIC_QP,                             hevc->i_picq);
    /* loop filter parameters */
    vhe_put_regs(regs, HWIF_ENC_DEBLOCKING_FILTER_CTRL,             hevc->b_deblocking_disable);// pps->b_deblocking_filter_disabled
    vhe_put_regs(regs, HWIF_ENC_DEBLOCKING_TC_OFFSET,              (hevc->i_tc_offset  /2)&0xF);
    vhe_put_regs(regs, HWIF_ENC_DEBLOCKING_BETA_OFFSET,            (hevc->i_beta_offset/2)&0xF);
    if (hevc->i_picw < 832 && hevc->i_pich < 480)
    {
        vhe_put_regs(regs, HWIF_ENC_BITS_EST_1N_CU_PENALTY,         0);
        vhe_put_regs(regs, HWIF_ENC_BITS_EST_TU_SPLIT_PENALTY,      2);
    }
    else
    {
        vhe_put_regs(regs, HWIF_ENC_BITS_EST_1N_CU_PENALTY,         5);
        vhe_put_regs(regs, HWIF_ENC_BITS_EST_TU_SPLIT_PENALTY,      3);
    }
    vhe_put_regs(regs, HWIF_ENC_SIZE_TBL_BASE,                      hndl->u_nalb);
    vhe_put_regs(regs, HWIF_ENC_NAL_SIZE_WRITE,                     hndl->u_nalb!=0);
    switch (hndl->e_pixf)
    {
    case MHVE_PIX_NV21:
        vhe_put_regs(regs, HWIF_ENC_CHROMA_SWAP,                    1);
    case MHVE_PIX_NV12:
        vhe_put_regs(regs, HWIF_ENC_INPUT_FORMAT,                   1);
        break;
    case MHVE_PIX_YUYV:
        vhe_put_regs(regs, HWIF_ENC_INPUT_FORMAT,                   2);
    default:
        break;
    }
    vhe_put_regs(regs, HWIF_ENC_ROWLENGTH,                          hndl->m_encp.i_pitch);
    vhe_put_regs(regs, HWIF_ENC_CABAC_INIT_FLAG,                    hevc->b_cabac_init);
    // compress
    vhe_put_regs(regs, HWIF_ENC_COMPRESSEDCOEFF_BASE,               hndl->u_coef);
    rpb = &hndl->m_rpbs[hevc->p_recn->i_id];
    vhe_put_regs(regs, HWIF_ENC_RECON_LUMA_COMPRESSOR_ENABLE,       0!=(hndl->u_conf&MHVE_CFG_COMPR));
    vhe_put_regs(regs, HWIF_ENC_RECON_CHROMA_COMPRESSOR_ENABLE,     0);
    vhe_put_regs(regs, HWIF_ENC_RECON_LUMA_COMPRESS_TABLE_BASE,     rpb->u_planes[VHE_COMPRY]);
    vhe_put_regs(regs, HWIF_ENC_RECON_CHROMA_COMPRESS_TABLE_BASE,   0);
    if (hevc->i_active_l0_count > 0)
    {
        ref = hevc_find_rpl(hevc, 0, 0);
        rpb = &hndl->m_rpbs[ref->i_id];
        vhe_put_regs(regs, HWIF_ENC_L0_REF0_LUMA_COMPRESSOR_ENABLE,     0!=(hndl->u_conf&MHVE_CFG_COMPR));
        vhe_put_regs(regs, HWIF_ENC_L0_REF0_CHROMA_COMPRESSOR_ENABLE,   0);
        vhe_put_regs(regs, HWIF_ENC_L0_REF0_LUMA_COMPRESS_TABLE_BASE,   rpb->u_planes[VHE_COMPRY]);
        vhe_put_regs(regs, HWIF_ENC_L0_REF0_CHROMA_COMPRESS_TABLE_BASE, 0);
    }
    if (hevc->i_active_l0_count > 1)
    {
        ref = hevc_find_rpl(hevc, 0, 1);
        rpb = &hndl->m_rpbs[ref->i_id];
        vhe_put_regs(regs, HWIF_ENC_L0_REF1_LUMA_COMPRESSOR_ENABLE,     0!=(hndl->u_conf&MHVE_CFG_COMPR));
        vhe_put_regs(regs, HWIF_ENC_L0_REF1_CHROMA_COMPRESSOR_ENABLE,   0);
        vhe_put_regs(regs, HWIF_ENC_L0_REF1_LUMA_COMPRESS_TABLE_BASE,   rpb->u_planes[VHE_COMPRY]);
        vhe_put_regs(regs, HWIF_ENC_L0_REF1_CHROMA_COMPRESS_TABLE_BASE, 0);
    }
    vhe_put_regs(regs, HWIF_ENC_SLICE_INT,                          0); // reject slice-done interrupt.

    return 0;
}

static int _set_conf(mhve_ops* mops, mhve_cfg* mcfg)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    int err = -1;
    switch (mcfg->type)
    {
    case MHVE_CFG_RES:
        if ((unsigned)mcfg->res.e_pixf <= MHVE_PIX_YUYV)
        {
            hevc_enc* hevc = hndl->p_hevc;
            pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
            sps_t* sps = pps->p_sps;
            hndl->e_pixf = mcfg->res.e_pixf;
            hndl->i_pixw = mcfg->res.i_pixw;
            hndl->i_pixh = mcfg->res.i_pixh;
            hndl->i_rpbn = mcfg->res.i_rpbn;
            hndl->u_conf = mcfg->res.u_conf;
            hevc->i_picw = hndl->i_pixw;
            hevc->i_pich = hndl->i_pixh;
            hevc->i_cb_w = (hevc->i_picw + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
            hevc->i_cb_h = (hevc->i_pich + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
            hndl->b_seqh = 1;
            err = 0;
        }
        break;
    case MHVE_CFG_DMA:
        if ((unsigned)mcfg->dma.i_dmem < 2)
        {
            uint addr = mcfg->dma.u_phys;
            int i = mcfg->dma.i_dmem;
            rpb_t* ref = hndl->m_rpbs+i;
            ref->i_index = -1;
            ref->u_planes[VHE_PIXELY] = !mcfg->dma.i_size[0]?0:addr;
            addr += mcfg->dma.i_size[0];
            ref->u_planes[VHE_PIXELC] = !mcfg->dma.i_size[1]?0:addr;
            addr += mcfg->dma.i_size[1];
            ref->u_planes[VHE_LUMA4N] = addr;
            addr += mcfg->dma.i_size[2];
            ref->u_planes[VHE_COMPRY] = !mcfg->dma.i_size[3]?0:addr;
            ref->b_valid = !ref->u_planes[VHE_PIXELY]?0:1;
            err = 0;
        }
        else if (mcfg->dma.i_dmem == -1)
        {
            hndl->p_otpt = mcfg->dma.p_vptr;
            hndl->u_otbs = mcfg->dma.u_phys;
            hndl->u_otsz = mcfg->dma.i_size[0];
            hndl->u_otrm = hndl->u_used = 0;
            err = 0;
        }
        else if (mcfg->dma.i_dmem == -2)
        {
            uint  addr = mcfg->dma.u_phys;
            char* vptr = mcfg->dma.p_vptr;
            hndl->p_nalp = (uint*)vptr;
            hndl->u_nalb = addr;
            addr += mcfg->dma.i_size[0];
            hndl->u_coef = 0;
            if (mcfg->dma.i_size[1])
                hndl->u_coef = addr;
            err = 0;
        }
        break;
    case MHVE_CFG_HEV:
        {
            hevc_enc* hevc = hndl->p_hevc;
            pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
            sps_t* sps = pps->p_sps;
            hevc->i_profile = mcfg->hev.i_profile;
            hevc->i_level = mcfg->hev.i_level;
            sps->i_log2_max_cb_size = mcfg->hev.i_log2_max_cb_size;
            sps->i_log2_min_cb_size = mcfg->hev.i_log2_min_cb_size;
            sps->i_log2_max_tr_size = mcfg->hev.i_log2_max_tr_size;
            sps->i_log2_min_tr_size = mcfg->hev.i_log2_min_tr_size;
            sps->i_max_tr_hierarchy_depth_intra = mcfg->hev.i_tr_depth_intra;
            sps->i_max_tr_hierarchy_depth_inter = mcfg->hev.i_tr_depth_inter;
            sps->b_scaling_list_enable = mcfg->hev.b_scaling_list_enable;
            sps->b_sao_enabled = mcfg->hev.b_sao_enable;
            sps->b_strong_intra_smoothing_enabled = mcfg->hev.b_strong_intra_smoothing;
            pps->b_cu_qp_delta_enabled = mcfg->hev.b_ctu_qp_delta_enable;
            pps->b_constrained_intra_pred = mcfg->hev.b_constrained_intra_pred;
            pps->b_deblocking_filter_override_enabled = mcfg->hev.b_deblocking_override_enable;
            pps->i_cb_qp_offset = mcfg->hev.i_cqp_offset;
            pps->i_cr_qp_offset = mcfg->hev.i_cqp_offset;
            pps->b_deblocking_filter_disabled =
            hevc->b_deblocking_disable = mcfg->hev.b_deblocking_disable;
            if (hevc->b_deblocking_disable)
                hevc->i_tc_offset = hevc->i_beta_offset = pps->i_tc_offset = pps->i_beta_offset = 0;
            else
            {
                hevc->i_tc_offset = pps->i_tc_offset = mcfg->hev.i_tc_offset_div2*2;
                hevc->i_beta_offset = pps->i_beta_offset = mcfg->hev.i_beta_offset_div2*2;
            }
            hndl->b_seqh = 1;
            err = 0;
        }
        break;
    case MHVE_CFG_VUI:
        {
            hevc_enc* hevc = hndl->p_hevc;
            pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
            sps_t* sps = pps->p_sps;
            sps->b_vui_param_pres = 0;
            if (0 != (sps->vui.b_video_full_range = (mcfg->vui.b_video_full_range!=0)))
            {
                sps->vui.b_video_signal_pres = 1;
                sps->vui.i_video_format = 5;
                sps->vui.b_colour_desc_pres = 0;
                sps->b_vui_param_pres = 1;
            }
            hndl->b_seqh = 1;
            err = 0;
        }
        break;
    case MHVE_CFG_LFT:
        {
            hevc_enc* hevc = hndl->p_hevc;
            pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
            if (!pps->b_deblocking_filter_override_enabled)
                break;
            if (!mcfg->lft.b_override)
            {
                hevc->b_deblocking_override = 0;
                hevc->b_deblocking_disable = pps->b_deblocking_filter_disabled;
                hevc->i_tc_offset = pps->i_tc_offset;
                hevc->i_beta_offset = pps->i_beta_offset;
                err = 0;
                break;
            }
            if (!mcfg->lft.b_disable && ((unsigned)(mcfg->lft.i_offsetA+6)>12 || (unsigned)(mcfg->lft.i_offsetB+6)>12))
                break;
            err = 0;
            hevc->b_deblocking_override = 1;
            if (!(hevc->b_deblocking_disable = mcfg->lft.b_disable))
            {
                hevc->i_tc_offset  = mcfg->lft.i_offsetA*2;
                hevc->i_beta_offset= mcfg->lft.i_offsetB*2;
            }
        }
        break;
    case MHVE_CFG_SPL:
        {
            hevc_enc* hevc = hndl->p_hevc;
            hevc->i_rows = mcfg->spl.i_rows;
            err = 0;
        }
        break;
    case MHVE_CFG_BAC:
        {
            hevc_enc* hevc = hndl->p_hevc;
            hevc->b_cabac_init = mcfg->bac.b_init;
            err = 0;
        }
        break;
    default:
        break;
    }
    return err;
}

static int _get_conf(mhve_ops* mops, mhve_cfg* mcfg)
{
    vhe_hndl* hndl = (vhe_hndl*)mops;
    int err = -1;
    switch (mcfg->type)
    {
    case MHVE_CFG_DMA:
        if (mcfg->dma.i_dmem == -1)
        {
            mcfg->dma.p_vptr = hndl->p_otpt;
            mcfg->dma.u_phys = hndl->u_otbs;
            mcfg->dma.i_size[0] = (int)hndl->u_otsz;
            err = 0;
        }
        break;
    case MHVE_CFG_RES:
        mcfg->res.e_pixf = hndl->e_pixf;
        mcfg->res.i_pixw = hndl->i_pixw;
        mcfg->res.i_pixh = hndl->i_pixh;
        mcfg->res.i_rpbn = hndl->i_rpbn;
        mcfg->res.u_conf = hndl->u_conf;
        err = 0;
        break;
    case MHVE_CFG_HEV:
        {
            hevc_enc* hevc = hndl->p_hevc;
            pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
            sps_t* sps = pps->p_sps;
            mcfg->hev.i_profile = hevc->i_profile;
            mcfg->hev.i_level = hevc->i_level;
            mcfg->hev.i_log2_max_cb_size = sps->i_log2_max_cb_size;
            mcfg->hev.i_log2_min_cb_size = sps->i_log2_min_cb_size;
            mcfg->hev.i_log2_max_tr_size = sps->i_log2_max_tr_size;
            mcfg->hev.i_log2_min_tr_size = sps->i_log2_min_tr_size;
            mcfg->hev.i_tr_depth_intra = sps->i_max_tr_hierarchy_depth_intra;
            mcfg->hev.i_tr_depth_inter = sps->i_max_tr_hierarchy_depth_inter;
            mcfg->hev.b_scaling_list_enable = sps->b_scaling_list_enable;
            mcfg->hev.b_sao_enable = sps->b_sao_enabled;
            mcfg->hev.b_strong_intra_smoothing = sps->b_strong_intra_smoothing_enabled;
            mcfg->hev.b_ctu_qp_delta_enable = pps->b_cu_qp_delta_enabled;
            mcfg->hev.b_constrained_intra_pred = pps->b_constrained_intra_pred;
            mcfg->hev.b_deblocking_override_enable = pps->b_deblocking_filter_override_enabled;
            mcfg->hev.i_cqp_offset = pps->i_cb_qp_offset;
            mcfg->hev.b_deblocking_disable = pps->b_deblocking_filter_disabled;
            mcfg->hev.i_tc_offset_div2 = pps->i_tc_offset/2;
            mcfg->hev.i_beta_offset_div2 = pps->i_beta_offset/2;
            err = 0;
        }
        break;
    case MHVE_CFG_VUI:
        {
            hevc_enc* hevc = hndl->p_hevc;
            pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
            sps_t* sps = pps->p_sps;
            mcfg->vui.b_video_full_range = sps->b_vui_param_pres!=0 && sps->vui.b_video_full_range!=0;
            err = 0;
        }
        break;
    case MHVE_CFG_LFT:
        {
            hevc_enc* hevc = hndl->p_hevc;
            mcfg->lft.b_override= (signed char)hevc->b_deblocking_override;
            mcfg->lft.b_disable = (signed char)hevc->b_deblocking_disable;
            mcfg->lft.i_offsetA = (signed char)hevc->i_tc_offset/2;
            mcfg->lft.i_offsetB = (signed char)hevc->i_beta_offset/2;
            err = 0;
        }
        break;
    case MHVE_CFG_SPL:
        {
            hevc_enc* hevc = hndl->p_hevc;
            mcfg->spl.i_rows = hevc->i_rows;
            mcfg->spl.i_bits = err = 0;
        }
        break;
    case MHVE_CFG_BAC:
        {
            hevc_enc* hevc = hndl->p_hevc;
            mcfg->bac.b_init = hevc->b_cabac_init!=0;
            err = 0;
        }
        break;
    default:
        break;
    }
    return err;
}
