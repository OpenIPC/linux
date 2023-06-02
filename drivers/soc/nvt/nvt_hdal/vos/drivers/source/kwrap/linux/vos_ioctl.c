/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <linux/module.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <plat/hardware.h> //for NVT_TIMER_BASE_VIRT
#include <linux/platform_device.h>
#include <linux/cdev.h>

#define __MODULE__    vos_ioctl
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/cpu.h>
#include <kwrap/dev.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include <kwrap/util.h>
#include "vos_ioctl.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define VOS_IOCTL_MINOR_CNT      1

#define VOS_STRCPY(dst, src, dst_size) do { \
	strncpy(dst, src, (dst_size)-1); \
	dst[(dst_size)-1] = '\0'; \
} while(0)

typedef struct {
	dev_t                   devt;
	struct cdev             cdev;
	struct class            *p_class;
	struct device           *p_device;
} VOS_IOCTL_T;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int vos_ioctl_debug_level = NVT_DBG_WRN;
static VOS_IOCTL_T g_ioctl = {0};

module_param_named(vos_ioctl_debug_level, vos_ioctl_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vos_ioctl_debug_level, "Debug message level");

//=============================================================================
// platform device
//=============================================================================
static int _dev_register(void);
static void _dev_unregister(void);

static int _dev_platdrv_probe(struct platform_device *pdev);
static int _dev_platdrv_remove(struct platform_device *pdev);

static int _dev_fops_open (struct inode *inode, struct file *file);
static int _dev_fops_release (struct inode *inode, struct file *file);
static long _dev_fops_ioctl (struct file *file,  unsigned int cmd, unsigned long arg);

static void _dev_platdev_release(struct device *dev)
{
}

static struct platform_device _dev_platdev_struct = {
	.name   =   VOS_IOCTL_DEV_NAME,
	.id     =   0,
	.resource = NULL,
	.dev = {
		.release = _dev_platdev_release,
	},
};

static struct platform_driver _dev_platdrv_struct = {
	.probe  =   _dev_platdrv_probe,
	.remove =   _dev_platdrv_remove,
	.driver =   {
		.name   =   VOS_IOCTL_DEV_NAME,
		.owner  =   THIS_MODULE,
	},
};

// Kernel interface
static struct file_operations _dev_fops = {
	.owner          =   THIS_MODULE,
	.open           =   _dev_fops_open,
	.release        =   _dev_fops_release,
	.unlocked_ioctl =   _dev_fops_ioctl,
};
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void vos_ioctl_init(void *param)
{
	DBG_FUNC_BEGIN("\r\n");

	if (0 != _dev_register()) {
		DBG_ERR("_dev_register failed\r\n");
		goto END_INIT;
	}

END_INIT:
	DBG_FUNC_END("\r\n");
	return;
}

void vos_ioctl_exit(void)
{
	_dev_unregister();
}
/*-----------------------------------------------------------------------------*/
/* Kernel IOCTL                                                                */
/*-----------------------------------------------------------------------------*/
static int _dev_register(void)
{
	//register device
	if (0 != platform_device_register(&_dev_platdev_struct)) {
		DBG_ERR("platform_device_register failed\n");
		return -1;
	}

	if (0 != platform_driver_register(&_dev_platdrv_struct)) {
		DBG_ERR("platform_driver_register failed\n");
		return -1;
	}

	return 0;
}

static void _dev_unregister(void)
{
	platform_driver_unregister(&_dev_platdrv_struct);
	platform_device_unregister(&_dev_platdev_struct);
}

static int _dev_platdrv_probe(struct platform_device *pdev)
{
	int reg_err = 0, cdev_err = 0;

	reg_err = vos_alloc_chrdev_region(&g_ioctl.devt, VOS_IOCTL_MINOR_CNT, VOS_IOCTL_DEV_NAME);
	if (reg_err) {
		DBG_ERR(" alloc_chrdev_region failed!\n");
		goto _dev_platdrv_probe_err;
	}

	cdev_init(&g_ioctl.cdev, &_dev_fops);
	g_ioctl.cdev.owner = THIS_MODULE;

	cdev_err = cdev_add(&g_ioctl.cdev, g_ioctl.devt, VOS_IOCTL_MINOR_CNT);
	if (cdev_err) {
		DBG_ERR(" cdev_add failed!\n");
		goto _dev_platdrv_probe_err;
	}

	g_ioctl.p_class = class_create(THIS_MODULE, VOS_IOCTL_DEV_NAME);
	if (IS_ERR(g_ioctl.p_class)) {
		DBG_ERR(" class_create failed!\n");
		goto _dev_platdrv_probe_err;
	}

	g_ioctl.p_device = device_create(g_ioctl.p_class, NULL, g_ioctl.devt, NULL, VOS_IOCTL_DEV_NAME);
	if (NULL == g_ioctl.p_device) {
		DBG_ERR(" device_create failed!\n");
		goto _dev_platdrv_probe_err;
	}
	return 0;

_dev_platdrv_probe_err:

	if (g_ioctl.p_device) {
		device_destroy(g_ioctl.p_class, g_ioctl.devt);
	}
	if (g_ioctl.p_class) {
		class_destroy(g_ioctl.p_class);
	}
	if (!cdev_err) {
		cdev_del(&g_ioctl.cdev);
	}
	if (!reg_err) {
		vos_unregister_chrdev_region(g_ioctl.devt, VOS_IOCTL_MINOR_CNT);
	}
	return -1;
}

