#include <linux/delay.h>

#include <mhve_ops.h>
#include <mhve_ios.h>
#include <rqct_ops.h>

#include <h2v3_def.h>
#include <h265_enc.h>
#include <h2v3_rqc.h>
#include <h2v3_reg.h>
#include <h2v3_ios.h>
#include <h2v3_ops.h>

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
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    return h2v3->rqct;
}
static void* _mvhe_job(mhve_ops* mops)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    return h2v3->mirr;
}
static void __ops_free(mhve_ops* mops)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    rqct_ops* rqct = h2v3->rqct;
    h265_enc* h265 = h2v3->h265;
    h2v3_mir* mirr = h2v3->mirr;
    if (rqct)
        rqct->release(rqct);
    if (h265)
        h265->release(h265);
    if (mirr)
        MEM_FREE(mirr);
//    MEM_FREE(h2v3);
}

void* mvheops_acquire(int idx)
{
    h2v3_ops* h2v3 = MEM_ALLC(sizeof(h2v3_ops));

    if (!h2v3)
    {
        printk(KERN_ERR"%s() alloc fail\n", __func__);
        return h2v3;
    }

    /* default setting */
    do
    {
        mhve_ops* mops = &h2v3->mops;
        /* OPs prototype configuration */
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
        MEM_COPY(mops->mhvename,"h2v3@0.0-00",12);
        /* RQCT */
        h2v3->rqct = rqctvhe_acquire(idx);
        /* h265 */
        h2v3->h265 = h265enc_acquire();
        h2v3->mirr = MEM_ALLC(sizeof(h2v3_mir));
        if (!h2v3->rqct || !h2v3->h265 || !h2v3->mirr)
        {
            mops->release(mops);
            MEM_FREE(h2v3);
            h2v3 = NULL;
            break;
        }

        h2v3->i_rpbn = 2;
        return h2v3;
    }
    while (0);

    return h2v3;
}

static int _seq_sync(mhve_ops* mops)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    rqct_ops* rqct = h2v3->rqct;
    h265_enc* h265 = h2v3->h265;
    int err = 0;
    if (0 != (err = h265_seq_init(h265, h2v3->i_rpbn)))
    {
        mops->seq_done(mops);
        return err;
    }
    h2v3->i_seqn = 0;
    h2v3->b_seqh = OPS_SEQH_START;
    rqct->seq_sync(rqct);
    return err;
}

static int _seq_conf(mhve_ops* mops)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    h2v3_mir* mirr = h2v3->mirr;
    h265_enc* h265 = h2v3->h265;
    uint* regs = mirr->regs;
    H2PutRegVal(regs, HWIF_ENC_AXI_WR_ID_E,         0);
    H2PutRegVal(regs, HWIF_ENC_AXI_RD_ID_E,         0);
    H2PutRegVal(regs, HWIF_ENC_IRQ_DIS,             ENCH2_IRQ_DISABLE);
    H2PutRegVal(regs, HWIF_ENC_INPUT_READ_CHUNK,    ENCH2_INPUT_READ_CHUNK);
    H2PutRegVal(regs, HWIF_ENC_AXI_READ_ID,         ENCH2_AXI_RD_ID);
    H2PutRegVal(regs, HWIF_ENC_AXI_WRITE_ID,        ENCH2_AXI_WR_ID);
    H2PutRegVal(regs, HWIF_ENC_BURST_DISABLE,       ENCH2_BURST_SCMD_DISABLE&(1));
    H2PutRegVal(regs, HWIF_ENC_BURST_INCR,          ENCH2_BURST_INCR_TYPE_ENABLED&(1));
    H2PutRegVal(regs, HWIF_ENC_DATA_DISCARD,        ENCH2_BURST_DATA_DISCARD_ENABLED&(1));
    H2PutRegVal(regs, HWIF_ENC_CLOCK_GATING,        ENCH2_ASIC_CLOCK_GATING_ENABLED&(1));
    H2PutRegVal(regs, HWIF_ENC_AXI_DUAL_CH,         ENCH2_AXI_2CH_DISABLE);
    H2PutRegVal(regs, HWIF_ENC_STREAM_SWAP,         ENCH2_OUT_SWAP);
    H2PutRegVal(regs, HWIF_ENC_PIX_IN_SWAP,         ENCH2_PIC_SWAP_YUV);
    H2PutRegVal(regs, HWIF_ENC_ROI_DQ_SWAP,         ENCH2_DQM_SWAP);
    H2PutRegVal(regs, HWIF_ENC_CTB_RC_SWAP,         ENCH2_RCM_SWAP);
    H2PutRegVal(regs, HWIF_ENC_SCALER_SWAP,         ENCH2_SCA_SWAP);
    H2PutRegVal(regs, HWIF_ENC_MODE,                ENCH2_CODEC_HEVC);
    /* bias */
    H2PutRegVal(regs, HWIF_ENC_BITS_EST_BIAS_INTRA_CU_8,       22);    // 25
    H2PutRegVal(regs, HWIF_ENC_BITS_EST_BIAS_INTRA_CU_16,      40);    // 48
    H2PutRegVal(regs, HWIF_ENC_BITS_EST_BIAS_INTRA_CU_32,      86);    // 108
    H2PutRegVal(regs, HWIF_ENC_BITS_EST_BIAS_INTRA_CU_64,      38*8);  // 48*8
    H2PutRegVal(regs, HWIF_ENC_INTER_SKIP_BIAS,                124);
    /* penalties */
    H2PutRegVal(regs, HWIF_ENC_BITS_EST_1N_CU_PENALTY,         5);
    H2PutRegVal(regs, HWIF_ENC_BITS_EST_TU_SPLIT_PENALTY,      3);
    if (h265->i_picw <= 832 && h265->i_pich <= 480)
    {
        H2PutRegVal(regs, HWIF_ENC_BITS_EST_1N_CU_PENALTY,     0);
        H2PutRegVal(regs, HWIF_ENC_BITS_EST_TU_SPLIT_PENALTY,  2);
    }
    H2PutRegVal(regs, HWIF_ENC_TIMEOUT_INT,     ENCH2_TIMEOUT_INTERRUPT&(1));
    H2PutRegVal(regs, HWIF_ENC_MAX_BURST,       ENCH2_AXI40_BURST_LENGTH);
    H2PutRegVal(regs, HWIF_TIMEOUT_OVERRIDE_E,  ENCH2_ASIC_TIMEOUT_OVERRIDE_ENABLE);
    H2PutRegVal(regs, HWIF_TIMEOUT_CYCLES,      ENCH2_ASIC_TIMEOUT_CYCLES);
    H2PutRegVal(regs, HWIF_ENC_INTRA_AREA_LFT,  0xFF);
    H2PutRegVal(regs, HWIF_ENC_INTRA_AREA_RHT,  0xFF);
    H2PutRegVal(regs, HWIF_ENC_INTRA_AREA_TOP,  0xFF);
    H2PutRegVal(regs, HWIF_ENC_INTRA_AREA_BOT,  0xFF);
    H2PutRegVal(regs, HWIF_ENC_ROI1_LFT,        0xFF);
    H2PutRegVal(regs, HWIF_ENC_ROI1_RHT,        0xFF);
    H2PutRegVal(regs, HWIF_ENC_ROI1_TOP,        0xFF);
    H2PutRegVal(regs, HWIF_ENC_ROI1_BOT,        0xFF);
    H2PutRegVal(regs, HWIF_ENC_ROI2_LFT,        0xFF);
    H2PutRegVal(regs, HWIF_ENC_ROI2_RHT,        0xFF);
    H2PutRegVal(regs, HWIF_ENC_ROI2_TOP,        0xFF);
    H2PutRegVal(regs, HWIF_ENC_ROI2_BOT,        0xFF);
    H2PutRegVal(regs, HWIF_ENC_CIR_START,       0);
    H2PutRegVal(regs, HWIF_ENC_CIR_PITCH,       0);
