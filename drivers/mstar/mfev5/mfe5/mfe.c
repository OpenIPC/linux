
#include <mfe.h>

#define _ALIGN(b,x)     (((x)+(1<<(b))-1)&(~((1<<(b))-1)))
#define _BITS_(s)       ((2<<(1?s))-(1<<(0?s)))
#define _MAX_(a,b)      ((a)>(b)?(a):(b))
#define _MIN_(a,b)      ((a)<(b)?(a):(b))

#define MVBLK_DEFAULT   \
    (MMFE_MVBLK_SKIP|MMFE_MVBLK_16x16|MMFE_MVBLK_16x8|MMFE_MVBLK_8x16|\
     MMFE_MVBLK_8x8|MMFE_MVBLK_8x4|MMFE_MVBLK_4x8|MMFE_MVBLK_4x4)

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
    mfe_hndl* hndl = (mfe_hndl*)mops;
    return hndl->p_rqct;
}
static void* _mmfe_job(mhve_ops* mops)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    return hndl->p_regs;
}
static void __ops_free(mhve_ops* mops)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    rqct_ops* rqct = hndl->p_rqct;
    mfe_regs* regs = hndl->p_regs;
    if (regs)
        MEM_FREE(regs);
    if (rqct)
        rqct->release(rqct);
    MEM_FREE(hndl);
}

void* mmfeops_acquire(int id)
{
    mhve_ops* mops = NULL;

    while (NULL != (mops = MEM_ALLC(sizeof(mfe_hndl))))
    {
        mfe_hndl* hndl = (mfe_hndl*)mops;
        sps_t* sps = &hndl->m_sps;
        pps_t* pps = &hndl->m_pps;
        slice_t* sh = &hndl->m_sh;

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

        hndl->p_rqct = rqctmfe_acquire(id);
        hndl->p_regs = MEM_ALLC(sizeof(mfe_regs));
        if (!hndl->p_regs || !hndl->p_rqct)
            break;
        hndl->i_refn = 1;
        hndl->i_dmvx = 32;
        hndl->i_dmvy = 16;
        hndl->i_subp = 2;
        hndl->i_blkp[0] = MVBLK_DEFAULT;
        hndl->i_blkp[1] = 0;
        /* user data */
        sps->i_profile_idc = 66;
        sps->i_level_idc = 30;
        sps->b_constraint_set0 = 0;
        sps->b_constraint_set1 = 1;
        sps->b_constraint_set2 = 0;
        sps->b_constraint_set3 = 0;
        sps->i_log2_max_frame_num = 5;
        sps->i_poc_type = 2;
        sps->i_log2_max_poc_lsb = 4; // not used for poc_type=2
        sps->i_num_ref_frames = hndl->i_refn;
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

        return mops;
    }
    if (mops)
        mops->release(mops);
    return NULL;
}

static int _seq_done(mhve_ops* mops)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    rqct_ops* rqct = hndl->p_rqct;
    rqct->seq_done(rqct);
    return 0;
}

#define MB_SIZE     16
#define CROP_UNIT_X 2
#define CROP_UNIT_Y 2

static int _seq_sync(mhve_ops* mops)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    rqct_ops* rqct = hndl->p_rqct;
    sps_t* sps = &hndl->m_sps;
    pps_t* pps = &hndl->m_pps;
    slice_t* sh = &hndl->m_sh;

    pps->sps = sps;
    sh->sps = sps;
    sh->pps = pps;
    /* gen sps */
    sps->i_num_ref_frames = hndl->i_refn;
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
    hndl->b_seqh = 1;
    hndl->i_seqn = 0;
    hndl->i_obits = 0;
    hndl->i_total = 0;

    return 0;
}

