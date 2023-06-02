#include <linux/platform_device.h>
#include <linux/slab.h>

#include "isp_ioctl.h"
#include "isp_dbg.h"
#include "isp_main.h"
#include "isp_proc.h"
#include "isp_version.h"
#include "ispt_api_int.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
UINT32 isp_id_list = 0x03; // path 1 + path 2
module_param_named(isp_id_list, isp_id_list, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isp_id_list, "List of isp id");
#ifdef DEBUG
UINT32 isp_debug_level = THIS_DBGLVL;
module_param_named(isp_debug_level, isp_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isp_debug_level, "Debug message level");
#endif

//=============================================================================
// global variable
//=============================================================================
static ISP_DRV_INFO *p_isp_pdrv_info;

#if (ISP_NEW_REG_METHOD)
static ISP_VOS_DRV isp_vos_drv;
#endif

//=============================================================================
// function declaration
//=============================================================================
INT32 __init nvt_isp_module_init(void);
void __exit nvt_isp_module_exit(void);

//=============================================================================
// extern functions
//=============================================================================
ISP_DEV_INFO *isp_get_dev_info(void)
{
	if (p_isp_pdrv_info == NULL) {
		DBG_ERR("isp device info NULL! \n");
		return NULL;
	}

	return &p_isp_pdrv_info->dev_info;
}

//=============================================================================
// misc device file operation
//=============================================================================
static INT32 isp_drv_open(struct inode *inode, struct file *file)
{
	INT32 rt = 0;

	if (p_isp_pdrv_info == NULL) {
		file->private_data = NULL;
		DBG_ERR("ISP driver is not installed! \n");
		rt = -ENXIO;
	} else {
		file->private_data = p_isp_pdrv_info;
	}

	return 0;
}

