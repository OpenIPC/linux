#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>

#include "include/eink_driver.h"
#include "include/eink_sys_source.h"


struct disp_layer_config_inner eink_para[16];
struct disp_layer_config2 eink_lyr_cfg2[16];

struct eink_driver_info g_eink_drvdata;

u32 force_temp;

static ssize_t eink_debug_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "debug=%d\n", eink_get_print_level());
}

static ssize_t eink_debug_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	if (strncasecmp(buf, "1", 1) == 0)
		g_eink_drvdata.eink_dbg_info = 1;
	else if (strncasecmp(buf, "0", 1) == 0)
		g_eink_drvdata.eink_dbg_info = 0;
	else if (strncasecmp(buf, "2", 1) == 0)
		g_eink_drvdata.eink_dbg_info = 2;
	else if (strncasecmp(buf, "3", 1) == 0)
		g_eink_drvdata.eink_dbg_info = 3;
	else
		pr_err("Error input!\n");

	return count;
}

static ssize_t eink_dbglvl_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "[EINK]get debug level = %d\n", eink_get_print_level());
}

static ssize_t eink_dbglvl_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	pr_info("[EINK]set debug level = %s\n", buf);

	if (buf[0] >= '0' && buf[0] <= '9') {
		eink_set_print_level(buf[0] - '0');
	} else {
		pr_info("please set level 0~9 range\n");
	}

	return count;
}

static ssize_t eink_temperature_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "[EINK]get temperture = %d\n", force_temp);
}

static ssize_t eink_temperature_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned long value = 0;
	int err = 0;

	pr_info("[EINK]set temperature = %s\n", buf);

	err = kstrtoul(buf, 10, &value);
	if (err) {
		pr_warn("[%s]Invalid size\n", __func__);
		return err;
	}
	force_temp = value;

	return count;
}

static ssize_t eink_sys_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct eink_manager *eink_mgr = NULL;
	struct disp_manager *disp_mgr = NULL;
	ssize_t count = 0;
	u32 width = 0, height = 0;
	int num_layers, layer_id;
	int num_chans, chan_id;

	eink_mgr = get_eink_manager();
	if (eink_mgr == NULL)
		return 0;
	disp_mgr = disp_get_layer_manager(0);
	if (disp_mgr == NULL)
		return 0;

	eink_mgr->get_resolution(eink_mgr, &width, &height);

	if (eink_mgr->enable_flag == false) {
		pr_warn("eink not enable yet!\n");
		return 0;
	}

	count += sprintf(buf + count, "eink_rate %d hz, panel_rate %d hz, fps:%d\n",
			eink_mgr->get_clk_rate(eink_mgr->ee_clk),
			eink_mgr->get_clk_rate(eink_mgr->panel_clk), eink_mgr->get_fps(eink_mgr));
	count += eink_mgr->dump_config(eink_mgr, buf + count);
	/* output */
	count += sprintf(buf + count,
			"\teink output\n");

	num_chans = bsp_disp_feat_get_num_channels(0);

	/* layer info */
	for (chan_id = 0; chan_id < num_chans; chan_id++) {
		num_layers =
			bsp_disp_feat_get_num_layers_by_chn(0,
					chan_id);
		for (layer_id = 0; layer_id < num_layers; layer_id++) {
			struct disp_layer *lyr = NULL;
			struct disp_layer_config config;

			lyr = disp_get_layer(0, chan_id, layer_id);
			config.channel = chan_id;
			config.layer_id = layer_id;
			disp_mgr->get_layer_config(disp_mgr, &config, 1);
			if (lyr && (true == config.enable) && lyr->dump)
				count += lyr->dump(lyr, buf + count);
		}
	}
	return count;
}

static ssize_t eink_sys_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static ssize_t eink_standby_cmd_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	pr_info("[%s]:buf = %s, count = %d\n", __func__, buf, (unsigned int)count);
	if (!strncmp(buf, "suspend", (count - 1)))
		eink_suspend(g_eink_drvdata.device);
	if (!strncmp(buf, "resume", (count - 1)))
		eink_resume(g_eink_drvdata.device);

	return count;
}