static int _enc_buff(mhve_ops* mops, mhve_vpb* mvpb)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    sps_t *sps = &hndl->m_sps;
    pps_t* pps = &hndl->m_pps;
    slice_t* sh = &hndl->m_sh;
    rqct_ops* rqct = hndl->p_rqct;
    mfe_regs* regs = hndl->p_regs;
    rpb_t* rpb;
    bs_t outbs, *bs = &outbs;
    rqct_buf vbuff;
    uchar nal;
    int i;

    hndl->m_encp = *mvpb;
    mvpb = &hndl->m_encp;

    if (hndl->b_seqh)
    {
        static int mbs_level[5][2] = {{30, 40500},{31,108000},{32,216000},{40,245760},{50,589824}};
        int mbs;
        sps->i_mb_w = hndl->i_mbsw;
        sps->i_mb_h = hndl->i_mbsh;
        sps->b_crop = 0;
        if (hndl->i_pixw&(MB_SIZE-1) || hndl->i_pixh&(MB_SIZE-1))
        {
            sps->b_crop = 1;
            sps->crop.i_left = sps->crop.i_top = 0;
            sps->crop.i_right = ((-hndl->i_pixw)&(MB_SIZE-1))/CROP_UNIT_X;
            sps->crop.i_bottom= ((-hndl->i_pixh)&(MB_SIZE-1))/CROP_UNIT_Y;
        }
        mbs = sps->i_mb_w * sps->i_mb_h;
        mbs = mbs * 30;
        for (i = 0; i < 4 && mbs_level[i][1] < mbs; i++) ;
        sps->i_level_idc = mbs_level[i][0];

        avc_reset(bs, hndl->m_seqh, 32);
        avc_write_nal(bs, (NAL_PRIO_HIGHEST<<5)|NAL_SPS);
        avc_write_sps(bs, sps);
        avc_flush(bs);
        avc_write_nal(bs, (NAL_PRIO_HIGHEST<<5)|NAL_PPS);
        avc_write_pps(bs, pps);
        avc_flush(bs);
        hndl->i_seqh = avc_count(bs);
        rqct->seq_sync(rqct);
    }

    vbuff.u_config = 0;
    if (hndl->m_encp.u_flags&MMFE_FLAGS_IDR)
        rqct->seq_sync(rqct);
    if (hndl->m_encp.u_flags&MMFE_FLAGS_DISPOSABLE)
        rqct->b_unrefp = 1;
    if (hndl->m_encp.u_flags&MMFE_FLAGS_NIGHT_MODE)
        vbuff.u_config |= RCTL_CFG_NIGHT;
    hndl->m_encp.u_flags &=~MMFE_FLAGS_NIGHT_MODE;

    rqct->enc_buff(rqct, &vbuff);

    rqct->enc_conf(rqct, &regs->mjob);

    if (IS_IPIC(rqct->i_pictyp))
    {
        sh->b_idr_pic = 1;
        sh->i_ref_idc = NAL_PRIO_HIGHEST;
        sh->i_type = SLICE_I;
        hndl->m_encp.u_flags|= MMFE_FLAGS_IDR;
        hndl->m_encp.u_flags&=~MMFE_FLAGS_DISPOSABLE;
        hndl->b_seqh = 1;
    }
    else
    {
        sh->b_idr_pic = 0;
        sh->i_ref_idc = (rqct->b_unrefp!=0)?NAL_PRIO_DISPOSED:NAL_PRIO_HIGH;
        sh->i_type = IS_PPIC(rqct->i_pictyp)?SLICE_P:SLICE_I;
        if (!rqct->b_unrefp)
            hndl->m_encp.u_flags&=~MMFE_FLAGS_DISPOSABLE;
    }
    sh->i_qp = rqct->i_enc_qp;

    for (i = 0; i <= hndl->i_refn; i++)
    {
        rpb = &hndl->m_rpbs[i];
        if (IS_FREE(rpb->i_state))
        {
            rpb->i_index = mvpb->i_index;
            if (!rpb->b_valid)
            {
                rpb->u_phys[RPB_YPIX] = mvpb->planes[0].u_phys;
                rpb->u_phys[RPB_CPIX] = mvpb->planes[1].u_phys;
                mvpb->i_index = -1;
            }
            hndl->p_recn = rpb;
            hndl->p_recn->i_state = RPB_STATE_BUSY;
            break;
        }
    }
    if (sh->b_idr_pic)
        for (i = 0; i < hndl->i_refn; i++)
        {
            rpb = hndl->m_dpbs[i];
            if (rpb)
            {
                if (!rpb->b_valid)
                    mvpb->i_index = rpb->i_index;
                rpb->i_state = RPB_STATE_FREE;
            }
            hndl->m_dpbs[i] = NULL;
        }
    /* set slice header */
    nal = (sh->i_ref_idc<<5) | NAL_SLICE;
    if (sh->b_idr_pic)
    {
        sh->i_idr_pic_id = (sh->i_idr_pic_id+1)%2;
        sh->i_frame_num = 0;
        sh->i_num_ref_idx_l0_active = 0;
        sh->i_poc = 0;
        nal = (NAL_PRIO_HIGHEST<<5) | NAL_IDR;
    }
    else if (sh->i_ref_idc)
    {
        sh->i_frame_num = (sh->i_frame_num+1)%(1<<sps->i_log2_max_frame_num);
    }

    avc_reset(bs, hndl->m_pich, 32);
    avc_write_nal(bs, nal);
    avc_write_slice_header(bs, sh);
    avc_flush(bs);
    hndl->i_pich = avc_count(bs);

    return 0;
}

