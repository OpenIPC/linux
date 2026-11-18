#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/soc/axera/ax_boardinfo.h>
#include "thermal_core.h"

#define AX_THM_MA_EN		0x4
#define AX_THM_MA_POR_EN	0x8
#define AX_THM_MA_CTRL		0xC
#define AX_THM_MA_POR_CTRL	0x14
#define AX_THM_CTRL		0x18
#define AX_THM_CLK_EN		0x1C	/*clk enable */
#define AX_THM_CLK_SELECT	0x20	/*clk select */
#define AX_THM_RSTN		0x24
#define AX_THM_MON_EN		0xC8
#define AX_THM_MON_CH		0xC4
#define AX_THM_MON_INTERVAL	0xCC
#define AX_THM_TEMP_HIGH	0xD0
#define AX_THM_TEMP_MEDIAN	0xD4
#define AX_THM_TEMP_LOW		0xD8
#define AX_THM_INT_MASK		0x104
#define AX_THM_INT_CLR		0x108
#define AX_THM_INT_STS		0x110

#define AX_MSR_ONE_EN		0x38
#define AX_MSR_VREF		0xB0

#define AX_THM_DATA0		0x78
#define AX_THM_DATA1		0x7C
#define AX_THM_DATA2		0x80
#define AX_THM_DATA3		0x84
#define AX_THM_DATA4		0x88
#define AX_THM_DATA5		0x8C
#define AX_THM_DATA6		0x90
#define AX_THM_DATA7		0x94
#define AX_THM_DATA8		0x98
#define AX_THM_DATA9		0x9C
#define AX_THM_SEL(x)		BIT(x)
#define AX620_DEFAULT_SENSOR	0

#define AX_ADC_CTRL		0x18
#define AX_ADC_CLK_EN		0x1C	/*clk enable */
#define AX_ADC_RSTN		0x24
#define AX_ADC_FILTER_VOL_SEL	0x2C
#define AX_ADC_FILTER_VOL_EN	0x30
#define AX_ADC_MON_EN		0xC8
#define AX_ADC_MON_CH		0xC4
#define AX_ADC_MON_INTERVAL	0xCC
#define AX_ADC_SEL		(0xf << 10 )
#define AX_ADC_EN		BIT(0)

#define AX_THM_TEMP_HIGH_INT	(1 << 18)
#define AX_THM_TEMP_MEDIAN_INT	(1 << 17)
#define AX_THM_TEMP_LOW_INT	(1 << 16)
#define COMMON_SYS_BASE		0x2340000
#define COMMON_SYS_CLK_EB1_SET	0x34
#define COMMON_SYS_CLK_EB1_CLR	0x38
#define COMMON_SYS_SW_RST0_CLR	0x5C
struct axera_thermal_sensor {
	struct thermal_zone_device *tzd;
	u32 id;
	u32 thres[3];
};

static void __iomem *common_sys_base;
static struct axera_thermal_data *axera_thermal_data_g;
static int thm_vref, vref_vol; //cal_temp read from efuse,thm_cal_b is the intercept of y=kx+b;
struct axera_thermal_data {
	int (*get_temp) (struct axera_thermal_data * data);
	int (*enable_sensor) (struct axera_thermal_data * data);
	int (*disable_sensor) (struct axera_thermal_data * data);
	struct platform_device *pdev;
	struct mutex		lock;
	struct axera_thermal_sensor sensor;
	void __iomem *regs;
	int irq;
	spinlock_t              spin_lock;
};

static int step2temp(int step)
{
	u32 temp;
	if (vref_vol == 0)
		vref_vol = 0x1f1;
	temp = (100000 * (90000 * step / (vref_vol * 242)) - 27315000 - 450000) / 100;
	return temp;
}

static int temp2step(int temp)
{
	u32 step;

	step = ((temp * 100) + 450 + 27315) / 100 * (vref_vol * 242) / 90000;
	return step;
}

static int vref_vol_temp[3];
static int caculate_vref_vol(struct axera_thermal_data *data)
{
	vref_vol_temp[0] = vref_vol_temp[1];
	vref_vol_temp[1] = vref_vol_temp[2];
	vref_vol_temp[2] = readl(data->regs + AX_MSR_VREF);
	vref_vol = (vref_vol_temp[0] + vref_vol_temp[1] + vref_vol_temp[2]) / 3;
	return vref_vol;
}

