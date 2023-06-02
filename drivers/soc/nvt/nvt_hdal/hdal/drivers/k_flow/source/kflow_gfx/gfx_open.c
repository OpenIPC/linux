#if defined(__LINUX)
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#endif

#include <gximage/gfx_internal.h>
#include <gximage/gfx_open.h>

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          gfx_open
#define __DBGLVL__          NVT_DBG_WRN
#include "kwrap/debug.h"
unsigned int gfx_open_debug_level = __DBGLVL__;
///////////////////////////////////////////////////////////////////////////////

#if defined(__LINUX)

void gfx_memset(void *buf, unsigned char val, int len)
{
	memset(buf, val, len);
}

void gfx_memcpy(void *buf, void *src, int len)
{
	memcpy(buf, src, len);
}

void* gfx_alloc(int size)
{
	return kmalloc(size, GFP_ATOMIC);
}

void gfx_free(void *buf)
{
	kfree(buf);
}

int gfx_copy_from_user(void *dst, void *src, int len)
{
	return copy_from_user(dst, (void __user *)src, len);
}

int gfx_copy_to_user(void *dst, void *src, int len)
{
	return copy_to_user((void __user *)dst, src, len);
}

int gfx_seq_printf(void *m, const char *fmtstr, ...)
{
	int     len = 0;

	va_list marker;
	va_start(marker, fmtstr);
	seq_printf(m, fmtstr, marker);
	va_end(marker);
	
	return len;
}

int gfx_snprintf(char *buf, int size, const char *fmtstr, ...)
{
	int     len = 0;

	va_list marker;
	va_start(marker, fmtstr);
	len = vsnprintf(buf, size, fmtstr, marker);
	va_end(marker);
	
	return len;
}

static int proc_show(struct seq_file *m, void *v) {
	seq_printf(m, "nothing!\n");
	return 0;
}

static int proc_open(struct inode *inode, struct  file *file) {
	return single_open(file, proc_show, NULL);
}

static long proc_ioctl(struct file* f, unsigned int cmd, unsigned long arg)
{
	return nvt_gfx_ioctl(-1, cmd, (void*)arg);
}

static struct proc_dir_entry *proc_file_entry = NULL;
static const struct file_operations proc_file_fops = {
	.owner            = THIS_MODULE,
	.open             = proc_open,
	.unlocked_ioctl   = proc_ioctl,
	.release          = single_release,
};

static int info_show(struct seq_file *m, void *v) {

	return gfx_info_show(m, v);
}

static int info_open(struct inode *inode, struct  file *file) {
	return single_open(file, info_show, NULL);
}

static const struct file_operations info_fops = {
	.owner            = THIS_MODULE,
	.open             = info_open,
	.release          = single_release,
	.read             = seq_read,
	.llseek           = seq_lseek,
};

static int help_show(struct seq_file *m, void *v) {

	gfx_cmd_showhelp(m, v);

	return 0;
}

static int help_open(struct inode *inode, struct  file *file) {
	return single_open(file, help_show, NULL);
}

static const struct file_operations help_fops = {
	.owner            = THIS_MODULE,
	.open             = help_open,
	.release          = single_release,
	.read             = seq_read,
	.llseek           = seq_lseek,
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_gfx_probe(struct platform_device *pdev);
static int nvt_gfx_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_gfx_resume(struct platform_device *pdev);
static int nvt_gfx_remove(struct platform_device *pdev);
int __init nvt_gfx_module_init(void);
void __exit nvt_gfx_module_exit(void);

//=============================================================================
// function define
//=============================================================================

static int nvt_gfx_probe(struct platform_device *pdev)
{
	return 0;
}

static int nvt_gfx_remove(struct platform_device *pdev)
{
	return 0;
}

static int nvt_gfx_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int nvt_gfx_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver nvt_gfx_driver = {
	.driver = {
		.name   = "nvt_gfx",
		.owner  = THIS_MODULE,
	},
	.probe      = nvt_gfx_probe,
	.remove     = nvt_gfx_remove,
	.suspend    = nvt_gfx_suspend,
	.resume     = nvt_gfx_resume
};

#if defined(_GROUP_KO_)
#undef __init
#undef __exit
#undef module_init
#undef module_exit
#define __init
#define __exit
#define module_init(x)
#define module_exit(x)
#endif

int __init nvt_gfx_module_init(void)
{
	int                      ret;
	struct proc_dir_entry    *proc_file_entry = NULL;
	struct proc_dir_entry    *proc_parent     = NULL;

	if(nvt_gfx_init()){
		DBG_ERR("nvt_gfx_init() fail\n");
		return -1;
	}

	ret = platform_driver_register(&nvt_gfx_driver);

	proc_file_entry = proc_create("nvt_gfx", 0, NULL, &proc_file_fops);
	if(proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/nvt_gfx\n");
		return -ENOMEM;
	}

	proc_parent = proc_mkdir("hdal/gfx",NULL);
	if(!proc_parent){
		DBG_ERR("fail to create /proc/gfx\r\n");
		return -ENOMEM;
	}

	proc_file_entry = proc_create("info", 0, proc_parent, &info_fops);
	if(proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/hdal/info\r\n");
		return -ENOMEM;
	}

	proc_file_entry = proc_create("help", 0, proc_parent, &help_fops);
	if(proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/hdal/help\r\n");
		return -ENOMEM;
	}

	return 0;
}

void __exit nvt_gfx_module_exit(void)
{
	nvt_gfx_exit();

	if(proc_file_entry){
		proc_file_entry = NULL;
		remove_proc_entry("nvt_gfx", NULL);
		remove_proc_entry("hdal/gfx/info", NULL);
		remove_proc_entry("hdal/gfx/help", NULL);
		remove_proc_entry("hdal/gfx", NULL);
	}

	platform_driver_unregister(&nvt_gfx_driver);
}

module_init(nvt_gfx_module_init);
module_exit(nvt_gfx_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("gfx driver");
MODULE_LICENSE("GPL");
#endif