static int _deq_buff(mhve_ops* mops, mhve_vpb* mvpb)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    *mvpb = hndl->m_encp;
    return 0;
}

static int _enc_done(mhve_ops* mops)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    slice_t* sh = &hndl->m_sh;
    rpb_t* dpb = NULL;
    rqct_ops* rqct = hndl->p_rqct;
    mfe_regs* regs = hndl->p_regs;
    mhve_vpb* vpb = &hndl->m_encp;

    rqct->enc_done(rqct, &regs->mjob);

    hndl->i_obits = (rqct->i_bitcnt);
    hndl->i_total+= (rqct->i_bitcnt)/8;
    hndl->u_oused+= (rqct->i_bitcnt + regs->delta)/8;
    /* update ref-list */
    dpb = hndl->p_recn;
    if (sh->i_ref_idc)
    {   /* add recon. into reference list */
        int i;
        for (i = 0; i < hndl->i_refn; i++)
        {
            rpb_t* out = hndl->m_dpbs[i];
            hndl->m_dpbs[i] = dpb;
            dpb = out;
        }
        if (++sh->i_num_ref_idx_l0_active > sh->sps->i_num_ref_frames)
            sh->i_num_ref_idx_l0_active = sh->sps->i_num_ref_frames;
    }
    if (dpb)
    {
        if (!dpb->b_valid)
            vpb->i_index = dpb->i_index;
        dpb->i_index = -1;
        dpb->i_state = RPB_STATE_FREE;
    }
    hndl->p_recn = NULL;
    ++sh->i_poc;
    ++hndl->i_seqn;
    hndl->b_seqh = 0;
    return 0;
}

#define MMFE_FLAGS_CTRL (MMFE_FLAGS_IDR|MMFE_FLAGS_DISPOSABLE)

static int _out_buff(mhve_ops* mops, mhve_cpb* mcpb)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    int err = 0;
    if (mcpb->i_index >= 0)
    {
        hndl->u_oused = 0;
        mcpb->planes[0].u_phys = 0;
        mcpb->planes[0].i_size = 0;
        return err;
    }
    err = hndl->u_oused;
    mcpb->i_index = 0;
    mcpb->planes[0].u_phys = hndl->u_obase;
    mcpb->planes[0].i_size = err;
    mcpb->i_stamp = hndl->m_encp.i_stamp;
    mcpb->i_flags = (MMFE_FLAGS_SOP|MMFE_FLAGS_EOP)|(hndl->m_encp.u_flags&MMFE_FLAGS_CTRL);
    return err;
}

static int _seq_conf(mhve_ops* mops)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    mfe_regs* regs = hndl->p_regs;
    pps_t* pps = &hndl->m_pps;
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
#if defined(MMFE_IMI_LBW_ADDR)
    regs->reg68_s_marb_eimi_block = 1;
    regs->reg68_s_marb_lbwd_mode = 1;
    regs->reg6b_s_marb_imi_saddr_lo = (MMFE_IMI_LBW_ADDR>> 4)&0xFFFF;
    regs->reg6c_s_marb_imi_saddr_hi = (MMFE_IMI_LBW_ADDR>>20)|0x1000;
    regs->reg6c_s_marb_imi_cache_size = 0; // imi cache size (0: 64kB, 1:32kB, 2:16kB, 3:8kB)
