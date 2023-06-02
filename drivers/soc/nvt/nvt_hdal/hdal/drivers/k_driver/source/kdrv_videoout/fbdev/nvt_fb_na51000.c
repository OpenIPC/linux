/*
 *	Copyright (c) 2018 Novatek
 *
 * Novatek Framebuffer Driver
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * Driver based on skeletonfb.c, sa1100fb.c and others.
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <asm/types.h>
#include "nvt_dbg.h"
#include "nvt_fb_na51000.h"
#include "dispcomn.h"
#include "display.h"
#include "kdrv_videoout/nvt_fb.h"
#include "kwrap/dev.h"

#define MODULE_NAME "fb_misc"
#define MODULE_MINOR_COUNT 1

#define IDE_MAX_LAYER 4
//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================

unsigned int nvt_fb_debug_level = (/*NVT_FB_DBG_IND | */ NVT_FB_DBG_WRN | NVT_FB_DBG_ERR);
module_param_named(nvt_fb_debug_level, nvt_fb_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(nvt_fb_debug_level, "NVT FB debug level");

//=============================================================================
// Global variable
//=============================================================================
static struct platform_device *nvt_fb_device;
struct nvt_fb_info *nvt_fb;
struct semaphore nvt_sem;
unsigned char init = 0;

unsigned char layer_list[IDE_MAX_LAYER] = {IDE_DISPLAYER_VDO1, IDE_DISPLAYER_VDO2, IDE_DISPLAYER_OSD1, IDE_DISPLAYER_OSD2};
unsigned char fb_name[NVT_TOTAL_FB][MAX_FB_NAME] = {{"IDE1_V1"},
													{"IDE1_V2"},
													{"IDE1_O1"},
													{"IDE1_O2"},
													{"IDE2_V1"},
													{"IDE2_V2"},
													{"IDE2_O1"},
													{"IDE2_O2"} };

static int nvt_init_fb_drv(void);
int nvt_fb_miscdev_init(void);
int nvt_fb_miscdev_exit(void);
int nvt_fb_init_ide_layer(struct nvt_fb_layer_para *layer_para);

#if NVT_FB_CREATE_BUFFER_BY_FB /*frame buffer is allocated from HDAL, so we don't need to allocate it in fb driver*/
int nvt_fb_map_video_memory(struct fb_info *info, unsigned int fb_if)
{
	struct nvt_fb_info *fbinfo = ((struct nvt_fb_par *)(info->par))->info;
	dma_addr_t map_dma = 0;
	unsigned map_size;


	if (NVT_FB_IS_OSD(fb_if)) {
		map_size = PAGE_ALIGN(info->fix.smem_len * NVT_OSD_PAN_DISPLAY_NUM);
	} else if (NVT_FB_IS_VIDEO(fb_if)) {
		map_size = PAGE_ALIGN(info->fix.smem_len * NVT_VD_PAN_DISPLAY_NUM);
	} else {
		return -EINVAL;
	}

	dma_set_coherent_mask(nvt_fb->dev, DMA_BIT_MASK(32));

	nvt_dbg(IND, "map_video_memory(fbi=%p) map_size:0x%x\n", fbinfo, map_size);
	info->screen_base = dma_alloc_writecombine(fbinfo->dev, map_size,
		&map_dma, GFP_KERNEL);

	if (info->screen_base == NULL) {
		DBG_ERR("Out of memory\n");
		return -ENOMEM;
	}

	if (info->screen_base) {
		/* prevent initial garbage on screen */
		DBG_IND("map_video_memory: clear %p:%08x\n",
			info->screen_base, map_size);
		memset(info->screen_base, 0x00, map_size);

		info->fix.smem_start = map_dma;

		DBG_ERR("map_video_memory: phy=%08lx vir=%p size=%08x\n",
			info->fix.smem_start, info->screen_base, map_size);
	}

	return info->screen_base ? 0 : -ENOMEM;
}

void nvt_fb_unmap_video_memory(struct fb_info *info)
{
	struct nvt_fb_info *fbinfo = ((struct nvt_fb_par *)(info->par))->info;

	dma_free_writecombine(fbinfo->dev, PAGE_ALIGN(info->fix.smem_len),
	info->screen_base, info->fix.smem_start);
}

#endif


int nvt_fb_alloc(struct nvt_fb_info *pnvt_fb_info, unsigned int ide_id, unsigned int layer_id)
{
	struct fb_info *fbinfo;
	struct nvt_fb_par *par;

	fbinfo = framebuffer_alloc(sizeof(struct nvt_fb_par), pnvt_fb_info->dev);

	if (!fbinfo) {
		/* goto error path */
		DBG_ERR("fb allocate failed\n");
		return -ENOMEM;
	}

	par = fbinfo->par;
	par->info = pnvt_fb_info;

	par->fb_id = NVT_FB_MAKE_FB_ID(ide_id, layer_id);
	pnvt_fb_info->fb[par->fb_id] = fbinfo;
	par->flags = FBINFO_DEFAULT;

	return 0;
}

int nvt_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct nvt_fb_par *fb_par = (struct nvt_fb_par *) info->par;
	unsigned int addr_phy;
	unsigned int total_bit;
	unsigned int total_byte;
	int ret = 0;
	int layer_id;
	DISP_ID ide_id;

	nvt_dbg(IND, "off_x:%d off_y:%d vir_x:%d vir_y:%d res_x:%d res_y:%d bit_per_pixel:%d\n", var->xoffset, var->yoffset, var->xres_virtual, var->yres_virtual, var->xres, var->yres, var->bits_per_pixel);

	ide_id = (DISP_ID) NVT_FB_GET_IDE_ID(fb_par->fb_id);
	layer_id = NVT_FB_GET_LAYER_ID(fb_par->fb_id);

	total_bit = var->xoffset * var->bits_per_pixel + var->yoffset * var->xres_virtual * var->bits_per_pixel;
	total_byte = (total_bit + 7) / 8;
	nvt_dbg(IND, "Total bit:%d byte:%d\n", total_bit, total_byte);

	if (((var->yoffset + var->yres) > var->yres_virtual) || (((var->yoffset + var->yres) == (var->yres_virtual)) && \
		((var->xoffset + var->xres) > var->xres_virtual))) {
		nvt_dbg(ERR, "Over buffer size:off_x:%d off_y:%d vir_x:%d vir_y:%d res_x:%d res_y:%d\n", var->xoffset, var->yoffset, var->xres_virtual, var->yres_virtual, var->xres, var->yres);
		return -EINVAL;
	}

	addr_phy = info->fix.smem_start + total_byte;

	nvt_dbg(IND, "Pan_disp fb_if:0x%x fix_mem:0x%lx total:0x%x addr_phy:0x%x\n", ide_id, info->fix.smem_start, total_byte, addr_phy);

	if (NVT_FB_IS_OSD(layer_id)) {
		ret = display_obj_set_osd_addr(ide_id, addr_phy, 1);
	} else if (NVT_FB_IS_VIDEO(layer_id)) {
		/* ret = display_obj_set_osd_addr(ide_id, addr_phy, 1); */
 		nvt_dbg(ERR, "Unsupported operation:0x%x\n", layer_id);
	} else {
		nvt_dbg(ERR, "unknown id found:0x%x/n", layer_id);
	}

	nvt_dbg(IND, "-\n");
	return ret;
}

