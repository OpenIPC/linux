
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#include <ms_platform.h>
#include <ms_msys.h>

#include <mdrv_mvhe_io.h>
#include <mdrv_rqct_io.h>

#include <mvhe_defs.h>

#define _ALIGN(a,b) (((a)+(1<<(b))-1)&(~((1<<(b))-1)))

static int _vhectx_streamon(void* pctx, int on);
static int _vhectx_set_parm(void* pctx, mvhe_parm* parm);
static int _vhectx_get_parm(void* pctx, mvhe_parm* parm);
static int _vhectx_set_ctrl(void* pctx, mvhe_ctrl* ctrl);
static int _vhectx_get_ctrl(void* pctx, mvhe_ctrl* ctrl);
static int _vhectx_enc_pict(void* pctx, mvhe_buff* buff);
static int _vhectx_get_bits(void* pctx, mvhe_buff* buff);
static int _vhectx_compress(void* pctx, mvhe_buff* buff);
static int _vhectx_put_data(void* pctx, mvhe_buff* buff);
static int _vhectx_set_rqcf(void* pctx, rqct_conf* rqcf);
static int _vhectx_get_rqcf(void* pctx, rqct_conf* rqcf);
static void _vhectx_release(void* pctx)
{
    mvhe_ctx* mctx = pctx;
    if (atomic_dec_and_test(&mctx->i_refcnt))
    {
        mhve_ops* mops = mctx->p_handle;
        while (mctx->i_dmems > 0)
            msys_release_dmem(&mctx->m_dmems[--mctx->i_dmems]);
        mutex_lock(&mctx->m_stream);
        mctx->p_handle = NULL;
        if (mops)
        {
            if (mops->release)
                mops->release(mops);
            kfree(mops);
            mctx->p_handle = NULL;
        }
        mutex_unlock(&mctx->m_stream);
#if MVHE_TIMER_SIZE>0
        if (mctx->p_timer)
        {
            kfree(mctx->p_timer);
            mctx->p_timer = NULL;
        }
#endif
        if (mctx->p_usrdt)
        {
            kfree(mctx->p_usrdt);
            mctx->p_usrdt = NULL;
        }
        kfree(mctx);
    }
}
static void _vhectx_adduser(void* pctx)
{
    mvhe_ctx* mctx = pctx;
    atomic_inc(&mctx->i_refcnt);
}

void* mvheops_acquire(int);

mvhe_ctx*
mvhectx_acquire(
    mvhe_dev*   mdev)
{
    mvhe_ctx* mctx = NULL;
    mhve_ops* mops = NULL;

    if (!mdev || !(mctx = kzalloc(sizeof(mvhe_ctx), GFP_KERNEL)))
    {
        printk(KERN_ERR"%s() alloc fail\n", __func__);
        return mctx;
    }

    mctx->p_usrdt = NULL;
    do
    {
        if (!(mops = mvheops_acquire(mdev->i_rctidx)))
            break;
        if (!(mctx->p_usrdt = kzalloc(MVHE_USER_DATA_SIZE, GFP_KERNEL)))
            break;
        mctx->i_usrsz = mctx->i_usrcn = 0;
#if MVHE_TIMER_SIZE>0
        mctx->p_timer = kzalloc(MVHE_TIMER_SIZE, GFP_KERNEL);
        mctx->i_numbr = 0;
#endif
        mutex_init(&mctx->m_stream);
        mutex_init(&mctx->m_encode);
        mctx->i_state = MVHE_CTX_STATE_NULL;
        mctx->release =_vhectx_release;
        mctx->adduser =_vhectx_adduser;
        atomic_set(&mctx->i_refcnt, 1);
        mctx->p_handle = mops;
        mctx->i_strid = STREAM_ID_DEFAULT;
        return mctx;
    }
    while (0);

    if (mctx->p_usrdt)
    {
        kfree(mctx->p_usrdt);
        mctx->p_usrdt = NULL;
    }
    if (mops)
    {
        if (mops->release)
            mops->release(mops);
        kfree(mops);
        mctx->p_handle = NULL;
    }
    if (mctx)
    {
        kfree(mctx);
        mctx = NULL;
    }
    return mctx;
}

long
mvhectx_actions(
    mvhe_ctx*       mctx,
    unsigned int    cmd,
    void*           arg)
{
    int err = 0;
    switch (cmd)
    {
    case IOCTL_MVHE_S_PARM:
        err = _vhectx_set_parm(mctx, (mvhe_parm*)arg);
        break;
    case IOCTL_MVHE_G_PARM:
        err = _vhectx_get_parm(mctx, (mvhe_parm*)arg);
        break;
    case IOCTL_MVHE_STREAMON:
        err = _vhectx_streamon(mctx, 1);
        break;
    case IOCTL_MVHE_STREAMOFF:
        err = _vhectx_streamon(mctx, 0);
        break;
    case IOCTL_MVHE_S_PICT:
        err = _vhectx_enc_pict(mctx, (mvhe_buff*)arg);
        break;
    case IOCTL_MVHE_G_BITS:
        err = _vhectx_get_bits(mctx, (mvhe_buff*)arg);
        break;
    case IOCTL_MVHE_S_CTRL:
        err = _vhectx_set_ctrl(mctx, (mvhe_ctrl*)arg);
        break;
    case IOCTL_MVHE_G_CTRL:
        err = _vhectx_get_ctrl(mctx, (mvhe_ctrl*)arg);
        break;
    case IOCTL_MVHE_ENCODE:
        err = _vhectx_compress(mctx, (mvhe_buff*)arg);
        break;
    case IOCTL_MVHE_S_DATA:
        err = _vhectx_put_data(mctx, (mvhe_buff*)arg);
        break;
    case IOCTL_RQCT_S_CONF:
        err = _vhectx_set_rqcf(mctx, (rqct_conf*)arg);
        break;
    case IOCTL_RQCT_G_CONF:
        err = _vhectx_get_rqcf(mctx, (rqct_conf*)arg);
        break;
    default:
        err = -EINVAL;
        break;
    }
    return err;
}

#define MVHE_COEF_LEN  (36*1024)
#define MVHE_NALU_LEN  ( 4*1024)
#define MVHE_MISC_LEN  (MVHE_COEF_LEN+MVHE_NALU_LEN)

