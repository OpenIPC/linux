#define DEBUG
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/usb/otg.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>

#include <linux/jz_dwc.h>
#include <soc/base.h>

#include "core.h"
#include "gadget.h"
#include "dwc2_jz.h"

#define OTG_CLK_NAME		"otg1"
#define VBUS_REG_NAME		"vbus"
#define CGU_USB_CLK_NAME	"cgu_usb"
#define USB_PWR_CLK_NAME	"pwc_usb"
#define DWC2_HOST_ID_TIMER_INTERVAL (HZ / 2)
#define DWC2_HOST_ID_MAX_DOG_COUNT  3
struct dwc2_jz {
	struct platform_device  dwc2;
	struct device		*dev;
	struct clk		*clk;
	struct clk		*cgu_clk;
	struct clk		*pwr_clk;
	struct mutex	irq_lock;		/* protect irq op */
	struct wake_lock	resume_wake_lock;
	struct work_struct  turn_off_work;

	/*device*/
#if DWC2_DEVICE_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	struct jzdwc_pin 	*dete_pin;	/* Host mode may used this pin to judge extern vbus mode
						 * Device mode may used this pin judge B-device insert */
	int 		dete_irq;
#endif
	int			pullup_on;
#endif
#if DWC2_HOST_MODE_ENABLE
	/*host*/
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	int			id_irq;
	struct jzdwc_pin 	*id_pin;
#endif
	struct jzdwc_pin 	*drvvbus_pin;		/*Use drvvbus pin or regulator to set vbus on*/
	struct regulator 	*vbus;
	atomic_t	vbus_on;
	struct work_struct	vbus_work;
	void *work_data;
	struct input_dev	*input;
#endif
};

#define to_dwc2_jz(dwc) container_of((dwc)->pdev, struct dwc2_jz, dwc2)

#if DWC2_HOST_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
int dwc2_jz_input_init(struct dwc2_jz *jz) {
#ifdef CONFIG_USB_DWC2_INPUT_EVDEV
	int ret = 0;
	struct input_dev *input = NULL;
	input = input_allocate_device();
	if (input == NULL)
		return -ENOMEM;

	input->name = "dwc2";
	input->dev.parent = jz->dev;

	__set_bit(EV_KEY, input->evbit);
	__set_bit(KEY_POWER2, input->keybit);

	if ((ret = input_register_device(input)) < 0)
		goto error;

	jz->input = input;
	return 0;
error:
	input_free_device(input);
	return ret;
#else
	jz->input = NULL;
	return 0;
#endif
}

static void dwc2_jz_input_cleanup(struct dwc2_jz *jz)
{
	if (jz->input)
		input_unregister_device(jz->input);
}

#ifdef CONFIG_USB_DWC2_INPUT_EVDEV
static void __dwc2_jz_input_report_key(struct dwc2_jz *jz,
					unsigned int code, int value)
{
	if (jz->input) {
		input_report_key(jz->input, code, value);
		input_sync(jz->input);
	}
}
#endif

static void __dwc2_input_report_power2_key(struct dwc2_jz *jz) {
#ifdef CONFIG_USB_DWC2_INPUT_EVDEV
	if (jz->input) {
		__dwc2_jz_input_report_key(jz, KEY_POWER2, 1);
		msleep(50);
		__dwc2_jz_input_report_key(jz, KEY_POWER2, 0);
	}
#endif
}
#endif
#endif

void dwc2_clk_enable(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);
	if (jz->pwr_clk)
		clk_enable(jz->pwr_clk);
	clk_enable(jz->cgu_clk);
	clk_enable(jz->clk);
}

void dwc2_clk_disable(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);

	clk_disable(jz->clk);
	clk_disable(jz->cgu_clk);
	if (jz->pwr_clk)
		clk_disable(jz->pwr_clk);
}
#if DWC2_DEVICE_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
/*usb insert detect*/
struct jzdwc_pin __attribute__((weak)) dwc2_dete_pin = {
	.num = -1,
	.enable_level = -1,
};
#endif
#endif

#if DWC2_HOST_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
/*usb host plug insert detect*/
struct jzdwc_pin __attribute__((weak)) dwc2_id_pin = {
	.num = -1,
	.enable_level = -1,
};
#endif

/*usb drvvbus pin*/
struct jzdwc_pin __attribute__((weak)) dwc2_drvvbus_pin = {
	.num = -1,
	.enable_level = -1,
};
#endif