int nvt_fb_free(struct fb_info *info)
{
#if NVT_FB_CREATE_BUFFER_BY_FB
	nvt_fb_unmap_video_memory(info);
#endif

	unregister_framebuffer(info);
	framebuffer_release(info);
	return 0;
}

/*
 *  Initialization
 */
static int nvt_fb_probe(struct platform_device *pdev)
{
#ifdef FB_FORCE_INIT
	struct nvt_fb_layer_para fb_para;
#endif
	int ret = 0;
	int fb_index;

    /*
     * Dynamically allocate info and par
     */
	nvt_dbg(IND, "+\n");
	nvt_fb = kzalloc(sizeof(struct nvt_fb_info), GFP_KERNEL);
	if (!nvt_fb) {
		nvt_dbg(ERR, "allocate failed\n");
		ret = -ENOMEM;
		goto ERR_OUT;
	}

	for (fb_index = 0 ; fb_index < NVT_TOTAL_FB ; fb_index++) {
		nvt_fb->fb[fb_index] = NULL;
		nvt_fb->free_ioremap[fb_index] = 0;
	}

	nvt_fb->dev = &pdev->dev;

	platform_set_drvdata(pdev, nvt_fb);

#ifdef FB_FORCE_INIT
	fb_para.layer_id = NVT_LAYER_OSD_1;
	fb_para.ide_id = NVT_IDE1;
	ret = nvt_fb_init_ide_layer(&fb_para);
#endif

	return ret;

ERR_OUT:
	return ret;
}

