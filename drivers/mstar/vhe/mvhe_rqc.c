
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

#include <mdrv_mvhe_io.h>
#include <mdrv_rqct_io.h>

#include <mvhe_defs.h>

long
mvherqc_ioctls(
    void*           pctx,
    unsigned int    cmd,
    void*           arg)
{
    int err = -EINVAL;
    mvhe_ctx* mctx = pctx;
    mutex_lock(&mctx->m_stream);
    if (NULL != mctx->p_handle)
    {
        mhve_ops* mops = mctx->p_handle;
        rqct_ops* rqct = mops->rqct_ops(mops);
        switch (cmd)
        {
        case IOCTL_RQCT_S_CONF:
            if (!rqct->set_rqcf(rqct, (rqct_cfg*)arg))
                err = 0;
            break;
        case IOCTL_RQCT_G_CONF:
            if (!rqct->get_rqcf(rqct, (rqct_cfg*)arg))
                err = 0;
            break;
        default:
            break;
        }
    }
    mutex_unlock(&mctx->m_stream);
    return err;
}

size_t
mvherqc_lprint(
    void*   pctx,
    char*   line,
    size_t  size)
{
    mvhe_ctx* mctx = pctx;
    mhve_ops* mops = mctx->p_handle;
    rqct_ops* rqct = NULL;
    if (size > RQCT_LINE_SIZE)
        size = RQCT_LINE_SIZE;
    mutex_lock(&mctx->m_stream);
    if (!mops)
        size = 0;
    else
    {
        rqct = mops->rqct_ops(mops);
        memset(line,0,size);
        strncpy(line, rqct->print_line, size-1);
        memset(rqct->print_line, 0, RQCT_LINE_SIZE);
    }
    mutex_unlock(&mctx->m_stream);
    return size;
}