#else
    regs->reg68_s_marb_eimi_block = 0;
    regs->reg68_s_marb_lbwd_mode = 0;
    regs->reg6b_s_marb_imi_saddr_lo = 0;
    regs->reg6c_s_marb_imi_saddr_hi = 0;
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
    regs->reg20_s_me_16x16_disable = !(hndl->i_blkp[0]&MMFE_MVBLK_16x16);
    regs->reg20_s_me_16x8_disable = !(hndl->i_blkp[0]&MMFE_MVBLK_16x8);
    regs->reg20_s_me_8x16_disable = !(hndl->i_blkp[0]&MMFE_MVBLK_8x16);
    regs->reg20_s_me_8x8_disable = !(hndl->i_blkp[0]&MMFE_MVBLK_8x8);
    regs->reg20_s_me_8x4_disable = !(hndl->i_blkp[0]&MMFE_MVBLK_8x4);
    regs->reg20_s_me_4x8_disable = !(hndl->i_blkp[0]&MMFE_MVBLK_4x8);
    regs->reg20_s_me_4x4_disable = !(hndl->i_blkp[0]&MMFE_MVBLK_4x4);
    regs->reg20_s_mesr_adapt = 1;
    regs->reg21_s_ime_sr16 = (hndl->i_dmvx<=16);
    regs->reg21_s_ime_umv_disable = 0;
    regs->reg22_s_ime_mesr_max_addr = (hndl->i_dmvy==16 ? 95 : 85);
    regs->reg22_s_ime_mesr_min_addr = (hndl->i_dmvy==16 ?  0 : 10);
    regs->reg23_s_ime_mvx_min = _MAX_(-hndl->i_dmvx+32,-32+32);
    regs->reg23_s_ime_mvx_max = _MIN_( hndl->i_dmvx+32, 31+31);
    regs->reg24_s_ime_mvy_min = _MAX_(-hndl->i_dmvy+16,-16+16);
    regs->reg24_s_ime_mvy_max = _MIN_( hndl->i_dmvy+16, 16+15);
    regs->reg25_s_fme_quarter_disable = (hndl->i_subp!=2);
    regs->reg25_s_fme_half_disable = (hndl->i_subp==0);
    regs->reg25_s_fme_pmv_enable = 0!=(hndl->i_blkp[0]&MMFE_MVBLK_SKIP);
    regs->reg2b_s_ieap_constraint_intra = pps->b_constrained_intra_pred;
    regs->reg2b_s_ieap_last_mode = 8;
    regs->reg2b_s_ieap_ccest_en = 1;
    regs->reg2b_s_ieap_ccest_thr = 3;
    regs->reg2b_s_ieap_drop_i16 = 0;
    regs->reg2b_s_ieap_early_termination = 1;
    regs->reg2c_g_ieap_sram_4x2_swap = 1;
    if (hndl->i_mbsw < 16)
        regs->regfd_eco = 1;
    return 0;
}

static int _put_data(mhve_ops* mops, void* user, int size)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    mfe_regs* regs = hndl->p_regs;
    int  bcnt = 0;
    int  bits = 0;
    char* src;
    char* dst = hndl->p_okptr;
    /* insert parm-sets */
    if (hndl->b_seqh)
    {
        MEM_COPY(dst, hndl->m_seqh, hndl->i_seqh/8);
        bcnt+= hndl->i_seqh;
        dst += hndl->i_seqh/8;
    }
    /* insert user-data */
    if (size > 0)
    {
        MEM_COPY(dst, user, size);
        bcnt+= size*8;
        dst += size;
    }
    /* insert slice-header */
    MEM_COPY(dst, hndl->m_pich, hndl->i_pich/8+1);
    bits = hndl->i_pich;
    bcnt+= bits;
    hndl->u_oused = (bcnt>>6)*8;
    src = hndl->p_okptr + hndl->u_oused;
    dst = regs->coded;
    MEM_COPY(dst, src, ((bcnt/8)%8)+1);
    regs->count = bcnt&63;
    regs->delta = regs->count - bits;
    return 0;
}