#define INTRA_SIZE_FACTOR0    506
#define INTRA_SIZE_FACTOR1    506
#define INTRA_SIZE_FACTOR2    709
#define INTRA_SIZE_FACTOR3    709
#define INTRA_MODE_FACTOR0     24
#define INTRA_MODE_FACTOR1     37
#define INTRA_MODE_FACTOR2     78
    H2PutRegVal(regs, HWIF_ENC_INTRA_SIZE_FACTOR_0, INTRA_SIZE_FACTOR0);
    H2PutRegVal(regs, HWIF_ENC_INTRA_SIZE_FACTOR_1, INTRA_SIZE_FACTOR1);
    H2PutRegVal(regs, HWIF_ENC_INTRA_SIZE_FACTOR_2, INTRA_SIZE_FACTOR2);
    H2PutRegVal(regs, HWIF_ENC_INTRA_SIZE_FACTOR_3, INTRA_SIZE_FACTOR3);
    H2PutRegVal(regs, HWIF_ENC_INTRA_MODE_FACTOR_0, INTRA_MODE_FACTOR0);
    H2PutRegVal(regs, HWIF_ENC_INTRA_MODE_FACTOR_1, INTRA_MODE_FACTOR1);
    H2PutRegVal(regs, HWIF_ENC_INTRA_MODE_FACTOR_2, INTRA_MODE_FACTOR2);
    return 0;
}

static int _seq_done(mhve_ops* mops)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    h265_enc* h265 = h2v3->h265;
    rqct_ops* rqct = h2v3->rqct;
    rqct->seq_done(rqct);
    h265_seq_done(h265);
    return 0;
}

static int _enc_buff(mhve_ops* mops, mhve_vpb* mvpb)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    rqct_ops* rqct = h2v3->rqct;
    h265_enc* h265 = h2v3->h265;
    mhve_job* mjob = (mhve_job*)h2v3->mirr;
    rqct_buf  rqcb;
    rqct_cfg  rqcf;
    sps_t *sps = h265_find_set(h265, HEVC_SPS, 0);
    int type, rpsi = 0;
    int err = 0, idx;

    h2v3->m_encp = *mvpb;
    mvpb = &h2v3->m_encp;

    /* RQCT sequence header */
    if (h2v3->b_seqh&OPS_SEQH_START)
    {
        rqct->seq_sync(rqct);
        rqct->seq_conf(rqct);
        h265->m_pps.i_init_qp = rqct->i_enc_qp;
        h2v3->b_seqh = OPS_SEQH_RESET;
        h265_seq_sync(h265);
    }
    else
    {
        if (h2v3->b_seqh&OPS_SEQH_RESET)
        {
            rqct->seq_sync(rqct);
            h265_seq_sync(h265);
        }
    }
    if (mvpb->u_flags&MHVE_FLAGS_FORCEI)
    {
        rqct->seq_sync(rqct);
        h265_seq_sync(h265);
    }
    if (mvpb->u_flags&MHVE_FLAGS_DISREF)
        rqct->b_unrefp = 1;
    rqcb.u_config = 0;
    /* RQCT decide picture type */
    if (0 != (err = rqct->enc_buff(rqct, &rqcb)))
        return err;
    /* Prepare SPS/PPS in sequence header */
    if (h2v3->b_seqh&OPS_SEQH_RESET)
    {
        if (h265->m_sps.vui.b_timing_info_pres && h265->m_vps.b_timing_info_pres)
        {
            rqcf.type = RQCT_CFG_FPS;
            if (!(err = rqct->get_rqcf(rqct, &rqcf)))
            {
                h265->m_sps.vui.i_num_units_in_tick = (uint)rqcf.fps.d_fps;
                h265->m_sps.vui.i_time_scale = (uint)rqcf.fps.n_fps*2;
                h265->m_vps.i_num_units_in_tick = (uint)rqcf.fps.d_fps;
                h265->m_vps.i_time_scale = (uint)rqcf.fps.n_fps*2;
            }
        }
        h265_seq_conf(h265);
        h2v3->b_seqh = OPS_SEQH_WRITE;
    }
    /* RQCT register setting */
    rqct->enc_conf(rqct, mjob);

    /* Prepare slice header */
    if (IS_IPIC(rqct->i_pictyp))
    {
        type = HEVC_ISLICE;
        mvpb->u_flags = MHVE_FLAGS_FORCEI;
        h2v3->b_seqh = OPS_SEQH_WRITE;
    }
    else
    {
        if (sps->b_long_term_ref_pics_pres && IS_LTRPPIC(rqct->i_pictyp))
        {
            type = HEVC_LTRPSLICE;
        }
        else
        {    type = HEVC_PSLICE;
            if (mvpb->u_flags & MHVE_FLAGS_DISREF)
                rpsi = !rqct->b_unrefp;
            if (rqct->b_unrefp == 0)
                mvpb->u_flags &= ~MHVE_FLAGS_DISREF;
        }
    }
    h265->i_picq = rqct->i_enc_qp;
    /* pick picture buffer for reconstructing */
    if (0 <= (idx = h265_enc_buff(h265, type, !rqct->b_unrefp)))
    {
        rpb_t* rpb = h2v3->m_rpbs+idx;
        rpb->i_index = mvpb->i_index;
        /* shrink mode: recn buffer address set equal to input buffer */
        if (!rpb->b_valid)
        {
            rpb->u_planes[RPB_YPIXEL] = mvpb->planes[0].u_phys;
            rpb->u_planes[RPB_CPIXEL] = mvpb->planes[1].u_phys;
            mvpb->i_index = -1;
        }
    }
    else
    {
        err = -1;
    }
    return err;
}

static int _deq_buff(mhve_ops* mops, mhve_vpb* mvpb)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    *mvpb = h2v3->m_encp;
    return 0;
}

static int _enc_conf(mhve_ops* mops)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    h265_enc* h265 = h2v3->h265;
    mhve_vpb* mvpb =&h2v3->m_encp;
    h2v3_mir* mirr = h2v3->mirr;
    pps_t *pps = h265_find_set(h265, HEVC_PPS, 0);
    sps_t *sps = pps->p_sps;
    pic_t *ref, *recn = h265->p_recn;
    rpb_t *rpb;
    uint *regs = mirr->regs;
    /* config ouput buffer address/size */
    H2PutRegVal(regs, HWIF_ENC_SLICE_SIZE,                  h265->i_rows);
    H2PutRegVal(regs, HWIF_ENC_BASE_OUTBS_ADDR,             h2v3->u_otbs+h2v3->u_used);
    H2PutRegVal(regs, HWIF_ENC_OUTBS_BUF_LIMIT,             h2v3->u_otsz-h2v3->u_used);
