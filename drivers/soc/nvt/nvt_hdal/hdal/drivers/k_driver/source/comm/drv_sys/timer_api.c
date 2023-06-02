#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <kwrap/file.h>
#include "timer_api.h"
#include "timer_drv.h"
#include "timer_dbg.h"
#include "emu_timer.h"

int nvt_timer_api_auto_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
/*
        emu_timer_auto();
*/
        nvt_dbg(IND, "%s: done\n", __func__);

        return 0;
}

#if 0
int nvt_timer_api_write_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr, reg_value;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul (pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_timer_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}
#endif

int nvt_timer_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	mm_segment_t old_fs;
	int fp;
	int len = 0;
	unsigned char *pbuffer;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	fp = vos_file_open(pargv[0], O_RDONLY, 0);
	if (-1 == fp) {
	    nvt_dbg(ERR, "failed in file open:%s\n", pargv[0]);
		return -EFAULT;
	}

	pbuffer = kmalloc(256, GFP_KERNEL);
	if (pbuffer == NULL) {
		vos_file_close(fp);
		return -ENOMEM;
	}

	old_fs = get_fs();
	set_fs(get_ds());

	len = vos_file_read(fp, pbuffer, 256);

	/* Do something after get data from file */

	kfree(pbuffer);
	vos_file_close(fp);
	set_fs(old_fs);

	return len;
}

#if 0
int nvt_timer_api_read_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_timer_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}
#endif
