#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "dai_api.h"
#include "dai_drv.h"
#include "dai_dbg.h"
#include <kwrap/file.h>

#if(NVT_DAI_TEST_CMD ==1)

int nvt_dai_api_write_reg(PDAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value;

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

	if (reg_addr != 0) {
		nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);
		nvt_dai_drv_write_reg(pmodule_info, reg_addr, reg_value);
	}

	return 0;
}

int nvt_dai_api_write_pattern(PDAI_INFO pmodule_info, unsigned char argc, char **pargv)
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
	vos_file_close(fd);


	return len;
}

int nvt_dai_api_read_reg(PDAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (reg_addr != 0) {
		nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
		value = nvt_dai_drv_read_reg(pmodule_info, reg_addr);
		nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	}

	return 0;
}

#endif
