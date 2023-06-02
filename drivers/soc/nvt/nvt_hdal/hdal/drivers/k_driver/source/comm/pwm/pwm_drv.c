#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
//#include "frammap/frammap_if.h"
#include "pwm_drv.h"
#include "pwm_ioctl.h"
#include "pwm_dbg.h"
#include "comm/pwm.h"
#include "pwm_platform.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_pwm_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_pwm_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_pwm_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_pwm_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_pwm_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_pwm_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_pwm_drv_init(MODULE_INFO *pmodule_info)
{
	int err = 0;

	init_waitqueue_head(&pmodule_info->pwm_wait_queue);
	spin_lock_init(&pmodule_info->pwm_spinlock);
	sema_init(&pmodule_info->pwm_sem, 1);
	init_completion(&pmodule_info->pwm_completion);
	tasklet_init(&pmodule_info->pwm_tasklet, nvt_pwm_drv_do_tasklet, (unsigned long)pmodule_info);

	/* initial clock here */



	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_pwm_drv_isr, IRQF_TRIGGER_HIGH, "PWM_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		err = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	pwm_platform_create_resource(pmodule_info);


	/* Add HW Module initialization here when driver loaded */

	return err;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return err;
}

int nvt_pwm_drv_remove(MODULE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/

	pwm_platform_release_resource();

	return 0;
}

int nvt_pwm_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_pwm_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_pwm_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long argc)
{
	//REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int loop_count;
	int err = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", if_id, cmd);



	switch (cmd) {
	case PWM_IOC_START:
		/*call someone to start operation*/
		break;

	case PWM_IOC_STOP:
		/*call someone to stop operation*/
		break;
#if 0
	case PWM_IOC_READ_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err) {
			reg_info.reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info.reg_addr);
			err = copy_to_user((void __user *)argc, &reg_info, sizeof(REG_INFO));
		}
		break;

	case PWM_IOC_WRITE_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err)
			WRITE_REG(reg_info.reg_value, pmodule_info->io_addr[if_id] + reg_info.reg_addr);
		break;

	case PWM_IOC_READ_REG_LIST:
		err = copy_from_user(&reg_info_list, (void __user *)argc, sizeof(REG_INFO_LIST));
		if (!err) {
			for (loop_count = 0 ; loop_count < reg_info_list.reg_cnt; loop_count++)
				reg_info_list.reg_list[loop_count].reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info_list.reg_list[loop_count].reg_addr);

			err = copy_to_user((void __user *)argc, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case PWM_IOC_WRITE_REG_LIST:
		err = copy_from_user(&reg_info_list, (void __user *)argc, sizeof(REG_INFO_LIST));
		if (!err)
			for (loop_count = 0 ; loop_count < reg_info_list.reg_cnt ; loop_count++)
				WRITE_REG(reg_info_list.reg_list[loop_count].reg_value, pmodule_info->io_addr[if_id] + reg_info_list.reg_list[loop_count].reg_addr);
		break;
#endif
		/* Add other operations here */
	}

	return err;
}

irqreturn_t nvt_pwm_drv_isr(int irq, void *devid)
{
	//PMODULE_INFO pmodule_info = (PMODULE_INFO)devid;

	/* simple triggle and response mechanism*/
	//complete(&pmodule_info->pwm_completion);


	/*  Tasklet for bottom half mechanism */
	//tasklet_schedule(&pmodule_info->pwm_tasklet);
	pwm_isr();

	return IRQ_HANDLED;
}

int nvt_pwm_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->pwm_completion);
	return 0;
}

int nvt_pwm_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_pwm_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_pwm_drv_do_tasklet(unsigned long data)
{
	PMODULE_INFO pmodule_info = (PMODULE_INFO)data;

	nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pmodule_info->pwm_completion);
}


void nvt_pwm_drv_open_pwm(unsigned long channel, unsigned long base_period, unsigned long rising, unsigned long falling, unsigned long clk_div, unsigned long inv, unsigned long cycle, unsigned long reload)
{
	PWM_CFG cfg;
	/*
	void __iomem *top_addr;
	UINT32 tmp_reg;

	top_addr = ioremap_nocache(0xf0010000, 0x200);

	// pwm pinmux enable
	if (channel > 7) {
		tmp_reg = READ_REG(top_addr + 0x18);

		tmp_reg |= (0x10000 << ((channel - 8) * 3) );

		WRITE_REG(tmp_reg, top_addr + 0x18);
	} else {
		tmp_reg = READ_REG(top_addr + 0x1c);

		tmp_reg |= (0x1 << (channel * 3) );

		WRITE_REG(tmp_reg, top_addr + 0x1c);
	}

	// pgpio function change
	tmp_reg = READ_REG(top_addr + 0xa8);

	tmp_reg &= ~(0x1 << channel);

	WRITE_REG(tmp_reg, top_addr + 0xa8);
	*/

	if (reload) {
		pwm_pwm_reload_config(0x1 << channel, (INT32)rising, (INT32)falling, (INT32)base_period);

		pwm_pwm_reload(0x1 << channel);

	} else {
		// pwm open, config, and start
		pwm_open(0x1 << channel);

		if (channel < 4)
			pwm_pwm_config_clock_div(PWM0_3_CLKDIV, clk_div);
		else if ((channel >= 4) && (channel < 8))
			pwm_pwm_config_clock_div(PWM4_7_CLKDIV, clk_div);
		else
			pwm_pwm_config_clock_div(channel, clk_div);

		cfg.ui_prd = base_period;
		cfg.ui_rise = rising;
		cfg.ui_fall = falling;
		cfg.ui_inv = inv;
		cfg.ui_on_cycle = cycle;

		pwm_pwm_config((0x1 << channel), &cfg);

		pwm_pwm_enable(0x1 << channel);

		if (cycle != 0) {
			pwm_wait((0x1 << channel), PWM_TYPE_PWM);

			pwm_close((0x1 << channel), FALSE);
/*
			// pinmux disable
			tmp_reg = READ_REG(top_addr + 0x1c);

			tmp_reg &= ~(0x1 << channel);

			WRITE_REG(tmp_reg, top_addr + 0x1c);

			// pgpio function change
			tmp_reg = READ_REG(top_addr + 0xa8);

			tmp_reg |= (0x1 << channel);

			WRITE_REG(tmp_reg, top_addr + 0xa8);
*/
		}
	}
}

