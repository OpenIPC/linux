
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>

#include <ms_platform.h>
#include <ms_msys.h>

#include <mdrv_mfe_io.h>

#include <mst_def.h>

#include <types.h>
#include <mfe_avc.h>
#include <mfe_rc.h>
#include <mfe_regs.h>
#include <mfe_asic.h>
#include <mfe_hdlr.h>

static int mfe_ops_streamon(void*, int on);
static int mfe_ops_enc_pict(void*, mmfe_buffer* b);
static int mfe_ops_get_bits(void*, mmfe_buffer* b);
static int mfe_ops_set_parm(void*, mmfe_params* p);
static int mfe_ops_get_parm(void*, mmfe_params* p);

long
mstmfe_ops(
    void*           mfh,
    unsigned int    cmd,
    void*           arg)
{
    int err = 0;

    switch (cmd)
    {
    case IOCTL_MFE_S_PARM:
        err = mfe_ops_set_parm(mfh, (mmfe_params*)arg);
        break;
    case IOCTL_MFE_G_PARM:
        err = mfe_ops_get_parm(mfh, (mmfe_params*)arg);
        break;
    case IOCTL_MFE_STREAMON:
        err = mfe_ops_streamon(mfh, 1);
        break;
    case IOCTL_MFE_STREAMOFF:
        err = mfe_ops_streamon(mfh, 0);
        break;
    case IOCTL_MFE_S_PICT:
        err = mfe_ops_enc_pict(mfh, (mmfe_buffer*)arg);
        break;
    case IOCTL_MFE_G_BITS:
        err = mfe_ops_get_bits(mfh, (mmfe_buffer*)arg);
        break;
    default:
        err = -EINVAL;
        break;
    }

    return (long)err;
}

static const char* KEY_MFE_DMEM="MFE_DMEM";

#define MIU_ALIGN(b,a)  (((a)+(1<<(b))-1)&(~((1<<(b))-1)))
#define CPB_SIZE        (1024*1024)

static int
request_kma(
    struct device*  dev,
    kma_t*          kma,
    int             reqsize)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name, KEY_MFE_DMEM, strlen(KEY_MFE_DMEM)+1);
    dmem.length = reqsize;
    if (0 != msys_request_dmem(&dmem))
    {
        kma->mem_vptr = NULL;
        kma->bus_addr = 0;
        kma->miu_addr = 0;
        kma->size = 0;
        return -ENOMEM;
    }
    kma->mem_vptr = (void*)((uintptr_t)dmem.kvirt);
    kma->bus_addr = dmem.phys;
    kma->miu_addr = Chip_Phys_to_MIU(dmem.phys);
    kma->size = dmem.length;

    return 0;
}

static int
release_kma(
    struct device*  dev,
    kma_t*          kma)
{
	MSYS_DMEM_INFO dmem;
    if (!kma || !kma->mem_vptr)
    {
        return -EINVAL;
    }
	memcpy(dmem.name, KEY_MFE_DMEM, strlen(KEY_MFE_DMEM)+1);
	dmem.length = kma->size;
	dmem.kvirt = (unsigned long long)((uintptr_t)kma->mem_vptr);
	dmem.phys = (unsigned long long)((uintptr_t)kma->bus_addr);
	msys_release_dmem(&dmem);

    return 0;
}

