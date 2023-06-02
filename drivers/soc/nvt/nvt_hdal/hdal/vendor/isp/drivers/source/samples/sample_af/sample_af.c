
#include "isp_dev.h"
#include "isp_mtr.h"

//=============================================================================
// function declaration
//=============================================================================
int __init sample_af_module_init(void);
void __exit sample_af_module_exit(void);

static void af_api_reserve(UINT32 addr);
static void af_api_get_test(UINT32 addr);
static void af_api_set_test(UINT32 addr);

//=============================================================================
// data structure
//=============================================================================
#define AF_ID_MAX      2
#define VA_WIN_X       8
#define VA_WIN_Y       8
#define AF_ITEM_MAX    4

typedef void (*sample_af_fp)(UINT32 addr);

typedef struct _AF_CMD_INFO {
	UINT32 cmd_index;
	UINT32 cmd_size;
} AF_CMD_INFO;

//=============================================================================
// global
//=============================================================================
ISP_MODULE sample_af_module;

wait_queue_head_t sample_af_event_queue[AF_ID_MAX];

struct task_struct *sample_af_thread;

CTL_MTR_DRV_TAB *pmtr_drv = NULL;

static UINT32 af_id= 0;

static AF_CMD_INFO sample_af_info[AF_ITEM_MAX] = {
	// id      size
	{ 0,     0  },
	{ 1,     4  },
	{ 2,     8  },
	{ 3,    16  },
};

static sample_af_fp af_get_tab[AF_ITEM_MAX] = {
	af_api_reserve,
	af_api_get_test,
	af_api_reserve,
	af_api_reserve,
};

static sample_af_fp af_set_tab[AF_ITEM_MAX] = {
	af_api_reserve,
	af_api_set_test,
	af_api_reserve,
	af_api_reserve,
};

//=============================================================================
// AF API function
//=============================================================================
static void af_api_reserve(UINT32 addr)
{
	printk

("af_api_reserve\n");
}

static void af_api_get_test(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;

	*data = 0x76543210;

	printk

("af_api_get_test\n");
}

static void af_api_set_test(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;
	UINT32 tmp[2] = { 0, 0 };

	tmp[0] = data[0];
	tmp[1] = data[1];

	printk

("af_api_set_test:%u,%u\n", tmp[0], tmp[1]);
}

INT32 sample_af_ioctl(INT32 cmd, UINT32 arg, UINT32 *buf_addr)
{
	INT32 ret = 0;
	UINT32 item = _IOC_NR(cmd);
	UINT32 size = sample_af_info[item].cmd_size;

	if (item >= AF_ITEM_MAX) {
		printk("ERR: item(%d) out of range\n", item);
		return E_SYS;
	}

	if ((_IOC_DIR(cmd) == _IOC_READ) || (_IOC_DIR(cmd) == (_IOC_READ | _IOC_WRITE))) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			printk("ERR: copy_from_user ERROR, (%u %u)\n", _IOC_DIR(cmd), item);
			return ret;
		}

		af_get_tab[item]((UINT32)buf_addr);

		ret = copy_to_user((void __user *)arg, buf_addr, size) ? -EFAULT : 0;
		if (ret < 0) {
			printk("ERR: copy_to_user ERROR, (%u %u)\n", _IOC_DIR(cmd), item);
			return ret;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			printk("ERR: copy_from_user ERROR, (%u %u)\n", _IOC_DIR(cmd), item);
			return ret;
		}

		af_set_tab[item]((UINT32)buf_addr);
	} else {
		printk("ERR: only support read/write!\n");

		return -ENOIOCTLCMD;
	}

	return ret;
}

//=============================================================================
// 
//=============================================================================
static UINT32 sample_set_focus_pos(UINT32 id, INT32 focus_pos)
{
	MTR_CTL_CMD mtr_cmd = { 0 };

	mtr_cmd.argu[0] = focus_pos;

	if ((pmtr_drv != NULL) && (pmtr_drv->set_focus != NULL)) {
		pmtr_drv->set_focus(id, MTR_SET_FOCUS_POSITION, &mtr_cmd);
	}

	return mtr_cmd.data[0];
}

static void sample_af_alg_process(UINT32 id)
{
	UINT32 i, j, idx, va_sum = 0;
	INT32 set_fcs_pos;
	ISP_VA_RSLT *va_rslt = NULL;

	va_rslt = isp_dev_get_va(id);

	if (va_rslt == NULL) {
		printk("isp_dev_get_va(id = %d) NULL!\n", id);
		return;
	} else {
		set_fcs_pos = 60;

		sample_set_focus_pos(id, set_fcs_pos);	  // set focus position to motor driver

		for (j = 0; j < VA_WIN_Y; j++) {
			for (i = 0; i < VA_WIN_X; i++) {
				idx = j * VA_WIN_X + i;
				va_sum += (va_rslt->g1_h[idx] + va_rslt->g1_v[idx] + va_rslt->g2_h[idx] + va_rslt->g2_v[idx]);
			}
		}
	}
}

static INT32 do_af_thread(void *private)
{
	UINT32 id = *((UINT32 *)private);

	do {
		if (wait_event_timeout(sample_af_event_queue[id], 1, msecs_to_jiffies(1000)) != 0) {
			sample_af_alg_process(id);
		}
	} while (!kthread_should_stop());

	return 0;
}

void sample_af_task_trig(unsigned int id, void *arg)
{
	af_id = (id < AF_ID_MAX) ? id : (AF_ID_MAX - 1);

	wake_up(&sample_af_event_queue[af_id]);
}

static INT32 sample_af_construct(void)
{
	UINT32 i;

	sprintf(sample_af_module.name, "SAMPLE_AF");
	sample_af_module.private   = NULL;
	sample_af_module.fn_init   = NULL;
	sample_af_module.fn_uninit = NULL;
	sample_af_module.fn_trig   = sample_af_task_trig;
	sample_af_module.fn_pause  = NULL;
	sample_af_module.fn_resume = NULL;
	sample_af_module.fn_ioctl  = sample_af_ioctl;

	// register to isp
	isp_dev_reg_af_module(&sample_af_module);

	for (i = 0; i < AF_ID_MAX; i++) {
		init_waitqueue_head(&sample_af_event_queue[i]);
	}

	sample_af_thread = kthread_create(do_af_thread, &af_id, "sample_af_alg");
	if (!IS_ERR(sample_af_thread)) {
		wake_up_process(sample_af_thread);
	}

	pmtr_drv = isp_dev_get_mtr_drv_tab();

	return 0;
}

static void sample_af_deconstruct(void)
{
	memset(&sample_af_module, 0x00, sizeof(ISP_MODULE));

	if (sample_af_thread) {
		kthread_stop(sample_af_thread);
	}
}

static int __init sample_af_init_module(void)
{
	printk("sample af\n");

	sample_af_construct();

	return 0;
}

static void __exit sample_af_exit_module(void)
{
	sample_af_deconstruct();
}

module_init(sample_af_init_module);
module_exit(sample_af_exit_module);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("sample af module");
MODULE_LICENSE("GPL");