int nvt_fb_init_ide_layer(struct nvt_fb_layer_para *layer_para)
{
	struct fb_info *fbinfo = NULL;
	int fb_if;
	int ret = 0;
#if !(NVT_FB_CREATE_BUFFER_BY_FB)
	int pfn;
#endif

	nvt_dbg(IND, "+\n");
	if (nvt_fb == NULL) {
		nvt_dbg(ERR, "nvt_fb_info is invalid:0x%x\n", (unsigned int)nvt_fb);
		return -EINVAL;
	}

	if ((layer_para->ide_id > NVT_MAX_IDE) || (layer_para->layer_id >= IDE_MAX_LAYER)) {
		nvt_dbg(ERR, "Invalid IDE id:%d or layer id%d\n", layer_para->ide_id, layer_para->layer_id);
		return -EINVAL;
	}

#if NVT_FB_GET_BUFFER_FROM_HDAL
	if ((layer_para->fb_pa_addr == 0) && (layer_para->fb_va_addr == 0)) {
		nvt_dbg(ERR, "NULL buffer pointer found pa:0x%x va:0x%x\n", layer_para->fb_pa_addr, layer_para->fb_va_addr);
		return -EINVAL;
	}

	if (layer_para->buffer_len == 0) {
		nvt_dbg(ERR, "NULL buffer lenght:%d\n", layer_para->buffer_len);
		return -EINVAL;
	}
#endif

	if (display_obj_get_layer_support(layer_para->ide_id, layer_list[layer_para->layer_id]) == -1) {
		nvt_dbg(ERR, "unspported fb layer:%d of ide%d\n", layer_para->layer_id, layer_para->ide_id);
		return -EINVAL;
	}

	fb_if = NVT_FB_MAKE_FB_ID(layer_para->ide_id, layer_para->layer_id);
	if (nvt_fb->fb[fb_if] != NULL) {
		nvt_dbg(ERR, "fb(ide:%d layer:%d) already create\n", layer_para->ide_id, layer_para->layer_id);
		return -EALREADY;
	}

#if 0
	if (layer_para->fb_va_addr == 0) {
		DBG_ERR("Invalid vir addr:0x%x\n", layer_para->fb_va_addr);
		return -EINVAL;
	}
#endif
	if ((layer_para->fb_pa_addr == 0) || (layer_para->fb_pa_addr == 0xFFFFFFFF)) {
		nvt_dbg(ERR, "Invalid phy addr:0x%x\n", layer_para->fb_pa_addr);
		return -EINVAL;
	}

	ret = nvt_fb_alloc(nvt_fb, layer_para->ide_id, layer_para->layer_id);
	if (ret != 0) {
		goto ERR_ALLOC;
	}

	nvt_fb->free_ioremap[fb_if] = 0;
	fbinfo = nvt_fb->fb[fb_if];

#if NVT_FB_CREATE_BUFFER_BY_FB
	/* Initialize video memory */
	/*frame buffer is allocated from HDAL, so we don't need to allocate it in fb driver*/
    /* Just get proper buffer length */
	nvt_dbg(IND, "IND, Buffer created by FB\n");

	if (NVT_FB_IS_OSD(layer_para->layer_id)) {
		if (display_obj_get_osd_buf_addr(layer_para->ide_id, &fbinfo->fix.smem_start,
									&fbinfo->screen_base, &fbinfo->fix.smem_len)) {
			return -ENOMEM;
		}
	} else if (NVT_FB_IS_VIDEO(layer_para->layer_id)) {
		if (display_obj_get_video1_buf_addr(layer_para->ide_id, &fbinfo->fix.smem_start,
									&fbinfo->screen_base, &fbinfo->fix.smem_len)) {
			return -ENOMEM;
		}
	} else {
		nvt_dbg(ERR, "Unknow layid:0x%x\n", layer_para->ide_id);
		return -ENOMEM;
	}

	ret = nvt_fb_map_video_memory(fbinfo, fb_if);
	if (ret) {
		DBG_ERR("Failed to allocate video buffer: %d\n", ret);
		ret = -ENOMEM;
		return ret;
	}

#elif NVT_FB_GET_BUFFER_FROM_IDE
	/* Initialize video memory */
	/*frame buffer is allocated from HDAL, so we don't need to allocate it in fb driver*/
    /* Just get proper buffer length */
	nvt_dbg(IND, "Buffer obtain from IDE\n");
	if (NVT_FB_IS_OSD(layer_para->layer_id)) {
		if (display_obj_get_osd_buf_addr(layer_para->ide_id, &fbinfo->fix.smem_start,
										&fbinfo->screen_base, &fbinfo->fix.smem_len)) {
			nvt_dbg(ERR, "Get osd buff err:%d\n", layer_para->ide_id);
			return -ENOMEM;
		}
	} else if (NVT_FB_IS_VIDEO(layer_para->layer_id)) {
		if (display_obj_get_video1_buf_addr(layer_para->ide_id, &fbinfo->fix.smem_start,
										&fbinfo->screen_base, &fbinfo->fix.smem_len)) {
			nvt_dbg(ERR, "Get video buff err:%d\n", layer_para->ide_id);
			return -ENOMEM;
		}
	} else {
		nvt_dbg(ERR, "Unknow layid:0x%x\n", layer_para->ide_id);
		return -ENOMEM;
	}

	if (fbinfo->fix.smem_start == 0) {
		DBG_ERR("NULL buffer pointer found pa:0x%x\n", (unsigned int)fbinfo->fix.smem_start);
		return -EINVAL;
	}

	if (fbinfo->fix.smem_len == 0) {
		DBG_ERR("NULL buffer lenght:%d\n", fbinfo->fix.smem_len);
		return -EINVAL;
	}

	pfn = (fbinfo->fix.smem_start >> PAGE_SHIFT);
	if (pfn_valid(pfn)) {
		fbinfo->screen_base = __va(fbinfo->fix.smem_start);
	} else {
		fbinfo->screen_base = ioremap(fbinfo->fix.smem_start, fbinfo->fix.smem_len);
		nvt_fb->free_ioremap[fb_if] = 1;
	}

#elif NVT_FB_GET_BUFFER_FROM_HDAL
	nvt_dbg(IND, "Buffer obtain from HDAL\n");
	if (layer_para->fb_va_addr == 0) {
		pfn = (layer_para->fb_pa_addr >> PAGE_SHIFT);
		if (pfn_valid(pfn)) {
			fbinfo->screen_base = __va(layer_para->fb_pa_addr);
		} else {
			fbinfo->screen_base = ioremap(layer_para->fb_pa_addr, layer_para->buffer_len);
			nvt_fb->free_ioremap[fb_if] = 1;
		}
	} else {
		fbinfo->screen_base = (char __iomem *)layer_para->fb_va_addr;
	}

	fbinfo->fix.smem_start = layer_para->fb_pa_addr;;
	fbinfo->fix.smem_len = layer_para->buffer_len;
#else

	"Please select a way to get buffer address"

#endif

	if (NVT_FB_IS_OSD(layer_para->layer_id)) {
		ret = nvt_fb_osd_probe(fbinfo, layer_para->ide_id, layer_para->layer_id);
	} else if (NVT_FB_IS_VIDEO(layer_para->layer_id)) {
		ret = nvt_fb_video_probe(fbinfo, layer_para->ide_id, layer_para->layer_id);
	} else {
		nvt_dbg(ERR, "Unknow fb layer%d\n", layer_para->layer_id);
	}

	if (ret) {
		goto ERR_REGISER_FB;
	}

	memcpy(&fbinfo->fix.id[0], &fb_name[fb_if][0], MAX_FB_NAME);

	DBG_IND("fb id:%d fbinfo:0x%x\n", fb_if, (unsigned int)fbinfo);

	DBG_IND("pa:0x%x va:0x%x len:0x%x\n", (unsigned int)fbinfo->fix.smem_start, (unsigned int)fbinfo->screen_base, (unsigned int)fbinfo->fix.smem_len);

	memset(fbinfo->screen_base, 0x00, fbinfo->fix.smem_len);

	ret = register_framebuffer(fbinfo);
	if (ret) {
		DBG_ERR("Register FB failed\n");
		framebuffer_release(fbinfo);
		goto ERR_REGISER_FB;
	}

	nvt_dbg(IND, "%s frame buffer device\n", fbinfo->fix.id);

/*  nvt_fb_pan_display(&fbinfo->var, fbinfo);  */

ERR_REGISER_FB:
ERR_ALLOC:

	return ret;
}

