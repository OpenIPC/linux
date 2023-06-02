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
#endif

#include "kdrv_gfx2d/kdrv_ise.h"
#include "kdrv_gfx2d/kdrv_ise_lmt.h"
#include "kdrv_gfx2d/kdrv_grph.h"
#include "kdrv_gfx2d/kdrv_grph_lmt.h"
#include "gximage/gximage.h"
#include "gximage/gximage_internal.h"
#include "kdrv_gfx2d/kdrv_ise.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__        gximage_open
#define __DBGLVL__                NVT_DBG_WRN
#include "kwrap/debug.h"
unsigned int gximage_open_debug_level = __DBGLVL__;
///////////////////////////////////////////////////////////////////////////////

int gximg_check_ise_limit(KDRV_ISE_MODE *ise_op);
int gximg_check_grph_limit(GRPH_CMD cmd, UINT32 bits, GRPH_IMG *image, int check_wh);

#if defined(__LINUX)
extern ER gximg_scale_config_mode(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, KDRV_ISE_MODE *mode, int *count);

void gximg_memset(void *buf, unsigned char val, int len)
{
	memset(buf, val, len);
}

void gximg_memcpy(void *buf, void *src, int len)
{
	memcpy(buf, src, len);
}

EXPORT_SYMBOL(gximg_init_buf);
EXPORT_SYMBOL(gximg_init_buf_h_align);
EXPORT_SYMBOL(gximg_init_buf_ex);
EXPORT_SYMBOL(gximg_calc_require_size);
EXPORT_SYMBOL(gximg_calc_require_size_h_align);
EXPORT_SYMBOL(gximg_fill_data);
EXPORT_SYMBOL(gximg_fill_data_no_flush);
EXPORT_SYMBOL(gximg_fill_data_ex);
EXPORT_SYMBOL(gximg_copy_data);
EXPORT_SYMBOL(gximg_copy_data_no_flush);
EXPORT_SYMBOL(gximg_copy_data_ex);
EXPORT_SYMBOL(gximg_copy_color_key_data);
EXPORT_SYMBOL(gximg_copy_blend_data);
EXPORT_SYMBOL(gximg_copy_blend_data_no_flush);
EXPORT_SYMBOL(gximg_copy_blend_data_ex);
EXPORT_SYMBOL(gximg_copy_blend_data_ex_no_flush);
EXPORT_SYMBOL(gximg_set_parm);
EXPORT_SYMBOL(gximg_get_parm);
EXPORT_SYMBOL(gximg_scale_data);
EXPORT_SYMBOL(gximg_scale_data_no_flush);
EXPORT_SYMBOL(gximg_scale_data_ex);
EXPORT_SYMBOL(gximg_rotate_data_ex);
EXPORT_SYMBOL(gximg_rotate_data);
EXPORT_SYMBOL(gximg_rotate_data_no_flush);
EXPORT_SYMBOL(gximg_rotate_paste_data);
EXPORT_SYMBOL(gximg_rotate_paste_data_no_flush);
EXPORT_SYMBOL(gximg_argb_to_yuv_blend);
EXPORT_SYMBOL(gximg_argb_to_yuv_blend_no_flush);
EXPORT_SYMBOL(gximg_rgb_to_yuv_color_key);
EXPORT_SYMBOL(gximg_color_transform);
EXPORT_SYMBOL(gximg_quad_cover);
EXPORT_SYMBOL(gximg_quad_cover_no_flush);
EXPORT_SYMBOL(gximg_quad_cover_with_flush);
EXPORT_SYMBOL(gximg_scale_config_mode);
EXPORT_SYMBOL(gximg_raw_graphic);
EXPORT_SYMBOL(gximg_check_ise_limit);
EXPORT_SYMBOL(gximg_check_grph_limit);

//=============================================================================
// function declaration
//=============================================================================
static int nvt_gximage_open(struct inode *inode, struct file *file);
static int nvt_gximage_release(struct inode *inode, struct file *file);
static int nvt_gximage_probe(struct platform_device *pdev);
static int nvt_gximage_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_gximage_resume(struct platform_device *pdev);
static int nvt_gximage_remove(struct platform_device *pdev);
int __init nvt_gximage_module_init(void);
void __exit nvt_gximage_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_gximage_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int nvt_gximage_release(struct inode *inode, struct file *file)
{
	return 0;
}

struct file_operations nvt_gximage_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_gximage_open,
	.release = nvt_gximage_release,
	.llseek  = no_llseek,
};

static int nvt_gximage_probe(struct platform_device *pdev)
{
	nvt_dbg(IND, "%s\n", pdev->name);

	return 0;
}

static int nvt_gximage_remove(struct platform_device *pdev)
{
	nvt_dbg(IND, "\n");

	return 0;
}

static int nvt_gximage_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}


static int nvt_gximage_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver nvt_gximage_driver = {
	.driver = {
			.name   = "nvt_gximage",
			.owner = THIS_MODULE,
		      },
	.probe      = nvt_gximage_probe,
	.remove     = nvt_gximage_remove,
	.suspend = nvt_gximage_suspend,
	.resume = nvt_gximage_resume
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

int __init nvt_gximage_module_init(void)
{
	int ret;
	
	if(gximage_init()){
		printk(KERN_ERR "gximage_init() fail\n");
		return -1;
	}

	ret = platform_driver_register(&nvt_gximage_driver);

	return 0;
}

void __exit nvt_gximage_module_exit(void)
{
	nvt_dbg(WRN, "\n");
	
	gximage_exit();

	platform_driver_unregister(&nvt_gximage_driver);
}

module_init(nvt_gximage_module_init);
module_exit(nvt_gximage_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("GxImage driver");
MODULE_LICENSE("GPL");
#endif