static int
_vhectx_streamon(
    void*           pctx,
    int             on)
{
    mvhe_ctx* mctx = pctx;
    mvhe_dev* mdev = mctx->p_device;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    mhve_cfg mcfg;
    rqct_cfg rqcf;
    int i, err = 0;

    mutex_lock(&mctx->m_stream);
    do
    if (on)
    {
        int pixels_size, luma4n_size, chroma_size, output_size;
        int compr_ysize, compr_csize, rcbmap_size, roimap_size;
        int rpbuff_size;
        int outsize;
        int score, rpbn, maxw, maxh;
        uint  addr;
        char* kptr;
        msys_mem* pdmem;
        compr_ysize = compr_csize = 0;
        if (MVHE_CTX_STATE_NULL != mctx->i_state)
            break;
        mctx->i_dmems = 0;
        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        maxw = mctx->i_max_w;
        maxh = mctx->i_max_h;
        pixels_size = maxw*maxh;
        luma4n_size = pixels_size/4;
        chroma_size = pixels_size/2;
        rcbmap_size = pixels_size/2048;
        rcbmap_size = _ALIGN(rcbmap_size, 4);
#if defined(ROIMAP)
        roimap_size = pixels_size/128;
        roimap_size = _ALIGN(roimap_size, 4);
#else
        roimap_size = 0;
#endif
        output_size = _ALIGN(pixels_size,19);
        if (mcfg.res.u_conf&MHVE_CFG_OMMAP)
            output_size = 0;
        /* Calculate compression buffer size */
        if (mcfg.res.u_conf&MHVE_CFG_COMPR)
        {
            compr_ysize = _ALIGN((maxw*maxh)/512,4);
            compr_csize = (_ALIGN(maxw,8)*maxh)/128;
        }
        score = pixels_size >> 12;
        /* shrink mode luma and chroma buffer buffer set from user */
        if (mctx->i_imode == MVHE_IMODE_PLUS)
            pixels_size = chroma_size = 0;
        rpbn = mcfg.res.i_rpbn;
        rpbuff_size = pixels_size + luma4n_size + chroma_size + compr_ysize + compr_csize;
        do
        {
            /* config output buffer */
            outsize = output_size + MVHE_MISC_LEN + rcbmap_size*2+roimap_size;
            pdmem = &mctx->m_dmems[mctx->i_dmems++];

            if (mctx->i_strid == STREAM_ID_DEFAULT)
            {
                snprintf(pdmem->name, 15, "S%d:VHEDMOUT", mctx->i_index);
            }
            else
            {
                /* encoder buffer name alignment */
                snprintf(pdmem->name, 15, "S%d:VENCDMOUT", mctx->i_strid);
            }
            pdmem->length = outsize;
            if (0 != (err = msys_request_dmem(pdmem)))
                break;
            addr = Chip_Phys_to_MIU(pdmem->phys);
            kptr = (char*)((uintptr_t)pdmem->kvirt);
            mctx->i_omode = MVHE_OMODE_MMAP;
            mctx->p_ovptr = NULL;
            mctx->u_ophys = 0;
            mctx->i_osize = mctx->i_ormdr = 0;
            if (output_size > 0)
            {
                mcfg.type = MHVE_CFG_DMA;
                mcfg.dma.i_dmem = MHVE_CFG_DMA_OUTPUT_BUFFER;
                mcfg.dma.p_vptr = kptr;
                mcfg.dma.u_phys = addr;
                mcfg.dma.i_size[0] = output_size;
                mops->set_conf(mops, &mcfg);
                mctx->i_omode = MVHE_OMODE_USER;
                mctx->p_ovptr = kptr;
                mctx->u_ophys = addr;
                mctx->i_osize = output_size;
                kptr += output_size;
                addr += output_size;
            }
            /* config NALU buffer */
            mcfg.type = MHVE_CFG_DMA;
            mcfg.dma.i_dmem = MHVE_CFG_DMA_NALU_BUFFER;
            mcfg.dma.p_vptr = kptr;
            mcfg.dma.u_phys = addr;
            mcfg.dma.i_size[0] = MVHE_NALU_LEN;
            mcfg.dma.i_size[1] = compr_ysize?MVHE_COEF_LEN:0;
            mops->set_conf(mops, &mcfg);
            kptr += MVHE_MISC_LEN;
            addr += MVHE_MISC_LEN;
            rqcf.type = RQCT_CFG_RCM;
            rqcf.rcm.p_kptr = kptr;
            rqcf.rcm.u_phys = addr;
            rqcf.rcm.i_size = rcbmap_size;
            rqct->set_rqcf(rqct, &rqcf);
            kptr += rcbmap_size*2;
            addr += rcbmap_size*2;
            rqcf.type = RQCT_CFG_DQM;
#if defined(ROIMAP)
            mcfg.type = MHVE_CFG_HEV;
            mops->get_conf(mops, &mcfg);
            rqcf.dqm.i_unit = 8>>(3&(mcfg.hev.b_ctu_qp_delta_enable-1));
            rqcf.dqm.p_kptr = kptr;
            rqcf.dqm.u_phys = addr;
            rqcf.dqm.i_size = roimap_size;
            rqcf.dqm.i_dqmw = mctx->i_max_w>>3;
            rqcf.dqm.i_dqmh = mctx->i_max_h>>3;
#else
            rqcf.dqm.i_unit = 1;
            rqcf.dqm.i_dqmw = mctx->i_max_w>>6;
            rqcf.dqm.i_dqmh = mctx->i_max_h>>6;
#endif
            rqct->set_rqcf(rqct, &rqcf);

            /* config reconstruct/reference buffer */
            for (i = 0; i < rpbn; i++)
            {
                pdmem = &mctx->m_dmems[mctx->i_dmems++];
                pdmem->length = rpbuff_size;
                if (mctx->i_strid == STREAM_ID_DEFAULT)
                {
                    snprintf(pdmem->name, 15, "S%d:VHEDMRP%d", mctx->i_index, i);
                }
                else
                {
                    /* encoder buffer name alignment */
                    snprintf(pdmem->name, 15, "S%d:VENCDMP%d", mctx->i_strid, i);
                }
                if (0 != (err = msys_request_dmem(pdmem)))
                    break;
                addr = Chip_Phys_to_MIU(pdmem->phys);
                kptr = (char*)((uintptr_t)pdmem->kvirt);
                mcfg.type = MHVE_CFG_DMA;
                mcfg.dma.i_dmem = i;
                mcfg.dma.u_phys = addr;
                mcfg.dma.p_vptr = kptr;
                mcfg.dma.i_size[0] = pixels_size;
                mcfg.dma.i_size[1] = chroma_size;
                mcfg.dma.i_size[2] = luma4n_size;
                mcfg.dma.i_size[3] = compr_ysize;
                mcfg.dma.i_size[4] = compr_csize;
                mops->set_conf(mops, &mcfg);
            }
            rqcf.type = RQCT_CFG_FPS;
            rqct->get_rqcf(rqct, &rqcf);
            score *= (int)rqcf.fps.n_fps;
            score /= (int)rqcf.fps.d_fps;
            mctx->i_score = score;
        }
        while (0);

        if (!err && !(err = mops->seq_sync(mops)))
        {
            mops->seq_conf(mops);
            mvhedev_poweron(mdev, mctx->i_score);
            mdev->i_counts[mctx->i_index][0] = mdev->i_counts[mctx->i_index][1] = 0;
            mdev->i_counts[mctx->i_index][2] = mdev->i_counts[mctx->i_index][3] = 0;
            mdev->i_counts[mctx->i_index][4] = 0;
            mctx->i_state = MVHE_CTX_STATE_IDLE;
            break;
        }
    }
    else
    {
        if (MVHE_CTX_STATE_NULL == mctx->i_state)
            break;
        mvhedev_poweron(mdev,-mctx->i_score);
        mctx->i_state = MVHE_CTX_STATE_NULL;
        mctx->i_score = 0;
        mops->seq_done(mops);
#if 0
        printk("<%d>vhe performance:\n",mctx->i_index);
        for (i = 0; i < MVHE_TIMER_SIZE/8; i++)
            printk("<%d>%4d/%4d/%8d\n",mctx->i_index,mctx->p_timer[i].tm_dur[0],mctx->p_timer[i].tm_dur[1],mctx->p_timer[i].tm_cycles);
#endif
    }
    while (0);
    if (!on || err)
    {
        while (mctx->i_dmems > 0)
            msys_release_dmem(&mctx->m_dmems[--mctx->i_dmems]);
    }
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_vhectx_set_parm(
    void*           pctx,
    mvhe_parm*      parm)
{
    mvhe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    int err = -EINVAL;

    mutex_lock(&mctx->m_stream);
    if (MVHE_CTX_STATE_NULL == mctx->i_state)
    {
        mhve_cfg mcfg;
        rqct_cfg rqcf;
        unsigned char b_long_term_reference;
        switch (parm->type)
        {
        case MVHE_PARM_RES:
            /* check LTR mode */
            mcfg.type = MHVE_CFG_LTR;
            err = mops->get_conf(mops, &mcfg);
            b_long_term_reference = mcfg.ltr.b_long_term_reference;
            if ((unsigned)parm->res.i_pixfmt > MVHE_PIXFMT_YUYV)
                break;
            if ((parm->res.i_pict_w%16) || (parm->res.i_pict_h%8))
                break;
            mctx->i_max_w = _ALIGN(parm->res.i_pict_w,6);
            mctx->i_max_h = _ALIGN(parm->res.i_pict_h,6);
            //TODO: add stream id
            mctx->i_strid = parm->res.i_strid;
            mcfg.type = MHVE_CFG_RES;
            mcfg.res.e_pixf = parm->res.i_pixfmt;
            mcfg.res.i_pixw = parm->res.i_pict_w;
            mcfg.res.i_pixh = parm->res.i_pict_h;
            /* recn buffer number depend on LTR mode */
            if (b_long_term_reference)
                mcfg.res.i_rpbn = 3;
            else
                mcfg.res.i_rpbn = 2;
            mcfg.res.u_conf = 0;
            if (parm->res.i_flags&MVHE_FLAGS_COMPR)
                mcfg.res.u_conf |= MHVE_CFG_COMPR;
            if (parm->res.i_outlen < 0)
                mcfg.res.u_conf |= MHVE_CFG_OMMAP;
            if (parm->res.i_outlen <-1)
                mctx->i_imode = MVHE_IMODE_PLUS;
            mops->set_conf(mops, &mcfg);
            rqcf.type = RQCT_CFG_RES;
            rqcf.res.i_picw = parm->res.i_pict_w;
            rqcf.res.i_pich = parm->res.i_pict_h;
            rqct->set_rqcf(rqct, &rqcf);
            err = 0;
            break;
        case MVHE_PARM_FPS:
            if (parm->fps.i_den == 0 || parm->fps.i_num == 0)
                break;
            rqcf.type = RQCT_CFG_FPS;
            rqcf.fps.n_fps = (short)parm->fps.i_num;
            rqcf.fps.d_fps = (short)parm->fps.i_den;
            err = rqct->set_rqcf(rqct, &rqcf);
            break;
        case MVHE_PARM_GOP:
            if (parm->gop.i_pframes >= 0)
            {
                rqcf.type = RQCT_CFG_SEQ;
                if (!rqct->get_rqcf(rqct, &rqcf))
                {
                    rqcf.seq.i_period = parm->gop.i_pframes+1;
                    if (!rqct->set_rqcf(rqct, &rqcf))
                    {
                        parm->gop.i_bframes = 0;
                        err = 0;
                    }
                }
            }
            break;
        case MVHE_PARM_BPS:
            if ((unsigned)parm->bps.i_method <= RQCT_METHOD_VBR)
            {
                rqcf.type = RQCT_CFG_SEQ;
                if (!rqct->get_rqcf(rqct, &rqcf))
                {
                    rqcf.seq.i_method = parm->bps.i_method;
                    rqcf.seq.i_leadqp = parm->bps.i_ref_qp;
                    rqcf.seq.i_btrate = parm->bps.i_bps;
                    err = rqct->set_rqcf(rqct, &rqcf);
                }
            }
            break;
        case MVHE_PARM_HEVC:
            if ((parm->hevc.i_log2_max_cb_size-3) > 3 ||
                (parm->hevc.i_log2_min_cb_size-3) > 3 ||
                parm->hevc.i_log2_max_cb_size < parm->hevc.i_log2_min_cb_size)
                break;
            if ((parm->hevc.i_log2_max_tr_size-2) > 3 ||
                (parm->hevc.i_log2_min_tr_size-2) > 3 ||
                parm->hevc.i_log2_max_tr_size < parm->hevc.i_log2_min_tr_size ||
                parm->hevc.i_log2_max_tr_size < parm->hevc.i_log2_min_cb_size)
                break;
            if (parm->hevc.i_tr_depth_intra > (parm->hevc.i_log2_max_cb_size-parm->hevc.i_log2_min_tr_size) ||
                parm->hevc.i_tr_depth_inter > (parm->hevc.i_log2_max_cb_size-parm->hevc.i_log2_min_tr_size))
                break;
            if ((unsigned)(parm->hevc.i_tc_offset_div2+6) > 12 ||
                (unsigned)(parm->hevc.i_beta_offset_div2+6) > 12 ||
                (unsigned)(parm->hevc.i_cqp_offset+12) > 24)
                break;
            mcfg.type = MHVE_CFG_HEV;
            mcfg.hev.i_profile = parm->hevc.i_profile;
            mcfg.hev.i_level = parm->hevc.i_level;
            mcfg.hev.i_log2_max_cb_size = parm->hevc.i_log2_max_cb_size;
            mcfg.hev.i_log2_min_cb_size = parm->hevc.i_log2_min_cb_size;
            mcfg.hev.i_log2_max_tr_size = parm->hevc.i_log2_max_tr_size;
            mcfg.hev.i_log2_min_tr_size = parm->hevc.i_log2_min_tr_size;
            mcfg.hev.i_tr_depth_intra = parm->hevc.i_tr_depth_intra;
            mcfg.hev.i_tr_depth_inter = parm->hevc.i_tr_depth_inter;
            mcfg.hev.b_scaling_list_enable = parm->hevc.b_scaling_list_enable;
            mcfg.hev.b_ctu_qp_delta_enable = parm->hevc.b_ctu_qp_delta_enable;
            mcfg.hev.b_sao_enable = parm->hevc.b_sao_enable;
            mcfg.hev.i_cqp_offset = parm->hevc.i_cqp_offset;
            mcfg.hev.b_strong_intra_smoothing = parm->hevc.b_strong_intra_smoothing;
            mcfg.hev.b_constrained_intra_pred = parm->hevc.b_constrained_intra_pred;
            mcfg.hev.b_deblocking_override_enable = parm->hevc.b_deblocking_override_enable;
            mcfg.hev.b_deblocking_disable = parm->hevc.b_deblocking_disable;
            mcfg.hev.i_tc_offset_div2 = parm->hevc.i_tc_offset_div2;
            mcfg.hev.i_beta_offset_div2 = parm->hevc.i_beta_offset_div2;
            err = mops->set_conf(mops, &mcfg);
            break;
        case MVHE_PARM_VUI:
            mcfg.type = MHVE_CFG_VUI;
            mcfg.vui.b_video_full_range = parm->vui.b_video_full_range!=0;
            mcfg.vui.b_timing_info_pres = parm->vui.b_timing_info_pres!=0;
            err = mops->set_conf(mops, &mcfg);
            break;
        case MVHE_PARM_LTR:
            /* OPs set LTR mode */
            mcfg.type = MHVE_CFG_LTR;
            mcfg.ltr.b_long_term_reference = parm->ltr.b_long_term_reference;
            mcfg.ltr.b_enable_pred = parm->ltr.b_enable_pred;
            err = mops->set_conf(mops, &mcfg);
            b_long_term_reference = mcfg.ltr.b_long_term_reference;
            /* Set recn buffer depend on LTR mode */
            mcfg.type = MHVE_CFG_RES;
            if (!mops->get_conf(mops, &mcfg))
            {
                mcfg.res.i_rpbn = b_long_term_reference ? 3 : 2;
                err = mops->set_conf(mops, &mcfg);
            }
            /* RQCT control LTR P-frame period */
            rqcf.type = RQCT_CFG_LTR;
            if (!rqct->get_rqcf(rqct, &rqcf))
            {
                rqcf.ltr.i_period = b_long_term_reference ? parm->ltr.i_ltr_period : 0;
                err = rqct->set_rqcf(rqct, &rqcf);
            }
            break;
        default:
            break;
        }
    }
    mutex_unlock(&mctx->m_stream);
    return err;
}

static int
_vhectx_get_parm(
    void*           pctx,
    mvhe_parm*      parm)
{
    int err = 0;
    mvhe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    mhve_cfg  mcfg;
    rqct_cfg  rqcf;

    mutex_lock(&mctx->m_stream);
    switch (parm->type)
    {
    case MVHE_PARM_IDX:
        parm->idx.i_stream = mctx->i_index;
        break;
    case MVHE_PARM_RES:
        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        parm->res.i_pict_w = mcfg.res.i_pixw;
        parm->res.i_pict_h = mcfg.res.i_pixh;
        parm->res.i_strid  = mctx->i_strid;
        parm->res.i_pixfmt = mcfg.res.e_pixf;
        parm->res.i_outlen = 0;
        parm->res.i_flags = (mcfg.res.u_conf&MHVE_CFG_COMPR)?MVHE_FLAGS_COMPR:0;
        break;
    case MVHE_PARM_FPS:
        rqcf.type = RQCT_CFG_FPS;
        if (!(err = rqct->get_rqcf(rqct, &rqcf)))
        {
            parm->fps.i_den = rqcf.fps.d_fps;
            parm->fps.i_num = rqcf.fps.n_fps;
        }
        break;
    case MVHE_PARM_GOP:
    case MVHE_PARM_BPS:
        rqcf.type = RQCT_CFG_SEQ;
        if (!(err = rqct->get_rqcf(rqct, &rqcf)))
        {
            if (parm->type == MVHE_PARM_BPS)
            {
                parm->bps.i_method = rqcf.seq.i_method;
                parm->bps.i_ref_qp = rqcf.seq.i_leadqp;
                parm->bps.i_bps = rqcf.seq.i_btrate;
                break;
            }
            parm->gop.i_pframes = rqcf.seq.i_period-1;
            parm->gop.i_bframes = 0; /* not support b-frame */
        }
        break;
    case MVHE_PARM_HEVC:
        mcfg.type = MHVE_CFG_HEV;
        if (0 != (err = mops->get_conf(mops, &mcfg)))
            break;
        parm->hevc.i_profile = mcfg.hev.i_profile;
        parm->hevc.i_level = mcfg.hev.i_level;
        parm->hevc.i_log2_max_cb_size = mcfg.hev.i_log2_max_cb_size;
        parm->hevc.i_log2_min_cb_size = mcfg.hev.i_log2_min_cb_size;
        parm->hevc.i_log2_max_tr_size = mcfg.hev.i_log2_max_tr_size;
        parm->hevc.i_log2_min_tr_size = mcfg.hev.i_log2_min_tr_size;
        parm->hevc.i_tr_depth_intra = mcfg.hev.i_tr_depth_intra;
        parm->hevc.i_tr_depth_inter = mcfg.hev.i_tr_depth_inter;
        parm->hevc.b_scaling_list_enable = mcfg.hev.b_scaling_list_enable;
        parm->hevc.b_ctu_qp_delta_enable = mcfg.hev.b_ctu_qp_delta_enable;
        parm->hevc.b_sao_enable = mcfg.hev.b_sao_enable;
        parm->hevc.b_strong_intra_smoothing = mcfg.hev.b_strong_intra_smoothing;
        parm->hevc.b_constrained_intra_pred = mcfg.hev.b_constrained_intra_pred;
        parm->hevc.b_deblocking_disable = mcfg.hev.b_deblocking_disable;
        parm->hevc.b_deblocking_override_enable = mcfg.hev.b_deblocking_override_enable;
        parm->hevc.i_tc_offset_div2 = mcfg.hev.i_tc_offset_div2;
        parm->hevc.i_beta_offset_div2 = mcfg.hev.i_beta_offset_div2;
        parm->hevc.i_cqp_offset = mcfg.hev.i_cqp_offset;
        break;
    case MVHE_PARM_VUI:
        mcfg.type = MHVE_CFG_VUI;
        if (!(err = mops->get_conf(mops, &mcfg)))
        {
            parm->vui.b_video_full_range = 0!=mcfg.vui.b_video_full_range;
            parm->vui.b_timing_info_pres = 0!=mcfg.vui.b_timing_info_pres;
        }
        break;
    case MVHE_PARM_LTR:
        mcfg.type = MHVE_CFG_LTR;
        if (!(err = mops->get_conf(mops, &mcfg)))
        {
            parm->ltr.b_long_term_reference = mcfg.ltr.b_long_term_reference;
            if (mcfg.ltr.b_long_term_reference)
            {
                parm->ltr.b_enable_pred = mcfg.ltr.b_enable_pred;
                rqcf.type = RQCT_CFG_LTR;
                if (!rqct->get_rqcf(rqct, &rqcf))
                {
                    parm->ltr.i_ltr_period = rqcf.ltr.i_period;
                }
            }
            else
            {
                parm->ltr.i_ltr_period = 0;
                parm->ltr.b_enable_pred = 0;
            }
        }
        break;
    default:
        err = -EINVAL;
        break;
    }
    mutex_unlock(&mctx->m_stream);
    return err;
}

static int
_vhectx_set_ctrl(
    void*       pctx,
    mvhe_ctrl*  ctrl)
{
    int err = -EINVAL;
    mvhe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    rqct_cfg rqcf;
    mhve_cfg mcfg;

    mutex_lock(&mctx->m_stream);
    if (MVHE_CTX_STATE_NULL < mctx->i_state)
    switch (ctrl->type)
    {
    case MVHE_CTRL_ROI:
        mcfg.type = MHVE_CFG_HEV;
        mops->get_conf(mops, &mcfg);
        if (!mcfg.hev.b_ctu_qp_delta_enable)
            break;
        rqcf.type = RQCT_CFG_ROI;
        rqcf.roi.i_roiidx = ctrl->roi.i_index;
        rqcf.roi.i_roidqp = ctrl->roi.i_dqp;
        rqcf.roi.i_posx = ctrl->roi.i_cbx;
        rqcf.roi.i_posy = ctrl->roi.i_cby;
        rqcf.roi.i_recw = ctrl->roi.i_cbw;
        rqcf.roi.i_rech = ctrl->roi.i_cbh;
        err = rqct->set_rqcf(rqct, &rqcf);
        break;
    case MVHE_CTRL_SPL:
        mcfg.type = MHVE_CFG_SPL;
        mcfg.spl.i_rows = ctrl->spl.i_rows;
        mcfg.spl.i_bits = 0;
        err = mops->set_conf(mops, &mcfg);
        break;
    case MVHE_CTRL_BAC:
        mcfg.type = MHVE_CFG_BAC;
        mcfg.bac.b_init = ctrl->bac.b_init!=0;
        err = mops->set_conf(mops, &mcfg);
        break;
    case MVHE_CTRL_DBK:
        mcfg.type = MHVE_CFG_LFT;
        mcfg.lft.b_override = ctrl->dbk.b_override;
        mcfg.lft.b_disable = ctrl->dbk.b_disable;
        mcfg.lft.i_offsetA = ctrl->dbk.i_tc_offset_div2;
        mcfg.lft.i_offsetB = ctrl->dbk.i_beta_offset_div2;
        err = mops->set_conf(mops, &mcfg);
        break;
    case MVHE_CTRL_SEQ:
        if ((unsigned)ctrl->seq.i_pixfmt <= MVHE_PIXFMT_YUYV &&
            ctrl->seq.i_pixelw >= 128 &&
            ctrl->seq.i_pixelh >= 128 &&
            ctrl->seq.d_fps > 0 &&
            ctrl->seq.n_fps > 0 &&
            (ctrl->seq.i_pixelw*ctrl->seq.i_pixelh) <= (mctx->i_max_w*mctx->i_max_h))
        {
            int dqmw, dqmh, size = 0;
            mcfg.type = MHVE_CFG_RES;
            mops->get_conf(mops, &mcfg);
            mcfg.res.e_pixf = ctrl->seq.i_pixfmt;
            mcfg.res.i_pixw = ctrl->seq.i_pixelw;
            mcfg.res.i_pixh = ctrl->seq.i_pixelh;
            if ((err = mops->set_conf(mops, &mcfg)))
                break;
            dqmw = _ALIGN(mcfg.res.i_pixw,6)>>3;
            dqmh = _ALIGN(mcfg.res.i_pixh,6)>>3;
            size = dqmw*dqmh/2;
            rqcf.type = RQCT_CFG_FPS;
            rqcf.fps.n_fps = ctrl->seq.n_fps;
            rqcf.fps.d_fps = ctrl->seq.d_fps;
            if ((err = rqct->set_rqcf(rqct, &rqcf)))
                break;
            rqcf.type = RQCT_CFG_RES;
            rqcf.res.i_picw = ctrl->seq.i_pixelw;
            rqcf.res.i_pich = ctrl->seq.i_pixelh;
            if ((err = rqct->set_rqcf(rqct, &rqcf)))
                break;
            /* disable ROI */
            rqcf.type = RQCT_CFG_ROI;
            rqcf.roi.i_roiidx = -1;
            rqct->set_rqcf(rqct, &rqcf);
            /* reset dqm */
            rqcf.type = RQCT_CFG_DQM;
            rqct->get_rqcf(rqct, &rqcf);
#if defined(ROIMAP)
            mcfg.type = MHVE_CFG_HEV;
            mops->get_conf(mops, &mcfg);
            rqcf.dqm.i_unit = 8>>(3&(mcfg.hev.b_ctu_qp_delta_enable-1));
            rqcf.dqm.i_dqmw = dqmw;
            rqcf.dqm.i_dqmh = dqmh;
            rqcf.dqm.i_size = size;
#else
            rqcf.dqm.i_dqmw = dqmw>>3;
            rqcf.dqm.i_dqmh = dqmh>>3;
            rqcf.dqm.i_size = 0;
            rqcf.dqm.i_unit = 1;
#endif
            rqct->set_rqcf(rqct, &rqcf);

            mcfg.type = MHVE_CFG_DMA;
            mcfg.dma.i_dmem = MHVE_CFG_DMA_RESET_RECN_BUFFER;
            mops->set_conf(mops, &mcfg);
        }
        break;
    case MVHE_CTRL_LTR:
        /* MUST set param first(can't open LTR mode after streamon) */
        mcfg.type = MHVE_CFG_LTR;
        err = mops->get_conf(mops, &mcfg);
        /* RQCT control LTR P-frame period */
        if (mcfg.ltr.b_long_term_reference)
        {
            mcfg.ltr.b_enable_pred = ctrl->ltr.b_enable_pred;
            err = mops->set_conf(mops, &mcfg);
            rqcf.type = RQCT_CFG_LTR;
            if (!rqct->get_rqcf(rqct, &rqcf))
            {
                rqcf.ltr.i_period = ctrl->ltr.i_ltr_period;
                if (!rqct->set_rqcf(rqct, &rqcf))
                    err = 0;
            }
        }
        err = 0;
        break;
    default:
        break;
    }
    mutex_unlock(&mctx->m_stream);
    return err;
}

static int
_vhectx_get_ctrl(
    void*       pctx,
    mvhe_ctrl*  ctrl)
{
    int err = -EINVAL;
    mvhe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    rqct_cfg rqcf;
    mhve_cfg mcfg;

    mutex_lock(&mctx->m_stream);
    if (MVHE_CTX_STATE_NULL < mctx->i_state)
    switch (ctrl->type)
    {
    case MVHE_CTRL_ROI:
        rqcf.type = RQCT_CFG_ROI;
        rqcf.roi.i_roiidx = ctrl->roi.i_index;
        if (!(err = rqct->get_rqcf(rqct, &rqcf)))
        {
            ctrl->roi.i_dqp = rqcf.roi.i_roidqp;
            ctrl->roi.i_cbx = rqcf.roi.i_posx;
            ctrl->roi.i_cby = rqcf.roi.i_posy;
            ctrl->roi.i_cbw = rqcf.roi.i_recw;
            ctrl->roi.i_cbh = rqcf.roi.i_rech;
        }
        break;
    case MVHE_CTRL_SPL:
        mcfg.type = MHVE_CFG_SPL;
        if (!(err = mops->get_conf(mops, &mcfg)))
        {
            ctrl->spl.i_rows = mcfg.spl.i_rows;
            ctrl->spl.i_bits = 0;
        }
        break;
    case MVHE_CTRL_BAC:
        mcfg.type = MHVE_CFG_BAC;
        if (!(err = mops->get_conf(mops, &mcfg)))
            ctrl->bac.b_init = mcfg.bac.b_init;
        break;
    case MVHE_CTRL_DBK:
        mcfg.type = MHVE_CFG_LFT;
        if (!(err = mops->get_conf(mops, &mcfg)))
        {
            ctrl->dbk.b_override = (short)mcfg.lft.b_override;
            ctrl->dbk.b_disable  = (short)mcfg.lft.b_disable;
            ctrl->dbk.i_tc_offset_div2   = (short)mcfg.lft.i_offsetA;
            ctrl->dbk.i_beta_offset_div2 = (short)mcfg.lft.i_offsetB;
        }
        break;
    case MVHE_CTRL_SEQ:
        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        ctrl->seq.i_pixelw = mcfg.res.i_pixw;
        ctrl->seq.i_pixelh = mcfg.res.i_pixh;
        ctrl->seq.i_pixfmt = mcfg.res.e_pixf;
        rqcf.type = RQCT_CFG_FPS;
        rqct->get_rqcf(rqct, &rqcf);
        ctrl->seq.n_fps = (int)rqcf.fps.n_fps;
        ctrl->seq.d_fps = (int)rqcf.fps.d_fps;
        err = 0;
        break;
    case MVHE_CTRL_LTR:
        mcfg.type = MHVE_CFG_LTR;
        err = mops->get_conf(mops, &mcfg);
        /* RQCT control LTR P-frame period */
        if (mcfg.ltr.b_long_term_reference)
        {
            ctrl->ltr.b_enable_pred = mcfg.ltr.b_enable_pred;
            rqcf.type = RQCT_CFG_LTR;
            if (!rqct->get_rqcf(rqct, &rqcf))
            {
                ctrl->ltr.i_ltr_period = rqcf.ltr.i_period;
            }
        }
        else
        {
            ctrl->ltr.b_enable_pred = 0;
            ctrl->ltr.i_ltr_period = 0;
        }
        err = 0;
        break;
    default:
        break;
    }
    mutex_unlock(&mctx->m_stream);

    return err;
}

#define DISABLE_DISPOSABLE  0
#if DISABLE_DISPOSABLE
#define MVHE_FLAGS_CONTROL  (MVHE_FLAGS_IDR|MVHE_FLAGS_NIGHT_MODE)
#else
#define MVHE_FLAGS_CONTROL  (MVHE_FLAGS_IDR|MVHE_FLAGS_DISPOSABLE|MVHE_FLAGS_NIGHT_MODE)
#endif

static int
_vhectx_enc_pict(
    void*           pctx,
    mvhe_buff*      buff)
{
    int err = -EINVAL;
    int pitch = 0;
    mvhe_ctx* mctx = pctx;
    mvhe_dev* mdev = mctx->p_device;
    mhve_ops* mops = mctx->p_handle;
    mhve_cpb* pcpb = mctx->m_mcpbs;

    if (buff->i_memory != MVHE_MEMORY_MMAP)
        return -EINVAL;
    pitch = buff->i_stride;
    if (pitch < buff->i_width)
        pitch = buff->i_width;

    mutex_lock(&mctx->m_stream);
    do
    if (MVHE_CTX_STATE_IDLE == mctx->i_state && MVHE_OMODE_USER == mctx->i_omode)
    {
        mhve_vpb mvpb;
        mhve_cfg mcfg;

        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        if (buff->i_planes != 2 && (MHVE_PIX_NV21 >= mcfg.res.e_pixf))
            break;
        if (buff->i_planes != 1 && (MHVE_PIX_YUYV == mcfg.res.e_pixf))
            break;
        if (mcfg.res.i_pixw != buff->i_width || mcfg.res.i_pixh != buff->i_height)
            break;

        mvpb.i_index = buff->i_index;
        mvpb.u_flags = buff->i_flags&MVHE_FLAGS_CONTROL;
        mvpb.i_stamp = buff->i_timecode;
        mvpb.i_pitch = pitch;
        mvpb.planes[1].u_phys = 0;
        mvpb.planes[1].i_bias = 0;
        mvpb.planes[0].u_phys = buff->planes[0].mem.phys;
        mvpb.planes[0].i_bias = buff->planes[0].i_bias;
        if (mcfg.res.e_pixf <= MHVE_PIX_NV12)
        {
            mvpb.planes[1].u_phys = buff->planes[1].mem.phys;
            mvpb.planes[1].i_bias = buff->planes[1].i_bias;
        }

        mctx->i_state = MVHE_CTX_STATE_BUSY;
        if (!(err = mops->enc_buff(mops, &mvpb)))
        do
        {
            mops->put_data(mops, mctx->p_usrdt, mctx->i_usrsz);
            mops->enc_conf(mops);
            mvhedev_pushjob(mdev, mctx);
        }
        while (0 < (err = mops->enc_done(mops)));

        if (!err)
        {
            mctx->i_usrcn = mctx->i_usrsz = 0;
            pcpb->i_index = -1;
        }

        mctx->i_ormdr =
        err =
        mops->out_buff(mops, pcpb);
    }
    while (0);
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_vhectx_get_bits(
    void*           pctx,
    mvhe_buff*      buff)
{
    mvhe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    int err = -ENODATA;

    if (buff->i_memory != MVHE_MEMORY_USER || buff->i_planes != 1)
        return -EINVAL;

    mutex_lock(&mctx->m_stream);
    if (MVHE_CTX_STATE_BUSY == mctx->i_state && MVHE_OMODE_USER == mctx->i_omode)
    {
        int flags = 0;
        int  rmdr = mctx->i_ormdr;
        int  size = mctx->m_mcpbs->planes[0].i_size;
        char* ptr = mctx->p_ovptr + size - rmdr;

        if (rmdr == size)
            flags = MVHE_FLAGS_SOP;
        if (buff->planes[0].i_size < rmdr)
            rmdr = buff->planes[0].i_size;
        buff->i_flags = 0;
        if (copy_to_user(buff->planes[0].mem.uptr, ptr, rmdr))
            err = -EFAULT;
        else
        {
            buff->planes[0].i_used = rmdr;
            buff->i_timecode = mctx->m_mcpbs->i_stamp;
            mctx->i_ormdr -= rmdr;
            err = 0;
        }
        if (0 == mctx->i_ormdr)
        {
            mctx->i_state = MVHE_CTX_STATE_IDLE;
            flags |= MVHE_FLAGS_EOP;
            mops->out_buff(mops, mctx->m_mcpbs);
        }
        if (!err)
            buff->i_flags = flags|(mctx->m_mcpbs->i_flags&MVHE_FLAGS_CONTROL);
    }
    mutex_unlock(&mctx->m_stream);

    return err;
}

extern int msys_find_dmem_by_phys(unsigned long long phys, msys_mem* pdmem);

static void*
phys2kptr(
    unsigned long long  phys)
{
    msys_mem dmem;
    char* kptr = NULL;
    if (0 == msys_find_dmem_by_phys(phys, &dmem))
    {
        kptr = (char*)(uintptr_t)dmem.kvirt;
        kptr += (phys - dmem.phys);
    }
    return kptr;
}

static int
_vhectx_compress(
    void*       pctx,
    mvhe_buff*  buff)
{
    int err = -EINVAL;
    int pitch = 0;
    mvhe_buff* buf = buff;
    mvhe_buff* out = buff + 1;
    mvhe_ctx* mctx = pctx;
    mvhe_dev* mdev = mctx->p_device;
    mhve_ops* mops = mctx->p_handle;

    if (buff->i_memory != MVHE_MEMORY_MMAP)
        return -EINVAL;
    if (out->i_memory != MVHE_MEMORY_MMAP || out->i_planes != 1)
        return -EINVAL;
    pitch = buff->i_stride;
    if (pitch < buff->i_width)
        pitch = buff->i_width;

    mutex_lock(&mctx->m_stream);
    do
    if (MVHE_CTX_STATE_IDLE == mctx->i_state && MVHE_OMODE_MMAP == mctx->i_omode)
    {
        uint  addr;
        void* kptr;
        mhve_cpb mcpb;
        mhve_vpb mvpb;
        mhve_cfg mcfg;
        unsigned char   b_long_term_reference;

        /* get LTR mode status */
        mcfg.type = MHVE_CFG_LTR;
        err = mops->get_conf(mops, &mcfg);
        b_long_term_reference = mcfg.ltr.b_long_term_reference;

        /* check pixel format and resolution */
        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        if (buff->i_planes != 2 && (MHVE_PIX_NV21 >= mcfg.res.e_pixf))
            break;
        if (buff->i_planes != 1 && (MHVE_PIX_YUYV == mcfg.res.e_pixf))
            break;
        if (mcfg.res.i_pixw != buf->i_width || mcfg.res.i_pixh != buf->i_height)
            break;

        /* setup input buffer */
        mcpb.i_index = 0;
        mvpb.i_index = buf->i_index;
        mvpb.i_stamp = buf->i_timecode;
        mvpb.u_flags = buf->i_flags&MVHE_FLAGS_CONTROL;
        /* TODO: fix LTR mode can't not open disposable */
        if (b_long_term_reference)
            mvpb.u_flags &= ~MVHE_FLAGS_DISPOSABLE;
        mvpb.i_pitch = pitch;
        mvpb.planes[1].u_phys = 0;
        mvpb.planes[1].i_bias = 0;
        addr = (uint)Chip_Phys_to_MIU(buf->planes[0].mem.phys);
        mvpb.planes[0].u_phys = addr;
        mvpb.planes[0].i_bias = buf->planes[0].i_bias;
        if (mcfg.res.e_pixf <= MHVE_PIX_NV21)
        {
            addr = (uint)Chip_Phys_to_MIU(buf->planes[1].mem.phys);
            mvpb.planes[1].u_phys = addr;
            mvpb.planes[1].i_bias = buf->planes[1].i_bias;
        }

        /* setup output buffer */
        addr = Chip_Phys_to_MIU(out->planes[0].mem.phys);
        if (!(kptr = phys2kptr(out->planes[0].mem.phys)))
            break;
        mcfg.type = MHVE_CFG_DMA;
        mcfg.dma.i_dmem = MHVE_CFG_DMA_OUTPUT_BUFFER;
        mcfg.dma.p_vptr = kptr;
        mcfg.dma.u_phys = addr;
        mcfg.dma.i_size[0] = out->planes[0].i_size;
        mops->set_conf(mops, &mcfg);

        mctx->i_state = MVHE_CTX_STATE_BUSY;
        do
        {
            if (0 != (err = mops->enc_buff(mops, &mvpb)))
            {
                printk(KERN_ERR "%s() enc_buff err\n", __func__);
                break;
            }
            mops->put_data(mops, mctx->p_usrdt, mctx->i_usrsz);
            if(0 != (err = mops->enc_conf(mops)))
            {
                printk(KERN_ERR "%s() enc_conf err\n", __func__);
                break;
            }
            /* call hw to encode frame */
            if (0 != (err = mvhedev_pushjob(mdev, mctx)))
            {
                printk(KERN_ERR "%s() mvhedev_pushjob err\n", __func__);
                break;
            }
            //TODO: super frame
            if (0 < (err = mops->enc_done(mops)))
            {
                printk(KERN_ERR "vhe-re-encode\n");
            }
        }
        while (err > 0);

        if (!err)
        {
            mctx->i_usrsz = mctx->i_usrcn = 0;
            mcpb.i_index = -1;
        }

        mops->deq_buff(mops, &mvpb);
        buf->i_index = mvpb.i_index;

        err = mops->out_buff(mops, &mcpb);
        out->planes[0].i_used = mcpb.planes[0].i_size;
        out->planes[0].i_bias = mcpb.planes[0].i_bias;
        out->i_timecode = mcpb.i_stamp;
        out->i_flags = mcpb.i_flags;

        mctx->i_state = MVHE_CTX_STATE_IDLE;
    }
    while (0);
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_vhectx_put_data(
    void*       pctx,
    mvhe_buff*  buff)
{
    int err = -EINVAL;
    mvhe_ctx* mctx = pctx;
    void* dst;

    if (buff->i_memory != MVHE_MEMORY_USER || buff->i_planes != 1)
        return err;
    if (buff->planes[0].mem.uptr == NULL ||
        buff->planes[0].i_size < buff->planes[0].i_used ||
        buff->planes[0].i_used > 1024)
        return err;

    mutex_lock(&mctx->m_stream);
    do
    if (MVHE_CTX_STATE_NULL != mctx->i_state && 4 > mctx->i_usrcn)
    {
        dst = mctx->p_usrdt + mctx->i_usrsz;
        err = -EFAULT;
        if (copy_from_user(dst, buff->planes[0].mem.uptr, buff->planes[0].i_used))
            break;
        mctx->i_usrsz += buff->planes[0].i_used;
        mctx->i_usrcn++;
        err = 0;
    }
    while (0);
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_vhectx_set_rqcf(
    void*       pctx,
    rqct_conf*  rqcf)
{
    int err = -EINVAL;
    mvhe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);

    mutex_lock(&mctx->m_stream);
    if ((unsigned)rqcf->type < RQCT_CONF_END && !rqct->set_rqcf(rqct, (rqct_cfg*)rqcf))
        err = 0;
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_vhectx_get_rqcf(
    void*       pctx,
    rqct_conf*  rqcf)
{
    int err = -EINVAL;
    mvhe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);

    mutex_lock(&mctx->m_stream);
    if ((unsigned)rqcf->type < RQCT_CONF_END && !rqct->get_rqcf(rqct, (rqct_cfg*)rqcf))
        err = 0;
    mutex_unlock(&mctx->m_stream);

    return err;
}
