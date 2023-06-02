
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

#if defined(MVHE_MCM_FORCE_OFF)
#include <mcm_id.h>
#endif
#include <ms_platform.h>
#include <ms_msys.h>

#include <mdrv_mvhe_io.h>
#include <mdrv_rqct_io.h>

#include <mhve_ios.h>
#include <mhve_ops.h>
#include <rqct_ops.h>

#include <mvhe_defs.h>

char*     rqctvhe_comment(int);
mhve_ios* mvheios_acquire(char* tags);

MODULE_DESCRIPTION("VHE Linux Kernel Driver Module");
MODULE_AUTHOR("Wenho Lee <wenho.lee@mstarsemi.com>");
MODULE_LICENSE("GPL");

static int  mvhe_open(struct inode*, struct file*);
static int  mvhe_release(struct inode*, struct file*);
static long mvhe_ioctl(struct file*, unsigned int, unsigned long);

static struct file_operations mvhe_fops = {
    .owner = THIS_MODULE,
    .open = mvhe_open,
    .release = mvhe_release,
    .unlocked_ioctl = mvhe_ioctl,
};

static int
mvhe_open(
    struct inode*   inode,
    struct file*    file)
{
    mvhe_dev* mdev = container_of(inode->i_cdev,mvhe_dev,m_cdev);
    mvhe_ctx* mctx;
    if (!(mctx = mvhectx_acquire(mdev)))
        return -ENOMEM;
    if (0 <= mvhedev_register(mdev, mctx))
    {
        file->private_data = mctx;
        return 0;
    }
    mctx->release(mctx);
    return -EINVAL;
}

static int
mvhe_release(
    struct inode*   inode,
    struct file*    file)
{
    mvhe_dev* mdev = container_of(inode->i_cdev,mvhe_dev,m_cdev);
    mvhe_ctx* mctx = file->private_data;
    mvhedev_unregister(mdev, mctx);
    file->private_data = NULL;
    mctx->release(mctx);
    return 0;
}

static long
mvhe_ioctl(
    struct file*    file,
    unsigned int    cmd,
    unsigned long   arg)
{
    mvhe_ctx* mctx = file->private_data;
    size_t n = _IOC_SIZE(cmd);
    char buf[256];
    long err = -EFAULT;
    void __user *uptr = (void __user *)arg;
    unsigned int v = MVHEIF_VERSION_ID;
    int type = _IOC_TYPE(cmd);

    if (type != MAGIC_RQCT && type != MAGIC_MVHE)
        return -EINVAL;
    switch (cmd)
    {
    case IOCTL_RQCT_VERSION:
        if (access_ok(ACCESS_WRITE,uptr,sizeof(unsigned int)))
        {
            v = RQCTIF_VERSION_ID;
            err = __put_user(v,(unsigned int*)uptr);
        }
        break;
    case IOCTL_MVHE_VERSION:
        if (access_ok(ACCESS_WRITE,uptr,sizeof(unsigned int)))
        {
            v = MVHEIF_VERSION_ID;
            err = __put_user(v,(unsigned int*)uptr);
        }
        break;
    default:
        if ((_IOC_DIR(cmd) & _IOC_WRITE) && copy_from_user(buf, uptr, n))
            break;
        if (0 > (err = mvhectx_actions(mctx, cmd, buf)))
            break;
        if ((_IOC_DIR(cmd) & _IOC_READ) && copy_to_user(uptr, buf, n))
            break;
        err = 0;
        break;
    }

    return err;
}

static int mrqc_open(struct inode*, struct file*);
static int mrqc_release(struct inode*, struct file*);
static long mrqc_ioctl(struct file*, unsigned int, unsigned long);
static ssize_t mrqc_read(struct file*, char __user*, size_t, loff_t*);

static struct file_operations mrqc_fops = {
    .owner = THIS_MODULE,
    .open = mrqc_open,
    .release = mrqc_release,
    .unlocked_ioctl = mrqc_ioctl,
    .read = mrqc_read,
};

static int
mrqc_open(
    struct inode*   inode,
    struct file*    file)
{
    int err = -EINVAL;
    mvhe_rqc* mrqc = container_of(inode->i_cdev,mvhe_rqc,m_cdev);
    mvhe_ctx* mctx = NULL;
    mutex_lock(mrqc->p_mutex);
    if (NULL != (mctx = mrqc->p_mctx))
    {
        mctx->adduser(mctx);
        file->private_data = mctx;
        err = 0;
    }
    mutex_unlock(mrqc->p_mutex);
    return err;
}

