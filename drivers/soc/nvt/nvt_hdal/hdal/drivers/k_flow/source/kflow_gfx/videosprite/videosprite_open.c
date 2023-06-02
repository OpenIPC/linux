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
#include <linux/file.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#endif

#include "videosprite/videosprite_internal.h"
#include "videosprite/videosprite_open.h"
#include "videosprite/videosprite_ime.h"
#include "videosprite/videosprite_enc.h"
#include "videosprite/videosprite_vo.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__        videosprite_open
#define __DBGLVL__                NVT_DBG_WRN
#include "kwrap/debug.h"
unsigned int videosprite_open_debug_level = __DBGLVL__;
///////////////////////////////////////////////////////////////////////////////

#if defined(__LINUX)

void vds_memset(void *buf, unsigned char val, int len)
{
	memset(buf, val, len);
}

void vds_memcpy(void *buf, void *src, int len)
{
	memcpy(buf, src, len);
}

void* vds_alloc(int size)
{
	return kmalloc(size, GFP_ATOMIC);
}

void vds_free(void *buf)
{
	kfree(buf);
}

int vds_copy_from_user(void *dst, void *src, int len)
{
	return copy_from_user(dst, (void __user *)src, len);
}

int vds_copy_to_user(void *dst, void *src, int len)
{
	return copy_to_user((void __user *)dst, src, len);
}

int vds_seq_printf(void *m, const char *fmtstr, ...)
{
	int     len = 0;

	va_list marker;
	va_start(marker, fmtstr);
	seq_printf(m, fmtstr, marker);
	va_end(marker);
	
	return len;
}

int vds_snprintf(char *buf, int size, const char *fmtstr, ...)
{
	int     len = 0;

	va_list marker;
	va_start(marker, fmtstr);
	len = vsnprintf(buf, size, fmtstr, marker);
	va_end(marker);
	
	return len;
}

int vds_sprintf(char *buf, const char *fmtstr, ...)
{
	int     len = 0;

	va_list marker;
	va_start(marker, fmtstr);
	len = vsprintf(buf, fmtstr, marker);
	va_end(marker);
	
	return len;
}

int vds_sscanf(char *buf, const char *fmtstr, ...)
{
	int     len = 0;

	va_list marker;
	va_start(marker, fmtstr);
	len = vsscanf(buf, fmtstr, marker);
	va_end(marker);
	
	return len;
}

int vds_strcmp(char *s1, char *s2)
{
	return strcmp(s1, s2);
}

static int api_proc_show(struct seq_file *m, void *v) {
	return 0;
}

static int api_proc_open(struct inode *inode, struct  file *file) {

	if(vds_init()){
		DBG_ERR("vds_init() fail\n");
		return -1;
	}

	return single_open(file, api_proc_show, NULL);
}

static long api_proc_ioctl(struct file* f, unsigned int cmd, unsigned long arg)
{
	return nvt_vds_ioctl (-1, cmd, (void*)arg);
}

int api_release(struct inode *node, struct file *flip)
{
	vds_uninit();
	
	return 0;
}

static struct proc_dir_entry *api_proc_file_entry = NULL;
static const struct file_operations api_proc_file_fops = {
	.owner            = THIS_MODULE,
	.open             = api_proc_open,
	.unlocked_ioctl   = api_proc_ioctl,
	.release          = api_release,
};

static int info_show(struct seq_file *m, void *v) {

	vds_info_show(m, v);

	return 0;
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

static int cmd_show(struct seq_file *m, void *v) {

	vds_cmd_show(m, v);

	return 0;
}

static int cmd_open(struct inode *inode, struct  file *file) {
	return single_open(file, cmd_show, NULL);
}
static ssize_t cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	char cmd_line[128];

	if (copy_from_user(cmd_line, buf, size)) {
		return 0;
	}
	
	vds_cmd_write(cmd_line, size);
	
	return size;
}

static const struct file_operations cmd_fops = {
	.owner            = THIS_MODULE,
	.open             = cmd_open,
	.release          = single_release,
	.read             = seq_read,
	.write            = cmd_write,
	.llseek           = seq_lseek,
};

