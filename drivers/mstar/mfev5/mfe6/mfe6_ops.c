#include <mfe6_ops.h>

#define _ALIGN(b,x)     (((x)+(1<<(b))-1)&(~((1<<(b))-1)))
#define _BITS_(s)       ((2<<(1?s))-(1<<(0?s)))
#define _MAX_(a,b)      ((a)>(b)?(a):(b))
#define _MIN_(a,b)      ((a)<(b)?(a):(b))

#define LOG2_MAX_FRAME_NUM_DEFAULT  8

#define MVBLK_DEFAULT   \
    (MHVE_INTER_SKIP|MHVE_INTER_16x16|MHVE_INTER_16x8|MHVE_INTER_8x16|\
     MHVE_INTER_8x8|MHVE_INTER_8x4|MHVE_INTER_4x8|MHVE_INTER_4x4)

static int   _seq_sync(mhve_ops*);
static int   _seq_conf(mhve_ops*);
static int   _seq_done(mhve_ops*);
static int   _enc_buff(mhve_ops*, mhve_vpb*);
static int   _deq_buff(mhve_ops*, mhve_vpb*);
static int   _put_data(mhve_ops*, void*,int);
static int   _enc_conf(mhve_ops*);
static int   _enc_done(mhve_ops*);
static int   _out_buff(mhve_ops*, mhve_cpb*);
static int   _set_conf(mhve_ops*, mhve_cfg*);
static int   _get_conf(mhve_ops*, mhve_cfg*);
static void* _rqct_ops(mhve_ops* mops)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    return mfe6->p_rqct;
}
static void* _mmfe_job(mhve_ops* mops)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    return mfe6->p_regs;
}
static void __ops_free(mhve_ops* mops)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    rqct_ops* rqct = mfe6->p_rqct;
    mfe6_reg* regs = mfe6->p_regs;
    if (rqct)
        rqct->release(rqct);
    if (regs)
        MEM_FREE(regs);
//    MEM_FREE(mops);
}

void* mmfeops_acquire(int id)
{
    mfe6_ops* mfe6 = MEM_ALLC(sizeof(mfe6_ops));

    if (!mfe6)
    {
        printk(KERN_ERR"%s() alloc fail\n", __func__);
        return mfe6;
    }

    /* default setting */
    do
    {
        mhve_ops* mops = &mfe6->ops;
        sps_t* sps = &mfe6->m_sps;
        pps_t* pps = &mfe6->m_pps;
        slice_t* sh = &mfe6->m_sh;
        /* OPs prototype configuration */
        mops->release = __ops_free;
        mops->rqct_ops = _rqct_ops;
        mops->mhve_job = _mmfe_job;
        mops->seq_sync = _seq_sync;
        mops->seq_conf = _seq_conf;
        mops->seq_done = _seq_done;
        mops->enc_buff = _enc_buff;
        mops->deq_buff = _deq_buff;
        mops->put_data = _put_data;
        mops->enc_conf = _enc_conf;
        mops->enc_done = _enc_done;
        mops->out_buff = _out_buff;
        mops->set_conf = _set_conf;
        mops->get_conf = _get_conf;

        mfe6->p_rqct = rqctmfe_acquire(id);
        mfe6->p_regs = MEM_ALLC(sizeof(mfe6_reg));
        mfe6->i_refn = 1;
        mfe6->i_dmvx = 32;
        mfe6->i_dmvy = 16;
        mfe6->i_subp = 2;
        mfe6->i_blkp[0] = MVBLK_DEFAULT;
        mfe6->i_blkp[1] = 0;
        /* user data */
        sps->i_profile_idc = 66;
        sps->i_level_idc = 30;
        sps->b_constraint_set0 = 0;
        sps->b_constraint_set1 = 1;
        sps->b_constraint_set2 = 0;
        sps->b_constraint_set3 = 0;
        sps->i_log2_max_frame_num = LOG2_MAX_FRAME_NUM_DEFAULT;
        sps->i_poc_type = 2;
        sps->i_log2_max_poc_lsb = 4; // not used for poc_type=2
        sps->i_num_ref_frames = mfe6->i_refn;
        sps->b_gaps_in_frame_num_value_allow = 0;
        sps->b_direct8x8_inference = 1;
        sps->b_crop = 0;
        /* gen pps */
        pps->i_pps_id = 0;
        pps->b_cabac = 0;
        pps->i_num_ref_idx_l0_default_active = sps->i_num_ref_frames;
        pps->i_num_ref_idx_l1_default_active = sps->i_num_ref_frames;
        pps->i_pic_init_qp = 26;
        pps->i_pic_init_qs = 26;
        pps->i_cqp_idx_offset = 0;
        pps->b_redundant_pic_cnt = 0;
        pps->b_deblocking_filter_control = 0;
        pps->b_constrained_intra_pred = 0;
        /* gen slice-header */
        sh->i_disable_deblocking_filter_idc = 0;
        sh->i_alpha_c0_offset_div2 = 0;
        sh->i_beta_offset_div2 = 0;

        if (!mfe6->p_rqct || !mfe6->p_regs)
        {
            mops->release(mops);
            kfree(mops);
        }
        mfe6->i_rpbn = 2;
        return mfe6;
    }
    while(0);

    return mfe6;
}

static int _seq_done(mhve_ops* mops)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    rqct_ops* rqct = mfe6->p_rqct;
    int i;

    rqct->seq_done(rqct);

    for (i = 0; i < mfe6->i_refn; i++)
    {
        mfe6->m_dpbs[AVC_REF_L0][i] = mfe6->m_dpbs[AVC_REF_L1][i] = mfe6->m_dpbs[AVC_REF_LTR][i] = NULL;
    }

    return 0;
}

#define MB_SIZE     16
#define CROP_UNIT_X 2
#define CROP_UNIT_Y 2