#if DWC2_DEVICE_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
static int __dwc2_get_detect_pin_status(struct dwc2_jz *jz) {
	int insert = 0;
	if (gpio_is_valid(jz->dete_pin->num)) {
		insert = gpio_get_value(jz->dete_pin->num);
		if (jz->dete_pin->enable_level == LOW_ENABLE)
			return !insert;
	}
	return insert;
}
#endif
#endif

#if DWC2_HOST_MODE_ENABLE
static int __dwc2_get_id_level(struct dwc2_jz* jz) {
	int id_level = 1;
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	if (gpio_is_valid(jz->id_pin->num)) {
		id_level = gpio_get_value(jz->id_pin->num);
		if (jz->id_pin->enable_level == HIGH_ENABLE)
			id_level = !id_level;
	}
#endif
	return id_level;
}
int dwc2_get_id_level(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);
	return __dwc2_get_id_level(jz);
}
EXPORT_SYMBOL_GPL(dwc2_get_id_level);
#endif

void dwc2_gpio_irq_mutex_lock(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);
	mutex_lock(&jz->irq_lock);
}
EXPORT_SYMBOL_GPL(dwc2_gpio_irq_mutex_lock);

void dwc2_gpio_irq_mutex_unlock(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);
	mutex_unlock(&jz->irq_lock);
}
EXPORT_SYMBOL_GPL(dwc2_gpio_irq_mutex_unlock);

#if DWC2_DEVICE_MODE_ENABLE
extern void dwc2_gadget_plug_change(int plugin);

#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
static void usb_plug_change(struct dwc2_jz *jz) {

	int insert = __dwc2_get_detect_pin_status(jz);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	pr_info("DWC USB %s\n", insert ? "connect" : "disconnect");
	dwc2_disable_global_interrupts(dwc);
	synchronize_irq(dwc->irq);
	flush_work(&dwc->otg_id_work);
	dwc2_gadget_plug_change(insert);
	if (dwc->lx_state != DWC_OTG_L3)
		dwc2_enable_global_interrupts(dwc);
}

static irqreturn_t usb_detect_interrupt(int irq, void *dev_id)
{
	struct dwc2_jz	*jz = (struct dwc2_jz *)dev_id;

	mutex_lock(&jz->irq_lock);
	msleep(100);
	usb_plug_change(jz);
	mutex_unlock(&jz->irq_lock);
	return IRQ_HANDLED;
}
#endif
#endif /* !DWC2_DEVICE_MODE_ENABLE */

#if DWC2_HOST_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
static irqreturn_t usb_host_id_interrupt(int irq, void *dev_id) {
	struct dwc2_jz	*jz = (struct dwc2_jz *)dev_id;
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	int is_first = 1, dither_count = DWC2_HOST_ID_MAX_DOG_COUNT;
	unsigned long flags;

	mutex_lock(&jz->irq_lock);
	wake_lock(&jz->resume_wake_lock);
	msleep(50);
	while (1) {
		if (__dwc2_get_id_level(jz) == 0) { /* host */
			if (likely(!dwc->suspended)) {
				dwc2_spin_lock_irqsave(dwc, flags);
				dwc2_turn_on(dwc);
				dwc2_spin_unlock_irqrestore(dwc, flags);
				if (is_first) /*slcao's version just report power2_key in the first time,
								it's a bug or not, we just keep it and take care of it*/
					__dwc2_input_report_power2_key(jz);
			}
			break;
		}
		/*keep filter*/
		is_first = 0;
		if (!dither_count)
			break;
		dither_count--;
		mutex_unlock(&jz->irq_lock);
		schedule_timeout_uninterruptible(DWC2_HOST_ID_TIMER_INTERVAL + 1);
		mutex_lock(&jz->irq_lock);
	}
	mutex_unlock(&jz->irq_lock);
	wake_lock_timeout(&jz->resume_wake_lock, 3 * HZ);
	return IRQ_HANDLED;
}
#endif
static int __dwc2_get_drvvbus_level(struct dwc2_jz *jz)
{
	int drvvbus = 0;
	if (gpio_is_valid(jz->drvvbus_pin->num)) {
		drvvbus = gpio_get_value(jz->drvvbus_pin->num);
		if (jz->drvvbus_pin->enable_level == LOW_ENABLE)
			drvvbus = !drvvbus;
	}
	return drvvbus;
}