static int 
mfe_ops_streamon(
    void*   mfh,
    int     on)
{
    mst_ctx* ctx = (mst_ctx*)mfh;
    mfe_hdlr* h = (mfe_hdlr*)ctx->drv_data;
    mfe_asic* d = h->asic;
    int err = 0;

    /* check valid parameters */
    if (on)
    {
        if (h->i_display_w <= 0 || h->i_display_h <= 0)
        {
            return -EINVAL;
        }
        if (h->i_bps <= 0)
        {
            return -EINVAL;
        }
        if (h->fr_fps.num < h->fr_fps.den || h->fr_fps.den <= 0)
        {
            return -EINVAL;
        }
    }

    mutex_lock(&h->stream_lock);
    if (on)
    {
        int i, total_size, bs_size, gn_size, luma_size, cbcr_size;
        uint32 rambase;
        /* set external parameters */
        h->i_codec_type = REG_ENC_MODE_H264;
        /* reset counters */
        h->i_seq_numb = h->i_cnt_ppic = 0;
        h->i_num_bpic = h->i_cnt_bpic = 0;
        h->i_last_bits = 0;
        h->i_stat_size = 0;
        h->i_pic_avgqp = 0;
        /* intermediate parameters */
        h->i_mb_w = h->i_picture_w>>4;
        h->i_mb_h = h->i_picture_h>>4;
        h->i_mb_wxh = h->i_mb_w * h->i_mb_h;
        h->i_num_ref_frames = 1;
        /* .... */
        h->clk_speed = speed_high;
        h->pixel_fmt = pixel_tile_8x8;
        /* allocate output buffer */
        h->p_outbuf = kzalloc(CPB_SIZE, GFP_KERNEL);
        h->i_outlen = CPB_SIZE;
        h->i_uselen = 0;
        h->i_remlen = 0;
        /* bit rate control */
        h->cvbr.m_nVPMbRow = MFE_ER_MBY;
        h->cvbr.m_nVPSize = MFE_ER_BS_TH;
        /* calculate required buffer size */
        bs_size = CPB_SIZE;
        gn_size = MIU_ALIGN(8,64*h->i_mb_w);
        luma_size = MIU_ALIGN(8,256*h->i_mb_wxh);
        cbcr_size = MIU_ALIGN(8,128*h->i_mb_wxh);
        total_size = gn_size + bs_size + (luma_size + cbcr_size) * (h->i_num_ref_frames + 1);
        /* requeset required dma-buff from system */
        if (!(err = request_kma(ctx->mdev->pdev, &h->kma, total_size)))
        {   /* layout allocated buffer to GN/frames/outbs */
            rambase = h->kma.miu_addr;
            h->p_vptr_bs = h->kma.mem_vptr;
            h->i_base_bs = rambase;
            h->i_size_bs = bs_size;
            rambase += h->i_size_bs;
            h->i_base_gn = rambase;
            h->i_size_gn = gn_size;
            rambase += h->i_size_gn;
            for (i = 0; i <= h->i_num_ref_frames; i++)
            {
                h->frames[i].i_idx = i;
                h->frames[i].b_busy = 0;
                h->frames[i].base_y = rambase;
                rambase += luma_size;
                h->frames[i].base_c = rambase;
                rambase += cbcr_size;
            }
            if (!(err = mfe_hdlr_seq_init(h)))
            {
                mutex_lock(d->lock);
                if (0 == d->i_ctx_count++)
                {
                    mfe_regs_power_on(d->p_pmbase, 1, 4);
                }
                mutex_unlock(d->lock);

                h->b_stream = 1;
            }
            else
            {
                release_kma(ctx->mdev->pdev, &h->kma);
            }
        }
    }
    else
    {
        release_kma(ctx->mdev->pdev, &h->kma);

        printk("cpb<-end->-%8d-\n",h->i_stat_size);

        mfe_hdlr_seq_exit(h);

        mutex_lock(d->lock);
        if (0 == --d->i_ctx_count)
        {
            mfe_regs_power_on(d->p_pmbase, 0, 0);
        }
        mutex_unlock(d->lock);

        h->b_stream = 0;
    }
    mutex_unlock(&h->stream_lock);

    return err;
}

