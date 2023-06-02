#if defined(__LINUX)
#elif defined(__FREERTOS)
#include <rtos_na51089/interrupt.h>
#endif

#include "nvt_vdocdc_drv.h"
#include "nvt_vdocdc_dbg.h"
#include "h26x.h"

//VOS_MODULE_VERSION(VDOCDC, 1, 00, 000, 00);
//VOS_MODULE_VERSION(VDOCDC, KDRV_VDOCDC_VERSION_0, KDRV_VDOCDC_VERSION_1, KDRV_VDOCDC_VERSION_2, KDRV_VDOCDC_VERSION_3);

#if defined(__LINUX)
static void nvt_vdocdc_drv_do_tasklet(unsigned long data)
{
	MODULE_INFO *p_module_info = (MODULE_INFO *)data;
	//nvt_dbg(INFO, "\n");

	/* do something you want*/
	complete(&p_module_info->comp);
}

irqreturn_t nvt_h26x_drv_isr(int irq, void *devid)
{
	MODULE_INFO *p_module_info = (MODULE_INFO *)devid;

	/* simple triggle and response mechanism*/
	complete(&p_module_info->comp);

	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&p_module_info->tasklet);

	h26x_isr();//hk

    return IRQ_HANDLED;
}

int nvt_vdocdc_drv_init(MODULE_INFO *p_module_info)
{
	int iRet = 0;

	init_waitqueue_head(&p_module_info->wait_queue);
	spin_lock_init(&p_module_info->spinlock);
	sema_init(&p_module_info->sem, 1);
	init_completion(&p_module_info->comp);
	tasklet_init(&p_module_info->tasklet, nvt_vdocdc_drv_do_tasklet, (unsigned long)p_module_info);

	/* clock enable */
	clk_prepare(p_module_info->pclk[0]);
#if 0
	clk_enable(p_module_info->pclk[0]);
#endif

	DBG_INFO("clk_prepare %lu\n", (unsigned long)(clk_get_rate(p_module_info->pclk[0])));
	h26x_setClk((UINT32)clk_get_rate(p_module_info->pclk[0]));

	/* register IRQ here*/
	if(request_irq(p_module_info->iinterrupt_id[0], nvt_h26x_drv_isr, IRQF_TRIGGER_HIGH, "H26X_INT", p_module_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", p_module_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	DBG_INFO("IRQ = 0x%d\n", (int)(p_module_info->iinterrupt_id[0]));

	/* Add HW Module initialization here when driver loaded */

	h26x_setAPBAddr((UINT32)p_module_info->io_addr[0]);
	h26x_setRstAddr((UINT32)ioremap_nocache(0xF0020080, 0x10));

	h26x_create_resource();

	h26x_open();

	return iRet;

FAIL_FREE_IRQ:

	free_irq(p_module_info->iinterrupt_id[0], p_module_info);

	/* Add error handler here */

	return iRet;
}

int nvt_vdocdc_drv_remove(MODULE_INFO *p_module_info)
{
	//Free IRQ
	free_irq(p_module_info->iinterrupt_id[0], p_module_info);

	h26x_release_resource();

	h26x_close();

	return 0;
}
#elif defined(__FREERTOS)

static unsigned int g_h26x_chn_cnt = 0;

irqreturn_t nvt_h26x_drv_isr(int irq, void *devid)
{
	h26x_isr();//hk

    return IRQ_HANDLED;
}

int nvt_vdocdc_drv_init(void)
{
	if (g_h26x_chn_cnt == 0) {
		request_irq(INT_ID_H26X, nvt_h26x_drv_isr, IRQF_TRIGGER_HIGH, "H26X_INT", 0);

		h26x_create_resource();
		h26x_setAPBAddr(0xf0a10000);
		h26x_setRstAddr(0xf0020080);
		h26x_setClk(320000000);
		h26x_open();
	}

	g_h26x_chn_cnt++;

	return 0;
}

int nvt_vdocdc_drv_remove(void)
{
	g_h26x_chn_cnt--;

	if (g_h26x_chn_cnt == 0) {
		free_irq(INT_ID_H26X, 0);
		h26x_release_resource();
		h26x_close();
	}

	return 0;
}
#endif
