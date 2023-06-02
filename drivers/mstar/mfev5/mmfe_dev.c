
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

#include <ms_platform.h>
#include <ms_msys.h>

#include <mmfe_defs.h>

int
mmfedev_register(
    mmfe_dev*   mdev,
    mmfe_ctx*   mctx)
{
    int i = 0;
    mutex_lock(&mdev->m_mutex);
    mctx->i_index = -1;
    while (i < MMFE_STREAM_NR)
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
mmfedev_unregister(
    mmfe_dev*   mdev,
    mmfe_ctx*   mctx)
{
    int id = mctx->i_index;
    mutex_lock(&mdev->m_mutex);
    mdev->m_regrqc[id].p_mctx = NULL;
    mctx->p_device = NULL;
    mctx->i_index = -1;
    mutex_unlock(&mdev->m_mutex);
    return 0;
}

static int dev_clkrate(mmfe_dev*, int);
static int dev_poweron(mmfe_dev*, int);

int
mmfedev_poweron(
    mmfe_dev*   mdev,
    int         rate)
{
    if (0 == rate)
        return 0;
    down(&mdev->m_sem);
    do
    {
        mdev->i_score += rate;
        if (mdev->i_users > 0)
            dev_poweron(mdev, 0);
        if (rate > 0)
            mdev->i_users++;
        else if (--mdev->i_users == 0)
            break;
        if (mdev->i_clkidx < 0)
            mdev->i_ratehz = dev_clkrate(mdev, mdev->i_score*900);
        dev_poweron(mdev, 1);
    }
    while (0);
    up(&mdev->m_sem);
    return 0;
}

static int
dev_clkrate(
    mmfe_dev*   mdev,
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

static int
dev_poweron(
    mmfe_dev*   mdev,
    int         on)
{
    int i = on!=0;
    struct clk* clk;
    mhve_ios* mios = mdev->p_asicip;
    mios->irq_mask(mios, 0xFF);
    if (i == 0)
    {
        clk = mdev->p_clocks[0];
        clk_set_parent(clk, clk_get_parent_by_index(clk, 0));
        while (i < MMFE_CLOCKS_NR && (clk = mdev->p_clocks[i++]))
            clk_disable_unprepare(clk);
        return 0;
    }
    while (i < MMFE_CLOCKS_NR && (clk = mdev->p_clocks[i++]))
        clk_prepare_enable(clk);
    clk = mdev->p_clocks[0];
    clk_set_rate(clk, mdev->i_ratehz);
    clk_prepare_enable(clk);
    return 0;
}

int
mmfedev_suspend(
    mmfe_dev*   mdev)
{
    down(&mdev->m_sem);
    if (mdev->i_users > 0)
        dev_poweron(mdev, 0);
    return 0;
}

int
mmfedev_resume(
    mmfe_dev*   mdev)
{
    if (mdev->i_users > 0)
        dev_poweron(mdev, 1);
    up(&mdev->m_sem);
    return 0;
}

static uint gettime(void)
{
    struct timeval tv;
    do_gettimeofday(&tv);
    return (uint)((tv.tv_sec*1000000+tv.tv_usec+500)/1000);
}

int
mmfedev_pushjob(
    mmfe_dev*   mdev,
    mmfe_ctx*   mctx)
{
    mhve_ops* mops = mctx->p_handle;
    mhve_ios* mios = mdev->p_asicip;
    mhve_job* mjob = mops->mhve_job(mops);
    int otmr, itmr, id = mctx->i_index;
    int err = 0;
    long tick;

    otmr = gettime();
    down(&mdev->m_sem);
    itmr = gettime();
    mdev->i_state = MMFE_DEV_STATE_BUSY;
    mios->enc_fire(mios, mjob);
    if (0 == (tick = wait_event_timeout(mdev->m_wqh, mdev->i_state==MMFE_DEV_STATE_IDLE, 60)))
    {
        mjob->i_code = MHVEJOB_TIME_OUT;
        printk(KERN_ERR"mfe-wait event to(%ld)\n", tick);
        err = -1;
    }
    itmr = gettime() - itmr;
    up(&mdev->m_sem);
    otmr = gettime() - otmr;

    if (otmr > mdev->i_thresh && mdev->i_thresh > 0)
        mdev->i_counts[id][0]++;
    if (otmr > mdev->i_counts[id][1])
        mdev->i_counts[id][1] = otmr;
    if (itmr > mdev->i_counts[id][2])
        mdev->i_counts[id][2] = itmr;
    if (mjob->i_tick > mdev->i_counts[id][3])
        mdev->i_counts[id][3] = mjob->i_tick;
#if MMFE_TIMER_SIZE>0
    id = mctx->i_numbr&((MMFE_TIMER_SIZE/8)-1);
    if(id==0){mctx->i_numbr = 0;}
    mdev->i_counts[mctx->i_index][4] -= mctx->p_timer[id].tm_cycles/(MMFE_TIMER_SIZE/8);
    mctx->p_timer[id].tm_dur[0] = (unsigned char)otmr;
    mctx->p_timer[id].tm_dur[1] = (unsigned char)itmr;
    mctx->p_timer[id].tm_cycles = (int)mjob->i_tick;
    mctx->i_numbr++;
    mdev->i_counts[mctx->i_index][4] += mctx->p_timer[id].tm_cycles/(MMFE_TIMER_SIZE/8);
#endif
    return err;
}

int
mmfedev_isr_fnx(
    mmfe_dev*   mdev)
{
    mhve_ios* mios = mdev->p_asicip;
    //if (!mios->isr_func(mios, 0))
    //{
    //    mdev->i_state = MMFE_DEV_STATE_IDLE;
    //    wake_up(&mdev->m_wqh);
    //}
    mios->isr_func(mios, 0);
    mdev->i_state = MMFE_DEV_STATE_IDLE;
    wake_up(&mdev->m_wqh);
    return 0;
}