static void dwc2_vbus_work(struct work_struct *work)
{
	struct dwc2_jz* jz = container_of(work, struct dwc2_jz, vbus_work);
	struct dwc2 *dwc = (struct dwc2 *)jz->work_data;
	int old_is_on = atomic_read(&jz->vbus_on);
	int is_on = 0, ret = 0;

	if (IS_ERR_OR_NULL(jz->vbus) && !gpio_is_valid(jz->drvvbus_pin->num))
		return;

	is_on = old_is_on ? dwc2_is_host_mode(dwc) : 0;

	dev_info(jz->dev, "set vbus %s(%s) for %s mode\n",
			is_on ? "on" : "off",
			old_is_on ? "on" : "off",
			dwc2_is_host_mode(dwc) ? "host" : "device");

	if (!IS_ERR_OR_NULL(jz->vbus)) {
		if (is_on && !regulator_is_enabled(jz->vbus))
			ret = regulator_enable(jz->vbus);
		else if (!is_on && regulator_is_enabled(jz->vbus))
			ret = regulator_disable(jz->vbus);
		WARN(ret != 0, "dwc2 usb host ,regulator can not be used\n");
	} else {
		if (is_on && !__dwc2_get_drvvbus_level(jz))
			gpio_direction_output(jz->drvvbus_pin->num,
					jz->drvvbus_pin->enable_level == HIGH_ENABLE);
		else if (!is_on && __dwc2_get_drvvbus_level(jz))
			gpio_direction_output(jz->drvvbus_pin->num,
					jz->drvvbus_pin->enable_level == LOW_ENABLE);
	}
	return;
}

void jz_set_vbus(struct dwc2 *dwc, int is_on)
{
	struct dwc2_jz* jz = (struct dwc2_jz *)to_dwc2_jz(dwc);

	/*CHECK it lost some vbus set is ok ??*/
	atomic_set(&jz->vbus_on, !!is_on);
	jz->work_data = (void *)dwc;
	if (!work_pending(&jz->vbus_work)) {
		schedule_work(&jz->vbus_work);
		if (!in_atomic())
			flush_work(&jz->vbus_work);
	}
}
EXPORT_SYMBOL_GPL(jz_set_vbus);

static ssize_t jz_vbus_show(struct device *dev,
		struct device_attribute *attr,
		char *buf) {
	struct dwc2_jz *jz = dev_get_drvdata(dev);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	int vbus_is_on = 0;

	if (!IS_ERR_OR_NULL(jz->vbus)) {
		if (regulator_is_enabled(jz->vbus))
			vbus_is_on = 1;
		else
			vbus_is_on = 0;
	} else if (gpio_is_valid(jz->drvvbus_pin->num)) {
		vbus_is_on = __dwc2_get_drvvbus_level(jz);
	} else {
		vbus_is_on = dwc2_is_host_mode(dwc);
	}

	return sprintf(buf, "%s\n", vbus_is_on ? "on" : "off");
}

static ssize_t jz_vbus_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct dwc2_jz *jz = dev_get_drvdata(dev);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	int is_on = 0;

	if (strncmp(buf, "on", 2) == 0)
		is_on = 1;
	jz_set_vbus(dwc, is_on);

	return count;
}

static DEVICE_ATTR(vbus, S_IWUSR | S_IRUSR,
		jz_vbus_show, jz_vbus_set);

#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
static ssize_t jz_id_show(struct device *dev,
		struct device_attribute *attr,
		char *buf) {
	struct dwc2_jz *jz = dev_get_drvdata(dev);
	int id_level = __dwc2_get_id_level(jz);

	return sprintf(buf, "%s\n", id_level ? "1" : "0");
}

static DEVICE_ATTR(id, S_IRUSR |  S_IRGRP | S_IROTH,
		jz_id_show, NULL);
#endif
#endif	/* DWC2_HOST_MODE_ENABLE */

static void dwc2_power_off(struct dwc2 *dwc)
{
	if (dwc->lx_state != DWC_OTG_L3) {
		pr_info("dwc otg power off\n");
		dwc->phy_inited = 0;
		dwc2_disable_global_interrupts(dwc);
		jz_otg_phy_suspend(1);
		jz_otg_phy_powerdown();
		dwc2_clk_disable(dwc);
		dwc->lx_state = DWC_OTG_L3;
	}
	return;
}