static int _seq_sync(mhve_ops* mops)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    rqct_ops* rqct = mfe6->p_rqct;
    sps_t* sps = &mfe6->m_sps;
    pps_t* pps = &mfe6->m_pps;
    slice_t* sh = &mfe6->m_sh;
    int i;

    pps->sps = sps;
    sh->sps = sps;
    sh->pps = pps;
    /* gen sps */
    sps->i_num_ref_frames = sh->b_long_term_reference ? 2 : 1;//mfe6->i_refn;
    sps->b_vui_param_pres = 1;
    sps->vui.b_video_signal_pres = 1;
    sps->vui.i_video_format = 5;
    sps->vui.b_video_full_range = 1;
    sps->vui.b_colour_desc_pres = 0;
    sps->vui.i_colour_primaries = 1;
    sps->vui.i_transf_character = 1;
    sps->vui.i_matrix_coeffs = 1;
    /* gen pps */
    pps->i_pps_id = 0;
    pps->i_num_ref_idx_l0_default_active = sps->i_num_ref_frames;
    pps->i_num_ref_idx_l1_default_active = sps->i_num_ref_frames;
    pps->i_pic_init_qp = 26;
    pps->i_pic_init_qs = 26;
    pps->b_redundant_pic_cnt = 0;
    /* rate control */
    rqct->seq_sync(rqct);
    /*!rate control */
    mfe6->b_seqh = 1;
    mfe6->i_seqn = 0;
    mfe6->i_obits = 0;
    mfe6->i_total = 0;

    /* initialize picture buffer */
    for (i = 0; i < mfe6->i_rpbn; i++)
    {
        mfe6->m_rpbs[i].i_state = RPB_STATE_FREE;
    }
    for (i = mfe6->i_rpbn; i < RPB_MAX_NR; i++)
    {
        mfe6->m_rpbs[i].i_state = RPB_STATE_INVL;
    }

    return 0;
}

static int _enc_buff(mhve_ops* mops, mhve_vpb* mvpb)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    sps_t *sps = &mfe6->m_sps;
    pps_t* pps = &mfe6->m_pps;
    slice_t* sh = &mfe6->m_sh;
    rqct_ops* rqct = mfe6->p_rqct;
    mhve_job* mjob = (mhve_job*)mfe6->p_regs;
    rpb_t* rpb;
    rqct_buf rqcb;
    rqct_cfg  rqcf;
    bs_t outbs, *bs = &outbs;
    uchar nal;
    int i, err = 0;

    mfe6->m_encp = *mvpb;
    mvpb = &mfe6->m_encp;

    /* RQCT sequence header */
    if (mfe6->b_seqh)
        rqct->seq_sync(rqct);
    if (mvpb->u_flags&MHVE_FLAGS_FORCEI)
        rqct->seq_sync(rqct);
    if (mvpb->u_flags&MHVE_FLAGS_DISREF)
        rqct->b_unrefp = 1;
    rqcb.u_config = 0;
    /* RQCT decide picture type */
    if (0 != (err = rqct->enc_buff(rqct, &rqcb)))
        return err;
    /* Prepare SPS/PPS in sequence header */
    if (mfe6->b_seqh)
    {
        static int mbs_level[5][2] = {{30, 40500},{31,108000},{32,216000},{40,245760},{50,589824}};
        int mbs;
        sps->i_mb_w = mfe6->i_mbsw;
        sps->i_mb_h = mfe6->i_mbsh;
        sps->b_crop = 0;
        if (mfe6->i_pixw&(MB_SIZE-1) || mfe6->i_pixh&(MB_SIZE-1))
        {
            sps->b_crop = 1;
            sps->crop.i_left = sps->crop.i_top = 0;
            sps->crop.i_right = ((-mfe6->i_pixw)&(MB_SIZE-1))/CROP_UNIT_X;
            sps->crop.i_bottom= ((-mfe6->i_pixh)&(MB_SIZE-1))/CROP_UNIT_Y;
        }
        mbs = sps->i_mb_w * sps->i_mb_h;
        mbs = mbs * 30;
        for (i = 0; i < 4 && mbs_level[i][1] < mbs; i++) ;
        sps->i_level_idc = mbs_level[i][0];

        if (sps->vui.b_timing_info_pres)
        {
            rqcf.type = RQCT_CFG_FPS;
            if (!(err = rqct->get_rqcf(rqct, &rqcf)))
            {
                sps->vui.i_num_units_in_tick = (uint)rqcf.fps.d_fps;
                sps->vui.i_time_scale = (uint)rqcf.fps.n_fps*2;
            }
            sps->vui.b_fixed_frame_rate = 1;
        }

        avc_reset(bs, mfe6->m_seqh, 32);
        avc_write_nal(bs, (NAL_PRIO_HIGHEST<<5)|NAL_SPS);
        avc_write_sps(bs, sps);
        avc_flush(bs);
        avc_write_nal(bs, (NAL_PRIO_HIGHEST<<5)|NAL_PPS);
        avc_write_pps(bs, pps);
        avc_flush(bs);
        mfe6->i_seqh = avc_count(bs);
#if 0
        {
            int i;
            printk("mfe6->i_seqh = %d\n, start dump m_seqh : ", mfe6->i_seqh);
            for(i=0;i<mfe6->i_seqh/8;i++)
                printk("%x ", mfe6->m_seqh[i]);
            printk("\n end of dump\n");
        }
#endif
    }
    /* RQCT register setting */
    rqct->enc_conf(rqct, mjob);

    /* Prepare slice header paremeter */
    if (IS_IPIC(rqct->i_pictyp))
    {
        sh->b_idr_pic = 1;
        sh->i_ref_idc = NAL_PRIO_HIGHEST;
        sh->b_ref_pic_list_modification_flag_l0 = 0;
        sh->i_type = SLICE_I;
        sh->i_frm_num = 0;
        /* LTR setting */
        if (sh->b_long_term_reference)
        {
            sh->i_idr_pid = (sh->i_idr_pid+1)%2;
            sh->i_num_ref_idx_l0_active = 0;
            sh->i_poc = 0;
        }
        mvpb->u_flags|= MHVE_FLAGS_FORCEI;
        mvpb->u_flags&=~MHVE_FLAGS_DISREF;
        mfe6->b_seqh = 1;
    }
    else
    {
        sh->b_idr_pic = 0;
        sh->i_frm_num = (sh->i_frm_num+1)%(1<<sps->i_log2_max_frame_num);
        if (sh->b_long_term_reference && IS_LTRPPIC(rqct->i_pictyp))
        {
            sh->i_ref_idc = NAL_PRIO_HIGH;
            sh->i_type = SLICE_P;
            sh->b_ref_pic_list_modification_flag_l0 = 1;
            sh->rpl0_t[0].modification_of_pic_nums_idc = 2;
            sh->rpl0_t[0].long_term_pic_num = 0;
            sh->rpl0_t[1].modification_of_pic_nums_idc = 3;
            /* LTR P ref P */
            if (mfe6->b_enable_pred)
                sh->b_adaptive_ref_pic_marking_mode = 1;
            else
                sh->b_adaptive_ref_pic_marking_mode = 0;
            sh->mmc_t[0].memory_management_control = 5;
            sh->mmc_t[1].memory_management_control = 6;
            sh->mmc_t[1].long_term_frame_idx = 0;
            sh->mmc_t[2].memory_management_control = 0;
        }
        else
        {
            sh->i_ref_idc = (rqct->b_unrefp!=0)?NAL_PRIO_DISPOSED:NAL_PRIO_HIGH;
            sh->i_type = IS_PPIC(rqct->i_pictyp)? SLICE_P : SLICE_B;
            sh->b_ref_pic_list_modification_flag_l0 = 0;
            sh->b_adaptive_ref_pic_marking_mode = 0;
            if (!rqct->b_unrefp)
                mvpb->u_flags &= ~MHVE_FLAGS_DISREF;
        }
    }
    sh->i_qp = rqct->i_enc_qp;

    /* pick picture buffer for reconstructing */
    rpb = mfe6->p_recn;
    if (!rpb)
    {
        /* find free buffer */
        for (i = 0; i < mfe6->i_rpbn; i++)
            if (IS_FREE(mfe6->m_rpbs[i].i_state))
                break;
        if (i == mfe6->i_rpbn)
        {
            printk(KERN_ERR"%s-buf idx overflow(%d)\n", __func__, i);
            return -1;
        }
        rpb = &mfe6->m_rpbs[i];
        /* setup buffer state */
        rpb->i_state = RPB_STATE_BUSY;
        if (sh->b_long_term_reference)
        {
            /* Long term keep buffer */
            if (IS_IPIC(rqct->i_pictyp) || (mfe6->b_enable_pred && IS_LTRPPIC(rqct->i_pictyp)))
                rpb->i_state = RPB_STATE_KEEP;
        }
        mfe6->p_recn = rpb;
    }
    /* not allocate direct memory */
    if (!rpb->b_valid)
    {
        rpb->i_index = mvpb->i_index;
        rpb->u_phys[RPB_YPIX] = mvpb->planes[0].u_phys;
        rpb->u_phys[RPB_CPIX] = mvpb->planes[1].u_phys;
        mvpb->i_index = -1;
    }

    /* prepare slice header */
    nal = (sh->i_ref_idc<<5) | NAL_SLICE;
    if (sh->b_idr_pic)
        nal = (NAL_PRIO_HIGHEST<<5) | NAL_IDR;
    avc_reset(bs, mfe6->m_pich, 32);
    avc_write_nal(bs, nal);
    avc_write_slice_header(bs, sh);
    avc_flush(bs);
    mfe6->i_pich = avc_count(bs);

    return 0;
}