static int
mrqc_release(
    struct inode*   inode,
    struct file*    file)
{
    mvhe_ctx* mctx = file->private_data;
    mctx->release(mctx);
    return 0;
}

static long
mrqc_ioctl(
    struct file*    file,
    unsigned int    cmd,
    unsigned long   arg)
{
    size_t n = _IOC_SIZE(cmd);
    char buf[256];
    long err = -EFAULT;
    void __user *uptr = (void __user *)arg;
    void* mctx = file->private_data;
    unsigned int v = RQCTIF_VERSION_ID;

    if (_IOC_TYPE(cmd) != MAGIC_RQCT)
        return -EINVAL;
    switch (cmd)
    {
    case IOCTL_RQCT_VERSION:
        if (access_ok(ACCESS_WRITE,uptr,sizeof(unsigned int)))
            err = __put_user(v,(unsigned int*)uptr);
        break;
    default:
        if ((_IOC_DIR(cmd) & _IOC_WRITE) && copy_from_user(buf, uptr, n))
            break;
        if (0 > (err = mvherqc_ioctls(mctx, cmd, buf)))
            break;
        if ((_IOC_DIR(cmd) & _IOC_READ) && copy_to_user(uptr, buf, n))
            break;
        err = 0;
        break;
    }

    return err;
}

static ssize_t
mrqc_read(
    struct file*    file,
    char __user*    buff,
    size_t          size,
    loff_t*         fpos)
{
    char line[RQCT_LINE_SIZE];
    long err = -EFAULT;
    void __user *uptr = (void __user*)buff;
    void* mctx = file->private_data;
    size_t lsize = mvherqc_lprint(mctx, line, RQCT_LINE_SIZE);
    if (size > lsize)
        size = lsize;
    if (size > 0 && copy_to_user(uptr, line, size))
        return (ssize_t)err;
    return (ssize_t)size;
}

static void mvhe_dev_release(struct device* dev) {}

static ssize_t mvhe_tmr_store(struct device* dev, struct device_attribute* attr, const char* buff, size_t n)
{
    mvhe_dev* mdev = container_of(dev,mvhe_dev,m_dev);
    int i, thresh = 0;
    if (0 != kstrtol(buff, 10, (long int*)&thresh))
        return -EINVAL;
    mdev->i_thresh = thresh;
    for (i = 0; i < MVHE_STREAM_NR; i++)
        mdev->i_counts[i][0] = mdev->i_counts[i][1] = mdev->i_counts[i][2] = mdev->i_counts[i][3] = mdev->i_counts[i][4] = 0;
    return n;
}

static ssize_t mvhe_tmr_print(struct device* dev, struct device_attribute* attr, char* buff)
{
    char* str = buff;
    char* end = buff + PAGE_SIZE;
    mvhe_dev* mdev = container_of(dev,mvhe_dev,m_dev);
    int i;
    for (i = 0; i < MVHE_STREAM_NR; i++)
        str += scnprintf(str,end-str,"inst-%d:%8d/%5d/%5d/ %8d / %8d\n", i, mdev->i_counts[i][0], mdev->i_counts[i][1], mdev->i_counts[i][2], mdev->i_counts[i][3], mdev->i_counts[i][4]);
    str += scnprintf(str,end-str,"thresh:%8d\n",mdev->i_thresh);
    return (str - buff);
}

static DEVICE_ATTR(tmr,0644,mvhe_tmr_print,mvhe_tmr_store);

static ssize_t mvhe_clk_store(struct device* dev, struct device_attribute* attr, const char* buff, size_t n)
{
    mvhe_dev* mdev = container_of(dev,mvhe_dev,m_dev);
    struct clk* clock = clk_get(mdev->p_dev, "CKG_vhe");
    struct clk* ck = NULL;
    int id = 0;
    if (0 != kstrtol(buff, 10, (long int*)&id))
        return -EINVAL;
    ck = clk_get_parent_by_index(clock, id);
    if (ck == NULL)
        mdev->i_ratehz = mdev->i_clkidx = -1;
    else
    {
        mdev->i_clkidx = id;
        mdev->i_ratehz = clk_get_rate(ck);
    }
    clk_put(clock);
    return n;
}

