#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "sif_drv.h"
#include "sif_ioctl.h"
#include "sif_dbg.h"
#include "sif.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_sif_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_sif_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_sif_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
static unsigned long sif_lock_flags;
//UINT32 _REGIOBASE = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
static int nvt_sif_channel_open(SIF_CH Ch)
{
    return sif_open(Ch);
}

static int nvt_sif_channel_close(SIF_CH Ch)
{
    return sif_close(Ch);
}

static int nvt_sif_channel_isopened(SIF_CH Ch)
{
    return sif_isOpened(Ch);
}

static void nvt_sif_setConfig(SIF_CH Ch, SIF_CONFIG_ID ConfigID, UINT32 uiConfig)
{
    sif_setConfig(Ch, ConfigID, uiConfig);
}

static unsigned long nvt_sif_getConfig(SIF_CH Ch, SIF_CONFIG_ID ConfigID)
{
    return sif_getConfig(Ch, ConfigID);
}

static void nvt_sif_send(SIF_CH Ch, UINT32 uiData0, UINT32 uiData1, UINT32 uiData2)
{
    sif_send(Ch, uiData0, uiData1, uiData2);
}

static void nvt_sif_waitTransferEnd(SIF_CH Ch)
{
    sif_waitTransferEnd(Ch);
}