static int _deq_buff(mhve_ops* mops, mhve_vpb* mvpb)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    *mvpb = mfe6->m_encp;
    return 0;
}

static int _enc_done(mhve_ops* mops)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    slice_t* sh = &mfe6->m_sh;
    rqct_ops* rqct = mfe6->p_rqct;
    mfe6_reg* regs = mfe6->p_regs;
    mhve_vpb* vpb = &mfe6->m_encp;
    rpb_t* dpb = mfe6->p_recn;
    int err = 0;
    int i;

    /* rqct control */
    /* feedback to rate-controller */
    /* actually always return err = 0 */
    if ((err = rqct->enc_done(rqct, &regs->mjob)))
    {
        /* not allocate direct memory */
        if (!dpb->b_valid)
        {
            vpb->i_index = dpb->i_index;
            dpb->i_index = -1;
        }
        mfe6->i_obits =
        mfe6->u_oused = 0;
        return err;
    }
    /* output information */
    mfe6->i_obits = (rqct->i_bitcnt);
    mfe6->i_total+= (rqct->i_bitcnt)/8;
    mfe6->u_oused+= (rqct->i_bitcnt + regs->bits_delta)/8;
    /* update ref-list */
    if (sh->i_ref_idc)
    {   /* add recon. into reference list */
        int b_ltr_keep = 0;
        rpb_t* out = NULL;

        /* get LTR status */
        if (dpb)
            b_ltr_keep = IS_KEEP(dpb->i_state);

//        /* clear reference count and buffer ref. status */
//        for (i = 0; i < mfe6->i_rpbn; i++)
//        {
//            rpb_t* rpb = mfe6->m_rpbs+i;
//            if (!IS_KEEP(rpb->i_state))
//                rpb->i_state = RPB_STATE_FREE;
//        }
        /* update LTR list */
        if (b_ltr_keep)
        {
            /* return previos LTR buffer if encoded picture is LTR frame */
            out = mfe6->m_dpbs[AVC_REF_LTR][0];
            mfe6->m_dpbs[AVC_REF_LTR][0] = dpb;
            sh->i_frm_num = 0;
        }
        else
        {
            /* find the return buffer */
            for (i = 0; i < mfe6->i_rpbn; i++)
            {
                rpb_t* rpb = mfe6->m_rpbs+i;
                if (IS_BUSY(rpb->i_state) && rpb != dpb)
                {
                    out = rpb;
                    break;
                }
            }
        }
        /* update STR list */
        for (i = 0; i < mfe6->i_refn; i++)
        {
            mfe6->m_dpbs[AVC_REF_L0][i] = dpb;
        }
        dpb = out;
        if (++sh->i_num_ref_idx_l0_active > mfe6->i_refn)
            sh->i_num_ref_idx_l0_active = mfe6->i_refn;
    }
    /* return buffer information */
    if (dpb)
    {
        /* shrink mode: find the return buffer index */
        if (!dpb->b_valid)
            vpb->i_index = dpb->i_index;
        dpb->i_index = -1;
        dpb->i_state = RPB_STATE_FREE;
    }
    mfe6->p_recn = NULL;
    ++sh->i_poc;
    ++mfe6->i_seqn;
    mfe6->b_seqh=0;
    return 0;
}

