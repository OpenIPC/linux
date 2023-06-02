
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <mdrv_mfe_io.h>

#include <types.h>

#include <mfe_avc.h>
#include <mfe_rc.h>
#include <mst_def.h>
#include <mfe_regs.h>
#include <mfe_asic.h>
#include <mfe_hdlr.h>

#define BLOCK_TYPE_4x4  0
#define BLOCK_TYPE_4x8  0
#define BLOCK_TYPE_8x4  0

#define _ALIGN_(b,x)    (((x)+(1<<(b))-1)&(~((1<<(b))-1)))
#define _BITS_(s)       ((2<<(0?s))-(1<<(1?s)))
#define _MAX_(a,b)      ((a)>(b)?(a):(b))
#define _MIN_(a,b)      ((a)<(b)?(a):(b))

int
mfe_hdlr_request(
    void**      pp)
{
    mfe_hdlr* h;
    int err = -ENOMEM;

    if (NULL != (h = kzalloc(sizeof(mfe_hdlr), GFP_KERNEL)))
    {
        sps_t* sps = &h->sps;
        pps_t* pps = &h->pps;
        slice_t* sh = &h->sh;
        h->i_num_ref_frames = 1;
        h->i_dmv_x = 16;
        h->i_dmv_y = 16;
        h->i_subpel = 2;
        h->i_mv_block[0] = MV_BLOCK_SKIP|MV_BLOCK_16x16|MV_BLOCK_16x8|MV_BLOCK_8x16|MV_BLOCK_8x8;
        h->i_mv_block[1] = 0;
        sps->i_profile_idc = 66;
        sps->i_level_idc = 30;
        sps->b_constraint_set0 = 0;
        sps->b_constraint_set1 = 1;
        sps->b_constraint_set2 = 0;
        sps->b_constraint_set3 = 0;
        sps->i_log2_max_frame_num = 5;
        sps->i_poc_type = 2;
        sps->i_log2_max_poc_lsb = 4; // not used for poc_type=2
        sps->i_num_ref_frames = h->i_num_ref_frames;
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
        pps->b_redundant_pic_cnt = 0;
        pps->b_deblocking_filter_control = 0;
        pps->b_constrained_intra_pred = 0;
        /* gen slice-header */
        sh->i_disable_deblocking_filter_idc = 0;
        sh->i_alpha_c0_offset = 0;
        sh->i_beta_offset = 0;

        mutex_init(&h->stream_lock);
        mutex_init(&h->encode_lock);
        h->b_stream = 0;
        h->i_encode = 0;
        err = 0;
    }
    *pp = h;
    return err;
}

int
mfe_hdlr_release(
    void*       p)
{
    if (p)
    {
        kfree(p);
    }
    return 0;
}

int
mfe_hdlr_seq_exit(
    mfe_hdlr*   h)
{
    cvbr_rc* rc = &h->cvbr;

    cvbr_CloseRateControl(rc);

    kfree(h->regs);
    h->regs = NULL;
    return 0;
}

#define HD_MB_SIZE  ((1280*720)/256)