static int _dev_platdrv_remove(struct platform_device *pdev)
{
	device_destroy(g_ioctl.p_class, g_ioctl.devt);
	class_destroy(g_ioctl.p_class);

	cdev_del(&g_ioctl.cdev);
	vos_unregister_chrdev_region(g_ioctl.devt, VOS_IOCTL_MINOR_CNT);

	return 0;
}

static int _dev_fops_open(struct inode *inode, struct file *file)
{
	return 0;
}


static int _dev_fops_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long _dev_fops_ioctl(struct file *file,  unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	// CPU IOCTL
	case VOS_CPU_IOCMD_DCACHE_SYNC:
		ret = _IOFUNC_CPU_IOCMD_DCACHE_SYNC(arg);
		break;
	case VOS_CPU_IOCMD_GET_PHY_ADDR:
		ret = _IOFUNC_CPU_IOCMD_GET_PHY_ADDR(arg);
		break;
	case VOS_CPU_IOCMD_SYNC_CPU:
		ret = _IOFUNC_CPU_IOCMD_SYNC_CPU(arg);
		break;

	// DEBUG IOCTL
	case VOS_DEBUG_IOCMD_HALT:
		ret = _IOFUNC_DEBUG_IOCMD_HALT(arg);
		break;

	// FLAG IOCTL
	case VOS_FLAG_IOCMD_CREATE:
		ret = _IOFUNC_FLAG_IOCMD_CREATE(arg);
		break;
	case VOS_FLAG_IOCMD_SET:
		ret = _IOFUNC_FLAG_IOCMD_SET(arg);
		break;
	case VOS_FLAG_IOCMD_CLR:
		ret = _IOFUNC_FLAG_IOCMD_CLR(arg);
		break;
	case VOS_FLAG_IOCMD_WAIT:
		ret = _IOFUNC_FLAG_IOCMD_WAIT(arg);
		break;
	case VOS_FLAG_IOCMD_CHK:
		ret = _IOFUNC_FLAG_IOCMD_CHK(arg);
		break;
	case VOS_FLAG_IOCMD_DESTROY:
		ret = _IOFUNC_FLAG_IOCMD_DESTROY(arg);
		break;

	// SEM IOCTL
	case VOS_SEM_IOCMD_CREATE:
		ret = _IOFUNC_SEM_IOCMD_CREATE(arg);
		break;
	case VOS_SEM_IOCMD_WAIT:
		ret = _IOFUNC_SEM_IOCMD_WAIT(arg);
		break;
	case VOS_SEM_IOCMD_SIG:
		ret = _IOFUNC_SEM_IOCMD_SIG(arg);
		break;
	case VOS_SEM_IOCMD_DESTROY:
		ret = _IOFUNC_SEM_IOCMD_DESTROY(arg);
		break;

	// TASK IOCTL
	case VOS_TASK_IOCMD_SET_UINFO:
		ret = _IOFUNC_TASK_IOCMD_SET_UINFO(arg);
		break;
	case VOS_TASK_IOCMD_GET_UINFO:
		ret = _IOFUNC_TASK_IOCMD_GET_UINFO(arg);
		break;
	case VOS_TASK_IOCMD_REG_N_RUN:
		ret = _IOFUNC_TASK_IOCMD_REG_N_RUN(arg);
		break;
	case VOS_TASK_IOCMD_SET_PRIORITY:
		ret = _IOFUNC_TASK_IOCMD_SET_PRIORITY(arg);
		break;
	case VOS_TASK_IOCMD_DESTROY:
		ret = _IOFUNC_TASK_IOCMD_DESTROY(arg);
		break;
	case VOS_TASK_IOCMD_RETURN:
		ret = _IOFUNC_TASK_IOCMD_RETURN(arg);
		break;
	case VOS_TASK_IOCMD_RESUME:
		ret = _IOFUNC_TASK_IOCMD_RESUME(arg);
		break;
	case VOS_TASK_IOCMD_CONVERT_HDL:
		ret = _IOFUNC_TASK_IOCMD_CONVERT_HDL(arg);
		break;

	// PERF IOCTL
	case VOS_PERF_IOCMD_MARK:
		ret = _IOFUNC_PERF_IOCMD_MARK(arg);
		break;
	case VOS_PERF_IOCMD_LIST_MARK:
		ret = _IOFUNC_PERF_IOCMD_LIST_MARK(arg);
		break;
	case VOS_PERF_IOCMD_LIST_DUMP:
		ret = _IOFUNC_PERF_IOCMD_LIST_DUMP(arg);
		break;
	case VOS_PERF_IOCMD_LIST_RESET:
		ret = _IOFUNC_PERF_IOCMD_LIST_RESET(arg);
		break;

	default:
		DBG_WRN("Unknown IOCMD 0x%08X\n", (u32)cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}