#define MHVE_FLAGS_CTRL (MHVE_FLAGS_FORCEI|MHVE_FLAGS_DISREF)

static int _out_buff(mhve_ops* mops, mhve_cpb* mcpb)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    mhve_vpb* mvpb = &mfe6->m_encp;
    rqct_ops* rqct = mfe6->p_rqct;
    slice_t* sh = &mfe6->m_sh;
    int err = 0;
    if (mcpb->i_index >= 0)
    {
        mfe6->u_oused = 0;
        mcpb->planes[0].u_phys = 0;
        mcpb->planes[0].i_size = 0;
        mcpb->i_stamp = 0;
        mcpb->i_flags = (MHVE_FLAGS_SOP | MHVE_FLAGS_EOP);
        return err;
    }
    err = mfe6->u_oused;
    mcpb->i_index = 0;
    mcpb->planes[0].u_phys = mfe6->u_obase;
    mcpb->planes[0].i_size = err;
    mcpb->planes[0].i_bias = 0;
    mcpb->i_stamp = mvpb->i_stamp;
    mcpb->i_flags = (MHVE_FLAGS_SOP | MHVE_FLAGS_EOP);
    if (err > 0)
        mcpb->i_flags |= (mvpb->u_flags & MHVE_FLAGS_CTRL);
    /* Add flag for LTR P-frame */
    if (sh->b_long_term_reference && IS_LTRPPIC(rqct->i_pictyp))
        mcpb->i_flags |= MVHE_FLAGS_LTR_PFRAME;
    return err;
}

static int _seq_conf(mhve_ops* mops)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    mfe6_reg* regs = mfe6->p_regs;
    pps_t* pps = &mfe6->m_pps;
    /* HW-IP related regs setting */
    // pre-fetch
    regs->reg68_s_prfh_cryc_en = 1; // increase the tolerance of DRAM latency
    regs->reg68_s_prfh_refy_en = 1; // increase the tolerance of DRAM latency
    regs->reg6d_s_prfh_cryc_idle_cnt = 0;
    regs->reg6d_s_prfh_refy_idle_cnt = 0;
    // pixel format
    regs->reg03_g_mstar_tile = 1;
    regs->regd6_g_yuvldr_en = 0;
    regs->regf4_s_plnrldr_en = 1;
    // qtable
    regs->reg03_g_qmode = 0;
    // frame coding only
    regs->reg03_g_fldpic_en = 0;
    // NOT SW Buffer Mode
    regs->reg73_g_sw_buffer_mode = 0;
    regs->reg18_g_jpe_buffer_mode = 1;    // frame-mode
    regs->reg18_g_jpe_fsvs_mode = 0;
    regs->reg18_g_viu_soft_rstz = 1;
    // IMI buffer: low bandwidth
    regs->reg68_s_marb_imi_burst_thd = 8;
    regs->reg68_s_marb_imi_timeout = 7;
    regs->reg68_s_marb_imilast_thd = 4;
    // IMI setting
    regs->reg68_s_marb_eimi_block = 0;
    regs->reg68_s_marb_lbwd_mode = 0;
    regs->reg6b_s_marb_imi_saddr_lo = 0;
    regs->reg6c_s_marb_imi_saddr_hi = 0;
#if defined(MMFE_IMI_LBW_ADDR)
    if (mfe6->i_pctw < 2048)
    {
        regs->reg68_s_marb_eimi_block = 1;
        regs->reg68_s_marb_lbwd_mode = 1;
        regs->reg6b_s_marb_imi_saddr_lo = (MMFE_IMI_LBW_ADDR>> 4)&0xFFFF;
        regs->reg6c_s_marb_imi_saddr_hi = (MMFE_IMI_LBW_ADDR>>20)|0x1000;
        regs->reg6c_s_marb_imi_cache_size = 0; // imi cache size (0: 64kB, 1:32kB, 2:16kB, 3:8kB)
    }
#endif
    // IMI buffer: dbf last-line
#if defined(MMFE_IMI_DBF_ADDR)
    regs->reg4f_s_gn_bwr_mode = 3;
    regs->reg4e_s_gn_mvibuf_saddr_lo = (MMFE_IMI_DBF_ADDR>> 3)&0xFFFF;
    regs->reg4f_s_gn_mvibuf_saddr_hi = (MMFE_IMI_DBF_ADDR>>19)|0x0800;
#else
    regs->reg4f_s_gn_bwr_mode = 0;
    regs->reg4e_s_gn_mvibuf_saddr_lo = 0;
    regs->reg4f_s_gn_mvibuf_saddr_hi = 0;