static int ax620_thermal_get_temp(struct axera_thermal_data *data)
{
	int val;
	int temp;
	unsigned long flags;
	spin_lock_irqsave(&data->spin_lock, flags);
	void __iomem *regs;
	regs = data->regs;
	val = (readl(regs + AX_THM_DATA0)) & 0x3ff;
	vref_vol = caculate_vref_vol(data);
	temp = step2temp(val);
	spin_unlock_irqrestore(&data->spin_lock, flags);
	return temp;
}


static int axera_thermal_get_temp(void *__data, int *temp)
{
	struct axera_thermal_data *data = __data;
	/*struct axera_thermal_sensor *sensor = &data->sensor; */
	*temp = data->get_temp(data);
	return 0;
}

static int ax_adc_config(struct axera_thermal_data *data)
{
	u32 val;
	void __iomem *regs;
	regs = data->regs;

	val = readl(regs + AX_ADC_CLK_EN);
	val |= AX_ADC_EN;
	writel(val, regs + AX_ADC_CLK_EN);

	writel(0x0, regs + AX_ADC_FILTER_VOL_SEL);/*set filter num*/
	writel(0xf, regs + AX_ADC_FILTER_VOL_EN);/*set filter enable*/
	writel(0x100, regs + AX_ADC_MON_INTERVAL);

	val = readl(regs + AX_ADC_MON_CH);
	val |= AX_ADC_SEL;
	writel(val ,regs + AX_ADC_MON_CH);

	return 0;
}

static int ax_thmeral_config(struct axera_thermal_data *data)
{
	void __iomem *regs;
	regs = data->regs;
	u32 val = 0;

	writel(0x0, regs + AX_THM_INT_MASK);
	writel(0x1, regs + AX_THM_MA_CTRL);
	writel(0x1, regs);

	val = thm_vref | 0x18;
	writel(val, regs + AX_THM_CTRL);

	val = readl(regs + AX_THM_CLK_EN);
	val |= 0x2;
	writel(val, regs + AX_THM_CLK_EN);

	val = readl(regs + AX_THM_MON_CH);
	val |= AX_THM_SEL(0);
	writel(val, regs + AX_THM_MON_CH);

	writel(0x70000, regs + AX_THM_INT_MASK);
	return 0;
}

static int ax620_thermal_enable_sensor(struct axera_thermal_data *data)
{
	void __iomem *regs;
	void __iomem *base;
	u32 val;

	misc_info_t *misc_info;
	struct axera_thermal_sensor *sensor = &data->sensor;
	regs = data->regs;

	base = ioremap(MISC_INFO_ADDR, sizeof(misc_info_t));
	if(!base) {
		printk("MISC_INFO_ADDR IOREMAP ERR\n");
		return -1;
	}

	misc_info = (misc_info_t *) base;
	thm_vref = (misc_info->thm_vref) << 5;
	if (!thm_vref)
		thm_vref = (0x7 << 5);
	iounmap(base);

	mutex_lock(&data->lock);
	writel(0x0, regs + AX_THM_RSTN);
	writel(0x0, regs + AX_THM_MON_EN);
	ax_adc_config(data);
	ax_thmeral_config(data);
	writel(0x1, regs + AX_THM_RSTN);
	writel(BIT(14), regs + AX_MSR_ONE_EN);
	udelay(100);
	vref_vol = readl(regs + AX_MSR_VREF);
	val = temp2step(sensor->thres[0]);
	writel(val, regs + AX_THM_TEMP_LOW);
	val = temp2step(sensor->thres[1]);
	writel(val, regs + AX_THM_TEMP_MEDIAN);
	val = temp2step(sensor->thres[2]);
	writel(val, regs + AX_THM_TEMP_HIGH);
	val = readl(regs + AX_THM_MON_CH);
	val |= BIT(14);
	writel(val, regs + AX_THM_MON_CH);
	writel(0x1, regs + AX_THM_MON_EN);
	mutex_unlock(&data->lock);

	vref_vol_temp[0] = vref_vol;
	vref_vol_temp[1] = vref_vol;
	vref_vol_temp[2] = vref_vol;
	return 0;
}