int nvt_fb_deinit_ide_layer(struct nvt_fb_layer_para *layer_para)
{
	int fb_if;
	int ret = 0;

	DBG_IND("+\n");

	if ((layer_para == NULL) || (nvt_fb == NULL)) {
		DBG_ERR("NULL pointer found layer:0x%x nvt_fb:0x%x\n", (unsigned int)layer_para, (unsigned int)nvt_fb);
		return -1;
	}

	fb_if = NVT_FB_MAKE_FB_ID(layer_para->ide_id, layer_para->layer_id);

	if (nvt_fb->fb[fb_if] != NULL) {
		if (nvt_fb->free_ioremap[fb_if] == 1) {
			if (nvt_fb->fb[fb_if]->screen_base != NULL) {
				iounmap(nvt_fb->fb[fb_if]->screen_base);
			}
			nvt_fb->free_ioremap[fb_if] = 0;
		}

		ret = nvt_fb_free(nvt_fb->fb[fb_if]);
		nvt_fb->fb[fb_if] = NULL;
	} else {
		DBG_ERR("Release a NULL fb\n");
		return -EINVAL;
	}

	return ret;
}

int nvt_fb_get_layer_supported(unsigned int *layer_supported)
{
	int layer_index;
	int ide_index;

	if (layer_supported == NULL) {
		DBG_ERR("NULL pointer for save layer supported parameter\n");
		return -EINVAL;
	}

	*layer_supported = 0;

	for (ide_index = 0 ; ide_index <= NVT_MAX_IDE; ide_index++) {
		for (layer_index = 0 ; layer_index < 4; layer_index++) {
			if (display_obj_get_layer_support(ide_index, layer_list[layer_index]) == 0) {
				switch (ide_index) {
				case NVT_IDE1:
					switch (layer_list[layer_index]) {
					case IDE_DISPLAYER_VDO1:
						*layer_supported |= NVT_VIDEO_1;
						break;
					case IDE_DISPLAYER_VDO2:
						*layer_supported |= NVT_VIDEO_2;
						break;
					case IDE_DISPLAYER_OSD1:
						*layer_supported |= NVT_OSD_1;
						break;
					case IDE_DISPLAYER_OSD2:
						*layer_supported |= NVT_OSD_2;
						break;
					}
					break;

				case NVT_IDE2:
					switch (layer_list[layer_index]) {
					case IDE_DISPLAYER_VDO1:
						*layer_supported |= NVT_IDE2_VIDEO_1;
						break;
					case IDE_DISPLAYER_VDO2:
						*layer_supported |= NVT_IDE2_VIDEO_2;
						break;
					case IDE_DISPLAYER_OSD1:
						*layer_supported |= NVT_IDE2_OSD_1;
						break;
					case IDE_DISPLAYER_OSD2:
						*layer_supported |= NVT_IDE2_OSD_2;
						break;
					}
					break;
				}
			}
		}
	}

	DBG_IND("Layer maping:0x%x\n", *layer_supported);
	return 0;
}