static int
mfe_ops_enc_pict(
    void*           mfh,
    mmfe_buffer*    buf)
{
    mst_ctx* ctx = (mst_ctx*)mfh;
    mfe_hdlr* h = (mfe_hdlr*)ctx->drv_data;
    mfe_asic* d = h->asic;
    void* iobase = d->p_iobase;
    uint irq = 0;
    int err = -EBUSY;

    if (buf->i_memory != MEMORY_MMAP || buf->i_planes != 2)
    {
        return -EINVAL;
    }

    mutex_lock(&h->encode_lock);
    if (!h->i_encode)
    {
        h->i_encode = 1;
        /* picture type decision */
        mfe_hdlr_pic_mode(h);

        h->input.base_y = buf->planes[0].addr;
        h->input.base_c = buf->planes[1].addr;
        h->i_timecode = buf->timecode;

        mfe_hdlr_pic_init(h);
        mfe_hdlr_pic_prep_regs(h);

        mutex_lock(d->lock);
        mfe_regs_enc_fire(iobase, h->regs);
        irq = mfe_regs_enc_wait(iobase);
        mfe_regs_enc_done(iobase, h->regs);
        mutex_unlock(d->lock);

        mfe_hdlr_pic_done(h);
        mfe_hdlr_pic_read_bits(h);

        h->i_encode = 2;
        err = 0;
    }
    mutex_unlock(&h->encode_lock);

    return err;
}

static int
mfe_ops_get_bits(
    void*           mfh,
    mmfe_buffer*    buf)
{
    mst_ctx* ctx = (mst_ctx*)mfh;
    mfe_hdlr* h = (mfe_hdlr*)ctx->drv_data;
    int err = -ENODATA;

    if (buf->i_memory != MEMORY_USER || buf->i_planes != 1)
    {
        return -EINVAL;
    }

    mutex_lock(&h->encode_lock);
    if (h->i_encode > 1)
    {
        int rem = h->i_remlen;
        int use = h->i_uselen;
        uchar* ptr = h->p_outbuf;
        ptr = ptr + use - rem;

        err = -EFAULT;
        if (buf->planes[0].size < rem)
        {
            rem = buf->planes[0].size;
        }
        if (!copy_to_user(buf->planes[0].uptr, ptr, rem))
        {
            buf->planes[0].used = rem;
            buf->timecode = h->i_timecode;
            h->i_remlen -= rem;
            if (!h->i_remlen)
            {
                h->i_encode = 0;
                h->i_uselen = 0;
            }
            err = 0;
            printk("cpb<%5d>-%8d-%2d\n",h->i_seq_numb,rem,h->sh.i_qp);
        }
    }
    mutex_unlock(&h->encode_lock);

    return err;
}

static int
mfe_ops_set_parm(
    void*           mfh,
    mmfe_params*    par)
{
    mst_ctx* ctx = (mst_ctx*)mfh;
    mfe_hdlr* h = (mfe_hdlr*)ctx->drv_data;
    int err = 0;

    mutex_lock(&h->stream_lock);
    if (!h->b_stream)
    {
        switch (par->i_params)
        {
        case MVIDPARAMS_PIXELS:
            h->i_codec_type = REG_ENC_MODE_H264;
            h->i_picture_w = MIU_ALIGN(4,par->pixels.i_pict_w);
            h->i_picture_h = MIU_ALIGN(4,par->pixels.i_pict_h);
            h->i_display_w = par->pixels.i_pict_w;
            h->i_display_h = par->pixels.i_pict_h;
            break;
        case MVIDPARAMS_FRAMERATE:
            h->fr_fps.num = par->framerate.i_num;
            h->fr_fps.den = par->framerate.i_den;
            break;
        case MVIDPARAMS_MOTION:
            h->i_subpel = par->motion.i_subpel;
            h->i_dmv_x = (par->motion.i_dmv_x);
            h->i_dmv_x = h->i_dmv_x < 8 ? 8 : h->i_dmv_x;
            h->i_dmv_x = h->i_dmv_x >16 ?16 : h->i_dmv_x;
            h->i_dmv_y = (par->motion.i_dmv_y+7)&~7;
            h->i_dmv_y = h->i_dmv_y ==8 ? 8 : 16;
            h->i_mv_block[0] = par->motion.i_mv_block[0];
            h->i_mv_block[1] = 0;
            break;
        case MVIDPARAMS_BITRATE:
            h->i_method = par->bitrate.i_method;
            h->i_bps = par->bitrate.i_kbps * 1024;
            h->i_qp = par->bitrate.i_qp;
            break;
        case MVIDPARAMS_GOP:
            h->i_num_ppic = par->gop.i_pframes;
            h->i_num_bpic = 0;
            break;
        case MVIDPARAMS_AVC:
            {
                sps_t* sps = &h->sps;
                pps_t* pps = &h->pps;
                slice_t* sh = &h->sh;
                h->i_num_ref_frames = 1;
                sps->i_profile_idc = 66; // baseline profile
                sps->i_level_idc = 30;
                pps->b_cabac = 0;
                pps->b_deblocking_filter_control = par->avc.b_deblock_filter_control;
                pps->b_constrained_intra_pred = par->avc.b_constrained_intra_pred;
                sh->i_disable_deblocking_filter_idc = par->avc.i_disable_deblocking_idc;
                sh->i_alpha_c0_offset = par->avc.i_alpha_c0_offset;
                sh->i_beta_offset = par->avc.i_beta_offset;
            }
            break;
        default:
            printk("unsupported config\n");
            err = -EINVAL;
            break;
        }
    }
    else
    {
        err = -EINVAL;
    }
    mutex_unlock(&h->stream_lock);

    return err;
}

