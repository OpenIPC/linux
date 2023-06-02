#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "pwm_api.h"
#include "pwm_drv.h"
#include "pwm_dbg.h"
#include <kwrap/file.h>

int nvt_pwm_api_write_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

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

	nvt_pwm_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

int nvt_pwm_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
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

int nvt_pwm_api_read_reg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
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

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_pwm_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

int nvt_pwm_api_open_pwm(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int channel, base_period, rising, falling, clk_div, cycle, inv, reload;

	if (argc != 8) {
		nvt_dbg(ERR, "wrong argument:%d\n", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &channel)) {
		nvt_dbg(ERR, "invalid channel:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[1], 0, &base_period)) {
		nvt_dbg(ERR, "invalid base period:%s\n", pargv[1]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[2], 0, &rising)) {
		nvt_dbg(ERR, "invalid rising:%s\n", pargv[2]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[3], 0, &falling)) {
		nvt_dbg(ERR, "invalid falling:%s\n", pargv[3]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[4], 0, &clk_div)) {
		nvt_dbg(ERR, "invalid clk div:%s\n", pargv[4]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[5], 0, &inv)) {
		nvt_dbg(ERR, "invalid invert:%s\n", pargv[5]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[6], 0, &cycle)) {
		nvt_dbg(ERR, "invalid cycle:%s\n", pargv[6]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[7], 0, &reload)) {
		nvt_dbg(ERR, "invalid cycle:%s\n", pargv[7]);
		return -EINVAL;
	}

	nvt_dbg(IND, "PWM: %d, base period: %d, rising: %d, falling %d, clk div: %d, inv: %d, cycle: %d, reload: %d\n", channel, base_period, rising, falling, clk_div, inv, cycle, reload);

	nvt_pwm_drv_open_pwm(channel, base_period, rising, falling, clk_div, inv, cycle, reload);

	return 0;
}

int nvt_pwm_api_close_pwm(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int channel;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d\n", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &channel)) {
		nvt_dbg(ERR, "invalid channel:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "PWM: %d\n", channel);

	nvt_pwm_drv_close_pwm(channel);

	return 0;
}

int nvt_pwm_api_open_mstep(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int channel, dir, phase_type, step_phase, threshold_en, threshold, clk_div, cycle;

	if (argc != 8) {
		nvt_dbg(ERR, "wrong argument:%d\n", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &channel)) {
		nvt_dbg(ERR, "invalid channel:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[1], 0, &dir)) {
		nvt_dbg(ERR, "invalid dir:%s\n", pargv[1]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[2], 0, &phase_type)) {
		nvt_dbg(ERR, "invalid phase_type:%s\n", pargv[2]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[3], 0, &step_phase)) {
		nvt_dbg(ERR, "invalid step_phase:%s\n", pargv[3]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[4], 0, &threshold_en)) {
		nvt_dbg(ERR, "invalid threshold_en:%s\n", pargv[4]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[5], 0, &threshold)) {
		nvt_dbg(ERR, "invalid threshold:%s\n", pargv[5]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[6], 0, &clk_div)) {
		nvt_dbg(ERR, "invalid clk_div:%s\n", pargv[6]);
		return -EINVAL;
	}

	if (kstrtouint(pargv[7], 0, &cycle)) {
		nvt_dbg(ERR, "invalid cycle:%s\n", pargv[7]);
		return -EINVAL;
	}

	nvt_dbg(IND, "MS: %d, dir: %d, phase_type: %d, step_phase %d, threshold_en: %d, threshold: %d, clk_div: %d, cycle: %d\n", channel, dir, phase_type, step_phase, threshold_en, threshold, clk_div, cycle);

	nvt_pwm_drv_open_mstep(channel, dir, phase_type, step_phase, threshold_en, threshold, clk_div, cycle);

	return 0;
}

int nvt_pwm_api_close_mstep(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int channel;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d\n", argc);
		return -EINVAL;
	}

	if (kstrtouint(pargv[0], 0, &channel)) {
		nvt_dbg(ERR, "invalid channel:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "PWM: %d\n", channel);

	nvt_pwm_drv_close_mstep(channel);

	return 0;
}