int
mfe_hdlr_seq_init(
    mfe_hdlr*   h)
{
    mfe_regs* regs = NULL;
    cvbr_rc* rc = &h->cvbr;
    sps_t* sps = &h->sps;
    pps_t* pps = &h->pps;
    slice_t* sh = &h->sh;

    if (!(regs = kzalloc(sizeof(mfe_regs), GFP_KERNEL)))
    {
        return -ENOMEM;
    }
    pps->sps = sps;
    sh->sps = sps;
    sh->pps = pps;
    /* gen sps */
    sps->i_num_ref_frames = h->i_num_ref_frames;
    sps->i_mb_w = h->i_mb_w;
    sps->i_mb_h = h->i_mb_h;
    sps->b_crop = 0;
    if (sps->i_mb_h > 36)
    {
        sps->i_level_idc = 31;
    }
    /* gen pps */
    pps->i_pps_id = 0;
    pps->i_num_ref_idx_l0_default_active = sps->i_num_ref_frames;
    pps->i_num_ref_idx_l1_default_active = sps->i_num_ref_frames;
    pps->i_pic_init_qp = 26;
    pps->i_pic_init_qs = 26;
    pps->b_redundant_pic_cnt = 0;

    regs->reg20_s_me_16x16_disable = !(h->i_mv_block[0]&MV_BLOCK_16x16);
    regs->reg20_s_me_16x8_disable = !(h->i_mv_block[0]&MV_BLOCK_16x8);
    regs->reg20_s_me_8x16_disable = !(h->i_mv_block[0]&MV_BLOCK_8x16);
    regs->reg20_s_me_8x8_disable = !(h->i_mv_block[0]&MV_BLOCK_8x8);
    regs->reg20_s_me_8x4_disable = !(h->i_mv_block[0]&MV_BLOCK_8x4);
    regs->reg20_s_me_4x8_disable = !(h->i_mv_block[0]&MV_BLOCK_4x8);
    regs->reg20_s_me_4x4_disable = !(h->i_mv_block[0]&MV_BLOCK_4x4);
    regs->reg20_s_mesr_adapt = 1;
    regs->reg21_s_ime_sr16 = (h->i_dmv_x<=16);
    regs->reg21_s_ime_umv_disable = 0;
    regs->reg22_s_ime_mesr_max_addr = (h->i_dmv_y==16 ? 95 : 85);
    regs->reg22_s_ime_mesr_min_addr = (h->i_dmv_y==16 ?  0 : 10);
    regs->reg23_s_ime_mvx_min = _MAX_(-h->i_dmv_x+32,-16+32);
    regs->reg23_s_ime_mvx_max = _MIN_( h->i_dmv_x+32, 16+31);
    regs->reg24_s_ime_mvy_min = _MAX_(-h->i_dmv_y+16,-16+16);
    regs->reg24_s_ime_mvy_max = _MIN_( h->i_dmv_y+16, 16+15);
    regs->reg25_s_fme_quarter_disable = (h->i_subpel!=2);
    regs->reg25_s_fme_half_disable = (h->i_subpel==0);
    regs->reg25_s_fme_pmv_enable = 0!=(h->i_mv_block[0]&MV_BLOCK_SKIP);
    regs->reg25_s_fme_mode_no = (h->i_mb_wxh <= 396);
    regs->reg2b_s_ieap_constraint_intra = pps->b_constrained_intra_pred;
    regs->reg2b_s_ieap_last_mode = 8;
    regs->reg2b_s_ieap_ccest_en = 1;
    regs->reg2b_s_ieap_ccest_thr = 3;
    regs->reg2b_s_ieap_drop_i16 = 0;

    h->regs = regs;
    /* rate control */
    rc->fr_fps.num = h->fr_fps.num;
    rc->fr_fps.den = h->fr_fps.den;
    rc->i_fps = rc->fr_fps.num / rc->fr_fps.den;
    rc->i_codec_type = h->i_codec_type;
    rc->i_pict_w = h->i_picture_w;
    rc->i_pict_h = h->i_picture_h;
    rc->i_bps = h->i_bps;
    rc->b_fps_fixed = 1;
    rc->i_num_ppic = h->i_num_ppic;
    rc->i_num_bpic = h->i_num_bpic;
    rc->i_method = CBR;
    rc->b_mb_layer = 1;
    rc->i_bps_max = 0;
#if 0 // Test const qp
    rc->i_method = CONST_QP;
    rc->b_mb_layer = 0;
    rc->i_qp = 12;
#endif
    cvbr_InitRateControl(rc);
    /*!rate control */

    return 0;
}

void
mfe_hdlr_pic_mode(
    mfe_hdlr*   h)
{
    slice_t* sh = &h->sh;

    if (0 == (h->i_cnt_bpic%(1+h->i_num_bpic)))
    {
        if (0 == (h->i_cnt_ppic%(1+h->i_num_ppic)))
        {
            sh->i_type = SLICE_I;
            sh->b_idr_pic = 1;
            sh->i_ref_idc = NAL_PRIO_HIGHEST;
            h->i_cnt_ppic = 0;
        }
        else
        {
            sh->i_type = SLICE_P;
            sh->b_idr_pic = 0;
            sh->i_ref_idc = NAL_PRIO_HIGH;
        }
        ++h->i_cnt_ppic;
        h->i_cnt_bpic = 0;
    }
    else
    {
        sh->i_type = SLICE_B;
        sh->b_idr_pic = 0;
        sh->i_ref_idc = NAL_PRIO_DISPOSED;
    }
    ++h->i_cnt_bpic;
}

