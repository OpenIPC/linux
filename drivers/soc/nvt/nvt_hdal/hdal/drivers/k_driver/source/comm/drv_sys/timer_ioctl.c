#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/slab.h>

#include "timer.h"
#include "timer_ioctl.h"
#include "timer_platform.h"
#include "timer_dbg.h"
#include "kwrap/type.h"
#include "kwrap/error_no.h"


typedef struct {
	TIMER_ID                    timer_idx;
	unsigned short			    interval;
	unsigned char				running;
	struct timer_list			curr_timer;
	struct list_head			timer_list;
} TIMER_CELL;


typedef struct {
	unsigned char				init;
	//unsigned int				total_timer;
	//struct list_head			timer_list;
	//atomic_t		            timer_bit;
	unsigned int				timer_active;  // 32 bit represent each timer is active or not
	unsigned int				timer_event;   // 32 bit represent each timer have event or not
	wait_queue_head_t			wait_q;
} KER_TIMER_CTRL;

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Function Protype                                                      */
/*-----------------------------------------------------------------------------*/
static KER_TIMER_CTRL* ker_timer_getctrl_p(void);

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static KER_TIMER_CTRL g_timer_ctrl;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

/**
   _ker_timer_getctrl_p
 * @remarks
 * @param none
 * @code

 * @endcode
 * @return
 * @see
*/
static KER_TIMER_CTRL *ker_timer_getctrl_p(void)
{
	return &g_timer_ctrl;
}

/*
static TIMER_CELL *exist_timer(TIMER_ID id)
{
	KER_TIMER_CTRL* p_ctrl = NULL;
	struct list_head *iterator = NULL;
	TIMER_CELL* pstCurrCell = NULL;

	p_ctrl = ker_timer_getctrl_p();
	list_for_each(iterator, &p_ctrl->timer_list) {
		if (list_entry(iterator, TIMER_CELL, timer_list)->timer_idx == id) {
			pstCurrCell = list_entry(iterator, TIMER_CELL, timer_list);
			break;
		}
	}
	return pstCurrCell;
}
*/


static void timer_cb(UINT32 timer_id)
{
	KER_TIMER_CTRL *p_ctrl = (KER_TIMER_CTRL *)ker_timer_getctrl_p();
    //TIMER_CELL* pstTimerCell = (TIMER_CELL*)NULL;
    unsigned long spin_flags = 0;

	#if 0
	pstTimerCell = exist_timer(timer_id);
	//DBG_ERR("timer_id 0x%x, pstTimerCell = 0x%x\r\n", timer_id, (int)pstTimerCell);

	if (pstTimerCell) {
		spin_flags = timer_platform_spin_lock();
		atomic_set(&p_ctrl->timer_bit, 1);
        pstTimerCell->running = true;
		wake_up_interruptible(&p_ctrl->wait_q);
		timer_platform_spin_unlock(spin_flags);
	}
	#endif

	spin_flags = timer_platform_spin_lock();
	p_ctrl->timer_event |= (1 << timer_id);
	timer_platform_spin_unlock(spin_flags);
	wake_up_interruptible(&p_ctrl->wait_q);
	//DBG_ERR("timer_event = 0x%x\r\n", p_ctrl->timer_event);

}

int nvt_timer_drv_ioctl_init(void)
{
	KER_TIMER_CTRL *p_ctrl = NULL;

	p_ctrl = ker_timer_getctrl_p();
	init_waitqueue_head(&p_ctrl->wait_q);
	//INIT_LIST_HEAD(&p_ctrl->timer_list);
	return 0;
}


int nvt_timer_rcv_event(MODULE_INFO *pmodule_info, UINT32 timer_id, UINT32 *event)
{
	unsigned long spin_flags = 0;
	KER_TIMER_CTRL *p_ctrl = (KER_TIMER_CTRL *)ker_timer_getctrl_p();

	//DBG_ERR("nvt_timer_rcv_event timer_id = 0x%x\r\n", timer_id);
	while (1) {
		int                   ret;

		// the event is empty , need to wait
		ret = wait_event_interruptible(p_ctrl->wait_q, p_ctrl->timer_event != 0);
		// wake up by signal
		if (ret == -ERESTARTSYS) {
			return ret;
		}
		//DBG_ERR("wait_event_interruptible\r\n");
		spin_flags = timer_platform_spin_lock();
		if (p_ctrl->timer_event) {
			//*event = p_ctrl->timer_event & timer_id;
			*event = p_ctrl->timer_event;
			p_ctrl->timer_event = 0;
			timer_platform_spin_unlock(spin_flags);
			break;
		} else {
			timer_platform_spin_unlock(spin_flags);
			continue;
		}
	}
	return E_OK;
}