//    H2PutRegVal(regs, HWIF_ENC_BASE_OUTBS_ADDR,             h2v3->u_otbs+h2v3->i_bias);
//    H2PutRegVal(regs, HWIF_ENC_OUTBS_BUF_LIMIT,             h2v3->u_otsz-h2v3->i_bias);
    /* config slice type */
    if (recn->i_type == HEVC_ISLICE)
    {
        H2PutRegVal(regs, HWIF_ENC_FRAME_CODING_TYPE,       1);
        H2PutRegVal(regs, HWIF_ENC_NAL_UNIT_TYPE,           IDR_W_RADL);
        H2PutRegVal(regs, HWIF_ENC_NUH_TEMPORAL_ID,         0);
    }
    else
    {
        H2PutRegVal(regs, HWIF_ENC_FRAME_CODING_TYPE,       0);
        H2PutRegVal(regs, HWIF_ENC_NAL_UNIT_TYPE,           TRAIL_R);
        H2PutRegVal(regs, HWIF_ENC_NUH_TEMPORAL_ID,         0);
    }
    H2PutRegVal(regs, HWIF_ENC_STRONG_INTRA_SMOOTHING_ENABLED_FLAG, sps->b_strong_intra_smoothing_enabled);
    H2PutRegVal(regs, HWIF_ENC_CONSTRAINED_INTRA_PRED_FLAG, pps->b_constrained_intra_pred);
    H2PutRegVal(regs, HWIF_ENC_POC, h265->i_poc);
    H2PutRegVal(regs, HWIF_ENC_OUTBS_MODE,                  0); // 0:stream, 1:nalu
    /* config current(input) picture */
    H2PutRegVal(regs, HWIF_ENC_BASE_INPUT_PIXY,             mvpb->planes[0].u_phys+mvpb->planes[0].i_bias);
    H2PutRegVal(regs, HWIF_ENC_BASE_INPUT_PIXC,             mvpb->planes[1].u_phys+mvpb->planes[1].i_bias);
    H2PutRegVal(regs, HWIF_ENC_CHROMA_SWAP,                 0);
    switch (h2v3->e_pixf)
    {
    case MHVE_PIX_NV21:
        H2PutRegVal(regs, HWIF_ENC_CHROMA_SWAP,             1);
    case MHVE_PIX_NV12:
        H2PutRegVal(regs, HWIF_ENC_INPUT_FORMAT,            1);
        break;
    case MHVE_PIX_YVYU:
        H2PutRegVal(regs, HWIF_ENC_CHROMA_SWAP,             1);
    case MHVE_PIX_YUYV:
        H2PutRegVal(regs, HWIF_ENC_INPUT_FORMAT,            2);
    default:
        break;
    }
    H2PutRegVal(regs, HWIF_ENC_MIN_CB_SIZE,                 sps->i_log2_min_cb_size-3);
    H2PutRegVal(regs, HWIF_ENC_MAX_CB_SIZE,                 sps->i_log2_max_cb_size-3);
    H2PutRegVal(regs, HWIF_ENC_MIN_TRB_SIZE,                sps->i_log2_min_tr_size-2);
    H2PutRegVal(regs, HWIF_ENC_MAX_TRB_SIZE,                sps->i_log2_max_tr_size-2);
    H2PutRegVal(regs, HWIF_ENC_PIC_WIDTH,                   sps->i_pic_w_min_cbs / 8);
    H2PutRegVal(regs, HWIF_ENC_PIC_HEIGHT,                  sps->i_pic_h_min_cbs / 8);
    H2PutRegVal(regs, HWIF_ENC_PPS_DEBLOCKING_FILTER_OVERRIDE_ENABLED_FLAG, pps->b_deblocking_filter_override_enabled);
    H2PutRegVal(regs, HWIF_ENC_SLICE_DEBLOCKING_FILTER_OVERRIDE_FLAG,       h265->b_deblocking_override);
    H2PutRegVal(regs, HWIF_ENC_PIC_INIT_QP,                 pps->i_init_qp);
    H2PutRegVal(regs, HWIF_ENC_CU_QP_DELTA_ENABLED,         pps->b_cu_qp_delta_enabled!=0);
    H2PutRegVal(regs, HWIF_ENC_DIFF_CU_QP_DELTA_DEPTH,      pps->i_diff_cu_qp_delta_depth);
    H2PutRegVal(regs, HWIF_ENC_CHROMA_QP_OFFSET,            pps->i_cb_qp_offset&0x1F);
    H2PutRegVal(regs, HWIF_ENC_SAO_ENABLE,                  sps->b_sao_enabled);
    H2PutRegVal(regs, HWIF_ENC_MAX_TRANS_HIERARCHY_DEPTH_INTER, sps->i_max_tr_hierarchy_depth_inter);
    H2PutRegVal(regs, HWIF_ENC_MAX_TRANS_HIERARCHY_DEPTH_INTRA, sps->i_max_tr_hierarchy_depth_intra);
    /* recn picture */
    rpb =&h2v3->m_rpbs[recn->i_id];
    H2PutRegVal(regs, HWIF_ENC_BASE_RECON_PIXY,             rpb->u_planes[RPB_YPIXEL]);
    H2PutRegVal(regs, HWIF_ENC_BASE_RECON_PIXC,             rpb->u_planes[RPB_CPIXEL]);
    H2PutRegVal(regs, HWIF_ENC_BASE_RECON_Y4NX,             rpb->u_planes[RPB_LUMA4N]);
    H2PutRegVal(regs, HWIF_ENC_RECON_COMPR_Y_EN,            rpb->u_planes[RPB_YCOMPR]!=0);
    H2PutRegVal(regs, HWIF_ENC_RECON_COMPR_C_EN,            rpb->u_planes[RPB_CCOMPR]!=0);
    H2PutRegVal(regs, HWIF_ENC_BASE_RECON_CMPY,             rpb->u_planes[RPB_YCOMPR]);
    H2PutRegVal(regs, HWIF_ENC_BASE_RECON_CMPC,             rpb->u_planes[RPB_CCOMPR]);
//    H2PutRegVal(regs, HWIF_ENC_NUM_SHORT_TERM_REF_PIC_SETS_V2,  sps->i_num_short_term_ref_pic_sets);
    H2PutRegVal(regs, HWIF_ENC_NUM_SHORT_TERM_REF_PIC_SETS_V2,  h265->i_num_short_term_ref_pic_sets);
//    printk("====================================================================================\n");
//    printk("poc(%d)\n", h265->i_poc);
//    printk("rps->i_id(%d)\n", recn->i_id);
//    printk("HWIF_ENC_BASE_RECON_PIXY(0x%08X)\n", rpb->u_planes[RPB_YPIXEL]);
    /* config slice header */
    if (recn->p_rps)
    {
        rps_t* rps = recn->p_rps;
        H2PutRegVal(regs, HWIF_ENC_RPS_ID_V2,               rps->i_id);
        H2PutRegVal(regs, HWIF_ENC_NUM_NEGATIVE_PICS,       rps->i_num_neg_pics);
        H2PutRegVal(regs, HWIF_ENC_NUM_POSITIVE_PICS,       rps->i_num_pos_pics);
        H2PutRegVal(regs, HWIF_ENC_L0_DELTA_POC0,           rps->i_dpoc[0]&0x3FF);
        H2PutRegVal(regs, HWIF_ENC_L0_USED_BY_CURR_PIC0,    rps->b_used[0]);
        if (rps->i_num_neg_pics > 1)
        {
            H2PutRegVal(regs, HWIF_ENC_L0_DELTA_POC1,       rps->i_dpoc[1]&0x3FF);
            H2PutRegVal(regs, HWIF_ENC_L0_USED_BY_CURR_PIC1,rps->b_used[1]);
        }
    }
    /* reference-list0 */
    if (h265->i_active_l0_count > 0)
    {
        if (recn->i_type == HEVC_LTRPSLICE)
            ref = h265_find_rpl(h265, HEVC_REF_LTR, 0);
        else
            ref = h265_find_rpl(h265, HEVC_REF_ACTIVE_L0, 0);
        if (!ref)
        {
            printk(KERN_ERR"h265_find_rpl return NULL\n");
            return -1;
        }
        rpb =&h2v3->m_rpbs[ref->i_id];
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP0_PIXY,         rpb->u_planes[RPB_YPIXEL]);
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP0_PIXC,         rpb->u_planes[RPB_CPIXEL]);
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP0_Y4NX,         rpb->u_planes[RPB_LUMA4N]);
        H2PutRegVal(regs, HWIF_ENC_L0RP0_COMPR_Y_EN,        rpb->u_planes[RPB_YCOMPR]!=0);
        H2PutRegVal(regs, HWIF_ENC_L0RP0_COMPR_C_EN,        rpb->u_planes[RPB_CCOMPR]!=0);
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP0_CMPY,         rpb->u_planes[RPB_YCOMPR]);
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP0_CMPC,         rpb->u_planes[RPB_CCOMPR]);
//        printk("ref->i_id(%d)\n", ref->i_id);
//        printk("HWIF_ENC_BASE_L0RP0_PIXY(0x%08X)\n", rpb->u_planes[RPB_YPIXEL]);
    }