static void dwc2_turn_off_work(struct work_struct *work)
{
	struct dwc2_jz *jz = container_of(work, struct dwc2_jz, turn_off_work);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	unsigned long flags;
	unsigned int id;

	mutex_lock(&jz->irq_lock);
	id = dwc2_get_id_level(dwc);
	dwc2_spin_lock_irqsave(dwc, flags);
	if (!dwc->plugin && !dwc2_has_ep_enabled(dwc) && id)
		dwc2_power_off(dwc);
	dwc2_spin_unlock_irqrestore(dwc, flags);
	mutex_unlock(&jz->irq_lock);
}
/*call with dwc2 spin lock*/
int dwc2_turn_off(struct dwc2 *dwc, bool graceful)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);

	if (graceful)
		schedule_work(&jz->turn_off_work);
	else
		dwc2_power_off(dwc);
	return 0;
}
EXPORT_SYMBOL_GPL(dwc2_turn_off);

static void dwc2_power_on(struct dwc2 *dwc)
{
	if (dwc->lx_state == DWC_OTG_L3) {
		pr_info("power on and reinit dwc2 otg\n");
		dwc2_clk_enable(dwc);
		jz_otg_ctr_reset();
		jz_otg_phy_init(dwc2_usb_mode());
		jz_otg_phy_suspend(0);
		dwc2_core_init(dwc);
#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)
		if (dwc2_is_device_mode(dwc))
			dwc2_device_mode_init(dwc);
#endif
		dwc->lx_state = DWC_OTG_L0;
		dwc2_enable_global_interrupts(dwc);
	}
}

/*call with dwc2 spin lock*/
int dwc2_turn_on(struct dwc2* dwc)
{
	dwc2_power_on(dwc);
	return 0;
}
EXPORT_SYMBOL_GPL(dwc2_turn_on);

static struct attribute *dwc2_jz_attributes[] = {
#if DWC2_HOST_MODE_ENABLE
	&dev_attr_vbus.attr,
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	&dev_attr_id.attr,
#endif
#endif	/*DWC2_HOST_MODE_ENABLE*/
	NULL
};

static const struct attribute_group dwc2_jz_attr_group = {
	.attrs = dwc2_jz_attributes,
};


static u64 dwc2_jz_dma_mask = DMA_BIT_MASK(32);

