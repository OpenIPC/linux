#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direction.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/ctype.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/clk.h>
#include "pm_interface.h"



struct firmware_interface{
	struct device *dev;
	unsigned int *text;
	int fm_size;
	int space_size;
	struct pm_param *param;
};
struct firmware_interface *fw_data;

static struct firmware_interface fw_interface;

static int x2000_pm_valid(suspend_state_t state)
{
	unsigned long flags;
	int ret;
	printk("x2000_pm_valid\n");

	local_irq_save(flags);
	printk("enter pm_valid\n");
	ret = fw_data->param->valid((int)fw_data->param);
	printk("exit pm_valid\n");
	local_irq_restore(flags);
	return ret;
}

static int x2000_pm_begin(suspend_state_t state)
{
	unsigned long flags;
	printk("x2000_pm_begin\n");

	local_irq_save(flags);
	fw_data->param->begin((int)fw_data->param);
	local_irq_restore(flags);
	return 0;
}

static int x2000_pm_prepare(void)
{
	unsigned long flags;
	printk("x2000_pm_prepare\n");

	local_irq_save(flags);
	fw_data->param->prepare((int)fw_data->param);
	local_irq_restore(flags);
	return 0;
}

static int x2000_pm_enter(suspend_state_t state)
{
	unsigned long flags;
	printk("x2000_pm_enter\n");

	local_irq_save(flags);
	fw_data->param->state = state;
	fw_data->param->enter((int)fw_data->param);
	local_irq_restore(flags);
	return 0;
}

static void x2000_pm_finish(void)
{
	unsigned long flags;
	printk("x2000_pm_finish\n");

	local_irq_save(flags);
	fw_data->param->finish((int)fw_data->param);
	local_irq_restore(flags);
}

static void x2000_pm_end(void)
{
	unsigned long flags;
	printk("x2000_pm_end!\n");

	local_irq_save(flags);
	fw_data->param->end((int)fw_data->param);
	local_irq_restore(flags);
}


static const struct platform_suspend_ops x2000_pm_ops = {
	.valid		= x2000_pm_valid,
	.begin		= x2000_pm_begin,
	.prepare	= x2000_pm_prepare,
	.enter		= x2000_pm_enter,
	.finish		= x2000_pm_finish,
	.end		= x2000_pm_end,
};


