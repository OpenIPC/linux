#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/syslog.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define PWM_NUM_RED	1
#define PWM_NUM_GREEN	2
#define PWM_NUM_BLUE	0

#define PERIOD_1S	1000000000
struct pwm_led_dev {
	struct pwm_device *red_dev;
	struct pwm_device *green_dev;
	struct pwm_device *blue_dev;

};

static struct pwm_led_dev *t_pwm_led_dev;

static inline void set_pwm(struct pwm_device *dev, int duty_ns, int period_ns);

int axp_leds_show(void)
{
	/* show pwm led red, freq:50%, period:1s */
	set_pwm(t_pwm_led_dev->red_dev, PERIOD_1S/2, PERIOD_1S);

	return 0;
}

int axp_leds_control(unsigned int idx, unsigned int duty_ns)
{
	struct pwm_device *pwm_dev = NULL;

	if (idx > 3)
		return -1;

	switch (idx) {
	case 0:
		pwm_dev = t_pwm_led_dev->red_dev;
		break;
	case 1:
		pwm_dev = t_pwm_led_dev->green_dev;
		break;
	case 2:
		pwm_dev = t_pwm_led_dev->blue_dev;
		break;
	default:
		return -1;
	}

	if (duty_ns > 0)
		set_pwm(pwm_dev, duty_ns, duty_ns*2);
	else
		pwm_disable(pwm_dev);

	return 0;
}
EXPORT_SYMBOL(axp_leds_control);

static inline void set_pwm(struct pwm_device *dev, int duty_ns, int period_ns)
{
	pwm_config(dev, duty_ns, period_ns);
	pwm_enable(dev);
}

static int pwm_leds_init(void)
{
	t_pwm_led_dev = kzalloc(sizeof(struct pwm_led_dev), GFP_KERNEL);

	if (!t_pwm_led_dev) {
		pr_err("[pwm_leds] request memory fail!\n");
		return -1;
	}

	t_pwm_led_dev->red_dev = pwm_request(PWM_NUM_RED, "led_red");
	if (t_pwm_led_dev->red_dev ==  NULL ||
			IS_ERR(t_pwm_led_dev->red_dev)) {
		pr_err("[pwm_leds] pwm_request red fail!\n");
		goto err1;
	}

	t_pwm_led_dev->green_dev = pwm_request(PWM_NUM_GREEN, "green_red");
	if (t_pwm_led_dev->green_dev ==  NULL ||
			IS_ERR(t_pwm_led_dev->green_dev)) {
		pr_err("[pwm_leds] pwm_request green fail!\n");
		goto err2;
	}

	t_pwm_led_dev->blue_dev = pwm_request(PWM_NUM_BLUE, "blue_red");
	if (t_pwm_led_dev->blue_dev ==  NULL ||
			IS_ERR(t_pwm_led_dev->blue_dev)) {
		pr_err("[pwm_leds] pwm_request blue fail!\n");
		goto err3;
	}

	/* close RGB, default polarity is inversed.*/
	set_pwm(t_pwm_led_dev->red_dev, PERIOD_1S, PERIOD_1S);
	set_pwm(t_pwm_led_dev->green_dev, PERIOD_1S, PERIOD_1S);
	set_pwm(t_pwm_led_dev->blue_dev, PERIOD_1S, PERIOD_1S);

	pr_debug("pwm leds init success!\n");

	return 0;

err3:
	pwm_free(t_pwm_led_dev->green_dev);
err2:
	pwm_free(t_pwm_led_dev->red_dev);
err1:
	kfree(t_pwm_led_dev);

	return -1;
}

static int pwm_leds_deinit(void)
{
	pwm_free(t_pwm_led_dev->blue_dev);
	pwm_free(t_pwm_led_dev->green_dev);
	pwm_free(t_pwm_led_dev->red_dev);
	kfree(t_pwm_led_dev);

	return 0;
}


static int axp_leds_init(void)
{
	int ret;

	printk("=====axp led init.====\n");
	ret = pwm_leds_init();
	if (ret < 0) {
		pr_err("pwm leds init err.\n");
		return -1;
	}

	return 0;
}

static void __exit axp_leds_exit(void)
{
	pwm_leds_deinit();
}

module_init(axp_leds_init);
module_exit(axp_leds_exit);

MODULE_DESCRIPTION("show leds, used for axp");
MODULE_AUTHOR("kirin");
MODULE_LICENSE("GPL");