static int help_show(struct seq_file *m, void *v) {

	vds_seq_printf(m, "1. 'cat /proc/hdal/osg/info' will show all the osg configuration\n");
	vds_seq_printf(m, "2. 'echo xxx > /proc/hdal/comm/osg' can dynamically change osg configuration\n");
	vds_seq_printf(m, "---------------------------------------------------------------------\n");
	vds_seq_printf(m, "The currently support input command are stamp attribute, mask, mosaic.\n");
	vds_seq_printf(m, "Stamp buffer and image are not supported\n");
	vds_seq_printf(m, "---------------------------------------------------------------------\n");
	vds_seq_printf(m, "format of echo data to /proc/hdal/osg/cmd :\n");
	vds_seq_printf(m, "\tstamp : phase osg pid io start x y alpha cken ckval layer region\n");
	vds_seq_printf(m, "\t\tex: set the 5th stamp of output id 3 of videoenc to position[1024,512] and layer(1) region(8)\n");
	vds_seq_printf(m, "\t\t\techo videoenc stamp 5 3 1 1024 512 255 0 0 1 8\n");
	vds_seq_printf(m, "\tmask : phase osg pid io start x y w h solid thick color alpha\n");
	vds_seq_printf(m, "\t\tex: set the 5th green mask of device id 3 of videoout to position[1024,512] and size 256x128\n");
	vds_seq_printf(m, "\t\t\techo videoout mask 5 3 1 1024 512 256 128 1 0 0x0FF00 255\n");
	vds_seq_printf(m, "\tmosaic : phase osg pid io start x y w h mosaic_blk_w mosaic_blk_h\n");
	vds_seq_printf(m, "\t\tex: set the 5th mosaic of device id 3 of videoprocess to position[1024,512] and size 256x128\n");
	vds_seq_printf(m, "\t\t\techo videoprocess mosaic 5 3 1 1024 512 256 128 32 32\n");

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

static int save_show(struct seq_file *m, void *v) {

	vds_seq_printf(m, "echo phase pid vid director to save stamps\n");

	return 0;
}

static int save_open(struct inode *inode, struct  file *file) {
	return single_open(file, save_show, NULL);
}

static ssize_t save_write(struct file *file, const char __user *data, size_t size, loff_t *off)
{
	int len = size, phase = -1, pid = -1, vid = -1;
	char cmd_line[256], directory[128];

	if (len == 0 || len > 256) {
		DBG_ERR("Command length(%d) is 0 or > 256!\n", len);
		return size;
	}

	if (copy_from_user(cmd_line, data, len)) {
		return size;
	}

	cmd_line[len - 1] = '\0';
	sscanf(cmd_line, "%d %d %d %s ", &phase, &pid, &vid, directory);
	directory[sizeof(directory) - 1] = '\0';
	if(phase == -1 || pid == -1 || vid == -1){
		DBG_ERR("invalid phase(%d) or pid(%d) or vid(%d)\n", phase, pid, vid);
		return size;
	}
	if(strlen(directory) == 0){
		DBG_ERR("directory is not specified\n");
		return size;
	}
	
	vds_save_write(phase, pid, vid, directory);

	return size;
}

int vds_save_image(char *filename, void *p_addr, int size)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	unsigned long long offset = 0;
			
	old_fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
	set_fs(old_fs);
	if (IS_ERR(filp)) {
		DBG_ERR("fail to open %s\n", filename);
		return -1;
	}

	old_fs = get_fs();
	set_fs(get_ds());
	offset = 0;

	kernel_write(filp, p_addr, size, &offset);

	set_fs(old_fs);
	vfs_fsync(filp, 0);
	filp_close(filp, NULL);

	return 0;
}

static const struct file_operations save_fops = {
	.owner            = THIS_MODULE,
	.open             = save_open,
	.release          = single_release,
	.read             = seq_read,
	.write            = save_write,
	.llseek           = seq_lseek,
};

static int latency_show(struct seq_file *m, void *v)
{
	vds_latency_show(m, v, 0);
	return 0;
}

static int latency_open(struct inode *inode, struct  file *file) {
	return single_open(file, latency_show, NULL);
}

static ssize_t latency_write(struct file *file, const char __user *data, size_t size, loff_t *off)
{
	vds_latency_show(NULL, NULL, 1);
	return size;
}

