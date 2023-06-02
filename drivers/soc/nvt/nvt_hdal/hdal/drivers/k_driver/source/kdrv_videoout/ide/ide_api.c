#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "ide_api.h"
#include "ide_drv.h"
#include "ide_dbg.h"
#include <kwrap/file.h>

int nvt_ide_api_write_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
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

	nvt_ide_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

int nvt_ide_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
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

int nvt_ide_api_read_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
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
	value = nvt_ide_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

int nvt_ide_api_dump(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int id;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &id)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R IDE %d\n", id);
	nvt_ide_drv_dump(pmodule_info, id);

	return 0;
}