static int _enc_conf(mhve_ops* mops)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    mhve_vpb* encp = &hndl->m_encp;
    mfe_regs* regs = hndl->p_regs;
    sps_t* sps = &hndl->m_sps;
    pps_t* pps = &hndl->m_pps;
    slice_t* sh = &hndl->m_sh;
    uint phys;
    ushort v;
    // codec type ( only support h.264 )
    regs->reg00_g_enc_mode = MFE_REG_ENC_H264;
    regs->reg03_g_cabac_en = pps->b_cabac;
    switch (hndl->e_pixf)
    {
    case MHVE_PIX_NV21:
        regs->regf3_s_plnrldr_c_swap = MFE_REG_PLNRLDR_VU;
    case MHVE_PIX_NV12:
        regs->regf3_s_plnrldr_format = MFE_REG_PLNRLDR_420;
        regs->reg90_g_capture_width_y= hndl->m_encp.i_pitch;
        regs->reg91_g_capture_width_c= hndl->m_encp.i_pitch>>1;
        break;
    case MHVE_PIX_YVYU:
        regs->regf3_s_plnrldr_c_swap = MFE_REG_PLNRLDR_VU;
    case MHVE_PIX_YUYV:
        regs->regf3_s_plnrldr_format = MFE_REG_PLNRLDR_422;
        regs->reg90_g_capture_width_y= hndl->m_encp.i_pitch;
        regs->reg91_g_capture_width_c= 0;
        break;
    default:
        break;
    }
    // resolution
    regs->reg01_g_pic_width = hndl->i_pctw;
    regs->reg02_g_pic_height= hndl->i_pcth;
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
    regs->reg39_s_mdc_h264_fnum_value = sh->i_frame_num;
    regs->reg3a_s_mdc_h264_idr_pic_id = sh->i_idr_pic_id;
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
    if (sh->i_num_ref_idx_l0_active)
    {
        regs->reg0a_g_ref_y_addr0_lo = (ushort)(hndl->m_dpbs[0]->u_phys[RPB_YPIX]>> 8);
        regs->reg0b_g_ref_y_addr0_hi = (ushort)(hndl->m_dpbs[0]->u_phys[RPB_YPIX]>>24);
        regs->reg0e_g_ref_c_addr0_lo = (ushort)(hndl->m_dpbs[0]->u_phys[RPB_CPIX]>> 8);
        regs->reg0f_g_ref_c_addr0_hi = (ushort)(hndl->m_dpbs[0]->u_phys[RPB_CPIX]>>24);
    }
    regs->reg12_g_rec_y_addr_lo = (ushort)(hndl->p_recn->u_phys[RPB_YPIX]>> 8);
    regs->reg13_g_rec_y_addr_hi = (ushort)(hndl->p_recn->u_phys[RPB_YPIX]>>24);
    regs->reg14_g_rec_c_addr_lo = (ushort)(hndl->p_recn->u_phys[RPB_CPIX]>> 8);
    regs->reg15_g_rec_c_addr_hi = (ushort)(hndl->p_recn->u_phys[RPB_CPIX]>>24);
	// Output buffers: Must be 8-byte aligned.
    regs->reg3f_s_bspobuf_hw_en = 0;
	// GN
    regs->reg4c_s_gn_saddr_lo = (ushort)(hndl->u_mbp_base>> 3);
    regs->reg4d_s_gn_saddr_hi = (ushort)(hndl->u_mbp_base>>19);
    regs->reg4d_s_gn_saddr_mode = 0; // 0: gn data row, 1: gn data frame.
    // Motion search
    regs->reg20_s_me_ref_en_mode = (1<<sh->i_num_ref_idx_l0_active)-1;
    // FME
    regs->reg21_s_ime_ime_wait_fme = 1;
    regs->reg25_s_fme_pipeline_on = regs->reg21_s_ime_ime_wait_fme ? 1 : 0;
    // P8x8 BOUND RECT
    regs->reg21_s_ime_boundrect_en = (sps->i_profile_idc==66 && sps->i_level_idc<=30);
    regs->reg21_s_ime_h264_p8x8_ctrl_en = 0;//mfeinfo->i_p8x8_max_count < hndl->i_mb_wxh ? 1 : 0;

    regs->reg25_s_fme_mode0_refno = 1;
    regs->reg25_s_fme_mode1_refno = 1;
    regs->reg25_s_fme_mode2_refno = 1;
    regs->reg25_s_fme_mode_no = (hndl->i_mbsn <= 396);
	// Intra update (force disabled)
    regs->reg2f_s_txip_irfsh_en = 0;
    regs->reg2c_s_quan_idx_last = 63;
    if (regs->reg2c_s_quan_idx_last < 63)
        regs->reg2c_s_quan_idx_swlast = 1;
    else
        regs->reg2c_s_quan_idx_swlast = 0;
    /* multi-slice mode */
    if (sps->i_poc_type != 2)
        regs->reg03_g_er_mode = _BITS_(1:0);
    else
    {
        v = hndl->i_rows > 0;
        if (hndl->i_bits > 0)
            v += 2;
        regs->reg03_g_er_mode = _BITS_(1:0)&(v-1);
        /* multi-slice by bits */
        regs->reg04_g_er_bs_th = hndl->i_bits;
        /* multi-slice by row: 1/2/4/8 */
        for (v = 0; v < 3 && hndl->i_rows > (int)(1<<v); v++) ;
        regs->reg03_g_er_mby = v;
    }

    if (regs->regf2_g_roi_en != 0)
        regs->reg00_g_mbr_en = 0;
    regs->reg00_g_rec_en = 1;

    regs->obase[0] = hndl->u_obase + hndl->u_oused;
    regs->osize[0] = hndl->u_osize - hndl->u_oused;
    return 0;
}

