
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#include <mdrv_mfe_io.h>

#include <mmfe_ops.h>

#include <mst_def.h>
#include <types.h>
#include <mfe_avc.h>
#include <mfe_rc.h>
#include <mfe_regs.h>
#include <mfe_asic.h>
#include <mfe_hdlr.h>

MODULE_LICENSE("GPL");

static mst_dev* mstmfe;

static int
mstmfe_open(
    struct inode*   inode,
    struct file*    file)
{
    mst_ctx* ctx = kzalloc(sizeof(mst_ctx), GFP_KERNEL);
    mfe_asic* d = mstmfe->dev_data;

    if (ctx)
    {
        mfe_hdlr* h = NULL;
        ctx->mdev = mstmfe;
        if (mfe_hdlr_request(&ctx->drv_data))
        {
            kfree(ctx);
            return -ENOMEM;
        }
        h = ctx->drv_data;
        h->asic = d;
        file->private_data = ctx;
        return 0;
    }

    return -ENOMEM;
}

static int
mstmfe_release(
    struct inode*   inode,
    struct file*    file)
{
    mst_ctx* ctx = file->private_data;
    mfe_hdlr* h = (mfe_hdlr*)ctx->drv_data;

    ctx->mdev = NULL;
    if (h && h->regs)
    {
        kfree(h->regs);
    }
    mfe_hdlr_release(h);
    kfree(ctx);
    file->private_data = NULL;

    return 0;
}

static long
mstmfe_ioctl(
    struct file*    file,
    unsigned int    cmd,
    unsigned long   arg)
{
    size_t n = _IOC_SIZE(cmd);
    char buf[256];
    long err = 0;
    void __user *uptr = (void __user *)arg;
    void* mfh = file->private_data;

    if ((_IOC_DIR(cmd) & _IOC_WRITE) && copy_from_user(buf, uptr, n))
    {
        return -EFAULT;
    }
    if (0 != (err = mstmfe_ops(mfh, cmd, buf)))
    {
        return err;
    }
    if ((_IOC_DIR(cmd) & _IOC_READ) && copy_to_user(uptr, buf, n))
    {
        return -EFAULT;
    }

    return 0;
}

static struct class mstar_class = {
    .name = "mstar",
    .owner = THIS_MODULE,
};

struct file_operations mst_fops = {
    .owner = THIS_MODULE,
    .open = mstmfe_open,
    .release = mstmfe_release,
    .unlocked_ioctl = mstmfe_ioctl,
};

static void mstmfe_dev_release(struct device* dev) {}

static int mstmfe_major = 0;
static int mstmfe_minor = 0;

static int
mstmfe_probe(
    struct platform_device* pdev)
{
    int err = 0;
    dev_t dev;
    mfe_asic* d = NULL;
    struct resource* res;

    if (0 > (err = alloc_chrdev_region(&dev, mstmfe_minor, 1, "mstar")))
    {
        return err;
    }

    mstmfe_major = MAJOR(dev);

    if (!(mstmfe = kzalloc(sizeof(mst_dev), GFP_KERNEL)))
    {
        goto err_exit;
    }
    d = kzalloc(sizeof(mfe_asic), GFP_KERNEL);
    mstmfe->dev_data = d;

    if (NULL == (mstmfe->cdev = cdev_alloc()))
    {
        goto err_exit;
    }
    mstmfe->cdev->ops = &mst_fops;
    mstmfe->cdev->owner = THIS_MODULE;

    if (0 > (err = cdev_add(mstmfe->cdev,MKDEV(mstmfe_major,mstmfe_minor), 1)))
    {
        return err;
    }
    mstmfe->ldev.devt = MKDEV(mstmfe_major,mstmfe_minor);
    mstmfe->ldev.class = &mstar_class;
    mstmfe->ldev.parent = NULL;
    mstmfe->ldev.release = mstmfe_dev_release;
    dev_set_name(&mstmfe->ldev, "%s", "mmfe");

    if (0 > (err = device_register(&mstmfe->ldev)))
    {
        cdev_del(mstmfe->cdev);
        return err;
    }
    mstmfe->pdev = &pdev->dev;
    
    d->lock = &mstmfe->mutex;
    mutex_init(d->lock);
    d->i_ctx_count = 0;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    d->p_pmbase = (void*)res->start;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    d->p_iobase = (void*)res->start;

    printk("mmfe built at %s on %s\n",__TIME__,__DATE__);

    return err;

err_exit:
    if (mstmfe && mstmfe->dev_data)
    {
        kfree(mstmfe->dev_data);
    }
    if (mstmfe)
    {
        kfree(mstmfe);
    }
    if (mstmfe->cdev)
    {
        cdev_del(mstmfe->cdev);
    }
    mstmfe = NULL;

    return err;
}

static int
mstmfe_remove(
    struct platform_device* pdev)
{
    int devno = MKDEV(mstmfe_major,mstmfe_minor);

    kfree(mstmfe->dev_data);
    cdev_del(mstmfe->cdev);

    device_unregister(&mstmfe->ldev);

    kfree(mstmfe);
    mstmfe = NULL;

    unregister_chrdev_region(devno, 1);

    return 0;
}

static const struct of_device_id mmfe_of_match_tables[] = {
	{ .compatible = "mstar,mfe" },
	{},
};
static struct platform_driver mstmfe_pdrv = {
    .probe = mstmfe_probe,
    .remove = mstmfe_remove,
    .driver = {
        .name = "mmfe",
        .owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mmfe_of_match_tables),
    }
};

static int  __init mstmfe_init(void)
{
    int err = 0;

    if (0 > (err = class_register(&mstar_class)))
    {
        class_unregister(&mstar_class);
        return err;
    }

    return platform_driver_register(&mstmfe_pdrv);
}

static void __exit mstmfe_exit(void)
{
    platform_driver_unregister(&mstmfe_pdrv);

    class_unregister(&mstar_class);
}

module_init(mstmfe_init);
module_exit(mstmfe_exit);

