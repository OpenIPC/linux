#ifdef __KERNEL
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#endif

#if !defined __FREERTOS
//#include "timer_ioctl.h"
#include "timer_dbg.h"
#include "timer_drv.h"
#include "timer_platform.h"
#else
#include "include/timer_drv.h"
#include "include/timer_platform.h"
#endif

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
#if !defined __FREERTOS
int nvt_timer_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_timer_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
static int log_timer_slience = 0;
static int losing_event_count[20] = {0};
#endif
void nvt_timer_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_timer_drv_isr(int irq, void *devid);
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
#if !defined __FREERTOS
int nvt_timer_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_timer_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}
#endif

#if defined __FREERTOS
irq_bh_handler_t nvt_timer_bh_ist(int irq, unsigned long event, void *data)
{
	nvt_timer_drv_do_tasklet(event);

	return (irq_bh_handler_t) IRQ_HANDLED;
}

int nvt_timer_drv_init(void)
{
	timer_platform_create_resource();

	request_irq(INT_ID_TIMER, nvt_timer_drv_isr, IRQF_TRIGGER_HIGH, "timer", 0);

	request_irq_bh(INT_ID_TIMER, (irq_bh_handler_t) nvt_timer_bh_ist, IRQF_BH_PRI_HIGH);
	return 0;
}
#else
int nvt_timer_drv_init(MODULE_INFO *pmodule_info)
{
	int err = 0;

	init_waitqueue_head(&pmodule_info->wait_queue);
	//spin_lock_init(&pmodule_info->xxx_spinlock);
	//sema_init(&pmodule_info->xxx_sem, 1);
	tasklet_init(&pmodule_info->tasklet, nvt_timer_drv_do_tasklet, (unsigned long)pmodule_info);
	nvt_timer_drv_ioctl_init();

	/* allocate graphic resource here */
    timer_platform_create_resource(pmodule_info);

	/* initial clock here */



	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_timer_drv_isr, IRQF_TRIGGER_HIGH | IRQF_SHARED, "TIMER_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		err = -ENODEV;
		goto FAIL_FREE_IRQ;
	}


	/* Add HW Module initialization here when driver loaded */

	return err;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return err;
}
#endif

#if defined __FREERTOS
int nvt_timer_drv_remove(void)
{
	free_irq(INT_ID_TIMER, 0);

	free_irq_bh(INT_ID_TIMER, 0);

	timer_platform_release_resource();
	return 0;
}
#else
int nvt_timer_drv_remove(MODULE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/

	/* release OS resources */
        timer_platform_release_resource();


	return 0;
}
#endif

#if !defined __FREERTOS
int nvt_timer_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_timer_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}
#endif

irqreturn_t nvt_timer_drv_isr(int irq, void *devid)
{
//	PMODULE_INFO pmodule_info = (PMODULE_INFO)devid;
	UINT32 flags;
	int ret;

        // Accquire available timer ID
	flags =	timer_platform_spin_lock();

	ret = timer_isr();

	timer_platform_spin_unlock(flags);
	/* simple triggle and response mechanism*/
//	complete(&pmodule_info->xxx_completion);


	/*  Tasklet for bottom half mechanism */
//	tasklet_schedule(&pmodule_info->xxx_tasklet);

	return ret;
}

#if !defined __FREERTOS
int nvt_timer_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	//wait_for_completion(&pmodule_info->xxx_completion);
	return 0;
}
#endif

#if 0
int nvt_timer_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}
#endif

#if 0
int nvt_timer_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}
#endif

void nvt_timer_drv_do_tasklet(unsigned long data)
{
#if (TIMER_BOTTOMHALF_SEL == TIMER_BOTTOMHALF_TASKLET)
	timer_platform_ist(data);
#endif
//	PMODULE_INFO pmodule_info = (PMODULE_INFO)data;

//	nvt_dbg(IND, "\n");

	/* do something you want*/
//	complete(&pmodule_info->xxx_completion);
}

#if !defined __FREERTOS
int nvt_get_silence_timer(void)
{
	return log_timer_slience;
}

void nvt_set_silence_timer(int timer_number)
{
	log_timer_slience |= (0x1 << timer_number);
}

void nvt_clear_silence_timer(void)
{
	log_timer_slience = 0;
}

int nvt_get_losing_event_count(int timer_number)
{
	if (timer_number < 20)
		return losing_event_count[timer_number];
	else
		return -1;
}

void nvt_set_losing_event_count(int timer_number)
{
	if (timer_number < 20)
		losing_event_count[timer_number]++;
}
#endif