static int ax620_thermal_disable_sensor(struct axera_thermal_data *data)
{
	void __iomem *regs;
	regs = data->regs;
	mutex_lock(&data->lock);
	writel(0x0, regs + AX_THM_MON_EN);
	/* close AIN0-3, open temp 0 */
	writel(0x1, regs + AX_ADC_MON_CH);
	writel(0x1, regs + AX_THM_MON_EN);
	mutex_unlock(&data->lock);
	return 0;
}

static int ax620_thermal_probe(struct axera_thermal_data *data)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	struct resource *res;
	data->get_temp = ax620_thermal_get_temp;
	data->enable_sensor = ax620_thermal_enable_sensor;
	data->disable_sensor = ax620_thermal_disable_sensor;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	data->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(data->regs)) {
		dev_err(dev, "failed to get io address\n");
		return PTR_ERR(data->regs);
	}
	data->irq = platform_get_irq(pdev, 0);
	if (data->irq < 0) {
		dev_err(dev, "ax620e_thermal_probe error\n");
		return data->irq;
	}
	axera_thermal_data_g = data;
	mutex_init(&data->lock);
	data->sensor.id = AX620_DEFAULT_SENSOR;
	return 0;
}

static const struct thermal_zone_of_device_ops axera_of_thermal_ops = {
	.get_temp = axera_thermal_get_temp,
};

static int axera_thermal_register_sensor(struct platform_device *pdev,
					 struct axera_thermal_data *data,
					 struct axera_thermal_sensor *sensor)
{
	int ret, i;
	const struct thermal_trip *trip;
	sensor->tzd = devm_thermal_zone_of_sensor_register(&pdev->dev,
							   sensor->id, data,
							   &axera_of_thermal_ops);
	if (IS_ERR(sensor->tzd)) {
		ret = PTR_ERR(sensor->tzd);
		sensor->tzd = NULL;
		dev_err(&pdev->dev, "failed to register sensor id %d: %d\n",
			sensor->id, ret);
		return ret;
	}
	trip = of_thermal_get_trip_points(sensor->tzd);

	for (i = 0; i < of_thermal_get_ntrips(sensor->tzd); i++) {
		sensor->thres[i] = trip[i].temperature;
	}
	return 0;
}

static void axera_thermal_toggle_sensor(struct axera_thermal_sensor *sensor,
					bool on)
{
	struct thermal_zone_device *tzd = sensor->tzd;
	tzd->ops->set_mode(tzd, on ? THERMAL_DEVICE_ENABLED : THERMAL_DEVICE_DISABLED);
}

static int ax_thm_irq_unmask(struct axera_thermal_data *data, unsigned int int_status)
{
	void __iomem *regs = data->regs;
	unsigned int val;

	val = readl(regs + AX_THM_INT_MASK);
	val |= int_status;
	writel(val, regs + AX_THM_INT_MASK);

	return 0;
}

static int ax_thm_irq_mask(struct axera_thermal_data *data, unsigned int int_status)
{
	void __iomem *regs = data->regs;
	unsigned int val;

	val = readl(regs + AX_THM_INT_MASK);
	val &= ~int_status;
	writel(val, regs + AX_THM_INT_MASK);

	return 0;
}

static int ax_thm_irq_clr(struct axera_thermal_data *data, unsigned int int_status)
{
	void __iomem *regs = data->regs;
	unsigned int val;

	val = readl(regs + AX_THM_INT_CLR);
	val |= int_status;
	writel(val, regs + AX_THM_INT_CLR);

	return 0;
}

static irqreturn_t axera_thermal_alarm_irq(int irq, void *dev)
{
	struct axera_thermal_data *data = dev;
	int temp;
	unsigned int int_status;
	void __iomem *regs = data->regs;

	int_status = readl(regs + AX_THM_INT_STS);
	int_status &= (0x7 << 16);
	if (!int_status)
		return IRQ_HANDLED;
	ax_thm_irq_mask(data, int_status);

	switch (int_status) {
	case AX_THM_TEMP_LOW_INT:
		temp = data->get_temp(data);
		pr_debug("%s enter,temp:%d\n", __func__, temp);
		break;
	case AX_THM_TEMP_MEDIAN_INT:
		temp = data->get_temp(data);
		pr_debug("%s enter,temp:%d\n", __func__, temp);
		break;
	case AX_THM_TEMP_HIGH_INT:
		temp = data->get_temp(data);
		pr_debug("%s enter,temp:%d\n", __func__, temp);
		break;
	}

	ax_thm_irq_unmask(data, int_status);
	ax_thm_irq_clr(data, int_status);
	return IRQ_HANDLED;
}