static DEVICE_ATTR(debug, 0660, eink_debug_show, eink_debug_store);
static DEVICE_ATTR(dbglvl, 0660, eink_dbglvl_show, eink_dbglvl_store);
static DEVICE_ATTR(sys, 0660, eink_sys_show, eink_sys_store);
static DEVICE_ATTR(standby_cmd, 0660, NULL, eink_standby_cmd_store);
static DEVICE_ATTR(temperature, 0660, eink_temperature_show, eink_temperature_store);

static struct attribute *eink_attributes[] = {
	&dev_attr_debug.attr,
	&dev_attr_sys.attr,
	&dev_attr_dbglvl.attr,
	&dev_attr_standby_cmd.attr,
	&dev_attr_temperature.attr,
	NULL
};

static struct attribute_group eink_attribute_group = {
	.name = "attr",
	.attrs = eink_attributes
};

/* unload resources of eink device */
static void eink_unload_resource(struct eink_driver_info *drvdata)
{
	if (!IS_ERR_OR_NULL(drvdata->init_para.panel_clk))
		clk_put(drvdata->init_para.panel_clk);
	if (!IS_ERR_OR_NULL(drvdata->init_para.ee_clk))
		clk_put(drvdata->init_para.ee_clk);
	if (!IS_ERR_OR_NULL(drvdata->init_para.de_clk))
		clk_put(drvdata->init_para.de_clk);
	if (drvdata->init_para.ee_reg_base)
		iounmap(drvdata->init_para.ee_reg_base);
	if (drvdata->init_para.de_reg_base)
		iounmap(drvdata->init_para.de_reg_base);
	return;
}

static int eink_init(struct init_para *para)
{
	eink_get_sys_config(para);
	fmt_convert_mgr_init(para);
	eink_mgr_init(para);
	return 0;
}

static void eink_exit(void)
{
/*
	fmt_convert_mgr_exit(para);
	eink_mgr_exit(para);
*/
	return;
}