static int
mfe_ops_get_parm(
    void*           mfh,
    mmfe_params*    par)
{
    mst_ctx* ctx = (mst_ctx*)mfh;
    mfe_hdlr* h = (mfe_hdlr*)ctx->drv_data;
    int err = 0;

    mutex_lock(&h->stream_lock);
    switch (par->i_params)
    {
    case MVIDPARAMS_PIXELS:
        par->pixels.i_pict_w = h->i_display_w;
        par->pixels.i_pict_h = h->i_display_h;
        par->pixels.i_pixfmt = 0;
        break;
    case MVIDPARAMS_FRAMERATE:
        par->framerate.i_num = h->fr_fps.num;
        par->framerate.i_den = h->fr_fps.den;
        break;
    case MVIDPARAMS_MOTION:
        par->motion.i_dmv_x = h->i_dmv_x;
        par->motion.i_dmv_y = h->i_dmv_y;
        par->motion.i_subpel = h->i_subpel;
        par->motion.i_mv_block[0] = h->i_mv_block[0];
        par->motion.i_mv_block[1] = 0;
        break;
    case MVIDPARAMS_BITRATE:
        par->bitrate.i_method = h->i_method;
        par->bitrate.i_kbps = h->i_bps>>10;
        par->bitrate.i_qp = h->i_qp;
        break;
    case MVIDPARAMS_GOP:
        par->gop.i_pframes = h->i_num_ppic;
        par->gop.i_bframes = 0;
        break;
    case MVIDPARAMS_AVC:
        {
            sps_t* sps = &h->sps;
            pps_t* pps = &h->pps;
            slice_t* sh = &h->sh;
            par->avc.i_profile = sps->i_profile_idc;
            par->avc.i_level = sps->i_level_idc;
            par->avc.i_num_ref_frames = 1;
            par->avc.b_cabac = pps->b_cabac;
            par->avc.b_deblock_filter_control = pps->b_deblocking_filter_control;
            par->avc.b_constrained_intra_pred = pps->b_constrained_intra_pred;
            par->avc.i_disable_deblocking_idc = sh->i_disable_deblocking_filter_idc;
            par->avc.i_alpha_c0_offset = sh->i_alpha_c0_offset;
            par->avc.i_beta_offset = sh->i_beta_offset;
            par->avc.b_multiple_slices = 0;
        }
        break;
    default:
        printk("unsupported config\n");
        err = -EINVAL;
        break;
    }
    mutex_unlock(&h->stream_lock);

    return err;
}