#endif
    // Frame Coding Only
    regs->reg2a_s_mvdctl_ref0_offset = 0;
    regs->reg2a_s_mvdctl_ref1_offset = 0;
    regs->reg7d_s_txip_eco0 = 0;
    regs->reg7d_s_txip_eco1 = 0;
    regs->reg7d_eco_mreq_stallgo = 1;
    regs->reg7d_eco_marb_stallgo = 0;
    regs->reg7d_reserved1 = 0;
    regs->reg7d_eco_bsp_stuffing = 1;
    regs->reg7d_eco_bsp_rdy_fix = 1;
    regs->reg7d_eco_bsp_multi_slice_fix = 0;
    /* global regs values (codec related) */
    regs->reg20_s_me_16x16_disable = !(mfe6->i_blkp[0]&MHVE_INTER_16x16);
    regs->reg20_s_me_16x8_disable = !(mfe6->i_blkp[0]&MHVE_INTER_16x8);
    regs->reg20_s_me_8x16_disable = !(mfe6->i_blkp[0]&MHVE_INTER_8x16);
    regs->reg20_s_me_8x8_disable = !(mfe6->i_blkp[0]&MHVE_INTER_8x8);
    regs->reg20_s_me_8x4_disable = !(mfe6->i_blkp[0]&MHVE_INTER_8x4);
    regs->reg20_s_me_4x8_disable = !(mfe6->i_blkp[0]&MHVE_INTER_4x8);
    regs->reg20_s_me_4x4_disable = !(mfe6->i_blkp[0]&MHVE_INTER_4x4);
    regs->reg20_s_mesr_adapt = 1;
    regs->reg21_s_ime_sr16 = (mfe6->i_dmvx<=16);
    regs->reg21_s_ime_umv_disable = 0;
    regs->reg22_s_ime_mesr_max_addr = (mfe6->i_dmvy==16 ? 95 : 85);
    regs->reg22_s_ime_mesr_min_addr = (mfe6->i_dmvy==16 ?  0 : 10);
    regs->reg23_s_ime_mvx_min = _MAX_(-mfe6->i_dmvx+32,-32+32);
    regs->reg23_s_ime_mvx_max = _MIN_( mfe6->i_dmvx+32, 31+31);
    regs->reg24_s_ime_mvy_min = _MAX_(-mfe6->i_dmvy+16,-16+16);
    regs->reg24_s_ime_mvy_max = _MIN_( mfe6->i_dmvy+16, 16+15);
    regs->reg25_s_fme_quarter_disable = (mfe6->i_subp!=2);
    regs->reg25_s_fme_half_disable = (mfe6->i_subp==0);
    regs->reg25_s_fme_pmv_enable = 0!=(mfe6->i_blkp[0]&MHVE_INTER_SKIP);
    regs->reg2b_s_ieap_constraint_intra = pps->b_constrained_intra_pred;
    regs->reg2b_s_ieap_last_mode = 8;
    regs->reg2b_s_ieap_ccest_en = 0;
    regs->reg2b_s_ieap_ccest_thr = 0;
    regs->reg2b_s_ieap_drop_i16 = 0;
    regs->reg2b_s_ieap_early_termination = 1;
    regs->reg2c_g_ieap_sram_4x2_swap = 1;
    if (mfe6->i_mbsw < 16)
        regs->regfd_eco = 1;
    return 0;
}

static int _put_data(mhve_ops* mops, void* user, int size)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    mfe6_reg* regs = mfe6->p_regs;
    char* src;
    char* dst;
    int  bcnt = 0;
    int  bits = 0;

    if (size > 0)
    {
        dst = mfe6->p_okptr;
        /* insert parm-sets */
        if (mfe6->b_seqh)
        {
            MEM_COPY(dst, mfe6->m_seqh, mfe6->i_seqh/8);
            bcnt+= mfe6->i_seqh;
            dst += mfe6->i_seqh/8;
        }
        /* insert user-data */
        MEM_COPY(dst, user, size);
        bcnt+= size*8;
        dst += size;
        /* insert slice-header */
        MEM_COPY(dst, mfe6->m_pich, mfe6->i_pich/8+1);
        bits = mfe6->i_pich;
        bcnt+= bits;
        mfe6->u_oused = (bcnt>>11)*256;
        src = mfe6->p_okptr + mfe6->u_oused;
        dst = regs->bits_coded;
        MEM_COPY(dst, src, ((bcnt+7)/8)&255);
        regs->bits_count = bcnt&2047;
        regs->bits_delta = regs->bits_count - bits;
    }
    else
    {
        dst = regs->bits_coded;
        /* insert parm-sets */
        if (mfe6->b_seqh)
        {
            MEM_COPY(dst, mfe6->m_seqh, mfe6->i_seqh/8);
            bcnt+= mfe6->i_seqh;
            dst += mfe6->i_seqh/8;
        }
        /* insert slice-header */
        MEM_COPY(dst, mfe6->m_pich, mfe6->i_pich/8+1);
        bits = mfe6->i_pich;
        bcnt+= bits;
        regs->bits_count = bcnt;
        regs->bits_delta = bcnt - bits;
        mfe6->u_oused = 0;
    }
    return 0;
}