int nvt_timer_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg)
{
	int __user *argp = (int __user*)arg;
    ER erReturn;
    TIMER_ID timer_id;
	UINT32 spin_flags;
    KER_TIMER_CTRL *p_ctrl = NULL;
	//TIMER_CELL *pstTimerCell = (TIMER_CELL *)NULL;
    p_ctrl = ker_timer_getctrl_p();

	switch (cmd) {
	    case IOCTL_TIMER_OPEN: {
	        erReturn = timer_open(&timer_id, timer_cb);
	        if (erReturn != E_OK) {
	            return erReturn;
	        }
	        if(copy_to_user(argp, &timer_id, sizeof(timer_id)))
	            return -EFAULT;


			#if 0
	        pstTimerCell = exist_timer(timer_id);
			if (!pstTimerCell) {
				pstTimerCell = (TIMER_CELL*)kzalloc(sizeof(TIMER_CELL), GFP_KERNEL);
			}
			if (pstTimerCell) {
				spin_flags = timer_platform_spin_lock();
				pstTimerCell->timer_idx = (ID)timer_id;
				pstTimerCell->running = false;
				list_add_tail(&pstTimerCell->timer_list, &p_ctrl->timer_list);
				timer_platform_spin_unlock(spin_flags);
			}
			#else
			spin_flags = timer_platform_spin_lock();
			p_ctrl->timer_active |= (1 << timer_id);
			timer_platform_spin_unlock(spin_flags);

			//DBG_IND("open timer_id 0x%x, p_ctrl->timer_active = 0x%x\r\n", timer_id, (int)p_ctrl->timer_active);

			#endif
	        break;

	    }
		case IOCTL_TIMER_GET_CURCOUNT: {
	        unsigned long ret;
	        if(copy_from_user(&ret, argp, sizeof(ret)))
	            return -EFAULT;
	        ret = timer_get_current_count((TIMER_ID) ret);
	        if(copy_to_user(argp, &ret, sizeof(ret)))
	            return -EFAULT;
			break;
		}
	    case IOCTL_TIMER_CLOSE: {
	        //TIMER_CELL *pstTimerPosition = NULL, *pstTimerTmp = NULL;

	        if(copy_from_user(&timer_id, argp, sizeof(timer_id)))
	            return -EFAULT;


			#if 0
			list_for_each_entry_safe(pstTimerPosition, pstTimerTmp, &p_ctrl->timer_list, timer_list) {
			    if (pstTimerPosition->timer_idx == timer_id) {
	                erReturn = timer_close(timer_id);
	                if (erReturn != E_OK) {
	                    return erReturn;
	                }
					spin_flags = timer_platform_spin_lock();
					list_del(&pstTimerPosition->timer_list);
					timer_platform_spin_unlock(spin_flags);
					kfree(pstTimerPosition);
					break;
				}
			}
			#endif
			spin_flags = timer_platform_spin_lock();
			p_ctrl->timer_active &= (~(1 << timer_id));
			timer_platform_spin_unlock(spin_flags);

			//DBG_ERR("close timer_id 0x%x, p_ctrl->timer_active = 0x%x\r\n", timer_id, (int)p_ctrl->timer_active);
	        timer_close(timer_id);
			break;
	    }
	    case IOCTL_TIMER_CONFIG: {
	        TIMER_IOC_CONFIG config;
	        if(copy_from_user(&config, argp, sizeof(config)))
	            return -EFAULT;
	        erReturn = timer_cfg(config.id, config.expires, config.mode, config.state);
	        if (erReturn != E_OK) {
	            return erReturn;
	        }
			break;

	    }
	    case IOCTL_TIMER_WAITTIMEUP: {
	        if(copy_from_user(&timer_id, argp, sizeof(timer_id)))
	            return -EFAULT;
	        erReturn = timer_wait_timeup(timer_id);
	        if (erReturn != E_OK) {
	            return erReturn;
	        }
	        break;

	    }
		case IOCTL_TIMER_RCV_EVENT: {
			TIMER_IOC_RCV_EVENT  ioc_event;
			UINT32               timer_id, event;

			if(copy_from_user(&ioc_event, argp, sizeof(ioc_event)))
	            return -EFAULT;
			timer_id = ioc_event.timer_id_all;
	        erReturn = nvt_timer_rcv_event(pmodule_info, timer_id, &event);
	        if (erReturn != E_OK) {
	            return erReturn;
	        }
			ioc_event.event = event;
			if(copy_to_user(argp, &ioc_event, sizeof(ioc_event)))
	            return -EFAULT;
	        break;

	    }
		default: {
			DBG_ERR("ioctl 0x%x not support \r\n", cmd);
			return E_NOSPT;
		}
	}

    return E_OK;
}