static int nvt_fb_remove(struct platform_device *pdev)
{
	int fb_index;

	DBG_IND("+\n");

	if (nvt_fb == NULL)
		return -1;

	for (fb_index = 0 ; fb_index < NVT_TOTAL_FB ; fb_index++) {
		if (nvt_fb->fb[fb_index] != NULL) {
			if (nvt_fb->free_ioremap[fb_index] == 1) {
				if (nvt_fb->fb[fb_index]->screen_base != NULL) {
					iounmap(nvt_fb->fb[fb_index]->screen_base);
				}
				nvt_fb->free_ioremap[fb_index] = 0;
			}

			nvt_fb_free(nvt_fb->fb[fb_index]);
			nvt_fb->fb[fb_index] = NULL;
		}
	}

	kfree(nvt_fb);
	nvt_fb = NULL;
	return 0;
}

#ifdef CONFIG_PM
/**
 *	xxxfb_suspend - Optional but recommended function. Suspend the device.
 *	@dev: platform device
 *	@msg: the suspend event code.
 *
 *      See Documentation/power/devices.txt for more information
 */
static int nvt_fb_suspend(struct platform_device *pdev, pm_message_t msg)
{
	struct nvt_fb_info *info = platform_get_drvdata(pdev);

	/* suspend here */
	return 0;
}