//    printk("====================================================================================\n");
    /* reference-list0 */
    if (h265->i_active_l0_count > 1)
    {
        ref = h265_find_rpl(h265, HEVC_REF_ACTIVE_L0, 1);
        if (!ref)
        {
            printk(KERN_ERR"h265_find_rpl return NULL\n");
            return -1;
        }
        rpb =&h2v3->m_rpbs[ref->i_id];
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP1_PIXY,         rpb->u_planes[RPB_YPIXEL]);
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP1_PIXC,         rpb->u_planes[RPB_CPIXEL]);
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP1_Y4NX,         rpb->u_planes[RPB_LUMA4N]);
        H2PutRegVal(regs, HWIF_ENC_L0RP1_COMPR_Y_EN,        rpb->u_planes[RPB_YCOMPR]!=0);
        H2PutRegVal(regs, HWIF_ENC_L0RP1_COMPR_C_EN,        rpb->u_planes[RPB_CCOMPR]!=0);
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP1_CMPY,         rpb->u_planes[RPB_YCOMPR]);
        H2PutRegVal(regs, HWIF_ENC_BASE_L0RP1_CMPC,         rpb->u_planes[RPB_CCOMPR]);
    }
    H2PutRegVal(regs, HWIF_ENC_ACTIVE_OVERRIDE_FLAG,        0);
    H2PutRegVal(regs, HWIF_ENC_ACTIVE_L0_CNT,               h265->i_active_l0_count);
    H2PutRegVal(regs, HWIF_ENC_ACTIVE_L1_CNT,               h265->i_active_l1_count); // only for B-Frame, not support
    if (recn->i_type != HEVC_ISLICE)
    {
//        h265->b_active_override = h265->i_active_l0_count!=pps->i_num_ref_idx_l0_default_active;
        h265->b_active_override = h265->i_active_l0_count!=h265->i_num_ref_idx_l0_default_active;
        H2PutRegVal(regs, HWIF_ENC_ACTIVE_OVERRIDE_FLAG,    h265->b_active_override);
    }
    H2PutRegVal(regs, HWIF_ENC_SCALING_LIST_ENABLED_FLAG,   sps->b_scaling_list_enable);
#if 0
    H2PutRegVal(regs, HWIF_ENC_RCROI_ENABLE,            0); // FIXME: force disable rc&roi
    H2PutRegVal(regs, HWIF_ENC_ROIMAPDELTAQPADDR,       val->roiMapDeltaQpAddr);
    H2PutRegVal(regs, HWIF_ENC_INTRA_AREA_LEFT,         val->intraAreaLeft);
    H2PutRegVal(regs, HWIF_ENC_INTRA_AREA_RIGHT,        val->intraAreaRight);
    H2PutRegVal(regs, HWIF_ENC_INTRA_AREA_TOP,          val->intraAreaTop);
    H2PutRegVal(regs, HWIF_ENC_INTRA_AREA_BOTTOM,       val->intraAreaBottom);
    H2PutRegVal(regs, HWIF_ENC_ROI1_LEFT,               val->roi1Left);
    H2PutRegVal(regs, HWIF_ENC_ROI1_RIGHT,              val->roi1Right);
    H2PutRegVal(regs, HWIF_ENC_ROI1_TOP,                val->roi1Top);
    H2PutRegVal(regs, HWIF_ENC_ROI1_BOTTOM,             val->roi1Bottom);
    H2PutRegVal(regs, HWIF_ENC_ROI2_LEFT,               val->roi2Left);
    H2PutRegVal(regs, HWIF_ENC_ROI2_RIGHT,              val->roi2Right);
    H2PutRegVal(regs, HWIF_ENC_ROI2_TOP,                val->roi2Top);
    H2PutRegVal(regs, HWIF_ENC_ROI2_BOTTOM,             val->roi2Bottom);
    H2PutRegVal(regs, HWIF_ENC_ROI1_DELTA_QP_RC,        val->roi1DeltaQp);
    H2PutRegVal(regs, HWIF_ENC_ROI2_DELTA_QP_RC,        val->roi2DeltaQp);

    H2PutRegVal(regs, HWIF_ENC_QP_MIN,                  val->qpMin);
    H2PutRegVal(regs, HWIF_ENC_QP_MAX,                  val->qpMax);
    H2PutRegVal(regs, HWIF_ENC_RC_QPDELTA_RANGE,        val->rcQpDeltaRange);
#endif
    H2PutRegVal(regs, HWIF_ENC_DEBLOCKING_FILTER_CTRL,  h265->b_deblocking_disable);
    H2PutRegVal(regs, HWIF_ENC_DEBLOCKING_TC_OFFSET,   (h265->i_tc_offset  /2)&0xF);
    H2PutRegVal(regs, HWIF_ENC_DEBLOCKING_BETA_OFFSET, (h265->i_beta_offset/2)&0xF);
    H2PutRegVal(regs, HWIF_ENC_BASE_SLICE_NALS,         h2v3->u_nalb);
    H2PutRegVal(regs, HWIF_ENC_NAL_SIZE_WRITE,          h2v3->u_nalb!=0);
    H2PutRegVal(regs, HWIF_ENC_COFFSET,                 0);
    H2PutRegVal(regs, HWIF_ENC_YOFFSET,                 0);
    H2PutRegVal(regs, HWIF_ENC_ROWLENGTH,               h2v3->m_encp.i_pitch);
    H2PutRegVal(regs, HWIF_ENC_BASE_COMPR_COEF,         h2v3->u_coef);
    H2PutRegVal(regs, HWIF_ENC_CABAC_INIT_FLAG,         h265->b_cabac_init);
    H2PutRegVal(regs, HWIF_ENC_SLICE_INT,               0); /*reject slice-done interrupt */
    H2PutRegVal(regs, HWIF_ENC_OUTPUT_BITWIDTH_C,       0);
    H2PutRegVal(regs, HWIF_ENC_OUTPUT_BITWIDTH_Y,       0);
#if 0
    H2PutRegVal(regs, HWIF_ENC_INPUT_ROTATION,          val->inputImageRotation);
    H2PutRegVal(regs, HWIF_ENC_RGBCOEFFA,               val->colorConversionCoeffA);
    H2PutRegVal(regs, HWIF_ENC_RGBCOEFFB,               val->colorConversionCoeffB);
    H2PutRegVal(regs, HWIF_ENC_RGBCOEFFC,               val->colorConversionCoeffC);
    H2PutRegVal(regs, HWIF_ENC_RGBCOEFFE,               val->colorConversionCoeffE);
    H2PutRegVal(regs, HWIF_ENC_RGBCOEFFF,               val->colorConversionCoeffF);
    H2PutRegVal(regs, HWIF_ENC_RMASKMSB,                val->rMaskMsb);
    H2PutRegVal(regs, HWIF_ENC_GMASKMSB,                val->gMaskMsb);
    H2PutRegVal(regs, HWIF_ENC_BMASKMSB,                val->bMaskMsb);
    H2PutRegVal(regs, HWIF_ENC_XFILL,                   val->xFill);
    H2PutRegVal(regs, HWIF_ENC_YFILL,                   val->yFill);
    H2PutRegVal(regs, HWIF_ENC_SCALE_MODE,              val->scaledWidth > 0 ? 2 : 0);
    H2PutRegVal(regs, HWIF_ENC_BASESCALEDOUTLUM,        val->scaledLumBase);
    H2PutRegVal(regs, HWIF_ENC_SCALEDOUTWIDTH,          val->scaledWidth);
    H2PutRegVal(regs, HWIF_ENC_SCALEDOUTWIDTHRATIO,     val->scaledWidthRatio);
    H2PutRegVal(regs, HWIF_ENC_SCALEDOUTHEIGHT,         val->scaledHeight);
    H2PutRegVal(regs, HWIF_ENC_SCALEDOUTHEIGHTRATIO,    val->scaledHeightRatio);
    H2PutRegVal(regs, HWIF_ENC_SCALEDSKIPLEFTPIXELCOLUMN,   val->scaledSkipLeftPixelColumn);
    H2PutRegVal(regs, HWIF_ENC_SCALEDSKIPTOPPIXELROW,   val->scaledSkipTopPixelRow);
    H2PutRegVal(regs, HWIF_ENC_VSCALE_WEIGHT_EN,        val->scaledVertivalWeightEn);
    H2PutRegVal(regs, HWIF_ENC_SCALEDHORIZONTALCOPY,    val->scaledHorizontalCopy);
    H2PutRegVal(regs, HWIF_ENC_SCALEDVERTICALCOPY,      val->scaledVerticalCopy);
