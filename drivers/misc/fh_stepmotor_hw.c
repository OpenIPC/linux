#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/module.h>

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include   <mach/pmu.h>
#include	<linux/sched.h>
#include <linux/clk.h>

#include "fh_stepmotor.h"


irqreturn_t fh_stepmotor_isr(int irq, void *dev_id)
{
	struct fh_sm_obj_t* obj = (struct fh_sm_obj_t*)dev_id;

	writel(0,obj->regs+MOTOR_INT_EN);
	writel(0,obj->regs+MOTOR_INT_STATUS);
	writel(1,obj->regs+MOTOR_INT_EN);
	
	complete_all(&obj->run_complete);

	return IRQ_HANDLED;
}


int fh_stepmotor_is_busy(struct fh_sm_obj_t* obj)
{
	int busy = readl(obj->regs+MOTOR_STATUS0)&0x01;
	return busy;
}


void fh_stepmotor_set_hw_param(struct fh_sm_obj_t* obj,struct fh_sm_param* param)
{
	unsigned int reg;
	// mode 
	reg = readl(obj->regs+MOTOR_MODE);
	reg &= (~0x3);
	reg |= param->mode&0x3;
	// direction
	if (param->direction) {
		reg |= 0x1<<4;
	} else {
		reg &= (~(0x1<<4));
	}
	// inverse A/B
	if (param->output_invert_A) {
		reg |= 0x1<<5;
	} else {
		reg &= (~(0x1<<5));
	}
	if (param->output_invert_B) {
		reg |= 0x1<<6;
	} else {
		reg &= (~(0x1<<6));
	}
	// microstep
	reg &= (~(0xf<<8));
	reg |= ((param->timingparam.microstep & 0xf)<<8);
	writel(reg,obj->regs+MOTOR_MODE);
	
	// clk
	reg = readl(obj->regs+MOTOR_TIMING0);
	reg = (param->timingparam.period<<16)|param->timingparam.counter;
	writel(reg,obj->regs+MOTOR_TIMING0); // clk div 10*10

	
	//copy
	reg = readl(obj->regs+MOTOR_TIMING1);
	reg &= (~0xff);
	reg |= (param->timingparam.copy & 0xff);
	writel(reg,obj->regs+MOTOR_TIMING1); // clk div 10*10



	if (fh_sm_manual_4 == param->mode || fh_sm_manual_8 == param->mode) {
		writel(param->manual_pwm_choosenA, obj->regs+MOTOR_MANUAL_CONFIG0);
		writel(param->manual_pwm_choosenB, obj->regs+MOTOR_MANUAL_CONFIG1);
	}

	


	
	
	
}

void fh_stepmotor_get_hw_param(struct fh_sm_obj_t* obj,struct fh_sm_param* param)
{
	unsigned int reg;
	// mode 
	reg = readl(obj->regs+MOTOR_MODE);
	
	param->mode = reg&0x3;
	param->direction = (reg>>4)&0x1;
	param->output_invert_A = (reg>>5)&0x1;
	param->output_invert_B = (reg>>6)&0x1;


	// microstep
		param->timingparam.microstep = (reg>>8)&0xf;
	
	
	// clk
	reg = readl(obj->regs+MOTOR_TIMING0);
	param->timingparam.period = (reg>>16);
	param->timingparam.counter = reg&0xffff;
	
	
	//copy
	reg = readl(obj->regs+MOTOR_TIMING1);
	param->timingparam.copy = reg & 0xff;

	if (fh_sm_manual_4 == param->mode || fh_sm_manual_8 == param->mode) {
		param->manual_pwm_choosenA = readl(obj->regs+MOTOR_MANUAL_CONFIG0);
		param->manual_pwm_choosenB = readl(obj->regs+MOTOR_MANUAL_CONFIG1);
	}
}



int fh_stepmotor_start_sync(struct fh_sm_obj_t* obj,int cycles)
{
	int fin = 0;
	unsigned int regcycle = 0;
	//cycle
	if (cycles <= 0) return 0;

	reinit_completion(&obj->run_complete);

	cycles -=1;
	regcycle = readl(obj->regs+MOTOR_TIMING1);
	regcycle = regcycle & (0x0000ffff);
	regcycle = regcycle|((cycles<<16)&0xffff0000);
	writel(regcycle,obj->regs+MOTOR_TIMING1);

	writel(1,obj->regs+MOTOR_CTRL);
	wait_for_completion_interruptible(&obj->run_complete);
	fin = (readl(obj->regs+MOTOR_STATUS0)>>8)&0x1ffff;
	
	return fin;

}

