#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <kwrap/file.h>
#include "affine_api.h"
#include "affine_drv.h"
#include "affine_dbg.h"
#include  <mach/fmem.h>
#include "emu_affine.h"


int nvt_affine_api_auto_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 1
		struct nvt_fmem_mem_info_t		buf_info = {0};
		static void *handle = NULL;
		int ret = 0;

		ret = nvt_fmem_mem_info_init(&buf_info, NVT_FMEM_ALLOC_CACHE, 0x600000, NULL);
		if (ret >= 0) {
			handle = fmem_alloc_from_cma(&buf_info, 0);
		}
		if (buf_info.vaddr != 0)
			emu_affine_auto((UINT32)buf_info.vaddr, buf_info.size);
#else

	frammap_buf_t      buf_info = {0};

	show_ddr_info();

	buf_info.size = 0x300000;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "nvt_alloc";
	buf_info.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info);

	nvt_dbg(IND, "VA: 0x%p, PA: 0x%x\n", buf_info.va_addr, buf_info.phy_addr);
	emu_affine_auto((UINT32)buf_info.va_addr, buf_info.size);

	nvt_dbg(IND, "%s: done\n", __func__);

	frm_free_buf_ddr(buf_info.va_addr);
#endif

	return 0;
}

#if 0
int nvt_affine_api_write_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
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

	nvt_affine_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}
#endif

int nvt_affine_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
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
	pbuffer = NULL;
	vos_file_close(fp);
	set_fs(old_fs);

	return len;
}

#if 0
int nvt_affine_api_read_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
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
	value = nvt_affine_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}
#endif