void
mfe_hdlr_pic_init(
    mfe_hdlr*   h)
{
    sps_t *sps = &h->sps;
    pps_t *pps = &h->pps;
    slice_t* sh = &h->sh;
    bs_t outbs, *bs = &outbs;
    uchar nal;
    int i;

    if (sh->b_idr_pic)
    {
        for (i = 0; i < h->i_num_ref_frames; i++)
        {
            if (h->dpb[i])
            {
                h->dpb[i]->b_busy = 0;
                h->dpb[i] = NULL;
            }
        }
    }
    if (sh->i_ref_idc)
    {
        for (i = 0; i <= h->i_num_ref_frames; i++)
        {
            if (!h->frames[i].b_busy)
            {
                h->recon = &h->frames[i];
                h->recon->b_busy = 1;
                break;
            }
        }
    }
    /* set slice header */
    nal = (sh->i_ref_idc<<5) | NAL_SLICE;
    if (sh->b_idr_pic)
    {
        sh->i_idr_pic_id = (sh->i_idr_pic_id+1)%2;
        sh->i_frame_num = 0;
        sh->i_num_ref_idx_l0_active = 0;
        nal = (NAL_PRIO_HIGHEST<<5) | NAL_IDR;
    }
    else if (sh->i_ref_idc)
    {
        sh->i_frame_num = (sh->i_frame_num+1)%(1<<sps->i_log2_max_frame_num);

        if (++sh->i_num_ref_idx_l0_active > sps->i_num_ref_frames)
        {
            sh->i_num_ref_idx_l0_active = sps->i_num_ref_frames;
        }
    }
    sh->i_qp = cvbr_InitFrame(&h->cvbr, sh->i_type==SLICE_I?I_VOP:P_VOP, 0);

    avc_reset(bs, h->code_sh, 64);
    /* first frame contains sps/pps */
    if (!h->i_seq_numb)
    {
        avc_write_nal(bs, (NAL_PRIO_HIGHEST<<5)|NAL_SPS);
        avc_write_sps(bs, sps);
        avc_flush(bs);
        avc_write_nal(bs, (NAL_PRIO_HIGHEST<<5)|NAL_PPS);
        avc_write_pps(bs, pps);
        avc_flush(bs);
    }
    avc_write_nal(bs, nal);
    avc_write_slice_header(bs, sh);
    avc_flush(bs);

    h->bits_sh = avc_count(bs);
}

void
mfe_hdlr_pic_read_bits(
    mfe_hdlr*   h)
{
    void* src = h->p_vptr_bs;
    void* dst = h->p_outbuf;
    int size;

    size = h->i_last_bits>>3;

    memcpy(dst, src, size);

    h->i_uselen = size;
    h->i_remlen = size;
    h->i_stat_size += size;
}

void
mfe_hdlr_pic_done(
    mfe_hdlr*   h)
{
    slice_t* sh = &h->sh;
    cvbr_rc* rc = &h->cvbr;
    mfe_regs* regs = h->regs;

    h->i_pic_avgqp = ((int)regs->reg29_s_mbr_last_frm_avg_qp_hi<<16) + regs->reg28_s_mbr_last_frm_avg_qp_lo;
    h->i_last_bits = ((int)regs->reg43_s_bsp_bit_cnt_hi<<16) + regs->reg42_s_bsp_bit_cnt_lo;
    /* rate control */
    rc->m_nLastFrameAvgQStep[0] = h->i_pic_avgqp;
    rc->m_nLastFrameAvgQStep[1] = 0;
    cvbr_UpdateFrame(rc, h->i_last_bits, 0, 0);
    /*!rate control */
    if (sh->i_ref_idc)
    {
        int i;
        frame_t* dpb = h->recon;
        h->recon = NULL;
        for (i = 0; i < h->i_num_ref_frames; i++)
        {
            frame_t* out = h->dpb[i];
            h->dpb[i] = dpb;
            dpb = out;
        }
        if (dpb)
        {
            dpb->b_busy = 0;
        }
    }
    ++h->i_seq_numb;
}