static int _enc_conf(mhve_ops* mops)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    mhve_vpb* encp = &mfe6->m_encp;
    mfe6_reg* regs = mfe6->p_regs;
    sps_t* sps = &mfe6->m_sps;
    pps_t* pps = &mfe6->m_pps;
    slice_t* sh = &mfe6->m_sh;
    rpb_t* rpb = NULL;
    uint phys;
    ushort v;
    // codec type ( only support h.264 )
    regs->reg00_g_enc_mode = MFE_REG_ENC_H264;
    regs->reg03_g_cabac_en = pps->b_cabac;
    switch (mfe6->e_pixf)
    {
    case MHVE_PIX_NV21:
        regs->regf3_s_plnrldr_c_swap = MFE_REG_PLNRLDR_VU;
    case MHVE_PIX_NV12:
        regs->regf3_s_plnrldr_format = MFE_REG_PLNRLDR_420;
        regs->reg90_g_capture_width_y= mfe6->m_encp.i_pitch;
        regs->reg91_g_capture_width_c= mfe6->m_encp.i_pitch>>1;
        break;
    case MHVE_PIX_YVYU:
        regs->regf3_s_plnrldr_c_swap = MFE_REG_PLNRLDR_VU;
    case MHVE_PIX_YUYV:
        regs->regf3_s_plnrldr_format = MFE_REG_PLNRLDR_422;
        regs->reg90_g_capture_width_y= mfe6->m_encp.i_pitch;
        regs->reg91_g_capture_width_c = 0;
        break;
    default:
        break;
    }
    // resolution
    regs->reg01_g_pic_width = mfe6->i_pctw;
    regs->reg02_g_pic_height= mfe6->i_pcth;
    // mdc
    regs->reg29_s_mbr_qp_cidx_offset = pps->i_cqp_idx_offset;
    // clock gating
    regs->reg16 = 0xFFFF;
    // per-frame: ref_frames/frame_type
    regs->reg00_g_frame_type = sh->i_type==SLICE_I?MFE_REG_ITYPE:sh->i_type==SLICE_P?MFE_REG_PTYPE:MFE_REG_BTYPE;
    regs->reg00_g_ref_no = sh->i_num_ref_idx_l0_active==2?MFE_REG_REF_NUM_TWO:MFE_REG_REF_NUM_ONE;
    // per-frame: slice parameters
    regs->reg39_s_mdc_h264_nal_ref_idc = sh->i_ref_idc;
    regs->reg39_s_mdc_h264_nal_unit_type = sh->b_idr_pic;
    regs->reg39_s_mdc_h264_fnum_bits = sps->i_log2_max_frame_num-5;
    regs->reg39_s_mdc_h264_dbf_control = pps->b_deblocking_filter_control;
    regs->reg39_s_mdc_h264_fnum_value = sh->i_frm_num;
    regs->reg3a_s_mdc_h264_idr_pic_id = sh->i_idr_pid;
    regs->reg3a_s_mdc_h264_disable_dbf_idc = sh->i_disable_deblocking_filter_idc;
    regs->reg3a_s_mdc_h264_alpha = sh->i_alpha_c0_offset_div2;
    regs->reg3a_s_mdc_h264_beta = sh->i_beta_offset_div2;
    regs->reg3a_s_mdc_h264_ridx_aor_flag = (sh->i_num_ref_idx_l0_active!=sh->pps->i_num_ref_idx_l0_default_active);
    // per-frame: buffer setting: current/reference/reconstructing
    phys = encp->planes[0].u_phys+encp->planes[0].i_bias;
    regs->reg06_g_cur_y_addr_lo = (ushort)(phys>> 8);
    regs->reg07_g_cur_y_addr_hi = (ushort)(phys>>24);
    phys = encp->planes[1].u_phys+encp->planes[1].i_bias;
    regs->reg08_g_cur_c_addr_lo = (ushort)(phys>> 8);
    regs->reg09_g_cur_c_addr_hi = (ushort)(phys>>24);

    /* setup ref. picture */
    if (sh->b_long_term_reference && sh->b_ref_pic_list_modification_flag_l0)
    {
        rpb = mfe6->m_dpbs[AVC_REF_LTR][0];
    }
    else if (sh->i_num_ref_idx_l0_active)
    {
        rpb = mfe6->m_dpbs[AVC_REF_L0][0];
    }
    if (rpb)
    {
        regs->reg0a_g_ref_y_addr0_lo = (ushort)(rpb->u_phys[RPB_YPIX]>> 8);
        regs->reg0b_g_ref_y_addr0_hi = (ushort)(rpb->u_phys[RPB_YPIX]>>24);
        regs->reg0e_g_ref_c_addr0_lo = (ushort)(rpb->u_phys[RPB_CPIX]>> 8);
        regs->reg0f_g_ref_c_addr0_hi = (ushort)(rpb->u_phys[RPB_CPIX]>>24);
    }
    else if (!rpb && sh->i_type != SLICE_I)
    {
        printk(KERN_ERR"%s() null ref buf err.\n", __func__);
        return -1;
    }

    /* setup recn buffer */
    regs->reg12_g_rec_y_addr_lo = (ushort)(mfe6->p_recn->u_phys[RPB_YPIX]>> 8);
    regs->reg13_g_rec_y_addr_hi = (ushort)(mfe6->p_recn->u_phys[RPB_YPIX]>>24);
    regs->reg14_g_rec_c_addr_lo = (ushort)(mfe6->p_recn->u_phys[RPB_CPIX]>> 8);
    regs->reg15_g_rec_c_addr_hi = (ushort)(mfe6->p_recn->u_phys[RPB_CPIX]>>24);
	// Output buffers: Must be 8-byte aligned.
    regs->reg3f_s_bspobuf_hw_en = 0;
	// GN
    regs->reg4c_s_gn_saddr_lo = (ushort)(mfe6->u_mbp_base>> 3);
    regs->reg4d_s_gn_saddr_hi = (ushort)(mfe6->u_mbp_base>>19);
    regs->reg4d_s_gn_saddr_mode = 0; // 0: gn data row, 1: gn data frame.
    // Motion search
    regs->reg20_s_me_ref_en_mode = (1<<sh->i_num_ref_idx_l0_active)-1;
    // FME
    regs->reg21_s_ime_ime_wait_fme = 1;
    regs->reg25_s_fme_pipeline_on = regs->reg21_s_ime_ime_wait_fme ? 1 : 0;
    // P8x8 BOUND RECT
    regs->reg21_s_ime_boundrect_en = (sps->i_profile_idc==66 && sps->i_level_idc<=30);
    regs->reg21_s_ime_h264_p8x8_ctrl_en = 0;//mfeinfo->i_p8x8_max_count < mfe6->i_mb_wxh ? 1 : 0;

    regs->reg25_s_fme_mode0_refno = 1;
    regs->reg25_s_fme_mode1_refno = 1;
    regs->reg25_s_fme_mode2_refno = 1;
    regs->reg25_s_fme_mode_no = (mfe6->i_mbsn <= 396);
	// Intra update (force disabled)
    regs->reg2f_s_txip_irfsh_en = 0;
    regs->reg2c_s_quan_idx_last = 63;
    if (regs->reg2c_s_quan_idx_last < 63)
        regs->reg2c_s_quan_idx_swlast = 1;
    else
        regs->reg2c_s_quan_idx_swlast = 0;
    /* multi-slice mode */
    v = mfe6->i_rows > 0;
    if (mfe6->i_bits > 0)
        v += 2;
    regs->reg03_g_er_mode = _BITS_(1:0)&(v-1);
    /* multi-slice by bits */
    regs->reg04_g_er_bs_th = mfe6->i_bits;
    /* multi-slice by row: 1/2/4/8 */
    for (v = 0; v < 3 && mfe6->i_rows > (int)(1<<v); v++) ;
        regs->reg03_g_er_mby = v;
    /* set poc */
    regs->regf7_s_mdc_h264_poc = sh->i_poc;
    regs->regf8_s_mdc_h264_poc_enable = (sps->i_poc_type == 0 ? 1 : 0);
    regs->regf8_s_mdc_h264_poc_width = sps->i_log2_max_poc_lsb;

    if (regs->regf2_g_roi_en != 0)
        regs->reg00_g_mbr_en = 0;
    regs->reg00_g_rec_en = 1;

    regs->coded_data = regs->bits_coded;
    regs->coded_bits = regs->bits_count;
    regs->outbs_addr = mfe6->u_obase + mfe6->u_oused;
    regs->outbs_size = mfe6->u_osize - mfe6->u_oused;
    return 0;
}