void fh_stepmotor_start_async(struct fh_sm_obj_t* obj,int cycles)
{
	unsigned int regcycle = 0;
	
	if (cycles <= 0) return;
	cycles -= 1;
	regcycle = readl(obj->regs+MOTOR_TIMING1);
	regcycle = regcycle & (0x0000ffff);
	regcycle = regcycle|((cycles<<16)&0xffff0000);
	writel(regcycle,obj->regs+MOTOR_TIMING1);

	writel(1,obj->regs+MOTOR_CTRL);

}


int fh_stepmotor_stop(struct fh_sm_obj_t* obj)
{
	int fin = 0;
	if (fh_stepmotor_is_busy(obj)) {
		reinit_completion(&obj->run_complete);
		writel(0,obj->regs+MOTOR_CTRL);
		wait_for_completion_interruptible(&obj->run_complete);
		//wait_for_completion_interruptible_timeout(&obj->run_complete, HZ);

	}
	fin = (readl(obj->regs+MOTOR_STATUS0)>>8)&0x1ffff;
	return fin;
}
int fh_stepmotor_get_current_cycle(struct fh_sm_obj_t* obj)
{


	int fin = (readl(obj->regs+MOTOR_STATUS0)>>8)&0x1ffff;
	
	return fin;
}


int fh_stepmotor_set_lut(struct fh_sm_obj_t* obj,struct fh_sm_lut* lut)
{
	
	int i = 0;
	for (i = 0; i< lut->lutsize/sizeof(u32);i++) {
		writel(lut->lut[i],obj->regs+MOTOR_MEM+i*4);
	}
	
	
	
	return 5;
}
int fh_stepmotor_get_lutsize(struct fh_sm_obj_t* obj)
{
	return 256;
}
int fh_stepmotor_get_lut(struct fh_sm_obj_t* obj,struct fh_sm_lut* lut)
{
	int i;
	for (i = 0; i< lut->lutsize/sizeof(u32);i++) {
		lut->lut[i] = readl(obj->regs+MOTOR_MEM+i*4);
	}
	//lut->lut[0] = 0xfedc;
	
	return 5;
}
const u32 fhsm_deflut[] = {
0x00000080,0x0003007f,0x0006007f,0x0009007f,0x000c007f,0x000f007f,0x0012007e,0x0015007e,
0x0018007d,0x001c007c,0x001f007c,0x0022007b,0x0025007a,0x00280079,0x002b0078,0x002e0077,
0x00300076,0x00330075,0x00360073,0x00390072,0x003c0070,0x003f006f,0x0041006d,0x0044006c,
0x0047006a,0x00490068,0x004c0066,0x004e0064,0x00510062,0x00530060,0x0055005e,0x0058005c,
0x005a005a,0x005c0058,0x005e0055,0x00600053,0x00620051,0x0064004e,0x0066004c,0x00680049,
0x006a0047,0x006c0044,0x006d0041,0x006f003f,0x0070003c,0x00720039,0x00730036,0x00750033,
0x00760030,0x0077002e,0x0078002b,0x00790028,0x007a0025,0x007b0022,0x007c001f,0x007c001c,
0x007d0018,0x007e0015,0x007e0012,0x007f000f,0x007f000c,0x007f0009,0x007f0006,0x007f0003,
};


void fh_stepmotor_init_hw_param(struct fh_sm_obj_t* obj)
{
	int i = 0;
	// param
	writel(0,obj->regs+MOTOR_MODE);
	writel(0x800000,obj->regs+MOTOR_TIMING0); // clk div 128*1
	writel(0x10010,obj->regs+MOTOR_TIMING1);
	writel(0,obj->regs+MOTOR_MANUAL_CONFIG0);
	writel(0,obj->regs+MOTOR_MANUAL_CONFIG1);
	writel(1,obj->regs+MOTOR_INT_EN);
	// def lut
	for (i = 0; i< 64;i++) {
		writel(fhsm_deflut[i],obj->regs+MOTOR_MEM+i*4);
	}
}





