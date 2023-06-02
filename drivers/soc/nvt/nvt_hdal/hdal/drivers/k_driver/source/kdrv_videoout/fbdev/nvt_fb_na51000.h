/*
 * linux/drivers/video/s3c2410fb.h
 *	Copyright (c) 2004 Arnaud Patard
 *
 *  S3C2410 LCD Framebuffer Driver
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
*/
#ifndef __NVT_FRAMEBUFFER_H__
#define __NVT_FRAMEBUFFER_H__

#include <linux/fb.h>

#define NVT_FB_BLANK                            (0x1)
/* 1: blank   0:unblank */
#define NVT_FB_GET_IDE_ID(x)                    ((x >> 2) & 0xF)
#define NVT_FB_GET_LAYER_ID(x)                  (x & 0x3)
#define NVT_FB_MAKE_FB_ID(IDE, LAYER_ID)        (((IDE & 0xF) << 2) | (LAYER_ID & 0x3))
#define NVT_FB_IS_OSD(x)                        (x & 0x2)
#define NVT_FB_IS_VIDEO(x)                      (!(x & 0x2))

#define NVT_FB_DEV_NAME	                        "fb_misc"
#define NVT_FB_CREATE_BUFFER_BY_FB              0
#define NVT_FB_GET_BUFFER_FROM_IDE              0
#define NVT_FB_GET_BUFFER_FROM_HDAL             1

#define NVT_TOTAL_FB                            8
#define MAX_FB_NAME                             8
#define NVT_OSD_PAN_DISPLAY_NUM                 2
#define NVT_VD_PAN_DISPLAY_NUM                  1


enum {
	IDE_DISPLAYER_OSD1 = 0x02,                  ///< select display layer osd 1
	IDE_DISPLAYER_OSD2 = 0x04,                  ///< select display layer osd 2
	IDE_DISPLAYER_VDO1 = 0x08,                  ///< select display layer video 1
	IDE_DISPLAYER_VDO2 = 0x10                  ///< select display layer video 2
};

/* private data that is shared between all framebuffer */
struct nvt_fb_info {
	struct device *dev;
	struct fb_info *fb[NVT_TOTAL_FB];
	unsigned char free_ioremap[NVT_TOTAL_FB];
/*
	unsigned long clk_rate;
*/
};

/* framebuffer private data */
struct nvt_fb_par {
	unsigned int fb_id;

	struct resource		*mem;
	void __iomem		*io;
	void __iomem		*irq_base;

	unsigned int		flags;
	unsigned int		palette_ready;

#ifdef CONFIG_CPU_FREQ
	struct notifier_block	freq_transition;
#endif

	/* keep these registers in case we need to re-write palette */
	/*u32 palette_buffer[256]; palette info was retain in IDE driver, */
	/*                         so we don't need it in frame buffer*/
	unsigned int palette_sz;

	struct nvt_fb_info *info;
};

int nvt_fb_alloc(struct nvt_fb_info *pnvt_fb_info, unsigned int ide_id, unsigned int fb_if);
void nvt_fb_unmap_video_memory(struct fb_info *info);
int nvt_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);

int nvt_fb_video_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
int nvt_fb_video_probe(struct fb_info *fbinfo, unsigned int ide_id, unsigned int fb_if);

int nvt_fb_osd_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
int nvt_fb_osd_probe(struct fb_info *fbinfo, unsigned int ide_id, unsigned int fb_if);

#endif