static int _set_conf(mhve_ops* mops, mhve_cfg* mcfg)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    int i, err = -1;
    switch (mcfg->type)
    {
    case MHVE_CFG_RES:
        if ((unsigned)mcfg->res.e_pixf <= MHVE_PIX_YVYU)
        {
            hndl->e_pixf = mcfg->res.e_pixf;
            hndl->i_pixw = mcfg->res.i_pixw;
            hndl->i_pixh = mcfg->res.i_pixh;
            hndl->i_pctw = _ALIGN(4,mcfg->res.i_pixw);
            hndl->i_pcth = _ALIGN(4,mcfg->res.i_pixh);
            hndl->i_rpbn = mcfg->res.i_rpbn;
            hndl->u_conf = mcfg->res.u_conf;
            hndl->i_mbsw = hndl->i_pctw >>4;
            hndl->i_mbsh = hndl->i_pcth >>4;
            hndl->i_mbsn = hndl->i_mbsw*hndl->i_mbsh;
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
            ref->i_index =-1;
            ref->i_state = RPB_STATE_FREE;
            ref->u_phys[RPB_YPIX] = !mcfg->dma.i_size[0]?0:addr;
            addr += mcfg->dma.i_size[0];
            ref->u_phys[RPB_CPIX] = !mcfg->dma.i_size[1]?0:addr;
            addr += mcfg->dma.i_size[1];
            ref->b_valid = ref->u_phys[RPB_YPIX]!=0;
            err = 0;
        }
        else if (mcfg->dma.i_dmem == -1)
        {
            hndl->p_okptr = mcfg->dma.p_vptr;
            hndl->u_obase = mcfg->dma.u_phys;
            hndl->u_osize = mcfg->dma.i_size[0];
            hndl->u_oused = 0;
            err = 0;
        }
        else if (mcfg->dma.i_dmem == -2)
        {
            hndl->u_mbp_base = mcfg->dma.u_phys;
            err = 0;
        }
        break;
    case MHVE_CFG_MOT:
        hndl->i_subp = mcfg->mot.i_subp;
        hndl->i_dmvx = mcfg->mot.i_dmvx;
        hndl->i_dmvy = mcfg->mot.i_dmvy; 
        hndl->i_blkp[0] = mcfg->mot.i_blkp[0];
        hndl->i_blkp[1] = 0;
        err = 0;
        break;
    case MHVE_CFG_AVC:
        {
            sps_t* sps = &hndl->m_sps;
            pps_t* pps = &hndl->m_pps;
            slice_t* sh = &hndl->m_sh;
            hndl->i_refn = 1;
            sps->i_profile_idc = MMFE_AVC_PROFILE_MP;
            if (mcfg->avc.i_profile < MMFE_AVC_PROFILE_MP)
            {
                sps->i_profile_idc = MMFE_AVC_PROFILE_BP;
                pps->b_cabac = 0;
            }
            if (mcfg->avc.i_level < MMFE_AVC_LEVEL_3)
                sps->i_level_idc = MMFE_AVC_LEVEL_3;
            if (mcfg->avc.i_level > MMFE_AVC_LEVEL_4)
                sps->i_level_idc = MMFE_AVC_LEVEL_4;
            sps->i_poc_type = mcfg->avc.i_poc_type==0?0:2;
            pps->b_cabac = mcfg->avc.b_entropy_coding_type!=0;
            pps->b_deblocking_filter_control = mcfg->avc.b_deblock_filter_control;
            pps->b_constrained_intra_pred = mcfg->avc.b_constrained_intra_pred;
            sh->i_disable_deblocking_filter_idc = mcfg->avc.i_disable_deblocking_idc;
            sh->i_alpha_c0_offset_div2 = mcfg->avc.i_alpha_c0_offset;
            sh->i_beta_offset_div2 = mcfg->avc.i_beta_offset;
            hndl->b_seqh = 1;
            err = 0;
        }
        break;
    case MHVE_CFG_VUI:
        {
            sps_t* sps = &hndl->m_sps;
            sps->b_vui_param_pres = 0;
            sps->vui.b_video_full_range = mcfg->vui.b_video_full_range!=0;
            if (sps->vui.b_video_full_range)
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
    case MHVE_CFG_SPL:
        hndl->i_bits = mcfg->spl.i_bits;
        hndl->i_rows = 0;
        if (0 < mcfg->spl.i_rows)
        {
            for (i = 1; i < 8 && mcfg->spl.i_rows > i; i = i<<1) ;
            hndl->i_rows = mcfg->spl.i_rows = i;
        }
        err = 0;
        break;
    default:
        break;
    }
    return err;
}

