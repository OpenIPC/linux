#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/virtio.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
static int sysmap_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sysmap_close(struct inode *inode, struct file *file)
{
	return 0;
}

static int sys_devmap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int retval;

	if (filp->f_flags & O_SYNC) {
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	} else {
#ifdef	CONFIG_64BIT
		vma->vm_page_prot =
			__pgprot(pgprot_val(vma->vm_page_prot) | PTE_WRITE |
				PTE_DIRTY);
#else
		vma->vm_page_prot = __pgprot(pgprot_val(vma->vm_page_prot) | L_PTE_PRESENT
			| L_PTE_YOUNG | L_PTE_DIRTY | L_PTE_MT_DEV_CACHED);
#endif
	}
	retval = remap_pfn_range(vma,
				 vma->vm_start,
				 vma->vm_pgoff,
				 vma->vm_end - vma->vm_start,
				 vma->vm_page_prot);
	if (retval) {
		return -EAGAIN;
	}
	return 0;
}

static struct file_operations sys_devmap_fops = {
	.owner = THIS_MODULE,
	.open = sysmap_open,
	.release = sysmap_close,
	.mmap = sys_devmap_mmap,
};

static struct miscdevice ax_sysmap_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ax_sysmap",
	.fops = &sys_devmap_fops
};

static int register_sysmap_dev(void)
{
	misc_register(&ax_sysmap_miscdev);
	return 0;
}

static int unregister_sysmap_dev(void)
{
	misc_deregister(&ax_sysmap_miscdev);
	return 0;
}

static int ax_sysmap_probe(struct platform_device *pdev)
{
	register_sysmap_dev();
	return 0;
}

static int ax_sysmap_remove(struct platform_device *pdev)
{
	unregister_sysmap_dev();
	return 0;
}

static const struct of_device_id ax_sysmap_of_match[] = {
	{.compatible = "axera,ax_sysmap"},
	{}
};

static struct platform_driver ax_sysmap_driver = {
	.probe = ax_sysmap_probe,
	.remove = ax_sysmap_remove,
	.driver = {
		   .name = KBUILD_MODNAME,
		   .of_match_table = ax_sysmap_of_match,
		   },
};

static int __init ax_sysmap_init(void)
{
	platform_driver_register(&ax_sysmap_driver);
	return 0;
}

static void ax_sysmap_exit(void)
{
	platform_driver_unregister(&ax_sysmap_driver);
}

late_initcall(ax_sysmap_init);
module_exit(ax_sysmap_exit);
MODULE_DESCRIPTION("axera ax_sysmap driver");
MODULE_AUTHOR("Axera Inc.");
MODULE_LICENSE("GPL v2");