static int _set_conf(mhve_ops* mops, mhve_cfg* mcfg)
{
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    rqct_ops* rqct = mops->rqct_ops(mops);
    rqct_cfg  rqcf;
    int i, err = -1;
    switch (mcfg->type)
    {
    case MHVE_CFG_RES:
        if ((unsigned)mcfg->res.e_pixf <= MHVE_PIX_YVYU)
        {
            mfe6->e_pixf = mcfg->res.e_pixf;
            mfe6->i_pixw = mcfg->res.i_pixw;
            mfe6->i_pixh = mcfg->res.i_pixh;
            mfe6->i_pctw = _ALIGN(4,mcfg->res.i_pixw);
            mfe6->i_pcth = _ALIGN(4,mcfg->res.i_pixh);
            mfe6->i_rpbn = mcfg->res.i_rpbn;
            mfe6->u_conf = mcfg->res.u_conf;
            mfe6->i_mbsw = mfe6->i_pctw >>4;
            mfe6->i_mbsh = mfe6->i_pcth >>4;
            mfe6->i_mbsn = mfe6->i_mbsw*mfe6->i_mbsh;
            mfe6->b_seqh = 1;
            err = 0;
        }
        break;
    case MHVE_CFG_DMA:
        if (mcfg->dma.i_dmem >= 0)
        {
            uint addr = mcfg->dma.u_phys;
            int i = mcfg->dma.i_dmem;
            rpb_t* ref = mfe6->m_rpbs+i;
            ref->i_index =-1;
            ref->i_state = RPB_STATE_FREE;
            ref->u_phys[RPB_YPIX] = !mcfg->dma.i_size[0]?0:addr;
            addr += mcfg->dma.i_size[0];
            ref->u_phys[RPB_CPIX] = !mcfg->dma.i_size[1]?0:addr;
            addr += mcfg->dma.i_size[1];
            ref->b_valid = ref->u_phys[RPB_YPIX]!=0;
            err = 0;
        }
        else if (mcfg->dma.i_dmem == MHVE_CFG_DMA_OUTPUT_BUFFER)
        {
            mfe6->p_okptr = mcfg->dma.p_vptr;
            mfe6->u_obase = mcfg->dma.u_phys;
            mfe6->u_osize = mcfg->dma.i_size[0];
            mfe6->u_oused = 0;
            err = 0;
        }
        else if (mcfg->dma.i_dmem == MHVE_CFG_DMA_NALU_BUFFER)
        {
            mfe6->u_mbp_base = mcfg->dma.u_phys;
            err = 0;
        }
        break;
    case MHVE_CFG_MOT:
        mfe6->i_subp = mcfg->mot.i_subp;
        mfe6->i_dmvx = mcfg->mot.i_dmvx;
        mfe6->i_dmvy = mcfg->mot.i_dmvy;
        mfe6->i_blkp[0] = mcfg->mot.i_blkp[0];
        mfe6->i_blkp[1] = 0;
        err = 0;
        break;
    case MHVE_CFG_AVC:
        {
            sps_t* sps = &mfe6->m_sps;
            pps_t* pps = &mfe6->m_pps;
            slice_t* sh = &mfe6->m_sh;
            mfe6->i_refn = 1;
            sps->i_profile_idc = MHVE_AVC_PROFILE_MP;
            if (mcfg->avc.i_profile < MHVE_AVC_PROFILE_MP)
            {
                sps->i_profile_idc = MHVE_AVC_PROFILE_BP;
                pps->b_cabac = 0;
            }
            if (mcfg->avc.i_level < MHVE_AVC_LEVEL_3)
                sps->i_level_idc = MHVE_AVC_LEVEL_3;
            if (mcfg->avc.i_level > MHVE_AVC_LEVEL_4)
                sps->i_level_idc = MHVE_AVC_LEVEL_4;
            sps->i_poc_type = mcfg->avc.i_poc_type==0?0:2;
            pps->b_cabac = mcfg->avc.b_entropy_coding_type!=0;
            pps->b_deblocking_filter_control = mcfg->avc.b_deblock_filter_control;
            pps->b_constrained_intra_pred = mcfg->avc.b_constrained_intra_pred;
            sh->i_disable_deblocking_filter_idc = mcfg->avc.i_disable_deblocking_idc;
            sh->i_alpha_c0_offset_div2 = mcfg->avc.i_alpha_c0_offset;
            sh->i_beta_offset_div2 = mcfg->avc.i_beta_offset;
            mfe6->b_seqh = 1;
            err = 0;
        }
        break;
    case MHVE_CFG_VUI:
        {
            sps_t* sps = &mfe6->m_sps;
            sps->b_vui_param_pres = 0;
            sps->vui.b_video_full_range = mcfg->vui.b_video_full_range != 0;
            if (sps->vui.b_video_full_range)
            {
                sps->vui.b_video_signal_pres = 1;
                sps->vui.i_video_format = 5;
                sps->vui.b_colour_desc_pres = 0;
                sps->b_vui_param_pres = 1;
            }
            sps->vui.b_timing_info_pres = mcfg->vui.b_timing_info_pres != 0;
            if (sps->vui.b_timing_info_pres)
            {
                rqcf.type = RQCT_CFG_FPS;
                if (!(err = rqct->get_rqcf(rqct, &rqcf)))
                {
                    sps->vui.i_num_units_in_tick = (uint)rqcf.fps.d_fps;
                    sps->vui.i_time_scale = (uint)rqcf.fps.n_fps*2;
                    sps->vui.b_fixed_frame_rate = 1;
                }
            }
            mfe6->b_seqh = 1;
            err = 0;
        }
        break;
    case MHVE_CFG_SPL:
        mfe6->i_bits = mcfg->spl.i_bits;
        mfe6->i_rows = 0;
        if (0 < mcfg->spl.i_rows)
        {
            for (i = 1; i < 8 && mcfg->spl.i_rows > i; i = i<<1);
            mfe6->i_rows = mcfg->spl.i_rows = i;
        }
        err = 0;
        break;
    case MHVE_CFG_LTR:
        {
            slice_t* sh = &mfe6->m_sh;
            sps_t* sps = &mfe6->m_sps;
            if (mcfg->ltr.b_long_term_reference)
            {
                /* NALU setting */
                sh->b_long_term_reference = 1;
                sh->b_num_ref_idx_override = 1;
                sps->i_log2_max_frame_num = LOG2_MAX_FRAME_NUM_DEFAULT+2;
                sps->b_gaps_in_frame_num_value_allow = 1;
                mfe6->b_enable_pred = mcfg->ltr.b_enable_pred;
            }
            else
            {
                /* NALU setting */
                sh->b_long_term_reference = 0;
                sh->b_num_ref_idx_override = 0;
                sps->i_log2_max_frame_num = LOG2_MAX_FRAME_NUM_DEFAULT;
                sps->b_gaps_in_frame_num_value_allow = 0;
                mfe6->b_enable_pred = 0;
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
    mfe6_ops* mfe6 = (mfe6_ops*)mops;
    int err = -1;
    switch (mcfg->type)
    {
    case MHVE_CFG_RES:
        mcfg->res.e_pixf = mfe6->e_pixf;
        mcfg->res.i_pixw = mfe6->i_pixw;
        mcfg->res.i_pixh = mfe6->i_pixh;
        mcfg->res.i_rpbn = mfe6->i_rpbn;
        mcfg->res.u_conf = mfe6->u_conf;
        err = 0;
        break;
    case MHVE_CFG_MOT:
        mcfg->mot.i_subp = mfe6->i_subp;
        mcfg->mot.i_dmvx = mfe6->i_dmvx;
        mcfg->mot.i_dmvy = mfe6->i_dmvy;
        mcfg->mot.i_blkp[0] = mfe6->i_blkp[0];
        mcfg->mot.i_blkp[1] = 0;
        err = 0;
        break;
    case MHVE_CFG_AVC:
        {
            sps_t* sps = &mfe6->m_sps;
            pps_t* pps = &mfe6->m_pps;
            slice_t* sh = &mfe6->m_sh;
            mcfg->avc.i_profile = sps->i_profile_idc;
            mcfg->avc.i_level = sps->i_level_idc;
            mcfg->avc.i_num_ref_frames = mfe6->i_refn;
            mcfg->avc.i_poc_type = sps->i_poc_type;
            mcfg->avc.b_entropy_coding_type = pps->b_cabac;
            mcfg->avc.b_deblock_filter_control = pps->b_deblocking_filter_control;
            mcfg->avc.b_constrained_intra_pred = pps->b_constrained_intra_pred;
            mcfg->avc.i_disable_deblocking_idc = sh->i_disable_deblocking_filter_idc;
            mcfg->avc.i_alpha_c0_offset = sh->i_alpha_c0_offset_div2;
            mcfg->avc.i_beta_offset = sh->i_beta_offset_div2;
            err = 0;
        }
        break;
    case MHVE_CFG_VUI:
        {
            sps_t* sps = &mfe6->m_sps;
            mcfg->vui.b_video_full_range = sps->b_vui_param_pres!=0 && sps->vui.b_video_full_range!=0;
            mcfg->vui.b_timing_info_pres = sps->b_vui_param_pres!=0 && sps->vui.b_timing_info_pres!=0;
            err = 0;
        }
        break;
    case MHVE_CFG_SPL:
        mcfg->spl.i_rows = mfe6->i_rows;
        mcfg->spl.i_bits = mfe6->i_bits;
        err = 0;
        break;
    case MHVE_CFG_LTR:
        {
            slice_t* sh = &mfe6->m_sh;
            if (sh->b_long_term_reference)
            {
                mcfg->ltr.b_long_term_reference = 1;
                mcfg->ltr.b_enable_pred = mfe6->b_enable_pred;
            }
            else
            {
                mcfg->ltr.b_long_term_reference = 0;
                mcfg->ltr.b_enable_pred = 0;
            }
            err = 0;
        }
        break;
    default:
        break;
    }
    return err;
}