#endif
#if 0
    /* rate-controller */
    H2PutRegVal(regs, HWIF_ENC_TARGETPICSIZE,           val->targetPicSize);
    H2PutRegVal(regs, HWIF_ENC_MINPICSIZE,              val->minPicSize);
    H2PutRegVal(regs, HWIF_ENC_MAXPICSIZE,              val->maxPicSize);
    H2PutRegVal(regs, HWIF_ENC_CTBRCBITMEMADDR_CURRENT, val->ctbRcBitMemAddrCur);
    H2PutRegVal(regs, HWIF_ENC_CTBRCBITMEMADDR_PREVIOUS,val->ctbRcBitMemAddrPre);
    H2PutRegVal(regs, HWIF_ENC_CTBRCTHRDMIN,            val->ctbRcThrdMin);
    H2PutRegVal(regs, HWIF_ENC_CTBRCTHRDMAX,            val->ctbRcThrdMax);
    H2PutRegVal(regs, HWIF_ENC_CTBBITSMIN,              val->ctbBitsMin);
    H2PutRegVal(regs, HWIF_ENC_CTBBITSMAX,              val->ctbBitsMax);
    H2PutRegVal(regs, HWIF_ENC_TOTALLCUBITS,            val->totalLcuBits);
    H2PutRegVal(regs, HWIF_ENC_BITSRATIO,               val->bitsRatio);
    H2PutRegVal(regs, HWIF_ENC_QP_FRACTIONAL,           val->qpfrac);
    H2PutRegVal(regs, HWIF_ENC_QP_DELTA_GAIN,           val->qpDeltaMBGain);
    H2PutRegVal(regs, HWIF_ENC_COMPLEXITY_OFFSET,       val->offsetMBComplexity);
    H2PutRegVal(regs, HWIF_ENC_RC_BLOCK_SIZE,           val->rcBlockSize);
    H2PutRegVal(regs, HWIF_ENC_RC_CTBRC_SLICEQPOFFSET,  val->offsetSliceQp & mask_6b);
#endif
#if 0
    /* reference pic lists modification */
    H2PutRegVal(regs, HWIF_ENC_LISTS_MODI_PRESENT_FLAG,     val->lists_modification_present_flag);
    H2PutRegVal(regs, HWIF_ENC_REF_PIC_LIST_MODI_FLAG_L0,   val->ref_pic_list_modification_flag_l0);
    H2PutRegVal(regs, HWIF_ENC_LIST_ENTRY_L0_PIC0,          val->list_entry_l0[0]);
    H2PutRegVal(regs, HWIF_ENC_LIST_ENTRY_L0_PIC1,          val->list_entry_l0[1]);
    H2PutRegVal(regs, HWIF_ENC_REF_PIC_LIST_MODI_FLAG_L1,   val->ref_pic_list_modification_flag_l1);
    H2PutRegVal(regs, HWIF_ENC_LIST_ENTRY_L1_PIC0,          val->list_entry_l1[0]);
    H2PutRegVal(regs, HWIF_ENC_LIST_ENTRY_L1_PIC1,          val->list_entry_l1[1]);
#endif
#if 0
    /* denoise related */
    H2PutRegVal(regs, HWIF_ENC_NOISE_REDUCTION_ENABLE,      val->noiseReductionEnable);
    H2PutRegVal(regs, HWIF_ENC_NOISE_LOW,                   val->noiseLow);
    val->nrMbNumInvert = (1<<MB_NUM_BIT_WIDTH)/(((((val->picWidth + 63) >> 6) << 6) * (((val->picHeight + 63) >> 6) << 6))/(16*16));
    H2PutRegVal(regs, HWIF_ENC_NR_MBNUM_INVERT_REG,         val->nrMbNumInvert);
    H2PutRegVal(regs, HWIF_ENC_SLICEQP_PREV,                val->nrSliceQPPrev& mask_6b);
    H2PutRegVal(regs, HWIF_ENC_THRESH_SIGMA_CUR,            val->nrThreshSigmaCur& mask_21b);
    H2PutRegVal(regs, HWIF_ENC_SIGMA_CUR,                   val->nrSigmaCur& mask_16b);
#endif
    /* not continue encode */
    H2PutRegVal(regs, HWIF_ENC_BUFFER_FULL_CONTINUE,        0);
    return 0;
}

static int _put_data(mhve_ops* mops, void* data, int size)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    h265_enc* h265 = h2v3->h265;
    sps_t*   sps = h265_find_set(h265, HEVC_SPS, 0);
    void* dst;

    /* find writer pointer */
    dst = (char*)h2v3->p_otpt + h2v3->u_used;
    if (sps->b_long_term_ref_pics_pres)
    {
        /* clear buffer head bias to 0x80 */
        MEM_SETV(dst, 0x80, SLICE_HEADER_BIAS_SIZE*2);
    }
    /* insert sequence-header(VPS/SPS/PPS) */
    if (h2v3->b_seqh & OPS_SEQH_WRITE)
    {
        MEM_COPY(dst, h265->m_seqh, h265->i_seqh);
        h2v3->u_used += h265->i_seqh;
        dst += h265->i_seqh;
        if (sps->b_long_term_ref_pics_pres)
        {
            /* clear buffer head bias to 0x80 */
            MEM_SETV(dst, 0x80, SLICE_HEADER_BIAS_SIZE*2);
        }
    }
    /* insert user data */
    if (size > 0)
    {
        MEM_COPY(dst, data, size);
        h2v3->u_used += size;
        dst += size;
        if (sps->b_long_term_ref_pics_pres)
        {
            /* clear buffer head bias to 0x80 */
            MEM_SETV(dst, 0x80, SLICE_HEADER_BIAS_SIZE*2);
        }
    }

    /* padding bias for hack hw slice header */
    if (sps->b_long_term_ref_pics_pres)
    {
        /* reset bias */
        h2v3->i_bias = 0;
        /* if LTR mode padding N SLICE_HEADER_BIAS_SIZE */
        while (h2v3->i_bias - (int)h2v3->u_used < SLICE_HEADER_BIAS_SIZE)
        {
            h2v3->i_bias += SLICE_HEADER_BIAS_SIZE;
        }
        /* Add padding NALU */
        if (h2v3->b_seqh & OPS_SEQH_WRITE)
        {
            int nalu_len;
            nalu_len = h265_write_padding_head(dst, SLICE_HEADER_BIAS_SIZE-(h2v3->u_used%SLICE_HEADER_BIAS_SIZE));
            if (nalu_len > 0)
            {
                h2v3->u_used += nalu_len;
                dst += nalu_len;
                /* clear buffer head bias to 0x80 */
                MEM_SETV(dst, 0x80, SLICE_HEADER_BIAS_SIZE*2);
            }
        }
        h2v3->u_used = h2v3->i_bias;
    }
    /* check buffer used length */
    if (h2v3->u_used > h2v3->u_otsz)
    {
        printk(KERN_ERR"%s() used(%u) > buf(%u)\n", __func__, h2v3->u_used, h2v3->u_otsz);
        return -1;
    }
    return h2v3->u_used;
}

static int _enc_done(mhve_ops* mops)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    h265_enc* h265 = h2v3->h265;
    rqct_ops* rqct = h2v3->rqct;
    h2v3_mir* mirr = h2v3->mirr;
    mhve_vpb* vpb = &h2v3->m_encp;
    sps_t*   sps = h265_find_set(h265, HEVC_SPS, 0);
    slice_t* sh  = h265_find_set(h265, HEVC_SLICE, 0);
    int idx, err = 0;
    char* dst;


