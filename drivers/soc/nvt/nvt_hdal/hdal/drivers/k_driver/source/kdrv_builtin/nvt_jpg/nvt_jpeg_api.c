#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <kwrap/file.h>
#include "jpeg_api.h"
#include "jpeg_drv.h"
#include "jpeg_dbg.h"
#include "jpeg_proc.h"
#include "jpeg_file.h"



int nvt_jpg_api_write_reg(PJPG_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr, reg_value;

	printk("nvt_jpg_api_write_reg\r\n");

	if (argc != 2) {
		//nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		//nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		//nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	//nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	//nvt_jpg_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}


UINT32 testaddr;
int nvt_jpg_api_write_pattern(PJPG_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
//	mm_segment_t old_fs;
//	int fp;
	int len = 0;
//	unsigned char *pbuffer;
//	FST_FILE filehdl;
//	UINT32 fsize;
//	INT32 fstatus;
//	UINT8 *pbuf;

	printk("nvt_jpg_api_write_pattern\r\n");
#if 0
	filehdl = filesys_openfile("/mnt/sd/01.raw", FST_OPEN_READ);
	if (filehdl == NULL) {
		//auto_msg(("%s: open file %s fail\r\n", __func__, vGrphAutoLineBuf));
		//return AUTOTEST_RESULT_FAIL;
		printk("GG\r\n");
	}
	fsize = 320 * 240;
	fstatus = filesys_readfile(filehdl, (UINT8 *)testaddr, &fsize, 0, NULL);
	pbuf = (UINT8 *)testaddr;

	printk("data = 0x%x 0x%x 0x%x 0x%x\r\n", (unsigned int)(*pbuf), (unsigned int)(*(pbuf + 1)), (unsigned int)(*(pbuf + 2)), (unsigned int)(*(pbuf + 3)));
	printk("data = 0x%x 0x%x 0x%x 0x%x\r\n", (unsigned int)(*(pbuf + 0x2000)), (unsigned int)(*(pbuf + 0x2001)), (unsigned int)(*(pbuf + 0x3000)), (unsigned int)(*(pbuf + 0x3001)));
	if (fstatus != 0) {
		//auto_msg(("%s: read file %s fail\r\n", __func__, vGrphAutoLineBuf));
		filesys_closefile(filehdl);
		//return AUTOTEST_RESULT_FAIL;
		printk("GG2\r\n");
	}
	filesys_closefile(filehdl);
	printk("done\r\n");


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
	nvt_dbg(INFO, "pbuffer = 0x%x\n", (unsigned int)pbuffer);
	vos_file_close(fp);
	set_fs(old_fs);
#endif
	return len;
}

int nvt_jpg_api_read_reg(PJPG_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr;
	//unsigned long value;

	printk("nvt_jpg_api_read_reg\r\n");

	if (argc != 1) {
		//nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		//nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	//nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	//value = nvt_jpg_drv_read_reg(pmodule_info, reg_addr);

	//nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}
#endif