int nvt_sif_drv_open(PMODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(INFO, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/
	return 0;
}


int nvt_sif_drv_release(PMODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(INFO, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_sif_drv_init(MODULE_INFO* pmodule_info)
{
	int iRet = 0;
    nvt_dbg(INFO, "\n");
    //printk(KERN_INFO  "[Ben] \033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__);

	init_waitqueue_head(&pmodule_info->sif_wait_queue);
	//spin_lock_init(&pmodule_info->sif_spinlock);
	sema_init(&pmodule_info->sif_sem, 1);

	/* initial clock here */
	clk_prepare(pmodule_info->pclk[0]);
	clk_enable(pmodule_info->pclk[0]);

	/* register IRQ here*/
	if(request_irq(pmodule_info->iinterrupt_id[0], nvt_sif_drv_isr, IRQF_TRIGGER_HIGH, "SIF_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	/* Add HW Module initialization here when driver loaded */
	//_REGIOBASE = (UINT32)pmodule_info->io_addr[0];

    sif_init((UINT32)pmodule_info->io_addr[0]);

	return iRet;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return iRet;
}

int nvt_sif_drv_remove(MODULE_INFO* pmodule_info)
{
    nvt_dbg(INFO, "\n");

	sif_exit();

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	clk_disable(pmodule_info->pclk[0]);
	clk_unprepare(pmodule_info->pclk[0]);

	return 0;
}

int nvt_sif_drv_suspend(MODULE_INFO* pmodule_info)
{
	nvt_dbg(INFO, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_sif_drv_resume(MODULE_INFO* pmodule_info)
{
	nvt_dbg(INFO, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_sif_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
//	REG_INFO_LIST reg_info_list;
//	int iLoop;
	int iRet = 0;

    SIF_CH channel;
    CONFIG_INFO config_info;
    TX_INFO tx_info_w;

	nvt_dbg(INFO, "IF-%d cmd:%x\n", ucIF, uiCmd);

	switch(uiCmd) {
		case XXX_IOC_START:
			/*call someone to start operation*/
			break;

		case XXX_IOC_STOP:
			/*call someone to stop operation*/
			break;

		case XXX_IOC_READ_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet) {
		        reg_info.uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
			}
			break;

		case XXX_IOC_WRITE_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet)
				WRITE_REG(reg_info.uiValue, pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
			break;
/*
		case XXX_IOC_READ_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet) {
				for(iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++)
			        reg_info_list.RegList[iLoop].uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);

        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
			}
			break;
		case XXX_IOC_WRITE_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet)
				for(iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++)
					WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
			break;
*/
		/* Add other operations here */
		case SIF_IOC_OPEN:
            nvt_dbg(INFO, "SIF_IOC_OPEN\n");
			iRet = copy_from_user(&channel, (void __user *)ulArg, sizeof(SIF_CH));
	        if(!iRet) {
                nvt_dbg(INFO, "channel = %d\n", channel);
                nvt_sif_channel_open(channel);
			}
			break;

		case SIF_IOC_CLOSE:
            nvt_dbg(INFO, "SIF_IOC_CLOSE\n");
			iRet = copy_from_user(&channel, (void __user *)ulArg, sizeof(SIF_CH));
	        if(!iRet) {
                nvt_dbg(INFO, "channel = %d\n", channel);
                nvt_sif_channel_close(channel);
			}
            break;

		case SIF_IOC_ISOPENED:
            nvt_dbg(INFO, "SIF_IOC_ISOPENED\n");
			iRet = copy_from_user(&channel, (void __user *)ulArg, sizeof(SIF_CH));
	        if(!iRet) {
                nvt_dbg(INFO, "channel = %d is opened or not (%d)\n", channel, nvt_sif_channel_isopened(channel));
			}
            break;

		case SIF_IOC_SET_CONFIG:
            nvt_dbg(INFO, "SIF_IOC_SET_CONFIG\n");
			iRet = copy_from_user(&config_info, (void __user *)ulArg, sizeof(CONFIG_INFO));
	        if(!iRet) {
                nvt_dbg(INFO, "config_info.Ch = %d\n", config_info.Ch);
                nvt_dbg(INFO, "config_info.ConfigID = %d\n", config_info.ConfigID);
                nvt_dbg(INFO, "config_info.uiConfig = %d\n", config_info.uiConfig);
                spin_lock_irqsave(&pmodule_info->sif_spinlock, sif_lock_flags);
                nvt_sif_setConfig(config_info.Ch, config_info.ConfigID, config_info.uiConfig);
                spin_unlock_irqrestore(&pmodule_info->sif_spinlock, sif_lock_flags);
			}
            break;

		case SIF_IOC_GET_CONFIG:
            nvt_dbg(INFO, "SIF_IOC_GET_CONFIG\n");
			iRet = copy_from_user(&config_info, (void __user *)ulArg, sizeof(CONFIG_INFO));
	        if(!iRet) {
                spin_lock_irqsave(&pmodule_info->sif_spinlock, sif_lock_flags);
                config_info.uiConfig = nvt_sif_getConfig(config_info.Ch, config_info.ConfigID);
                spin_unlock_irqrestore(&pmodule_info->sif_spinlock, sif_lock_flags);
                nvt_dbg(INFO, "config_info.Ch = %d\n", config_info.Ch);
                nvt_dbg(INFO, "config_info.ConfigID = %d\n", config_info.ConfigID);
                nvt_dbg(INFO, "config_info.uiConfig = %d\n", config_info.uiConfig);
                iRet = copy_to_user((void __user *)ulArg, &config_info, sizeof(CONFIG_INFO));
			}
            break;
		case SIF_IOC_SEND:
            nvt_dbg(INFO, "SIF_IOC_SEND\n");
			iRet = copy_from_user(&tx_info_w, (void __user *)ulArg, sizeof(TX_INFO));
	        if(!iRet) {
                nvt_dbg(INFO, "tx_info_w.Ch = %d\n", tx_info_w.Ch);
                nvt_dbg(INFO, "tx_info_w.uiData0 = 0x%x\n", tx_info_w.uiData0);
                nvt_dbg(INFO, "tx_info_w.uiData1 = 0x%x\n", tx_info_w.uiData1);
                nvt_dbg(INFO, "tx_info_w.uiData2 = 0x%x\n", tx_info_w.uiData2);
			}
            spin_lock_irqsave(&pmodule_info->sif_spinlock, sif_lock_flags);
            nvt_sif_send(tx_info_w.Ch, tx_info_w.uiData0, tx_info_w.uiData1, tx_info_w.uiData2);
            spin_unlock_irqrestore(&pmodule_info->sif_spinlock, sif_lock_flags);
            break;

		case SIF_IOC_WAIT_END:
            nvt_dbg(INFO, "SIF_IOC_WAIT_END\n");
			iRet = copy_from_user(&channel, (void __user *)ulArg, sizeof(SIF_CH));
	        if(!iRet) {
                nvt_dbg(INFO, "channel = %d\n", channel);
                nvt_sif_waitTransferEnd(channel);
			}
            break;
	}

	return iRet;
}

irqreturn_t nvt_sif_drv_isr(int irq, void *devid)
{
    sif_isr();

    return IRQ_HANDLED;
}

int nvt_xxx_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_xxx_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}
