
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <ms_platform.h>
#include <ms_msys.h>

#include <mvhe_defs.h>

int
mvhedev_register(
    mvhe_dev*   mdev,
    mvhe_ctx*   mctx)
{
    int i = 0;
    mutex_lock(&mdev->m_mutex);
    mctx->i_index = -1;
    while (i < MVHE_STREAM_NR)
    {
        if (mdev->m_regrqc[i++].p_mctx)
            continue;
        mdev->m_regrqc[--i].p_mctx = mctx;
        mctx->p_device = mdev;
        mctx->i_index = i;
        break;
    }
    mutex_unlock(&mdev->m_mutex);
    return mctx->i_index;
}

int
mvhedev_unregister(
    mvhe_dev*   mdev,
    mvhe_ctx*   mctx)
{
    int id = mctx->i_index;
    mutex_lock(&mdev->m_mutex);
    mdev->m_regrqc[id].p_mctx = NULL;
    mctx->p_device = NULL;
    mctx->i_index = -1;
    mutex_unlock(&mdev->m_mutex);
    return 0;
}

static int dev_poweron(mvhe_dev*, int);
static int dev_clkrate(mvhe_dev*, int);

int
mvhedev_poweron(
    mvhe_dev*   mdev,
    int         rate)
{
    if (0 == rate)
        return 0;
    down(&mdev->m_sem);
    do
    {
        mdev->i_score += rate;
#if defined(MVHE_MCM_ENABLE)
        if (mdev->i_users > 0)
            dev_poweron(mdev, 0);
#endif
        if (rate > 0)
            mdev->i_users++;
        else if (--mdev->i_users == 0)
            break;
        if (mdev->i_clkidx < 0)
            mdev->i_ratehz = dev_clkrate(mdev, mdev->i_score*8000);
#if defined(MVHE_MCM_ENABLE)
        dev_poweron(mdev, 1);
#endif
    }
    while (0);
    up(&mdev->m_sem);
    return 0;
}

static int
dev_poweron(
    mvhe_dev*   mdev,
    int         on)
{
    int i = on!=0;
    mhve_ios* mios = mdev->p_asicip;
    struct clk* clk = mdev->p_clocks[0];
    mios->irq_mask(mios, 0x0F);
    if (i == 0)
    {
        clk_set_parent(clk, clk_get_parent_by_index(clk, 0));
        while (i < MVHE_CLOCKS_NR && (clk = mdev->p_clocks[i++]))
            clk_disable_unprepare(clk);
        return 0;
    }
    while (i < MVHE_CLOCKS_NR && (clk = mdev->p_clocks[i++]))
        clk_prepare_enable(clk);
    clk = mdev->p_clocks[0];
    clk_set_rate(clk, mdev->i_ratehz);
    clk_prepare_enable(clk);
    return 0;
}

static int
dev_clkrate(
    mvhe_dev*   mdev,
    int         rate)
{
    struct clk* clock = mdev->p_clocks[0];
    struct clk* clk;
    int i = 0, best = 0;
    while (!(clk = clk_get_parent_by_index(clock, i++)))
    {
        int r = clk_get_rate(clk);

        if (rate > best && best < r)
            best = r;
        if (rate < best && rate < r && r < best)
            best = r;
    }
    return best;
}

int
mvhedev_suspend(
    mvhe_dev*   mdev)
{
    down(&mdev->m_sem);
#if defined(MVHE_MCM_ENABLE)
    if (mdev->i_users > 0)
        dev_poweron(mdev, 0);
#endif
    return 0;
}

int
mvhedev_resume(
    mvhe_dev*   mdev)
{
#if defined(MVHE_MCM_ENABLE)
    if (mdev->i_users > 0)
        dev_poweron(mdev, 1);
#endif
    up(&mdev->m_sem);
    return 0;
}

static uint
gettime(void)
{
    struct timeval tv;
    do_gettimeofday(&tv);
    return (uint)((tv.tv_sec*1000000+tv.tv_usec+500)/1000);
}

int
mvhedev_pushjob(
    mvhe_dev*   mdev,
    mvhe_ctx*   mctx)
{
    mhve_ops* mops = mctx->p_handle;
    mhve_ios* mios = mdev->p_asicip;
    mhve_job* mjob = mops->mhve_job(mops);
    int err = 0, ot, it, id = mctx->i_index;
    long tick;

    /* tick calculate */
    ot = gettime();
    down(&mdev->m_sem);
    it = gettime();
#if !defined(MVHE_MCM_ENABLE)
    dev_poweron(mdev, 1);
#endif
    mdev->i_state = MVHE_DEV_STATE_BUSY;
    /* set register to hw */
    mios->enc_fire(mios, mjob);
    /* wait encode done interrupt */
    tick = wait_event_timeout(mdev->m_wqh, (mdev->i_state!=MVHE_DEV_STATE_BUSY), msecs_to_jiffies(1000));
//    if (0 == tick || tick == msecs_to_jiffies(1000))
    if (0 == tick || mdev->i_state != MVHE_DEV_STATE_IDLE)
    {
        mjob->i_code = MHVEJOB_TIME_OUT;
        printk(KERN_ERR"vhe-wait event to(%ld)\n", tick);
        err = -1;
    }
#if !defined(MVHE_MCM_ENABLE)
    dev_poweron(mdev, 0);
#endif
    /* tick calculate */
    it = gettime() - it;
    up(&mdev->m_sem);
    ot = gettime() - ot;

    if (ot > mdev->i_thresh && mdev->i_thresh > 0)
        mdev->i_counts[id][0]++;
    if (ot > mdev->i_counts[id][1])
        mdev->i_counts[id][1] = ot;
    if (it > mdev->i_counts[id][2])
        mdev->i_counts[id][2] = it;
    if ((int)mjob->i_tick > mdev->i_counts[id][3])
        mdev->i_counts[id][3] = (int)mjob->i_tick;
#if MVHE_TIMER_SIZE>0
    id = mctx->i_numbr&((MVHE_TIMER_SIZE/8)-1);
    if(id == 0){mctx->i_numbr = 0;}
    mdev->i_counts[mctx->i_index][4] -= mctx->p_timer[id].tm_cycles/(MVHE_TIMER_SIZE/8);
    mctx->p_timer[id].tm_dur[0] = (unsigned char)ot;
    mctx->p_timer[id].tm_dur[1] = (unsigned char)it;
    mctx->p_timer[id].tm_cycles = (int)mjob->i_tick;
    mctx->i_numbr++;
    mdev->i_counts[mctx->i_index][4] += mctx->p_timer[id].tm_cycles/(MVHE_TIMER_SIZE/8);
#endif
    return err;
}

int
mvhedev_isr_fnx(
    mvhe_dev*   mdev)
{
    mhve_ios* mios = mdev->p_asicip;
    mios->isr_func(mios, 0);
    mdev->i_state = MVHE_DEV_STATE_IDLE;
    wake_up(&mdev->m_wqh);
    return 0;
}