static int _get_conf(mhve_ops* mops, mhve_cfg* mcfg)
{
    mfe_hndl* hndl = (mfe_hndl*)mops;
    int err = -1;
    switch (mcfg->type)
    {
    case MHVE_CFG_RES:
        mcfg->res.e_pixf = hndl->e_pixf;
        mcfg->res.i_pixw = hndl->i_pixw;
        mcfg->res.i_pixh = hndl->i_pixh;
        mcfg->res.i_rpbn = hndl->i_rpbn;
        mcfg->res.u_conf = hndl->u_conf;
        err = 0;
        break;
    case MHVE_CFG_MOT:
        mcfg->mot.i_subp = hndl->i_subp;
        mcfg->mot.i_dmvx = hndl->i_dmvx;
        mcfg->mot.i_dmvy = hndl->i_dmvy;
        mcfg->mot.i_blkp[0] = hndl->i_blkp[0];
        mcfg->mot.i_blkp[1] = 0;
        err = 0;
        break;
    case MHVE_CFG_AVC:
        {
            sps_t* sps = &hndl->m_sps;
            pps_t* pps = &hndl->m_pps;
            slice_t* sh = &hndl->m_sh;
            mcfg->avc.i_profile = sps->i_profile_idc;
            mcfg->avc.i_level = sps->i_level_idc;
            mcfg->avc.i_num_ref_frames = hndl->i_refn;
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
            sps_t* sps = &hndl->m_sps;
            mcfg->vui.b_video_full_range = sps->b_vui_param_pres!=0 && sps->vui.b_video_full_range!=0;
            err = 0;
        }
        break;
    case MHVE_CFG_SPL:
        mcfg->spl.i_rows = hndl->i_rows;
        mcfg->spl.i_bits = hndl->i_bits;
        err = 0;
        break;
    default:
        break;
    }
    return err;
}