static int dwc2_jz_probe(struct platform_device *pdev) {
	struct platform_device		*dwc2;
	struct dwc2_jz		*jz;
	struct dwc2_platform_data	*dwc2_plat_data;
	int	ret = -ENOMEM;
	bool init_power_off = false;

	printk(KERN_DEBUG"dwc2 otg probe start\n");
	jz = devm_kzalloc(&pdev->dev, sizeof(*jz), GFP_KERNEL);
	if (!jz) {
		dev_err(&pdev->dev, "not enough memory\n");
		return -ENOMEM;
	}

	/*
	 * Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we move to full device tree support this will vanish off.
	 */
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &dwc2_jz_dma_mask;

	platform_set_drvdata(pdev, jz);
	dwc2_plat_data = devm_kzalloc(&pdev->dev,
			sizeof(struct dwc2_platform_data), GFP_KERNEL);
	if (!dwc2_plat_data)
		return -ENOMEM;
	dwc2 = &jz->dwc2;
	dwc2->name = "dwc2";
	dwc2->id = -1;
	device_initialize(&dwc2->dev);
	dma_set_coherent_mask(&dwc2->dev, pdev->dev.coherent_dma_mask);
	dwc2->dev.parent = &pdev->dev;
	dwc2->dev.dma_mask = pdev->dev.dma_mask;
	dwc2->dev.dma_parms = pdev->dev.dma_parms;
	dwc2->dev.platform_data = dwc2_plat_data;

	jz->dev	= &pdev->dev;
	mutex_init(&jz->irq_lock);
	INIT_WORK(&jz->turn_off_work, dwc2_turn_off_work);

	jz->clk = devm_clk_get(&pdev->dev, OTG_CLK_NAME);
	if (IS_ERR(jz->clk)) {
		dev_err(&pdev->dev, "clk gate get error\n");
		return PTR_ERR(jz->clk);
	}

	jz->cgu_clk = devm_clk_get(&pdev->dev, CGU_USB_CLK_NAME);
	if (IS_ERR(jz->cgu_clk)) {
		jz->cgu_clk = NULL;
	}

	jz->pwr_clk = devm_clk_get(&pdev->dev, USB_PWR_CLK_NAME);
	if (IS_ERR(jz->pwr_clk))
		jz->pwr_clk = NULL;

	if (jz->cgu_clk) {
		clk_set_rate(jz->cgu_clk, 24000000);
		clk_enable(jz->cgu_clk);
	}
	clk_enable(jz->clk);
	if (jz->pwr_clk)
		clk_enable(jz->pwr_clk);

	wake_lock_init(&jz->resume_wake_lock, WAKE_LOCK_SUSPEND, "usb insert wake lock");

#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)	/*DEVICE*/
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	jz->dete_irq = -1;
	jz->dete_pin = &dwc2_dete_pin;
	if (gpio_is_valid(jz->dete_pin->num)) {
		ret = devm_gpio_request_one(&pdev->dev, jz->dete_pin->num,
				GPIOF_DIR_IN, "usb-insert-detect");
		if (!ret) {
			jz->dete_irq = gpio_to_irq(jz->dete_pin->num);
		}
	}
	if (jz->dete_irq < 0)
		dwc2_plat_data->keep_phy_on = 1;
#else
	dwc2_plat_data->keep_phy_on = 1;
#endif
#endif	/*DEVICE*/

#if !IS_ENABLED(CONFIG_USB_DWC2_DEVICE_ONLY)	/*HOST*/
#ifndef CONFIG_USB_DWC2_DRVVBUS_FUNCTION_PIN
	jz->drvvbus_pin = &dwc2_drvvbus_pin;
	jz->vbus = regulator_get(NULL, VBUS_REG_NAME);
	if (IS_ERR_OR_NULL(jz->vbus)) {
		if (gpio_is_valid(jz->drvvbus_pin->num)) {
			ret = devm_gpio_request_one(&pdev->dev, jz->drvvbus_pin->num,
					GPIOF_DIR_OUT, "drvvbus_pin");
			if (ret < 0) jz->drvvbus_pin = NULL;
		}
		if (!jz->drvvbus_pin)
			dev_warn(&pdev->dev, "Not Found %s regulator \n", VBUS_REG_NAME);
		jz->vbus = NULL;
	}
	INIT_WORK(&jz->vbus_work, dwc2_vbus_work);
	atomic_set(&jz->vbus_on, 0);
#endif

#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	jz->id_pin = &dwc2_id_pin;
	jz->id_irq = -1;
	if(gpio_is_valid(jz->id_pin->num)) {
		ret = devm_gpio_request_one(&pdev->dev, jz->id_pin->num,
				GPIOF_DIR_IN, "otg-id-detect");
		if (!ret) {
			jz->id_irq = gpio_to_irq(jz->id_pin->num);
		}
	}
	if (jz->id_irq < 0)
		dwc2_plat_data->keep_phy_on = 1;
	dwc2_jz_input_init(jz);
#else
	dwc2_plat_data->keep_phy_on = 1;
#endif
#endif	/*HOST*/

	jz_otg_ctr_reset();
	jz_otg_phy_init(dwc2_usb_mode());

	ret = platform_device_add_resources(dwc2, pdev->resource,
					pdev->num_resources);
	if (ret) {
		dev_err(&pdev->dev, "couldn't add resources to dwc2 device\n");
		goto fail_register_dwc2_dev;
	}

	ret = platform_device_add(dwc2);
	if (ret) {
		dev_err(&pdev->dev, "failed to register dwc2 device\n");
		goto fail_register_dwc2_dev;
	}

#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)
	if (dwc2_plat_data->keep_phy_on) {
		dwc2_gadget_plug_change(1);
	}
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	else {
		ret = devm_request_threaded_irq(&pdev->dev,
				jz->dete_irq,
				NULL,
				usb_detect_interrupt,
				IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
				"usb-detect", jz);
		if (ret) {
			dev_err(&pdev->dev, "request usb-detect fail\n");
			goto fail_reuqest_irq;
		}
		if (__dwc2_get_detect_pin_status(jz)) {
			mutex_lock(&jz->irq_lock);
			dwc2_gadget_plug_change(1);
			mutex_unlock(&jz->irq_lock);
		} else
			init_power_off = true;
	}
#endif
#endif

#if !IS_ENABLED(CONFIG_USB_DWC2_DEVICE_ONLY)
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	if (!dwc2_plat_data->keep_phy_on) {
		unsigned long flags = 0;
		if (jz->id_pin->enable_level == HIGH_ENABLE)
			flags = IRQF_TRIGGER_RISING;
		else
			flags = IRQF_TRIGGER_FALLING;
		ret = devm_request_threaded_irq(&pdev->dev,
				jz->id_irq,
				NULL,
				usb_host_id_interrupt,
				flags | IRQF_ONESHOT,
				"usb-host-id",
				jz);
		if (ret) {
			dev_err(&pdev->dev, "request host id interrupt fail!\n");
			goto fail_reuqest_irq;
		}
		if (__dwc2_get_id_level(jz))
			init_power_off = false;
	}
#endif /*CONFIG_BOARD_HAS_NO_DETE_FACILITY*/
#endif
	if (init_power_off) {
		struct dwc2 *dwc = platform_get_drvdata(dwc2);
		dwc2_turn_off(dwc, true);
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &dwc2_jz_attr_group);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs group\n");
	}

	printk(KERN_DEBUG"dwc2 otg probe success\n");
	return 0;