static ssize_t mvhe_clk_print(struct device* dev, struct device_attribute* attr, char* buff)
{
    char* str = buff;
    char* end = buff + PAGE_SIZE;
    mvhe_dev* mdev = container_of(dev,mvhe_dev,m_dev);
    if (mdev->i_clkidx < 0)
        str += scnprintf(str,end-str,"clkid:%3d(dynamic)\n",mdev->i_clkidx);
    else
        str += scnprintf(str,end-str,"clkid:%3d(%3d mhz)\n",mdev->i_clkidx,mdev->i_ratehz/1000000);
    return (str - buff);
}

static DEVICE_ATTR(clk,0644,mvhe_clk_print,mvhe_clk_store);

static ssize_t mvhe_rct_store(struct device* dev, struct device_attribute* attr, const char* buff, size_t n)
{
    mvhe_dev* mdev = container_of(dev,mvhe_dev,m_dev);
    int id = 0;
    char* brief;
    if (0 != kstrtol(buff, 10, (long int*)&id))
        return -EINVAL;
    if ((brief = rqctvhe_comment(id)) && *brief)
        mdev->i_rctidx = id;
    return n;
}

static ssize_t mvhe_rct_print(struct device* dev, struct device_attribute* attr, char* buff)
{
    char* str = buff;
    char* end = buff + PAGE_SIZE;
    mvhe_dev* mdev = container_of(dev,mvhe_dev,m_dev);
    char* brief;
    int j;
    for (j = 0; (brief = rqctvhe_comment(j)); j++)
        if (*brief)
            str += scnprintf(str,end-str,"(%c)[%d]%s\n",j==mdev->i_rctidx?'*':' ',j,brief);
    return (str - buff);
}

static DEVICE_ATTR(rct,0644,mvhe_rct_print,mvhe_rct_store);

static irqreturn_t mvhe_isr(int irq, void* priv);

static int
mvhe_probe(
    struct platform_device* pdev)
{
    int i, err = 0;
    dev_t dev;
    mvhe_dev* mdev = NULL;
    mhve_ios* mios = NULL;
    mhve_reg mregs;
    struct resource* res;
    struct clk* clock;
    int major, minor = 0;

    if (0> (err = alloc_chrdev_region(&dev, minor, 1, "mstar_mvhe")))
        return err;
    major = MAJOR(dev);
    do
    {
        mvhe_rqc* mrqc;
        err = -ENOMEM;
        if (!(mdev = kzalloc(sizeof(struct mvhe_dev), GFP_KERNEL)))
            break;
        mutex_init(&mdev->m_mutex);
        sema_init(&mdev->m_sem, 1);
        mrqc = mdev->m_regrqc;
        while (minor < MVHE_STREAM_NR)
        {
            cdev_init(&mrqc[minor].m_cdev, &mrqc_fops);
            mrqc[minor].m_cdev.owner = THIS_MODULE;
            if (0 > (err = cdev_add(&mrqc[minor].m_cdev, MKDEV(major,minor), 1)))
                break;
            mrqc[minor].p_mutex = &mdev->m_mutex;
            minor++;
        }
        if (minor < MVHE_STREAM_NR)
            break;
        mdev->i_major = major;
        mdev->i_minor = minor;
        cdev_init(&mdev->m_cdev, &mvhe_fops);
        mdev->m_cdev.owner = THIS_MODULE;
        if (0 > (err = cdev_add(&mdev->m_cdev, MKDEV(major,minor), 1)))
            break;
        mdev->m_dev.devt = MKDEV(major,minor);
        mdev->m_dev.class = msys_get_sysfs_class();
        mdev->m_dev.parent = NULL;
        mdev->m_dev.release = mvhe_dev_release;
        dev_set_name(&mdev->m_dev, "%s", "mvhe");
        if (0 > (err = device_register(&mdev->m_dev)))
            break;
        if (!(mdev->p_asicip = mvheios_acquire("h2v1")))
            break;
        mios = mdev->p_asicip;
        mdev->p_dev = &pdev->dev;
        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        mregs.i_id = 0;
        mregs.base = (void*)IO_ADDRESS(res->start);
        mregs.size = (int)(res->end - res->start);
        mios->set_bank(mios, &mregs);
        res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
        mregs.i_id = 1;
        mregs.base = (void*)IO_ADDRESS(res->start);
        mregs.size = (int)(res->end - res->start);
        mios->set_bank(mios, &mregs);
        res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
        mdev->i_irq = (int)res->start;
        init_waitqueue_head(&mdev->m_wqh);
        if (0 != (err = request_irq(mdev->i_irq, mvhe_isr, IRQF_SHARED, "mvhe_isr", mdev)))
            break;
        err = -EINVAL;
        clock = clk_get(&pdev->dev, "CKG_vhe");
        if (IS_ERR(clock))
            break;
        mdev->p_clocks[0] = clock;
        mdev->i_clkidx = 1;
        mdev->i_ratehz = clk_get_rate(clk_get_parent_by_index(clock,mdev->i_clkidx));
        for (i = 1; i < MVHE_CLOCKS_NR; i++)
        {
            clock = of_clk_get(pdev->dev.of_node, i);
            if (IS_ERR(clock))
                break;
            mdev->p_clocks[i] = clock;
        }
        mdev->i_rctidx = 2;
        dev_set_drvdata(&pdev->dev, mdev);

        device_create_file(&mdev->m_dev, &dev_attr_tmr);
        device_create_file(&mdev->m_dev, &dev_attr_clk);
        device_create_file(&mdev->m_dev, &dev_attr_rct);
//      printk("mvhe built at %s on %s\n",__TIME__,__DATE__);
#if defined(MVHE_MCM_FORCE_OFF)
        /* disable vhe-mcm */
        Chip_Function_Set(CHIP_FUNC_MCM_DISABLE_ID, MCM_ID_VHE);
#endif
        return 0;
    }
    while (0);

