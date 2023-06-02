
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

#include <mdrv_mmfe_io.h>
#include <mdrv_rqct_io.h>

#include <mmfe_defs.h>

MODULE_LICENSE("GPL");

static int  mmfe_open(struct inode*, struct file*);
static int  mmfe_release(struct inode*, struct file*);
static long mmfe_ioctl(struct file*, unsigned int, unsigned long);

static struct file_operations mmfe_fops = {
    .owner = THIS_MODULE,
    .open = mmfe_open,
    .release = mmfe_release,
    .unlocked_ioctl = mmfe_ioctl,
};

static int
mmfe_open(
    struct inode*   inode,
    struct file*    file)
{
    mmfe_dev* mdev = container_of(inode->i_cdev,mmfe_dev,m_cdev);
    mmfe_ctx* mctx;
    if (!(mctx = mmfectx_acquire(mdev)))
        return -ENOMEM;
    if (0 <= mmfedev_register(mdev, mctx))
    {
        file->private_data = mctx;
        return 0;
    }
    mctx->release(mctx);
    return -EINVAL;
}

static int
mmfe_release(
    struct inode*   inode,
    struct file*    file)
{
    mmfe_dev* mdev = container_of(inode->i_cdev,mmfe_dev,m_cdev);
    mmfe_ctx* mctx = file->private_data;
    mmfedev_unregister(mdev, mctx);
    file->private_data = NULL;
    mctx->release(mctx);
    return 0;
}