void
mfe_hdlr_pic_prep_regs(
    mfe_hdlr*   h)
{
	cvbr_rc* rc = &h->cvbr;
    mfe_regs* regs = h->regs;
    sps_t* sps = &h->sps;
    pps_t* pps = &h->pps;
    slice_t* sh = &h->sh;

    regs->reg68_s_marb_eimi_block = 0;

    regs->reg00_g_enc_mode = REG_ENC_MODE_H264;
    regs->reg01_g_pic_width = h->i_picture_w;
    regs->reg02_g_pic_height = h->i_picture_h;
    // qtable
    regs->reg03_g_qmode = 0;
    // frame coding only
    regs->reg03_g_fldpic_en = 0;
    // mdc
    regs->reg29_s_mbr_qp_cidx_offset = 0;
    // pre-fetch
    regs->reg68_s_prfh_cryc_en = 1;
    regs->reg68_s_prfh_refy_en = 1;
    regs->reg6d_s_prfh_cryc_idle_cnt = 0;
    regs->reg6d_s_prfh_refy_idle_cnt = 0;
    // NOT SW Buffer Mode
    regs->reg73_g_sw_buffer_mode = 0;
    regs->reg18_g_jpe_buffer_mode = 1;    // frame-mode
    regs->reg18_g_jpe_fsvs_mode = 0;
    regs->reg18_g_viu_soft_rstz = 1;
    // read default value when seq_num==0
    // regs->reg81 = REGR(MFEBASE,0x81,"value");
    // regs->reg68 = REGR(MFEBASE,0x68,"value");

    // clock gating
    regs->reg16 = 0xFFFF;
    // per-frame: ref_frames/frame_type
    regs->reg00_g_frame_type = sh->i_type==SLICE_I?0:sh->i_type==SLICE_P?1:2;
    regs->reg00_g_ref_no = sh->i_num_ref_idx_l0_active==2?1:0;
    // per-frame: slice parameters
    regs->reg39_s_mdc_h264_nal_ref_idc = sh->i_ref_idc;
    regs->reg39_s_mdc_h264_nal_unit_type = sh->b_idr_pic;
    regs->reg39_s_mdc_h264_fnum_bits = sps->i_log2_max_frame_num-5;
    regs->reg39_s_mdc_h264_dbf_control = pps->b_deblocking_filter_control;
    regs->reg39_s_mdc_h264_fnum_value = sh->i_frame_num;
    regs->reg3a_s_mdc_h264_idr_pic_id = sh->i_idr_pic_id;
    regs->reg3a_s_mdc_h264_disable_dbf_idc = sh->i_disable_deblocking_filter_idc;
    regs->reg3a_s_mdc_h264_alpha = sh->i_alpha_c0_offset;
    regs->reg3a_s_mdc_h264_beta = sh->i_beta_offset;
    regs->reg3a_s_mdc_h264_ridx_aor_flag = (sh->i_num_ref_idx_l0_active!=sh->pps->i_num_ref_idx_l0_default_active);
    // per-frame: buffer setting: current/reference/reconstructing
    regs->reg06_g_cur_y_addr_lo = (ushort)(h->input.base_y>> 8);
    regs->reg07_g_cur_y_addr_hi = (ushort)(h->input.base_y>>24);
    regs->reg08_g_cur_c_addr_lo = (ushort)(h->input.base_c>> 8);
    regs->reg09_g_cur_c_addr_hi = (ushort)(h->input.base_c>>24);
    if (sh->i_num_ref_idx_l0_active)
    {
        regs->reg0a_g_ref_y_addr0_lo = (ushort)(h->dpb[0]->base_y>> 8);
        regs->reg0b_g_ref_y_addr0_hi = (ushort)(h->dpb[0]->base_y>>24);
        regs->reg0e_g_ref_c_addr0_lo = (ushort)(h->dpb[0]->base_c>> 8);
        regs->reg0f_g_ref_c_addr0_hi = (ushort)(h->dpb[0]->base_c>>24);
    }
    regs->reg12_g_rec_y_addr_lo = (ushort)(h->recon->base_y>> 8);
    regs->reg13_g_rec_y_addr_hi = (ushort)(h->recon->base_y>>24);
    regs->reg14_g_rec_c_addr_lo = (ushort)(h->recon->base_c>> 8);
    regs->reg15_g_rec_c_addr_hi = (ushort)(h->recon->base_c>>24);
	// Output buffers: Must be 8-byte aligned.
    regs->reg3f_s_bspobuf_hw_en = 0;
	// GN
    regs->reg4c_s_gn_saddr_lo = (ushort)(h->i_base_gn>> 3);
    regs->reg4d_s_gn_saddr_hi = (ushort)(h->i_base_gn>>19);
    regs->reg4d_s_gn_saddr_mode = 0;
    // IMI buffer
    regs->reg68_s_marb_lbwd_mode = 0;
    regs->reg6b_s_marb_imi_saddr_lo = 0;
    regs->reg6c_s_marb_imi_saddr_hi = 0;
    // Motion search
    regs->reg20_s_me_ref_en_mode = (1<<sh->i_num_ref_idx_l0_active)-1;
    // FME
    regs->reg21_s_ime_ime_wait_fme = 1;
    regs->reg25_s_fme_pipeline_on = regs->reg21_s_ime_ime_wait_fme ? 1 : 0;
    // P8x8 BOUND RECT
    regs->reg21_s_ime_boundrect_en = (sps->i_profile_idc==66 && sps->i_level_idc<=30);
    regs->reg21_s_ime_h264_p8x8_ctrl_en = 0;//mfeinfo->i_p8x8_max_count < h->i_mb_wxh ? 1 : 0;

    regs->reg25_s_fme_mode0_refno = 1;
    regs->reg25_s_fme_mode1_refno = 1;
    regs->reg25_s_fme_mode2_refno = 1;
	// Intra update (force disabled)
    regs->reg2f_s_txip_irfsh_en = 0;

    regs->reg2c_s_quan_idx_last = 63;
    if (regs->reg2c_s_quan_idx_last < 63) {
        regs->reg2c_s_quan_idx_swlast = 1;
    }
    else {
        regs->reg2c_s_quan_idx_swlast = 0;
    }

    /* bitrate controller */
    // MBR
    regs->reg00_g_mbr_en = rc->b_mb_layer ? 1 : 0;
    regs->reg26_s_mbr_pqp_dlimit = LEFT_QP_DIFF_LIMIT;
    regs->reg26_s_mbr_uqp_dlimit = TOP_QP_DIFF_LIMIT;
	// er_en
	if (rc->m_nVPMbRow > 0 && rc->m_nVPSize <= 0) {
        regs->reg03_g_er_mode = 0;
    }
	else if (rc->m_nVPMbRow <= 0 && rc->m_nVPSize > 0) {
        regs->reg03_g_er_mode = 1;
    }
	else if (rc->m_nVPMbRow > 0 && rc->m_nVPSize > 0) {
        regs->reg03_g_er_mode = 2;
    }
	else {
        regs->reg03_g_er_mode = 3;
    }

	if (rc->m_nVPMbRow == 0 || rc->m_nVPMbRow == 1) {
        regs->reg03_g_er_mby = 0;
    }
	else if (rc->m_nVPMbRow == 2) {
        regs->reg03_g_er_mby = 1;
    }
	else if (rc->m_nVPMbRow == 4) {
        regs->reg03_g_er_mby = 2;
    }
	else if (rc->m_nVPMbRow == 8) {
        regs->reg03_g_er_mby = 3;
    }
    else if (rc->m_nVPMbRow > 0) {
		MST_ASSERT(0);
    }

    if (regs->reg03_g_er_mode==1 || regs->reg03_g_er_mode==2) {
        regs->reg04_g_er_bs_th = rc->m_nVPSize;
    }

    regs->reg00_g_qscale = sh->i_qp;
    regs->reg27_s_mbr_frame_qstep = rc->m_nFrameQStep;
    regs->reg26_s_mbr_tmb_bits = rc->m_nTargetMbBits;

	MST_ASSERT(rc->m_nMinQStep<(2<<7));
	// QP/QStep: Min, max
    regs->reg2a_s_mbr_qp_min = rc->m_nMinQP;
    regs->reg2a_s_mbr_qp_max = rc->m_nMaxQP;
    regs->reg6e_s_mbr_qstep_min = rc->m_nMinQStep;
    regs->reg6f_s_mbr_qstep_max = rc->m_nMaxQStep;
    /*!bitrate controller */
    regs->reg00_g_rec_en = 1;

    // Frame Coding Only
    regs->reg2a_s_mvdctl_ref0_offset = 0;
    regs->reg2a_s_mvdctl_ref1_offset = 0;
    regs->reg7d_eco_bsp_rdy_fix = 1;

    regs->coded_data = h->code_sh;
    regs->coded_bits = h->bits_sh;
    regs->outbs_addr = h->i_base_bs;
    regs->outbs_size = h->i_size_bs;
}