    if (mdev)
    {
        mhve_ios* mios = mdev->p_asicip;
        cdev_del(&mdev->m_cdev);
        if (mios)
            mios->release(mios);
        kfree(mdev);
    }

    return err;
}

static int
mvhe_remove(
    struct platform_device* pdev)
{
    mvhe_dev* mdev = dev_get_drvdata(&pdev->dev);
    mhve_ios* mios = mdev->p_asicip;
    int devno = MKDEV(mdev->i_major,0);
    int i = 0;

    free_irq(mdev->i_irq, mvhe_isr);

    clk_put(mdev->p_clocks[0]);

    while (i < MVHE_STREAM_NR)
        cdev_del(&mdev->m_regrqc[i++].m_cdev);
    cdev_del(&mdev->m_cdev);

    device_unregister(&mdev->m_dev);

    if (mios)
        mios->release(mios);
    kfree(mdev);

    dev_set_drvdata(&pdev->dev, NULL);

    unregister_chrdev_region(devno, MVHE_STREAM_NR+1);

    return 0;
}

static int
mvhe_suspend(
    struct platform_device* pdev,
    pm_message_t            state)
{
    mvhe_dev* mdev = dev_get_drvdata(&pdev->dev);

    return mvhedev_suspend(mdev);
}

static int
mvhe_resume(
    struct platform_device* pdev)
{
    mvhe_dev* mdev = dev_get_drvdata(&pdev->dev);

    return mvhedev_resume(mdev);
}

static irqreturn_t mvhe_isr(int irq, void* priv)
{
    mvhe_dev* mdev = (mvhe_dev*)priv;

    mvhedev_isr_fnx(mdev);
    return IRQ_HANDLED;
}

static const struct of_device_id mvhe_of_match_tables[] = {
    { .compatible = "mstar,vhe" },
    {},
};
static struct platform_driver mvhe_pdrv = {
    .probe = mvhe_probe,
    .remove = mvhe_remove,
    .suspend = mvhe_suspend,
    .resume = mvhe_resume,
    .driver = {
        .name = "mvhe",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(mvhe_of_match_tables),
    }
};

static int  __init mvhe_init(void)
{
    return platform_driver_register(&mvhe_pdrv);
}

static void __exit mvhe_exit(void)
{
    platform_driver_unregister(&mvhe_pdrv);
}

module_init(mvhe_init);
module_exit(mvhe_exit);