/**
 *	xxxfb_resume - Optional but recommended function. Resume the device.
 *	@dev: platform device
 *
 *      See Documentation/power/devices.txt for more information
 */
static int nvt_fb_resume(struct platform_device *pdev)
{
	struct nvt_fb_info *info = platform_get_drvdata(pdev);

	/* resume here */
	return 0;
}
#else
#define nvt_fb_suspend NULL
#define nvt_fb_resume NULL
#endif /* CONFIG_PM */

static struct platform_driver nvt_fb_driver = {
	.probe = nvt_fb_probe,
	.remove = nvt_fb_remove,
	.suspend = nvt_fb_suspend, /* optional but recommended */
	.resume = nvt_fb_resume,   /* optional but recommended */
	.driver = {
		.name = "nvt_fb",
	},
};

static int nvt_init_fb_drv(void)
{
	int ret = 0;

	DBG_IND("\n");
	if (init != 0) {
		DBG_ERR("FB already initial\n");
		return -EALREADY;
	}

	init = 1;
	ret = platform_driver_register(&nvt_fb_driver);

	if (!ret) {
		nvt_fb_device = platform_device_register_simple("nvt_fb", 0, NULL, 0);

		if (IS_ERR(nvt_fb_device)) {
			platform_driver_unregister(&nvt_fb_driver);
			ret = PTR_ERR(nvt_fb_device);
			DBG_ERR("Device unregister err\n");

		}
	} else {
			DBG_ERR("driver register err\n");
	}

	return ret;
}

static void __exit nvt_deinit_fb_drv(void)
{
	DBG_ERR("\n");
	platform_device_unregister(nvt_fb_device);
	platform_driver_unregister(&nvt_fb_driver);
	init = 0;
}