static INT32 isp_drv_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static INT32 common_ioctl(INT32 cmd, UINT32 arg, UINT32 *buf_addr)
{
	INT32 ret = 0;
	UINT32 size;

	size = ispt_api_get_item_size(_IOC_NR(cmd));

	if ((_IOC_DIR(cmd) == _IOC_READ) || (_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = ispt_api_get_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("get command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = copy_to_user((void __user *)arg, buf_addr, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_to_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = ispt_api_set_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("set command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else {
		DBG_ERR("only support read/write \n");
	}

	return ret;
}

static INT32 do_ioc_common(ISP_DEV_INFO *pdev_info, UINT32 cmd, UINT32 arg)
{
	INT32 ret = 0;
	UINT32 *buf_addr = pdev_info->ioctl_buf;

	common_ioctl(cmd, arg, buf_addr);

	return ret;
}

static INT32 do_ioc_common_wait_vd(ISP_DEV_INFO *pdev_info, UINT32 cmd, UINT32 arg)
{
	INT32 ret = 0;
	ISPT_WAIT_VD wait_vd;

	common_ioctl(cmd, arg, (UINT32 *)&wait_vd);

	return ret;
}

static INT32 do_ioc_ae(ISP_DEV_INFO *pdev_info, UINT32 cmd, UINT32 arg)
{
	INT32 ret = 0;
	UINT32 *buf_addr = pdev_info->ioctl_buf;

	if ((pdev_info->isp_ae == NULL) || (pdev_info->isp_ae->fn_ioctl == NULL)) {
		return -ENXIO;
	}

	ret = pdev_info->isp_ae->fn_ioctl(cmd, arg, buf_addr);

	return ret;
}

static INT32 do_ioc_awb(ISP_DEV_INFO *pdev_info, UINT32 cmd, UINT32 arg)
{
	INT32 ret = 0;
	UINT32 *buf_addr = pdev_info->ioctl_buf;

	if ((pdev_info->isp_awb == NULL) || (pdev_info->isp_awb->fn_ioctl == NULL)) {
		return -ENXIO;
	}

	ret = pdev_info->isp_awb->fn_ioctl(cmd, arg, buf_addr);

	return ret;
}

static INT32 do_ioc_af(ISP_DEV_INFO *pdev_info, UINT32 cmd, UINT32 arg)
{
	INT32 ret = 0;
	UINT32 *buf_addr = pdev_info->ioctl_buf;

	if ((pdev_info->isp_af == NULL) || (pdev_info->isp_af->fn_ioctl == NULL)) {
		return -ENXIO;
	}

	ret = pdev_info->isp_af->fn_ioctl(cmd, arg, buf_addr);

	return ret;
}

static INT32 do_ioc_iq(ISP_DEV_INFO *pdev_info, UINT32 cmd, UINT32 arg)
{
	ER ret = 0;
	UINT32 *buf_addr = pdev_info->ioctl_buf;

	if ((pdev_info->isp_iq == NULL) || (pdev_info->isp_iq->fn_ioctl == NULL)) {
		return -ENXIO;
	}

	ret = pdev_info->isp_iq->fn_ioctl(cmd, arg, buf_addr);

	return ret;
}

static long isp_drv_ioctl(struct file *filp, UINT32 cmd, unsigned long arg)
{
	ISP_DRV_INFO *pdrv_info = (ISP_DRV_INFO *)filp->private_data;
	ISP_DEV_INFO *pdev_info = &pdrv_info->dev_info;
	INT32 ret = 0;

	if ((pdrv_info == NULL) || (pdev_info == NULL)) {
		return -ENXIO;
	}

	if ((_IOC_TYPE(cmd) == ISP_IOC_COMMON) && (_IOC_NR(cmd) == ISPT_ITEM_WAIT_VD)) {
		ret = do_ioc_common_wait_vd(pdev_info, cmd, arg);
	} else {
		down(&pdev_info->ioc_mutex);
		switch (_IOC_TYPE(cmd)) {
		case ISP_IOC_COMMON:
			ret = do_ioc_common(pdev_info, cmd, arg);
			break;

		case ISP_IOC_AE:
			ret = do_ioc_ae(pdev_info, cmd, arg);
			break;

		case ISP_IOC_AWB:
			ret = do_ioc_awb(pdev_info, cmd, arg);
			break;

		case ISP_IOC_AF:
			ret = do_ioc_af(pdev_info, cmd, arg);
			break;

		case ISP_IOC_IQ:
			ret = do_ioc_iq(pdev_info, cmd, arg);
			break;

		default:
			DBG_ERR(" IOC_TYPE = 0x%x is not support! \n", _IOC_TYPE(cmd));
			ret = -ENOIOCTLCMD;
			break;
		}
		up(&pdev_info->ioc_mutex);
	}
	return ret;
}

//=============================================================================
// platform driver
//=============================================================================
struct file_operations isp_drv_fops = {
	.owner   = THIS_MODULE,
	.open    = isp_drv_open,
	.release = isp_drv_release,
	.unlocked_ioctl = isp_drv_ioctl,
	.llseek  = no_llseek,
};

static void isp_remove_drv(ISP_DRV_INFO *pdrv_info)
{
	#if (ISP_NEW_REG_METHOD)
	unsigned char ucloop;
	ISP_VOS_DRV* pisp_vos_drv = &isp_vos_drv;

	for (ucloop = 0 ; ucloop < (ISP_MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pisp_vos_drv->pdevice[ucloop]);

	class_destroy(pisp_vos_drv->pmodule_class);
	cdev_del(&pisp_vos_drv->cdev);
	vos_unregister_chrdev_region(pisp_vos_drv->dev_id, ISP_MODULE_MINOR_COUNT);
	#endif

	if (pdrv_info) {
		#if (!ISP_NEW_REG_METHOD)
		if (pdrv_info->miscdev.fops)
			misc_deregister(&pdrv_info->miscdev);
		#endif
		isp_dev_deconstruct(&pdrv_info->dev_info);
		isp_proc_remove(pdrv_info);
		kfree(pdrv_info);
		pdrv_info = NULL;
		p_isp_pdrv_info = NULL;
	}
}

static INT32 isp_remove(struct platform_device *pdev)
{
	ISP_DRV_INFO *pdrv_info = platform_get_drvdata(pdev);

	isp_remove_drv(pdrv_info);

	return 0;
}

static INT32 isp_probe(struct platform_device *pdev)
{
	INT32 ret = 0;
	#if (ISP_NEW_REG_METHOD)
	unsigned char ucloop;
	ISP_VOS_DRV* pisp_vos_drv = &isp_vos_drv;
	#endif

	p_isp_pdrv_info = kzalloc(sizeof(ISP_DRV_INFO), GFP_KERNEL);
	if (p_isp_pdrv_info == NULL) {
		DBG_ERR("failed to alloc isp_drv_info_t! \n");
		ret = -ENOMEM;
		isp_remove_drv((ISP_DRV_INFO *)pdev);
		return ret;
	}

	platform_set_drvdata(pdev, p_isp_pdrv_info);
	ret = isp_dev_construct(&p_isp_pdrv_info->dev_info);
	if (ret < 0) {
		DBG_ERR("failed to construct isp_dev_info_t \n");
		isp_remove_drv((ISP_DRV_INFO *)pdev);
		return ret;
	}

	ret = isp_proc_init(p_isp_pdrv_info);
	if (ret < 0) {
		DBG_ERR("failed to initialize proc! \n");
		isp_remove_drv((ISP_DRV_INFO *)pdev);
		return ret;
	}

	#if (ISP_NEW_REG_METHOD)
	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pisp_vos_drv->dev_id, ISP_MODULE_MINOR_COUNT, ISP_DEV_NAME)) {
		pr_err("Can't get device ID\n");
		return -ENODEV;
	}

	/* Register character device for the volume */
	cdev_init(&pisp_vos_drv->cdev, &isp_drv_fops);
	pisp_vos_drv->cdev.owner = THIS_MODULE;

	if (cdev_add(&pisp_vos_drv->cdev, pisp_vos_drv->dev_id, ISP_MODULE_MINOR_COUNT)) {
		pr_err("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pisp_vos_drv->pmodule_class = class_create(THIS_MODULE, ISP_DEV_NAME);
	if(IS_ERR(pisp_vos_drv->pmodule_class)) {
		pr_err("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (ISP_MODULE_MINOR_COUNT ) ; ucloop++) {
		pisp_vos_drv->pdevice[ucloop] = device_create(pisp_vos_drv->pmodule_class, NULL
			, MKDEV(MAJOR(pisp_vos_drv->dev_id), (ucloop + MINOR(pisp_vos_drv->dev_id))), NULL
			, ISP_DEV_NAME);

		if(IS_ERR(pisp_vos_drv->pdevice[ucloop])) {

			pr_err("failed in creating device%d.\n", ucloop);

			if (ucloop == 0) {
				device_unregister(pisp_vos_drv->pdevice[ucloop]);
			}

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
	return ret;

	FAIL_CLASS:
	class_destroy(pisp_vos_drv->pmodule_class);

	FAIL_CDEV:
	cdev_del(&pisp_vos_drv->cdev);
	vos_unregister_chrdev_region(pisp_vos_drv->dev_id, ISP_MODULE_MINOR_COUNT);

	return ret;
	#else
	// register misc device
	p_isp_pdrv_info->miscdev.minor = MISC_DYNAMIC_MINOR;
	p_isp_pdrv_info->miscdev.name = ISP_DEV_NAME;
	p_isp_pdrv_info->miscdev.fops = &isp_drv_fops;
	ret = misc_register(&p_isp_pdrv_info->miscdev);
	if (ret < 0) {
		DBG_ERR("failed to register misc device! \n");
		return ret;
	}

	return ret;
	#endif
}

static struct platform_driver isp_driver = {
	.probe = isp_probe,
	.remove = isp_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = ISP_DEV_NAME,
	},
};

//=============================================================================
// platform device
//=============================================================================
static void isp_release(struct device *dev)
{
}

static struct platform_device isp_device = {
	.id = 0,
	.num_resources = 0,
	.resource = NULL,
	.name = ISP_DEV_NAME,
	.dev = {
		.release = isp_release,
	},
};

static INT32 __init isp_module_init(void)
{
	INT32 ret = 0;

	ret = platform_device_register(&isp_device);
	if (ret) {
		DBG_ERR("failed to register isp device \n");
		return ret;
	}

	ret = platform_driver_register(&isp_driver);
	if (ret) {
		DBG_ERR("failed to register isp driver \n");
		platform_device_unregister(&isp_device);
		return ret;
	}

	return ret;
}

static void __exit isp_module_exit(void)
{
	platform_driver_unregister(&isp_driver);
	platform_device_unregister(&isp_device);
}

module_init(isp_module_init);
module_exit(isp_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvt_isp driver");
MODULE_LICENSE("GPL");