//static u64 eink_dmamask = DMA_BIT_MASK(32);
static int eink_probe(struct platform_device *pdev)
{
	int ret = 0, counter = 0;
	struct device_node *node = pdev->dev.of_node;
	struct eink_driver_info *drvdata = NULL;

	if (!of_device_is_available(node)) {
		pr_err("EINK device is not configed!\n");
		return -ENODEV;
	}

	drvdata = &g_eink_drvdata;

	eink_set_print_level(EINK_PRINT_LEVEL);
	/* reg_base */
	drvdata->init_para.ee_reg_base = of_iomap(node, counter);
	if (!drvdata->init_para.ee_reg_base) {
		pr_err("of_iomap eink reg base failed!\n");
		ret =  -ENOMEM;
		goto err_out;
	}
	counter++;
	EINK_INFO_MSG("ee reg_base=0x%p\n", drvdata->init_para.ee_reg_base);

	drvdata->init_para.de_reg_base = of_iomap(node, counter);
	if (!drvdata->init_para.de_reg_base) {
		pr_err("of_iomap de wb reg base failed\n");
		ret =  -ENOMEM;
		goto err_out;
	}
	EINK_INFO_MSG("de reg_base=0x%p\n", drvdata->init_para.de_reg_base);

	/* clk */
	counter = 0;
	drvdata->init_para.de_clk = of_clk_get(node, counter);
	if (IS_ERR_OR_NULL(drvdata->init_para.de_clk)) {
		pr_err("get disp engine clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.ee_clk);
		goto err_out;
	}
	counter++;
	drvdata->init_para.ee_clk = of_clk_get(node, counter);
	if (IS_ERR_OR_NULL(drvdata->init_para.ee_clk)) {
		pr_err("get eink engine clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.ee_clk);
		goto err_out;
	}
	counter++;
	drvdata->init_para.panel_clk = of_clk_get(node, counter);
	if (IS_ERR_OR_NULL(drvdata->init_para.panel_clk)) {
		pr_err("get edma clk clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.panel_clk);
		goto err_out;
	}
	/* irq */
	counter = 0;
	drvdata->init_para.ee_irq_no = irq_of_parse_and_map(node, counter);
	if (!drvdata->init_para.ee_irq_no) {
		pr_err("get eink irq failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	counter++;
	EINK_INFO_MSG("eink irq_no=%u\n", drvdata->init_para.ee_irq_no);

	drvdata->init_para.de_irq_no = irq_of_parse_and_map(node, counter);
	if (!drvdata->init_para.de_irq_no) {
		pr_err("get de irq failed!\n");
		ret = -EINVAL;
		goto err_out;
	}
	EINK_INFO_MSG("de irq_no=%u\n", drvdata->init_para.de_irq_no);

	drvdata->device = &pdev->dev;
/*
	pdev->dev.dma_mask = &eink_dmamask;
	pdev->dev.coherent_dma_mask = eink_dmamask;
*/
	platform_set_drvdata(pdev, (void *)drvdata);

#if defined(CONFIG_ION_SUNXI) && defined(EINK_CACHE_MEM)
	init_eink_ion_mgr(&g_eink_drvdata.ion_mgr);
#endif

	eink_init(&drvdata->init_para);

	g_eink_drvdata.eink_mgr = get_eink_manager();

	pm_runtime_set_active(drvdata->device);
	/* inc usercount so eink not runtime suspend */
	pm_runtime_get_noresume(drvdata->device);
	pm_runtime_set_autosuspend_delay(drvdata->device, 600);
	pm_runtime_use_autosuspend(drvdata->device);
	pm_runtime_enable(drvdata->device);
	device_enable_async_suspend(drvdata->device);

	pr_info("[EINK] probe finish!\n");

	return 0;

err_out:
	eink_unload_resource(drvdata);
	dev_err(&pdev->dev, "eink probe failed, errno %d!\n", ret);
	return ret;
}

static void eink_shutdown(struct platform_device *pdev)
{
	int ret = 0;
	bool empty = false;
	u32 not_empty_cnt = 0;
	struct buf_manager *buf_mgr = NULL;
	struct eink_manager *mgr = get_eink_manager();

	if (mgr == NULL) {
		pr_err("[%s]:eink mgr is NULL!\n", __func__);
		return;
	}
	pr_info("[%s]:\n", __func__);

	mutex_lock(&mgr->standby_lock);
	if (mgr->shutdown_state) {
		EINK_INFO_MSG("eink has already shutdown!\n");
		mutex_unlock(&mgr->standby_lock);
		return;
	}

	buf_mgr = mgr->buf_mgr;
	if (buf_mgr == NULL) {
		pr_err("[%s]:buf mgr is NULL\n", __func__);
		mutex_unlock(&mgr->standby_lock);
		return;
	}

	do {
		empty = buf_mgr->is_upd_list_empty(buf_mgr) && buf_mgr->is_coll_list_empty(buf_mgr);
		if (empty == false) {
			if (not_empty_cnt == 2000) {
				buf_mgr->wait_for_newest_img_node(buf_mgr);
			}
			not_empty_cnt++;
		}
		msleep(5);
	} while ((empty == false) && (not_empty_cnt <= 2000));

	if (not_empty_cnt > 2000) {
		pr_warn("[%s]:wait for buf empty timeout! fresh newest pic\n", __func__);
	}

	eink_clk_disable(mgr);

	ret = mgr->eink_mgr_disable(mgr);
	if (ret == 0) {
		mgr->shutdown_state = 1;
	} else {
		pr_warn("[%s]:fail to disable ee when shutdown\n", __func__);
	}

	mutex_unlock(&mgr->standby_lock);
	pr_info("[%s]:finish!\n", __func__);

	return;
}

static int eink_remove(struct platform_device *pdev)
{
	struct eink_driver_info *drvdata;

	dev_info(&pdev->dev, "%s\n", __func__);

	eink_shutdown(pdev);

	pm_runtime_set_suspended(g_eink_drvdata.device);
	pm_runtime_dont_use_autosuspend(g_eink_drvdata.device);
	pm_runtime_disable(g_eink_drvdata.device);

	drvdata = platform_get_drvdata(pdev);
	if (drvdata != NULL) {
		eink_unload_resource(drvdata);
		platform_set_drvdata(pdev, NULL);
	} else
		pr_err("%s:drvdata is NULL!\n", __func__);

	pr_info("%s finish!\n", __func__);

	return 0;
}

static int eink_runtime_suspend(struct device *dev)
{
	int ret = 0;
	bool empty = false;
	u32 not_empty_cnt = 0;
	struct buf_manager *buf_mgr = NULL;
	struct eink_manager *mgr = get_eink_manager();

	if (mgr == NULL) {
		pr_err("[%s]:eink mgr is NULL!\n", __func__);
		return -1;
	}
	pr_info("[%s]:-----------------\n", __func__);

	mutex_lock(&mgr->standby_lock);
	if (mgr->suspend_state) {
		EINK_INFO_MSG("eink has already suspend!\n");
		mutex_unlock(&mgr->standby_lock);
		return 0;
	}

	buf_mgr = mgr->buf_mgr;
	if (buf_mgr == NULL) {
		pr_err("[%s]:buf mgr is NULL\n", __func__);
		mutex_unlock(&mgr->standby_lock);
		return -1;
	}

	do {
		empty = buf_mgr->is_upd_list_empty(buf_mgr) && buf_mgr->is_coll_list_empty(buf_mgr);
		if (empty == false) {
			if (not_empty_cnt == 2000) {
				buf_mgr->wait_for_newest_img_node(buf_mgr);
			}
			not_empty_cnt++;
		}
		msleep(5);
	} while ((empty == false) && (not_empty_cnt <= 2000));

	if (not_empty_cnt > 2000) {
		pr_warn("[%s]:wait for buf empty timeout!fresh newest pic\n", __func__);
	}

#if 0
	if (mgr->detect_fresh_task) {
		kthread_stop(mgr->detect_fresh_task);
		pr_info("[%s]:Stop eink update thread!\n", __func__);
		mgr->detect_fresh_task = NULL;
	}
#endif

	if (mgr->get_temperature_task) {
		kthread_stop(mgr->get_temperature_task);
		EINK_INFO_MSG("stop get_temperature_task\n");
		mgr->get_temperature_task = NULL;
	}

	if (mgr->clk_en_flag)
		eink_clk_disable(mgr);

	ret = mgr->eink_mgr_disable(mgr);
	if (ret == 0) {
		mgr->suspend_state = 1;
	} else {
		pr_warn("[%s]:fail to disable ee when suspend\n", __func__);
	}

	mutex_unlock(&mgr->standby_lock);
	pr_info("[%s]:finish!\n", __func__);

	return ret;
}

static int eink_runtime_idle(struct device *dev)
{
	return 0;
}

static int eink_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct eink_manager *mgr = get_eink_manager();
	struct index_manager *index_mgr = NULL;

	if (mgr == NULL) {
		pr_err("[%s]:eink mgr is NULL\n", __func__);
		return -1;
	}

	index_mgr = mgr->index_mgr;
	if (index_mgr == NULL) {
		pr_err("[%s]:index mgr is NULL\n", __func__);
		return -1;
	}

	pr_info("[%s]:-----------------\n", __func__);

	mutex_lock(&mgr->standby_lock);
	if (mgr->suspend_state == 0) {
		mutex_unlock(&mgr->standby_lock);
		EINK_INFO_MSG("eink has already resume\n");
		return 0;
	}

	if (mgr->get_temperature_task == NULL) {
		mgr->get_temperature_task = kthread_create(eink_get_temperature_thread, (void *)mgr, "get temperature proc");
		if (IS_ERR_OR_NULL(mgr->get_temperature_task)) {
			pr_err("create getting temperature of eink thread fail!\n");
			ret = PTR_ERR(mgr->get_temperature_task);
			return ret;
		} else {
			ret = wake_up_process(mgr->get_temperature_task);
			EINK_INFO_MSG("resume get_temperature_task\n");
		}
	}

	ret = eink_clk_enable(mgr);
	index_mgr->set_rmi_addr(index_mgr);

	mgr->suspend_state = 0;

#if 0
	if (mgr->detect_fresh_task == NULL) {
		mgr->detect_fresh_task = kthread_create(detect_fresh_thread, (void *)mgr, "eink_fresh_proc");
		if (IS_ERR_OR_NULL(mgr->detect_fresh_task)) {
			pr_err("%s:create fresh thread failed!\n", __func__);
			ret = PTR_ERR(mgr->detect_fresh_task);
			return ret;
		}

		wake_up_process(mgr->detect_fresh_task);
	}
#endif
	mutex_unlock(&mgr->standby_lock);
	pr_info("[%s]:finish!\n", __func__);

	return ret;
}

int eink_suspend(struct device *dev)
{
	int ret = 0;

	if (!pm_runtime_status_suspended(dev)) {
		eink_runtime_suspend(dev);

		pm_runtime_disable(dev);
		pm_runtime_set_suspended(dev);
		pm_runtime_enable(dev);
	}

	pr_info("[%s]:finish!\n", __func__);
	return ret;
}

int eink_resume(struct device *dev)
{
	int ret = 0;

	if (!pm_runtime_active(dev)) {
		eink_runtime_resume(dev);

		pm_runtime_disable(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	}

	pr_info("[%s]:finish!\n", __func__);
	return ret;
}

int eink_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s open the device!\n", __func__);
	return ret;
}

int eink_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s finish!\n", __func__);
	return ret;
}

long eink_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	unsigned long karg[4];
	unsigned long ubuffer[4] = { 0 };
	struct eink_manager *eink_mgr = NULL;
	const unsigned int eink_lyr_cfg2_size = ARRAY_SIZE(eink_lyr_cfg2);

	eink_mgr = g_eink_drvdata.eink_mgr;

	if (copy_from_user
			((void *)karg, (void __user *)arg, 4 * sizeof(unsigned long))) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	ubuffer[0] = *(unsigned long *)karg;
	ubuffer[1] = (*(unsigned long *)(karg + 1));
	ubuffer[2] = (*(unsigned long *)(karg + 2));
	ubuffer[3] = (*(unsigned long *)(karg + 3));

	switch (cmd) {
	case EINK_UPDATE_IMG:
		{
			s32 i = 0;
			struct eink_upd_cfg eink_cfg;

			if (IS_ERR_OR_NULL((void __user *)ubuffer[2])) {
				__wrn("incoming pointer of user is ERR or NULL");
				return -EFAULT;
			}
			if (ubuffer[1] == 0 || ubuffer[1] > eink_lyr_cfg2_size) {
				__wrn("layer number need to be set from 1 to %d\n", eink_lyr_cfg2_size);
				return -EFAULT;
			}

			if (!eink_mgr) {
				pr_err("there is no eink manager!\n");
				break;
			}

			memset(eink_lyr_cfg2, 0,
					16 * sizeof(struct disp_layer_config2));
			if (copy_from_user(eink_lyr_cfg2, (void __user *)ubuffer[2],
						sizeof(struct disp_layer_config2) * ubuffer[1])) {
				pr_err("copy_from_user fail\n");
				return -EFAULT;
			}

			memset(&eink_cfg, 0, sizeof(struct eink_upd_cfg));
			if (copy_from_user(&eink_cfg, (void __user *)ubuffer[0],
						sizeof(struct eink_upd_cfg))) {
				pr_err("copy_from_user fail\n");
				return -EFAULT;
			}

			for (i = 0; i < ubuffer[1]; i++)
				__disp_config2_transfer2inner(&eink_para[i],
						&eink_lyr_cfg2[i]);

			printk("\n");
			printk("%s:eink_cfg fmt = %d, mode =%d\n", __func__, eink_cfg.out_fmt, eink_cfg.upd_mode);

			if (eink_mgr->eink_update)
				ret = eink_mgr->eink_update(eink_mgr,
						(struct disp_layer_config_inner *)&eink_para[0],
						(unsigned int)ubuffer[1],
						(struct eink_upd_cfg *)&eink_cfg);
			break;
		}

	case EINK_SET_TEMP:
		{
			if (eink_mgr->set_temperature)
				ret = eink_mgr->set_temperature(eink_mgr, ubuffer[0]);
			break;
		}

	case EINK_GET_TEMP:
		{
			if (eink_mgr->get_temperature)
				ret = eink_mgr->get_temperature(eink_mgr);
			break;
		}

	case EINK_SET_GC_CNT:
		{
			if (eink_mgr->eink_set_global_clean_cnt)
				ret = eink_mgr->eink_set_global_clean_cnt(eink_mgr, ubuffer[0]);
			break;
		}

	case EINK_SELF_STANDBY:
		{
			if (g_eink_drvdata.device == NULL) {
				pr_err("Eink device is NULL! return\n");
				ret = -EINVAL;
				break;
			}
			/* ubuffer[0] = 1 runtime_suspend, 0 runtime_resume */
			if (ubuffer[0]) {
				pm_runtime_put(g_eink_drvdata.device);
				/* not need system suspend */
				/* before set status must disable runtime can effect */
				pm_runtime_disable(g_eink_drvdata.device);
				pm_runtime_set_suspended(g_eink_drvdata.device);
				pm_runtime_enable(g_eink_drvdata.device);
			} else {
				pm_runtime_get_sync(g_eink_drvdata.device);
				/* not need system resume */
				/* before set status must disable runtime can effect */
				pm_runtime_disable(g_eink_drvdata.device);
				pm_runtime_set_active(g_eink_drvdata.device);
				pm_runtime_enable(g_eink_drvdata.device);
			}
			break;
		}

	default:
		pr_err("The cmd is err!\n");
		break;
	}
	return ret;

}

const struct file_operations eink_fops = {
	.owner = THIS_MODULE,
	.open = eink_open,
	.release = eink_release,
	.unlocked_ioctl = eink_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = eink_ioctl,
#endif
};

static const struct dev_pm_ops eink_pm_ops = {
	.runtime_suspend = eink_runtime_suspend,
	.runtime_resume = eink_runtime_resume,
	.runtime_idle = eink_runtime_idle,
	.suspend = eink_suspend,
	.resume = eink_resume,
};

static const struct of_device_id sunxi_eink_match[] = {
	{.compatible = "allwinner,sunxi-eink"},
	{},
};

static struct platform_driver eink_driver = {
	.probe = eink_probe,
	.remove = eink_remove,
	.shutdown = eink_shutdown,
	.driver = {
		.name = EINK_MODULE_NAME,
		.owner = THIS_MODULE,
		.pm = &eink_pm_ops,
		.of_match_table = sunxi_eink_match,
	},
};


static int __init eink_module_init(void)
{
	int ret = 0;
	struct eink_driver_info *drvdata = NULL;

	pr_info("[EINK]%s:\n", __func__);

	drvdata = &g_eink_drvdata;

	alloc_chrdev_region(&drvdata->devt, 0, 1, EINK_MODULE_NAME);
	drvdata->pcdev = cdev_alloc();
	cdev_init(drvdata->pcdev, &eink_fops);
	drvdata->pcdev->owner = THIS_MODULE;
	ret = cdev_add(drvdata->pcdev, drvdata->devt, 1);
	if (ret) {
		pr_err("eink cdev add major(%d) failed!\n", MAJOR(drvdata->devt));
		return -1;
	}
	drvdata->pclass = class_create(THIS_MODULE, EINK_MODULE_NAME);
	if (IS_ERR(drvdata->pclass)) {
		pr_err("eink create class error!\n");
		ret = PTR_ERR(drvdata->pclass);
		return ret;
	}

	drvdata->eink_dev = device_create(drvdata->pclass, NULL,
			drvdata->devt, NULL, EINK_MODULE_NAME);
	if (IS_ERR(drvdata->eink_dev)) {
		pr_err("eink device_create error!\n");
		ret = PTR_ERR(drvdata->eink_dev);
		return ret;
	}
	platform_driver_register(&eink_driver);

	ret = sysfs_create_group(&drvdata->eink_dev->kobj, &eink_attribute_group);
	if (ret < 0)
		pr_err("sysfs_create_file fail!\n");

	pr_info("[EINK]%s: finish\n", __func__);

	return 0;
}

static void __init eink_module_exit(void)
{
	struct eink_driver_info *drvdata = NULL;

	pr_info("[EINK]%s:\n", __func__);

	drvdata = &g_eink_drvdata;

	eink_exit();

#if defined(CONFIG_ION_SUNXI) && defined(EINK_CACHE_MEM)
	deinit_eink_ion_mgr(&g_eink_drvdata.ion_mgr);
#endif
	platform_driver_unregister(&eink_driver);
	device_destroy(drvdata->pclass, drvdata->devt);
	class_destroy(drvdata->pclass);
	cdev_del(drvdata->pcdev);
	unregister_chrdev_region(drvdata->devt, 1);
	return;
}

late_initcall(eink_module_init);
module_exit(eink_module_exit);

/* module_platform_driver(eink_driver); */

MODULE_DEVICE_TABLE(of, sunxi_eink_match);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liuli@allwinnertech.com");
MODULE_DESCRIPTION("Sunxi Eink");
