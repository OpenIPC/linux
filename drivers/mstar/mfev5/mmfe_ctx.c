#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/semaphore.h>

#include <ms_platform.h>
#include <ms_msys.h>

#include <mdrv_mmfe_io.h>
#include <mdrv_rqct_io.h>

#include <mmfe_defs.h>

static int _mfectx_streamon(void* pctx, int on);
static int _mfectx_enc_pict(void* pctx, mmfe_buff* buff);
static int _mfectx_get_bits(void* pctx, mmfe_buff* buff);
static int _mfectx_set_parm(void* pctx, mmfe_parm* parm);
static int _mfectx_get_parm(void* pctx, mmfe_parm* parm);
static int _mfectx_set_ctrl(void* pctx, mmfe_ctrl* ctrl);
static int _mfectx_get_ctrl(void* pctx, mmfe_ctrl* ctrl);
static int _mfectx_compress(void* pctx, mmfe_buff* buff);
static int _mfectx_put_data(void* pctx, mmfe_buff* buff);
static int _mfectx_set_rqcf(void* pctx, rqct_conf* rqcf);
static int _mfectx_get_rqcf(void* pctx, rqct_conf* rqcf);
static void _mfectx_release(void* pctx)
{
    mmfe_ctx* mctx = pctx;
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
#if MMFE_TIMER_SIZE>0
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
static void _mfectx_adduser(void* pctx)
{
    mmfe_ctx* mctx = pctx;
    atomic_inc(&mctx->i_refcnt);
}

void* mmfeops_acquire(int);

mmfe_ctx*
mmfectx_acquire(
    mmfe_dev*   mdev)
{
    mmfe_ctx* mctx = NULL;
    mhve_ops* mops = NULL;

    if (!mdev || !(mctx = kzalloc(sizeof(mmfe_ctx), GFP_KERNEL)))
    {
        printk(KERN_ERR"%s() alloc fail\n", __func__);
        return mctx;
    }

    mctx->p_usrdt = NULL;
    do
    {
        if (!(mops = mmfeops_acquire(mdev->i_rctidx)))
            break;
        if (!(mctx->p_usrdt = kzalloc(MMFE_USER_DATA_SIZE, GFP_KERNEL)))
            break;
#if MMFE_TIMER_SIZE>0
        mctx->p_timer = kzalloc(MMFE_TIMER_SIZE, GFP_KERNEL);
        mctx->i_numbr = 0;
#endif
        mutex_init(&mctx->m_stream);
        mutex_init(&mctx->m_encode);
        mctx->i_state = MMFE_CTX_STATE_NULL;
        mctx->release =_mfectx_release;
        mctx->adduser =_mfectx_adduser;
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
mmfectx_actions(
    mmfe_ctx*       mctx,
    unsigned int    cmd,
    void*           arg)
{
    int err = 0;
    switch (cmd)
    {
    case IOCTL_MMFE_S_PARM:
        err = _mfectx_set_parm(mctx, (mmfe_parm*)arg);
        break;
    case IOCTL_MMFE_G_PARM:
        err = _mfectx_get_parm(mctx, (mmfe_parm*)arg);
        break;
    case IOCTL_MMFE_STREAMON:
        err = _mfectx_streamon(mctx, 1);
        break;
    case IOCTL_MMFE_STREAMOFF:
        err = _mfectx_streamon(mctx, 0);
        break;
    case IOCTL_MMFE_S_PICT:
        err = _mfectx_enc_pict(mctx, (mmfe_buff*)arg);
        break;
    case IOCTL_MMFE_G_BITS:
        err = _mfectx_get_bits(mctx, (mmfe_buff*)arg);
        break;
    case IOCTL_MMFE_S_CTRL:
        err = _mfectx_set_ctrl(mctx, (mmfe_ctrl*)arg);
        break;
    case IOCTL_MMFE_G_CTRL:
        err = _mfectx_get_ctrl(mctx, (mmfe_ctrl*)arg);
        break;
    case IOCTL_MMFE_ENCODE:
        err = _mfectx_compress(mctx, (mmfe_buff*)arg);
        break;
    case IOCTL_MMFE_S_DATA:
        err = _mfectx_put_data(mctx, (mmfe_buff*)arg);
        break;
    case IOCTL_RQCT_S_CONF:
        err = _mfectx_set_rqcf(mctx, (rqct_conf*)arg);
        break;
    case IOCTL_RQCT_G_CONF:
        err = _mfectx_get_rqcf(mctx, (rqct_conf*)arg);
        break;
    default:
        err = -EINVAL;
        break;
    }
    return (long)err;
}

#define _ALIGN_(b,a)   (((a)+(1<<(b))-1)&(~((1<<(b))-1)))

#define MBPIXELS_Y      256
#define MBPIXELS_C     (MBPIXELS_Y/2)
#define MBGNDATA        128
#define MBROWMAX        168     //(2688/16)

static int
_mfectx_streamon(
    void*   pctx,
    int     on)
{
    mmfe_ctx* mctx = pctx;
    mmfe_dev* mdev = mctx->p_device;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    mhve_cfg mcfg;
    rqct_cfg rqcf;
    int i, err = 0;

    mutex_lock(&mctx->m_stream);
    do
    if (on)
    {
        int size_out, size_mbs, size_mbp, size_dqm;
        int size_lum, size_chr, mbw, mbh, mbn;
        int score, rpbn;
        uint addr;
        char*kptr;
        msys_mem* pdmem;
        if (MMFE_CTX_STATE_NULL != mctx->i_state)
            break;
        mctx->i_dmems = 0;
        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        mbw = _ALIGN_(4,mcfg.res.i_pixw)/16;
        mbh = _ALIGN_(4,mcfg.res.i_pixh)/16;
        mbn = mbw*mbh;
        /* calculate required buffer size */
        size_mbp = _ALIGN_( 8,MBROWMAX*MBGNDATA);
        size_dqm = _ALIGN_( 8,(mbn+1)/2+16);
        size_mbs = size_mbp + size_dqm;
        size_lum = _ALIGN_( 8,mbn*MBPIXELS_Y);
        size_chr = _ALIGN_( 8,mbn*MBPIXELS_C);
        size_out = _ALIGN_(12,size_lum);
        if (mctx->i_omode == MMFE_OMODE_MMAP)
            size_out = 0;
        /* intermediate parameters */
        rpbn = mcfg.res.i_rpbn;
        do
        {   /* output-mode */
            mctx->p_ovptr = NULL;
            mctx->u_ophys = 0;
            mctx->i_osize = mctx->i_ormdr = 0;
            if (size_out > 0)
            {
                pdmem = &mctx->m_dmems[mctx->i_dmems++];
                snprintf(pdmem->name, 15, "S%d:MFEDMBS", mctx->i_index);
                pdmem->length = size_out;
                if (0 != (err = msys_request_dmem(pdmem)))
                    break;
                addr = Chip_Phys_to_MIU(pdmem->phys);
                kptr = (char*)(uintptr_t)pdmem->kvirt;
                mcfg.type = MHVE_CFG_DMA;
                mcfg.dma.i_dmem = MHVE_CFG_DMA_OUTPUT_BUFFER;
                mcfg.dma.p_vptr = kptr;
                mcfg.dma.u_phys = addr;
                mcfg.dma.i_size[0] = size_out;
                mops->set_conf(mops, &mcfg);
                mctx->p_ovptr = kptr;
                mctx->u_ophys = addr;
                mctx->i_osize = size_out;
            }
            pdmem = &mctx->m_dmems[mctx->i_dmems++];
            if (mctx->i_strid == STREAM_ID_DEFAULT)
            {
                snprintf(pdmem->name, 15, "S%d:MFEDMMB", mctx->i_index);
            }
            else
            {
                /* encoder buffer name alignment */
                snprintf(pdmem->name, 15, "S%d:VENCDMOUT", mctx->i_strid);
            }
            pdmem->length = size_mbs;
            if (0 != (err = msys_request_dmem(pdmem)))
                break;
            addr = Chip_Phys_to_MIU(pdmem->phys);
            kptr = (char*)(uintptr_t)pdmem->kvirt;
            /* Q-Map allocate memory */
            rqcf.type = RQCT_CFG_DQM;
            rqcf.dqm.u_phys = addr; addr += size_dqm;
            rqcf.dqm.p_kptr = kptr;
            rqcf.dqm.i_dqmw = mbw;
            rqcf.dqm.i_dqmh = mbh;
            rqcf.dqm.i_size = size_dqm;
            rqct->set_rqcf(rqct, &rqcf);
            /* mb-row: GN data */
            mcfg.type = MHVE_CFG_DMA;
            mcfg.dma.i_dmem = MHVE_CFG_DMA_NALU_BUFFER;
            mcfg.dma.u_phys = addr;
            mcfg.dma.p_vptr = NULL;
            mcfg.dma.i_size[0] = size_mbp;
            mops->set_conf(mops, &mcfg);
            /* ref/rec picture buffers */
            if (mctx->i_imode == MMFE_IMODE_PURE)
            {
                /* create internal buffer for reconstruct */
                for (i = 0; i < rpbn; i++)
                {
                    pdmem = &mctx->m_dmems[mctx->i_dmems++];
                    pdmem->length = size_lum + size_chr;
                    if (mctx->i_strid == STREAM_ID_DEFAULT)
                    {
                        snprintf(pdmem->name, 15, "S%d:MFEDMP%d", mctx->i_index, i);
                    }
                    else
                    {
                        /* encoder buffer name alignment */
                        snprintf(pdmem->name, 15, "S%d:VENCDMP%d", mctx->i_strid, i);
                    }
                    if (0 != (err = msys_request_dmem(pdmem)))
                        break;
                    addr = Chip_Phys_to_MIU(pdmem->phys);
                    mcfg.type = MHVE_CFG_DMA;
                    mcfg.dma.i_dmem = i;
                    mcfg.dma.u_phys = addr;
                    mcfg.dma.p_vptr = NULL;
                    mcfg.dma.i_size[0] = size_lum;
                    mcfg.dma.i_size[1] = size_chr;
                    mops->set_conf(mops, &mcfg);
                }
            }
            else if (mctx->i_imode == MMFE_IMODE_PLUS)
            {
                /* use input buffer as reconstruct buffer */
                for (i = 0; i < rpbn; i++)
                {
                    mcfg.type = MHVE_CFG_DMA;
                    mcfg.dma.i_dmem = i;
                    mcfg.dma.u_phys = 0;
                    mcfg.dma.p_vptr = NULL;
                    mcfg.dma.i_size[0] = 0;
                    mcfg.dma.i_size[1] = 0;
                    mops->set_conf(mops, &mcfg);
                }
            }
            rqcf.type = RQCT_CFG_FPS;
            rqct->get_rqcf(rqct, &rqcf);
            score = mbn;
            score *= (int)rqcf.fps.n_fps;
            score /= (int)rqcf.fps.d_fps;
            mctx->i_score = score;
        }
        while (0);

        if (!err && !(err = mops->seq_sync(mops)))
        {
            mops->seq_conf(mops);
            mmfedev_poweron(mdev, mctx->i_score);
            mdev->i_counts[mctx->i_index][0] = mdev->i_counts[mctx->i_index][1] = 0;
            mdev->i_counts[mctx->i_index][2] = mdev->i_counts[mctx->i_index][3] = 0;
            mdev->i_counts[mctx->i_index][4] = 0;
            mctx->i_state = MMFE_CTX_STATE_IDLE;
            break;
        }
    }
    else
    {
        if (MMFE_CTX_STATE_NULL == mctx->i_state)
            break;
        mmfedev_poweron(mdev,-mctx->i_score);
        mctx->i_state = MMFE_CTX_STATE_NULL;
        mctx->i_score = 0;
        mops->seq_done(mops);
#if 0
        printk("<%d>mfe performance:\n",mctx->i_index);
        for (i = 0; i < MMFE_TIMER_SIZE/8; i++)
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

#define SEMI420(f)  ((f)>=MHVE_PIX_NV12&&(f)<=MHVE_PIX_NV21)
#define PACK422(f)  ((f)>=MHVE_PIX_YUYV&&(f)<=MHVE_PIX_YVYU)
#define MMFE_FLAGS_CONTROL  (MMFE_FLAGS_IDR|MMFE_FLAGS_DISPOSABLE|MMFE_FLAGS_NIGHT_MODE)

static int
_mfectx_enc_pict(
    void*           pctx,
    mmfe_buff*      buff)
{
    int err = -EINVAL;
    int pitch = 0;
    mmfe_ctx* mctx = pctx;
    mmfe_dev* mdev = mctx->p_device;
    mhve_ops* mops = mctx->p_handle;
    mhve_cpb* pcpb = mctx->m_mcpbs;

    if (buff->i_memory != MMFE_MEMORY_MMAP)
        return -EINVAL;
    pitch = buff->i_stride;
    if (pitch < buff->i_width)
        pitch = buff->i_width;

    mutex_lock(&mctx->m_stream);
    do
    if (MMFE_CTX_STATE_IDLE == mctx->i_state && MMFE_OMODE_USER == mctx->i_omode)
    {
        mhve_vpb mvpb;
        mhve_cfg mcfg;

        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        if (buff->i_planes != 2 && (MHVE_PIX_NV21 >= mcfg.res.e_pixf))
            break;
        if (buff->i_planes != 1 && (MHVE_PIX_YUYV <= mcfg.res.e_pixf))
            break;
        if (mcfg.res.i_pixw != buff->i_width || mcfg.res.i_pixh != buff->i_height)
            break;

        mvpb.i_index = buff->i_index;
        mvpb.i_stamp = buff->i_timecode;
        mvpb.u_flags = buff->i_flags&MMFE_FLAGS_CONTROL;
        mvpb.i_pitch = pitch;
        mvpb.planes[1].u_phys = 0;
        mvpb.planes[1].i_bias = 0;
        mvpb.planes[0].u_phys = buff->planes[0].mem.phys;
        mvpb.planes[0].i_bias = buff->planes[0].i_bias;
        if (mcfg.res.e_pixf <= MHVE_PIX_NV21)
        {
            mvpb.planes[1].u_phys = buff->planes[1].mem.phys;
            mvpb.planes[1].i_bias = buff->planes[1].i_bias;
        }

        mctx->i_state = MMFE_CTX_STATE_BUSY;
        if (!(err = mops->enc_buff(mops, &mvpb)))
        do
        {
            mops->put_data(mops, mctx->p_usrdt, mctx->i_usrsz);
            mops->enc_conf(mops);
            mmfedev_pushjob(mdev, mctx);
        }
        while (0 < (err = mops->enc_done(mops)));

        if (!err)
        {
            mctx->i_usrcn = mctx->i_usrsz = 0;
            pcpb->i_index = -1;
        }

        mctx->i_ormdr =
        err =
        mops->out_buff(mops, mctx->m_mcpbs);
    }
    while (0);
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_mfectx_get_bits(
    void*           pctx,
    mmfe_buff*      buff)
{
    mmfe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    int err = -ENODATA;

    if (buff->i_memory != MMFE_MEMORY_USER || buff->i_planes != 1)
        return -EINVAL;

    mutex_lock(&mctx->m_stream);
    if (MMFE_CTX_STATE_BUSY == mctx->i_state && MMFE_OMODE_USER == mctx->i_omode)
    {
        int flags = 0;
        int  rmdr = mctx->i_ormdr;
        int  size = mctx->m_mcpbs->planes[0].i_size;
        char* ptr = mctx->p_ovptr + size - rmdr;

        if (rmdr == size)
            flags = MMFE_FLAGS_SOP;
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
            mctx->i_state = MMFE_CTX_STATE_IDLE;
            flags |= MMFE_FLAGS_EOP;
            mops->out_buff(mops, mctx->m_mcpbs);
        }
        if (!err)
            buff->i_flags = flags|(mctx->m_mcpbs->i_flags&MMFE_FLAGS_CONTROL);
    }
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_mfectx_set_parm(
    void*           pctx,
    mmfe_parm*      parm)
{
    mmfe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    int err = -EINVAL;

    mutex_lock(&mctx->m_stream);
    if (mctx->i_state == MMFE_CTX_STATE_NULL)
    {
        rqct_ops* rqct = mops->rqct_ops(mops);
        mhve_cfg  mcfg;
        rqct_cfg  rqcf;
        unsigned char b_long_term_reference;
        switch (parm->type)
        {
        case MMFE_PARM_RES:
            /* check LTR mode */
            mcfg.type = MHVE_CFG_LTR;
            err = mops->get_conf(mops, &mcfg);
            b_long_term_reference = mcfg.ltr.b_long_term_reference;
            if ((unsigned)parm->res.i_pixfmt > MMFE_PIXFMT_YUYV)
                break;
            if ((parm->res.i_pict_w%16) || (parm->res.i_pict_h%8))
                break;
            mctx->i_max_w = _ALIGN_(4,parm->res.i_pict_w);
            mctx->i_max_h = _ALIGN_(4,parm->res.i_pict_h);
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
            mctx->i_omode = MMFE_OMODE_USER;
            mctx->i_imode = MMFE_IMODE_PURE;
            if (parm->res.i_outlen < 0)
                mctx->i_omode = MMFE_OMODE_MMAP;
            if (parm->res.i_outlen <-1)
                mctx->i_imode = MMFE_IMODE_PLUS;
            mops->set_conf(mops, &mcfg);
            rqcf.type = RQCT_CFG_RES;
            rqcf.res.i_picw = mctx->i_max_w;
            rqcf.res.i_pich = mctx->i_max_h;
            rqct->set_rqcf(rqct, &rqcf);
            err = 0;
            break;
        case MMFE_PARM_FPS:
            if (parm->fps.i_num > 0 && parm->fps.i_den > 0)
            {
                rqcf.type = RQCT_CFG_FPS;
                rqcf.fps.n_fps = parm->fps.i_num;
                rqcf.fps.d_fps = parm->fps.i_den;
                rqct->set_rqcf(rqct, &rqcf);
                err = 0;
            }
            break;
        case MMFE_PARM_MOT:
            mcfg.type = MHVE_CFG_MOT;
            mcfg.mot.i_subp = parm->mot.i_subpel;
            mcfg.mot.i_dmvx = _MIN(_MAX(8,parm->mot.i_dmv_x),32);
            mcfg.mot.i_dmvy = parm->mot.i_dmv_y<=8?8:16;
            mcfg.mot.i_blkp[0] = parm->mot.i_mvblks[0];
            mcfg.mot.i_blkp[1] = 0;
            err = mops->set_conf(mops, &mcfg);
            break;
        case MMFE_PARM_BPS:
            if ((unsigned)parm->bps.i_method > RQCT_METHOD_VBR)
                break;
            rqcf.type = RQCT_CONF_SEQ;
            if (!rqct->get_rqcf(rqct, &rqcf))
            {
                rqcf.seq.i_method = parm->bps.i_method;
                rqcf.seq.i_btrate = parm->bps.i_bps;
                rqcf.seq.i_leadqp = parm->bps.i_ref_qp;
                err = rqct->set_rqcf(rqct, &rqcf);
            }
            break;
        case MMFE_PARM_GOP:
            if (parm->gop.i_pframes < 0)
                break;
            rqcf.type = RQCT_CONF_SEQ;
            if (!rqct->get_rqcf(rqct, &rqcf))
            {
                rqcf.seq.i_period = parm->gop.i_pframes+1;
                if (!rqct->set_rqcf(rqct, &rqcf))
                    err = 0;
            }
            break;
        case MMFE_PARM_AVC:
            mcfg.type = MHVE_CFG_AVC;
            mcfg.avc.i_profile = parm->avc.i_profile;
            mcfg.avc.i_level = parm->avc.i_level;
            mcfg.avc.i_num_ref_frames = parm->avc.i_num_ref_frames;
            mcfg.avc.i_poc_type = parm->avc.i_poc_type;
            mcfg.avc.b_entropy_coding_type = parm->avc.b_cabac;
            mcfg.avc.b_constrained_intra_pred = parm->avc.b_constrained_intra_pred;
            mcfg.avc.b_deblock_filter_control = parm->avc.b_deblock_filter_control;
            mcfg.avc.i_disable_deblocking_idc = parm->avc.i_disable_deblocking_idc;
            mcfg.avc.i_alpha_c0_offset = parm->avc.i_alpha_c0_offset;
            mcfg.avc.i_beta_offset = parm->avc.i_beta_offset;
            err = mops->set_conf(mops, &mcfg);
            break;
        case MMFE_PARM_VUI:
            mcfg.type = MHVE_CFG_VUI;
            mcfg.vui.b_video_full_range = parm->vui.b_video_full_range != 0;
            mcfg.vui.b_timing_info_pres = parm->vui.b_timing_info_pres != 0;
            err = mops->set_conf(mops, &mcfg);
            break;
        case MMFE_PARM_LTR:
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
            printk("unsupported config\n");
            break;
        }
    }
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_mfectx_get_parm(
    void*           pctx,
    mmfe_parm*      parm)
{
    int err = 0;
    mmfe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    mhve_cfg  mcfg;
    rqct_cfg  rqcf;

    mutex_lock(&mctx->m_stream);
    switch (parm->type)
    {
    case MMFE_PARM_IDX:
        parm->idx.i_stream = mctx->i_index;
        break;
    case MMFE_PARM_RES:
        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        parm->res.i_pict_w = mctx->i_max_w;
        parm->res.i_pict_h = mctx->i_max_h;
        parm->res.i_strid  = mctx->i_strid;
        parm->res.i_pixfmt = mcfg.res.e_pixf;
        parm->res.i_outlen = 0;
        parm->res.i_flags = 0;
        break;
    case MMFE_PARM_FPS:
        rqcf.type = RQCT_CFG_FPS;
        if (!(err = rqct->get_rqcf(rqct, &rqcf)))
        {
            parm->fps.i_num = (int)rqcf.fps.n_fps;
            parm->fps.i_den = (int)rqcf.fps.d_fps;
        }
        break;
    case MMFE_PARM_MOT:
        mcfg.type = MHVE_CFG_MOT;
        mops->get_conf(mops, &mcfg);
        parm->mot.i_dmv_x = mcfg.mot.i_dmvx;
        parm->mot.i_dmv_y = mcfg.mot.i_dmvy;
        parm->mot.i_subpel= mcfg.mot.i_subp;
        parm->mot.i_mvblks[0] = mcfg.mot.i_blkp[0];
        parm->mot.i_mvblks[1] = 0;
        break;
    case MMFE_PARM_BPS:
    case MMFE_PARM_GOP:
        rqcf.type = RQCT_CONF_SEQ;
        if (!rqct->get_rqcf(rqct, &rqcf))
        {
            if (parm->type == MMFE_PARM_BPS)
            {
                parm->bps.i_method = rqcf.seq.i_method;
                parm->bps.i_ref_qp = rqcf.seq.i_leadqp;
                parm->bps.i_bps = rqcf.seq.i_btrate;
            }
            else
            {
                parm->gop.i_pframes = rqcf.seq.i_period-1;
                parm->gop.i_bframes = 0;
            }
        }
        break;
    case MMFE_PARM_AVC:
        mcfg.type = MHVE_CFG_AVC;
        if (!(err = mops->get_conf(mops, &mcfg)))
        {
            parm->avc.i_profile = mcfg.avc.i_profile;
            parm->avc.i_level = mcfg.avc.i_level;
            parm->avc.i_num_ref_frames = mcfg.avc.i_num_ref_frames;
            parm->avc.i_poc_type = mcfg.avc.i_poc_type;
            parm->avc.b_cabac = mcfg.avc.b_entropy_coding_type;
            parm->avc.b_deblock_filter_control = mcfg.avc.b_deblock_filter_control;
            parm->avc.b_constrained_intra_pred = mcfg.avc.b_constrained_intra_pred;
            parm->avc.i_disable_deblocking_idc = mcfg.avc.i_disable_deblocking_idc;
            parm->avc.i_alpha_c0_offset = mcfg.avc.i_alpha_c0_offset;
            parm->avc.i_beta_offset = mcfg.avc.i_beta_offset;
        }
        break;
    case MMFE_PARM_VUI:
        mcfg.type = MHVE_CFG_VUI;
        if (!(err = mops->get_conf(mops, &mcfg)))
        {
            parm->vui.b_video_full_range = 0!=mcfg.vui.b_video_full_range;
            parm->vui.b_timing_info_pres = 0!=mcfg.vui.b_timing_info_pres;
        }
        break;
    case MMFE_PARM_LTR:
        mcfg.type = MHVE_CFG_LTR;
        if (!(err = mops->get_conf(mops, &mcfg)))
        {
            parm->ltr.b_long_term_reference = mcfg.ltr.b_long_term_reference;
            if (mcfg.ltr.b_long_term_reference)
            {
                rqcf.type = RQCT_CFG_LTR;
                if (!rqct->get_rqcf(rqct, &rqcf))
                {
                    parm->ltr.i_ltr_period = rqcf.ltr.i_period;
                }
                parm->ltr.b_enable_pred = mcfg.ltr.b_enable_pred;
            }
            else
            {
                parm->ltr.i_ltr_period = 0;
                parm->ltr.b_enable_pred = 0;
            }
        }
        break;
    default:
        printk("unsupported config\n");
        break;
    }
    mutex_unlock(&mctx->m_stream);
    return err;
}

static int
_mfectx_set_ctrl(
    void*           pctx,
    mmfe_ctrl*      ctrl)
{
    int err = -EINVAL;
    mmfe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    rqct_cfg rqcf;
    mhve_cfg mcfg;

    mutex_lock(&mctx->m_stream);
    if (mctx->i_state > MMFE_CTX_STATE_NULL)
    switch (ctrl->type)
    {
    case MMFE_CTRL_ROI:
        if (ctrl->roi.i_index >= RQCT_ROI_NR || ctrl->roi.i_index < -1)
            break;
        rqcf.type = RQCT_CFG_ROI;
        rqcf.roi.i_roiidx = ctrl->roi.i_index;
        err = 0;
        if ((unsigned)(ctrl->roi.i_dqp + 15) > 30)
        {
            rqcf.roi.i_roidqp = 0;
            rqct->set_rqcf(rqct, &rqcf);
            break;
        }
        rqcf.roi.i_roidqp = ctrl->roi.i_dqp;
        rqcf.roi.i_posx = ctrl->roi.i_mbx;
        rqcf.roi.i_posy = ctrl->roi.i_mby;
        rqcf.roi.i_recw = ctrl->roi.i_mbw;
        rqcf.roi.i_rech = ctrl->roi.i_mbh;
        rqct->set_rqcf(rqct, &rqcf);
        break;
    case MMFE_CTRL_SPL:
        mcfg.type = MHVE_CFG_SPL;
        mcfg.spl.i_rows = ctrl->spl.i_rows;
        mcfg.spl.i_bits = ctrl->spl.i_bits;
        mops->set_conf(mops, &mcfg);
        err = 0;
        break;
    case MMFE_CTRL_SEQ:
        if ((unsigned)ctrl->seq.i_pixfmt <= MMFE_PIXFMT_YVYU &&
            ctrl->seq.i_pixelw >= 128 &&
            ctrl->seq.i_pixelh >= 128 &&
            ctrl->seq.d_fps > 0 &&
            ctrl->seq.n_fps > 0)
        {
            int mbw, mbh, mbn;
            mcfg.type = MHVE_CFG_RES;
            mops->get_conf(mops, &mcfg);
            mcfg.res.e_pixf = ctrl->seq.i_pixfmt;
            mcfg.res.i_pixw = ctrl->seq.i_pixelw;
            mcfg.res.i_pixh = ctrl->seq.i_pixelh;
            if ((err = mops->set_conf(mops, &mcfg)))
                break;
            rqcf.type = RQCT_CFG_FPS;
            rqcf.fps.n_fps = (short)ctrl->seq.n_fps;
            rqcf.fps.d_fps = (short)ctrl->seq.d_fps;
            if ((err = rqct->set_rqcf(rqct, &rqcf)))
                break;
            mbw = _ALIGN_(4,mcfg.res.i_pixw)>>4;
            mbh = _ALIGN_(4,mcfg.res.i_pixh)>>4;
            mbn = mbw*mbh;
            rqcf.type = RQCT_CFG_RES;
            rqcf.res.i_picw = (short)(mbw*16);
            rqcf.res.i_pich = (short)(mbh*16);
            if ((err = rqct->set_rqcf(rqct, &rqcf)))
                break;
            /* disable ROI */
            rqcf.type = RQCT_CFG_ROI;
            rqcf.roi.i_roiidx = -1;
            rqct->set_rqcf(rqct, &rqcf);
            /* reset dqm */
            rqcf.type = RQCT_CFG_DQM;
            rqct->get_rqcf(rqct, &rqcf);
            rqcf.dqm.i_dqmw = mbw;
            rqcf.dqm.i_dqmh = mbh;
            rqcf.dqm.i_size = _ALIGN_(8,(mbn+1)/2+16);
            rqct->set_rqcf(rqct, &rqcf);
            err = 0;
        }
        break;
    case MMFE_CTRL_LTR:
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
_mfectx_get_ctrl(
    void*           pctx,
    mmfe_ctrl*      ctrl)
{
    int err = -EINVAL;
    mmfe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);
    rqct_cfg rqcf;
    mhve_cfg mcfg;

    mutex_lock(&mctx->m_stream);
    if (mctx->i_state > MMFE_CTX_STATE_NULL)
    switch (ctrl->type)
    {
    case MMFE_CTRL_ROI:
        rqcf.type = RQCT_CFG_ROI;
        rqct->get_rqcf(rqct, &rqcf);
        ctrl->roi.i_dqp = rqcf.roi.i_roidqp;
        ctrl->roi.i_mbx = rqcf.roi.i_posx;
        ctrl->roi.i_mby = rqcf.roi.i_posy;
        ctrl->roi.i_mbw = rqcf.roi.i_recw;
        ctrl->roi.i_mbh = rqcf.roi.i_rech;
        err = 0;
        break;
    case MMFE_CTRL_SPL:
        mcfg.type = MHVE_CFG_SPL;
        mops->get_conf(mops, &mcfg);
        ctrl->spl.i_rows = mcfg.spl.i_rows;
        ctrl->spl.i_bits = mcfg.spl.i_bits;
        err = 0;
        break;
    case MMFE_CTRL_SEQ:
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
    case MMFE_CTRL_LTR:
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
_mfectx_compress(
    void*       pctx,
    mmfe_buff*  buff)
{
    mmfe_buff* buf = buff;
    mmfe_buff* out = buff + 1;
    mmfe_ctx* mctx = pctx;
    mmfe_dev* mdev = mctx->p_device;
    mhve_ops* mops = mctx->p_handle;
    int pitch, err = -1;

    if (buff->i_memory != MMFE_MEMORY_MMAP)
        return -EINVAL;
    if (out->i_memory != MMFE_MEMORY_MMAP || out->i_planes != 1)
        return -EINVAL;
    pitch = buff->i_stride;
    if (pitch < buff->i_width)
        pitch = buff->i_width;

    mutex_lock(&mctx->m_stream);
    do
    if (MMFE_CTX_STATE_IDLE == mctx->i_state && MMFE_OMODE_MMAP == mctx->i_omode)
    {
        uint  addr;
        void* kptr;
        mhve_cfg mcfg;
        mhve_cpb mcpb;
        mhve_vpb mvpb;

        /* align resolution to 32 */
        mcfg.type = MHVE_CFG_RES;
        mops->get_conf(mops, &mcfg);
        if (buff->i_planes != 2 && (MHVE_PIX_NV21 >= mcfg.res.e_pixf))
            break;
        if (buff->i_planes != 1 && (MHVE_PIX_YUYV == mcfg.res.e_pixf))
            break;
        if (mcfg.res.i_pixw != buf->i_width ||
            mcfg.res.i_pixh != buf->i_height)
            break;
        if (mcfg.res.e_pixf > MHVE_PIX_NV21)
            pitch *= 2;

        /* setup input buffer */
        mcpb.i_index = 0;
        mvpb.i_index = buf->i_index;
        mvpb.i_stamp = buf->i_timecode;
        mvpb.u_flags = buf->i_flags&MMFE_FLAGS_CONTROL;
        mvpb.i_pitch = pitch;
        mvpb.planes[1].u_phys = 0;
        mvpb.planes[1].i_bias = 0;
        addr = (uint)Chip_Phys_to_MIU(buf->planes[0].mem.phys);
        mvpb.planes[0].u_phys = addr;
        mvpb.planes[0].i_bias = buf->planes[0].i_bias;
        if (mcfg.res.e_pixf <= MHVE_PIX_NV21)
        {
            addr = Chip_Phys_to_MIU(buf->planes[1].mem.phys);
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

        mctx->i_state = MMFE_CTX_STATE_BUSY;
        do
        {
            if (0 != (err = mops->enc_buff(mops, &mvpb)))
            {
                printk(KERN_ERR "%s() enc_buff err\n", __func__);
                break;
            }
            mops->put_data(mops, mctx->p_usrdt, mctx->i_usrsz);
            if (0 != (mops->enc_conf(mops)))
            {
                printk(KERN_ERR "%s() enc_conf err\n", __func__);
                break;
            }
            if (0 != (mmfedev_pushjob(mdev, mctx)))
            {
                printk(KERN_ERR "%s() mvhedev_pushjob err\n", __func__);
                break;
            }
            if (0 < (err = mops->enc_done(mops)))
            {
                printk(KERN_ERR "mfe-re-encode\n");
            }
        }
        while (err > 0);

        if (!err)
        {
            mctx->i_usrcn = mctx->i_usrsz = 0;
            mcpb.i_index = -1;
        }

        mops->deq_buff(mops, &mvpb);
        buff->i_index = mvpb.i_index;

        err = mops->out_buff(mops, &mcpb);
        out->planes[0].i_used = _MIN(mcpb.planes[0].i_size, mcfg.dma.i_size[0]);
        out->planes[0].i_bias = mcpb.planes[0].i_bias;
        out->i_timecode = mcpb.i_stamp;
        out->i_flags = mcpb.i_flags;

        mctx->i_state = MMFE_CTX_STATE_IDLE;
    }
    while (0);
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_mfectx_put_data(
    void*       pctx,
    mmfe_buff*  buff)
{
    int err = -EINVAL;
    mmfe_ctx* mctx = pctx;
    void* dst;

    if (buff->i_memory != MMFE_MEMORY_USER || buff->i_planes != 1)
        return err;
    if (buff->planes[0].mem.uptr == NULL ||
        buff->planes[0].i_size < buff->planes[0].i_used ||
        buff->planes[0].i_used > 1024)
        return err;

    mutex_lock(&mctx->m_stream);
    do
    if (MMFE_CTX_STATE_NULL != mctx->i_state && 4 > mctx->i_usrcn)
    {
        dst = (char*)mctx->p_usrdt + mctx->i_usrsz;
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
_mfectx_set_rqcf(
    void*       pctx,
    rqct_conf*  rqcf)
{
    int err = -EINVAL;
    mmfe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);

    mutex_lock(&mctx->m_stream);
    if ((unsigned)rqcf->type < RQCT_CONF_END && !rqct->set_rqcf(rqct, (rqct_cfg*)rqcf))
        err = 0;
    mutex_unlock(&mctx->m_stream);

    return err;
}

static int
_mfectx_get_rqcf(
    void*       pctx,
    rqct_conf*  rqcf)
{
    int err = -EINVAL;
    mmfe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = mops->rqct_ops(mops);

    mutex_lock(&mctx->m_stream);
    if ((unsigned)rqcf->type < RQCT_CONF_END && !rqct->get_rqcf(rqct, (rqct_cfg*)rqcf))
        err = 0;
    mutex_unlock(&mctx->m_stream);

    return err;
}