void nvt_pwm_drv_close_pwm(unsigned long channel)
{
	//void __iomem *top_addr;
	//UINT32 tmp_reg;

	//top_addr = ioremap_nocache(0xf0010000, 0x200);

	pwm_close((0x1 << channel), TRUE);
/*
	// pinmux disable
	tmp_reg = READ_REG(top_addr + 0x1c);

	tmp_reg &= ~(0x1 << channel);

	WRITE_REG(tmp_reg, top_addr + 0x1c);

	// pgpio function change
	tmp_reg = READ_REG(top_addr + 0xa8);

	tmp_reg |= (0x1 << channel);

	WRITE_REG(tmp_reg, top_addr + 0xa8);
*/
}

void nvt_pwm_drv_open_mstep(unsigned long channel, unsigned long dir, unsigned long phase_type, unsigned long step_phase, unsigned long threshold_en, unsigned long threshold, unsigned long clk_div, unsigned long cycle)
{
	MS_CH_PHASE_CFG phase_cfg;
	MSCOMMON_CFG cfg;
	//void __iomem *top_addr;
	//UINT32 tmp_reg;
/*
	top_addr = ioremap_nocache(0xf0010000, 0x200);

	// pwm pinmux enable
	tmp_reg = READ_REG(top_addr + 0x1c);

	tmp_reg |= (0x249 << (channel * 12));

	WRITE_REG(tmp_reg, top_addr + 0x1c);

	// pgpio function change
	tmp_reg = READ_REG(top_addr + 0xa8);

	tmp_reg &= ~(0xf << (channel * 4));

	WRITE_REG(tmp_reg, top_addr + 0xa8);
*/
	// ms open, config, and start
	pwm_open_set((PWM_MS_CHANNEL_SET)channel);

	pwm_mstep_config_clock_div((PWM_MS_CHANNEL_SET)channel, clk_div);

	phase_cfg.ui_ch0_phase = 0;
	phase_cfg.ui_ch1_phase = 2;
	phase_cfg.ui_ch2_phase = 4;
	phase_cfg.ui_ch3_phase = 6;

	cfg.ui_dir = dir;
	cfg.ui_phase_type = phase_type;
	cfg.ui_step_per_phase = step_phase;
	cfg.ui_threshold_en = threshold_en;
	cfg.ui_threshold = threshold;
	cfg.ui_on_cycle = cycle;
	cfg.is_square_wave = 0;

	pwm_mstep_config_set((PWM_MS_CHANNEL_SET)channel, &phase_cfg, &cfg);

	pwm_pwm_enable_set((PWM_MS_CHANNEL_SET)channel);

	pwm_mstep_enable_set((PWM_MS_CHANNEL_SET)channel);

	if (cycle != 0) {
		pwm_wait_set((PWM_MS_CHANNEL_SET)channel);

		pwm_close_set((PWM_MS_CHANNEL_SET)channel, FALSE);
/*
		// pinmux disable
		tmp_reg = READ_REG(top_addr + 0x1c);

		tmp_reg &= ~(0xf << (channel * 4));

		WRITE_REG(tmp_reg, top_addr + 0x1c);

		// pgpio function change
		tmp_reg = READ_REG(top_addr + 0xa8);

		tmp_reg |= (0xf << (channel * 4));

		WRITE_REG(tmp_reg, top_addr + 0xa8);
*/
	}
}

void nvt_pwm_drv_close_mstep(unsigned long channel)
{
	//oid __iomem *top_addr;
	//UINT32 tmp_reg;

	//top_addr = ioremap_nocache(0xf0010000, 0x200);

	pwm_mstep_disable_set((PWM_MS_CHANNEL_SET) channel);

	pwm_close_set((PWM_MS_CHANNEL_SET)channel, TRUE);
/*
	// pinmux disable
	tmp_reg = READ_REG(top_addr + 0x1c);

	tmp_reg &= ~(0xf << (channel * 4));

	WRITE_REG(tmp_reg, top_addr + 0x1c);

	// pgpio function change
	tmp_reg = READ_REG(top_addr + 0xa8);

	tmp_reg |= (0xf << (channel * 4));

	WRITE_REG(tmp_reg, top_addr + 0xa8);
*/
}

