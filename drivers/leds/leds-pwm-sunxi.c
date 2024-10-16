//#define DEBUG
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/syslog.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/leds.h>

#define CAMERA_NUM	1
#define MUTE_NUM	2
#define BRIGHTNESS_MAX	100

struct sunxi_pwmled {
	int led_num;
	struct device *dev;
	struct led_classdev *pcdev;
	struct pwm_device **pwm_dev;
	int mute_brightness;
	int camera_brightness;
	struct mutex pwm_muter;
};
struct sunxi_pwmled *sunxi_pwmled;

static void set_pwm(int num, int level)
{
	int brightness = BRIGHTNESS_MAX - level;
	unsigned duty_ns, period_ns ;
	struct sunxi_pwmled *led = sunxi_pwmled;

	mutex_lock(&led->pwm_muter);

	/*1000000000/50000 freq=50KHz, 1s=1000000000ns*/
	period_ns = 20000;
	duty_ns = (period_ns * brightness)/BRIGHTNESS_MAX ;

	if (num <= led->led_num) {
		pwm_config(led->pwm_dev[num - 1], duty_ns, period_ns);
		pwm_enable(led->pwm_dev[num - 1]);
	}
	pr_debug("[pwm_leds] pwm_config led%d:<%d | %d>\n",
			num, duty_ns, period_ns);

	mutex_unlock(&led->pwm_muter);
}

static ssize_t get_pin_ctrl(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

/*
 * usage: echo x y z > pin_ctrl.
 * x : led num.
 * y : led color(RGB), unuse now.
 * z : brightness.
 */
static ssize_t set_pin_ctrl(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret;
	char *priv = NULL, *next = NULL;
	long unsigned int num, color, brightness;
	struct sunxi_pwmled *led = sunxi_pwmled;


	/* parse x */
	priv = (char *)buf;
	next = strchr(priv, ' ');
	if (next == NULL) {
		pr_err("invalid_argument.\n");
		return -1;
	}
	*next = '\0';
	ret = kstrtoul(priv, 10, &num);
	if (ret)
		return ret;

	/* parse y */
	priv = next + 1;
	next = strchr(priv, ' ');
	if (next == NULL) {
		pr_err("invalid_argument.\n");
		return -1;
	}
	*next = '\0';
	ret = kstrtoul(priv, 10, &color);
	if (ret)
		return ret;

	/* parse z */
	priv = next + 1;
	ret = kstrtoul(priv, 10, &brightness);
	if (ret)
		return ret;

	pr_debug("set x y z: %ld, %ld, %ld.\n", num, color, brightness);

	if (num > led->led_num)
		return -1;

	/* save brightness. */
	if (num == MUTE_NUM) {
		led->mute_brightness = brightness;
		led->camera_brightness = 0;
	} else if (num == CAMERA_NUM) {
		led->camera_brightness = brightness;
		led->mute_brightness = 0;
    }

	if (led->camera_brightness) {
		/* show camera led. */
		set_pwm(MUTE_NUM, 0);
		set_pwm(CAMERA_NUM, led->camera_brightness);
	} else {
		/* camera has closed, show mute led. */
		set_pwm(CAMERA_NUM, 0);
		set_pwm(MUTE_NUM, led->mute_brightness);
	}

	return size;
}

static DEVICE_ATTR(pin_ctrl, 0664,
		get_pin_ctrl, set_pin_ctrl);

static const struct attribute *pwmleds_attributes[] = {
	&dev_attr_pin_ctrl.attr,
	NULL
};

static int set_led_brightness(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	/* unuse now.*/
	return 0;
}

static int pwm_deinit(struct sunxi_pwmled *led)
{
	int i;

	for (i = 0; i < led->led_num; i++)
		pwm_free(led->pwm_dev[i]);

	return 0;
}

static int pwm_init(struct sunxi_pwmled *led)
{
	int ret, i;
	int channel;
	char name[32] = {0};
	size_t size;
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	ret = of_property_read_u32(np, "led_num", &led->led_num);
	if (ret) {
		pr_err("failed to get the value of led_num.\n");
		return ret;
	}

	size = sizeof(struct pwm_device) * led->led_num;
	led->pwm_dev = kzalloc(size, GFP_KERNEL);
	if (!led->pwm_dev)
		return -ENOMEM;

	for (i = 0; i < led->led_num; i++) {
		sprintf(name, "pwmled_channel%d", i);
		ret = of_property_read_u32(np, name, &channel);
		if (ret) {
			pr_err("failed to get the value of pwm_channel%d.\n", i);
			return ret;
		}
		led->pwm_dev[i] = pwm_request(channel, "pwmled");
		if (led->pwm_dev[i] ==  NULL || IS_ERR(led->pwm_dev[i])) {
			pr_err("pwm_request channel%d fail!\n", i);
			return ret;
		}

	}
	pr_debug("pwm_leds: pwm init success!\n");

	return 0;
}

static int pwm_led_probe(struct platform_device *pdev)
{
	int ret, i;
	struct sunxi_pwmled *led;
	struct led_classdev *pcdev;

	led = kzalloc(sizeof(struct sunxi_pwmled), GFP_KERNEL);
	if (!led) {
		pr_err("kzalloc failed.\n");
		ret = -ENOMEM;
	}

	sunxi_pwmled = led;

	platform_set_drvdata(pdev, led);
	led->dev = &pdev->dev;

	ret = pwm_init(led);
	if (ret) {
		pr_err("pwm init err.\n");
		goto err1;
	}

	pcdev = kzalloc(sizeof(struct led_classdev), GFP_KERNEL);
	if (!pcdev)
		return -ENOMEM;
	led->pcdev = pcdev;

	pcdev->name = kzalloc(16, GFP_KERNEL);
	sprintf((char *)pcdev->name, "led");
	pcdev->brightness = LED_OFF;
	pcdev->brightness_set_blocking = set_led_brightness;

	ret = led_classdev_register(&pdev->dev, pcdev);
	if (ret < 0) {
		pr_err("led_classdev_register failed!\n");
		goto err2;
	}

	mutex_init(&led->pwm_muter);

	ret = sysfs_create_files(&pcdev->dev->kobj, pwmleds_attributes);
	if (ret) {
		pr_err("pwmled sysfs create err\n");
		goto err3;
	}

	/* close all led. */
	for (i = 1; i < led->led_num; i++)
		set_pwm(i, 0);

	pr_debug("pwm led init success.\n");
	return 0;

err3:
	led_classdev_unregister(pcdev);
err2:
	kfree(pcdev->name);
	kfree(pcdev);
	pwm_deinit(led);
err1:
	kfree(led);

	return -1;

}

static int pwm_led_remove(struct platform_device *pdev)
{
	struct sunxi_pwmled *led = sunxi_pwmled;
	struct led_classdev *pcdev = led->pcdev;

	led_classdev_unregister(pcdev);
	kfree(pcdev->name);
	kfree(pcdev);
	pwm_deinit(led);
	kfree(led);

	return 0;
}

static const struct of_device_id sunxi_led_dt_ids[] = {
	{.compatible = "allwinner,sunxi-pwmled"},
	{}
};

static struct platform_driver pwm_led_driver = {
	.probe          = pwm_led_probe,
	.remove         = pwm_led_remove,
	.driver         = {
		.name   = "sunxi-pwmled",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_led_dt_ids,
	},
};

module_platform_driver(pwm_led_driver);

MODULE_DESCRIPTION("pwmled driver.");
MODULE_AUTHOR("linzejia");
MODULE_LICENSE("GPL");
