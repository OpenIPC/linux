#ifndef __NVT_FB_H__
#define __NVT_FB_H__

enum NVT_IDE_LAYER {
	NVT_LAYER_VIDEO_1,
	NVT_LAYER_VIDEO_2,
	NVT_LAYER_OSD_1,
	NVT_LAYER_OSD_2,
	NVT_MAX_LAYER = NVT_LAYER_OSD_2
};

enum NVT_FB_IDE_IF {
	NVT_IDE1 = 0,
	NVT_IDE2,
	NVT_MAX_IDE = NVT_IDE2
};

enum NVT_FB_IF_SUPPORTED {
	NVT_VIDEO_1      = 0x1,
	NVT_VIDEO_2      = 0x2,
	NVT_OSD_1        = 0x4,
	NVT_OSD_2        = 0x8,
	NVT_IDE2_VIDEO_1 = 0x10,
	NVT_IDE2_VIDEO_2 = 0x20,
	NVT_IDE2_OSD_1	 = 0x40,
	NVT_IDE2_OSD_2	 = 0x80
};

/* This structure is used to initial an IDE layer */
struct nvt_fb_layer_para {
	unsigned int   fb_va_addr;
	unsigned int   fb_pa_addr;
	unsigned int   buffer_len;
	unsigned char  ide_id;
	unsigned char  layer_id;
};

#define FB_IOC_MAGIC       'F'

/* graphics information setting, the graphics are the source patterns(fonts or images)  that blit to target */
#define IOCTL_FB_LAYER_INIT              _IOWR(FB_IOC_MAGIC, 3, struct nvt_fb_layer_para)
#define IOCTL_FB_LAYER_DEINIT            _IOW(FB_IOC_MAGIC, 4, struct nvt_fb_layer_para)
#define IOCTL_FB_ID_GET_SUPPORTED        _IOR(FB_IOC_MAGIC, 5, unsigned int)

#endif
