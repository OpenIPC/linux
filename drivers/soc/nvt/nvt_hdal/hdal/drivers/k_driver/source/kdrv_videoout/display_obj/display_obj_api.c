#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "display_obj_api.h"
#include "display_obj_drv.h"
#include "display_obj_dbg.h"
#include <kwrap/file.h>

#ifdef CONFIG_NVT_SMALL_HDAL
#define VERF_API_EN 0
#else
#define VERF_API_EN 1
#endif

int nvt_dispobj_api_write_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_dispobj_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

int nvt_dispobj_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int len = 0;
	unsigned char *pbuffer;
	int fd;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	fd = vos_file_open(pargv[0], O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\n", pargv[0]);
		return -EFAULT;
	}

	pbuffer = kmalloc(256, GFP_KERNEL);
	if (pbuffer == NULL) {
		vos_file_close(fd);
		return -ENOMEM;
	}



	len = vos_file_read(fd, (void *)pbuffer, 256);

	/* Do something after get data from file */

	kfree(pbuffer);
	pbuffer = NULL;
	vos_file_close(fd);

	return len;
}

int nvt_dispobj_api_read_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_dispobj_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

#if VERF_API_EN
int nvt_dispobj_api_lcd_open(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int open;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &open)) {
		nvt_dbg(ERR, "invalid open:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "open lcd %d\n", open);

	nvt_dispobj_drv_lcd_open(pmodule_info, open);
	return 0;
}

int nvt_dispobj_api_tv_open(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int open;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &open)) {
		nvt_dbg(ERR, "invalid open:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "open tv %d\n", open);

	nvt_dispobj_drv_tv_open(pmodule_info, open);
	return 0;
}


int nvt_dispobj_api_fill_background(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int color;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &color)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "background color %d\n", color);

	nvt_dispobj_drv_fill_background(pmodule_info, color);
	return 0;
}

int nvt_dispobj_api_fill_video(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int format;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &format)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "video format %d\n", format);

	nvt_dispobj_drv_fill_video(pmodule_info, format);
	return 0;
}

int nvt_dispobj_api_fill_video2(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int format;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &format)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "video2 format %d\n", format);

	nvt_dispobj_drv_fill_video2(pmodule_info, format);
	return 0;
}

int nvt_dispobj_api_fill_osd(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int format;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &format)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "osd format %d\n", format);

	nvt_dispobj_drv_fill_osd(pmodule_info, format);
	return 0;
}

int nvt_dispobj_api_v1_scaling(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int w, h, window;

	if (argc != 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &w)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[1], 0, &h)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[1]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[2], 0, &window)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[2]);
		return -EINVAL;
	}

	nvt_dbg(IND, "video1 %s w %d, h %d\n", window ? "window" : "buffer", w, h);
	nvt_dispobj_drv_v1_scaling(pmodule_info, w, h, window);
	return 0;
}

int nvt_dispobj_api_v2_scaling(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int w, h, window;

	if (argc != 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &w)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[1], 0, &h)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[1]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[2], 0, &window)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[2]);
		return -EINVAL;
	}

	nvt_dbg(IND, "video2 %s w %d, h %d\n", window ? "window" : "buffer", w, h);
	nvt_dispobj_drv_v2_scaling(pmodule_info, w, h, window);
	return 0;
}

int nvt_dispobj_api_osd_scaling(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int w, h, window;

	if (argc != 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &w)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[1], 0, &h)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[1]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[2], 0, &window)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[2]);
		return -EINVAL;
	}

	nvt_dbg(IND, "osd %s w %d, h %d\n", window ? "window" : "buffer", w, h);
	nvt_dispobj_drv_osd_scaling(pmodule_info, w, h, window);
	return 0;
}
#endif