static long
mmfe_ioctl(
    struct file*    file,
    unsigned int    cmd,
    unsigned long   arg)
{
    mmfe_ctx* mctx = file->private_data;
    size_t n = _IOC_SIZE(cmd);
    char buf[256];
    long err = -EFAULT;
    void __user *uptr = (void __user *)arg;
    unsigned int v = MMFEIF_VERSION_ID;
    int type = _IOC_TYPE(cmd);

    if (type != MAGIC_RQCT && type != MAGIC_MMFE)
        return -EINVAL;
    switch (cmd)
    {
    case IOCTL_RQCT_VERSION:
        v = RQCTIF_VERSION_ID;
        if (access_ok(ACCESS_WRITE,uptr,sizeof(unsigned int)))
            err = __put_user(v,(unsigned int*)uptr);
        break;
    case IOCTL_MMFE_VERSION:
        v = MMFEIF_VERSION_ID;
        if (access_ok(ACCESS_WRITE,uptr,sizeof(unsigned int)))
            err = __put_user(v,(unsigned int*)uptr);
        break;
    default:
        if ((_IOC_DIR(cmd) & _IOC_WRITE) && copy_from_user(buf, uptr, n))
            break;
        if (0 > (err = mmfectx_actions(mctx, cmd, buf)))
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
    mmfe_rqc* mrqc = container_of(inode->i_cdev,mmfe_rqc,m_cdev);
    mmfe_ctx* mctx = NULL;
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
    mmfe_ctx* mctx = file->private_data;
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
        if (0 > (err = mmferqc_ioctls(mctx, cmd, buf)))
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
    size_t lsize = mmferqc_lprint(mctx, line, RQCT_LINE_SIZE);
    if (size > lsize)
        size = lsize;
    if (size > 0 && copy_to_user(uptr, line, size))
        return (ssize_t)err;
    return (ssize_t)size;
}

static void mmfe_dev_release(struct device* dev) {}

static ssize_t mmfe_tmr_store(struct device* dev, struct device_attribute* attr, const char* buff, size_t n)
{
    mmfe_dev* mdev = container_of(dev,mmfe_dev,m_dev);
    int i, thresh = 0;
    if (0 != kstrtol(buff, 10, (long int*)&thresh))
        return -EINVAL;
    mdev->i_thresh = thresh;
    for (i = 0; i < MMFE_STREAM_NR; i++)
        mdev->i_counts[i][0] = mdev->i_counts[i][1] = mdev->i_counts[i][2] = mdev->i_counts[i][3] = mdev->i_counts[i][4] = 0;
    return n;
}

static ssize_t mmfe_tmr_print(struct device* dev, struct device_attribute* attr, char* buff)
{
    char* str = buff;
    char* end = buff + PAGE_SIZE;
    mmfe_dev* mdev = container_of(dev,mmfe_dev,m_dev);
    int i;
    for (i = 0; i < MMFE_STREAM_NR; i++)
        str += scnprintf(str,end-str,"inst-%d:%8d/%5d/%5d/ %8d / %8d \n",i,mdev->i_counts[i][0],mdev->i_counts[i][1],mdev->i_counts[i][2],mdev->i_counts[i][3],mdev->i_counts[i][4]);
    str += scnprintf(str,end-str,"thresh:%8d\n",mdev->i_thresh);
    return (str - buff);
}

static DEVICE_ATTR(tmr,0644,mmfe_tmr_print,mmfe_tmr_store);

static ssize_t mmfe_clk_store(struct device* dev, struct device_attribute* attr, const char* buff, size_t n)
{
    mmfe_dev* mdev = container_of(dev,mmfe_dev,m_dev);
    struct clk* clock = mdev->p_clocks[0];
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
    return n;
}

static ssize_t mmfe_clk_print(struct device* dev, struct device_attribute* attr, char* buff)
{
    char* str = buff;
    char* end = buff + PAGE_SIZE;
    mmfe_dev* mdev = container_of(dev,mmfe_dev,m_dev);
    if (mdev->i_clkidx < 0)
        str += scnprintf(str,end-str,"clkidx:%3d(dynamic)\n",mdev->i_clkidx);
    else
        str += scnprintf(str,end-str,"clkidx:%3d(%3d mhz)\n",mdev->i_clkidx,mdev->i_ratehz/1000000);
    return (str - buff);
}

static DEVICE_ATTR(clk,0644,mmfe_clk_print,mmfe_clk_store);

char* rqctmfe_comment(int);

static ssize_t mmfe_rct_store(struct device* dev, struct device_attribute* attr, const char* buff, size_t n)
{
    mmfe_dev* mdev = container_of(dev,mmfe_dev,m_dev);
    int id = 0;
    char* brief;
    if (0 != kstrtol(buff, 10, (long int*)&id))
        return -EINVAL;
    if ((brief = rqctmfe_comment(id)) && *brief)
        mdev->i_rctidx = id;
    return n;
}

static ssize_t mmfe_rct_print(struct device* dev, struct device_attribute* attr, char* buff)
{
    char* str = buff;
    char* end = buff + PAGE_SIZE;
    mmfe_dev* mdev = container_of(dev,mmfe_dev,m_dev);
    char* brief;
    int j;
    for (j = 0; (brief = rqctmfe_comment(j)); j++)
        if (*brief)
            str += scnprintf(str,end-str,"(%c)[%d]%s\n",j==mdev->i_rctidx?'*':' ',j,brief);
    return (str - buff);
}

static DEVICE_ATTR(rct,0644,mmfe_rct_print,mmfe_rct_store);

static irqreturn_t mmfe_isr(int irq, void* priv);

mhve_ios* mmfeios_acquire(char* tags);

static int
mmfe_probe(
    struct platform_device* pdev)
{
    int i, err = 0;
    dev_t dev;
    mmfe_dev* mdev = NULL;
    mhve_ios* mios = NULL;
    mhve_reg mregs;
    struct resource* res;
    struct clk* clock;
    int major, minor = 0;

    if (0 > (err = alloc_chrdev_region(&dev, minor, MMFE_STREAM_NR+1, "mstar_mmfe")))
        return err;
    major = MAJOR(dev);
    do
    {
        mmfe_rqc* mrqc;
        err = -ENOMEM;
        if (!(mdev = kzalloc(sizeof(mmfe_dev), GFP_KERNEL)))
            break;
        mutex_init(&mdev->m_mutex);
        sema_init(&mdev->m_sem, 1);
        mrqc = mdev->m_regrqc;
        while (minor < MMFE_STREAM_NR)
        {
            cdev_init(&mrqc[minor].m_cdev, &mrqc_fops);
            mrqc[minor].m_cdev.owner = THIS_MODULE;
            if (0 > (err = cdev_add(&mrqc[minor].m_cdev, MKDEV(major,minor), 1)))
                break;
            mrqc[minor].p_mutex = &mdev->m_mutex;
            minor++;
        }
        if (minor < MMFE_STREAM_NR)
            break;
        mdev->i_major = major;
        mdev->i_minor = minor;
        cdev_init(&mdev->m_cdev, &mmfe_fops);
        mdev->m_cdev.owner = THIS_MODULE;
        if (0 > (err = cdev_add(&mdev->m_cdev, MKDEV(major,minor), 1)))
            break;
        mdev->m_dev.devt = MKDEV(major,minor);
        mdev->m_dev.class = msys_get_sysfs_class();
        mdev->m_dev.parent = NULL;
        mdev->m_dev.release = mmfe_dev_release;
        dev_set_name(&mdev->m_dev, "%s", "mmfe");
        if (0 > (err = device_register(&mdev->m_dev)))
            break;
        if (!(mdev->p_asicip = mmfeios_acquire("mfe5")))
            break;
        mios = mdev->p_asicip;
        mdev->p_dev = &pdev->dev;
        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        mregs.i_id = 0;
        mregs.base = (void*)IO_ADDRESS(res->start);
        mregs.size = (int)(res->end - res->start);
        mios->set_bank(mios, &mregs);
        res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
        mdev->i_irq = (int)res->start;
        init_waitqueue_head(&mdev->m_wqh);
        if (0 != (err = request_irq(mdev->i_irq, mmfe_isr, IRQF_SHARED, "mmfe_isr", mdev)))
            break;
        err = -EINVAL;
        clock = clk_get(&pdev->dev, "CKG_mfe");
        if (IS_ERR(clock))
            break;
        mdev->p_clocks[0] = clock;
        mdev->i_clkidx = 0;
        mdev->i_ratehz = clk_get_rate(clk_get_parent_by_index(clock,mdev->i_clkidx));
        for (i = 1; i < MMFE_CLOCKS_NR; i++)
        {
            clock = of_clk_get(pdev->dev.of_node, i);
            if (IS_ERR(clock))
                break;
            mdev->p_clocks[i] = clock;
        }
        mdev->i_rctidx = 1;
        dev_set_drvdata(&pdev->dev, mdev);

        device_create_file(&mdev->m_dev, &dev_attr_tmr);
        device_create_file(&mdev->m_dev, &dev_attr_clk);
        device_create_file(&mdev->m_dev, &dev_attr_rct);
//      printk("mmfe built at %s on %s\n",__TIME__,__DATE__);
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
mmfe_remove(
    struct platform_device* pdev)
{
    mmfe_dev* mdev = dev_get_drvdata(&pdev->dev);
    mhve_ios* mios = mdev->p_asicip;
    int devno = MKDEV(mdev->i_major,0);
    int i = 0;

    free_irq(mdev->i_irq, mmfe_isr);

    clk_put(mdev->p_clocks[0]);

    while (i < MMFE_STREAM_NR)
        cdev_del(&mdev->m_regrqc[i++].m_cdev);
    cdev_del(&mdev->m_cdev);

    device_unregister(&mdev->m_dev);

    if (mios)
        mios->release(mios);
    kfree(mdev);

    dev_set_drvdata(&pdev->dev, NULL);

    unregister_chrdev_region(devno, MMFE_STREAM_NR+1);

    return 0;
}

static int
mmfe_suspend(
    struct platform_device* pdev,
    pm_message_t            state)
{
    mmfe_dev* mdev = dev_get_drvdata(&pdev->dev);

    return mmfedev_suspend(mdev);
}

static int
mmfe_resume(
    struct platform_device* pdev)
{
    mmfe_dev* mdev = dev_get_drvdata(&pdev->dev);

    return mmfedev_resume(mdev);
}

static irqreturn_t mmfe_isr(int irq, void* priv)
{
    mmfe_dev* mdev = (mmfe_dev*)priv;

    mmfedev_isr_fnx(mdev);

    return IRQ_HANDLED;
}

static const struct of_device_id mmfe_of_match_tables[] = {
	{ .compatible = "mstar,mfe" },
	{},
};

static struct platform_driver mmfe_pdrv = {
    .probe = mmfe_probe,
    .remove = mmfe_remove,
    .suspend = mmfe_suspend,
    .resume = mmfe_resume,
    .driver = {
        .name = "mmfe",
        .owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mmfe_of_match_tables),
    }
};

static int  __init mmfe_init(void)
{
    return platform_driver_register(&mmfe_pdrv);
}

static void __exit mmfe_exit(void)
{
    platform_driver_unregister(&mmfe_pdrv);
}

module_init(mmfe_init);
module_exit(mmfe_exit);