static int pm_firmware_probe(struct platform_device *pdev)
{
	struct device_node *np;
	const struct firmware *firmware;
	char *fw_name;
	char mcu_uart_name[32];
	struct clk *mcu_uart_clk;
	struct pinctrl_state *state = NULL;
	struct pinctrl *p = NULL;
	int ret;

	PMTEXTENTRY entry;
	printk("pm_firmware_probe %d\n",pdev->num_resources);

	np = pdev->dev.of_node;
	if (!np)
		return -ENODEV;

	fw_data = &fw_interface;
	fw_data->dev = &pdev->dev;

	fw_data->text = (unsigned int *)FIRMWARE_ADDR;
	fw_data->space_size = FIRMWARE_SPACE;
	fw_name = FIRMWARE_NAME;


	ret = request_firmware(&firmware, fw_name, &pdev->dev);
	if (ret < 0) {
		pr_err("Error: %s request_firmware().\n",fw_name);
		goto err;
	}
	if(fw_data->space_size < firmware->size)
	{
		pr_err("Error: space[%d] too small.firmware size:%d\n",fw_data->space_size,firmware->size);
		goto err;
	}
	memcpy(fw_data->text, firmware->data, firmware->size);
//	dma_cache_sync(fw_data->dev, fw_data->text, firmware->size, DMA_TO_DEVICE);

	printk("firmware text: %p\n",fw_data->text);
	printk("firmware size: %d\n",firmware->size);
	fw_data->fm_size = firmware->size;

	entry = (PMTEXTENTRY)fw_data->text;
	release_firmware(firmware);
	printk("** run firmware **\n");
	fw_data->param = kmalloc(sizeof(struct pm_param), GFP_KERNEL);
	entry((void*)entry,&fw_data->param);

	printk("fw_data->param = %p\n",fw_data->param);


	fw_data->param->load_addr = (unsigned int)fw_data->text;
	fw_data->param->load_space = fw_data->space_size;
	fw_data->param->reload_pc = (unsigned int)kmalloc(fw_data->space_size, GFP_KERNEL);
	printk("reload_pc : 0x%x\n", fw_data->param->reload_pc);
	printk("load_addr : 0x%x\n", fw_data->param->load_addr);
	printk("load_space : 0x%x\n", fw_data->param->load_space);


	ret = of_property_read_u32(np, "console_idx", &(fw_data->param->uart_index));
	if (ret < 0) {
		printk("cannot get <console_idx>\n");
		goto err;
	}
	ret = of_property_read_u32(np, "mcu_uart_idx", &(fw_data->param->mcu_uart_idx));
	if (ret < 0) {
		printk("cannot get <mcu_uart_idx>\n");
		goto err;
	}
	ret = of_property_read_u32(np, "mcu_uart_baud", &(fw_data->param->mcu_uart_baud));
	if (ret < 0) {
		printk("cannot get <mcu_uart_baud>\n");
		goto err;
	}
	printk("uart_idx = %d\n", fw_data->param->uart_index);
	printk("mcu_uart_idx = %d\n", fw_data->param->mcu_uart_idx);
	printk("mcu_uart_baud = %d\n", fw_data->param->mcu_uart_baud);


	sprintf(mcu_uart_name, "gate_uart%d", fw_data->param->mcu_uart_idx);
	mcu_uart_clk = clk_get(NULL, mcu_uart_name);
	if (!mcu_uart_clk) {
		printk("cannot get %s ", mcu_uart_name);
		return -1;
	}
	clk_prepare_enable(mcu_uart_clk);


	p = pinctrl_get(fw_data->dev);
	if (IS_ERR_OR_NULL(p)) {
		printk("cannot get pinctrl\n");
	}
	sprintf(mcu_uart_name, "mcu_uart%d", fw_data->param->mcu_uart_idx);
	state = pinctrl_lookup_state(p, mcu_uart_name);
	if (!IS_ERR_OR_NULL(state)) {
		pinctrl_select_state(p, state);
	}


	printk("%s %d\n",__FILE__,__LINE__);

	return 0;
err:
	of_node_put(np);
	pr_err("pm_firmware_probe error!\n");
	return ret;
}

static int pm_firmware_remove(struct platform_device *pdev)
{
	kfree((unsigned int *)fw_data->param->reload_pc);
	return 0;
}

static const struct of_device_id ingenic_pm_match[] = {
	{.compatible = "ingenic,x2000-v12-pm",},
	{},
};

MODULE_DEVICE_TABLE(of, ingenic_pm_match);



static struct platform_driver x2000_pm_driver = {
	.driver = {
	      .name		= "x2000_v12_pm",
	      .owner		= THIS_MODULE,
	      .of_match_table	= ingenic_pm_match,
	},

	.probe		= pm_firmware_probe,
	.remove		= pm_firmware_remove,
};



static int x2000_pm_poweroff(void)
{
	unsigned long flags;
	printk("x2000_pm_poweroff\n");

	local_irq_save(flags);
	fw_data->param->poweroff((int)fw_data->param);
	local_irq_restore(flags);
	return 0;
}


static int x2000_poweroff_notifier_callback(struct notifier_block *nb, unsigned long action, void *data)
{
	if (action == SYS_POWER_OFF) {
		x2000_pm_poweroff();
	}

	return NOTIFY_DONE;
}

static struct notifier_block x2000_poweroff_notifier = {
	.notifier_call = x2000_poweroff_notifier_callback,
};

static int __init pm_init(void)
{
	platform_driver_register(&x2000_pm_driver);

	suspend_set_ops(&x2000_pm_ops);

	register_reboot_notifier(&x2000_poweroff_notifier);

	return 0;
}

static void __exit pm_exit(void)
{
	unregister_reboot_notifier(&x2000_poweroff_notifier);
	platform_driver_unregister(&x2000_pm_driver);
}

module_init(pm_init);
module_exit(pm_exit);


MODULE_LICENSE("GPL");