/*
	MISC related functions
*/
static long nvt_fb_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct nvt_fb_layer_para layer_para_ker;
	unsigned int layer_supported;
	int ret = 0;

	switch (cmd) {

	case IOCTL_FB_LAYER_INIT:
		if (copy_from_user((void *)&layer_para_ker, (void *)arg, sizeof(struct nvt_fb_layer_para))) {
			DBG_ERR("copy_from_user  err cmd:0x%x\n", cmd);
			ret = -EINVAL;
			goto ERR_EXIT;
		}
		ret = nvt_fb_init_ide_layer(&layer_para_ker);
		break;

	case IOCTL_FB_LAYER_DEINIT:
		if (copy_from_user((void *)&layer_para_ker, (void *)arg, sizeof(struct nvt_fb_layer_para))) {
			DBG_ERR("copy_from_user  err cmd:0x%x\n", cmd);
			ret = -EINVAL;
			goto ERR_EXIT;
		}
		nvt_fb_deinit_ide_layer(&layer_para_ker);
		break;

	case IOCTL_FB_ID_GET_SUPPORTED:
		ret = nvt_fb_get_layer_supported(&layer_supported);
		if (ret == 0) {
			if (copy_to_user((void *)arg, (void *)&layer_supported, sizeof(unsigned int))) {
				DBG_ERR("copy_to_user  err cmd:0x%x\n", cmd);
				ret = -EINVAL;
				goto ERR_EXIT;
			}
		}
	break;

	default:
		DBG_ERR("Unknown cmd ID:0x%x\n", cmd);
		ret = -EINVAL;
	}

ERR_EXIT:

	return ret;
}

struct file_operations nvt_fb_misc_fops = {
	.owner		    = THIS_MODULE,
	.unlocked_ioctl = nvt_fb_miscdev_ioctl,
};

#if 0
static struct miscdevice fb_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= NVT_FB_DEV_NAME,
	.fops		= &nvt_fb_misc_fops,
};
#else

struct fb_misc_drv_info {
	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	//struct resource* presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;
};
static struct fb_misc_drv_info m_pdrv_info;
#endif

int nvt_fb_miscdev_init(void)
{
	struct fb_misc_drv_info *pdrv_info = &m_pdrv_info;
	int ret = 0;
	unsigned char ucloop;

	DBG_WRN("Register fb misc driver:%s\n", fb_misc_device.name);

#if 0
	ret = misc_register(&fb_misc_device);
	if (ret != 0) {
		nvt_dbg(ERR, "Failed to register fb misc device '%s'\n", fb_misc_device.name);
	}
#else
	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		pr_err("Can't get device ID\n");
		return -ENODEV;
	}

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_fb_misc_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		pr_err("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, MODULE_NAME);
	if(IS_ERR(pdrv_info->pmodule_class)) {
		pr_err("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
			, MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
			, MODULE_NAME);

		if(IS_ERR(pdrv_info->pdevice[ucloop])) {

			pr_err("failed in creating device%d.\n", ucloop);

			if (ucloop == 0) {
				device_unregister(pdrv_info->pdevice[ucloop]);
			}

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
	return ret;

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

#endif

	return ret;
}

int nvt_fb_miscdev_exit(void)
{
	struct fb_misc_drv_info *pdrv_info = &m_pdrv_info;
	int ret = 0;
	unsigned char ucloop;

	DBG_WRN("Deregister fb misc driver:%s\n", fb_misc_device.name);

#if 0
	misc_deregister(&fb_misc_device);
#else
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);
#endif

	return ret;
}

static int __init nvt_fb_init(void)
{
	int ret;

	ret = nvt_fb_miscdev_init();
	if (ret != 0)
		goto ERROR;

	ret = nvt_init_fb_drv();

	sema_init(&nvt_sem, 1);
ERROR:
	return ret;
}

static void __exit nvt_fb_exit(void)
{
	nvt_fb_miscdev_exit();
	nvt_deinit_fb_drv();
}

module_init(nvt_fb_init);
module_exit(nvt_fb_exit);
MODULE_LICENSE("GPL");