//    int i;
//    pic_t* pic_ = h265->p_recn;
//    printk("=======================================\n");
//    printk("input(idx<%d>)-poc(%d), ref.(%d), keep(%d)\n", pic_->i_id, pic_->i_poc, pic_->b_reference, pic_->b_ltr_keep);
//    for (i=0;i<h265->i_rpbn;i++)
//    {
//        pic_ = h265->m_pics+i;
//        printk("[%d<idx-%d>]-poc(%d), ref.(%d), keep(%d)\n", i, pic_->i_id, pic_->i_poc, pic_->b_reference, pic_->b_ltr_keep);
//    }
//    printk("---------------------------------------\n");
#if defined(TRACEREG)
    uint* regs = mirr->regs;
    printk("regs[%3d]=0x%08X\n",ASIC_OUTPUT_SIZE,regs[ASIC_OUTPUT_SIZE]);
    printk("regs[%3d]=0x%08X\n",ASIC_CYCLE_COUNT,regs[ASIC_CYCLE_COUNT]);
    printk("regs[%3d]=0x%08X\n",ASIC_NZCOUNT_AVQ,regs[ASIC_NZCOUNT_AVQ]);
    printk("regs[%3d]=0x%08X\n",ASIC_INTRACU8NUM,regs[ASIC_INTRACU8NUM]);
    printk("regs[%3d]=0x%08X\n",ASIC_SKIP_CU8NUM,regs[ASIC_SKIP_CU8NUM]);
    printk("regs[%3d]=0x%08X\n",ASIC_PBF4NRDCOST,regs[ASIC_PBF4NRDCOST]);
    printk("regs[%3d]=0x%08X\n",ASIC_CTBBITS_BOUND,regs[ASIC_CTBBITS_BOUND]);
    printk("regs[%3d]=0x%08X\n",ASIC_TOTAL_LCUBITS,regs[ASIC_TOTAL_LCUBITS]);
    printk("regs[%3d]=0x%08X\n",ASIC_SIGMA0,regs[ASIC_SIGMA0]);
    printk("regs[%3d]=0x%08X\n",ASIC_SIGMA1,regs[ASIC_SIGMA1]);
    printk("regs[%3d]=0x%08X\n",ASIC_SIGMA2,regs[ASIC_SIGMA2]);
    printk("regs[%3d]=0x%08X\n",ASIC_QP_SUM,regs[ASIC_QP_SUM]);
    printk("regs[%3d]=0x%08X\n",ASIC_QP_NUM,regs[ASIC_QP_NUM]);
    printk("regs[%3d]=0x%08X\n",ASIC_PIC_COMPLEX,regs[ASIC_PIC_COMPLEX]);
#endif

    /* ltr mode, parse slice header and replace */
    do
    {
        if (sps->b_long_term_ref_pics_pres)
        {
            /* find hw slice header and parse */
            dst = (char*)h2v3->p_otpt + h2v3->i_bias;
            if (dst > (char*)h2v3->p_otpt)
            {
                MEM_COPY(h265->m_hwsh, dst, SLICE_HEADER_PARSE_SIZE);
            }
            else
            {
                printk("vhe-%s()dst<%p>, p_otpt<%p>\n", __func__, dst, h2v3->p_otpt);
                break;
            }
            h265->i_hwsh = h265_sh_parser(h265);
            /* clear hw slice header */
            if (h265->i_hwsh > 0 && h265->i_hwsh < SLICE_HEADER_BIAS_SIZE)
            {
                MEM_SETV(dst, 0, h265->i_hwsh);
            }
            else
            {
                printk("vhe-%s()h265->i_hwsh<%d>\n", __func__, h265->i_hwsh);
                break;
            }
            /* write sw slice header */
            if (h265->i_poc > 1)
                sh->num_long_term_pics = 1;
            else
                sh->num_long_term_pics = 0;

            if (!IS_IPIC(rqct->i_pictyp))
            {
                if (IS_LTRPPIC(rqct->i_pictyp))
                {
                    sh->short_term_ref_pic_set_idx = 1;
                    sh->used_by_curr_pic_lt_flag[0] = 1;
                }
                else
                {
                    sh->short_term_ref_pic_set_idx = 0;
                    sh->used_by_curr_pic_lt_flag[0] = 0;
                }
                /* set LTR POC */
                sh->poc_lsb_lt[0] = 0;
                if (h265->p_rpls[HEVC_REF_LTR][0] && h265->i_poc-h265->p_rpls[HEVC_REF_LTR][0]->i_poc > 1)
                    sh->poc_lsb_lt[0] = h265->p_rpls[HEVC_REF_LTR][0]->i_poc;
                sh->delta_poc_msb_present_flag[0] = 0;
            }
            h265->i_swsh = h265_sh_writer(h265);
            dst = (char*)dst + h265->i_hwsh - h265->i_swsh;
            /* insert sw slice header */
            if (dst > (char*)h2v3->p_otpt)
            {
                MEM_COPY(dst, h265->m_swsh, h265->i_swsh);
                if (sh->i_slice_type != SLICE_TYPE_I)
                    h2v3->i_bias = h2v3->i_bias + (h265->i_hwsh - h265->i_swsh);
                else
                    h2v3->i_bias = 0;
            }
            else
            {
                printk("vhe-%s()i_hwsh<%d>, i_swsh<%d>\n", __func__, h265->i_hwsh, h265->i_swsh);
                break;
            }
        }
    }
    while(0);

    /* rqct control */
    /* feedback to rate-controller */
    /* actually always return err = 0 */
    if ((err = rqct->enc_done(rqct, &mirr->mjob)))
    {
        rpb_t* rpb = &h2v3->m_rpbs[h265->p_recn->i_id];
        h2v3->u_size =
        h2v3->u_used =
        h2v3->u_otrm = 0;
        /* shrink mode: set cache buffer */
        if (!rpb->b_valid)
        {
            vpb->i_index = rpb->i_index;
            rpb->i_index = -1;
        }
        return err;
    }
    /* output information */
    h2v3->u_size = rqct->i_bitcnt/8;
    h2v3->u_used+= h2v3->u_size;
    h2v3->u_otrm = h2v3->u_used;
    if (0 <= (idx = h265_enc_done(h265)))
    {
        rpb_t* rpb = h2v3->m_rpbs+idx;
        /* shrink mode: return cache buffer */
        if (!rpb->b_valid)
        {
            vpb->i_index = rpb->i_index;
//            printk("%s(poc<%d>, idx<%d>)-i_index(%d)\n", __func__, h265->i_poc, idx, vpb->i_index);
        }
        rpb->i_index = -1;
//        printk("shrink return(idx<%d>)\n", vpb->i_index);
    }
//    printk("=======================================\n");
    h2v3->i_seqn++;
    h2v3->b_seqh=0;
    return 0;
}

#define MHVE_FLAGS_CTRL (MHVE_FLAGS_FORCEI|MHVE_FLAGS_DISREF)

static int _out_buff(mhve_ops* mops, mhve_cpb* mcpb)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    mhve_vpb* mvpb = &h2v3->m_encp;
    rqct_ops* rqct = h2v3->rqct;
    h265_enc* h265 = h2v3->h265;
    sps_t*   sps = h265_find_set(h265, HEVC_SPS, 0);
    int err = 0;
    if (mcpb->i_index >= 0)
    {
        h2v3->u_used = h2v3->u_otrm = 0;
        mcpb->planes[0].u_phys = 0;
        mcpb->planes[0].i_size = 0;
        mcpb->i_stamp = 0;
        mcpb->i_flags = (MHVE_FLAGS_SOP | MHVE_FLAGS_EOP);
        return err;
    }
    err = h2v3->u_used;
    mcpb->i_index = 0;
    mcpb->planes[0].u_phys = h2v3->u_otbs;
    mcpb->planes[0].i_size = err;
    mcpb->planes[0].i_bias = h2v3->i_bias;
    mcpb->i_stamp = mvpb->i_stamp;
    mcpb->i_flags = (MHVE_FLAGS_SOP | MHVE_FLAGS_EOP);
    if (err > 0)
        mcpb->i_flags |= (mvpb->u_flags & MHVE_FLAGS_CTRL);
    /* Add flag for LTR P-frame */
    if (sps->b_long_term_ref_pics_pres && IS_LTRPPIC(rqct->i_pictyp))
        mcpb->i_flags |= MVHE_FLAGS_LTR_PFRAME;
    return err;
}