static const struct file_operations latency_fops = {
	.owner            = THIS_MODULE,
	.open             = latency_open,
	.release          = single_release,
	.read             = seq_read,
	.write            = latency_write,
	.llseek           = seq_lseek,
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_vds_probe(struct platform_device *pdev);
static int nvt_vds_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_vds_resume(struct platform_device *pdev);
static int nvt_vds_remove(struct platform_device *pdev);
int __init nvt_vds_module_init(void);
void __exit nvt_vds_module_exit(void);

//=============================================================================
// function define
//=============================================================================

static int nvt_vds_probe(struct platform_device *pdev)
{
	return 0;
}

static int nvt_vds_remove(struct platform_device *pdev)
{
	return 0;
}

static int nvt_vds_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int nvt_vds_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver nvt_vds_driver = {
	.driver = {
		.name   = "nvt_videosprite",
		.owner  = THIS_MODULE,
	},
	.probe      = nvt_vds_probe,
	.remove     = nvt_vds_remove,
	.suspend    = nvt_vds_suspend,
	.resume     = nvt_vds_resume
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

int __init nvt_vds_module_init(void)
{
	int                   ret;
	struct proc_dir_entry *proc_file_entry = NULL;
	struct proc_dir_entry *proc_parent     = NULL;

	if(nvt_vds_init()){
		DBG_ERR("nvt_vds_init() fail\n");
		return -ENOMEM;
	}

	ret = platform_driver_register(&nvt_vds_driver);

	api_proc_file_entry = proc_create("nvt_videosprite", 0, NULL, &api_proc_file_fops);
	if(api_proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/nvt_videosprite\r\n");
		return -ENOMEM;
	}

	proc_parent = proc_mkdir("hdal/osg",NULL);
	if(!proc_parent){
		DBG_ERR("fail to create /proc/osg\r\n");
		return -ENOMEM;
	}

	proc_file_entry = proc_create("info", 0, proc_parent, &info_fops);
	if(proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/hdal/osg/info\r\n");
		return -ENOMEM;
	}

	proc_file_entry = proc_create("cmd", 0, proc_parent, &cmd_fops);
	if(proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/hdal/osg/cmd\r\n");
		return -ENOMEM;
	}

	proc_file_entry = proc_create("help", 0, proc_parent, &help_fops);
	if(proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/hdal/osg/help\r\n");
		return -ENOMEM;
	}

	proc_file_entry = proc_create("save", 0, proc_parent, &save_fops);
	if(proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/hdal/osg/save\r\n");
		return -ENOMEM;
	}

	proc_file_entry = proc_create("latency", 0, proc_parent, &latency_fops);
	if(proc_file_entry == NULL){
		DBG_ERR("fail to create /proc/hdal/osg/latency\r\n");
		return -ENOMEM;
	}

	return 0;
}

void __exit nvt_vds_module_exit(void)
{	
	nvt_vds_exit();

	if(api_proc_file_entry){
		api_proc_file_entry = NULL;
		remove_proc_entry("hdal/osg/latency", NULL);
		remove_proc_entry("hdal/osg/cmd", NULL);
		remove_proc_entry("hdal/osg/info", NULL);
		remove_proc_entry("hdal/osg/help", NULL);
		remove_proc_entry("hdal/osg/save", NULL);
		remove_proc_entry("hdal/osg", NULL);
	}
	
	platform_driver_unregister(&nvt_vds_driver);
}

EXPORT_SYMBOL(vds_memset);
EXPORT_SYMBOL(vds_lock_context);
EXPORT_SYMBOL(vds_unlock_context);
EXPORT_SYMBOL(vds_render_ime_context);
EXPORT_SYMBOL(vds_render_enc_ext);
EXPORT_SYMBOL(vds_render_vo);
EXPORT_SYMBOL(vds_render_coe_grh);
EXPORT_SYMBOL(vds_get_ime_palette);
EXPORT_SYMBOL(vds_get_coe_palette);
EXPORT_SYMBOL(vds_get_vo_palette);

EXPORT_SYMBOL(vds_max_coe_stamp);
EXPORT_SYMBOL(vds_max_ime_stamp);
EXPORT_SYMBOL(vds_max_ime_mask);

EXPORT_SYMBOL(vds_save_latency);

module_init(nvt_vds_module_init);
module_exit(nvt_vds_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("videosprite driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.01.000");
#endif