static int axera_thermal_probe(struct platform_device *pdev)
{
	struct axera_thermal_data *data;
	int (*platform_probe) (struct axera_thermal_data *);
	struct device *dev = &pdev->dev;
	int ret;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	data->pdev = pdev;
	platform_set_drvdata(pdev, data);
	dev_set_drvdata(dev, data);
	spin_lock_init(&data->spin_lock);

	platform_probe = of_device_get_match_data(dev);
	if (!platform_probe) {
		dev_err(dev, "failed to get probe func\n");
		return -EINVAL;
	}

	common_sys_base = ioremap(COMMON_SYS_BASE, 0x100);
	if (IS_ERR(common_sys_base)) {
		dev_err(dev, "failed to ioremap common_sys address\n");
		return PTR_ERR(common_sys_base);
	}
	writel(((1 << 10) | (1 << 14)), common_sys_base + COMMON_SYS_CLK_EB1_SET);
	writel(GENMASK(20, 19), common_sys_base + COMMON_SYS_SW_RST0_CLR);

	ret = platform_probe(data);
	if (ret)
		return ret;
	ret = axera_thermal_register_sensor(pdev, data, &data->sensor);
	if (ret) {
		dev_err(dev, "failed to register thermal sensor: %d\n", ret);
		return ret;
	}
	ret = data->enable_sensor(data);
	if (ret) {
		dev_err(dev, "Failed to setup the sensor: %d\n", ret);
		return ret;
	}

	if (data->irq) {
		ret = devm_request_irq(dev, data->irq, axera_thermal_alarm_irq,
				IRQF_SHARED, "axera_thermal", data);
		if (ret < 0) {
			dev_err(dev, "failed to request alarm irq: %d\n", ret);
			return ret;
		}
	}
	axera_thermal_toggle_sensor(&data->sensor, true);

	return 0;
}

static int axera_thermal_remove(struct platform_device *pdev)
{
	struct axera_thermal_data *data = platform_get_drvdata(pdev);
	struct axera_thermal_sensor *sensor = &data->sensor;
	axera_thermal_toggle_sensor(sensor, false);
	data->disable_sensor(data);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int axera_thermal_suspend(struct device *dev)
{
	struct axera_thermal_data *data = dev_get_drvdata(dev);
	writel(0x0, data->regs + AX_ADC_CLK_EN);
	writel(0x0, data->regs + AX_THM_MON_EN);
	udelay(10);
	writel(((1 << 10) | (1 << 14)), common_sys_base + COMMON_SYS_CLK_EB1_CLR);
	return 0;
}

static int axera_thermal_resume(struct device *dev)
{
	struct axera_thermal_data *data = dev_get_drvdata(dev);
	writel(((1 << 10) | (1 << 14)), common_sys_base + COMMON_SYS_CLK_EB1_SET);
	udelay(10);
	writel(0x3, data->regs + AX_ADC_CLK_EN);
	writel(0x1, data->regs + AX_THM_MON_EN);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(axera_thermal_pm_ops,
			 axera_thermal_suspend, axera_thermal_resume);

static const struct of_device_id of_axera_thermal_match[] = {
	{
		.compatible = "axera,ax620e-tsensor",
		.data = ax620_thermal_probe
	},
	{ /* end */ }
};

static struct platform_driver axera_thermal_driver = {
	.driver = {
		.name = "ax_thermal",
		.pm = &axera_thermal_pm_ops,
		.of_match_table = of_axera_thermal_match,
	},
	.probe = axera_thermal_probe,
	.remove = axera_thermal_remove,
};

module_platform_driver(axera_thermal_driver);

MODULE_AUTHOR("Axera");
MODULE_DESCRIPTION("Axera thermal driver");
MODULE_LICENSE("GPL v2");