#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
fail_reuqest_irq:
	platform_device_unregister(&jz->dwc2);
#endif
fail_register_dwc2_dev:
	platform_set_drvdata(pdev, NULL);
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	if (!dwc2_plat_data->keep_phy_on)
		wake_lock_destroy(&jz->resume_wake_lock);
#endif
#if DWC2_HOST_MODE_ENABLE
	if (!IS_ERR_OR_NULL(jz->vbus))
		regulator_put(jz->vbus);
#endif
	clk_disable(jz->clk);

	if (jz->cgu_clk)
		clk_disable(jz->cgu_clk);
	if (jz->pwr_clk)
		clk_disable(jz->pwr_clk);
	dev_err(&pdev->dev, "device probe failed\n");
	return ret;
}

static int dwc2_jz_remove(struct platform_device *pdev) {
	struct dwc2_jz	*jz = platform_get_drvdata(pdev);


	platform_set_drvdata(pdev, NULL);
#if DWC2_HOST_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	dwc2_jz_input_cleanup(jz);
#endif
	if (!IS_ERR_OR_NULL(jz->vbus))
		regulator_put(jz->vbus);
#endif
	clk_disable(jz->clk);

	if (jz->cgu_clk)
		clk_disable(jz->cgu_clk);
	if (jz->pwr_clk)
		clk_disable(jz->pwr_clk);
	platform_device_unregister(&jz->dwc2);

	return 0;
}

static int dwc2_jz_suspend(struct platform_device *pdev, pm_message_t state)
{
#ifndef  CONFIG_BOARD_HAS_NO_DETE_FACILITY
	struct dwc2_jz	*jz = platform_get_drvdata(pdev);
#if DWC2_DEVICE_MODE_ENABLE
	if (jz->dete_irq >= 0)
		enable_irq_wake(jz->dete_irq);
#endif

#if DWC2_HOST_MODE_ENABLE
	if (jz->id_irq >= 0)
		enable_irq_wake(jz->id_irq);
#endif
#endif /*CONFIG_BOARD_HAS_NO_DETE_FACILITY*/
	return 0;
}

static int dwc2_jz_resume(struct platform_device *pdev)
{
#ifndef  CONFIG_BOARD_HAS_NO_DETE_FACILITY
	struct dwc2_jz	*jz = platform_get_drvdata(pdev);

#if DWC2_DEVICE_MODE_ENABLE
	if (jz->dete_irq >= 0)
		disable_irq_wake(jz->dete_irq);
#endif

#if DWC2_HOST_MODE_ENABLE
	if (jz->id_irq >= 0)
		disable_irq_wake(jz->id_irq);
#endif
#endif /*CONFIG_BOARD_HAS_NO_DETE_FACILITY*/
	return 0;
}

static struct platform_driver dwc2_jz_driver = {
	.probe		= dwc2_jz_probe,
	.remove		= dwc2_jz_remove,
	.suspend	= dwc2_jz_suspend,
	.resume		= dwc2_jz_resume,
	.driver		= {
		.name	= "jz-dwc2",
		.owner =  THIS_MODULE,
	},
};

int dwc2_init(void);
void dwc2_exit(void);

static int __init dwc2_jz_init(void)
{
	dwc2_init();
	return platform_driver_register(&dwc2_jz_driver);
}

static void __exit dwc2_jz_exit(void)
{
	dwc2_exit();
	platform_driver_unregister(&dwc2_jz_driver);
}

/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
module_init(dwc2_jz_init);
module_exit(dwc2_jz_exit);
MODULE_ALIAS("platform:jz-dwc2");
MODULE_AUTHOR("Lutts Cao <slcao@ingenic.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DesignWare USB2.0 JZ Glue Layer");
