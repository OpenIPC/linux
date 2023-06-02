#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include <mach/fmem.h>

#include "jpeg_drv.h"
#include "jpeg_ioctl.h"



#include "jpeg_dbg.h"
//#include "../include/jpeg.h"
//#include "jpeg_enc.h"
#include "jpeg_file.h"
//#include "../include/jpg_enc.h"
#include "jpeg_platform.h"
#include "kdrv_jpeg_queue.h"
#include <plat-na51055/top.h>

extern void jpeg_isr(void);
//extern void jpg_patgen(void);
//extern void jpg_patdecgen(void);
//extern void emu_jpeg_encode(UINT32 dram_addr, UINT32 dram_size);
//extern void emu_jpeg_decode(UINT32 dram_addr, UINT32 dram_size);

extern struct clk *p_jpeg_clk;

UINT32      jpeg_clock;
/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_jpg_drv_wait_cmd_complete(PJPG_MODULE_INFO pmodule_info);
int nvt_jpg_drv_ioctl(unsigned char interface, JPG_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_jpg_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_jpg_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
//static DEFINE_SPINLOCK(my_lock);

//#define loc_cpu(myflags)   spin_lock_irqsave(&my_lock, myflags)
//#define unl_cpu(myflags)   spin_unlock_irqrestore(&my_lock, myflags)

typedef irqreturn_t (*irq_handler_t)(int, void *);


//KDRV_JPEG_TRIG_INFO g_jpeg_trig_info;

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
//int iEventFlag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_jpg_drv_open(PJPG_MODULE_INFO pmodule_info, unsigned char interface)
{
	nvt_dbg(INFO, "%d\n", interface);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_jpg_drv_release(PJPG_MODULE_INFO pmodule_info, unsigned char interface)
{
	nvt_dbg(INFO, "%d\n", interface);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_jpg_drv_init(JPG_MODULE_INFO *pmodule_info)
{
	int ret = 0;
	struct clk *pclk;

	init_waitqueue_head(&pmodule_info->jpg_wait_queue);
	//spin_lock_init(&pmodule_info->jpg_spinlock);
	sema_init(&pmodule_info->jpg_sem, 1);
	init_completion(&pmodule_info->jpg_completion);
	tasklet_init(&pmodule_info->jpg_tasklet, nvt_jpg_drv_do_tasklet, (unsigned long)pmodule_info);

	/* initial clock here */
	if  (nvt_get_chip_id() == CHIP_NA51055) {
		pclk = clk_get(NULL, "pllf320");
		if (IS_ERR(pclk)) {
			printk("%s: get source pllf320 fail\r\n", __func__);
		}
		jpeg_clock = 320;
	} else {
		pclk = clk_get(NULL, "fix480m");
		if (IS_ERR(pclk)) {
			printk("%s: get source fix480m fail\r\n", __func__);
		}
		jpeg_clock = 480;
	}
	clk_set_parent(pmodule_info->pclk[0], pclk);

	p_jpeg_clk = pmodule_info->pclk[0];
	clk_prepare(pmodule_info->pclk[0]);
	clk_enable(pmodule_info->pclk[0]);

	clk_put(pclk);
	//printk("clk_prepare\n");

	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_jpg_drv_isr, IRQF_TRIGGER_HIGH, "jpg_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	//printk("IRQ = 0x%d\n", (int)(pmodule_info->iinterrupt_id[0]));

	/* Add HW Module initialization here when driver loaded */
	jpeg_set_baseaddr((UINT32)pmodule_info->io_addr[0]);

	jpeg_create_resource();

	//jpg_patgen();
	//jpg_patdecgen();

	// init dsp trig info
	jpeg_platform_triginfo_init();
	//memset(&g_jpeg_trig_info, 0x00, sizeof(g_jpeg_trig_info));
	//g_jpeg_trig_info.p_queue = kdrv_jpeg_get_queue_by_coreid();
	//g_jpeg_trig_info.tri_func = (JPEG_TRI_FUNC) jpeg_trigger;


	// init jpeg queue
	kdrv_jpeg_queue_init_p();

	return ret;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return ret;
}

int nvt_jpg_drv_remove(JPG_MODULE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/

	return 0;
}

int nvt_jpg_drv_suspend(JPG_MODULE_INFO *pmodule_info)
{
	nvt_dbg(INFO, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_jpg_drv_resume(JPG_MODULE_INFO *pmodule_info)
{
	nvt_dbg(INFO, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_jpg_drv_ioctl(unsigned char interface, JPG_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg)
{
	//REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int loop;
	int ret = 0;

	nvt_dbg(INFO, "IF-%d cmd:%x\n", interface, cmd);

	switch (cmd) {
	case XXX_IOC_START:
		/*call someone to start operation*/
		break;

	case XXX_IOC_STOP:
		/*call someone to stop operation*/
		break;
#if 0
	case XXX_IOC_READ_REG:
		ret = copy_from_user(&reg_info, (void __user *)arg, sizeof(REG_INFO));
		if (!ret) {
			reg_info.reg_value = READ_REG(pmodule_info->io_addr[interface] + reg_info.reg_addr);
			ret = copy_to_user((void __user *)arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case XXX_IOC_WRITE_REG:
		ret = copy_from_user(&reg_info, (void __user *)arg, sizeof(REG_INFO));
		if (!ret) {
			WRITE_REG(reg_info.reg_value, pmodule_info->io_addr[interface] + reg_info.reg_addr);
		}
		break;

	case XXX_IOC_READ_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)arg, sizeof(REG_INFO_LIST));
		if (!ret) {
			for (loop = 0 ; loop < reg_info_list.reg_listcount; loop++) {
				reg_info_list.reg_list[loop].reg_value = READ_REG(pmodule_info->io_addr[interface] + reg_info_list.reg_list[loop].reg_addr);
			}

			ret = copy_to_user((void __user *)arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;

	case XXX_IOC_WRITE_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)arg, sizeof(REG_INFO_LIST));
		if (!ret)
			for (loop = 0 ; loop < reg_info_list.reg_listcount ; loop++) {
				WRITE_REG(reg_info_list.reg_list[loop].reg_value, pmodule_info->io_addr[interface] + reg_info_list.reg_list[loop].reg_addr);
			}
		break;
#endif

		/* Add other operations here */
	}

	return ret;
}

irqreturn_t nvt_jpg_drv_isr(int irq, void *devid)
{
	PJPG_MODULE_INFO pmodule_info = (PJPG_MODULE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->jpg_completion);

	jpeg_isr();

	//printk("tasklet_schedule\n");
	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->jpg_tasklet);
	nvt_dbg(INFO, "nvt_jpg_drv_isr\n");

	return IRQ_HANDLED;
}

int nvt_jpg_drv_wait_cmd_complete(PJPG_MODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->jpg_completion);
	return 0;
}

int nvt_jpg_drv_write_reg(PJPG_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_jpg_drv_read_reg(PJPG_MODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_jpg_drv_do_tasklet(unsigned long data)
{
	PJPG_MODULE_INFO pmodule_info = (PJPG_MODULE_INFO)data;

	nvt_dbg(INFO, "\n");
	//printk("nvt_jpg_drv_do_tasklet\r\n");
	jpeg_platform_ist(data);

	/* do something you want*/
	complete(&pmodule_info->jpg_completion);
}

#if 0
extern UINT32 testaddr;

void jpg_emu_enc(void)
{
	frammap_buf_t      buf_info = {0};

	//Allocate memory
	buf_info.size = 0x6000000;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "nvtmpp";
	buf_info.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info);

	printk("buf_info.va_addr = 0x%08x, buf_info.phy_addr = 0x%08x\r\n", (UINT32)buf_info.va_addr, buf_info.phy_addr);

	emu_jpeg_encode((UINT32)buf_info.va_addr, buf_info.size);
	frm_free_buf_ddr((void *)buf_info.va_addr);
}


void jpg_emu_dec(void)
{
	frammap_buf_t      buf_info = {0};

	//Allocate memory
	buf_info.size = 0x6000000;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "nvtmpp";
	buf_info.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info);

	printk("buf_info.va_addr = 0x%08x, buf_info.phy_addr = 0x%08x\r\n", (UINT32)buf_info.va_addr, buf_info.phy_addr);

	emu_jpeg_decode((UINT32)buf_info.va_addr, buf_info.size);
	frm_free_buf_ddr((void *)buf_info.va_addr);
}
#endif

#if 0
ER jpeg_add_queue(JPEG_CODEC_MODE codec_mode, void *p_param, KDRV_CALLBACK_FUNC *p_cb_func)
{
	KDRV_VDOENC_PARAM *p_enc_param;
	KDRV_VDODEC_PARAM *p_dec_param;
	KDRV_JPEG_TRIG_INFO    *p_trig;
	BOOL                is_block_mode = FALSE;

	ER  ret = 0;

	if (1) {

		loc_cpu();

		p_enc_param = (KDRV_VDOENC_PARAM *)p_param;

		if (p_cb_func == NULL || p_cb_func->callback == NULL) {

			//wait_flg_ptn = kdrv_rpc_get_free_bit_ptn_p();
			//if (wait_flg_ptn == 0) {
			//	DBG_ERR("No free bit_ptn\r\n");
			//	unl_cpu();
			//	return KDRV_RPC_ER_SYS;
			//}
			is_block_mode = TRUE;
		}
		p_trig = &g_jpeg_trig_info;

		// check if need add cmd to queue
		if (p_trig->is_busy == TRUE) {
			KDRV_JPEG_QUEUE_ELEMENT   element;

			element.p_cb_func   =  p_cb_func;
			if (codec_mode == JPEG_CODEC_MODE_ENC) {
				element.jpeg_enc_param = (KDRV_VDOENC_PARAM *) p_param;
				element.jpeg_mode = JPEG_CODEC_MODE_ENC;
			} else {
				element.jpeg_dec_param = (KDRV_VDODEC_PARAM *) p_param;
				element.jpeg_mode = JPEG_CODEC_MODE_DEC;
			}

			//element.p_user_data = p_user_data;
			//element.flg_ptn   = wait_flg_ptn;
			if (kdrv_jpeg_queue_add_p(p_trig->p_queue, &element) == -3/*KDRV_RPC_ER_QUEUE_FULL*/) {
				unl_cpu();
				return -3/*KDRV_RPC_ER_QUEUE_FULL*/;
			}
			unl_cpu();
		} else {
			p_trig->is_busy     = TRUE;
			p_trig->cb          = p_cb_func;
			//p_trig->user_data   = p_user_data;
			//p_trig->flg_ptn     = wait_flg_ptn;
			unl_cpu();

			p_trig->tri_func(codec_mode, p_param, p_cb_func);

			//p_trig->cc_send_cmd((PCC_CMD)p_rpc_param->cmd_buf);
		}

		if (is_block_mode) {
			//printk("wait cmd finish\r\n");
			//wai_flg(&flg_ptn, g_rpc_flgid, wait_flg_ptn, TWF_ORW|TWF_CLR);
			jpeg_platform_wait_flg();
			//kdrv_rpc_rel_bit_ptn_p(wait_flg_ptn);
			//printk("cmd finish\r\n");

			while (1) {
				UINT32  int_sts;

				int_sts = jpeg_get_status();

				if (int_sts & JPEG_INT_FRAMEEND) {
				    //printk("JPEG_INT_FRAMEEND\r\n");
					break;
				} else if (int_sts & JPEG_INT_BUFEND) {
					//printk("JPEG_INT_BUFEND\r\n");
					if (codec_mode == JPEG_CODEC_MODE_DEC) {
						p_dec_param = (KDRV_VDODEC_PARAM *) p_param;

						// Interrupt will be disabled in ISR, need to enable again
						jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND);
						jpeg_set_bsstartaddr(p_dec_param->bs_addr, p_dec_param->bs_size);
					}
				}
			}

			if (codec_mode == JPEG_CODEC_MODE_ENC) {
				nvt_dbg(INFO, "jpeg_add_queue - encode done\n");
			} else {
				nvt_dbg(INFO, "jpeg_add_queue - decode done\n");
			}
		}
		//printk("jpeg_add_queue - encode done\r\n");
	} else {
		p_dec_param = (KDRV_VDODEC_PARAM *)p_param;
		//printk("jpeg_add_queue - decode done\r\n");
	}
	return ret;
}
#endif

#endif

//KDRV_JPEG_TRIG_INFO *kdrv_jpeg_get_triginfo_by_coreid(void)
//{
//	return &g_jpeg_trig_info;
//}