static int _set_conf(mhve_ops* mops, mhve_cfg* mcfg)
{
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    h265_enc* h265 = h2v3->h265;
    rqct_ops* rqct = mops->rqct_ops(mops);
    rqct_cfg  rqcf;
    int err = -1;
    switch (mcfg->type)
    {
        pps_t* pps;
        sps_t* sps;
        vps_t* vps;
    case MHVE_CFG_RES:
        if ((unsigned)mcfg->res.e_pixf <= MHVE_PIX_YUYV)
        {
            pps = h265_find_set(h265, HEVC_PPS, 0);
            sps = pps->p_sps;
            h2v3->e_pixf = mcfg->res.e_pixf;
            h2v3->i_pixw = mcfg->res.i_pixw;
            h2v3->i_pixh = mcfg->res.i_pixh;
            h2v3->i_rpbn = mcfg->res.i_rpbn;
            h2v3->u_conf = mcfg->res.u_conf;
            h265->i_picw = h2v3->i_pixw;
            h265->i_pich = h2v3->i_pixh;
            h265->i_cb_w = (h265->i_picw + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
            h265->i_cb_h = (h265->i_pich + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
            h2v3->b_seqh|= OPS_SEQH_RESET;
            err = 0;
        }
        break;
    case MHVE_CFG_DMA:
        if (mcfg->dma.i_dmem >= 0)
        {
            uint addr = mcfg->dma.u_phys;
            int index = mcfg->dma.i_dmem;
            rpb_t* ref = h2v3->m_rpbs+index;
            ref->i_index = -1;
            ref->p_virt = mcfg->dma.p_vptr;

            ref->u_planes[RPB_YPIXEL] = !mcfg->dma.i_size[RPB_YPIXEL]?0:addr;
            ref->u_buf_size[RPB_YPIXEL] = mcfg->dma.i_size[RPB_YPIXEL];

            addr += mcfg->dma.i_size[RPB_YPIXEL];
            ref->u_planes[RPB_CPIXEL] = !mcfg->dma.i_size[RPB_CPIXEL]?0:addr;
            ref->u_buf_size[RPB_CPIXEL] = mcfg->dma.i_size[RPB_CPIXEL];

            addr += mcfg->dma.i_size[RPB_CPIXEL];
            ref->u_planes[RPB_LUMA4N] = !mcfg->dma.i_size[RPB_LUMA4N]?0:addr;
            ref->u_buf_size[RPB_LUMA4N] = mcfg->dma.i_size[RPB_LUMA4N];

            addr += mcfg->dma.i_size[RPB_LUMA4N];
            ref->u_planes[RPB_YCOMPR] = !mcfg->dma.i_size[RPB_YCOMPR]?0:addr;
            ref->u_buf_size[RPB_YCOMPR] = mcfg->dma.i_size[RPB_YCOMPR];

            addr += mcfg->dma.i_size[RPB_YCOMPR];
            ref->u_planes[RPB_CCOMPR] = !mcfg->dma.i_size[RPB_CCOMPR]?0:addr;
            ref->u_buf_size[RPB_CCOMPR] = mcfg->dma.i_size[RPB_CCOMPR];

            ref->b_valid = !ref->u_planes[RPB_YPIXEL]?0:1;
            err = 0;
//            printk("================================================================");
//            printk("vhe-buff(%d)addr:phy<0x%08X>, vir<%p>\n", index, addr, ref->p_virt);
//            printk("vhe-u_planes(%d)addr:0x%08X, size:%u\n", RPB_YPIXEL, ref->u_planes[RPB_YPIXEL], ref->u_buf_size[RPB_YPIXEL]);
//            printk("vhe-u_planes(%d)addr:0x%08X, size:%u\n", RPB_CPIXEL, ref->u_planes[RPB_CPIXEL], ref->u_buf_size[RPB_CPIXEL]);
//            printk("vhe-u_planes(%d)addr:0x%08X, size:%u\n", RPB_LUMA4N, ref->u_planes[RPB_LUMA4N], ref->u_buf_size[RPB_LUMA4N]);
//            printk("vhe-u_planes(%d)addr:0x%08X, size:%u\n", RPB_YCOMPR, ref->u_planes[RPB_YCOMPR], ref->u_buf_size[RPB_YCOMPR]);
//            printk("vhe-u_planes(%d)addr:0x%08X, size:%u\n", RPB_CCOMPR, ref->u_planes[RPB_CCOMPR], ref->u_buf_size[RPB_CCOMPR]);
//            printk("================================================================");
        }
        else if (mcfg->dma.i_dmem == MHVE_CFG_DMA_OUTPUT_BUFFER)
        {
            h2v3->p_otpt = mcfg->dma.p_vptr;
            h2v3->u_otbs = mcfg->dma.u_phys;
            h2v3->u_otsz = mcfg->dma.i_size[0];
            h2v3->u_otrm = h2v3->u_used = 0;
            h2v3->i_bias = 0;
            err = 0;
        }
        else if (mcfg->dma.i_dmem == MHVE_CFG_DMA_NALU_BUFFER)
        {
            uint  addr = mcfg->dma.u_phys;
            char* vptr = mcfg->dma.p_vptr;
            h2v3->p_nalp = (uint*)vptr;
            h2v3->u_nalb = addr;
            addr += mcfg->dma.i_size[0];
            h2v3->u_coef = 0;
            if (mcfg->dma.i_size[1])
                h2v3->u_coef = addr;
            err = 0;
        }
        else if (mcfg->dma.i_dmem == MHVE_CFG_DMA_RESET_RECN_BUFFER)
        {
//            rpb_t* ref;
#if 0
            char* vtr_addr;
            int index, i;
            printk(KERN_ERR"vhe-reset recn buffer\n");
            for (index = 0; index < 2; index++)
            {
                ref = h2v3->m_rpbs+index;
                vtr_addr = ref->p_virt;
                for (i = RPB_YPIXEL; i < RPB_PLANES; i++)
                {
                    if (ref->u_planes[i])
                    {
//                        if (i == RPB_YCOMPR || i == RPB_CCOMPR)
                        {
                            memset((void*)vtr_addr, 0, ref->u_buf_size[i]);
                            printk(KERN_ERR"vhe-buff[%d][%d] value= 0x%08X\n", index, i, *((uint*)vtr_addr));
                        }
                    }
                    vtr_addr+=ref->u_buf_size[i];
                }
            }
            ref->i_index = -1;
#else
            /* h2v3 compression mode workaround for change resolution hw clear status */
//            ref = h2v3->m_rpbs;
//            if (ref->u_planes[RPB_YCOMPR] || ref->u_planes[RPB_CCOMPR])
//            {
//                msleep(100);
//            }
#endif
            err = 0;
        }
        break;
    case MHVE_CFG_HEV:
        pps = h265_find_set(h265, HEVC_PPS, 0);
        sps = pps->p_sps;
        h265->i_profile = mcfg->hev.i_profile;
        h265->i_level = mcfg->hev.i_level;
        sps->i_log2_max_cb_size = mcfg->hev.i_log2_max_cb_size;
        sps->i_log2_min_cb_size = mcfg->hev.i_log2_min_cb_size;
        sps->i_log2_max_tr_size = mcfg->hev.i_log2_max_tr_size;
        sps->i_log2_min_tr_size = mcfg->hev.i_log2_min_tr_size;
        sps->i_max_tr_hierarchy_depth_intra = mcfg->hev.i_tr_depth_intra;
        sps->i_max_tr_hierarchy_depth_inter = mcfg->hev.i_tr_depth_inter;
        sps->b_scaling_list_enable = mcfg->hev.b_scaling_list_enable;
        sps->b_sao_enabled = mcfg->hev.b_sao_enable;
        sps->b_strong_intra_smoothing_enabled = mcfg->hev.b_strong_intra_smoothing;
        pps->i_diff_cu_qp_delta_depth = 0;
        pps->b_cu_qp_delta_enabled = mcfg->hev.b_ctu_qp_delta_enable>0;
        if (pps->b_cu_qp_delta_enabled)
            pps->i_diff_cu_qp_delta_depth = 3&(mcfg->hev.b_ctu_qp_delta_enable-1);
        pps->b_constrained_intra_pred = mcfg->hev.b_constrained_intra_pred;
        pps->b_deblocking_filter_override_enabled = mcfg->hev.b_deblocking_override_enable;
        pps->i_cb_qp_offset = mcfg->hev.i_cqp_offset;
        pps->i_cr_qp_offset = mcfg->hev.i_cqp_offset;
        pps->b_deblocking_filter_disabled =
        h265->b_deblocking_disable = mcfg->hev.b_deblocking_disable;
        if (h265->b_deblocking_disable)
            h265->i_tc_offset = h265->i_beta_offset = pps->i_tc_offset = pps->i_beta_offset = 0;
        else
        {
            h265->i_tc_offset = pps->i_tc_offset = mcfg->hev.i_tc_offset_div2*2;
            h265->i_beta_offset = pps->i_beta_offset = mcfg->hev.i_beta_offset_div2*2;
        }
        h2v3->b_seqh|= OPS_SEQH_RESET;
        err = 0;
        break;
    case MHVE_CFG_VUI:
        sps = h265_find_set(h265, HEVC_SPS, 0);
        sps->b_vui_param_pres = 0;
        if (0 != (sps->vui.b_video_full_range = (mcfg->vui.b_video_full_range!=0)))
        {
            sps->vui.b_video_signal_pres = 1;
            sps->vui.i_video_format = 5;
            sps->vui.b_colour_desc_pres = 0;
            sps->b_vui_param_pres = 1;
        }
        sps->vui.b_timing_info_pres = mcfg->vui.b_timing_info_pres!=0;
        sps->p_vps->b_timing_info_pres = mcfg->vui.b_timing_info_pres!=0;
        if (sps->vui.b_timing_info_pres && sps->p_vps->b_timing_info_pres)
        {
            rqcf.type = RQCT_CFG_FPS;
            if (!(err = rqct->get_rqcf(rqct, &rqcf)))
            {
                sps->vui.i_num_units_in_tick = (uint)rqcf.fps.d_fps;
                sps->vui.i_time_scale = (uint)rqcf.fps.n_fps*2;
                sps->p_vps->i_num_units_in_tick = (uint)rqcf.fps.d_fps;
                sps->p_vps->i_time_scale = (uint)rqcf.fps.n_fps*2;
            }
        }
        h2v3->b_seqh|= OPS_SEQH_RESET;
        err = 0;
        break;
    case MHVE_CFG_LFT:
        pps = h265_find_set(h265, HEVC_PPS, 0);
        if (!pps->b_deblocking_filter_override_enabled)
            break;
        if (!mcfg->lft.b_override)
        {
            h265->b_deblocking_override = 0;
            h265->b_deblocking_disable = pps->b_deblocking_filter_disabled;
            h265->i_tc_offset = pps->i_tc_offset;
            h265->i_beta_offset = pps->i_beta_offset;
            err = 0;
            break;
        }
        if (!mcfg->lft.b_disable && ((unsigned)(mcfg->lft.i_offsetA+6)>12 || (unsigned)(mcfg->lft.i_offsetB+6)>12))
            break;
        err = 0;
        h265->b_deblocking_override = 1;
        if (!(h265->b_deblocking_disable = mcfg->lft.b_disable))
        {
            h265->i_tc_offset  = mcfg->lft.i_offsetA*2;
            h265->i_beta_offset= mcfg->lft.i_offsetB*2;
        }
        break;
    case MHVE_CFG_SPL:
        h265->i_rows = mcfg->spl.i_rows;
        err = 0;
        break;
    case MHVE_CFG_BAC:
        h265->b_cabac_init = mcfg->bac.b_init;
        err = 0;
        break;
    case MHVE_CFG_LTR:
        {
            sps = h265_find_set(h265, HEVC_SPS, 0);
            vps = h265_find_set(h265, HEVC_VPS, 0);
            if (mcfg->ltr.b_long_term_reference)
            {
                /* NALU setting */
                vps->i_max_dec_pic_buffering = MAX_DEC_PIC_BUFFERING_DEFAULT+1;
                sps->i_max_dec_pic_buffering = MAX_DEC_PIC_BUFFERING_DEFAULT+1;
                sps->b_long_term_ref_pics_pres = 1;
                sps->i_log2_max_poc_lsb = 16;
                h265->b_enable_pred = mcfg->ltr.b_enable_pred;
            }
            else
            {
                /* NALU setting */
                vps->i_max_dec_pic_buffering = MAX_DEC_PIC_BUFFERING_DEFAULT;
                sps->i_max_dec_pic_buffering = MAX_DEC_PIC_BUFFERING_DEFAULT;
                sps->b_long_term_ref_pics_pres = 0;
                sps->i_log2_max_poc_lsb = 8;
                h265->b_enable_pred = 0;
            }
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
    h2v3_ops* h2v3 = (h2v3_ops*)mops;
    h265_enc* h265 = h2v3->h265;
    int err = -1;
    switch (mcfg->type)
    {
        pps_t* pps;
        sps_t* sps;
    case MHVE_CFG_RES:
        mcfg->res.e_pixf = h2v3->e_pixf;
        mcfg->res.i_pixw = h2v3->i_pixw;
        mcfg->res.i_pixh = h2v3->i_pixh;
        mcfg->res.i_rpbn = h2v3->i_rpbn;
        mcfg->res.u_conf = h2v3->u_conf;
        err = 0;
        break;
    case MHVE_CFG_DMA:
        if (mcfg->dma.i_dmem == MHVE_CFG_DMA_OUTPUT_BUFFER)
        {
            mcfg->dma.p_vptr = h2v3->p_otpt;
            mcfg->dma.u_phys = h2v3->u_otbs;
            mcfg->dma.i_size[0] = (int)h2v3->u_otsz;
            err = 0;
        }
        break;
    case MHVE_CFG_HEV:
        pps = h265_find_set(h265, HEVC_PPS, 0);
        sps = pps->p_sps;
        mcfg->hev.i_profile = h265->i_profile;
        mcfg->hev.i_level = h265->i_level;
        mcfg->hev.i_log2_max_cb_size = sps->i_log2_max_cb_size;
        mcfg->hev.i_log2_min_cb_size = sps->i_log2_min_cb_size;
        mcfg->hev.i_log2_max_tr_size = sps->i_log2_max_tr_size;
        mcfg->hev.i_log2_min_tr_size = sps->i_log2_min_tr_size;
        mcfg->hev.i_tr_depth_intra = sps->i_max_tr_hierarchy_depth_intra;
        mcfg->hev.i_tr_depth_inter = sps->i_max_tr_hierarchy_depth_inter;
        mcfg->hev.b_scaling_list_enable = sps->b_scaling_list_enable;
        mcfg->hev.b_sao_enable = sps->b_sao_enabled;
        mcfg->hev.b_strong_intra_smoothing = sps->b_strong_intra_smoothing_enabled;
        mcfg->hev.b_ctu_qp_delta_enable = pps->b_cu_qp_delta_enabled + pps->i_diff_cu_qp_delta_depth;
        mcfg->hev.b_constrained_intra_pred = pps->b_constrained_intra_pred;
        mcfg->hev.b_deblocking_override_enable = pps->b_deblocking_filter_override_enabled;
        mcfg->hev.i_cqp_offset = pps->i_cb_qp_offset;
        mcfg->hev.b_deblocking_disable = pps->b_deblocking_filter_disabled;
        mcfg->hev.i_tc_offset_div2 = pps->i_tc_offset/2;
        mcfg->hev.i_beta_offset_div2 = pps->i_beta_offset/2;
        err = 0;
        break;
    case MHVE_CFG_VUI:
        sps = h265_find_set(h265, HEVC_SPS, 0);
        mcfg->vui.b_video_full_range = sps->b_vui_param_pres!=0 && sps->vui.b_video_full_range!=0;
        err = 0;
        break;
    case MHVE_CFG_LFT:
        mcfg->lft.b_override= (signed char)h265->b_deblocking_override;
        mcfg->lft.b_disable = (signed char)h265->b_deblocking_disable;
        mcfg->lft.i_offsetA = (signed char)h265->i_tc_offset/2;
        mcfg->lft.i_offsetB = (signed char)h265->i_beta_offset/2;
        err = 0;
        break;
    case MHVE_CFG_SPL:
        mcfg->spl.i_rows = h265->i_rows;
        mcfg->spl.i_bits = err = 0;
        break;
    case MHVE_CFG_BAC:
        mcfg->bac.b_init = h265->b_cabac_init!=0;
        err = 0;
        break;
    case MHVE_CFG_LTR:
        sps = h265_find_set(h265, HEVC_SPS, 0);
        if (sps->b_long_term_ref_pics_pres)
        {
            mcfg->ltr.b_long_term_reference = 1;
            mcfg->ltr.b_enable_pred = h265->b_enable_pred;
        }
        else
        {
            mcfg->ltr.b_long_term_reference = 0;
            mcfg->ltr.b_enable_pred = 0;
        }
        err = 0;
        break;
    default:
        break;
    }
    return err;
}
